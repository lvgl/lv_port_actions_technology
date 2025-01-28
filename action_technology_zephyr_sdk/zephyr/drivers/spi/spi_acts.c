/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SPI driver for Actions SoC
 */

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(spi_acts);

#include "spi_context.h"
#include <errno.h>
#include <device.h>
#include <drivers/spi.h>
#include <drivers/dma.h>
#include <soc.h>
#include <board_cfg.h>
/* SPI registers macros*/
#define SPI_CTL_CLK_SEL_MASK		(0x1 << 31)
#define SPI_CTL_CLK_SEL_CPU			(0x0 << 31)
#define SPI_CTL_CLK_SEL_DMA			(0x1 << 31)

#define SPI_CTL_FIFO_WIDTH_MASK		(0x1 << 30)
#define SPI_CTL_FIFO_WIDTH_8BIT		(0x0 << 30)
#define SPI_CTL_FIFO_WIDTH_32BIT	(0x1 << 30)

#define SPI_CTL_MODE_MASK		(3 << 28)
#define SPI_CTL_MODE(x)			((x) << 28)
#define SPI_CTL_MODE_CPHA		(1 << 28)
#define SPI_CTL_MODE_CPOL		(1 << 29)

//#define SPI_CTL_DUAL_QUAD_SEL_MASK			(0x0 << 27)
//#define SPI_CTL_DUAL_QUAD_SEL_2x_4x			(0x0 << 27)
//#define SPI_CTL_DUAL_QUAD_SEL_DUAL_QUAL		(0x1 << 27)

#define SPI_CTL_RXW_DELAY_MASK		(0x1 << 26)
#define SPI_CTL_RXW_DELAY_2CYCLE	(0x0 << 26)
#define SPI_CTL_RXW_DELAY_3CYCLE	(0x1 << 26)

#define SPI_CTL_DMS_MASK		(0x1 << 25)
#define SPI_CTL_DMS_BURST8		(0x0 << 25)
#define SPI_CTL_DMS_SINGLE		(0x1 << 25)

#define SPI_CTL_TXCEB_MASK				(0x0 << 24)
#define SPI_CTL_TXCEB_NOT_CONVERT		(0x0 << 24)
#define SPI_CTL_TXCEB_CONVERT			(0x1 << 24)

#define SPI_CTL_RXCEB_MASK				(0x0 << 23)
#define SPI_CTL_RXCEB_NOT_CONVERT		(0x0 << 23)
#define SPI_CTL_RXCEB_CONVERT			(0x1 << 23)

#define SPI_CTL_MS_SEL_MASK			(0x1 << 22)
#define SPI_CTL_MS_SEL_MASTER		(0x0 << 22)
#define SPI_CTL_MS_SEL_SLAVE		(0x1 << 22)

#define SPI_CTL_SB_SEL_MASK		(0x1 << 21)
#define SPI_CTL_SB_SEL_MSB		(0x0 << 21)
#define SPI_CTL_SB_SEL_LSB		(0x1 << 21)

#if defined(CONFIG_SOC_SERIES_LEOPARD)
#define SPI_CTL_DELAYCHAIN_MASK		(0x1f << 15)
#define SPI_CTL_DELAYCHAIN_SHIFT	(15)
#define SPI_CTL_DELAYCHAIN(x)		((x) << 15)

#define SPI_STATUS_RX_HALF_FULL		(1 << 13)
#define SPI_STATUS_TX_HALF_FULL		(1 << 12)
#else
#define SPI_CTL_DELAYCHAIN_MASK		(0xf << 16)
#define SPI_CTL_DELAYCHAIN_SHIFT	(16)
#define SPI_CTL_DELAYCHAIN(x)		((x) << 16)
#endif

#define SPI_CTL_REQ_MASK		(0x0 << 15)
#define SPI_CTL_REQ_DISABLE		(0x0 << 15)
#define SPI_CTL_REQ_ENABLE		(0x1 << 15)

#define SPI_CTL_QPIEN_MASK			(0x0 << 14)
#define SPI_CTL_QPIEN_DISABLE		(0x0 << 14)
#define SPI_CTL_QPIEN_ENABLE		(0x1 << 14)

#define SPI_CTL_TOUT_CTRL_MASK			(0x0 << 12)
#define SPI_CTL_TOUT_CTRL_(x)			((x) << 12)

#define SPI_CTL_IO_MODE_MASK			(0x3 << 10)
#define SPI_CTL_IO_MODE_(x)				((x) << 10)


#define SPI_CTL_TX_IRQ_EN		(1 << 9)
#define SPI_CTL_RX_IRQ_EN		(1 << 8)
#define SPI_CTL_TX_DRQ_EN		(1 << 7)
#define SPI_CTL_RX_DRQ_EN		(1 << 6)
#define SPI_CTL_TX_FIFO_EN		(1 << 5)
#define SPI_CTL_RX_FIFO_EN		(1 << 4)
#define SPI_CTL_SS				(1 << 3)
#define SPI_CTL_LOOP			(1 << 2)
#define SPI_CTL_WR_MODE_MASK		(0x3 << 0)
#define SPI_CTL_WR_MODE_DISABLE		(0x0 << 0)
#define SPI_CTL_WR_MODE_READ		(0x1 << 0)
#define SPI_CTL_WR_MODE_WRITE		(0x2 << 0)
#define SPI_CTL_WR_MODE_READ_WRITE	(0x3 << 0)

#define SPI_STATUS_TX_FIFO_WERR		(1 << 11)
#define SPI_STATUS_RX_FIFO_WERR		(1 << 9)
#define SPI_STATUS_RX_FIFO_RERR		(1 << 8)
#define SPI_STATUS_RX_FULL		(1 << 7)
#define SPI_STATUS_RX_EMPTY		(1 << 6)
#define SPI_STATUS_TX_FULL		(1 << 5)
#define SPI_STATUS_TX_EMPTY		(1 << 4)
#define SPI_STATUS_TX_IRQ_PD		(1 << 3)
#define SPI_STATUS_RX_IRQ_PD		(1 << 2)
#define SPI_STATUS_BUSY			(1 << 0)
#define SPI_STATUS_ERR_MASK		(SPI_STATUS_RX_FIFO_RERR |	\
                     SPI_STATUS_RX_FIFO_WERR |	\
                     SPI_STATUS_TX_FIFO_WERR)

#define SPI_DMA_STRANFER_MIN_LEN 8
#define SPI_FIFO_LEN                   64
//#define CONFIG_STANDARD_SPI

struct acts_spi_controller
{
    volatile uint32_t ctrl;
    volatile uint32_t status;
    volatile uint32_t txdat;
    volatile uint32_t rxdat;
    volatile uint32_t bc;
} ;

struct acts_spi_config {
    struct acts_spi_controller *spi;
    uint32_t spiclk_reg;
    const char *dma_dev_name;
    uint8_t txdma_id;
    uint8_t txdma_chan;
    uint8_t rxdma_id;
    uint8_t rxdma_chan;
    uint8_t clock_id;
    uint8_t reset_id;
    uint8_t flag_use_dma:1;
#if defined(CONFIG_SOC_SERIES_LEOPARD)
    uint8_t spi_3wire:1;
    uint8_t delay_chain:6;
#else
    uint8_t delay_chain:7;
#endif
};

struct acts_spi_data {
    struct spi_context ctx;
    const struct device *dma_dev;
    struct k_sem dma_sync;
    uint8_t rxdma_chan;
    uint8_t txdma_chan;
};

#define DEV_CFG(dev) \
    ((const struct acts_spi_config *const)(dev)->config)
#define DEV_DATA(dev) \
    ((struct acts_spi_data *const)(dev)->data)

bool spi_acts_transfer_ongoing(struct acts_spi_data *data)
{
    return spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx);
}

static void spi_acts_wait_tx_complete(struct acts_spi_controller *spi)
{
    while (!(spi->status & SPI_STATUS_TX_EMPTY))
        ;
    /* wait until tx fifo is empty for master mode*/
    while (((spi->ctrl & SPI_CTL_MS_SEL_MASK) == SPI_CTL_MS_SEL_MASTER) &&
        spi->status & SPI_STATUS_BUSY)
        ;
}

static void spi_acts_set_clk(const struct acts_spi_config *cfg, uint32_t freq_khz)
{
#if 1
    /* setup spi1 clock to 78MHz, coreclk/2 */
    //sys_write32((sys_read32(CMU_SPICLK) & ~0xff00) | 0x0100, CMU_SPICLK);

    /* FIXME: call soc_freq_set_spi_freq()
     * setup spi2 clock to 396/6 =66m bps, coreclk/6
     */
    // devclk=180M, spiclk=devclk/2=90M
    clk_set_rate(cfg->clock_id, freq_khz*1000);
    k_busy_wait(100);
#endif
}


static void dma_done_callback(const struct device *dev, void *callback_data, uint32_t ch , int type)
{
    struct acts_spi_data *data = (struct acts_spi_data *)callback_data;

    //if (type != DMA_IRQ_TC)
        //return;

    LOG_DBG("spi dma transfer is done");

    k_sem_give(&data->dma_sync);
}

static int spi_acts_start_dma(const struct acts_spi_config *cfg,
                  struct acts_spi_data *data,
                  uint32_t dma_chan,
                  uint8_t *buf,
                  int32_t len,
                  bool is_tx,
                  dma_callback_t callback)
{
    struct acts_spi_controller *spi = cfg->spi;
    struct dma_config dma_cfg = {0};
    struct dma_block_config dma_block_cfg = {0};

    if (callback) {
        dma_cfg.dma_callback = callback;
        dma_cfg.user_data = data;
        dma_cfg.complete_callback_en = 1;
    }

    dma_cfg.block_count = 1;
    dma_cfg.head_block = &dma_block_cfg;
    dma_block_cfg.block_size = len;
    if (is_tx) {
        dma_cfg.dma_slot = cfg->txdma_id;
        dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
        dma_block_cfg.source_address = (uint32_t)buf;
        dma_block_cfg.dest_address = (uint32_t)&spi->txdat;
        dma_cfg.dest_data_size = 1;
    } else {
        dma_cfg.dma_slot = cfg->rxdma_id;
        dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
        dma_block_cfg.source_address = (uint32_t)&spi->rxdat;
        dma_block_cfg.dest_address = (uint32_t)buf;
        dma_cfg.source_data_size = 1;
    }

    if(len < 8)//data length is too short
    dma_cfg.source_burst_length = 1;

    if (dma_config(data->dma_dev, dma_chan, &dma_cfg)) {
        LOG_ERR("dma%d config error", dma_chan);
        return -1;
    }

    if (dma_start(data->dma_dev, dma_chan)) {
        LOG_ERR("dma%d start error", dma_chan);
        return -1;
    }

    return 0;
}

static void spi_acts_stop_dma(const struct acts_spi_config *cfg,
                   struct acts_spi_data *data,
                   uint32_t dma_chan)
{
    dma_stop(data->dma_dev, dma_chan);
}

static int spi_acts_read_data_by_dma(const struct acts_spi_config *cfg,
                     struct acts_spi_data *data,
                     uint8_t *buf, int32_t len)
{
    struct acts_spi_controller *spi = cfg->spi;
    int ret;

    spi->bc = len;
    spi->ctrl = (spi->ctrl & ~(SPI_CTL_WR_MODE_MASK | SPI_CTL_DMS_MASK)) |
            SPI_CTL_WR_MODE_READ | SPI_CTL_CLK_SEL_DMA |
            SPI_CTL_RX_DRQ_EN;

    if(len < 8)
        spi->ctrl |=  SPI_CTL_DMS_SINGLE;

    ret = spi_acts_start_dma(cfg, data, data->rxdma_chan, buf, len,
                 false, dma_done_callback);
    if (ret) {
        LOG_ERR("faield to start dma chan 0x%x\n", data->rxdma_chan);
        goto out;
    }

    /* wait until dma transfer is done */
    k_sem_take(&data->dma_sync, K_FOREVER);

out:
    spi_acts_stop_dma(cfg, data, data->rxdma_chan);
    spi->ctrl = spi->ctrl & ~(SPI_CTL_CLK_SEL_DMA | SPI_CTL_RX_DRQ_EN);

    return ret;
}

static int spi_acts_write_data_by_dma(const struct acts_spi_config *cfg,
                      struct acts_spi_data *data,
                      const uint8_t *buf, int32_t len)
{
    struct acts_spi_controller *spi = cfg->spi;
    int ret;

    spi->bc = len;
    spi->ctrl = (spi->ctrl & ~(SPI_CTL_WR_MODE_MASK | SPI_CTL_DMS_MASK)) |
            SPI_CTL_WR_MODE_WRITE | SPI_CTL_CLK_SEL_DMA |
            SPI_CTL_TX_DRQ_EN;

    if(len < 8)
        spi->ctrl |=  SPI_CTL_DMS_SINGLE;

    ret = spi_acts_start_dma(cfg, data, data->txdma_chan, (uint8_t *)buf, len,
                 true, dma_done_callback);
    if (ret) {
        LOG_ERR("faield to start tx dma chan 0x%x\n", data->txdma_chan);
        goto out;
    }

    /* wait until dma transfer is done */

    k_sem_take(&data->dma_sync, K_FOREVER);

    /* wait until TX FIFO empty */

    while(!(spi->status & SPI_STATUS_TX_EMPTY));

out:
    spi_acts_stop_dma(cfg, data, data->txdma_chan);
    spi->ctrl = spi->ctrl & ~(SPI_CTL_CLK_SEL_DMA | SPI_CTL_TX_DRQ_EN);

    return ret;
}

int spi_acts_write_read_data_by_dma(const struct acts_spi_config *cfg,
        struct acts_spi_data *data,
        const uint8_t *tx_buf, uint8_t *rx_buf, int32_t len)
{
    struct acts_spi_controller *spi = cfg->spi;
    int ret;

    spi->bc = len;
    spi->ctrl = (spi->ctrl & ~(SPI_CTL_WR_MODE_MASK | SPI_CTL_DMS_MASK)) |
            SPI_CTL_WR_MODE_READ_WRITE | SPI_CTL_CLK_SEL_DMA |
            SPI_CTL_TX_DRQ_EN | SPI_CTL_RX_DRQ_EN;

    if(len < 8)
        spi->ctrl |=  SPI_CTL_DMS_SINGLE;

    ret = spi_acts_start_dma(cfg, data, data->rxdma_chan, rx_buf, len,
                 false, dma_done_callback);
    if (ret) {
        LOG_ERR("faield to start dma rx chan 0x%x\n", data->rxdma_chan);
        goto out;
    }

    ret = spi_acts_start_dma(cfg, data, data->txdma_chan, (uint8_t *)tx_buf, len,
                 true, NULL);
    if (ret) {
        LOG_ERR("faield to start dma tx chan 0x%x\n", data->txdma_chan);
        goto out;
    }

    /* wait until dma transfer is done */

    k_sem_take(&data->dma_sync, K_FOREVER);

out:
    spi_acts_stop_dma(cfg, data, data->rxdma_chan);
    spi_acts_stop_dma(cfg, data, data->txdma_chan);

    spi->ctrl = spi->ctrl & ~(SPI_CTL_CLK_SEL_DMA |
            SPI_CTL_TX_DRQ_EN | SPI_CTL_RX_DRQ_EN);

    return 0;
}


static int spi_acts_write_data_by_cpu(struct acts_spi_controller *spi,
        const uint8_t *wbuf, int32_t len)
{
    int tx_len = 0;

    /* switch to write mode */
    spi->bc = len;
    spi->ctrl = (spi->ctrl & ~SPI_CTL_WR_MODE_MASK) | SPI_CTL_WR_MODE_WRITE;
    while (tx_len < len) {
        if(!(spi->status & SPI_STATUS_TX_FULL)) {
            spi->txdat = *wbuf++;
            tx_len++;
        }
    }
    return 0;
}

static int spi_acts_read_data_by_cpu(struct acts_spi_controller *spi,
        uint8_t *rbuf, int32_t len)
{
    int rx_len = 0;

    /* switch to write mode */
    spi->bc = len;
    spi->ctrl = (spi->ctrl & ~SPI_CTL_WR_MODE_MASK) | SPI_CTL_WR_MODE_READ;

    while (rx_len < len) {
        if(!(spi->status & SPI_STATUS_RX_EMPTY)) {
            *rbuf++ = spi->rxdat;
            rx_len++;
        }
    }

    return 0;
}

static int spi_acts_write_read_data_by_cpu(struct acts_spi_controller *spi,
        const uint8_t *wbuf, uint8_t *rbuf, int32_t len)
{
    int rx_len = 0, tx_len = 0;
	uint32_t cycle;
    /* switch to write mode */
    spi->bc = len;
    spi->ctrl = (spi->ctrl & ~SPI_CTL_WR_MODE_MASK) | SPI_CTL_WR_MODE_READ_WRITE;
	cycle = k_cycle_get_32();
    while (rx_len < len || tx_len < len) {

        while((tx_len < rx_len + SPI_FIFO_LEN - 2) && (tx_len < len) && !(spi->status & SPI_STATUS_TX_FULL)) {
            spi->txdat = *wbuf++;
            tx_len++;
        }

        while((rx_len < len) && !(spi->status & SPI_STATUS_RX_EMPTY)) {
            *rbuf++ = spi->rxdat;
            rx_len++;
        }

		if(k_cyc_to_ms_ceil32(k_cycle_get_32()-cycle) > 1000){
			LOG_ERR("spi err: txlen=%d, rxlen=%d, bc=0x%x, ctl=0x%x, status=0x%x\n",tx_len, rx_len, spi->bc, spi->ctrl, spi->status);
			break;
		}
    }
    return 0;
}

int spi_acts_write_data(const struct acts_spi_config *cfg, struct acts_spi_data *data,
    const uint8_t *tx_buf, int len)
{
    int ret;

    if (cfg->flag_use_dma) {
        ret = spi_acts_write_data_by_dma(cfg, data, tx_buf, len);
    } else {
        ret = spi_acts_write_data_by_cpu(cfg->spi, tx_buf, len);
    }

    return ret;
}

int spi_acts_read_data(const struct acts_spi_config *cfg, struct acts_spi_data *data,
    uint8_t *rx_buf, int len)
{
    int ret;

    if (cfg->flag_use_dma) {
        ret = spi_acts_read_data_by_dma(cfg, data,
            rx_buf, len);
    } else {
        ret = spi_acts_read_data_by_cpu(cfg->spi,
            rx_buf, len);
    }


    return ret;
}

int spi_acts_write_read_data(const struct acts_spi_config *cfg, struct acts_spi_data *data,
    uint8_t *tx_buf, uint8_t *rx_buf, int len)
{
    int ret;
    if (cfg->flag_use_dma) {
        ret = spi_acts_write_read_data_by_dma(cfg, data, tx_buf, rx_buf, len);
    } else {
		ret = spi_acts_write_read_data_by_cpu(cfg->spi, tx_buf, rx_buf, len);
    }

    return ret;
}

int spi_acts_transfer_data(const struct acts_spi_config *cfg, struct acts_spi_data *data)
{
    struct acts_spi_controller *spi = cfg->spi;
    struct spi_context *ctx = &data->ctx;
    int chunk_size;
    int ret = 0;

    chunk_size = spi_context_longest_current_buf(ctx);

    LOG_DBG("tx_len %d, rx_len %d, chunk_size %d",
        ctx->tx_len, ctx->rx_len, chunk_size);

    spi->ctrl |= SPI_CTL_RX_FIFO_EN | SPI_CTL_TX_FIFO_EN;

    if (ctx->tx_len && ctx->rx_len) {
		#ifdef CONFIG_STANDARD_SPI
		if(ctx->tx_buf && ctx->rx_buf){
			ret = spi_acts_write_read_data(cfg, data, (uint8_t*)ctx->tx_buf, ctx->rx_buf, chunk_size);
		}else if(ctx->tx_buf) {
			ret = spi_acts_write_data(cfg, data, (uint8_t*)ctx->tx_buf, chunk_size);
		}else{
			ret = spi_acts_read_data(cfg, data, ctx->rx_buf, chunk_size);
		}
		spi_context_update_tx(ctx, 1, chunk_size);
		spi_context_update_rx(ctx, 1, chunk_size);
		#else
		if(ctx->tx_buf != NULL) {
			ret = spi_acts_write_data(cfg, data, ctx->tx_buf, ctx->tx_len);
		}
		if (ctx->rx_buf != NULL) {
			ret = spi_acts_read_data(cfg, data, ctx->rx_buf, ctx->rx_len);
        }
        spi_context_update_tx(ctx, 1, ctx->tx_len);
        spi_context_update_rx(ctx, 1, ctx->rx_len);
		#endif

    } else if (ctx->tx_len) {
        ret = spi_acts_write_data(cfg, data, (uint8_t*)ctx->tx_buf, chunk_size);
        spi_context_update_tx(ctx, 1, chunk_size);
    } else {
        ret = spi_acts_read_data(cfg, data, ctx->rx_buf, chunk_size);
        spi_context_update_rx(ctx, 1, chunk_size);
    }
    if (!ret) {
        spi_acts_wait_tx_complete(spi);
        if (spi->status & SPI_STATUS_ERR_MASK) {
            ret = -EIO;
        }
    }
    if (ret) {
        LOG_ERR("spi(%p) transfer error: ctrl: 0x%x, status: 0x%x",
                spi, spi->ctrl, spi->status);
    }

    spi->ctrl = (spi->ctrl & ~SPI_CTL_WR_MODE_MASK) | SPI_CTL_WR_MODE_DISABLE;
    spi->status |= SPI_STATUS_ERR_MASK;

    return ret;
}

int spi_acts_configure(const struct acts_spi_config *cfg,
                struct acts_spi_data *spi,
                const struct spi_config *config)
{
    uint32_t ctrl, word_size;
    uint32_t op = config->operation;

    LOG_DBG("%p (prev %p): op 0x%x", config, spi->ctx.config, op);

    ctrl = SPI_CTL_DELAYCHAIN(cfg->delay_chain);

    if (spi_context_configured(&spi->ctx, config)) {
        /* Nothing to do */
        return 0;
    }

    spi_acts_set_clk(cfg, config->frequency / 1000);
#if defined(CONFIG_SOC_SERIES_LEOPARD)
    if (cfg->clock_id == CLOCK_ID_SPI2)
        ctrl |= SPI_CTL_IO_MODE_(cfg->spi_3wire);
#endif
    if (cfg->clock_id == CLOCK_ID_SPI3){
      if (op & SPI_LINES_DUAL){
          ctrl &= ~SPI_CTL_IO_MODE_MASK;
          ctrl |= SPI_CTL_IO_MODE_(2);
          LOG_INF("SPI_2X MODE==");
      }
      if (op & SPI_LINES_QUAD){
          ctrl &= ~SPI_CTL_IO_MODE_MASK;
          ctrl |= SPI_CTL_IO_MODE_(3);
          LOG_INF("SPI_4X MODE==");
      }
    }
    if (SPI_OP_MODE_SLAVE == SPI_OP_MODE_GET(op))
        ctrl |= SPI_CTL_MS_SEL_SLAVE;

    word_size = SPI_WORD_SIZE_GET(op);
    if (word_size == 8)
        ctrl |= SPI_CTL_FIFO_WIDTH_8BIT;
    else if (word_size == 32)
        ctrl |= SPI_CTL_FIFO_WIDTH_32BIT;
    else
        ctrl |= SPI_CTL_FIFO_WIDTH_8BIT;

    if (op & SPI_MODE_CPOL)
        ctrl |= SPI_CTL_MODE_CPOL;

    if (op & SPI_MODE_CPHA)
        ctrl |= SPI_CTL_MODE_CPHA;

    if (op & SPI_MODE_LOOP)
        ctrl |= SPI_CTL_LOOP;

    if (op & SPI_TRANSFER_LSB)
        ctrl |= SPI_CTL_SB_SEL_LSB;

    if (cfg->clock_id == CLOCK_ID_SPI1)
        ctrl |= SPI_CTL_REQ_ENABLE;

    cfg->spi->ctrl = ctrl;

    /* At this point, it's mandatory to set this on the context! */
    spi->ctx.config = config;

    spi_context_cs_configure(&spi->ctx);

    return 0;
}



int transceive(const struct device *dev,
              const struct spi_config *config,
              const struct spi_buf_set *tx_bufs,
              const struct spi_buf_set *rx_bufs,
              bool asynchronous,
              struct k_poll_signal *signal)
{
    const struct acts_spi_config *cfg =DEV_CFG(dev);;
    struct acts_spi_data *data = DEV_DATA(dev);;
    struct acts_spi_controller *spi = cfg->spi;
    int ret;

	if ((tx_bufs && !tx_bufs->count) && (rx_bufs && !rx_bufs->count)){
        return 0;
    }

    spi_context_lock(&data->ctx, asynchronous, signal);

    /* Configure */
    ret = spi_acts_configure(cfg, data, config);
    if (ret) {
        goto out;
    }

    /* Set buffers info */

    spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

    /* assert chip select */
    if (SPI_OP_MODE_MASTER == SPI_OP_MODE_GET(config->operation)) {
        if (data->ctx.config->cs) {
            spi_context_cs_control(&data->ctx, true);
        } else {
            spi->ctrl &= ~SPI_CTL_SS;
        }
    }
    do {
        ret = spi_acts_transfer_data(cfg, data);
    } while (!ret && spi_acts_transfer_ongoing(data));
    /* deassert chip select */
    if (SPI_OP_MODE_MASTER == SPI_OP_MODE_GET(config->operation)) {
        if (data->ctx.config->cs) {
            spi_context_cs_control(&data->ctx, false);
        } else {
            spi->ctrl |= SPI_CTL_SS;
        }
    }

out:
    spi_context_release(&data->ctx, ret);

    return ret;
}


static int spi_acts_transceive(const struct device *dev,
                  const struct spi_config *config,
                  const struct spi_buf_set *tx_bufs,
                  const struct spi_buf_set *rx_bufs)
{
  return transceive(dev, config, tx_bufs,  rx_bufs, false, NULL);

}

#ifdef CONFIG_SPI_ASYNC
static int spi_acts_transceive_async(const struct device *dev,
                   const struct spi_config *config,
                   const struct spi_buf_set *tx_bufs,
                   const struct spi_buf_set *rx_bufs,
                   struct k_poll_signal *async)
{
    return transceive(dev, config, tx_bufs,  rx_bufs, true, async);

}
#endif /* CONFIG_SPI_ASYNC */


static int spi_acts_release(const struct device *dev,
               const struct spi_config *config)
{
    struct acts_spi_data *data = dev->data;
    spi_context_unlock_unconditionally(&data->ctx);
    return 0;
}

#define SPI_TEST
#ifdef SPI_TEST
static int spi_test(const struct device *dev)
{
    struct spi_config config = {
        .frequency = 2500000,
        .operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8),
        .slave = 0,
		.cs = NULL,

    };

    struct spi_buf_set tx_bufs;
    struct spi_buf_set rx_bufs;
    struct spi_buf tx_buf[1];
    struct spi_buf rx_buf[1];
    u8_t buf_tx[16];
    u8_t buf_rx[48] = {0};
    int ret;

    memset(buf_tx, 0x86, 16);

    printk("spi test:%s\n", dev->name);

    tx_buf[0].buf = buf_tx;
    tx_buf[0].len = 1;
    rx_buf[0].buf = buf_rx;
    rx_buf[0].len = 1;
    rx_bufs.buffers = rx_buf;
    rx_bufs.count = 1;
    tx_bufs.buffers = tx_buf;
    tx_bufs.count = 1;
    ret = spi_transceive(dev, &config, &tx_bufs, &rx_bufs);
    if(ret)
        printk("spi test error\n");
    else
        printk("spi test pass\n");
    printk("buf_rx : 0x%x 0x%x 0x%x 0x%x\n", buf_rx[0], buf_rx[1], buf_rx[14], buf_rx[15]);

    return 0;
}
#endif

int spi_acts_init(const struct device *dev)
{
    const struct acts_spi_config *config = DEV_CFG(dev);
    struct acts_spi_data *data =  DEV_DATA(dev);
    int chan;

    k_sem_init(&data->dma_sync, 0, 1);
    if (config->flag_use_dma) {

        data->dma_dev = device_get_binding(config->dma_dev_name);
        if (!data->dma_dev){
            LOG_ERR("dma-dev binding err:%s\n", config->dma_dev_name);
            return -ENODEV;
        }
        chan = dma_request(data->dma_dev, config->txdma_chan);
        if(chan < 0){
            LOG_ERR("dma-dev txchan config err chan=%d\n", config->txdma_chan);
            return -ENODEV;
        }
        data->txdma_chan = chan;
        chan = dma_request(data->dma_dev, config->rxdma_chan);
        if(chan < 0){
            LOG_ERR("dma-dev rxchan config err chan=%d\n", config->txdma_chan);
            return -ENODEV;
        }
        data->rxdma_chan = chan;

    }
    printk("spi:clkreg=0x%x, dma=%d \n",config->spiclk_reg, config->flag_use_dma);

    /* enable spi controller clock */
    acts_clock_peripheral_enable(config->clock_id);
    /* reset spi controller */
    acts_reset_peripheral(config->reset_id);
    spi_context_unlock_unconditionally(&data->ctx);
#ifdef SPI_TEST
    spi_test(dev);
#endif

    return 0;
}

const struct spi_driver_api spi_acts_driver_api = {
    .transceive = spi_acts_transceive,
#ifdef CONFIG_SPI_ASYNC
    .transceive_async = spi_acts_transceive_async,
#endif
    .release = spi_acts_release,
};

#define  dma_use(n)  	(\
        .dma_dev_name =  CONFIG_DMA_0_NAME, \
        .txdma_id = CONFIG_SPI_##n##_DMA_ID,\
        .txdma_chan = CONFIG_SPI_##n##_TXDMA_CHAN,\
        .rxdma_id = CONFIG_SPI_##n##_DMA_ID,\
        .rxdma_chan = CONFIG_SPI_##n##_RXDMA_CHAN,\
        .flag_use_dma = 1,					\
        )

#define dma_not(n)	(\
        .flag_use_dma = 0, \
         )

#if defined(CONFIG_SOC_SERIES_LEOPARD)
#define SPI_ACTS_DEFINE_CONFIG(n)					\
    static const struct acts_spi_config spi_acts_config_##n = {    \
           .spi = (struct acts_spi_controller *)SPI##n##_REG_BASE,\
           .spiclk_reg = SPI##n##_REG_BASE,\
           .clock_id = CLOCK_ID_SPI##n,\
           .reset_id = RESET_ID_SPI##n,\
           .spi_3wire = 0,\
            COND_CODE_1(CONFIG_SPI_##n##_USE_DMA,dma_use(n), dma_not(n))\
    }
#else
#define SPI_ACTS_DEFINE_CONFIG(n)					\
    static const struct acts_spi_config spi_acts_config_##n = {    \
           .spi = (struct acts_spi_controller *)SPI##n##_REG_BASE,\
           .spiclk_reg = SPI##n##_REG_BASE,\
           .clock_id = CLOCK_ID_SPI##n,\
           .reset_id = RESET_ID_SPI##n,\
            COND_CODE_1(CONFIG_SPI_##n##_USE_DMA,dma_use(n), dma_not(n))\
    }
#endif

#define SPI_ACTS_DEVICE_INIT(n)						\
    static const struct device DEVICE_NAME_GET(spi_acts_##n);   \
    SPI_ACTS_DEFINE_CONFIG(n);					\
    static struct acts_spi_data spi_acts_dev_data_##n = {		\
        SPI_CONTEXT_INIT_LOCK(spi_acts_dev_data_##n, ctx),	\
        SPI_CONTEXT_INIT_SYNC(spi_acts_dev_data_##n, ctx),	\
    };								\
    DEVICE_DEFINE(spi_acts_##n,				\
                CONFIG_SPI_##n##_NAME,				\
                &spi_acts_init, NULL, &spi_acts_dev_data_##n,	\
                &spi_acts_config_##n, POST_KERNEL,		\
                CONFIG_SPI_INIT_PRIORITY, &spi_acts_driver_api);

#if IS_ENABLED(CONFIG_SPI_1)
                SPI_ACTS_DEVICE_INIT(1)
#endif
#if IS_ENABLED(CONFIG_SPI_2)
                SPI_ACTS_DEVICE_INIT(2)
#endif
#if IS_ENABLED(CONFIG_SPI_3)
                SPI_ACTS_DEVICE_INIT(3)
#endif


