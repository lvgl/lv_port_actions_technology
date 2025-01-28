/*
 * Copyright (c) 2020 Actions Corporation.
 * Author: Jinang Lv <lvjinang@actions-semi.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Actions USB OTG controller driver
 *
 * Actions USB OTG controller driver. The driver implements
 * the low level control routines to deal directly with the hardware.
 */

#include <kernel.h>
#include <sys/sys_io.h>
#include <board.h>
#include <init.h>
#include <drivers/dma.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/byteorder.h>
#include <devicetree.h>

#include <drivers/usb/usb_dc.h>
#include <drivers/usb/usb_hc.h>
#include <drivers/usb/usb_phy.h>
#include <soc_clock.h>
#include <soc_reset.h>
#include <soc_pmu.h>

#include "usb_otg_aotg.h"

#include <logging/log.h>
#define	LOG_LEVEL CONFIG_SYS_LOG_USB_OTG_DRIVER_LEVEL
LOG_MODULE_REGISTER(usb_otg_aotg);

#define	DMA_ID_USB0			4
#define DMA_IRQ_TC			(0) /* DMA completion flag */
#define DMA_IRQ_HF			(1) /* DMA half-full flag */

#define USB_AOTG_DMA_BURST8_LEN		32
#define USB_AOTG_DMA_MAX_SIZE		(128 * 1024)
#define USB_AOTG_DMA_MAX_LEN		(64 * 1024)

#ifdef CONFIG_USB_AOTG_UDC_DMA
#define USB_EP_OUT_DMA_CAP(ep_idx)	(ep_idx == USB_AOTG_OUT_EP_2)
#define USB_EP_IN_DMA_CAP(ep_idx)	(ep_idx == USB_AOTG_IN_EP_1)
#else
#define USB_EP_OUT_DMA_CAP(ep)		(false)
#define USB_EP_IN_DMA_CAP(ep)		(false)
#endif

/*
 * USB device controller endpoint private structure.
 */
struct aotg_dc_ep_ctrl_prv {
	uint8_t ep_ena;	/* Endpoint Enabled */
	uint8_t ep_type;
	uint16_t mps;	/* Endpoint Max packet size */
	usb_dc_ep_callback cb;	/* Endpoint callback function */
	/*
	 * "data_len" has different meanings in different cases.
	 *
	 * for ep0-in: set in setup phase.
	 * for ep0-out: set in setup or data phase.
	 * for epX-in legacy mode: not used.
	 * for epX-in new mode: set by upper layer, update in epin interrupt.
	 * for epX-in DMA: set by upper layer to help if need to set busy.
	 * for epX-out: set in epout interrupt.
	 * for epX-out async: set by upper layer, update in epout interrupt
	 * for epX-out DMA: set by upper layer.
	 */
	uint32_t data_len;
	uint32_t actual;	/* actual length: only for epX-out async */
	/*
	 * "data" has different meanings in different cases.
	 *
	 * for ep0-in: not used.
	 * for ep0-out: not used.
	 * for epX-in legacy mode: not used.
	 * for epX-in new mode: set by upper layer, update in epin interrupt.
	 * for epX-out: not used.
	 * for epX-out async: set by upper layer, update in epout interrupt.
	 */
	uint8_t *data;
	uint8_t multi;	/* multi-fifo */
};

/*
 * USB device controller private structure.
 */
struct usb_aotg_dc_prv {
	usb_dc_status_callback status_cb;
	struct aotg_dc_ep_ctrl_prv in_ep_ctrl[USB_AOTG_IN_EP_NUM+1];
	struct aotg_dc_ep_ctrl_prv out_ep_ctrl[USB_AOTG_OUT_EP_NUM+1];
	uint8_t attached;
	uint8_t phase;	/* Control transfer stage */
	uint8_t speed;
	union {
		uint8_t raw_setup[sizeof(struct usb_setup_packet)];
		struct usb_setup_packet setup;
	};
};

/*
 * USB host controller endpoint private structure.
 */
struct aotg_hc_ep_ctrl_prv {
	uint8_t ep_ena;	/* Endpoint Enabled */
	uint8_t ep_type;
	uint16_t mps;	/* Endpoint Max packet size */

	struct usb_request *urb;

	uint8_t ep_addr;	/* Endpoint Address */
	uint8_t err_count;
};

/*
 * USB host controller private structure.
 */
struct usb_aotg_hc_prv {
	uint32_t port;

	struct aotg_hc_ep_ctrl_prv in_ep_ctrl[USB_AOTG_IN_EP_NUM];
	struct aotg_hc_ep_ctrl_prv out_ep_ctrl[USB_AOTG_OUT_EP_NUM];
	uint8_t attached;
	uint8_t phase;	/* Control transfer stage */
	uint8_t speed;
};

struct usb_aotg_otg_prv {
	union {
#ifdef CONFIG_USB_AOTG_DC_ENABLED
		struct usb_aotg_dc_prv dc;
#endif
#ifdef CONFIG_USB_AOTG_HC_ENABLED
		struct usb_aotg_hc_prv hc;
#endif
	};
};

static struct usb_aotg_otg_prv usb_aotg;

static uint8_t usb_aotg_mode;

#ifdef CONFIG_USB_AOTG_DC_ENABLED
#define usb_aotg_dc	(usb_aotg.dc)
#endif
#ifdef CONFIG_USB_AOTG_HC_ENABLED
#define usb_aotg_hc	(usb_aotg.hc)
#endif

#ifdef CONFIG_USB_AOTG_UDC_DMA
struct usb_aotg_dma_prv {
	const struct device *dma_dev;
	int epin_dma;
	int epout_dma_single;
	int epout_dma_burst8;
	struct dma_config epin_dma_config;
	struct dma_config epout_dma_config_single;
	struct dma_config epout_dma_config_burst8;
	struct dma_block_config epin_dma_block;
	struct dma_block_config epout_dma_block_single;
	struct dma_block_config epout_dma_block_burst8;
};

static struct usb_aotg_dma_prv usb_aotg_dma;
#endif

/*
 * Dump aotg registers
 */
static inline void usb_aotg_reg_dump(void)
{
	LOG_DBG("USB AOTG registers:");

	LOG_DBG("IDVBUSCTRL: 0x%x", usb_read8(IDVBUSCTRL));
	LOG_DBG("DPDMCTRL: 0x%x", usb_read8(DPDMCTRL));
	LOG_DBG("LINESTATUS: 0x%x", usb_read8(LINESTATUS));
	LOG_DBG("USBIEN: 0x%x", usb_read8(USBIEN));
	LOG_DBG("OTGIEN: 0x%x", usb_read8(OTGIEN));
	LOG_DBG("USBEIEN: 0x%x", usb_read8(USBEIEN));
	LOG_DBG("USBIRQ: 0x%x", usb_read8(USBIRQ));
	LOG_DBG("OTGIRQ: 0x%x", usb_read8(OTGIRQ));
	LOG_DBG("USBEIRQ: 0x%x", usb_read8(USBEIRQ));
	LOG_DBG("OTGSTATE: 0x%x", usb_read8(OTGSTATE));
}

/*
 * Dump aotg endpoint registers
 */
static inline void usb_aotg_ep_reg_dump(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_ADDR2IDX(ep);

	ARG_UNUSED(ep_idx);

	LOG_DBG("USB AOTG EP 0x%x registers:", ep);

	if (USB_EP_DIR_IS_OUT(ep)) {
		LOG_DBG("HCINCTRL: 0x%x", usb_read8(HCINxCTRL(ep_idx)));
		LOG_DBG("STADDR: 0x%x", usb_read32(EPxOUT_STADDR(ep_idx)));
		LOG_DBG("OUT_CTRL: 0x%x", usb_read8(OUTxCTRL(ep_idx)));
		LOG_DBG("OUT_MAXPKT: 0x%x", usb_read16(OUTxMAXPKT(ep_idx)));
		LOG_DBG("OUT_IEN: 0x%x", usb_read8(OUTIEN));
		LOG_DBG("OUT_CS: 0x%x", usb_read8(OUTxCS(ep_idx)));
		LOG_DBG("OUT_SHTPKT: 0x%x", usb_read8(OUT_SHTPKT));
		LOG_DBG("OUT_BC: 0x%x", usb_read16(OUTxBC(ep_idx)));
		LOG_DBG("OUT_IRQ: 0x%x", usb_read8(OUTIRQ));
	} else {
		LOG_DBG("HCOUTCTRL: 0x%x", usb_read8(HCOUTxCTRL(ep_idx)));
		LOG_DBG("STADDR: 0x%x", usb_read32(EPxIN_STADDR(ep_idx)));
		LOG_DBG("IN_CTRL: 0x%x", usb_read8(INxCTRL(ep_idx)));
		LOG_DBG("IN_MAXPKT: 0x%x", usb_read16(INxMAXPKT(ep_idx)));
		LOG_DBG("IN_IEN: 0x%x", usb_read8(INIEN));
		LOG_DBG("IN_CS: 0x%x", usb_read8(INxCS(ep_idx)));
		LOG_DBG("IN_IRQ: 0x%x", usb_read8(INIRQ));
	}
}

/*
 * Check if the address of endpoint is valid
 *
 * NOTE: we don't check if the address of control endpoint is non-zero!
 */
static inline bool usb_aotg_ep_addr_valid(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_ADDR2IDX(ep);

	if (USB_EP_DIR_IS_OUT(ep) && (ep_idx < USB_AOTG_OUT_EP_NUM)) {
		return true;
	} else if (USB_EP_DIR_IS_IN(ep) && (ep_idx < USB_AOTG_IN_EP_NUM)) {
		return true;
	}

	return false;
}

/*
 * Enable USB controller clock
 */
static inline void usb_aotg_clock_enable(void)
{
	acts_clock_peripheral_enable(CLOCK_ID_USB);
}

/*
 * Disable USB controller clock
 */
static inline void usb_aotg_clock_disable(void)
{
	acts_clock_peripheral_disable(CLOCK_ID_USB);
}

/*
 * Reset USB controller
 */
static inline int usb_aotg_reset(void)
{
	acts_reset_peripheral(RESET_ID_USB2);
	acts_reset_peripheral(RESET_ID_USB);

	usb_aotg_reset_specific();

	/* Wait for USB controller until reset done */
	while (usb_read8(LINESTATUS) & BIT(OTGRESET)) {
		;
	}

	return 0;
}

/*
 * Enable controller
 */
static inline void usb_aotg_enable(void)
{
	usb_aotg_power_on();

	usb_aotg_clock_enable();

	usb_aotg_dpdm_init();

	usb_aotg_reset();
}

/*
 * Disable controller
 */
static inline void usb_aotg_disable(void)
{
	usb_aotg_disable_specific();

	usb_aotg_clock_disable();

	usb_aotg_power_off();
}

/*
 * Reset specific endpoint
 */
static inline int usb_aotg_ep_reset(uint8_t aotg_ep, uint8_t type)
{
	uint8_t ep_idx = USB_EP_ADDR2IDX(aotg_ep);

	if (!ep_idx) {
		LOG_DBG("Ep reset all endpoints");
	}

	/* Select specific endpoint */
	if (USB_EP_DIR_IS_OUT(aotg_ep)) {
		usb_write8(EPRST, ep_idx);
	} else {
		usb_write8(EPRST, ep_idx | BIT(EPRST_IO_IN));
	}

	if (type == USB_AOTG_EP_FIFO_RESET) {
		usb_set_bit8(EPRST, EPRST_FIFORST);
	} else if (type == USB_AOTG_EP_TOGGLE_RESET) {
		usb_set_bit8(EPRST, EPRST_TOGRST);
	} else if (type == USB_AOTG_EP_RESET) {
		usb_set_bit8(EPRST, EPRST_FIFORST);
		usb_set_bit8(EPRST, EPRST_TOGRST);
	} else {
		LOG_ERR("Ep reset type: %d", type);
		return -EINVAL;
	}

	return 0;
}

static inline int usb_aotg_exit(void)
{
	usb_aotg_enable();

	usb_aotg_disable();

	return 0;
}

/*
 * Write data to EPx FIFO
 *
 * @return  Actual length
 */
static inline int ep_write_fifo(uint8_t ep, const uint8_t *const data,
				uint32_t data_len)
{
	uint32_t i;
	uint32_t len;
	uint8_t left;
	uint8_t ep_idx = USB_EP_ADDR2IDX(ep);
	const uint8_t *buf = (const uint8_t *)data;

	uint8_t count = 0;

	if (USB_EP_IN_DMA_CAP(ep_idx)) {
		/* FIXME: wait 100us at most */
		while (usb_read8(INxCS(ep_idx)) & BIT(EPCS_BUSY)) {
			if (++count > 100) {
				break;
			}
			k_busy_wait(1);
		}
	}

	/* Check Busy */
	if (usb_read8(INxCS(ep_idx)) & BIT(EPCS_BUSY)) {
		LOG_DBG("EP 0x%x still busy", ep);
		return -EAGAIN;
	}

	if (USB_EP_IN_DMA_CAP(ep_idx)) {
		usb_write8(INIRQ, BIT(ep_idx));
		usb_set_bit8(INIEN, ep_idx);
	}

	/* 32-bit alignment */
	if (((long)buf & 0x3) == 0) {
		len = data_len >> 2;
		left = data_len & 0x3;
		for (i = 0; i < len; i++, buf += 4) {
			usb_write32(FIFOxDAT(ep_idx), *(uint32_t *)buf);
		}
	/* 16-bit alignment */
	} else if (((long)buf & 0x1) == 0) {
		len = data_len >> 1;
		left = data_len & 0x1;
		for (i = 0; i < len; i++, buf += 2) {
			usb_write16(FIFOxDAT(ep_idx), *(uint16_t *)buf);
		}
	} else {
		len = data_len;
		left = 0;
		for (i = 0; i < len; i++, buf++) {
			usb_write8(FIFOxDAT(ep_idx), *buf);
		}
	}

	/* Handle left byte(s) */
	for (i = 0; i < left; i++, buf++) {
		usb_write8(FIFOxDAT(ep_idx), *buf);
	}

	/* Set Byte Counter */
	usb_write16(INxBC(ep_idx), data_len);

	/* Set busy */
	usb_set_bit8(INxCS(ep_idx), EPCS_BUSY);

	return data_len;
}

/*
 * Read data from EP FIFO
 *
 * @return  Actual length
 */
static inline int ep_read_fifo(uint8_t ep, uint8_t *data, uint32_t data_len)
{
	uint32_t i;
	uint32_t len;
	uint8_t left;
	uint8_t ep_idx = USB_EP_ADDR2IDX(ep);
	uint32_t addr = FIFOxDAT(ep_idx);
	uint8_t *buf = (uint8_t *)data;

	/* Check Busy */
	if (usb_read8(OUTxCS(ep_idx)) & BIT(EPCS_BUSY)) {
		LOG_DBG("EP 0x%x still busy", ep);
		return -EBUSY;
	}

	/* 32-bit alignment */
	if (((long)buf & 0x3) == 0) {
		len = data_len >> 2;
		left = data_len & 0x3;
		for (i = 0; i < len; i++, buf += 4) {
			*(uint32_t *)buf = usb_read32(addr);
		}
	/* 16-bit alignment */
	} else if (((long)buf & 0x1) == 0) {
		len = data_len >> 1;
		left = data_len & 0x1;
		for (i = 0; i < len; i++, buf += 2) {
			*(uint16_t *)buf = usb_read16(addr);
		}
	} else {
		len = data_len;
		left = 0;
		for (i = 0; i < len; i++, buf++) {
			*buf = usb_read8(addr);
		}
	}

	/* Handle left byte(s) */
	for (i = 0; i < left; i++, buf++) {
		*buf = usb_read8(addr);
	}

	return data_len;
}

/*
 * Data stage: write data to EP0 FIFO
 *
 * @return  Actual length
 */
static inline int ep0_write_fifo(const uint8_t *const data, uint32_t data_len)
{
	uint32_t i;
	uint8_t j;
	uint32_t addr = EP0IN_FIFO;

	/* Check Busy */
	if (usb_read8(EP0CS) & BIT(EP0CS_INBUSY)) {
		LOG_DBG("still busy");
		return -EAGAIN;
	}

	/* 32-bit alignment */
	if (((long)data & 0x3) == 0) {
		for (i = 0; i < (data_len & ~0x3); i += 4) {
			usb_write32(addr + i, *(uint32_t *)(data + i));
		}
		for (j = 0; j < (data_len & 0x3); j++) {
			usb_write8(addr + i + j, *(data + i + j));
		}
	/* 16-bit alignment */
	} else if (((long)data & 0x1) == 0) {
		for (i = 0; i < (data_len & ~0x1); i += 2) {
			usb_write16(addr + i, *(uint16_t *)(data + i));
		}
		/* The last byte */
		if (data_len & 0x1) {
			usb_write8(addr + i, *(data + i));
		}
	} else {
		for (i = 0; i < data_len; i++) {
			usb_write8(addr + i, *(data + i));
		}
	}

	/* Set Byte Counter: set busy */
	usb_write8(IN0BC, data_len);

	return data_len;
}

/*
 * Data stage: read data from EP0 FIFO
 *
 * @return  Actual length
 */
static inline int ep0_read_fifo(uint8_t *data, uint32_t data_len)
{
	uint32_t i;
	uint8_t j;
	uint8_t len;
	uint32_t addr = EP0OUT_FIFO;

	/* Check Busy */
	if (usb_read8(EP0CS) & BIT(EP0CS_OUTBUSY)) {
		LOG_DBG("still busy");
		return -EBUSY;
	}

	/* Get Actual length */
	len = usb_read8(OUT0BC);
	if (data_len < len) {
		len = data_len;
	}

	/* 32-bit alignment */
	if (((long)data & 0x3) == 0) {
		for (i = 0; i < (len & ~0x3); i += 4) {
			*(uint32_t *)(data + i) = usb_read32(addr + i);
		}
		for (j = 0; j < (len & 0x3); j++) {
			*(data + i + j) = usb_read8(addr + i + j);
		}
	/* 16-bit alignment */
	} else if (((long)data & 0x1) == 0) {
		for (i = 0; i < (len & ~0x1); i += 2) {
			*(uint16_t *)(data + i) = usb_read16(addr + i);
		}
		/* The last byte */
		if (len & 0x1) {
			*(data + i) = usb_read8(addr + i);
		}
	} else {
		for (i = 0; i < len; i++) {
			*(data + i) = usb_read8(addr + i);
		}
	}

	return len;
}



/*
 * Device related
 */
#ifdef CONFIG_USB_AOTG_DC_ENABLED
/*
 * Check if the MaxPacketSize of endpoint is valid
 */
static inline bool aotg_dc_ep_mps_valid(uint16_t ep_mps,
				enum usb_dc_ep_type ep_type)
{
	enum usb_device_speed speed = usb_aotg_dc.speed;

	switch (ep_type) {
	case USB_DC_EP_CONTROL:
		return ep_mps <= USB_MAX_CTRL_MPS;

	case USB_DC_EP_BULK:
		if (speed == USB_SPEED_HIGH) {
			return ep_mps == USB_MAX_HS_BULK_MPS;
		} else {
			return ep_mps <= USB_MAX_FS_BULK_MPS;
		}

	case USB_DC_EP_INTERRUPT:
		if (speed == USB_SPEED_HIGH) {
			return ep_mps <= USB_MAX_HS_INTR_MPS;
		} else {
			return ep_mps <= USB_MAX_FS_INTR_MPS;
		}

	case USB_DC_EP_ISOCHRONOUS:
		if (speed == USB_SPEED_HIGH) {
			return ep_mps <= USB_MAX_HS_ISOC_MPS;
		} else {
			return ep_mps <= USB_MAX_FS_ISOC_MPS;
		}
	}

	return false;
}

/*
 * Check if the endpoint is enabled
 */
static inline bool aotg_dc_ep_is_enabled(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_ADDR2IDX(ep);

	if (USB_EP_DIR_IS_OUT(ep) && usb_aotg_dc.out_ep_ctrl[ep_idx].ep_ena) {
		return true;
	}
	if (USB_EP_DIR_IS_IN(ep) && usb_aotg_dc.in_ep_ctrl[ep_idx].ep_ena) {
		return true;
	}

	return false;
}

/*
 * Using single FIFO for every endpoint (except for ep0) by default
 */
static inline int aotg_dc_ep_alloc_fifo(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_ADDR2IDX(ep);

	if (!ep_idx) {
		return 0;
	}

	if (USB_EP_DIR_IS_OUT(ep)) {
		if (ep_idx >= USB_AOTG_OUT_EP_NUM) {
			return -EINVAL;
		}

		return aotg_dc_epout_alloc_fifo_specific(ep_idx);
	}

	if (ep_idx >= USB_AOTG_IN_EP_NUM) {
		return -EINVAL;
	}

	return aotg_dc_epin_alloc_fifo_specific(ep_idx);
}

/*
 * Max Packet Size Limit
 */
static inline int aotg_dc_set_max_packet(uint8_t ep, uint16_t ep_mps,
				enum usb_dc_ep_type ep_type)
{
	uint8_t ep_idx = USB_EP_ADDR2IDX(ep);

	/*
	 * NOTE: If the MaxPacketSize is beyond the limit, we won't try to
	 * assign a valid value, because it will fail anyway when the upper
	 * layer try to check capacity of the endpoint!
	 */
	if (!aotg_dc_ep_mps_valid(ep_mps, ep_type)) {
		return -EINVAL;
	}

	if (USB_EP_DIR_IS_OUT(ep)) {
		usb_write16(OUTxMAXPKT(ep_idx), ep_mps);
		usb_aotg_dc.out_ep_ctrl[ep_idx].mps = ep_mps;
	} else {
		usb_write16(INxMAXPKT(ep_idx), ep_mps);
		usb_aotg_dc.in_ep_ctrl[ep_idx].mps = ep_mps;
	}

	return 0;
}

/*
 * Endpoint type
 */
static inline void aotg_dc_set_ep_type(uint8_t ep, enum usb_dc_ep_type ep_type)
{
	uint8_t ep_idx = USB_EP_ADDR2IDX(ep);

	if (USB_EP_DIR_IS_OUT(ep)) {
		usb_aotg_dc.out_ep_ctrl[ep_idx].ep_type = ep_type;
	} else {
		usb_aotg_dc.in_ep_ctrl[ep_idx].ep_type = ep_type;
	}
}

/*
 * Initialize AOTG hardware endpoint.
 *
 * Set FIFO address, endpoint type, FIFO type, max packet size
 */
static inline int aotg_dc_ep_set(uint8_t ep, uint16_t ep_mps,
				enum usb_dc_ep_type ep_type)
{
	struct aotg_dc_ep_ctrl_prv *ep_ctrl;
	uint8_t ep_idx = USB_EP_ADDR2IDX(ep);
	int ret;

	LOG_DBG("Ep setup 0x%x, mps %d, type %d", ep, ep_mps, ep_type);

	/* Set FIFO address */
	ret = aotg_dc_ep_alloc_fifo(ep);
	if (ret) {
		return ret;
	}

	/* Set Max Packet */
	ret = aotg_dc_set_max_packet(ep, ep_mps, ep_type);
	if (ret) {
		return ret;
	}

	/* Set EP type */
	aotg_dc_set_ep_type(ep, ep_type);

	/* No need to set for endpoint 0 */
	if (!ep_idx) {
		return 0;
	}

	if (USB_EP_DIR_IS_OUT(ep)) {
		ep_ctrl = &usb_aotg_dc.out_ep_ctrl[ep_idx];
		if (ep_ctrl->multi) {
			/* Set endpoint type and FIFO type */
			usb_write8(OUTxCTRL(ep_idx),
				(ep_type << 2) | USB_AOTG_FIFO_DOUBLE);
			/* Set FIFOCTRL */
			usb_write8(FIFOCTRL, ep_idx | BIT(FIFOCTRL_AUTO));
		} else {
			usb_write8(OUTxCTRL(ep_idx),
				(ep_type << 2) | USB_AOTG_FIFO_SINGLE);
			/* Set FIFOCTRL */
			if (USB_EP_OUT_DMA_CAP(ep_idx)) {
				usb_write8(FIFOCTRL, ep_idx | BIT(FIFOCTRL_AUTO));
			} else {
				usb_write8(FIFOCTRL, ep_idx);
			}
		}
	} else {
		ep_ctrl = &usb_aotg_dc.in_ep_ctrl[ep_idx];
		if (ep_ctrl->multi) {
			usb_write8(INxCTRL(ep_idx),
				(ep_type << 2) | USB_AOTG_FIFO_DOUBLE);
		} else {
			usb_write8(INxCTRL(ep_idx),
				(ep_type << 2) | USB_AOTG_FIFO_SINGLE);
			if (USB_EP_IN_DMA_CAP(ep_idx)) {
				usb_write8(FIFOCTRL, ep_idx | BIT(FIFOCTRL_IO_IN) | BIT(FIFOCTRL_AUTO));
			} else {
				usb_write8(FIFOCTRL, ep_idx | BIT(FIFOCTRL_IO_IN));
			}
		}
	}

	return 0;
}

/*
 * Prepare for the next OUT transaction
 *
 * There are two ways: write OUTxBC or set busy for EPxCS.
 * I don't know which one is better really, just keep it both.
 */
static inline void aotg_dc_prep_rx(uint8_t ep_idx)
{
	/* Set busy */
	if (!ep_idx) {
#if 0
		usb_set_bit8(EP0CS, EP0CS_OUTBUSY);
#endif
		usb_write8(OUT0BC, 0x0);
	} else {
#if 0
		usb_write8(OUTxCS(ep_idx), 0);
#endif
		if (!(usb_read8(OUTxCS(ep_idx)) & BIT(EPCS_BUSY))) {
			usb_set_bit8(OUTxCS(ep_idx), EPCS_BUSY);
		}
	}
}

/*
 * Setup stage: read data from Setup FIFO
 */
static inline int ep0_read_setup(uint8_t *data, uint32_t data_len)
{
	uint8_t len = data_len;

	if (data_len > sizeof(struct usb_setup_packet)) {
		len = sizeof(struct usb_setup_packet);
	}

	memcpy(data, usb_aotg_dc.raw_setup, len);

	return len;
}

/*
 * Status stage
 */
static inline void handle_status(void)
{
	usb_set_bit8(EP0CS, EP0CS_NAK);
}

/*
 * Receive data from host: could be Setup, Data or Status
 *
 * @return  transfer length
 */
static inline int aotg_dc_rx(uint8_t ep, uint8_t *data, uint32_t data_len)
{
	int act_len = 0;

	/* Endpoint 0 */
	if (!USB_EP_ADDR2IDX(ep)) {
		if (usb_aotg_dc.phase == USB_AOTG_SETUP) {
			act_len = ep0_read_setup(data, data_len);
		} else if (usb_aotg_dc.phase == USB_AOTG_OUT_DATA) {
			act_len = ep0_read_fifo(data, data_len);
#if 0
		} else if (usb_aotg_dc.phase == USB_AOTG_OUT_STATUS) {
			/*
			 * Case 1: Setup, In Data, Out Status
			 */
			handle_status();
#endif
		}
	} else {
		act_len = ep_read_fifo(ep, data, data_len);
	}

	return act_len;
}

/*
 * Send data to host
 *
 * @return  transfer length
 */
static inline int aotg_dc_tx(uint8_t ep, const uint8_t *const data,
				uint32_t data_len)
{
	uint32_t act_len = 0;

	/* Endpoint 0 */
	if (!USB_EP_ADDR2IDX(ep)) {
		if ((usb_aotg_dc.phase == USB_AOTG_IN_DATA) && !data_len) {
			/*
			 * Handling zero-length packet for In-data phase
			 */
			act_len = ep0_write_fifo(data, data_len);
		} else if (!data_len) {
			/*
			 * Handling in-status stage
			 * Case 2: Setup, In Status
			 * Case 3: Setup, Out Data, In Status
			 */
			if (usb_aotg_dc.phase != USB_AOTG_SET_ADDRESS) {
				handle_status();
			}
			usb_aotg_dc.phase = USB_AOTG_IN_STATUS;
		} else {
			act_len = ep0_write_fifo(data, data_len);
			usb_aotg_dc.phase = USB_AOTG_IN_DATA;
		}
	} else {
		act_len = ep_write_fifo(ep, data, data_len);
	}

	return act_len;
}

static inline void aotg_dc_handle_otg(void)
{
	uint8_t tmp;
	uint8_t state;

	/* Clear OTG IRQ(s) */
	tmp = usb_read8(OTGIEN) & usb_read8(OTGIRQ);
	usb_write8(OTGIRQ, tmp);

	state = usb_read8(OTGSTATE);

	/* If AOTG enters b_peripheral state, enable Core interrupts */
	if (state == OTG_B_PERIPHERAL) {
#ifdef CONFIG_USB_AOTG_OTG_SUPPORT_HS
		usb_set_bit8(USBIEN, USBIEN_HS);
#endif
		usb_set_bit8(USBIEN, USBIEN_RESET);
		usb_set_bit8(USBIEN, USBIEN_SETUP);
	}

	LOG_DBG("State: 0x%x, irq: 0x%x", state, tmp);
}

static inline void aotg_dc_handle_reset(void)
{
	/* Clear USB Reset IRQ */
	usb_write8(USBIRQ, BIT(USBIRQ_RESET));

	usb_aotg_dc.speed = USB_SPEED_UNKNOWN;

	/* Clear USB SOF IRQ */
	usb_write8(USBIRQ, BIT(USBIRQ_SOF));
	/* Enable USB SOF IRQ */
	usb_set_bit8(USBIEN, USBIEN_SOF);

	LOG_DBG("");

#ifdef CONFIG_USB_AOTG_UDC_DMA
	dma_stop(usb_aotg_dma.dma_dev, usb_aotg_dma.epout_dma_single);
	dma_stop(usb_aotg_dma.dma_dev, usb_aotg_dma.epout_dma_burst8);
	dma_stop(usb_aotg_dma.dma_dev, usb_aotg_dma.epin_dma);
#endif

	/* Inform upper layers */
	if (usb_aotg_dc.status_cb) {
		usb_aotg_dc.status_cb(USB_DC_RESET, NULL);
	}
}

#ifdef CONFIG_USB_AOTG_OTG_SUPPORT_HS
static inline void aotg_dc_handle_hs(void)
{
	/* Clear USB high-speed IRQ */
	usb_write8(USBIRQ, BIT(USBIRQ_HS));

	usb_aotg_dc.speed = USB_SPEED_HIGH;

	LOG_DBG("");

	/* Inform upper layers */
	if (usb_aotg_dc.status_cb) {
		usb_aotg_dc.status_cb(USB_DC_HIGHSPEED, NULL);
	}
}
#endif

static inline void aotg_dc_handle_sof(void)
{
	/* Clear USB SOF IRQ */
	usb_write8(USBIRQ, BIT(USBIRQ_SOF));
	/* Disable USB SOF IRQ */
	usb_clear_bit8(USBIEN, USBIEN_SOF);

	if (usb_aotg_dc.speed == USB_SPEED_UNKNOWN) {
		usb_aotg_dc.speed = USB_SPEED_FULL;
	}

	LOG_DBG("");

	/* Inform upper layers */
	if (usb_aotg_dc.status_cb) {
		usb_aotg_dc.status_cb(USB_DC_SOF, NULL);
	}
}

static inline void aotg_dc_handle_resume(uint8_t usbeien)
{
	/* clear resume */
	usb_write8(USBEIRQ, BIT(USBEIRQ_RESUME) | usbeien);
	/* disable resume */
	usb_write8(USBEIEN, (~BIT(USBEIEN_RESUME)) & usbeien);
	if (usb_aotg_dc.status_cb) {
		usb_aotg_dc.status_cb(USB_DC_RESUME, NULL);
	}
}

static inline void aotg_dc_handle_suspend(uint8_t usbeien)
{
	usb_write8(USBIRQ, BIT(USBIRQ_SUSPEND));
	/* clear resume */
	usb_write8(USBEIRQ, BIT(USBEIRQ_RESUME) | usbeien);
	/* enable resume */
	usb_write8(USBEIEN, BIT(USBEIEN_RESUME) | usbeien);
	if (usb_aotg_dc.status_cb) {
		usb_aotg_dc.status_cb(USB_DC_SUSPEND, NULL);
	}
}

/*
 * Handle SETUP packet
 */
static inline void aotg_dc_handle_setup(void)
{
	usb_dc_ep_callback ep_cb;
	uint32_t addr = SETUP_FIFO;
	uint8_t i;

	/* Clear SETUP IRQ */
	usb_write8(USBIRQ, BIT(USBIRQ_SETUP));

	/* Fetch setup packet */
	for (i = 0; i < sizeof(struct usb_setup_packet); i++, addr++) {
		*(usb_aotg_dc.raw_setup + i) = usb_read8(addr);
	}

	/* Set setup data length */
	usb_aotg_dc.out_ep_ctrl[USB_AOTG_OUT_EP_0].data_len =
			sizeof(struct usb_setup_packet);

	/* Set in data length */
	if (USB_REQ_DIR_IN(usb_aotg_dc.setup.bmRequestType)) {
		usb_aotg_dc.in_ep_ctrl[USB_AOTG_IN_EP_0].data_len =
				usb_aotg_dc.setup.wLength;
	}

	usb_aotg_dc.phase = USB_AOTG_SETUP;

	ep_cb = usb_aotg_dc.out_ep_ctrl[USB_AOTG_OUT_EP_0].cb;
	/* Call the registered callback if any */
	if (ep_cb) {
		ep_cb(USB_AOTG_OUT_EP_0, USB_DC_EP_SETUP);
	}
}

/*
 * Handle EPxOUT data
 */
static inline void aotg_dc_handle_epout(uint8_t ep_idx)
{
	uint8_t ep = USB_EP_IDX2ADDR(ep_idx, USB_EP_DIR_OUT);
	struct aotg_dc_ep_ctrl_prv *ep_ctrl;
	uint16_t len;

	/* Clear EPxOUT IRQ */
	usb_write8(OUTIRQ, BIT(ep_idx));

	ep_ctrl = &usb_aotg_dc.out_ep_ctrl[ep_idx];

	/* Get Actual Length */
	if (!ep_idx) {
		/* Check phase before checking byte counter */
		if (usb_aotg_dc.phase == USB_AOTG_OUT_STATUS) {
			return;
		}
		/*
		 * EP0OUT data irq may come before EPxOUT token irq in
		 * out-status phase, filter it otherwise it will be disturbed.
		 * It may come even in in-data phase, can't explain!
		 *
		 * Keep it (only for record), we will check setup packet!
		 */
		if (usb_aotg_dc.phase == USB_AOTG_IN_DATA) {
			return;
		}
		/* Filter by direction and length of setup packet */
		if (USB_REQ_DIR_IN(usb_aotg_dc.setup.bmRequestType)) {
			return;
		} else if (usb_aotg_dc.setup.wLength == 0) {
			return;
		}

		/* NOTICE: OUT0BC will be set in setup phase and data phase */
		len = usb_read8(OUT0BC);
		if (len == 0) {
			return;
		}
		usb_aotg_dc.phase = USB_AOTG_OUT_DATA;
	} else {
		len = usb_read16(OUTxBC(ep_idx));
		if (len == 0) {
			LOG_DBG("ep 0x%x recv zero-length packet", ep);
		}

		if (ep_ctrl->data) {
			/* Disable interrupt if received all data to support auto-mode */
			if ((ep_ctrl->data_len == len) ||
			    (len < ep_ctrl->mps)) {
				usb_clear_bit8(OUTIEN, ep_idx);
				LOG_DBG("Disable irq");
			}

			/* FIXME: leave it to upper layer, should we clear fifo? */
			if (len > ep_ctrl->data_len) {
				LOG_DBG("babble: %d, %d", len, ep_ctrl->data_len);
				len = ep_ctrl->data_len;
			}

			ep_read_fifo(ep, ep_ctrl->data, len);
			ep_ctrl->actual += len;
			ep_ctrl->data_len -= len;
			ep_ctrl->data += len;

			if (ep_ctrl->data_len == 0) {
				LOG_DBG("0x%x done 0x%x", ep, usb_read8(OUTxCS(ep_idx)));
				ep_ctrl->data = NULL;
				goto done;
			} else if (len < ep_ctrl->mps) {
				LOG_DBG("0x%x short %d", ep, len);
				ep_ctrl->data = NULL;
				goto done;
			} else {
				/* read continue */
				if (!(usb_read8(OUTxCS(ep_idx)) & BIT(EPCS_BUSY))) {
					usb_set_bit8(OUTxCS(ep_idx), EPCS_BUSY);
				}
				return;
			}
		}
	}

	ep_ctrl->data_len = len;
done:
	/* Call the registered callback if any */
	if (ep_ctrl->cb) {
		ep_ctrl->cb(ep, USB_DC_EP_DATA_OUT);
	}
}

/*
 * Handle EPxIN data
 */
static inline void aotg_dc_handle_epin(uint8_t ep_idx)
{
	struct aotg_dc_ep_ctrl_prv *ep_ctrl;
	uint32_t len;
	uint8_t ep;

	/* Clear EPxIN IRQ */
	usb_write8(INIRQ, BIT(ep_idx));

	ep_ctrl = &usb_aotg_dc.in_ep_ctrl[ep_idx];
	ep = USB_EP_IDX2ADDR(ep_idx, USB_EP_DIR_IN);

	if (ep_idx == 0) {
		goto done;
	}

	/* NOTICE: should never happen! */
	if (ep_ctrl->data == NULL) {
		LOG_DBG("0x%x", ep_idx);
		goto done;
	}

	if (ep_ctrl->data_len == 0) {
		LOG_DBG("0x%x done", ep);
		ep_ctrl->data = NULL;
		goto done;
	}

	if (ep_ctrl->data_len > ep_ctrl->mps) {
		len = ep_ctrl->mps;
	} else {
		len = ep_ctrl->data_len;
	}

	ep_write_fifo(ep, ep_ctrl->data, len);

	ep_ctrl->data_len -= len;
	ep_ctrl->data += len;

	return;
done:
	/* Call the registered callback if any */
	if (ep_ctrl->cb) {
		ep_ctrl->cb(ep, USB_DC_EP_DATA_IN);
	}
}

/*
 * Handle EPxIN empty
 */
static inline void aotg_dc_handle_epin_empty(void)
{
	uint8_t ien = (usb_read8(INEMPTY_IEN) & INEMPTY_IEN_MASK) >> INEMPTY_IEN_SHIFT;
	uint8_t irq = (usb_read8(INEMPTY_IRQ) & INEMPTY_IRQ_MASK) >> INEMPTY_IRQ_SHIFT;
	uint8_t ep_msk = ien & irq;
	usb_dc_ep_callback ep_cb;
	uint8_t ep_idx, ep;
	uint8_t i;

	/* clear all pendings */
	usb_write8(INEMPTY_IRQ, INEMPTY_IRQ_MASK);
	/* disable all interrupts */
	usb_write8(INEMPTY_IEN, 0);

	for (i = 0; i < USB_AOTG_IN_EP_NUM; i++) {
		if (ep_msk & BIT(i)) {
			ep_idx = INEMPTY_IRQ2ADDR(i) & USB_EP_NUM_MASK;
			ep = USB_EP_IDX2ADDR(ep_idx, USB_EP_DIR_IN);

			LOG_DBG("ep: 0x%x", ep);

			ep_cb = usb_aotg_dc.in_ep_ctrl[ep_idx].cb;
			/* Call the registered callback if any */
			if (ep_cb) {
				ep_cb(ep, USB_DC_EP_DATA_IN);
			}
		}
	}
}

static inline void aotg_dc_handle_out_token_internal(uint8_t ep_idx)
{
	if (ep_idx == USB_AOTG_OUT_EP_0) {
		/*
		 * We may get one or more OUT token(s) even though
		 * we have set ACK bit already, filter the case!
		 */
		if (usb_aotg_dc.phase == USB_AOTG_OUT_STATUS) {
			return;
		}

		/*
		 * Case 1: Setup, In Data, Out Status
		 *
		 * Status stage: no need to check busy
		 */
		if (usb_aotg_dc.phase == USB_AOTG_IN_DATA) {
			handle_status();
			usb_aotg_dc.phase = USB_AOTG_OUT_STATUS;
			return;
		}
	}

	/* Do nothing */
	return;

	/*
	 * NOTE: out token only take cares of out status phase, don't
	 * use it for out data phase, because ep0 may be busy all the
	 * time and can never complete!
	 */
#if 0
	usb_dc_ep_callback ep_cb;

	while (usb_read8(EP0CS) & BIT(EP0CS_OUTBUSY)) {
		;
	}

	usb_aotg_dc.out_ep_ctrl[ep_idx].data_len = usb_read8(OUT0BC);
	usb_aotg_dc.phase = USB_AOTG_OUT_DATA;

	ep_cb = usb_aotg_dc.out_ep_ctrl[ep_idx].cb;
	ep_idx = USB_EP_IDX2ADDR(ep_idx, USB_EP_DIR_OUT);
	/* Call the registered callback if any */
	if (ep_cb) {
		ep_cb(ep_idx, USB_DC_EP_DATA_OUT);
	}
#endif
}

/*
 * Handle EPxOUT Token
 */
static inline void aotg_dc_handle_out_token(void)
{
	uint8_t ep_msk, i;

	/* Get endpoint mask */
	ep_msk = (usb_read8(OUT_TOKIRQ) & usb_read8(OUT_TOKIEN));

	/* NOTE: There may be more than one out token irqs simutaneously */
	for (i = 0; i < EPOUT_TOKEN_NUM; i++) {
		if (ep_msk & BIT(i)) {
			/* Clear IRQ */
			usb_write8(OUT_TOKIRQ, BIT(i));

			aotg_dc_handle_out_token_internal(i);
		}
	}
}

/*
 * Handle EPxIN Token
 */
static inline void aotg_dc_handle_in_token(void)
{
	struct aotg_dc_ep_ctrl_prv *ep_ctrl;
	uint8_t ep_msk, i;

	/* Get endpoint mask */
	ep_msk = usb_read8(IN_TOKIRQ) & usb_read8(IN_TOKIEN);

	/* NOTE: There may be more than one in token irqs simutaneously */
	for (i = 0; i < EPIN_TOKEN_NUM; i++) {
		if (ep_msk & BIT(i)) {
			/* Clear IRQ */
			usb_write8(IN_TOKIRQ, BIT(i));

			ep_ctrl = &usb_aotg_dc.in_ep_ctrl[i];
			/* Call the registered callback if any */
			if (ep_ctrl->cb) {
				ep_ctrl->cb(i, USB_DC_EP_DATA_IN);
			}
		}
	}
}

/*
 * Handle EPxOUT Short Packet
 */
static inline void aotg_dc_handle_short_packet(void)
{
	usb_write8(OUT_SHTPKT, usb_read8(OUT_SHTPKT));

	LOG_DBG("aotg short packet");
}

static inline void aotg_dc_isr_dispatch(uint8_t vector)
{
	uint8_t usbeien = usb_read8(USBEIEN) & USBEIEN_MASK;

	/* USB Resume */
	if (vector == UIV_RESUME) {
		aotg_dc_handle_resume(usbeien);
		return;
	}

	/*
	 * Make sure external IRQ has been cleared right!
	 *
	 * TODO: If two or more IRQs are pending simultaneously,
	 * EIRQ will be pending immediately after cleared or not?
	 */
	while (usb_read8(USBEIRQ) & BIT(USBEIRQ_EXTERN)) {
		usb_write8(USBEIRQ, BIT(USBEIRQ_EXTERN) | usbeien);
	}

	switch (vector) {
	/* OTG */
	case UIV_OTGIRQ:
		aotg_dc_handle_otg();
		break;

	/* USB Reset */
	case UIV_USBRST:
		aotg_dc_handle_reset();
		break;

#ifdef CONFIG_USB_AOTG_OTG_SUPPORT_HS
	/* High-speed */
	case UIV_HSPEED:
		aotg_dc_handle_hs();
		break;
#endif

	/* SOF */
	case UIV_SOF:
		aotg_dc_handle_sof();
		break;

	/* USB Suspend */
	case UIV_SUSPEND:
		aotg_dc_handle_suspend(usbeien);
		break;

	/* SETUP */
	case UIV_SUDAV:
		aotg_dc_handle_setup();
		break;

	/* EPxOUT */
	case UIV_EP0OUT:
	case UIV_EP1OUT:
	case UIV_EP2OUT:
	case UIV_EP3OUT:
#if (CONFIG_USB_AOTG_OTG_VERSION == USB_AOTG_VERSION_LEOPARD)
	case UIV_EP4OUT:
	case UIV_EP5OUT:
	case UIV_EP6OUT:
#endif
		aotg_dc_handle_epout(UIV_EPOUT_VEC2ADDR(vector));
		break;

	/* EPxIN */
	case UIV_EP0IN:
	case UIV_EP1IN:
	case UIV_EP2IN:
	case UIV_EP3IN:
#if (CONFIG_USB_AOTG_OTG_VERSION == USB_AOTG_VERSION_LEOPARD)
	case UIV_EP4IN:
	case UIV_EP5IN:
	case UIV_EP6IN:
#endif
		aotg_dc_handle_epin(UIV_EPIN_VEC2ADDR(vector));
		break;

	/* EPxIN empty */
	case UIV_HCOUTEMPTY:
		aotg_dc_handle_epin_empty();
		break;

	/*
	 * Using OUT Token for EP0OUT data transfer for assistance.
	 *
	 * AOTG handles in/out status by software (handle_status()),
	 * which means it depends on USB core totally. But USB core
	 * thinks that controller should take care of out status by
	 * itself (excluding int status phase). That means EP0 Data
	 * IRQ will never be pending until handle_status() called!
	 */
	case UIV_OUTTOKEN:
		aotg_dc_handle_out_token();
		break;

	case UIV_INTOKEN:
		aotg_dc_handle_in_token();
		break;

	/* Short Packet */
	case UIV_EPOUTSHTPKT:
		aotg_dc_handle_short_packet();
		break;

	default:
		break;
	}
}

/* Disable soft disconnect */
static inline void aotg_dc_connect(void)
{
	usb_clear_bit8(USBCS, USBCS_DISCONN);
}

/* Enable soft disconnect */
static inline void aotg_dc_disconnect(void)
{
	usb_set_bit8(USBCS, USBCS_DISCONN);
}

static void usb_dc_hardware_init(void)
{
	usb_write32(AVDDLDO_CTL, 0x05);
	k_msleep(1);
	usb_write32(SPLL_CTL, sys_read32(SPLL_CTL)|0x01);
	k_msleep(1);
	LOG_DBG("SPLL_CTL: 0x%x", sys_read32(SPLL_CTL));

	acts_clock_peripheral_enable(CLOCK_ID_USB);
	acts_clock_peripheral_enable(CLOCK_ID_USB2);
	LOG_DBG("CMU_DEVCLKEN0: 0x%x", sys_read32(CMU_DEVCLKEN0));
}

static void usb_hc_hardware_init(void)
{
	usb_write32(AVDDLDO_CTL, 0x05);
	k_msleep(1);
	usb_write32(SPLL_CTL, sys_read32(SPLL_CTL)|0x01);
	k_msleep(1);
	LOG_DBG("SPLL_CTL: 0x%x", sys_read32(SPLL_CTL));

	acts_clock_peripheral_enable(CLOCK_ID_USB);
	acts_clock_peripheral_enable(CLOCK_ID_USB2);
	LOG_DBG("CMU_DEVCLKEN0: 0x%x", sys_read32(CMU_DEVCLKEN0));
}

int usb_dc_attach(void)
{
	if (usb_aotg_dc.attached) {
		LOG_DBG("already");
		return 0;
	}

	usb_dc_hardware_init();

	aotg_dc_fifo_enable();

	usb_aotg_enable();

	usb_write8(IDVBUSCTRL, IDVBUS_DEVICE);
	usb_write8(DPDMCTRL, DPDM_HOST);

	aotg_dc_phy_init();

#ifdef CONFIG_USB_AOTG_DC_FS
	aotg_dc_force_fs();
#endif

	/* Enable OTG(b_peripheral) interrupt */
	usb_set_bit8(OTGIEN, OTGIEN_PERIPHERAL);
	/* Enable external interrupt */
	usb_set_bit8(USBEIEN, USBEIEN_EXTERN);

	irq_enable(USB_AOTG_IRQ);

	aotg_dc_connect();

	usb_aotg_reg_dump();

	/* status_cb is registered */
	usb_aotg_dc.speed = USB_SPEED_UNKNOWN;
	usb_aotg_dc.attached = 1;

	LOG_DBG("");

	return 0;
}

int usb_dc_detach(void)
{
	unsigned int key = irq_lock();
	uint8_t speed;

	if (!usb_aotg_dc.attached) {
		irq_unlock(key);
		LOG_DBG("already");
		return 0;
	}

	irq_disable(USB_AOTG_IRQ);

	aotg_dc_disconnect();

#ifdef CONFIG_USB_AOTG_UDC_DMA
	dma_stop(usb_aotg_dma.dma_dev, usb_aotg_dma.epout_dma_single);
	dma_stop(usb_aotg_dma.dma_dev, usb_aotg_dma.epout_dma_burst8);
	dma_stop(usb_aotg_dma.dma_dev, usb_aotg_dma.epin_dma);
#endif

	/* recover speed */
	speed = usb_aotg_dc.speed;
	memset(&usb_aotg_dc, 0, sizeof(usb_aotg_dc));
	usb_aotg_dc.speed = speed;

	usb_aotg_reset();

	usb_aotg_disable();

	aotg_dc_fifo_disable();

	irq_unlock(key);

	LOG_DBG("");

	return 0;
}

int usb_dc_reset(void)
{
	LOG_DBG("");

	/* Clear private data */
	memset(&usb_aotg_dc, 0, sizeof(usb_aotg_dc));

	return 0;
}

int usb_dc_set_address(const uint8_t addr)
{
	if (addr > USB_AOTG_MAX_ADDR) {
		return -EINVAL;
	}

	/* Respond "Set Address" automatically in device mode */
	LOG_DBG("Set Address: %d", usb_read8(FNADDR));

	/* Set Address phase */
	usb_aotg_dc.phase = USB_AOTG_SET_ADDRESS;

	/* Enable suspend */
	usb_write8(USBIRQ, BIT(USBIRQ_SUSPEND));
	usb_set_bit8(USBIEN, USBIEN_SUSPEND);

	return 0;
}

int usb_dc_set_status_callback(const usb_dc_status_callback cb)
{
	usb_aotg_dc.status_cb = cb;

	return 0;
}

int usb_dc_ep_check_cap(const struct usb_dc_ep_cfg_data * const cfg)
{
	LOG_DBG("ep %x, mps %d, type %d", cfg->ep_addr, cfg->ep_mps,
			cfg->ep_type);

	/*
	 * Check if the address of control endpoint is non-zero!
	 */
	if ((cfg->ep_type == USB_DC_EP_CONTROL)
	    && USB_EP_ADDR2IDX(cfg->ep_addr)) {
		LOG_ERR("invalid endpoint configuration");
		return -EINVAL;
	}

	if (!aotg_dc_ep_mps_valid(cfg->ep_mps, cfg->ep_type))  {
		LOG_WRN("unsupported packet size");
		return -EINVAL;
	}

	if (!usb_aotg_ep_addr_valid(cfg->ep_addr)) {
		LOG_WRN("endpoint 0x%x address out of range", cfg->ep_addr);
		return -EINVAL;
	}

	return 0;
}

int usb_dc_ep_configure(const struct usb_dc_ep_cfg_data * const ep_cfg)
{
	if (!usb_aotg_dc.attached || !usb_aotg_ep_addr_valid(ep_cfg->ep_addr))
		return -EINVAL;

	return aotg_dc_ep_set(ep_cfg->ep_addr, ep_cfg->ep_mps,
				ep_cfg->ep_type);
}

int usb_dc_ep_set_stall(const uint8_t ep)
{
	uint8_t ep_idx = USB_EP_ADDR2IDX(ep);
	unsigned int key;

	if (!usb_aotg_ep_addr_valid(ep)) {
		return -EINVAL;
	}

	key = irq_lock();
	if (!usb_aotg_dc.attached) {
		irq_unlock(key);
		return -ENODEV;
	}

	/* Endpoint 0 */
	if (!ep_idx) {
		usb_set_bit8(EP0CS, EP0CS_STALL);
		irq_unlock(key);
		return 0;
	}

	if (USB_EP_DIR_IS_OUT(ep)) {
		usb_set_bit8(OUTxCTRL(ep_idx), EPCTRL_STALL);
		/* clear pending in case data received before stalled */
		usb_write8(OUTIRQ, BIT(ep_idx));
	} else {
		usb_set_bit8(INxCTRL(ep_idx), EPCTRL_STALL);
	}

	/* Reset the data toggle */
	usb_aotg_ep_reset(ep, USB_AOTG_EP_TOGGLE_RESET);

	irq_unlock(key);
	return 0;
}

int usb_dc_ep_clear_stall(const uint8_t ep)
{
	uint8_t ep_idx = USB_EP_ADDR2IDX(ep);
	unsigned int key;

	if (!usb_aotg_ep_addr_valid(ep)) {
		return -EINVAL;
	}

	key = irq_lock();
	if (!usb_aotg_dc.attached) {
		irq_unlock(key);
		return -ENODEV;
	}

	if (!ep_idx) {
		/*
		 * case 1: EP0 stall will be cleared when SETUP token comes
		 * case 2: we can clear EP0CS_STALL bit to clear stall
		 */
		usb_clear_bit8(EP0CS, EP0CS_STALL);
		irq_unlock(key);
		return 0;
	}

	if (USB_EP_DIR_IS_OUT(ep)) {
		usb_clear_bit8(OUTxCTRL(ep_idx), EPCTRL_STALL);
	} else {
		usb_clear_bit8(INxCTRL(ep_idx), EPCTRL_STALL);
	}

	irq_unlock(key);
	return 0;
}

int usb_dc_ep_is_stalled(const uint8_t ep, uint8_t *const stalled)
{
	uint8_t ep_idx = USB_EP_ADDR2IDX(ep);
	unsigned int key;

	if (!usb_aotg_ep_addr_valid(ep)) {
		return -EINVAL;
	}

	if (!stalled) {
		return -EINVAL;
	}

	key = irq_lock();
	if (!usb_aotg_dc.attached) {
		irq_unlock(key);
		return -ENODEV;
	}

	*stalled = 0;

	/* Endpoint 0 */
	if (!ep_idx && (usb_read8(EP0CS) & BIT(EP0CS_STALL))) {
		*stalled = 1;
		irq_unlock(key);
		return 0;
	}

	if (USB_EP_DIR_IS_OUT(ep)) {
		if (usb_read8(OUTxCTRL(ep_idx)) & BIT(EPCTRL_STALL)) {
			*stalled = 1;
		}
	} else {
		if (usb_read8(INxCTRL(ep_idx)) & BIT(EPCTRL_STALL)) {
			*stalled = 1;
		}
	}

	irq_unlock(key);
	return 0;
}

int usb_dc_ep_halt(const uint8_t ep)
{
	/*
	 * Just set stall, by the way no clear halt operation?
	 */
	return usb_dc_ep_set_stall(ep);
}

int usb_dc_ep_enable(const uint8_t ep)
{
	uint8_t ep_idx = USB_EP_ADDR2IDX(ep);
	bool dir_out = USB_EP_DIR_IS_OUT(ep);

	if (!usb_aotg_dc.attached || !usb_aotg_ep_addr_valid(ep)) {
		return -EINVAL;
	}

	if (usb_aotg_ep_reset(ep, USB_AOTG_EP_RESET)) {
		return -EINVAL;
	}

	/* Enable EP interrupts */
	if (dir_out) {
		/* EP0 OUT uses OUT Token IRQ for assistance */
		if (!ep_idx) {
			usb_set_bit8(OUT_TOKIEN, EP0OUT_TOKEN);
		}
		if (!USB_EP_OUT_DMA_CAP(ep_idx)) {
			usb_set_bit8(OUTIEN, ep_idx);
		}
		usb_aotg_dc.out_ep_ctrl[ep_idx].ep_ena = 1;
	} else {
#if 0
		/* Using In Token IRQ for isochronous transfer */
		if (ep_type_isoc(usb_aotg_dc.in_ep_ctrl[ep_idx].ep_type)) {
			usb_set_bit8(IN_TOKIEN, ep_idx);
		} else {
			usb_set_bit8(INIEN, ep_idx);
		}
#else
		if (!USB_EP_IN_DMA_CAP(ep_idx)) {
			usb_set_bit8(INIEN, ep_idx);
		}
#endif
		usb_aotg_dc.in_ep_ctrl[ep_idx].ep_ena = 1;
	}

	/* Endpoint 0 */
	if (!ep_idx) {
		return 0;
	}

	if (dir_out) {
		/* Activate Ep */
		usb_set_bit8(OUTxCTRL(ep_idx), EPCTRL_VALID);
		/* Short Packet */
		/* usb_set_bit8(OUT_SHTPKT, ep_idx); */
	} else {
		usb_set_bit8(INxCTRL(ep_idx), EPCTRL_VALID);
	}

	/* Prepare EP for rx */
	if (dir_out) {
		aotg_dc_prep_rx(ep_idx);
	}

	usb_aotg_ep_reg_dump(ep);

	return 0;
}

int usb_dc_ep_disable(const uint8_t ep)
{
	uint8_t ep_idx = USB_EP_ADDR2IDX(ep);
	bool dir_out = USB_EP_DIR_IS_OUT(ep);

	if (!usb_aotg_dc.attached || !usb_aotg_ep_addr_valid(ep)) {
		return -EINVAL;
	}

	/* Disable EP interrupts */
	if (dir_out) {
		if (!ep_idx) {
			usb_clear_bit8(OUT_TOKIEN, EP0OUT_TOKEN);
		}
		usb_clear_bit8(OUTIEN, ep_idx);
		usb_aotg_dc.out_ep_ctrl[ep_idx].ep_ena = 0;
	} else {
		usb_clear_bit8(INIEN, ep_idx);
		usb_aotg_dc.in_ep_ctrl[ep_idx].ep_ena = 0;
	}

	/* Endpoint 0 */
	if (!ep_idx) {
		return 0;
	}

	/* De-activate Ep */
	if (dir_out) {
		usb_clear_bit8(OUTxCTRL(ep_idx), EPCTRL_VALID);
	} else {
		usb_clear_bit8(INxCTRL(ep_idx), EPCTRL_VALID);
	}

	return 0;
}

int usb_dc_ep_flush(const uint8_t ep)
{
	unsigned int key;
	int ret;

	if (!usb_aotg_ep_addr_valid(ep)) {
		return -EINVAL;
	}

	key = irq_lock();
	if (!usb_aotg_dc.attached) {
		irq_unlock(key);
		return -ENODEV;
	}

	ret = usb_aotg_ep_reset(ep, USB_AOTG_EP_FIFO_RESET);
	irq_unlock(key);

	return ret;
}

/*
 * legacy mode: write mps at most and depends on upper layer to write again
 *              if data_len >= mps for all out endpoints.
 * new mode: write once (controller will handle if data_len >= mps) for all
 *           non-control endpoints, control endpoints still works in legacy
 *           mode.
 */
int usb_dc_ep_write(const uint8_t ep, const uint8_t *const data,
				const uint32_t data_len, uint32_t * const ret_bytes)
{
	struct aotg_dc_ep_ctrl_prv *ep_ctrl;
	uint8_t ep_idx = USB_EP_ADDR2IDX(ep);
	unsigned int key;
	int ret;
	uint32_t len;

	if (!usb_aotg_ep_addr_valid(ep)) {
		LOG_ERR("No valid endpoint");
		return -EINVAL;
	}

	/* Check if IN ep */
	if (!USB_EP_DIR_IS_IN(ep)) {
		LOG_ERR("Wrong endpoint direction");
		return -EINVAL;
	}

	/* Check if ep enabled */
	if (!aotg_dc_ep_is_enabled(ep)) {
		LOG_ERR("Not enabled endpoint");
		return -EINVAL;
	}

	key = irq_lock();
	if (!usb_aotg_dc.attached) {
		irq_unlock(key);
		return -ENODEV;
	}

	ep_ctrl = &usb_aotg_dc.in_ep_ctrl[ep_idx];

#ifdef CONFIG_USB_AOTG_UDC_DMA
	/* need 32-bit alignment */
	if (((long)data & 0x3) != 0) {
		goto cpu_write;
	}

	if (USB_EP_IN_DMA_CAP(ep_idx)) {
		if ((data_len < USB_AOTG_DMA_BURST8_LEN) ||
		    (data_len % USB_AOTG_DMA_BURST8_LEN)) {
			goto cpu_write;
		}
		if (data_len >= USB_AOTG_DMA_MAX_SIZE) {
			len = USB_AOTG_DMA_MAX_LEN;
		} else {
			len = data_len;
		}

		ep_ctrl->data_len = len;

		/* Disable EP interrupt */
		usb_clear_bit8(INIEN, ep_idx);

		usb_write8(IN1_DMACTL, BIT(DMACTL_FIFORST));
		usb_write8(IN1_DMACTL, 0);
		usb_write8(IN1_DMALEN1L, (uint8_t)len);
		usb_write8(IN1_DMALEN1M, (uint8_t)(len >> 8));
		usb_write8(IN1_DMALEN1H, (uint8_t)(len >> 16) | (0x1 << 1));
		usb_write8(IN1_DMALEN2H, 0);
		usb_write8(IN1_DMACTL, BIT(DMACTL_START));

		dma_reload(usb_aotg_dma.dma_dev, usb_aotg_dma.epin_dma,
			(uint32_t)data, FIFO1DAT, len);
		dma_start(usb_aotg_dma.dma_dev, usb_aotg_dma.epin_dma);

		if (ret_bytes) {
			*ret_bytes = len;
		}

		irq_unlock(key);
		return 0;
	}
cpu_write:
#endif
	ep_ctrl->data = NULL;

	len = data_len > ep_ctrl->mps ? ep_ctrl->mps : data_len;

	if (ep_idx != 0) {
		goto new_mode;
	}

	ret = aotg_dc_tx(ep, data, len);
	if (ret < 0) {
		irq_unlock(key);
		return ret;
	}

	if (ret_bytes) {
		*ret_bytes = ret;
	}

	if (usb_aotg_dc.phase != USB_AOTG_IN_DATA) {
		irq_unlock(key);
		return 0;
	}

	usb_aotg_dc.in_ep_ctrl[ep_idx].data_len -= ret;
	if ((usb_aotg_dc.in_ep_ctrl[ep_idx].data_len == 0) ||
	    (ret < MAX_PACKET_SIZE0)) {
		/* Case 1: Setup, In Data, Out Status */
		handle_status();
		usb_aotg_dc.phase = USB_AOTG_OUT_STATUS;
	}

	irq_unlock(key);
	return 0;
new_mode:
	LOG_DBG("%d", data_len);

	ep_ctrl->data_len = data_len - len;
	ep_ctrl->data = (uint8_t *)data + len;

	ret = ep_write_fifo(ep, data, len);
	if (ret < 0) {
		irq_unlock(key);
		return ret;
	}

	if (ret_bytes) {
		*ret_bytes = data_len;
	}

	irq_unlock(key);
	return 0;
}

int usb_dc_ep_read(const uint8_t ep, uint8_t *const data,
		const uint32_t max_data_len, uint32_t * const read_bytes)
{
	if (usb_dc_ep_read_wait(ep, data, max_data_len, read_bytes) != 0) {
		return -EINVAL;
	}

	if (!data && !max_data_len) {
		/* When both buffer and max data to read are zero the above
		 * call would fetch the data len and we simply return.
		 */
		return 0;
	}

	if (usb_dc_ep_read_continue(ep) != 0) {
		return -EINVAL;
	}

	return 0;
}

int usb_dc_ep_set_callback(const uint8_t ep, const usb_dc_ep_callback cb)
{
	uint8_t ep_idx = USB_EP_ADDR2IDX(ep);

	if (!usb_aotg_dc.attached || !usb_aotg_ep_addr_valid(ep)) {
		return -EINVAL;
	}

	if (USB_EP_DIR_IS_IN(ep)) {
		usb_aotg_dc.in_ep_ctrl[ep_idx].cb = cb;
	} else {
		usb_aotg_dc.out_ep_ctrl[ep_idx].cb = cb;
	}

	return 0;
}

/*
 * read() and read_async() co-exist, which means fisrt read_async() should be
 * called ASAP to make sure outirq is after read_async(). It is recommended
 * called in status_cb().
 */
int usb_dc_ep_read_async(uint8_t ep, uint8_t *data, uint32_t max_data_len,
				uint32_t *read_bytes)
{
	struct aotg_dc_ep_ctrl_prv *ep_ctrl;
	uint8_t ep_idx = USB_EP_ADDR2IDX(ep);
	unsigned int key;

	if (!usb_aotg_ep_addr_valid(ep)) {
		LOG_ERR("No valid endpoint");
		return -EINVAL;
	}

	/* Check if OUT ep */
	if (!USB_EP_DIR_IS_OUT(ep)) {
		LOG_ERR("Wrong endpoint direction");
		return -EINVAL;
	}

	/* Allow to read 0 bytes */
	if (!data && max_data_len) {
		LOG_ERR("Wrong arguments");
		return -EINVAL;
	}

	/* Check if ep enabled */
	if (!aotg_dc_ep_is_enabled(ep)) {
		LOG_ERR("Not enabled endpoint");
		return -EINVAL;
	}

	key = irq_lock();
	if (!usb_aotg_dc.attached) {
		irq_unlock(key);
		return -ENODEV;
	}

	ep_ctrl = &usb_aotg_dc.out_ep_ctrl[ep_idx];

#ifdef CONFIG_USB_AOTG_UDC_DMA

	/* need 32-bit alignment */
	if (((long)data & 0x3) != 0) {
		goto cpu_mode;
	}

	if (!USB_EP_OUT_DMA_CAP(ep_idx)) {
		LOG_DBG("for DMA only");
		goto cpu_mode;
	}

	if (max_data_len >= USB_AOTG_DMA_MAX_SIZE) {
		max_data_len = USB_AOTG_DMA_MAX_LEN;
	}

	ep_ctrl->data_len = max_data_len;
	ep_ctrl->actual = 0;

	usb_write8(OUT2_DMACTL, BIT(DMACTL_FIFORST));
	usb_write8(OUT2_DMACTL, 0);
	usb_write8(OUT2_DMALENL, (uint8_t)max_data_len);
	usb_write8(OUT2_DMALENM, (uint8_t)(max_data_len >> 8));
	usb_write8(OUT2_DMALENH, (uint8_t)(max_data_len >> 16));
	if ((max_data_len >= USB_AOTG_DMA_BURST8_LEN) &&
	    !(max_data_len % USB_AOTG_DMA_BURST8_LEN)) {
		usb_write8(OUT2_DMACTL, BIT(DMACTL_START));

		dma_reload(usb_aotg_dma.dma_dev, usb_aotg_dma.epout_dma_burst8,
			FIFO2DAT, (uint32_t)data, max_data_len);
		dma_start(usb_aotg_dma.dma_dev, usb_aotg_dma.epout_dma_burst8);
	} else {
		usb_write8(OUT2_DMACTL, BIT(DMACTL_MODE) | BIT(DMACTL_START));

		dma_reload(usb_aotg_dma.dma_dev, usb_aotg_dma.epout_dma_single,
			FIFO2DAT, (uint32_t)data, max_data_len);
		dma_start(usb_aotg_dma.dma_dev, usb_aotg_dma.epout_dma_single);
	}

	if (read_bytes) {
		*read_bytes = max_data_len;
	}

	irq_unlock(key);
	return 0;
cpu_mode:
#endif

	/* FIXME: always return max_data_len */
	if (read_bytes) {
		*read_bytes = max_data_len;
	}

	LOG_DBG("%d", max_data_len);

	ep_ctrl = &usb_aotg_dc.out_ep_ctrl[ep_idx];
	ep_ctrl->data_len = max_data_len;
	ep_ctrl->data = data;
	ep_ctrl->actual = 0;

	/* Enable interrupt */
	usb_set_bit8(OUTIEN, ep_idx);

	aotg_dc_prep_rx(ep_idx);

	irq_unlock(key);
	return 0;
}

int usb_dc_ep_read_actual(uint8_t ep, uint32_t *read_bytes)
{
	struct aotg_dc_ep_ctrl_prv *ep_ctrl;
	uint8_t ep_idx;

	if (!USB_EP_DIR_IS_OUT(ep)) {
		return -EINVAL;
	}

	ep_idx = USB_EP_ADDR2IDX(ep) & USB_EP_NUM_MASK;
	ep_ctrl = &usb_aotg_dc.out_ep_ctrl[ep_idx];

	if (read_bytes) {
		*read_bytes = ep_ctrl->actual;
	}

	return 0;
}

#ifdef CONFIG_USB_AOTG_DC_MULTI_FIFO
static uint8_t usb_dc_ep_io_cancelled;

int usb_dc_ep_set_multi_fifo(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_ADDR2IDX(ep);

	if (USB_EP_DIR_IS_OUT(ep)) {
		usb_aotg_dc.out_ep_ctrl[ep_idx].multi = 1;
	} else {
		usb_aotg_dc.in_ep_ctrl[ep_idx].multi = 1;
	}

	return 0;
}

/*
 * Specific for epin multi-fito non-auto mode, depends on class driver heavily!
 * Until now, only tested for mass storage.
 *
 * for short packet: just like write() with new mode.
 * for others: write data_len bytes synchronously.
 */
int usb_dc_ep_write_pending(const uint8_t ep, uint8_t *data,
				uint32_t data_len, uint32_t * const ret_bytes)
{
	struct aotg_dc_ep_ctrl_prv *ep_ctrl;
	uint8_t ep_idx = USB_EP_ADDR2IDX(ep);
	int ret;
	uint32_t len;
	uint16_t count;
	uint8_t in_empty;
	uint8_t npak;
	uint32_t left;

	if (!usb_aotg_dc.attached || !usb_aotg_ep_addr_valid(ep)) {
		LOG_ERR("No valid endpoint");
		return -EINVAL;
	}

	/* for non-control endpoints */
	if (ep_idx == 0) {
		LOG_ERR("control endpoint");
		return -EINVAL;
	}

	/* Check if IN ep */
	if (!USB_EP_DIR_IS_IN(ep)) {
		LOG_ERR("Wrong endpoint direction");
		return -EINVAL;
	}

	/* Check if ep enabled */
	if (!aotg_dc_ep_is_enabled(ep)) {
		LOG_ERR("Not enabled endpoint");
		return -EINVAL;
	}

	LOG_DBG("%d", data_len);

	/* Disable interrupt always */
	usb_clear_bit8(INIEN, ep_idx);

	ep_ctrl = &usb_aotg_dc.in_ep_ctrl[ep_idx];
	if (data_len < ep_ctrl->mps) {
		goto write;
	}

	usb_dc_ep_io_cancelled = 0;

	npak = usb_read8(INxCS(ep_idx)) & EPCS_NPAK_MASK;

	left = data_len;
	while (left) {
		if (left >= ep_ctrl->mps) {
			len = ep_ctrl->mps;
		} else {
			len = left;
		}
		ep_write_fifo(ep, data, len);

		left -= len;
		data += len;

		LOG_DBG("CS: 0x%x", usb_read8(INxCS(ep_idx)));

		count = 0;
		while (usb_read8(INxCS(ep_idx)) & BIT(EPCS_BUSY)) {
			if (usb_dc_ep_io_cancelled) {
				goto exit;
			}

			if (++count < 1000) {
				k_busy_wait(1);
				continue;
			}

			k_sleep(K_MSEC(20));

			/* timeout: 1000us + 200ms */
			if (count > 1010) {
				LOG_ERR("timeout: 0x%x", usb_read8(INxCS(ep_idx)));
				goto exit;
			}
		}
	}

	/* wait for fifo empty */
	count = 0;
	while (1) {
		LOG_DBG("last CS: 0x%x", usb_read8(INxCS(ep_idx)));
		if ((usb_read8(INxCS(ep_idx)) & EPCS_NPAK_MASK) == npak) {
			break;
		}

		if (usb_dc_ep_io_cancelled) {
			goto exit;
		}

		if (++count < 1000) {
			k_busy_wait(1);
			continue;
		}

		k_sleep(K_MSEC(20));

		/* timeout: 1000us + 200ms */
		if (count > 1010) {
			LOG_ERR("last timeout 0x%x", usb_read8(INxCS(ep_idx)));
			goto exit;
		}
	}

	/* Call the registered callback if any */
	if (ep_ctrl->cb) {
		ep_ctrl->cb(ep, USB_DC_EP_DATA_IN);
	}

exit:
	if (ret_bytes) {
		*ret_bytes = data_len - left;
	}

	LOG_DBG("done CS: 0x%x", usb_read8(INxCS(ep_idx)));

	return 0;
write:
	in_empty = usb_read8(INEMPTY_IEN) & INEMPTY_IEN_MASK;
	/* clear pending */
	usb_write8(INEMPTY_IRQ, BIT(IRQ_INxEMPTY(ep_idx)) | in_empty);

	ret = ep_write_fifo(ep, data, data_len);
	if (ret < 0) {
		return ret;
	}
	/* enable empty */
	usb_write8(INEMPTY_IEN, BIT(IEN_INxEMPTY(ep_idx)) | in_empty);

	if (ret_bytes) {
		*ret_bytes = data_len;
	}

	return 0;
}

/*
 * Specific for epout multi-fito auto mode, depends on class driver heavily!
 * Until now, only tested for mass storage.
 *
 * for short packet: just like read_async(), but no need to set busy.
 * for others: read max_data_len bytes synchronously.
 */
int usb_dc_ep_read_pending(uint8_t ep, uint8_t *data, uint32_t data_len,
				uint32_t *read_bytes)
{
	struct aotg_dc_ep_ctrl_prv *ep_ctrl;
	uint8_t ep_idx = USB_EP_ADDR2IDX(ep);
	uint32_t len;
	uint16_t count;
	uint32_t left;

	if (!usb_aotg_dc.attached || !usb_aotg_ep_addr_valid(ep)) {
		LOG_ERR("No valid endpoint");
		return -EINVAL;
	}

	/* for non-control endpoints */
	if (ep_idx == 0) {
		LOG_ERR("control endpoint");
		return -EINVAL;
	}

	/* Check if OUT ep */
	if (!USB_EP_DIR_IS_OUT(ep)) {
		LOG_ERR("Wrong endpoint direction");
		return -EINVAL;
	}

	/* Allow to read 0 bytes */
	if (!data && data_len) {
		LOG_ERR("Wrong arguments");
		return -EINVAL;
	}

	/* Check if ep enabled */
	if (!aotg_dc_ep_is_enabled(ep)) {
		LOG_ERR("Not enabled endpoint");
		return -EINVAL;
	}

	LOG_DBG("%d", data_len);

	ep_ctrl = &usb_aotg_dc.out_ep_ctrl[ep_idx];

	if (data_len < ep_ctrl->mps) {
		goto read_async;
	}

	/* Disable interrupt */
	usb_clear_bit8(OUTIEN, ep_idx);

	usb_dc_ep_io_cancelled = 0;

	left = data_len;
	while (left) {
		count = 0;
		while (usb_read8(OUTxCS(ep_idx)) & BIT(EPCS_BUSY)) {
			if (usb_dc_ep_io_cancelled) {
				goto exit;
			}

			if (++count < 1000) {
				k_busy_wait(1);
				continue;
			}

			k_sleep(K_MSEC(20));

			/* timeout: 1000us + 200ms */
			if (count > 1010) {
				LOG_ERR("timeout: 0x%x left: %d",
					usb_read8(OUTxCS(ep_idx)), left);
				goto exit;
			}
		}

		len = usb_read16(OUTxBC(ep_idx));
		LOG_DBG("BC: %d", len);

		ep_read_fifo(ep, data, len);

		left -= len;
		data += len;

		LOG_DBG("CS: 0x%x", usb_read8(OUTxCS(ep_idx)));
	}

exit:
	if (read_bytes) {
		*read_bytes = data_len - left;
	}

	/* Clear EPxIN IRQ */
	usb_write8(OUTIRQ, BIT(ep_idx));

	/* Call the registered callback if any */
	if (ep_ctrl->cb) {
		ep_ctrl->cb(ep, USB_DC_EP_DATA_OUT);
	}

	return 0;
read_async:
	/* FIXME: always return max_data_len */
	if (read_bytes) {
		*read_bytes = data_len;
	}

	ep_ctrl->data_len = data_len;
	ep_ctrl->data = data;

	/* Enable interrupt */
	usb_set_bit8(OUTIEN, ep_idx);

	return 0;
}

int usb_dc_ep_io_cancel(void)
{
	usb_dc_ep_io_cancelled = 1;

	return 0;
}

#endif /* CONFIG_USB_AOTG_DC_MULTI_FIFO */

int usb_dc_ep_read_wait(uint8_t ep, uint8_t *data, uint32_t max_data_len,
				uint32_t *read_bytes)
{
	uint32_t data_len, bytes_to_copy;
	uint8_t ep_idx = USB_EP_ADDR2IDX(ep);
	unsigned int key;

	if (!usb_aotg_ep_addr_valid(ep)) {
		LOG_ERR("No valid endpoint");
		return -EINVAL;
	}

	/* Check if OUT ep */
	if (!USB_EP_DIR_IS_OUT(ep)) {
		LOG_ERR("Wrong endpoint direction");
		return -EINVAL;
	}

	/* Allow to read 0 bytes */
	if (!data && max_data_len) {
		LOG_ERR("Wrong arguments");
		return -EINVAL;
	}

	/* Check if ep enabled */
	if (!aotg_dc_ep_is_enabled(ep)) {
		LOG_ERR("Not enabled endpoint");
		return -EINVAL;
	}

	data_len = usb_aotg_dc.out_ep_ctrl[ep_idx].data_len;

	if (!data && !max_data_len) {
		/* When both buffer and max data to read are zero return
		 * the available data in buffer
		 */
		if (read_bytes) {
			*read_bytes = data_len;
		}

		return 0;
	}

	if (data_len > max_data_len) {
		LOG_ERR("Not enough room (%d) to copy all the rcvd data(%d)!",
			max_data_len, data_len);
		bytes_to_copy = max_data_len;
	} else {
		bytes_to_copy = data_len;
	}

	LOG_DBG("Read EP%d, req %d, read %d bytes",
		ep, max_data_len, bytes_to_copy);

	key = irq_lock();
	if (!usb_aotg_dc.attached) {
		irq_unlock(key);
		return -ENODEV;
	}

	aotg_dc_rx(ep, data, bytes_to_copy);
	irq_unlock(key);

	usb_aotg_dc.out_ep_ctrl[ep_idx].data_len -= bytes_to_copy;

	if (read_bytes) {
		*read_bytes = bytes_to_copy;
	}

	return 0;
}

int usb_dc_ep_read_continue(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_ADDR2IDX(ep);
	unsigned int key;

	if (!usb_aotg_ep_addr_valid(ep)) {
		LOG_ERR("No valid endpoint");
		return -EINVAL;
	}

	/* Check if OUT ep */
	if (!USB_EP_DIR_IS_OUT(ep)) {
		LOG_ERR("Wrong endpoint direction");
		return -EINVAL;
	}

	if (!usb_aotg_dc.out_ep_ctrl[ep_idx].data_len) {
		key = irq_lock();
		if (!usb_aotg_dc.attached) {
			irq_unlock(key);
			return -ENODEV;
		}

		aotg_dc_prep_rx(ep_idx);
		irq_unlock(key);
	}

	return 0;
}

int usb_dc_ep_mps(const uint8_t ep)
{
	uint8_t ep_idx = USB_EP_ADDR2IDX(ep);

	if (USB_EP_DIR_IS_OUT(ep)) {
		return usb_aotg_dc.out_ep_ctrl[ep_idx].mps;
	} else {
		return usb_aotg_dc.in_ep_ctrl[ep_idx].mps;
	}
}

#ifdef CONFIG_USB_AOTG_UDC_DMA
static void usb_aotg_epin_dma_callback(const struct device *dev, void *data,
				uint32_t channel, int reason)
{
	struct aotg_dc_ep_ctrl_prv *ep_ctrl;
	uint8_t ep;

	if (reason != DMA_IRQ_TC) {
		return;
	}

	ep_ctrl = &usb_aotg_dc.in_ep_ctrl[USB_AOTG_IN_EP_1];

	/* Set busy */
	if (ep_ctrl->data_len < ep_ctrl->mps) {
		usb_set_bit8(INxCS(USB_AOTG_IN_EP_1), EPCS_BUSY);
	}

	ep = USB_EP_IDX2ADDR(USB_AOTG_IN_EP_1, USB_EP_DIR_IN);
	/* Call the registered callback if any */
	if (ep_ctrl->cb) {
		ep_ctrl->cb(ep, USB_DC_EP_DATA_IN);
	}
}

static void usb_aotg_epout_dma_callback(const struct device *dev, void *data,
				uint32_t channel, int reason)
{
	struct aotg_dc_ep_ctrl_prv *ep_ctrl;

	if (reason != DMA_IRQ_TC) {
		return;
	}

	ep_ctrl = &usb_aotg_dc.out_ep_ctrl[USB_AOTG_OUT_EP_2];
	ep_ctrl->actual = ep_ctrl->data_len;

	/* Call the registered callback if any */
	if (ep_ctrl->cb) {
		ep_ctrl->cb(USB_AOTG_OUT_EP_2, USB_DC_EP_DATA_OUT);
	}
}

static int usb_aotg_dma_init(void)
{
	int ret;

	usb_aotg_dma.dma_dev = device_get_binding(CONFIG_DMA_0_NAME);
	if (!usb_aotg_dma.dma_dev) {
		printk("%s no dma_dev\n", __func__);
		return -ENODEV;
	}

	/*
	 * USB epout single DMA
	 */
	usb_aotg_dma.epout_dma_single = dma_request(usb_aotg_dma.dma_dev, 0xff);
	if (usb_aotg_dma.epout_dma_single < 0) {
		printk("%s epout single dma_request\n", __func__);
		return -EINVAL;
	}

	usb_aotg_dma.epout_dma_config_single.channel_direction = PERIPHERAL_TO_MEMORY;
	usb_aotg_dma.epout_dma_config_single.dma_callback = usb_aotg_epout_dma_callback;
	usb_aotg_dma.epout_dma_config_single.source_data_size = 1;
	usb_aotg_dma.epout_dma_config_single.source_burst_length = 1;
	usb_aotg_dma.epout_dma_config_single.dma_slot = DMA_ID_USB0;
	usb_aotg_dma.epout_dma_config_single.complete_callback_en = 1;
	usb_aotg_dma.epout_dma_config_single.head_block = &usb_aotg_dma.epout_dma_block_single;

	ret = dma_config(usb_aotg_dma.dma_dev, usb_aotg_dma.epout_dma_single,
		&usb_aotg_dma.epout_dma_config_single);
	if (ret) {
		printk("%s epout single dma_config %d\n", __func__, ret);
		return -EINVAL;
	}

	/*
	 * USB epout burst8 DMA
	 */
	usb_aotg_dma.epout_dma_burst8 = dma_request(usb_aotg_dma.dma_dev, 0xff);
	if (usb_aotg_dma.epout_dma_burst8 < 0) {
		printk("%s epout burst8 dma_request\n", __func__);
		return -EINVAL;
	}

	usb_aotg_dma.epout_dma_config_burst8.channel_direction = PERIPHERAL_TO_MEMORY;
	usb_aotg_dma.epout_dma_config_burst8.dma_callback = usb_aotg_epout_dma_callback;
	usb_aotg_dma.epout_dma_config_burst8.source_data_size = 4;
	usb_aotg_dma.epout_dma_config_burst8.source_burst_length = 0;
	usb_aotg_dma.epout_dma_config_burst8.dma_slot = DMA_ID_USB0;
	usb_aotg_dma.epout_dma_config_burst8.complete_callback_en = 1;
	usb_aotg_dma.epout_dma_config_burst8.head_block = &usb_aotg_dma.epout_dma_block_burst8;

	ret = dma_config(usb_aotg_dma.dma_dev, usb_aotg_dma.epout_dma_burst8,
		&usb_aotg_dma.epout_dma_config_burst8);
	if (ret) {
		printk("%s epout burst8 dma_config %d\n", __func__, ret);
		return -EINVAL;
	}

	/*
	 * USB epin burst8 DMA
	 */
	usb_aotg_dma.epin_dma = dma_request(usb_aotg_dma.dma_dev, 0xff);
	if (usb_aotg_dma.epin_dma < 0) {
		printk("%s epin dma_request\n", __func__);
		return -EINVAL;
	}

	usb_aotg_dma.epin_dma_config.channel_direction = MEMORY_TO_PERIPHERAL;
	usb_aotg_dma.epin_dma_config.dma_callback = usb_aotg_epin_dma_callback;
	usb_aotg_dma.epin_dma_config.source_data_size = 4;
	usb_aotg_dma.epout_dma_config_burst8.source_burst_length = 0;
	usb_aotg_dma.epin_dma_config.dma_slot = DMA_ID_USB0;
	usb_aotg_dma.epin_dma_config.complete_callback_en = 1;
	usb_aotg_dma.epin_dma_config.head_block = &usb_aotg_dma.epin_dma_block;

	ret = dma_config(usb_aotg_dma.dma_dev, usb_aotg_dma.epin_dma,
		&usb_aotg_dma.epin_dma_config);
	if (ret) {
		printk("%s epin dma_config %d\n", __func__, ret);
		return -EINVAL;
	}

	return 0;
}
#endif

enum usb_device_speed usb_dc_maxspeed(void)
{
#ifdef CONFIG_USB_AOTG_OTG_SUPPORT_HS
#ifdef CONFIG_USB_AOTG_DC_FS
	return USB_SPEED_FULL;
#else
	return USB_SPEED_HIGH;
#endif
#else
	return USB_SPEED_FULL;
#endif
}

enum usb_device_speed usb_dc_speed(void)
{
	return usb_aotg_dc.speed;
}

int usb_dc_do_remote_wakeup(void)
{
	usb_set_bit8(USBCS, USBCS_WAKEUP);

	return 0;
}
#endif /* CONFIG_USB_AOTG_DC_ENABLED */



/*
 * Host related
 */
#ifdef CONFIG_USB_AOTG_HC_ENABLED
static inline void aotg_hc_handle_reset(void);

/*
 * Map endpoint address of peripheral to controller's
 *
 * mapping rules:
 * 1. out_ep_ctrl[idx] used for peripheral's ep-in
 * 2. in_ep_ctrl[idx] used for peripheral's ep-out
 *
 * for examples:
 * 1. 0x81(peri_ep) -- 0x01(aotg_ep) -- dir_out -- out_ep_ctrl
 * 2. 0x01(peri_ep) -- 0x81(aotg_ep) -- dir_in --in_ep_ctrl
 * 3. 0x80(peri_ep) -- 0x00(aotg_ep) -- dir_out -- out_ep_ctrl
 * 4. 0x00(peri_ep) -- 0x80(aotg_ep) -- dir_in --in_ep_ctrl
 */
static inline int ep_map(uint8_t ep)
{
	struct aotg_hc_ep_ctrl_prv *ep_ctrl;
	uint8_t i;

	/* suppose ep0-in is 0x80 and ep0-out is 0x0 always */
	if (ep == USB_CONTROL_OUT_EP0) {
		usb_aotg_hc.in_ep_ctrl[0].ep_addr = ep;
		return 0;
	} else if (ep == USB_CONTROL_IN_EP0) {
		usb_aotg_hc.out_ep_ctrl[0].ep_addr = ep;
		return 0;
	}

	if (USB_EP_DIR_IS_OUT(ep)) {
		for (i = USB_AOTG_IN_EP_NUM - 1; i > USB_AOTG_EP0_IDX; i--) {
			ep_ctrl = &usb_aotg_hc.in_ep_ctrl[i];
			if (ep_ctrl->ep_ena) {
				continue;
			}
			ep_ctrl->ep_addr = ep;
			return 0;
		}
	} else {
		for (i = USB_AOTG_OUT_EP_1; i < USB_AOTG_OUT_EP_NUM; i++) {
			ep_ctrl = &usb_aotg_hc.out_ep_ctrl[i];
			if (ep_ctrl->ep_ena) {
				continue;
			}
			ep_ctrl->ep_addr = ep;
			return 0;
		}
	}

	return -EINVAL;
}

/*
 * Convert endpoint address of peripheral to controller's
 */
static inline uint8_t find_ep(uint8_t ep)
{
	struct aotg_hc_ep_ctrl_prv *ep_ctrl;
	uint8_t i;

	if (ep == USB_CONTROL_OUT_EP0) {
		return USB_CONTROL_IN_EP0;
	} else if (ep == USB_CONTROL_IN_EP0) {
		return USB_CONTROL_OUT_EP0;
	}

	if (USB_EP_DIR_IS_OUT(ep)) {
		for (i = USB_AOTG_IN_EP_NUM - 1; i > USB_AOTG_EP0_IDX; i--) {
			ep_ctrl = &usb_aotg_hc.in_ep_ctrl[i];
			if (ep_ctrl->ep_addr != ep) {
				continue;
			}
			return USB_EP_IDX2ADDR(i, USB_EP_DIR_IN);
		}
	} else {
		for (i = USB_AOTG_OUT_EP_NUM - 1; i > USB_AOTG_EP0_IDX; i--) {
			ep_ctrl = &usb_aotg_hc.out_ep_ctrl[i];
			if (ep_ctrl->ep_addr != ep) {
				continue;
			}
			return i;
		}
	}

	/* never */
	return 0;
}

/*
 * Check if the MaxPacketSize of endpoint is valid
 */
static inline bool aotg_hc_ep_mps_valid(uint16_t ep_mps, enum usb_ep_type ep_type)
{
	enum usb_device_speed speed = usb_aotg_hc.speed;

	switch (ep_type) {
	case USB_EP_CONTROL:
		/* Arbitrary: speed may be unknown */
		return ep_mps <= USB_MAX_CTRL_MPS;

	case USB_EP_BULK:
		if (speed == USB_SPEED_HIGH) {
			return ep_mps == USB_MAX_HS_BULK_MPS;
		} else if (speed == USB_SPEED_FULL) {
			return ep_mps <= USB_MAX_FS_BULK_MPS;
		}
		break;

	case USB_EP_INTERRUPT:
		if (speed == USB_SPEED_HIGH) {
			return ep_mps <= USB_MAX_HS_INTR_MPS;
		} else if (speed == USB_SPEED_FULL) {
			return ep_mps <= USB_MAX_FS_INTR_MPS;
		}
		break;

	case USB_EP_ISOCHRONOUS:
		if (speed == USB_SPEED_HIGH) {
			return ep_mps <= USB_MAX_HS_ISOC_MPS;
		} else if (speed == USB_SPEED_FULL) {
			return ep_mps <= USB_MAX_FS_ISOC_MPS;
		}
		break;
	}

	return false;
}

static inline bool aotg_hc_ep_is_enabled(uint8_t aotg_ep)
{
	uint8_t ep_idx = USB_EP_ADDR2IDX(aotg_ep);

	if (USB_EP_DIR_IS_OUT(aotg_ep) &&
	    usb_aotg_hc.out_ep_ctrl[ep_idx].ep_ena) {
		return true;
	}
	if (USB_EP_DIR_IS_IN(aotg_ep) &&
	    usb_aotg_hc.in_ep_ctrl[ep_idx].ep_ena) {
		return true;
	}

	return false;
}

/*
 * Using single FIFO for every endpoint (except for ep0) by default
 */
static inline int aotg_hc_ep_alloc_fifo(uint8_t aotg_ep)
{
	uint8_t ep_idx = USB_EP_ADDR2IDX(aotg_ep);

	if (!ep_idx) {
		return 0;
	}

	if (USB_EP_DIR_IS_OUT(aotg_ep)) {
		if (ep_idx >= USB_AOTG_OUT_EP_NUM) {
			return -EINVAL;
		}

		return aotg_hc_epout_alloc_fifo_specific(ep_idx);
	}

	if (ep_idx >= USB_AOTG_IN_EP_NUM) {
		return -EINVAL;
	}

	return aotg_hc_epin_alloc_fifo_specific(ep_idx);
}

/*
 * Max Packet Size Limit
 */
static inline int aotg_hc_set_max_packet(uint8_t aotg_ep, uint16_t ep_mps,
				enum usb_ep_type ep_type)
{
	uint8_t ep_idx = USB_EP_ADDR2IDX(aotg_ep);

	if (!aotg_hc_ep_mps_valid(ep_mps, ep_type)) {
		return -EINVAL;
	}

	if (USB_EP_DIR_IS_OUT(aotg_ep)) {
		usb_write16(OUTxMAXPKT(ep_idx), ep_mps);
		usb_aotg_hc.out_ep_ctrl[ep_idx].mps = ep_mps;
	} else {
		usb_write16(INxMAXPKT(ep_idx), ep_mps);
		usb_aotg_hc.in_ep_ctrl[ep_idx].mps = ep_mps;
	}

	return 0;
}

/*
 * Endpoint type
 */
static inline void aotg_hc_set_ep_type(uint8_t aotg_ep, enum usb_ep_type ep_type)
{
	uint8_t ep_idx = USB_EP_ADDR2IDX(aotg_ep);

	if (USB_EP_DIR_IS_OUT(aotg_ep)) {
		usb_aotg_hc.out_ep_ctrl[ep_idx].ep_type = ep_type;
	} else {
		usb_aotg_hc.in_ep_ctrl[ep_idx].ep_type = ep_type;
	}
}

static inline int aotg_hc_ep_set(uint8_t ep, uint16_t ep_mps,
				enum usb_ep_type ep_type)
{
	uint8_t aotg_ep = find_ep(ep);
	uint8_t ep_idx = USB_EP_ADDR2IDX(aotg_ep);
	int ret;

	LOG_DBG("0x%x(0x%x), mps %d, type %d", aotg_ep, ep, ep_mps,
			ep_type);

	/* Set FIFO address */
	ret = aotg_hc_ep_alloc_fifo(aotg_ep);
	if (ret) {
		return ret;
	}

	/* Set Max Packet */
	ret = aotg_hc_set_max_packet(aotg_ep, ep_mps, ep_type);
	if (ret) {
		return ret;
	}

	/* Set EP type */
	aotg_hc_set_ep_type(aotg_ep, ep_type);

	/* No need to set for endpoint 0 */
	if (!ep_idx) {
		usb_write8(EP0MAXPKT, ep_mps);
		return 0;
	}

	if (USB_EP_DIR_IS_OUT(aotg_ep)) {
		usb_write8(HCINxCTRL(ep_idx), ep);
		/* Set endpoint type and FIFO type */
		usb_write8(OUTxCTRL(ep_idx),
			(ep_type << 2) | USB_AOTG_FIFO_SINGLE);
	} else {
		usb_write8(HCOUTxCTRL(ep_idx), USB_EP_ADDR2IDX(ep));
		usb_write8(INxCTRL(ep_idx),
			(ep_type << 2) | USB_AOTG_FIFO_SINGLE);
	}

	return 0;
}

int usb_hc_ep_configure(const struct usb_hc_ep_cfg_data * const ep_cfg)
{
	if (!usb_aotg_hc.attached || ep_map(ep_cfg->ep_addr)) {
		return -EINVAL;
	}

	return aotg_hc_ep_set(ep_cfg->ep_addr, ep_cfg->ep_mps,
				ep_cfg->ep_type);
}

int usb_hc_ep_enable(const uint8_t ep)
{
	uint8_t aotg_ep = find_ep(ep);
	uint8_t ep_idx = USB_EP_ADDR2IDX(aotg_ep);
	bool dir_out = USB_EP_DIR_IS_OUT(aotg_ep);

	if (!usb_aotg_hc.attached || !usb_aotg_ep_addr_valid(aotg_ep)) {
		return -EINVAL;
	}

	/* NOTE: re-enable is allowed */

	if (dir_out) {
		usb_write8(OUTIRQ, BIT(IRQ_EPxOUT(ep_idx)));
		usb_set_bit8(OUTIEN, ep_idx);
		usb_set_bit8(HCINEPERRIEN, ep_idx);
		usb_aotg_hc.out_ep_ctrl[ep_idx].ep_ena = 1;
	} else {
		usb_write8(INIRQ, BIT(IRQ_EPxIN(ep_idx)));
		usb_set_bit8(INIEN, ep_idx);
		usb_set_bit8(HCOUTEPERRIEN, ep_idx);
		usb_aotg_hc.in_ep_ctrl[ep_idx].ep_ena = 1;
	}

	/* Endpoint 0 */
	if (!ep_idx) {
		return 0;
	}

	if (usb_aotg_ep_reset(aotg_ep, USB_AOTG_EP_RESET)) {
		return -EINVAL;
	}

	/* Activate endpoint and set FIFOCTRL */
	if (dir_out) {
		usb_set_bit8(OUTxCTRL(ep_idx), EPCTRL_VALID);
		usb_write8(FIFOCTRL, ep_idx);
	} else {
		usb_set_bit8(INxCTRL(ep_idx), EPCTRL_VALID);
		usb_write8(FIFOCTRL, ep_idx | BIT(FIFOCTRL_IO_IN));
	}

	usb_aotg_ep_reg_dump(aotg_ep);

	return 0;
}

static int aotg_hc_ep_disable(const uint8_t aotg_ep)
{
	struct aotg_hc_ep_ctrl_prv *ep_ctrl;
	uint8_t ep_idx = USB_EP_ADDR2IDX(aotg_ep);
	bool dir_out = USB_EP_DIR_IS_OUT(aotg_ep);
	struct usb_request *urb;

	if (!usb_aotg_hc.attached || !usb_aotg_ep_addr_valid(aotg_ep)) {
		return -EINVAL;
	}

	if (dir_out) {
		ep_ctrl = &usb_aotg_hc.out_ep_ctrl[ep_idx];
	} else {
		ep_ctrl = &usb_aotg_hc.in_ep_ctrl[ep_idx];
	}

	/* Already disabled */
	if (ep_ctrl->ep_ena == 0) {
		return 0;
	}

	ep_ctrl->ep_ena = 0;

	/* Disable EP interrupts */
	if (dir_out) {
		usb_clear_bit8(OUTIEN, ep_idx);
		usb_clear_bit8(HCINEPERRIEN, ep_idx);
	} else {
		usb_clear_bit8(INIEN, ep_idx);
		usb_clear_bit8(HCOUTEPERRIEN, ep_idx);
	}

	/* Endpoint 0 */
	if (!ep_idx) {
		goto done;
	}

	/* De-activate Ep */
	if (dir_out) {
		usb_clear_bit8(OUTxCTRL(ep_idx), EPCTRL_VALID);
	} else {
		usb_clear_bit8(INxCTRL(ep_idx), EPCTRL_VALID);
	}

done:
	urb = ep_ctrl->urb;
	if (urb) {
		ep_ctrl->urb = NULL;
		urb->status = -ENODEV;
		urb->complete(urb);
	}

	return 0;
}

int usb_hc_ep_disable(const uint8_t ep)
{
	const uint8_t aotg_ep = find_ep(ep);

	return aotg_hc_ep_disable(aotg_ep);
}

int usb_hc_ep_flush(const uint8_t ep)
{
	uint8_t aotg_ep = find_ep(ep);

	if (!usb_aotg_hc.attached || !usb_aotg_ep_addr_valid(aotg_ep)) {
		return -EINVAL;
	}

	return usb_aotg_ep_reset(aotg_ep, USB_AOTG_EP_FIFO_RESET);
}

int usb_hc_ep_reset(const uint8_t ep)
{
	uint8_t aotg_ep = find_ep(ep);

	if (!usb_aotg_hc.attached || !usb_aotg_ep_addr_valid(aotg_ep)) {
		return -EINVAL;
	}

	return usb_aotg_ep_reset(aotg_ep, USB_AOTG_EP_RESET);
}

int usb_hc_set_address(const uint8_t addr)
{
	if (addr > USB_AOTG_MAX_ADDR) {
		return -EINVAL;
	}

	usb_write8(FNADDR, addr);

	return 0;
}

static inline bool bus_reset_done(void)
{
	/* reset done after sending a SOF packet */
	if ((usb_read8(USBIRQ) & (BIT(USBIRQ_SOF) | BIT(USBIRQ_RESET))) ==
	    (BIT(USBIRQ_SOF) | BIT(USBIRQ_RESET))) {
		usb_write8(USBIRQ, usb_read8(USBIRQ));
		return true;
	} else {
		return false;
	}
}

/* FIXME: support port status only */
int usb_hc_root_control(void *buf, int len, struct usb_setup_packet *setup,
				int timeout)
{
	uint16_t req, value;
	int ret = -EPIPE;

	if (!usb_aotg_hc.attached) {
		return -ENODEV;
	}

	req = (setup->bmRequestType << 8) | setup->bRequest;
	value = sys_le16_to_cpu(setup->wValue);

	switch (req) {
	case GetPortStatus:
		if (bus_reset_done()) {
			aotg_hc_handle_reset();
		}
		*(uint32_t *)buf = sys_cpu_to_le32(usb_aotg_hc.port);
		return 4;

	case SetPortFeature:
		if (value == USB_PORT_FEAT_RESET) {
			usb_aotg_hc.port &= ~(USB_PORT_STAT_ENABLE
					| USB_PORT_STAT_LOW_SPEED
					| USB_PORT_STAT_HIGH_SPEED);

			usb_aotg_hc.port |= USB_PORT_STAT_RESET;
			/* port reset */
			usb_set_bit8(HCPORTCTRL, PORTRST);

			return 0;
		}
		break;

	default:
		break;
	}

	return ret;
}

static inline int aotg_hc_control_urb(struct usb_request *urb)
{
	struct aotg_hc_ep_ctrl_prv *ep_ctrl;

	if (urb->len == 0) {
		/* setup, in status */
		ep_ctrl = &usb_aotg_hc.in_ep_ctrl[USB_AOTG_EP0_IDX];
	} else if (urb->ep == USB_CONTROL_OUT_EP0) {
		/* setup, out data, in status */
		ep_ctrl = &usb_aotg_hc.in_ep_ctrl[USB_AOTG_EP0_IDX];
	} else {
		/* setup, in data, out status */
		ep_ctrl = &usb_aotg_hc.out_ep_ctrl[USB_AOTG_EP0_IDX];
	}

	if (ep_ctrl->urb != NULL) {
		return -EBUSY;
	}
	ep_ctrl->urb = urb;

	/* SETUP */
	usb_aotg_hc.phase = USB_AOTG_SETUP;
	usb_set_bit8(EP0CS, EP0CS_HCSET);
	ep0_write_fifo((const uint8_t *const)urb->setup_packet,
			sizeof(struct usb_setup_packet));

	return 0;
}

static inline void aotg_hc_tx_in_pkt(uint8_t ep_idx, uint16_t num_of_pkt)
{
	LOG_DBG("ep_idx: %d, num_of_pkt: %d", ep_idx, num_of_pkt);

	/* set the number of IN Packet(s) */
	usb_write8(HCINxCNTL(ep_idx), (uint8_t)num_of_pkt);
	usb_write8(HCINxCNTH(ep_idx), (uint8_t)(num_of_pkt >> 8));

	/* set busy */
	if (!(usb_read8(OUTxCS(ep_idx)) & BIT(EPCS_BUSY))) {
		usb_set_bit8(OUTxCS(ep_idx), EPCS_BUSY);
	}

	/* short packet control */
	/* usb_set_bit8(HCINCTRL, HCINx_SHORT(ep_idx)); */

	/* send IN Token */
	usb_set_bit8(HCINCTRL, HCINx_START(ep_idx));
}

int usb_hc_submit_urb(struct usb_request *urb)
{
	struct aotg_hc_ep_ctrl_prv *ep_ctrl;
	uint8_t aotg_ep = find_ep(urb->ep);
	unsigned int key;
	uint8_t ep_idx;
	int ret;

	if (!usb_aotg_hc.attached || !usb_aotg_ep_addr_valid(aotg_ep)) {
		return -EINVAL;
	}

	/* disconnect may happen */
	key = irq_lock();
	/* Check if ep enabled */
	if (!aotg_hc_ep_is_enabled(aotg_ep)) {
		irq_unlock(key);
		return -ENODEV;
	}

	ep_idx = USB_EP_ADDR2IDX(aotg_ep);
	if (!ep_idx) {
		ret = aotg_hc_control_urb(urb);
		irq_unlock(key);
		return ret;
	}

	if (USB_EP_DIR_IS_IN(aotg_ep)) {
		ep_ctrl = &usb_aotg_hc.in_ep_ctrl[ep_idx];
		if (ep_ctrl->urb != NULL) {
			irq_unlock(key);
			return -EBUSY;
		}

		ep_ctrl->urb = urb;
		urb->actual = urb->len > ep_ctrl->mps ? ep_ctrl->mps : urb->len;
		/* set actual before write, actual may not correct if error */
		if (ep_write_fifo(aotg_ep, urb->buf, urb->actual) < 0) {
			ep_ctrl->urb = NULL;	/* reset if failed */
			urb->actual = 0;
			irq_unlock(key);
			return -EAGAIN;
		}
		urb->buf += urb->actual;
	} else {
		ep_ctrl = &usb_aotg_hc.out_ep_ctrl[ep_idx];
		if (ep_ctrl->urb != NULL) {
			irq_unlock(key);
			return -EBUSY;
		}

		ep_ctrl->urb = urb;
		aotg_hc_tx_in_pkt(ep_idx, DIV_ROUND_UP(urb->len, ep_ctrl->mps));
	}

	irq_unlock(key);
	return 0;
}

int usb_hc_cancel_urb(struct usb_request *urb)
{
	struct aotg_hc_ep_ctrl_prv *ep_ctrl;
	uint8_t aotg_ep = find_ep(urb->ep);
	uint8_t ep_idx = USB_EP_ADDR2IDX(aotg_ep);
	bool dir_out = USB_EP_DIR_IS_OUT(aotg_ep);

	if (!usb_aotg_hc.attached || !usb_aotg_ep_addr_valid(aotg_ep)) {
		return -EINVAL;
	}

	if (dir_out) {
		ep_ctrl = &usb_aotg_hc.out_ep_ctrl[ep_idx];
	} else {
		ep_ctrl = &usb_aotg_hc.in_ep_ctrl[ep_idx];
	}

	/* Already disabled */
	if (ep_ctrl->ep_ena == 0) {
		return 0;
	}

	if (!ep_idx) {
		goto done;
	}

	if (dir_out) {
		usb_clear_bit8(HCINCTRL, HCINx_START(ep_idx));
	}

	usb_aotg_ep_reset(aotg_ep, USB_AOTG_EP_FIFO_RESET);

done:
	ep_ctrl->urb = NULL;
	urb->complete(urb);

	return 0;
}

static inline void aotg_hc_handle_otg(void)
{
	u8_t tmp;
	u8_t state;

	/* Clear OTG IRQ(s) */
	tmp = usb_read8(OTGIEN) & usb_read8(OTGIRQ);
	usb_write8(OTGIRQ, tmp);

	state = usb_read8(OTGSTATE);

	/* If AOTG enters a_host state, enable state change irq(a_host->a_wait_bcon) */
	if (state == OTG_A_HOST) {
		usb_set_bit8(OTGSTATE_IEN, OTGSTATE_A_WAIT_BCON);
	}

	LOG_DBG("State: 0x%x, irq: 0x%x", state, tmp);
}

static inline void aotg_hc_handle_otg_state(void)
{
	u8_t i;

	/* clear interrupt request */
	usb_set_bit8(OTGSTATE_IRQ, OTGSTATE_A_WAIT_BCON);

	if (usb_read8(OTGSTATE) == OTG_A_WAIT_BCON) {
		for (i = 0; i < USB_AOTG_IN_EP_NUM; i++) {
			aotg_hc_ep_disable(USB_EP_IDX2ADDR(i, USB_EP_DIR_IN));
		}
		for (i = 0; i < USB_AOTG_OUT_EP_NUM; i++) {
			aotg_hc_ep_disable(USB_EP_IDX2ADDR(i, USB_EP_DIR_OUT));
		}
	}
}

static inline void aotg_hc_handle_reset(void)
{
	uint8_t usbcs;

	/* Clear USB Reset IRQ */
	usb_write8(USBIRQ, BIT(USBIRQ_RESET));

	usb_aotg_hc.port &= ~USB_PORT_STAT_RESET;
	usb_aotg_hc.port |= USB_PORT_STAT_ENABLE;

	/* reset all ep-in */
	usb_aotg_ep_reset(0x80, USB_AOTG_EP_RESET);
	/* reset all ep-out */
	usb_aotg_ep_reset(0x0, USB_AOTG_EP_RESET);

	usbcs = usb_read8(USBCS);
	if (usbcs & BIT(USBCS_SPEED)) {
		usb_aotg_hc.speed = USB_SPEED_HIGH;
		usb_aotg_hc.port |= USB_PORT_STAT_HIGH_SPEED;
	} else if (usb_read8(USBCS) & BIT(USBCS_LS)) {
		usb_aotg_hc.speed = USB_SPEED_LOW;
		usb_aotg_hc.port |= USB_PORT_STAT_LOW_SPEED;
	} else {
		usb_aotg_hc.speed = USB_SPEED_FULL;
	}

	LOG_DBG("USBCS: 0x%x", usbcs);
}

static inline void aotg_hc_handle_sof(void)
{
	usb_set_bit8(USBIRQ, USBIRQ_SOF);
}

static inline void aotg_hc_handle_ep0out(void)
{
	struct aotg_hc_ep_ctrl_prv *ep_ctrl;
	struct usb_request *urb;
	uint8_t rx_len, mps;

	/* find urb */
	ep_ctrl = &usb_aotg_hc.out_ep_ctrl[USB_AOTG_EP0_IDX];
	urb = ep_ctrl->urb;
	if (!urb) {
		ep_ctrl = &usb_aotg_hc.in_ep_ctrl[USB_AOTG_EP0_IDX];
		urb = ep_ctrl->urb;
		if (!urb) {
			return;
		}
	}
	ep_ctrl->err_count = 0;

	switch (usb_aotg_hc.phase) {
	case USB_AOTG_IN_DATA:
		mps = usb_aotg_hc.out_ep_ctrl[USB_AOTG_EP0_IDX].mps;
		rx_len = urb->len > mps ? mps : urb->len;

		rx_len = ep0_read_fifo(urb->buf, rx_len);
		urb->buf += rx_len;
		urb->actual += rx_len;

		/* FIXME: handle babble */

		/* short packet or complete */
		if ((rx_len < mps) || (urb->actual == urb->len)) {
			usb_aotg_hc.phase = USB_AOTG_OUT_STATUS;

			usb_clear_bit8(EP0CS, EP0CS_HCSET);
			usb_set_bit8(EP0CS, EP0CS_SETTOGGLE);
			usb_write8(IN0BC, 0);
		} else {
			usb_write8(OUT0BC, 0x0);
		}
		break;

	case USB_AOTG_IN_STATUS:
		usb_aotg_hc.in_ep_ctrl[USB_AOTG_EP0_IDX].urb = NULL;
		urb->status = 0;
		urb->complete(urb);
		break;

	default:
		break;
	}
}

/*
 * Handle EPxOUT data
 */
static inline void aotg_hc_handle_epout(uint8_t ep_idx)
{
	struct aotg_hc_ep_ctrl_prv *ep_ctrl;
	struct usb_request *urb;
	bool done = false;
	uint16_t rx_len;
	uint8_t aotg_ep;

	usb_write8(OUTIRQ, BIT(IRQ_EPxOUT(ep_idx)));

	if (!ep_idx) {
		return aotg_hc_handle_ep0out();
	}

	ep_ctrl = &usb_aotg_hc.out_ep_ctrl[ep_idx];
	urb = ep_ctrl->urb;
	if (!urb) {
		LOG_DBG("ep_idx: %d no urb", ep_idx);
		return;
	}
	ep_ctrl->err_count = 0;

	LOG_DBG("hcepin ep_idx: %d, urb->actual: %d, urb->len: %d",
		ep_idx, urb->actual, urb->len);

	aotg_ep = USB_EP_IDX2ADDR(ep_idx, USB_EP_DIR_OUT);

	rx_len = usb_read16(OUTxBC(ep_idx));

	ep_read_fifo(aotg_ep, urb->buf + urb->actual, rx_len);
	urb->actual += rx_len;

	if (urb->actual > urb->len) {
		LOG_DBG("0x%x(0x%x) babble", aotg_ep, ep_ctrl->ep_addr);
		urb->status = -EIO;
		done = true;
	} else if (urb->actual == urb->len) {
		urb->status = 0;
		done = true;
	} else if (rx_len < ep_ctrl->mps) {
		LOG_DBG("0x%x(0x%x) short %d", aotg_ep, ep_ctrl->ep_addr,
				rx_len);
		urb->status = 0;
		done = true;
	} else {
		usb_set_bit8(OUTxCS(ep_idx), EPCS_BUSY);
	}

	if (done) {
		ep_ctrl->urb = NULL;
		urb->complete(urb);
	}
}

static inline void aotg_hc_handle_ep0in(void)
{
	struct aotg_hc_ep_ctrl_prv *ep_ctrl;
	struct usb_request *urb;
	uint8_t tx_len, mps;

	/* find urb */
	ep_ctrl = &usb_aotg_hc.out_ep_ctrl[USB_AOTG_EP0_IDX];
	urb = ep_ctrl->urb;
	if (!urb) {
		ep_ctrl = &usb_aotg_hc.in_ep_ctrl[USB_AOTG_EP0_IDX];
		urb = ep_ctrl->urb;
		if (!urb) {
			return;
		}
	}
	ep_ctrl->err_count = 0;

	switch (usb_aotg_hc.phase) {
	case USB_AOTG_SETUP:
		if (urb->len == 0) {
			usb_aotg_hc.phase = USB_AOTG_IN_STATUS;

			usb_set_bit8(EP0CS, EP0CS_SETTOGGLE);
			usb_write8(OUT0BC, 0x0);

			break;
		}
		if (urb->ep == USB_CONTROL_IN_EP0) {
			usb_aotg_hc.phase = USB_AOTG_IN_DATA;

			/* send IN token */
			usb_write8(OUT0BC, 0x0);
		} else {
			usb_aotg_hc.phase = USB_AOTG_OUT_DATA;

			mps = usb_aotg_hc.in_ep_ctrl[USB_AOTG_EP0_IDX].mps;
			urb->actual = urb->len > mps ? mps : urb->len;

			usb_clear_bit8(EP0CS, EP0CS_HCSET);
			ep0_write_fifo(urb->buf, urb->actual);
			urb->buf += urb->actual;
		}
		break;

	case USB_AOTG_OUT_DATA:
		if (urb->actual == urb->len) {
			usb_aotg_hc.phase = USB_AOTG_IN_STATUS;

			usb_set_bit8(EP0CS, EP0CS_SETTOGGLE);
			usb_write8(OUT0BC, 0x0);
		} else {
			mps = usb_aotg_hc.in_ep_ctrl[USB_AOTG_EP0_IDX].mps;
			tx_len = urb->len > mps ? mps : urb->len;
			if ((urb->len - urb->actual) > mps) {
				tx_len = mps;
			} else {
				tx_len = urb->len - urb->actual;
			}

			usb_clear_bit8(EP0CS, EP0CS_HCSET);
			ep0_write_fifo(urb->buf, tx_len);
			urb->buf += tx_len;
			urb->actual += tx_len;
		}
		break;

	case USB_AOTG_OUT_STATUS:
		usb_aotg_hc.out_ep_ctrl[USB_AOTG_EP0_IDX].urb = NULL;

		urb->status = 0;
		urb->complete(urb);
		break;

	default:
		break;
	}
}

/*
 * Handle EPxIN data
 */
static inline void aotg_hc_handle_epin(uint8_t ep_idx)
{
	struct aotg_hc_ep_ctrl_prv *ep_ctrl;
	struct usb_request *urb;
	uint16_t tx_len;
	uint8_t aotg_ep;

	usb_write8(INIRQ, BIT(IRQ_EPxIN(ep_idx)));

	if (!ep_idx) {
		return aotg_hc_handle_ep0in();
	}

	ep_ctrl = &usb_aotg_hc.in_ep_ctrl[ep_idx];
	urb = ep_ctrl->urb;
	if (!urb) {
		LOG_DBG("ep_idx: %d no urb", ep_idx);
		return;
	}
	ep_ctrl->err_count = 0;

	LOG_DBG("ep_idx: %d, urb->actual: %d, urb->len: %d",
		ep_idx, urb->actual, urb->len);

	if (urb->actual == urb->len) {
		ep_ctrl->urb = NULL;
		urb->status = 0;
		urb->complete(urb);
		return;
	}

	aotg_ep = USB_EP_IDX2ADDR(ep_idx, USB_EP_DIR_IN);

	tx_len = urb->len - urb->actual;
	tx_len = tx_len > ep_ctrl->mps ? ep_ctrl->mps : tx_len;

	ep_write_fifo(aotg_ep, urb->buf, tx_len);
	urb->buf += tx_len;
	urb->actual += tx_len;
}

static inline void aotg_hc_handle_epout_err(uint8_t ep_idx)
{
	struct aotg_hc_ep_ctrl_prv *ep_ctrl;
	struct usb_request *urb;
	uint8_t type;

	usb_write8(HCINEPERRIRQ, BIT(HCEPxERRIRQ(ep_idx)));

	ep_ctrl = &usb_aotg_hc.out_ep_ctrl[ep_idx];
	urb = ep_ctrl->urb;
	if (!urb) {
		return;
	}

	type = usb_read8(HCINxERR(ep_idx)) & HCEPERR_TYPE_MASK;
	LOG_DBG("ep_idx: %d, type: 0x%x", ep_idx, type);

	switch (type) {
	default:
		if (++ep_ctrl->err_count < ERR_COUNT_MAX) {
			/* resend or try again */
			usb_set_bit8(HCINxERR(ep_idx), HCEPERR_RESEND);
		} else {
			usb_clear_bit8(HCINCTRL, HCINx_START(ep_idx));
			ep_ctrl->err_count = 0;
			ep_ctrl->urb = NULL;
			urb->status = -EIO;
			urb->complete(urb);
		}
		break;

	/* could it happen? */
	case NO_ERR:
		break;

	/* stall */
	case ERR_STALL:
		usb_clear_bit8(HCINCTRL, HCINx_START(ep_idx));
		ep_ctrl->urb = NULL;
		urb->status = -EPIPE;
		urb->complete(urb);
		break;
	}
}

static inline void aotg_hc_handle_epin_err(uint8_t ep_idx)
{
	struct aotg_hc_ep_ctrl_prv *ep_ctrl;
	struct usb_request *urb;
	uint8_t type;

	usb_write8(HCOUTEPERRIRQ, BIT(HCEPxERRIRQ(ep_idx)));

	ep_ctrl = &usb_aotg_hc.in_ep_ctrl[ep_idx];
	urb = ep_ctrl->urb;
	if (!urb) {
		return;
	}

	type = usb_read8(HCOUTxERR(ep_idx)) & HCEPERR_TYPE_MASK;
	LOG_DBG("ep_idx: %d, type: 0x%x", ep_idx, type);

	switch (usb_read8(HCOUTxERR(ep_idx)) & HCEPERR_TYPE_MASK) {
	default:
		if (++ep_ctrl->err_count < ERR_COUNT_MAX) {
			/* resend or try again */
			usb_set_bit8(HCOUTxERR(ep_idx), HCEPERR_RESEND);
		} else {
			ep_ctrl->err_count = 0;
			ep_ctrl->urb = NULL;
			urb->status = -EIO;
			urb->complete(urb);
		}
		break;

	/* could it happen? */
	case NO_ERR:
		break;

	/* stall */
	case ERR_STALL:
		ep_ctrl->urb = NULL;
		urb->status = -EPIPE;
		urb->complete(urb);
		break;
	}
}

static inline void aotg_hc_isr_dispatch(uint8_t vector)
{
	uint8_t usbeien = usb_read8(USBEIEN) & USBEIEN_MASK;

	/*
	 * Make sure external IRQ has been cleared right!
	 *
	 * TODO: If two or more IRQs are pending simultaneously,
	 * EIRQ will be pending immediately after cleared or not?
	 */
	while (usb_read8(USBEIRQ) & BIT(USBEIRQ_EXTERN)) {
		usb_write8(USBEIRQ, BIT(USBEIRQ_EXTERN) | usbeien);
	}

	switch (vector) {
	/* OTG */
	case UIV_OTGIRQ:
		aotg_hc_handle_otg();
		break;

	case UIV_OTGSTATE:
		aotg_hc_handle_otg_state();
		break;

	/* USB Reset */
	case UIV_USBRST:
		aotg_hc_handle_reset();
		break;

	case UIV_SOF:
		aotg_hc_handle_sof();
		break;

	/* HCINx */
	case UIV_EP0OUT:
	case UIV_EP1OUT:
	case UIV_EP2OUT:
	case UIV_EP3OUT:
#if (CONFIG_USB_AOTG_OTG_VERSION == USB_AOTG_VERSION_LEOPARD)
	case UIV_EP4OUT:
	case UIV_EP5OUT:
	case UIV_EP6OUT:
#endif
		aotg_hc_handle_epout(UIV_EPOUT_VEC2ADDR(vector));
		break;

	/* HCOUTx */
	case UIV_EP0IN:
	case UIV_EP1IN:
	case UIV_EP2IN:
	case UIV_EP3IN:
#if (CONFIG_USB_AOTG_OTG_VERSION == USB_AOTG_VERSION_LEOPARD)
	case UIV_EP4IN:
	case UIV_EP5IN:
	case UIV_EP6IN:
#endif
		aotg_hc_handle_epin(UIV_EPIN_VEC2ADDR(vector));
		break;

	/* HCINxERR */
	case UIV_HCIN0ERR:
	case UIV_HCIN1ERR:
	case UIV_HCIN2ERR:
	case UIV_HCIN3ERR:
#if (CONFIG_USB_AOTG_OTG_VERSION == USB_AOTG_VERSION_LEOPARD)
	case UIV_HCIN4ERR:
	case UIV_HCIN5ERR:
	case UIV_HCIN6ERR:
#endif
		aotg_hc_handle_epout_err(UIV_HCINERR_VEC2ADDR(vector));
		break;

	/* HCOUTxERR */
	case UIV_HCOUT0ERR:
	case UIV_HCOUT1ERR:
	case UIV_HCOUT2ERR:
	case UIV_HCOUT3ERR:
#if (CONFIG_USB_AOTG_OTG_VERSION == USB_AOTG_VERSION_LEOPARD)
	case UIV_HCOUT4ERR:
	case UIV_HCOUT5ERR:
	case UIV_HCOUT6ERR:
#endif
		aotg_hc_handle_epin_err(UIV_HCOUTERR_VEC2ADDR(vector));
		break;

	default:
		break;
	}
}

int usb_hc_reset(void)
{
	int ret;

	ret = usb_aotg_reset();

	/* Clear private data */
	memset(&usb_aotg_hc, 0, sizeof(usb_aotg_hc));

	return ret;
}

int usb_hc_enable(void)
{
	if (usb_aotg_hc.attached) {
		LOG_DBG("already");
		return 0;
	}

	aotg_hc_fifo_enable();

	/* Enable OTG(a_host) interrupt */
	usb_set_bit8(OTGIEN, OTGIEN_LOCSOF);

	/* Enable external interrupt */
	usb_set_bit8(USBEIEN, USBEIEN_EXTERN);

	irq_enable(USB_AOTG_IRQ);

	memset(&usb_aotg_hc, 0, sizeof(usb_aotg_hc));
	usb_aotg_hc.port = USB_PORT_STAT_CONNECTION;
	usb_aotg_hc.attached = 1;

	usb_aotg_reg_dump();

	LOG_DBG("");

	return 0;
}

int usb_hc_disable(void)
{
	if (!usb_aotg_hc.attached) {
		LOG_DBG("already");
		return 0;
	}

	irq_disable(USB_AOTG_IRQ);

	memset(&usb_aotg_hc, 0, sizeof(usb_aotg_hc));

	usb_aotg_disable();

	aotg_hc_fifo_disable();

	LOG_DBG("");

	return 0;
}

/* usb ram alloc/free */
int usb_hc_fifo_control(bool enable)
{
	if (enable) {
		return aotg_hc_fifo_enable();
	} else {
		return aotg_hc_fifo_disable();
	}

	return 0;
}
#endif /* CONFIG_USB_AOTG_HC_ENABLED */


uint8_t usb_phy_get_id(void)
{
	return USB_ID_INVALID;
}

uint8_t usb_phy_get_vbus(void)
{
	if(soc_pmu_get_dc5v_status()){
		return USB_VBUS_HIGH;
	}

	return USB_VBUS_LOW;
}

bool usb_phy_hc_attached(void)
{
	/* full-speed or high-speed */
	if ((usb_read8(LINESTATUS) & LINESTATE_MASK) == LINESTATE_DP) {
		return true;
	}

	return false;
}

bool usb_phy_hc_connected(void)
{
#ifdef CONFIG_USB_AOTG_HC_ENABLED
	if ((usb_read8(USBSTATE) == OTG_A_HOST) && bus_reset_done()) {

		return true;
	}
#endif

	return false;
}

bool usb_phy_hc_disconnected(void)
{
	if (usb_read8(USBSTATE) != OTG_A_HOST) {
		return true;
	}

	return false;
}

bool usb_phy_dc_attached(void)
{
	return ((usb_read8(DPDMCTRL) & BIT(PLUGIN)) &&
		!(usb_read8(LINESTATUS) & LINESTATE_MASK));
}

bool usb_phy_dc_detached(void)
{
#ifdef CONFIG_USB_AOTG_DC_ENABLED
	return usb_aotg_dc.attached == 0;
#else
	return true;
#endif
}

bool usb_phy_dc_connected(void)
{
#ifdef CONFIG_USB_AOTG_DC_ENABLED
	return usb_aotg_dc.speed != USB_SPEED_UNKNOWN;
#else
	return false;
#endif
}

bool usb_phy_dc_disconnected(void)
{
	return !(usb_read8(DPDMCTRL) & BIT(PLUGIN));
}

void usb_phy_dc_disconnect(void)
{
#ifdef CONFIG_USB_AOTG_DC_ENABLED
	aotg_dc_disconnect();
#endif
}

int usb_phy_enter_a_idle(void)
{
	usb_aotg_mode = USB_OTG_HOST;
	usb_hc_hardware_init();
	usb_aotg_clock_enable();
	usb_aotg_dpdm_init();
	acts_reset_peripheral(RESET_ID_USB2);

	usb_write8(IDVBUSCTRL, IDVBUS_HOST);
	usb_write8(DPDMCTRL, DPDM_HOST);

	return 0;
}

int usb_phy_enter_b_idle(void)
{
	usb_aotg_mode = USB_OTG_DEVICE;
	usb_dc_hardware_init();
	usb_aotg_clock_enable();
	usb_aotg_dpdm_init();
	acts_reset_peripheral(RESET_ID_USB2);

	usb_write8(IDVBUSCTRL, IDVBUS_DEVICE);
	usb_write8(DPDMCTRL, DPDM_DEVICE);

	return 0;
}

int usb_phy_enter_a_wait_bcon(void)
{
#ifdef CONFIG_USB_AOTG_HC_ENABLED
	usb_aotg_enable();

	usb_write8(IDVBUSCTRL, IDVBUS_HOST);
	usb_write8(DPDMCTRL, DPDM_HOST);

	aotg_hc_phy_init();

	usb_set_bit8(OTGCTRL, OTGCTRL_BUSREQ);
#endif

	return 0;
}

static void usb_aotg_isr_handler(void)
{
	/* IRQ Vector */
	uint8_t vector = usb_read8(IVECT);

	LOG_DBG("vector: 0x%x", vector);

	if (usb_aotg_mode == USB_OTG_HOST) {
#ifdef CONFIG_USB_AOTG_HC_ENABLED
		aotg_hc_isr_dispatch(vector);
#endif
	} else if (usb_aotg_mode == USB_OTG_DEVICE) {
#ifdef CONFIG_USB_AOTG_DC_ENABLED
		aotg_dc_isr_dispatch(vector);
#endif
	}
}

int usb_phy_reset(void)
{
	usb_aotg_exit();

	return 0;
}

int usb_phy_init(void)
{
	usb_aotg_exit();

	/* Connect and enable USB interrupt */
	IRQ_CONNECT(USB_AOTG_IRQ, CONFIG_USB_AOTG_OTG_IRQ_PRIO,
			usb_aotg_isr_handler, 0, 0);
	irq_disable(USB_AOTG_IRQ);

	return 0;
}

int usb_phy_exit(void)
{
	usb_aotg_exit();

	usb_aotg_dpdm_exit();

	return 0;
}

static int usb_aotg_pre_init(const struct device *dev)
{
	ARG_UNUSED(dev);

#ifdef CONFIG_USB_AOTG_UDC_DMA
	usb_aotg_dma_init();
#endif
	return 0;
}

SYS_INIT(usb_aotg_pre_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
