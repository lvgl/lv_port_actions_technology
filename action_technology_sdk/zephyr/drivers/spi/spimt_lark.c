/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SPIMT driver for Actions SoC
 */


//#define DT_DRV_COMPAT actions_acts_spimt

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(spimt_acts);

#include "spi_context.h"
#include <errno.h>
#include <device.h>
#include <drivers/ipmsg.h>
#include <drivers/spimt.h>
#include <rbuf/rbuf_msg_sc.h>

#include <soc.h>
#include <board_cfg.h>
#include <linker/linker-defs.h>

#define SPIMT_CLOCK				(4000000)
//#define SPI_BUF_LEN				(4096)
#define SPI_MAX_BC				(32*1024)
#define SPI_DELAY				(8)

/**
  * @brief SPI Module (SPIMT)
  */
typedef struct {
  volatile uint32_t  CTL;							/*!< (@ 0x00000000) TASK Control Register									   */
  volatile uint32_t  DMA_CTL;						/*!< (@ 0x00000004) TASK DMA Control Register								   */
  volatile uint32_t  DMA_ADD;						/*!< (@ 0x00000008) TASK DMA Address Register								   */
  volatile uint32_t  DMA_CNT;						/*!< (@ 0x0000000C) TASK DMA Counter Register								   */
  volatile uint32_t  RESERVED[12];
} SPIMT_AUTO_TASK_Type; 						/*!< Size = 64 (0x40)														   */

typedef struct {                                /*!< (@ 0x40080000) SPIMT Structure                                            */
  volatile uint32_t  CTL;                          /*!< (@ 0x00000000) Control Register                                           */
  volatile uint32_t  NML_STA;                      /*!< (@ 0x00000004) Status Register                                            */
  volatile  uint32_t  NML_TXDAT;                    /*!< (@ 0x00000008) TX FIFO DATA Register                                      */
  volatile  uint32_t  NML_RXDAT;                    /*!< (@ 0x0000000C) RX FIFO DATA Register                                      */
  volatile uint32_t  NML_BC;                       /*!< (@ 0x00000010) Bytes Count Register                                       */
  volatile uint32_t  AUTO_TASK_STAT;               /*!< (@ 0x00000014) Auto task status Register                                  */
  volatile uint32_t  AUTO_TASK_IE;                 /*!< (@ 0x00000018) Auto task IRQ enable Register                              */
  volatile uint32_t  AUTO_TASK_IP;                 /*!< (@ 0x0000001C) Auto task IRQ pending Register                             */
  volatile uint32_t  TASK_CS_DLY;                  /*!< (@ 0x00000020) Task CS Delay Register                                     */
  volatile uint32_t  TASK_CS_SEL;                  /*!< (@ 0x00000024) Task CS Select Register                                    */
  volatile  uint32_t  RESERVED[54];
  volatile SPIMT_AUTO_TASK_Type AUTO_TASK[8];      /*!< (@ 0x00000100) TASK[0..7] Group                                           */
  volatile  uint32_t  RESERVED1[3904];
} SPIMT_Type;                                   /*!< Size = 768 (0x300)                                                        */

typedef SPIMT_Type  SPIMT_ARRAYType[1];         /*!< max. 2 instances available                                                */

#define SPIMT                       ((SPIMT_ARRAYType*)        SPIMT0_REG_BASE)

/* ==========================================================  CTL  ========================================================== */
#define SPIMT_CTL_CS_SEL_Msk              (0x40000UL)               /*!< CS_SEL (Bitfield-Mask: 0x01)                          */
#define SPIMT_CTL_MODSEL_Msk              (0x20000UL)               /*!< MODSEL (Bitfield-Mask: 0x01)                          */
#define SPIMT_CTL_MODE_SEL_Msk            (0x10000UL)               /*!< MODE_SEL (Bitfield-Mask: 0x01)                        */
#define SPIMT_CTL_RXWR_SEL_Msk            (0x4000UL)                /*!< RXWR_SEL (Bitfield-Mask: 0x01)                        */
#define SPIMT_CTL_DELAY_Msk               (0x3c00UL)                /*!< DELAY (Bitfield-Mask: 0x0f)                           */
#define SPIMT_CTL_3WIRE_Msk               (0x80UL)                  /*!< 3WIRE (Bitfield-Mask: 0x01)                           */
#define SPIMT_CTL_REQ_Msk                 (0x40UL)                  /*!< REQ (Bitfield-Mask: 0x01)                             */
#define SPIMT_CTL_TXFIFO_EN_Msk           (0x20UL)                  /*!< TXFIFO_EN (Bitfield-Mask: 0x01)                       */
#define SPIMT_CTL_RXFIFO_EN_Msk           (0x10UL)                  /*!< RXFIFO_EN (Bitfield-Mask: 0x01)                       */
#define SPIMT_CTL_SS_Msk                  (0x8UL)                   /*!< SS (Bitfield-Mask: 0x01)                              */
#define SPIMT_CTL_LOOP_Msk                (0x4UL)                   /*!< LOOP (Bitfield-Mask: 0x01)                            */
#define SPIMT_CTL_WR_Msk                  (0x3UL)                   /*!< WR (Bitfield-Mask: 0x03)                              */
#define SPIMT_CTL_CS_SEL_Pos              (18UL)                    /*!< CS_SEL (Bit 18)                                       */
#define SPIMT_CTL_DELAY_Pos               (10UL)                    /*!< DELAY (Bit 10)                                        */
#define SPIMT_CTL_WR_Pos                  (0UL)                     /*!< WR (Bit 0)                                            */
/* ========================================================  NML_STA  ======================================================== */
#define SPIMT_NML_STA_BUSY_Msk            (0x10UL)                  /*!< BUSY (Bitfield-Mask: 0x01)                            */
#define SPIMT_NML_STA_TXFU_Msk            (0x8UL)                   /*!< TXFU (Bitfield-Mask: 0x01)                            */
#define SPIMT_NML_STA_TXEM_Msk            (0x4UL)                   /*!< TXEM (Bitfield-Mask: 0x01)                            */
#define SPIMT_NML_STA_RXFU_Msk            (0x2UL)                   /*!< RXFU (Bitfield-Mask: 0x01)                            */
#define SPIMT_NML_STA_RXEM_Msk            (0x1UL)                   /*!< RXEM (Bitfield-Mask: 0x01)                            */
/* =====================================================  AUTO_TASK_IE  ====================================================== */
#define SPIMT_AUTO_TASK_IE_TSK0HFIE_Pos   (8UL)                     /*!< TSK0HFIE (Bit 8)                                      */
#define SPIMT_AUTO_TASK_IE_TSK0TCIE_Pos   (0UL)                     /*!< TSK0TCIE (Bit 0)                                      */
/* =====================================================  AUTO_TASK_IP  ====================================================== */
#define SPIMT_AUTO_TASK_IP_TSK0HFIP_Pos   (8UL)                     /*!< TSK0HFIP (Bit 8)                                      */
#define SPIMT_AUTO_TASK_IP_TSK0TCIP_Pos   (0UL)                     /*!< TSK0TCIP (Bit 0)                                      */
/* ======================================================  TASK_CS_SEL  ====================================================== */
#define SPIMT_TASK_CS_SEL_TSK0CS_SEL_Pos  (0UL)                     /*!< TSK0CS_SEL (Bit 0)                                    */
#define SPIMT_TASK_CS_SEL_TSK0CS_SEL_Msk  (0x3UL)                   /*!< TSK0CS_SEL (Bitfield-Mask: 0x03)                      */
/* ==========================================================  CTL  ========================================================== */
#define SPIMT_AUTO_TASK_CTL_SOFT_EN_Msk   (0x80000000UL)            /*!< SOFT_EN (Bitfield-Mask: 0x01)                         */
#define SPIMT_AUTO_TASK_CTL_SOFT_ST_Msk   (0x40000000UL)            /*!< SOFT_ST (Bitfield-Mask: 0x01)                         */
/* ========================================================  DMA_CTL  ======================================================== */
#define SPIMT_AUTO_TASK_DMA_CTL_DMARELD_Pos (1UL)                   /*!< DMARELD (Bit 1)                                       */
#define SPIMT_AUTO_TASK_DMA_CTL_DMASTART_Msk (0x1UL)                /*!< DMASTART (Bitfield-Mask: 0x01)                        */

struct acts_spimt_config {
	uint32_t ctl_reg;
	uint8_t bus_id;
	uint8_t clock_id;
	uint8_t reset_id;
	void (*irq_config_func)(void);
};

struct acts_spimt_data {
	struct spi_context ctx;
	uint8_t soft_started;
	spi_task_callback_t task_cb[SPI_TASK_NUM];
	void *task_cb_ctx[SPI_TASK_NUM];
	spi_task_t *task_attr[SPI_TASK_NUM];
	uint8_t *task_buf[SPI_TASK_NUM];
	uint32_t task_len[SPI_TASK_NUM];
};

#define DEV_CFG(dev) \
	((const struct acts_spimt_config *const)(dev)->config)
#define DEV_DATA(dev) \
	((struct acts_spimt_data *const)(dev)->data)


#if IS_ENABLED(CONFIG_SPIMT_0) || IS_ENABLED(CONFIG_SPIMT_1)

// switch to auto mode
static void spimt_auto_mode_set(int spi_dev)
{
	/* clear task IRQ pending */
	if((SPIMT[spi_dev]->CTL & SPIMT_CTL_MODSEL_Msk) == 0)
	{
		SPIMT[spi_dev]->AUTO_TASK_IP = SPIMT[spi_dev]->AUTO_TASK_IP;
		/* set spi auto mode */
		SPIMT[spi_dev]->CTL |= SPIMT_CTL_MODSEL_Msk;
	}
}

// switch to normal mode
static void spimt_auto_mode_cancel(int spi_dev)
{
	/* cancel spi auto mode */
	SPIMT[spi_dev]->CTL &= ~SPIMT_CTL_MODSEL_Msk;
}

// CS pin set low
static void spimt_cs_set_low(int spi_dev)
{
	/* cancel spi auto mode */
	spimt_auto_mode_cancel(spi_dev);

	/* set low */
	SPIMT[spi_dev]->CTL &= ~SPIMT_CTL_SS_Msk;
}

// CS pin set high
static void spimt_cs_set_high(int spi_dev)
{
	/* set high */
	SPIMT[spi_dev]->CTL |= SPIMT_CTL_SS_Msk;

	/* enable spi auto mode */
	spimt_auto_mode_set(spi_dev);
}

// CS pin set high/low
static void spimt_cs_set(int spi_dev, int enable)
{
	if (enable) {
		spimt_cs_set_low(spi_dev);
	} else {
		spimt_cs_set_high(spi_dev);
	}
}

// select CS pin num
static void spimt_cs_select(int spi_dev, int cs_num)
{
	SPIMT[spi_dev]->CTL &= ~SPIMT_CTL_CS_SEL_Msk;
	SPIMT[spi_dev]->CTL |= (cs_num << SPIMT_CTL_CS_SEL_Pos);
}

// normal read
static void spimt_read(int spi_dev, unsigned char *buf, unsigned int len)
{
	int left = len;
	int rx_len = 0;
	int i;

	/* clear status */
	SPIMT[spi_dev]->NML_STA = SPIMT[spi_dev]->NML_STA;

	while (left) {
		/* set byte count */
		rx_len = left > SPI_MAX_BC ? SPI_MAX_BC : left;
		left -= rx_len;
		SPIMT[spi_dev]->NML_BC = rx_len;

		/* read-only mode */
		SPIMT[spi_dev]->CTL |= (1 << SPIMT_CTL_WR_Pos);

		/* wait for data */
		for (i = 0; i < rx_len; i++) {
			while(SPIMT[spi_dev]->NML_STA & SPIMT_NML_STA_RXEM_Msk);
			buf[i] = SPIMT[spi_dev]->NML_RXDAT;
		}

		/* disable mode */
		SPIMT[spi_dev]->CTL &= ~SPIMT_CTL_WR_Msk;
	}
}

// normal write
static void spimt_write(int spi_dev, unsigned char *buf, unsigned int len)
{
	int i;

	/* clear status */
	SPIMT[spi_dev]->NML_STA = SPIMT[spi_dev]->NML_STA;

	/* enable write-only mode */
	SPIMT[spi_dev]->CTL |= (2 << SPIMT_CTL_WR_Pos);

	/* write data */
	for (i = 0; i < len; i ++) {
		while (SPIMT[spi_dev]->NML_STA & SPIMT_NML_STA_TXFU_Msk);
		SPIMT[spi_dev]->NML_TXDAT = buf[i];
	}

	/* wait for tx empty*/
	while(!(SPIMT[spi_dev]->NML_STA & SPIMT_NML_STA_TXEM_Msk));

	/* wait for bus idle*/
	while(SPIMT[spi_dev]->NML_STA & SPIMT_NML_STA_BUSY_Msk);

	/* disable write-only mode */
	SPIMT[spi_dev]->CTL &= ~SPIMT_CTL_WR_Msk;
}

/**
 * @breif config spi task
 *
 * @Note
 * 	task 0,1,2,3 route to CS0
 * 	task 4,5,6,7 route to CS1
 */
static void spimt_auto_task_config(int spi_dev, int task_id, const spi_task_t *task_attr, uint32_t addr)
{
	uint8_t *pdata = (uint8_t *)&task_attr->ctl;
	volatile uint32_t ctl = *(volatile unsigned int*)pdata;

	/* config task dma first */
	SPIMT[spi_dev]->AUTO_TASK[task_id].DMA_ADD = addr;//task_attr->dma.addr;
	SPIMT[spi_dev]->AUTO_TASK[task_id].DMA_CNT = task_attr->dma.len;
	if (task_attr->dma.len > 0) {
		SPIMT[spi_dev]->AUTO_TASK[task_id].DMA_CTL = SPIMT_AUTO_TASK_DMA_CTL_DMASTART_Msk
										| (task_attr->dma.reload << SPIMT_AUTO_TASK_DMA_CTL_DMARELD_Pos);
	}

	if (task_attr->irq_type & SPI_TASK_IRQ_CMPLT) {
		/* enable task DMA transmission complete IRQ */
		SPIMT[spi_dev]->AUTO_TASK_IE |= 1 << (SPIMT_AUTO_TASK_IE_TSK0TCIE_Pos + task_id);
	} else {
		/* disable task DMA transmission complete IRQ */
		SPIMT[spi_dev]->AUTO_TASK_IE &= ~(1 << (SPIMT_AUTO_TASK_IE_TSK0TCIE_Pos + task_id));
	}
	if (task_attr->irq_type & SPI_TASK_IRQ_HALF_CMPLT) {
		/* enable task DMA Half transmission complete IRQ */
		SPIMT[spi_dev]->AUTO_TASK_IE |= 1 << (SPIMT_AUTO_TASK_IE_TSK0HFIE_Pos + task_id);
	} else {
		/* disable task DMA Half transmission complete IRQ */
		SPIMT[spi_dev]->AUTO_TASK_IE &= ~(1 << (SPIMT_AUTO_TASK_IE_TSK0HFIE_Pos + task_id));
	}

	/* config task ctl*/
	SPIMT[spi_dev]->AUTO_TASK[task_id].CTL = (ctl & ~(SPIMT_AUTO_TASK_CTL_SOFT_EN_Msk | SPIMT_AUTO_TASK_CTL_SOFT_ST_Msk));

	/* config task cs*/
	SPIMT[spi_dev]->TASK_CS_SEL &= ~(SPIMT_TASK_CS_SEL_TSK0CS_SEL_Msk << (task_id * 2));
	SPIMT[spi_dev]->TASK_CS_SEL |= ((task_id / 4) << (task_id * 2));

}

// force trigger spi task by software if don't use externtal trigger sources
static void spimt_auto_task_soft_start(int spi_dev, int task_id)
{
	/* trigger task by software */
//	SPIMT[spi_dev]->AUTO_TASK[task_id].CTL &= ~SPIMT_AUTO_TASK_CTL_SOFT_ST_Msk;
	SPIMT[spi_dev]->AUTO_TASK[task_id].CTL |= (SPIMT_AUTO_TASK_CTL_SOFT_EN_Msk | SPIMT_AUTO_TASK_CTL_SOFT_ST_Msk);
}

// force trigger spi task by software if don't use externtal trigger sources
static void spimt_auto_task_soft_stop(int spi_dev, int task_id)
{
	/* trigger task by software */
	SPIMT[spi_dev]->AUTO_TASK[task_id].CTL &= ~(SPIMT_AUTO_TASK_CTL_SOFT_EN_Msk | SPIMT_AUTO_TASK_CTL_SOFT_ST_Msk);
}

//get the irq pending
__sleepfunc static int spimt_auto_task_irq_get_pending(int spi_dev)
{
	return SPIMT[spi_dev]->AUTO_TASK_IP & SPIMT[spi_dev]->AUTO_TASK_IE;
}

__sleepfunc static int spimt_auto_task_irq_mask(int task_id, int task_irq_type)
{
	int irq_pending_mask = 0;

	switch (task_irq_type) {
		case SPI_TASK_IRQ_CMPLT:
			irq_pending_mask = 1 << (SPIMT_AUTO_TASK_IP_TSK0TCIP_Pos + task_id);
			break;
		case SPI_TASK_IRQ_HALF_CMPLT:
			irq_pending_mask = 1 << (SPIMT_AUTO_TASK_IP_TSK0HFIP_Pos + task_id);
			break;
		default:
			break;
	}

	return irq_pending_mask;
}

//check if the irq is pending
//static int spimt_auto_task_irq_is_pending(int spi_dev, int task_id, int task_irq_type)
//{
//	return SPIMT[spi_dev]->AUTO_TASK_IP & spimt_auto_task_irq_mask(task_id, task_irq_type);
//}

//clear task irq pending
__sleepfunc static void spimt_auto_task_irq_clr_pending(int spi_dev, int task_id, int task_irq_type)
{
	SPIMT[spi_dev]->AUTO_TASK_IP = spimt_auto_task_irq_mask(task_id, task_irq_type);
}

bool spimt_acts_transfer_ongoing(struct acts_spimt_data *data)
{
	return spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx);
}

static int spimt_acts_transfer_data(const struct acts_spimt_config *cfg, struct acts_spimt_data *data)
{
	struct spi_context *ctx = &data->ctx;

//	if ((ctx->tx_len > SPI_BUF_LEN) || (ctx->rx_len > SPI_BUF_LEN)) {
//		LOG_ERR("buffer overflow");
//		return -1;
//	}

	LOG_DBG("tx_len %d, rx_len %d", ctx->tx_len, ctx->rx_len);

	if (ctx->tx_len) {
		spimt_write(cfg->bus_id, (uint8_t*)ctx->tx_buf, ctx->tx_len);
		spi_context_update_tx(ctx, 1, ctx->tx_len);
	}

	if (ctx->rx_len) {
		spimt_read(cfg->bus_id, (uint8_t*)ctx->rx_buf, ctx->rx_len);
		spi_context_update_rx(ctx, 1, ctx->rx_len);
	}

	return 0;
}

static int spimt_acts_configure(const struct acts_spimt_config *cfg,
			    struct acts_spimt_data *spi,
			    const struct spi_config *config)
{
	if (spi_context_configured(&spi->ctx, config)) {
		/* Nothing to do */
		return 0;
	}

	// code implement by user
	return 0;
}

static int transceive(const struct device *dev,
			  const struct spi_config *config,
			  const struct spi_buf_set *tx_bufs,
			  const struct spi_buf_set *rx_bufs,
		      bool asynchronous,
		      struct k_poll_signal *signal)
{
	const struct acts_spimt_config *cfg =DEV_CFG(dev);
	struct acts_spimt_data *data = DEV_DATA(dev);
	int ret;

	spi_context_lock(&data->ctx, asynchronous, signal);

	/* Configure */
	ret = spimt_acts_configure(cfg, data, config);
	if (ret) {
		goto out;
	}

	/* Set buffers info */
	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	/* Set cs active */
	spimt_cs_set(cfg->bus_id, 1);

	/* transfer data */
	do {
		ret = spimt_acts_transfer_data(cfg, data);
	} while (!ret && spimt_acts_transfer_ongoing(data));

	/* Set cs deactive */
	spimt_cs_set(cfg->bus_id, 0);

out:
	spi_context_release(&data->ctx, ret);
	return ret;
}

static int spimt_acts_transceive(const struct device *dev,
				  const struct spi_config *config,
				  const struct spi_buf_set *tx_bufs,
				  const struct spi_buf_set *rx_bufs)
{
  return transceive(dev, config, tx_bufs,  rx_bufs, false, NULL);

}

static int spimt_acts_release(const struct device *dev,
					const struct spi_config *config)
{
	struct acts_spimt_data *data = dev->data;
	spi_context_unlock_unconditionally(&data->ctx);
	return 0;
}

static void spimt_acts_register_callback(const struct device *dev, int task_id,
					spi_task_callback_t cb, void *context)
{
	struct acts_spimt_data *data = DEV_DATA(dev);

	data->task_cb[task_id] = cb;
	data->task_cb_ctx[task_id] = context;
}

static uint8_t* spimt_task_buf_start(struct acts_spimt_data *data,
					int task_id, uint32_t addr, uint16_t len, uint8_t rd)
{
	rbuf_t *rbuf;
	uint8_t *sbuf;
	uint16_t slen;

	/* check task len */
	if (len == 0) {
		LOG_ERR("task[%d] length %d error!", task_id, len);
		return NULL;
	}

	/* get task buf */
	sbuf = data->task_buf[task_id];

	/* check task buf len */
	if ((sbuf != NULL) && (len > data->task_len[task_id])) {
		/* free task buf */
		rbuf = RBUF_FR_BUF(sbuf);
		RB_MSG_FREE(rbuf);
		sbuf = NULL;
		data->task_buf[task_id] = NULL;
		data->task_len[task_id] = 0;
	}

	/* alloc task buf */
	if (sbuf == NULL) {
		slen = (len < 16) ? 16 : len;
		rbuf = RB_MSG_ALLOC(slen);
		if (rbuf != NULL) {
			sbuf = (uint8_t*)RBUF_TO_BUF(rbuf);
			data->task_buf[task_id] = sbuf;
			data->task_len[task_id] = slen;
		} else {
			LOG_ERR("task[%d] malloc(%d) error!", task_id, len);
		}
	}

	/* copy buf before writing */
	if (!rd && (addr != 0) && sbuf) {
		memcpy(sbuf, (void*)addr, len);
	}

	return sbuf;
}

static uint8_t* spimt_task_buf_stop(struct acts_spimt_data *data,
					int task_id, uint32_t addr, uint32_t len, uint8_t rd)
{
	uint8_t *sbuf;

	sbuf = data->task_buf[task_id];

	/* copy buf after reading */
	if (rd && (addr != 0) && sbuf) {
		memcpy((void*)addr, sbuf, len);
	}

	return sbuf;
}

static int spimt_acts_task_start(const struct device *dev, int task_id,
					const spi_task_t *attr)
{
	const struct acts_spimt_config *cfg =DEV_CFG(dev);
	struct acts_spimt_data *data = DEV_DATA(dev);
	uint8_t *buf;

	/* start dma buffer */
	buf = spimt_task_buf_start(data, task_id, attr->dma.addr, attr->dma.len, attr->ctl.rwsel);
	if (buf == NULL) {
		return -1;
	}

	/* save attr */
	data->task_attr[task_id] = (spi_task_t*)attr;

	/* select spi auto mode */
	spimt_auto_mode_set(cfg->bus_id);

	/* config spi task */
	spimt_auto_task_config(cfg->bus_id, task_id, attr, (uint32_t)buf);

	/* config ppi */
	if (!attr->ctl.soft) {
		/* disable ppi trigger */
		ppi_trig_src_en(attr->trig.trig, 0);

		/* clear ppi pending */
		ppi_trig_src_clr_pending(attr->trig.trig);

		/* config ppi trigger */
		ppi_task_trig_config(attr->trig.chan, attr->trig.task, attr->trig.trig);

		/* enable ppi trigger */
		ppi_trig_src_en(attr->trig.trig, attr->trig.en);
	} else {
		/* soft trigger */
		data->soft_started = 1;
		spimt_auto_task_soft_start(cfg->bus_id, task_id);
	}

	return 0;
}

static int spimt_acts_task_stop(const struct device *dev, int task_id)
{
	const struct acts_spimt_config *cfg =DEV_CFG(dev);
	struct acts_spimt_data *data = DEV_DATA(dev);
	const spi_task_t *attr = data->task_attr[task_id];

	/* disable ppi trigger */
	if ((attr != NULL) && (!attr->ctl.soft)) {
		ppi_trig_src_en(attr->trig.trig, 0);
	} else {
		spimt_auto_task_soft_stop(cfg->bus_id, task_id);
		data->soft_started = 0;
	}

	/* stop dma buffer */
	spimt_task_buf_stop(data, task_id, attr->dma.addr, attr->dma.len, attr->ctl.rwsel);

	/* clear attr */
	data->task_attr[task_id] = NULL;

	return 0;
}

static int spimt_acts_cs_select(const struct device *dev, int cs_num)
{
	const struct acts_spimt_config *cfg = DEV_CFG(dev);

	spimt_cs_select(cfg->bus_id, cs_num);

	return 0;
}

static int spimt_acts_ctl_reset(const struct device *dev)
{
	const struct acts_spimt_config *cfg = DEV_CFG(dev);

	// reset spimt
	acts_reset_peripheral(cfg->reset_id);

	/* set spi mode3 (default mode) */
	SPIMT[cfg->bus_id]->CTL = SPIMT_CTL_SS_Msk | (SPI_DELAY << SPIMT_CTL_DELAY_Pos)
												| SPIMT_CTL_RXFIFO_EN_Msk | SPIMT_CTL_TXFIFO_EN_Msk 
												| (1 <<  (16UL));//spimt mode 0 == 1 << 16 ; spimt mode3 == 0 << 16 

	return 0;
}

static const unsigned short spi_irq_list[2] = {
	SPI_TASK_IRQ_CMPLT,
	SPI_TASK_IRQ_HALF_CMPLT,
};

static void spimt_acts_isr(const struct device *dev)
{
	const struct acts_spimt_config *cfg = DEV_CFG(dev);
	struct acts_spimt_data *data =  DEV_DATA(dev);
	int task_id, irq_type, len;
	int pending = spimt_auto_task_irq_get_pending(cfg->bus_id);
	int pos = find_msb_set(pending) - 1;
	spi_task_callback_t cb;
	const spi_task_t *attr;
	uint8_t *buf;
	void *ctx;

	while (pos >= 0) {
		task_id = (pos % 8);
		irq_type = spi_irq_list[pos / 8];
		attr = data->task_attr[task_id];

		/* clear task pending */
		spimt_auto_task_irq_clr_pending(cfg->bus_id, task_id, irq_type);

		/* clear ppi pending */
		ppi_trig_src_clr_pending(SPIMT0_TASK0_CIP + attr->trig.task);
		if (!attr->ctl.soft) {
			ppi_trig_src_clr_pending(attr->trig.trig);
			if (attr->trig.trig <= TIMER4) {
				//timer_clear_pd(attr->trig.trig);
			}
		}

		/* call handler */
		cb = data->task_cb[task_id];
		if (cb != NULL) {
			/* get buffer */
			ctx = data->task_cb_ctx[task_id];
			buf = data->task_buf[task_id];
			len = attr->dma.len / 2;
			switch(irq_type) {
				case SPI_TASK_IRQ_CMPLT:
					buf += len;
					break;
				case SPI_TASK_IRQ_HALF_CMPLT:
					break;
			}
			cb(buf, len, ctx);
		}

		/* find msb */
		pending = spimt_auto_task_irq_get_pending(cfg->bus_id);
		pos = find_msb_set(pending) - 1;
	}


}

__sleepfunc uint8_t* spi_task_get_data(int bus_id, int task_id, int trig, int *plen)
{
	int len = 0;
	uint8_t *buf = NULL;
	int pending = spimt_auto_task_irq_get_pending(bus_id);

	/* clear task pending */
	if (pending & spimt_auto_task_irq_mask(task_id, SPI_TASK_IRQ_HALF_CMPLT)) {
		spimt_auto_task_irq_clr_pending(bus_id, task_id, SPI_TASK_IRQ_HALF_CMPLT);
		len = SPIMT[bus_id]->AUTO_TASK[task_id].DMA_CNT / 2;
		buf = (uint8_t *)SPIMT[bus_id]->AUTO_TASK[task_id].DMA_ADD;
	} else if (pending & spimt_auto_task_irq_mask(task_id, SPI_TASK_IRQ_CMPLT)) {
		spimt_auto_task_irq_clr_pending(bus_id, task_id, SPI_TASK_IRQ_CMPLT);
		len = SPIMT[bus_id]->AUTO_TASK[task_id].DMA_CNT / 2;
		buf = (uint8_t *)SPIMT[bus_id]->AUTO_TASK[task_id].DMA_ADD + len;
	}

	/* clear ppi pending */
	if (buf) {
		ppi_trig_src_clr_pending(SPIMT0_TASK0_CIP+task_id);
		if (trig >= 0) {
			ppi_trig_src_clr_pending(trig);
		}
	}

	if (plen) {
		*plen = len;
	}
	return buf;
}

#ifdef CONFIG_PM_DEVICE
int spimt_pm_control(const struct device *dev, enum pm_device_action action)
{
	struct acts_spimt_data *data =  DEV_DATA(dev);

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		if(data->soft_started) {
			LOG_ERR("spimt busy, cannot suspend");
			return -1;
		}
		break;
	default:
		return 0;
	}

	return 0;
}
#else
#define spimt_pm_control 	NULL
#endif

int spimt_acts_init(const struct device *dev)
{
	const struct acts_spimt_config *cfg = DEV_CFG(dev);
	struct acts_spimt_data *data =  DEV_DATA(dev);

	// enable clock
	acts_clock_peripheral_enable(cfg->clock_id);

	// reset spimt
	spimt_acts_ctl_reset(dev);

	// set clock
	clk_set_rate(cfg->clock_id, SPIMT_CLOCK);

	spi_context_unlock_unconditionally(&data->ctx);

	/* irq init */
	cfg->irq_config_func();

	data->soft_started = 0;

	return 0;
}


static const struct spimt_driver_api spimt_acts_driver_api = {
	.spi_api = {
		.transceive = spimt_acts_transceive,
		.release = spimt_acts_release,
	},
	.register_callback = spimt_acts_register_callback,
	.task_start = spimt_acts_task_start,
	.task_stop = spimt_acts_task_stop,
	.cs_select = spimt_acts_cs_select,
	.ctl_reset = spimt_acts_ctl_reset,
};

#endif  /*#if IS_ENABLED(CONFIG_SPIMT_0)*/

#define SPIMT_ACTS_DEFINE_CONFIG(n)					\
	static const struct device DEVICE_NAME_GET(spimt##n##_acts);		\
									\
	static void spimt##n##_acts_irq_config(void)			\
	{								\
		IRQ_CONNECT(IRQ_ID_SPI##n##MT, CONFIG_SPIMT_##n##_IRQ_PRI,	\
			    spimt_acts_isr,				\
			    DEVICE_GET(spimt##n##_acts), 0);		\
		irq_enable(IRQ_ID_SPI##n##MT);				\
	}								\
	static const struct acts_spimt_config spimt##n##_acts_config = {    \
		.ctl_reg = SPIMT##n##_REG_BASE,\
		.bus_id = n,\
		.clock_id = CLOCK_ID_SPIMT##n,\
		.reset_id = RESET_ID_SPIMT##n,\
		.irq_config_func = spimt##n##_acts_irq_config, \
	}

#define SPIMT_ACTS_DEFINE_DATA(n)						\
	static __act_s2_sleep_data struct acts_spimt_data spimt##n##_acts_dev_data = { 	\
		SPI_CONTEXT_INIT_LOCK(spimt##n##_acts_dev_data, ctx),	\
		SPI_CONTEXT_INIT_SYNC(spimt##n##_acts_dev_data, ctx),	\
	}

#define SPIMT_ACTS_DEVICE_INIT(n)					\
	SPIMT_ACTS_DEFINE_CONFIG(n);					\
	SPIMT_ACTS_DEFINE_DATA(n);						\
	DEVICE_DEFINE(spimt##n##_acts,				\
						CONFIG_SPIMT_##n##_NAME,					\
						&spimt_acts_init, spimt_pm_control, &spimt##n##_acts_dev_data,	\
						&spimt##n##_acts_config, POST_KERNEL,		\
						CONFIG_SPI_INIT_PRIORITY, &spimt_acts_driver_api);

#if IS_ENABLED(CONFIG_SPIMT_0)
SPIMT_ACTS_DEVICE_INIT(0)
#endif
#if IS_ENABLED(CONFIG_SPIMT_1)
SPIMT_ACTS_DEVICE_INIT(1)
#endif
