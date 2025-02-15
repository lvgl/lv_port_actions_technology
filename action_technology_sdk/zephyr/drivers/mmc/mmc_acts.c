/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <init.h>
#include <device.h>
#include <irq.h>
#include <drivers/dma.h>
#include <drivers/gpio.h>
#include <drivers/mmc/mmc.h>
#include <soc.h>
#include <string.h>
#include <board_cfg.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(mmc_acts, CONFIG_LOG_DEFAULT_LEVEL);


/* timeout define */
#define MMC_CMD_TIMEOUT_MS                (2000)
#define MMC_DAT_TIMEOUT_MS                (500)
#define MMC_DMA_BUFFER_BUSY_TIMEOUT_US    (5000)

/* SD_EN register */
#define SD_EN_DW_SHIFT                    (0)
#define SD_EN_DW(x)                       ((x) << SD_EN_DW_SHIFT)
#define SD_EN_DW_MASK                     SD_EN_DW(0x3)
#define	SD_EN_DW_1BIT                     SD_EN_DW(0x0)
#define	SD_EN_DW_4BIT                     SD_EN_DW(0x1)
#define	SD_EN_DW_8BIT                     SD_EN_DW(0x2) /* ONLY valid in SD0 */
#define SD_EN_SDIO                        BIT(3)
#define SD_EN_FIFO_WIDTH                  BIT(5) /* 0: 32bits 1: 8bits */
#define SD_EN_BUS_SEL_SHIFT               (6)
#define SD_EN_BUS_SEL(x)                  ((x) << SD_EN_BUS_SEL_SHIFT)
#define SD_EN_BUS_SEL_MASK                SD_EN_BUS_SEL(0x1)
#define	SD_EN_BUS_SEL_AHB                 SD_EN_BUS_SEL(0x0)
#define	SD_EN_BUS_SEL_DMA                 SD_EN_BUS_SEL(0x1)
#define SD_EN_ENABLE                      BIT(7)

#define SD_EN_CLK1                        BIT(8) /* 0: clock to pad SD_CLK0; 1: clock to pad SD_CLK1 */

#define SD_EN_SHIFT                       (9)
#define SD_EN_CSS_MASK                    (1 << SD_EN_SHIFT)
#define	SD_EN_CSS_LOW                     (0 << SD_EN_SHIFT)
#define	SD_EN_CSS_HIGH                    (1 << SD_EN_SHIFT)

#define SD_EN_RAND_SEL                    BIT(30) /* 0: USE EFFUSE bits; 1: use register SDx_SEED value */
#define SD_EN_RAND_EN                     BIT(31) /* 0: randomizer disable; 1: randomizer enable */

/* SD_CTL register */
#define SD_CTL_TM_SHIFT                   (0)
#define SD_CTL_TM(x)                      ((x) << SD_CTL_TM_SHIFT)
#define SD_CTL_TM_MASK                    SD_CTL_TM(0xf)
#define	SD_CTL_TM_CMD_NO_RESP             SD_CTL_TM(0x0)
#define	SD_CTL_TM_CMD_6B_RESP             SD_CTL_TM(0x1)
#define	SD_CTL_TM_CMD_17B_RESP            SD_CTL_TM(0x2)
#define	SD_CTL_TM_CMD_6B_RESP_BUSY        SD_CTL_TM(0x3)
#define	SD_CTL_TM_CMD_RESP_DATA_IN        SD_CTL_TM(0x4)
#define	SD_CTL_TM_CMD_RESP_DATA_OUT       SD_CTL_TM(0x5)
#define	SD_CTL_TM_DATA_IN                 SD_CTL_TM(0x6)
#define	SD_CTL_TM_DATA_OUT                SD_CTL_TM(0x7)
#define	SD_CTL_TM_CLK_OUT_ONLY            SD_CTL_TM(0x8)
#define SD_CTL_C7EN                       BIT(5) /* 0: enable CRC7 check; 1: disable CRC7 check */
#define SD_CTL_LBE                        BIT(6) /* enable last block send 8 more clocks */
#define SD_CTL_START                      BIT(7)
#define SD_CTL_TCN_SHIFT                  (8)
#define SD_CTL_TCN(x)                     ((x) << SD_CTL_TCN_SHIFT)
#define SD_CTL_TCN_MASK                   SD_CTL_TCN(0xf)
#define SD_CTL_SCC                        BIT(12)
#define SD_CTL_CMDLEN                     BIT(13) /* 0: don't drives CMD line to low; 1: drive CMD line to low */
#define SD_CTL_WDELAY_SHIFT               (16)
#define SD_CTL_WDELAY(x)                  ((x) << SD_CTL_WDELAY_SHIFT)
#define SD_CTL_WDELAY_MASK                SD_CTL_WDELAY(0xf)
#define SD_CTL_RDELAY_SHIFT               (20)
#define SD_CTL_RDELAY(x)                  ((x) << SD_CTL_RDELAY_SHIFT)
#define SD_CTL_RDELAY_MASK                SD_CTL_RDELAY(0xf)

/* SD_STATE register */
#define SD_STATE_C7ER                     BIT(0)
#define SD_STATE_RC16ER                   BIT(1)
#define SD_STATE_WC16ER                   BIT(2)
#define SD_STATE_CLC                      BIT(3)
#define SD_STATE_CLNR                     BIT(4)
#define SD_STATE_TRANS_IRQ_PD             BIT(5)
#define SD_STATE_TRANS_IRQ_EN             BIT(6)
#define SD_STATE_DAT0S                    BIT(7)
#define SD_STATE_SDIO_IRQ_EN              BIT(8)
#define SD_STATE_SDIO_IRQ_PD              BIT(9)
#define SD_STATE_DAT1S                    BIT(10)
#define SD_STATE_CMDS                     BIT(11)
#define SD_STATE_MEMRDY                   BIT(12)
#define SD_STATE_FIFO_FULL                BIT(12)
#define SD_STATE_FIFO_EMPTY               BIT(13)
#define SD_STATE_FIFO_RESET               BIT(14)
#define SD_STATE_TIMEOUT_ERROR            BIT(15)

#define SD_STATE_ERR_MASK                 (SD_STATE_CLNR | SD_STATE_WC16ER | \
											SD_STATE_RC16ER | SD_STATE_C7ER)
/* SD_CMD register */
#define SD_CMD_MASK                       (0xFF)

/* SD_BLK_SIZE register */
#define SD_BLK_SIZE_MASK                  (0x3FF)

/* SD_BLK_NUM register */
#define SD_BLK_NUM_MASK                   (0xFFFF)

#define MMC_IS_SD0_DEV(x)                 ((uint32_t)(x) == SD0_REG_BASE)

/* mmc hardware controller */
struct acts_mmc_controller {
	volatile uint32_t en;
	volatile uint32_t ctl;
	volatile uint32_t state;
	volatile uint32_t cmd;
	volatile uint32_t arg;
	volatile uint32_t rspbuf[5];
	volatile uint32_t dat;
	volatile uint32_t blk_size;
	volatile uint32_t blk_num;
};

struct acts_mmc_config {
	struct acts_mmc_controller *base;
	void (*irq_config_func)(void);
	const char *dma_dev_name;
	uint8_t clock_id;
	uint8_t reset_id;
	uint8_t clk_sel; /*0 or 1 select clk0 or clk1*/
	uint8_t dma_id;
	uint8_t dma_chan;
	uint8_t flag_use_dma:1;
	uint8_t bus_width:4;
	uint8_t data_reg_width:3;
	uint8_t use_irq_gpio:1;

	uint8_t sdio_irq_gpio;
	gpio_dt_flags_t sdio_sdio_gpio_flags;
	char *sdio_irq_gpio_name;
};

struct acts_mmc_data {
	struct k_sem trans_done;
	const struct device *dma_dev;

	uint32_t capability;

	uint8_t rdelay;
	uint8_t wdelay;
	uint8_t dma_chan;
	uint8_t reserved[1];

#if (CONFIG_MMC_YIELD_WAIT_DMA_DONE == 1)
	struct k_sem dma_sync;
#endif

	void (*sdio_irq_cbk)(void *arg);
	void *sdio_irq_cbk_arg;

	const struct device *sdio_irq_gpio_dev;
	struct gpio_callback sdio_irq_gpio_cb;

	uint8_t device_release_flag : 1; /* device has been released flag such as sd card pluged out */
};

#if IS_ENABLED(CONFIG_MMC_0)||IS_ENABLED(CONFIG_MMC_1)


#if (CONFIG_MMC_ACTS_ERROR_DETAIL == 1)
static void mmc_acts_dump_regs(struct acts_mmc_controller *mmc)
{

	LOG_INF( "** mmc contoller register ** \n");
	LOG_INF("       BASE: %08x\n", (uint32_t)mmc);
	LOG_INF("      SD_EN: %08x\n", mmc->en);
	LOG_INF("     SD_CTL: %08x\n", mmc->ctl);
	LOG_INF("   SD_STATE: %08x\n", mmc->state);
	LOG_INF("     SD_CMD: %08x\n", mmc->cmd);
	LOG_INF("     SD_ARG: %08x\n", mmc->arg);
	LOG_INF(" SD_RSPBUF0: %08x\n", mmc->rspbuf[0]);
	LOG_INF(" SD_RSPBUF1: %08x\n", mmc->rspbuf[1]);
	LOG_INF(" SD_RSPBUF2: %08x\n", mmc->rspbuf[2]);
	LOG_INF(" SD_RSPBUF3: %08x\n", mmc->rspbuf[3]);
	LOG_INF(" SD_RSPBUF4: %08x\n", mmc->rspbuf[4]);
	LOG_INF("SD_BLK_SIZE: %08x\n", mmc->blk_size);
	LOG_INF(" SD_BLK_NUM: %08x\n", mmc->blk_num);
#ifdef CMU_SD0CLK
	LOG_INF(" CMU_SD0CLK: %08x\n", sys_read32(CMU_SD0CLK));
#endif

#ifdef CMU_SD1CLK
	LOG_INF(" CMU_SD1CLK: %08x\n", sys_read32(CMU_SD1CLK));
#endif
}
#endif

static int mmc_acts_check_err(const struct acts_mmc_config *cfg,
			      struct acts_mmc_controller *mmc,
			      uint32_t resp_err_mask)
{
	uint32_t state = mmc->state & resp_err_mask;

	if (!(state & SD_STATE_ERR_MASK))
		return 0;

	if (state & SD_STATE_CLNR)
		LOG_ERR("Command No response");

	if (state & SD_STATE_C7ER)
		LOG_ERR("CRC command response Error");

	if (state & SD_STATE_RC16ER)
		LOG_ERR("CRC Read data Error");

	if (state & SD_STATE_WC16ER)
		LOG_ERR("CRC Write data Error");

	return 1;
}

static void mmc_acts_err_reset(const struct acts_mmc_config *cfg,
			       struct acts_mmc_controller *mmc)
{
	uint32_t en_bak, state_bak;

	en_bak = mmc->en;
	state_bak = mmc->state;

	/* reset mmc controller */
	acts_reset_peripheral(cfg->reset_id);

	mmc->en = en_bak;
	mmc->state = state_bak;
}

static void mmc_acts_trans_irq_setup(struct acts_mmc_controller *mmc,
				     bool enable)
{
	uint32_t key, state;

	key = irq_lock();

	state = mmc->state;

	/* don't clear sdio pending */
	state &= ~SD_STATE_SDIO_IRQ_PD;
	if (enable)
		state |= SD_STATE_TRANS_IRQ_EN;
	else
		state &= ~SD_STATE_TRANS_IRQ_EN;

	mmc->state = state;

	irq_unlock(key);
}

static void mmc_acts_sdio_irq_setup(struct acts_mmc_controller *mmc,
				    bool enable)
{
	uint32_t key, state;

	key = irq_lock();

	state = mmc->state;

	/* don't clear transfer irq pending */
	state &= ~SD_STATE_TRANS_IRQ_PD;
	if (enable)
		state |= SD_STATE_SDIO_IRQ_EN;
	else
		state &= ~SD_STATE_SDIO_IRQ_EN;

	mmc->state = state;

	irq_unlock(key);
}

static void mmc_acts_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct acts_mmc_config *cfg = dev->config;
	struct acts_mmc_data *data = dev->data;
	struct acts_mmc_controller *mmc = cfg->base;
	uint32_t state;

	state = mmc->state;

	LOG_DBG("enter isr: state 0x%x", state);

	if ((state & SD_STATE_TRANS_IRQ_EN) &&
	    ((state & SD_STATE_TRANS_IRQ_PD) ||
	    (state & SD_STATE_ERR_MASK))) {
		k_sem_give(&data->trans_done);
	}

	if ((state & SD_STATE_SDIO_IRQ_EN) &&
	    (state & SD_STATE_SDIO_IRQ_PD)) {
		if (data->sdio_irq_cbk) {
			data->sdio_irq_cbk(data->sdio_irq_cbk_arg);
		}
	}

	/* clear irq pending, keep the error bits */
	mmc->state = state & (SD_STATE_TRANS_IRQ_EN | SD_STATE_SDIO_IRQ_EN |
			      SD_STATE_TRANS_IRQ_PD | SD_STATE_SDIO_IRQ_PD);
}

static int mmc_acts_get_trans_mode(struct mmc_cmd *cmd, uint32_t *trans_mode,
				   uint32_t *rsp_err_mask)
{
	uint32_t mode =0, err_mask = 0;

	switch (mmc_resp_type(cmd)) {
	case MMC_RSP_NONE:
		mode = SD_CTL_TM_CMD_NO_RESP;
		break;

	case MMC_RSP_R1:
		if (cmd->buf) {
			if (cmd->flags & MMC_DATA_READ)
				mode = SD_CTL_TM_CMD_RESP_DATA_IN;
			else if (cmd->flags & MMC_DATA_WRITE)
				mode = SD_CTL_TM_CMD_RESP_DATA_OUT;
			else if (cmd->flags & MMC_DATA_WRITE_DIRECT)
				mode = SD_CTL_TM_DATA_OUT;
			else if (cmd->flags & MMC_DATA_READ_DIRECT)
				mode = SD_CTL_TM_DATA_IN;
		} else {
			mode = SD_CTL_TM_CMD_6B_RESP;
		}
		err_mask = SD_STATE_CLNR | SD_STATE_C7ER | SD_STATE_RC16ER |
			   SD_STATE_WC16ER;

		break;

	case MMC_RSP_R1B:
		mode = SD_CTL_TM_CMD_6B_RESP_BUSY;
		err_mask = SD_STATE_CLNR | SD_STATE_C7ER;
		break;

	case MMC_RSP_R2:
		mode = SD_CTL_TM_CMD_17B_RESP;
		err_mask = SD_STATE_CLNR | SD_STATE_C7ER;
		break;

	case MMC_RSP_R3:
		mode = SD_CTL_TM_CMD_6B_RESP;
		err_mask = SD_STATE_CLNR;
		break;

	default:
		LOG_ERR("unsupported RSP 0x%x\n", mmc_resp_type(cmd));
		return -ENOTSUP;
	}

	if (trans_mode)
		*trans_mode = mode;

	if (rsp_err_mask)
		*rsp_err_mask = err_mask;

	return 0;
}

static int mmc_acts_wait_cmd(struct acts_mmc_controller *mmc, int timeout_ms)
{
	uint32_t start_time, curr_time;

	start_time = k_cycle_get_32();
	while (mmc->ctl & SD_CTL_START) {
		curr_time = k_cycle_get_32();
		if (k_cyc_to_us_floor32(curr_time - start_time)
			>= (timeout_ms * 1000)) {
			LOG_ERR("mmc cmd timeout");
			return -ETIMEDOUT;
		}
	}

#if (CONFIG_MMC_WAIT_DAT1_BUSY == 1)
	start_time = k_cycle_get_32();
	while ((mmc->state & SD_STATE_DAT1S) == 0) {
		curr_time = k_cycle_get_32();
		if (k_cyc_to_us_floor32(curr_time - start_time)
			>= (timeout_ms * 1000)) {
			LOG_ERR("mmc dat1 timeout");
			return -ETIMEDOUT;
		}
	}
#endif

	return 0;
}

static bool mmc_acts_data_is_ready(struct acts_mmc_controller *mmc,
				   bool is_write, bool use_fifo)
{
	uint32_t state = mmc->state;

	if (use_fifo)
		if (is_write)
			return (!(state & SD_STATE_FIFO_FULL));
		else
			return (!(state & SD_STATE_FIFO_EMPTY));
	else
		return (state & SD_STATE_MEMRDY);
}

static int mmc_acts_transfer_by_cpu(const struct device *dev,
				    bool is_write, uint8_t *buf,
				    int len, uint32_t timeout_ms)
{
	const struct acts_mmc_config *cfg = dev->config;
	struct acts_mmc_controller *mmc = cfg->base;
	uint32_t start_time, curr_time;
	uint32_t data, data_len;

	mmc->ctl |= SD_CTL_START;

	start_time = k_cycle_get_32();
	while (len > 0) {
		if (mmc_acts_check_err(cfg, mmc, SD_STATE_CLNR)) {
			return -EIO;
		}
		if (mmc_acts_data_is_ready(mmc, is_write, CONFIG_MMC_STATE_FIFO)) {
			data_len = len < cfg->data_reg_width ? len : cfg->data_reg_width;

			if (is_write) {
				if (((uint32_t)buf & 0x3) || data_len < cfg->data_reg_width)
					memcpy(&data, buf, data_len);
				else
					data = *((uint32_t *)buf);

				mmc->dat = data;
			} else {
				data = mmc->dat;

				if (((uint32_t)buf & 0x3) || data_len < cfg->data_reg_width)
					memcpy(buf, &data, data_len);
				else
					*((uint32_t *)buf) = data;
			}

			buf += data_len;
			len -= data_len;
		}

		curr_time = k_cycle_get_32();
		if (k_cyc_to_us_floor32(curr_time - start_time)
			>= (timeout_ms * 1000)) {
			LOG_ERR("mmc io timeout, is_write %d", is_write);
			return -ETIMEDOUT;
		}
	}

	return 0;
}

#define DMA_IRQ_TC                           (0) /* DMA completion flag */
#define DMA_IRQ_HF                           (1) /* DMA half-full flag */
static void dma_done_callback(const struct device *dev, void *callback_data, uint32_t ch , int type)
{
	struct acts_mmc_data *data = (struct acts_mmc_data *)callback_data;

	ARG_UNUSED(dev);
	ARG_UNUSED(ch);

	if (type != DMA_IRQ_TC)
		return;

	LOG_DBG("mmc dma transfer is done:0x%x\n", (u32_t)data);

	k_sem_give(&data->dma_sync);
}

#ifdef CONFIG_ACTIONS_PRINTK_DMA
static int mmc_dma_wait_timeout(const struct device *dma_dev, uint32_t dma_chan, uint32_t timeout_us)
{
	uint32_t start_time, curr_time;
	struct dma_status stat = {0};
	int ret;

	start_time = k_cycle_get_32();

	while (1) {
		ret = dma_get_status(dma_dev, dma_chan, &stat);
		if (ret) {
			LOG_ERR("get dma(%d) status error %d\n", dma_chan, ret);
			return -EFAULT;
		}

		/* DMA transfer finish */
		if (!stat.pending_length)
			break;

		curr_time = k_cycle_get_32();
		if (k_cyc_to_us_floor32(curr_time - start_time) >= timeout_us) {
			LOG_ERR("wait mmc dma(%d) finish timeout", dma_chan);
			return -ETIMEDOUT;
		}
	}

	return 0;
}
#endif

static int mmc_acts_transfer_by_dma(const struct device *dev,
				    bool is_write, uint8_t *buf,
				    int len, uint32_t timeout_ms)
{
	const struct acts_mmc_config *cfg = dev->config;
	struct acts_mmc_data *data = dev->data;
	struct acts_mmc_controller *mmc = cfg->base;
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_block_cfg = {0};
	int err;

	data->device_release_flag = 0;

#if (CONFIG_MMC_YIELD_WAIT_DMA_DONE == 1)
	dma_cfg.dma_callback = dma_done_callback;
	dma_cfg.user_data = data;
	dma_cfg.complete_callback_en = 1;
#endif

	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_block_cfg;
	dma_block_cfg.block_size = len;

	/* SD0 FIFO width can select 8bits and 32bits  */
	if (MMC_IS_SD0_DEV(mmc)) {
		if (mmc->en & SD_EN_FIFO_WIDTH)
			dma_cfg.dest_data_size = 1; /* fifo width is 8bit */
		else
			dma_cfg.dest_data_size = 4; /* fifo width is 32bit */
	} else {
		/* SD1 FIFO width is fixed 8bits */
		dma_cfg.dest_data_size = 1;
	}

	if (is_write) {
		dma_cfg.dma_slot = cfg->dma_id;
		dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
		dma_block_cfg.source_address = (uint32_t)buf;
		dma_block_cfg.dest_address = (uint32_t)&mmc->dat;
	} else {
		dma_cfg.dma_slot = cfg->dma_id;
		dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
		dma_block_cfg.source_address = (uint32_t)&mmc->dat;
		dma_block_cfg.dest_address = (uint32_t)buf;
	}

	if (dma_config(data->dma_dev, data->dma_chan, &dma_cfg)) {
		LOG_ERR("dma%d config error\n", data->dma_chan);
		return -1;
	}

	if (dma_start(data->dma_dev, data->dma_chan)) {
		LOG_ERR("dma%d start error\n", data->dma_chan);
		return -1;
	}

	mmc->en |= SD_EN_BUS_SEL_DMA;
	mmc_acts_trans_irq_setup(mmc, true);

	/* start mmc controller state machine */
	mmc->ctl |= SD_CTL_START;

	/* wait until data transfer is done */
	err = k_sem_take(&data->trans_done, K_MSEC(timeout_ms));

	if (data->device_release_flag) {
		err = -ENXIO;
	} else {
		if(!err) {
#if (CONFIG_MMC_YIELD_WAIT_DMA_DONE == 1)
			err = k_sem_take(&data->dma_sync, K_MSEC(timeout_ms));
#else
			err = mmc_dma_wait_timeout(data->dma_dev, data->dma_chan,
					MMC_DMA_BUFFER_BUSY_TIMEOUT_US);
#endif
		}

		if (!err) {
			/* wait controller idle */
			err = mmc_acts_wait_cmd(mmc, timeout_ms);
		}
	}

	mmc_acts_trans_irq_setup(mmc, false);
	mmc->en &= ~SD_EN_BUS_SEL_DMA;
	dma_stop(data->dma_dev, data->dma_chan);

	return err;
}

#ifdef CONFIG_ACTIONS_PRINTK_DMA
static int mmc_acts_transfer_by_query(const struct device *dev,
				    bool is_write, uint8_t *buf,
				    int len, uint32_t timeout_ms)
{
	const struct acts_mmc_config *cfg = dev->config;
	struct acts_mmc_data *data = dev->data;
	struct acts_mmc_controller *mmc = cfg->base;
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_block_cfg = {0};
	int err = 0;

	data->device_release_flag = 0;

	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_block_cfg;
	dma_block_cfg.block_size = len;

	/* SD0 FIFO width can select 8bits and 32bits  */
	if (MMC_IS_SD0_DEV(mmc)) {
		if (mmc->en & SD_EN_FIFO_WIDTH)
			dma_cfg.dest_data_size = 1; /* fifo width is 8bit */
		else
			dma_cfg.dest_data_size = 4; /* fifo width is 32bit */
	} else {
		/* SD1 FIFO width is fixed 8bits */
		dma_cfg.dest_data_size = 1;
	}

	if (is_write) {
		dma_cfg.dma_slot = cfg->dma_id;
		dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
		dma_block_cfg.source_address = (uint32_t)buf;
		dma_block_cfg.dest_address = (uint32_t)&mmc->dat;
	} else {
		dma_cfg.dma_slot = cfg->dma_id;
		dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
		dma_block_cfg.source_address = (uint32_t)&mmc->dat;
		dma_block_cfg.dest_address = (uint32_t)buf;
	}

	if (dma_config(data->dma_dev, data->dma_chan, &dma_cfg)) {
		LOG_ERR("dma%d config error\n", data->dma_chan);
		return -1;
	}

	if (dma_start(data->dma_dev, data->dma_chan)) {
		LOG_ERR("dma%d start error\n", data->dma_chan);
		return -1;
	}

	mmc->en |= SD_EN_BUS_SEL_DMA;

	/* start mmc controller state machine */
	mmc->ctl |= SD_CTL_START;

	if (data->device_release_flag) {
		err = -ENXIO;
	} else {
		if(!err) {
			err = mmc_dma_wait_timeout(data->dma_dev, data->dma_chan,
					MMC_DMA_BUFFER_BUSY_TIMEOUT_US);
		}

		if (!err) {
			/* wait controller idle */
			err = mmc_acts_wait_cmd(mmc, timeout_ms);
		}
	}

	mmc->en &= ~SD_EN_BUS_SEL_DMA;
	dma_stop(data->dma_dev, data->dma_chan);

	return err;
}
#endif

static int mmc_acts_send_cmd(const struct device *dev, struct mmc_cmd *cmd)
{
	const struct acts_mmc_config *cfg = dev->config;
	struct acts_mmc_data *data = dev->data;
	struct acts_mmc_controller *mmc = cfg->base;
	int is_write = cmd->flags & (MMC_DATA_WRITE | MMC_DATA_WRITE_DIRECT);
	uint32_t ctl, rsp_err_mask, len, trans_mode;
	int err, timeout;
#ifdef CONFIG_ACTIONS_PRINTK_DMA
	extern int check_panic_exe(void);
#endif
	LOG_DBG("CMD%02d: arg 0x%x flags 0x%x is_write %d \n",
		cmd->opcode, cmd->arg, cmd->flags, !!is_write);
	LOG_DBG("       blk_num 0x%x blk_size 0x%x, buf %p \n",
		cmd->blk_num, cmd->blk_size, cmd->buf);
	trans_mode = 0;
	err = mmc_acts_get_trans_mode(cmd, &trans_mode, &rsp_err_mask);
	if (err) {
		return err;
	}

	ctl = trans_mode | SD_CTL_RDELAY(data->rdelay) |
	      SD_CTL_WDELAY(data->wdelay);

	if ((cmd->buf) && !(cmd->flags & MMC_LBE_DIS))
		ctl |= SD_CTL_LBE;

	/* sdio wifi need continues clock */
	if (data->capability & MMC_CAP_SDIO_IRQ)
		ctl |= SD_CTL_SCC;

#if (CONFIG_MMC_SD0_FIFO_WIDTH_8BITS != 1)
	if (MMC_IS_SD0_DEV(mmc)){
		if(((u32_t)cmd->buf & 0x3))
			mmc->en |= SD_EN_FIFO_WIDTH;
		else
			mmc->en &= ~SD_EN_FIFO_WIDTH;
	}
#endif


	mmc->ctl = ctl;
	mmc->arg = cmd->arg;
	mmc->cmd = cmd->opcode;
	mmc->blk_num = cmd->blk_num;
	mmc->blk_size = cmd->blk_size;

	if (!cmd->buf) {
		/* only command need to transfer */
		mmc->ctl |= SD_CTL_START;
	} else {
		/* command with data transfer */
		len = cmd->blk_num * cmd->blk_size;
		timeout = cmd->blk_num * MMC_DAT_TIMEOUT_MS;
		/* When SD0 FIFO and DMA width is 32bits, data address in memory shall align by 32bits */
#ifdef CONFIG_ACTIONS_PRINTK_DMA
		if (k_is_in_isr() && check_panic_exe()) {
			err = mmc_acts_transfer_by_query(dev, is_write,
				cmd->buf, len, timeout);
		} else
#endif
		if (!cfg->flag_use_dma) {
			err = mmc_acts_transfer_by_cpu(dev, is_write,
				cmd->buf, len, timeout);
		} else {
			err = mmc_acts_transfer_by_dma(dev, is_write,
				cmd->buf, len, timeout);
		}
	}

	err |= mmc_acts_wait_cmd(mmc, MMC_CMD_TIMEOUT_MS);
	err |= mmc_acts_check_err(cfg, mmc, rsp_err_mask);
	if (err) {
		/*
		 * FIXME: the operation of detecting card by polling maybe
		 * output no reponse error message periodically. So filter
		 * it out by config.
		 */
#if (CONFIG_MMC_ACTS_ERROR_DETAIL == 1)
		LOG_ERR("send cmd%d error, state 0x%x \n",
			cmd->opcode, mmc->state);
		mmc_acts_dump_regs(mmc);
#endif
		mmc_acts_err_reset(cfg, mmc);
		return -EIO;
	}

	/* process responses */
	if (!cmd->buf && (cmd->flags & MMC_RSP_PRESENT)) {
		if (cmd->flags & MMC_RSP_136) {
			/* MSB first */
			cmd->resp[3] = mmc->rspbuf[0];
			cmd->resp[2] = mmc->rspbuf[1];
			cmd->resp[1] = mmc->rspbuf[2];
			cmd->resp[0] = mmc->rspbuf[3];
		} else {
			cmd->resp[0] = (mmc->rspbuf[1] << 24) |
				(mmc->rspbuf[0] >> 8);
			cmd->resp[1] = (mmc->rspbuf[1] << 24) >> 8;
		}
	}
	return 0;
}

static int mmc_acts_set_sdio_irq_cbk(const struct device *dev,
				      sdio_irq_callback_t callback,
				      void *arg)
{
	struct acts_mmc_data *data = dev->data;

	if (!(data->capability & MMC_CAP_SDIO_IRQ))
		return -ENOTSUP;

	data->sdio_irq_cbk = callback;
	data->sdio_irq_cbk_arg = arg;

	return 0;
}

static void sdio_irq_gpio_callback(const struct device *port,
				   struct gpio_callback *cb,
				   uint32_t pins)
{
	struct acts_mmc_data *data =
		CONTAINER_OF(cb, struct acts_mmc_data, sdio_irq_gpio_cb);

	ARG_UNUSED(pins);

	if (data->sdio_irq_cbk) {
		data->sdio_irq_cbk(data->sdio_irq_cbk_arg);
	}
}

static void mmc_acts_sdio_irq_gpio_setup(const struct device *dev, bool enable)
{
	const struct acts_mmc_config *cfg = dev->config;
	struct acts_mmc_data *data = dev->data;

	if (enable)
		gpio_pin_interrupt_configure(data->sdio_irq_gpio_dev,
					 cfg->sdio_irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	else
		gpio_pin_interrupt_configure(data->sdio_irq_gpio_dev,
					  cfg->sdio_irq_gpio, GPIO_INT_EDGE_TO_INACTIVE);
}

static int mmc_acts_enable_sdio_irq(const struct device *dev, bool enable)
{
	const struct acts_mmc_config *cfg = dev->config;
	struct acts_mmc_data *data = dev->data;
	struct acts_mmc_controller *mmc = cfg->base;

	if (!(data->capability & MMC_CAP_SDIO_IRQ))
		return -ENOTSUP;

	if (data->capability & MMC_CAP_4_BIT_DATA) {
		mmc_acts_sdio_irq_setup(mmc, enable);
		mmc->en |= SD_EN_SDIO;
	} else {
		if(cfg->use_irq_gpio){
			mmc_acts_sdio_irq_gpio_setup(dev, enable);
		} else {
			LOG_ERR("enable_sdio_irq fail\n");
		}
	}
	return 0;
}

static int mmc_acts_set_bus_width(const struct device *dev, unsigned int bus_width)
{
	const struct acts_mmc_config *cfg = dev->config;
	struct acts_mmc_controller *mmc = cfg->base;

	LOG_DBG("bus_width=%d\n", bus_width);
	if (bus_width == MMC_BUS_WIDTH_1) {
		mmc->en &= ~SD_EN_DW_MASK;
		mmc->en |= SD_EN_DW_1BIT;
	} else if (bus_width == MMC_BUS_WIDTH_4) {
		mmc->en &= ~SD_EN_DW_MASK;
		mmc->en |= SD_EN_DW_4BIT;
	} else if (bus_width == MMC_BUS_WIDTH_8) {
		mmc->en &= ~SD_EN_DW_MASK;
		mmc->en |= SD_EN_DW_8BIT;
	} else {
		return -EINVAL;
	}

	return 0;
}

#ifdef CONFIG_SOC_SERIES_LEOPARD_FPGA
#define LEOPARD_FPGA_MMC_CLK_MAX_HZ	(1*1000*1000)	//sram-6*1000*1000
#endif

static int mmc_acts_set_clock(const struct device *dev, unsigned int rate_hz)
{
	const struct acts_mmc_config *cfg = dev->config;
	struct acts_mmc_data *data = dev->data;
	uint32_t rdelay, wdelay;

#ifdef CONFIG_SOC_SERIES_LEOPARD_FPGA
	if (rate_hz > LEOPARD_FPGA_MMC_CLK_MAX_HZ)
		rate_hz = LEOPARD_FPGA_MMC_CLK_MAX_HZ;
#endif

	/*
	 * Set the RDELAY and WDELAY based on the sd clk.
	 */
	 if (rate_hz < 200000) {
		rdelay = 0xa;
		wdelay = 0xa;
	} /*else if (rate_hz <= 15000000) {
		rdelay = 0xa;
		wdelay = 0xf;
	 } else if (rate_hz <= 30000000) {

		rdelay = 0x8;
		wdelay = 0x9;
	 }*/ else {
		rdelay = 0x8;
		wdelay = 0x8;
	 }
	clk_set_rate(cfg->clock_id, rate_hz);
	/* config delay chain */
	data->rdelay = rdelay;
	data->wdelay = wdelay;

	return 0;
}

static uint32_t mmc_acts_get_capability(const struct device *dev)
{
	struct acts_mmc_data *data = dev->data;
	return data->capability;
}

static int mmc_acts_release_device(const struct device *dev)
{
	struct acts_mmc_data *data = dev->data;

	u32_t key = irq_lock();
	data->device_release_flag = 1;
	k_sem_give(&data->trans_done);
	k_sem_reset(&data->trans_done);
	irq_unlock(key);

	return 0;
}

static const struct mmc_driver_api mmc_acts_driver_api = {
	.get_capability = mmc_acts_get_capability,
	.set_clock = mmc_acts_set_clock,
	.set_bus_width = mmc_acts_set_bus_width,
	.send_cmd = mmc_acts_send_cmd,

	.set_sdio_irq_callback = mmc_acts_set_sdio_irq_cbk,
	.enable_sdio_irq = mmc_acts_enable_sdio_irq,

	.release_device = mmc_acts_release_device,
};

static int mmc_acts_init(const struct device *dev)
{
	const struct acts_mmc_config *cfg = dev->config;
	struct acts_mmc_data *data = dev->data;
	struct acts_mmc_controller *mmc = cfg->base;
	int chan;

	LOG_INF("mmc_acts_init\n");
	if(cfg->flag_use_dma) {
		data->dma_dev = device_get_binding(cfg->dma_dev_name);
		if (!data->dma_dev) {
			LOG_ERR("cannot found dma device\n");
			return -ENODEV;
		}
		chan = dma_request(data->dma_dev, cfg->dma_chan);
		if(chan < 0){
			LOG_ERR("request dma chan config err chan=%d\n", cfg->dma_chan);
			return -ENODEV;
		}
		data->dma_chan = chan;
		LOG_INF("use dma=%d\n", chan);
	}

	if (cfg->use_irq_gpio) {
		data->sdio_irq_gpio_dev = device_get_binding(cfg->sdio_irq_gpio_name);
		if (!data->sdio_irq_gpio_dev) {
			LOG_ERR("cannot found sdio irq gpio dev device %s",
				cfg->sdio_irq_gpio_name);
			return -ENODEV;
		}

		/* Configure IRQ pin and the IRQ call-back/handler */
		gpio_pin_configure(data->sdio_irq_gpio_dev,
				   cfg->sdio_irq_gpio,
				GPIO_INPUT | GPIO_INT_EDGE_TO_INACTIVE | cfg->sdio_sdio_gpio_flags);

		gpio_init_callback(&data->sdio_irq_gpio_cb,
				   sdio_irq_gpio_callback,
				   BIT(cfg->sdio_irq_gpio));

		if (gpio_add_callback(data->sdio_irq_gpio_dev,
				      &data->sdio_irq_gpio_cb)) {
			LOG_ERR("add sdio irq fun fail dev device %s",
				cfg->sdio_irq_gpio_name);

			return -EINVAL;
		}

	}

	/* enable mmc controller clock */
	acts_clock_peripheral_enable(cfg->clock_id);

	/* reset mmc controller */
	acts_reset_peripheral(cfg->reset_id);

	/* set initial clock */
	mmc_acts_set_clock((struct device *)dev, 100000);

	/* enable mmc controller */
	if(cfg->clk_sel)
		mmc->en |= SD_EN_ENABLE | SD_EN_CLK1;
	else
		mmc->en |= SD_EN_ENABLE;

#if (CONFIG_MMC_SD0_FIFO_WIDTH_8BITS == 1)
	if (MMC_IS_SD0_DEV(mmc))
		mmc->en |= SD_EN_FIFO_WIDTH;
#endif

	k_sem_init(&data->trans_done, 0, 1);

#if (CONFIG_MMC_YIELD_WAIT_DMA_DONE == 1)
	k_sem_init(&data->dma_sync, 0, 1);
#endif

	if (MMC_IS_SD0_DEV(mmc)) {
#if (CONFIG_MMC_0_BUS_WIDTH == 8)
		data->capability = (MMC_CAP_8_BIT_DATA | MMC_CAP_4_BIT_DATA | MMC_CAP_SD_HIGHSPEED);
#elif (CONFIG_MMC_0_BUS_WIDTH == 4)
		data->capability = (MMC_CAP_4_BIT_DATA | MMC_CAP_SD_HIGHSPEED);
#else
		data->capability = MMC_CAP_SD_HIGHSPEED;
#endif

#if (CONFIG_MMC_0_ENABLE_SDIO_IRQ == 1)
		data->capability |= MMC_CAP_SDIO_IRQ;
#endif
	} else {
#if (CONFIG_MMC_1_BUS_WIDTH == 4)
		data->capability = (MMC_CAP_4_BIT_DATA | MMC_CAP_SD_HIGHSPEED);
#else
		data->capability = MMC_CAP_SD_HIGHSPEED;
#endif

#if (CONFIG_MMC_1_ENABLE_SDIO_IRQ == 1)
		data->capability |= MMC_CAP_SDIO_IRQ;
#endif
	}

	cfg->irq_config_func();
	return 0;
}

#ifdef CONFIG_PM_DEVICE
int mmc_acts_resume_controller(const struct device *dev)
{
	const struct acts_mmc_config *cfg = dev->config;
	struct acts_mmc_controller *mmc = cfg->base;

	/* reset mmc controller */
	acts_reset_peripheral(cfg->reset_id);

	/* enable mmc controller */
	if(cfg->clk_sel)
		mmc->en |= SD_EN_ENABLE | SD_EN_CLK1;
	else
		mmc->en |= SD_EN_ENABLE;

#if (CONFIG_MMC_SD0_FIFO_WIDTH_8BITS == 1)
	if (MMC_IS_SD0_DEV(mmc))
		mmc->en |= SD_EN_FIFO_WIDTH;
#endif

	return 0;
}

int mmc_acts_pm_control(const struct device *dev, enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		break;
	case PM_DEVICE_ACTION_RESUME:
		/* mmc regs need resume in condition of cpu pwrgating */
		mmc_acts_resume_controller(dev);
		break;
	default:
		break;
	}

	return ret;
}
#else
#define mmc_acts_pm_control 	NULL
#endif

#define dma_use(n)  	(\
		.dma_dev_name =  CONFIG_DMA_0_NAME, \
		.dma_id = CONFIG_MMC_##n##_DMA_ID,\
		.dma_chan = CONFIG_MMC_##n##_DMA_CHAN,\
		.flag_use_dma = 1,					\
		)

#define dma_not(n)	(\
		.flag_use_dma = 0, \
		 )

#define  gpio_irq_use(n)  	(\
		.sdio_irq_gpio_name =  CONFIG_MMC_##n##_GPIO_IRQ_DEV, \
		.sdio_irq_gpio = CONFIG_MMC_##n##_GPIO_IRQ_NUM,\
		.sdio_sdio_gpio_flags = CONFIG_MMC_##n##_GPIO_IRQ_FLAG,\
		.use_irq_gpio = 1,					\
		)

#define gpio_irq_not(n)	(\
		.use_irq_gpio = 0, \
		 )

#define MMC_ACTS_DEFINE_CONFIG(n)					\
	static const struct device DEVICE_NAME_GET(mmc##n##_acts);		\
	static void mmc##n##_acts_irq_config(void)			\
	{								\
		IRQ_CONNECT(IRQ_ID_SD##n, CONFIG_MMC_##n##_IRQ_PRI,	\
				mmc_acts_isr,				\
				DEVICE_GET(mmc##n##_acts), 0);		\
		irq_enable(IRQ_ID_SD##n);				\
	}												\
	static const struct acts_mmc_config mmc_acts_config_##n = {    \
		   .base = (struct acts_mmc_controller *)SD##n##_REG_BASE,\
		   .irq_config_func = mmc##n##_acts_irq_config,			\
		   .clock_id = CLOCK_ID_SD##n,\
		   .reset_id = RESET_ID_SD##n,\
		   .clk_sel = CONFIG_MMC_##n##_CLKSEL,\
		   .bus_width = CONFIG_MMC_##n##_BUS_WIDTH,\
		   .data_reg_width = CONFIG_MMC_##n##_DATA_REG_WIDTH,\
		    COND_CODE_1(CONFIG_MMC_##n##_USE_DMA, dma_use(n), dma_not(n))\
		    COND_CODE_1(CONFIG_MMC_##n##_USE_GPIO_IRQ, gpio_irq_use(n), gpio_irq_not(n))\
	}

//irq-gpios
#define MMC_ACTS_DEVICE_INIT(n)						\
	MMC_ACTS_DEFINE_CONFIG(n);					\
	static struct acts_mmc_data mmc_acts_dev_data_##n ;	\
	DEVICE_DEFINE(mmc##n##_acts,				\
			    CONFIG_MMC_##n##_NAME,				\
			    &mmc_acts_init, mmc_acts_pm_control, &mmc_acts_dev_data_##n,	\
			    &mmc_acts_config_##n, POST_KERNEL,		\
			    20, &mmc_acts_driver_api);

#if IS_ENABLED(CONFIG_MMC_0)
	MMC_ACTS_DEVICE_INIT(0)
#endif

#if IS_ENABLED(CONFIG_MMC_1)
	MMC_ACTS_DEVICE_INIT(1)
#endif

#endif  //#if IS_ENABLED(CONFIG_MMC_0)||IS_ENABLED(CONFIG_MMC_1)


