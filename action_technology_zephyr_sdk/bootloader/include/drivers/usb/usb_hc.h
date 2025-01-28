/* usb_hc.h - USB host controller driver interface */

/*
 * Copyright (c) 2020 Actions Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB host controller APIs
 *
 * This file contains the USB host controller APIs. All host controller
 * drivers should implement the APIs described in this file.
 */

#ifndef __USB_HC_H__
#define __USB_HC_H__

#include <device.h>
#include <drivers/usb/usb.h>

/*
 * Hub request types
 */

#define USB_RT_HUB	(USB_TYPE_CLASS | USB_RECIP_DEVICE)
#define USB_RT_PORT	(USB_TYPE_CLASS | USB_RECIP_OTHER)

/*
 * Hub Class feature numbers
 */
#define C_HUB_LOCAL_POWER   0
#define C_HUB_OVER_CURRENT  1

/*
 * Port feature numbers
 */
#define USB_PORT_FEAT_CONNECTION	0
#define USB_PORT_FEAT_ENABLE		1
#define USB_PORT_FEAT_SUSPEND		2
#define USB_PORT_FEAT_OVER_CURRENT	3
#define USB_PORT_FEAT_RESET		4
#define USB_PORT_FEAT_POWER		8
#define USB_PORT_FEAT_LOWSPEED		9
#define USB_PORT_FEAT_HIGHSPEED		10
#define USB_PORT_FEAT_C_CONNECTION	16
#define USB_PORT_FEAT_C_ENABLE		17
#define USB_PORT_FEAT_C_SUSPEND		18
#define USB_PORT_FEAT_C_OVER_CURRENT	19
#define USB_PORT_FEAT_C_RESET		20
#define USB_PORT_FEAT_TEST		21

/*
 * Changes to Port feature numbers for Super speed,
 * from USB 3.0 spec Table 10-8
 */
#define USB_SS_PORT_FEAT_U1_TIMEOUT	23
#define USB_SS_PORT_FEAT_U2_TIMEOUT	24
#define USB_SS_PORT_FEAT_C_LINK_STATE	25
#define USB_SS_PORT_FEAT_C_CONFIG_ERROR	26
#define USB_SS_PORT_FEAT_BH_RESET	28
#define USB_SS_PORT_FEAT_C_BH_RESET	29

/* wPortStatus bits */
#define USB_PORT_STAT_CONNECTION    0x0001
#define USB_PORT_STAT_ENABLE        0x0002
#define USB_PORT_STAT_SUSPEND       0x0004
#define USB_PORT_STAT_OVERCURRENT   0x0008
#define USB_PORT_STAT_RESET         0x0010
#define USB_PORT_STAT_POWER         0x0100
#define USB_PORT_STAT_LOW_SPEED     0x0200
#define USB_PORT_STAT_HIGH_SPEED    0x0400	/* support for EHCI */
#define USB_PORT_STAT_SUPER_SPEED   0x0600	/* faking support to XHCI */
#define USB_PORT_STAT_SPEED_MASK	\
	(USB_PORT_STAT_LOW_SPEED | USB_PORT_STAT_HIGH_SPEED)

/*
 * Changes to wPortStatus bit field in USB 3.0
 * See USB 3.0 spec Table 10-11
 */
#define USB_SS_PORT_STAT_LINK_STATE	0x01e0
#define USB_SS_PORT_STAT_POWER		0x0200
#define USB_SS_PORT_STAT_SPEED		0x1c00
#define USB_SS_PORT_STAT_SPEED_5GBPS	0x0000

/* wPortChange bits */
#define USB_PORT_STAT_C_CONNECTION  0x0001
#define USB_PORT_STAT_C_ENABLE      0x0002
#define USB_PORT_STAT_C_SUSPEND     0x0004
#define USB_PORT_STAT_C_OVERCURRENT 0x0008
#define USB_PORT_STAT_C_RESET       0x0010

/*
 * Changes to wPortChange bit fields in USB 3.0
 * See USB 3.0 spec Table 10-12
 */
#define USB_SS_PORT_STAT_C_BH_RESET	0x0020
#define USB_SS_PORT_STAT_C_LINK_STATE	0x0040
#define USB_SS_PORT_STAT_C_CONFIG_ERROR	0x0080

/* wHubCharacteristics (masks) */
#define HUB_CHAR_LPSM               0x0003
#define HUB_CHAR_COMPOUND           0x0004
#define HUB_CHAR_OCPM               0x0018

/*
 * Hub Status & Hub Change bit masks
 */
#define HUB_STATUS_LOCAL_POWER	0x0001
#define HUB_STATUS_OVERCURRENT	0x0002

#define HUB_CHANGE_LOCAL_POWER	0x0001
#define HUB_CHANGE_OVERCURRENT	0x0002

/* Mask for wIndex in get/set port feature */
#define USB_HUB_PORT_MASK	0xf

/*
 * Hub Port status
 */
struct usb_port_status {
	unsigned short wPortStatus;
	unsigned short wPortChange;
} __packed;

/*
 * Hub status
 */
struct usb_hub_status {
	unsigned short wHubStatus;
	unsigned short wHubChange;
} __packed;

/*
 *class requests for USB 2.0 hub
 */
#define ClearHubFeature		(0x2000 | USB_REQ_CLEAR_FEATURE)
#define ClearPortFeature	(0x2300 | USB_REQ_CLEAR_FEATURE)
#define GetHubDescriptor	(0xa000 | USB_REQ_GET_DESCRIPTOR)
#define GetHubStatus		(0xa000 | USB_REQ_GET_STATUS)
#define GetPortStatus		(0xa300 | USB_REQ_GET_STATUS)
#define SetHubFeature		(0x2000 | USB_REQ_SET_FEATURE)
#define SetPortFeature		(0x2300 | USB_REQ_SET_FEATURE)

/**
 * USB Endpoint Configuration.
 */
struct usb_hc_ep_cfg_data {
	/** The number associated with the EP in the device
	 *  configuration structure
	 *       IN  EP = 0x80 | \<endpoint number\>
	 *       OUT EP = 0x00 | \<endpoint number\>
	 */
	uint8_t ep_addr;
	uint16_t ep_mps;             /** Endpoint max packet size */
	enum usb_ep_type ep_type; /** Endpoint type */
};

struct usb_request;

typedef void (*usb_complete_t)(struct usb_request *);

struct usb_request {
	uint8_t ep;
	enum usb_ep_type ep_type;

	uint8_t *buf;	/* (in) associated data buffer */
	uint32_t len;	/* (in) data buffer length */
	uint32_t actual;	/* (return) actual transfer length */

	uint8_t *setup_packet;	/* (in) setup packet (control only) */

	/*
	 * case -EBUSY: init state
	 * case 0: transfer OK
	 * case -EIO: transfer error
	 * case -EPIPE: endpoint stall
	 * case -ECONNRESET: transfer cancelled or timeout
	 * case -ENODEV: endpoint disabled or device disconnected
	 */
	int status;	/* check status before actual */

	void *context;	/* (in) context for completion */
	usb_complete_t complete;	/* (in) completion routine */
};



/**
 * @brief configure endpoint
 *
 * Function to configure an endpoint. usb_hc_ep_cfg_data structure provides
 * the endpoint configuration parameters: endpoint address, endpoint maximum
 * packet size and endpoint type.
 *
 * @param[in] cfg Endpoint config
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_hc_ep_configure(const struct usb_hc_ep_cfg_data * const cfg);

/**
 * @brief enable the selected endpoint
 *
 * Function to enable the selected endpoint. Upon success interrupts are
 * enabled for the corresponding endpoint and the endpoint is ready for
 * transmitting/receiving data.
 *
 * @param[in] ep Endpoint address corresponding to the one
 *               listed in the device configuration table
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_hc_ep_enable(const uint8_t ep);

/**
 * @brief disable the selected endpoint
 *
 * Function to disable the selected endpoint. Upon success interrupts are
 * disabled for the corresponding endpoint and the endpoint is no longer able
 * for transmitting/receiving data.
 *
 * @param[in] ep Endpoint address corresponding to the one
 *               listed in the device configuration table
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_hc_ep_disable(const uint8_t ep);

/**
 * @brief flush the selected endpoint
 *
 * @param[in] ep Endpoint address corresponding to the one
 *               listed in the device configuration table
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_hc_ep_flush(const uint8_t ep);

/**
 * @brief reset the selected endpoint
 *
 * @param[in] ep Endpoint address corresponding to the one
 *               listed in the device configuration table
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_hc_ep_reset(const uint8_t ep);

/**
 * @brief set USB device address
 *
 * @param[in] addr device address
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_hc_set_address(const uint8_t addr);

/**
 * @brief control transfer for root-hub
 *
 * This function should never be blocked and could be called in interrupt
 * context.
 *
 * @return >=0 on success, negative errno code on fail.
 */
int usb_hc_root_control(void *buf, int len, struct usb_setup_packet *setup,
				int timeout);

/**
 * @brief start transfer for non-root-hub device
 *
 * This function should never be blocked and could be called in interrupt
 * context.
 *
 * @return =0 on success, negative errno code on fail.
 */
int usb_hc_submit_urb(struct usb_request *urb);

/**
 * @brief cancel transfer for non-root-hub device
 *
 * This function should never be blocked and could be called in interrupt
 * context.
 *
 * @return =0 on success, negative errno code on fail.
 */
int usb_hc_cancel_urb(struct usb_request *urb);

/**
 * @brief reset the USB host controller
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_hc_reset(void);

/**
 * @brief enable the USB host controller
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_hc_enable(void);

/**
 * @brief disable the USB host controller
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_hc_disable(void);

/**
 * @brief USB host controller FIFO control
 *
 * @return >=0 on success, negative errno code on fail.
 */
int usb_hc_fifo_control(bool enable);

#endif /* __USB_HC_H__ */
