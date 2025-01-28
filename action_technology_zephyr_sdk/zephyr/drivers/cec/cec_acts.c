/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief HDMI CEC driver for Actions SoC
 */

#include <errno.h>
#include <kernel.h>
#include <drivers/cec.h>
#include <soc.h>
#include <board_cfg.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(cec0, CONFIG_LOG_DEFAULT_LEVEL);

#ifndef BIT
#define BIT(n)  (1UL << (n))
#endif

#define CEC_TIMEOUT_MS						(2000)	/* transact timeout 2000ms */

/* cec cr */
#define CEC_MODE_SHIFT						(30)
#define CEC_MODE_MASK						(0x3 << CEC_MODE_SHIFT)
#define CEC_MODE_ENABLE						(1 << CEC_MODE_SHIFT)
#define CEC_ACKSTATUS						BIT(29)
#define CEC_LOGICAL_ADDR_SHIFT				(24)
#define CEC_LOGICAL_ADDR_MASK				(0xF << CEC_LOGICAL_ADDR_SHIFT)
#define CEC_LOGICAL_ADDR(x)					((x) << CEC_LOGICAL_ADDR_SHIFT)
#define CEC_TIMER_DIV_SHIFT					(16)
#define CEC_TIMER_DIV_MASK					(0xFF << CEC_TIMER_DIV_SHIFT)
#define CEC_TIMER_DIV(x)					((x) << CEC_TIMER_DIV_SHIFT)
#define CEC_PRE_DIV_SHIFT					(8)
#define CEC_PRE_DIV_MASK					(0xFF << CEC_PRE_DIV_SHIFT)
#define CEC_PRE_DIV(x)						((x) << CEC_PRE_DIV_SHIFT)
#define CEC_ACK_MODE						BIT(7)

/* cec rtcr */
#define CEC_PAD_IN							BIT(31)
#define CEC_WT_CNT_SHIFT					(5)
#define CEC_WT_CNT_MASK						(0x3F << CEC_WT_CNT_SHIFT)
#define CEC_LATTEST							BIT(4)
#define CEC_RETRY_NO_SHIFT					(0)
#define CEC_RETRY_NO_MASK					(0xF << CEC_RETRY_NO_SHIFT)
#define CEC_RETRY_NO(x)						((x) << CEC_RETRY_NO_SHIFT)

/* cec rxcr */
#define CEC_RX_MTYPE						BIT(16)
#define CEC_RX_EN							BIT(15)
#define CEC_RX_RST							BIT(14)
#define CEC_RX_CONTINUOUS					BIT(13)
#define CEC_RX_INT_EN						BIT(12)
#define CEC_INIT_ADDR_SHIFT					(8)
#define CEC_INIT_ADDR_MASK					(0xF << CEC_INIT_ADDR_SHIFT)
#define CEC_INIT_ADDR(x)					((x) << CEC_INIT_ADDR_SHIFT)
#define CEC_RX_EOM							BIT(7)
#define CEC_RX_INT							BIT(6)
#define CEC_RX_FIFO_OV						BIT(5)
#define CEC_RX_FIFO_CNT_SHIFT				(0)
#define CEC_RX_FIFO_CNT_MASK				(0x1F << CEC_RX_FIFO_CNT_SHIFT)

/* cec txcr */
#define CEC_TX_ADDR_EN						BIT(20)
#define CEC_TX_ADDR_SHIFT					(16)
#define CEC_TX_ADDR_MASK					(0xF << CEC_TX_ADDR_SHIFT)
#define CEC_TX_ADDR(x)						((x) << CEC_TX_ADDR_SHIFT)
#define CEC_TX_EN							BIT(15)
#define CEC_TX_RST							BIT(14)
#define CEC_TX_CONTINUOUS					BIT(13)
#define CEC_TX_INT_EN						BIT(12)
#define CEC_DEST_ADDR_SHIFT					(8)
#define CEC_DEST_ADDR_MASK					(0xF << CEC_DEST_ADDR_SHIFT)
#define CEC_DEST_ADDR(x)					((x) << CEC_DEST_ADDR_SHIFT)
#define CEC_TX_EOM							BIT(7)
#define CEC_TX_INT							BIT(6)
#define CEC_TX_FIFO_UD						BIT(5)
#define CEC_TX_FIFO_CNT_SHIFT				(0)
#define CEC_TX_FIFO_CNT_MASK				(0x1F << CEC_TX_FIFO_CNT_SHIFT)

/* cec rxtcr */
#define CEC_RX_START_LOW_SHIFT				(24)
#define CEC_RX_START_LOW_MASK				(0xFF << CEC_RX_START_LOW_SHIFT)
#define CEC_RX_START_PERIOD_SHIFT			(16)
#define CEC_RX_START_PERIOD_MASK			(0xFF << CEC_RX_START_PERIOD_SHIFT)
#define CEC_RX_DATA_SAMPLE_SHIFT			(8)
#define CEC_RX_DATA_SAMPLE_MASK				(0xFF << CEC_RX_DATA_SAMPLE_SHIFT)
#define CEC_RX_DATA_PERIOD_SHIFT			(0)
#define CEC_RX_DATA_PERIOD_MASK				(0xFF << CEC_RX_DATA_PERIOD_SHIFT)

/* cec txtcr0 */
#define CEC_TX_START_LOW_SHIFT				(8)
#define CEC_TX_START_LOW_MASK				(0xFF << CEC_TX_START_LOW_SHIFT)
#define CEC_TX_START_HIGH_SHIFT				(0)
#define CEC_TX_START_HIGH_MASK				(0xFF << CEC_TX_START_HIGH_SHIFT)

/* cec txtcr1 */
#define CEC_TX_DATA_LOW_SHIFT				(16)
#define CEC_TX_DATA_LOW_MASK				(0xFF << CEC_TX_DATA_LOW_SHIFT)
#define CEC_TX_DATA_MID_SHIFT				(8)
#define CEC_TX_DATA_MID_MASK				(0xFF << CEC_TX_DATA_01_SHIFT)
#define CEC_TX_DATA_HIGH_SHIFT				(0)
#define CEC_TX_DATA_HIGH_MASK				(0xFF << CEC_TX_DATA_HIGH_SHIFT)

/* cec pad */
#define CEC_REG_CEC_ENB						BIT(0)

#define CEC_RETRY_MAX_NUM					(5)
#define CEC_RXTCR_DEFAULT					(0x8cbc2a51)
#define CEC_TXTCR0_DEFAULT					(0x9420)
#define CEC_TXTCR1_DEFAULT					(0x182424)

/* CEC state enumration */
enum cec_state {
	CEC_STATE_NORMAL = 0,
	CEC_STATE_ERROR
};

/* CEC controller */
struct cec_acts_controller {
	volatile uint32_t cr; /* cec control register */
	volatile uint32_t rtcr; /* cec re-transmission control register */
	volatile uint32_t rxcr; /* cec rx control register */
	volatile uint32_t txcr; /* cec tx control register */
	volatile uint32_t txdr; /* cec tx data register */
	volatile uint32_t rxdr; /* cec rx data register */
	volatile uint32_t rxtcr; /* cec rx timing control register */
	volatile uint32_t txtcr0; /* cec tx timing control register 0 */
	volatile uint32_t txtcr1; /* cec tx timing control register 1 */
	volatile uint32_t pad; /* cec pad register */
};

/* cec device configration data */
struct acts_cec_config {
	struct cec_acts_controller *base;
	void (*irq_config_func)(void);
	uint16_t clock_id;
	uint16_t reset_id;
	uint8_t local_address;
};

/* cec driver private data */
struct acts_cec_data {
	struct k_mutex mutex;
	struct k_sem rx_done;
	struct k_sem tx_done;
	enum cec_state state;
};

static void cec_acts_dump_regs(struct cec_acts_controller *cec)
{
	LOG_INF("CEC base 0x%x:\n"
		"    cr: %08x    rtcr: %08x   rxcr: %08x\n"
		"  txcr: %08x    txdr: %08x   rxdr: %08x\n"
		" rxtcr: %08x  txtcr0: %08x txtcr1: %08x\n"
		"   pad: %08x\n",
		(unsigned int)cec,
		cec->cr, cec->rtcr, cec->rxcr,
		cec->txcr, cec->txdr, cec->rxdr,
		cec->rxtcr, cec->txtcr0, cec->txtcr1,
		cec->pad);
}

static int cec_rx_reset(struct cec_acts_controller *cec)
{
	/* Write 1 to clear rx state and its FIFO */
	cec->rxcr |= CEC_RX_RST;
	while (cec->rxcr & CEC_RX_RST)
		;
	return 0;
}

static int cec_rx_disable(struct cec_acts_controller *cec)
{
	/* Disable RX IRQ */
	cec->rxcr &= ~CEC_RX_INT_EN;
	/* Clear RX IRQ pending */
	cec->rxcr |= CEC_RX_INT;

	return cec_rx_reset(cec);
}

static int cec_tx_reset(struct cec_acts_controller *cec)
{
	/* write 1 to clear tx state and its FIFO */
	cec->txcr |= CEC_TX_RST;
	while (cec->txcr & CEC_TX_RST)
		;
	return 0;
}

static int cec_tx_disable(struct cec_acts_controller *cec)
{
	/* Disable TX IRQ */
	cec->txcr &= ~CEC_TX_INT_EN;
	/* Clear TX IRQ pending */
	cec->txcr |= CEC_TX_INT;

	return cec_tx_reset(cec);
}

static int cec_rx_enable(struct cec_acts_controller *cec)
{
	/* RX and IRQ enable */
	cec->rxcr |= (CEC_RX_EN | CEC_RX_INT_EN);
	/* Clear RX IRQ pending */
	cec->rxcr |= CEC_RX_INT;
	return 0;
}

static int cec_tx_enable(struct cec_acts_controller *cec)
{
	/* TX and IRQ enable */
	cec->txcr |= (CEC_TX_EN | CEC_TX_INT_EN);
	/* Clear TX IRQ pending */
	cec->txcr |= CEC_TX_INT;
	return 0;
}

static int cec_set_local_address(const struct device *dev, uint8_t local_addr)
{
	const struct acts_cec_config *config = dev->config;
	struct cec_acts_controller *cec = config->base;

	/* config initiator address */
	cec->cr &= ~CEC_LOGICAL_ADDR_MASK;
	cec->cr |= CEC_LOGICAL_ADDR(local_addr);
	return 0;
}

static void cec_update_state(const struct device *dev, enum cec_state state)
{
	struct acts_cec_data *data = dev->data;
	data->state = state;
}

static int cec_control_init(const struct device *dev)
{
	const struct acts_cec_config *config = dev->config;
	struct cec_acts_controller *cec = config->base;
	uint32_t reg;
	int ret;

	/* use default value #CEC_RETRY_MAX_NUM to set retry time */
	reg = cec->rtcr & ~CEC_RETRY_NO_MASK;
	cec->rtcr = reg | CEC_RETRY_NO(CEC_RETRY_MAX_NUM);

	cec->rxtcr = CEC_RXTCR_DEFAULT;
	cec->txtcr0 = CEC_TXTCR0_DEFAULT;
	cec->txtcr1 = CEC_TXTCR1_DEFAULT;
	cec->pad = 0;

	/* setup CEC clock 0.8Mhz */
	cec->cr = (cec->cr & ~CEC_PRE_DIV_MASK) | CEC_PRE_DIV(40);

	/* Enable CEC mode */
	cec->cr = (cec->cr & ~CEC_MODE_MASK) | CEC_MODE_ENABLE;

	cec_rx_disable(cec);
	cec_tx_disable(cec);

	/* By defalut to enable cec rx */
	ret = cec_rx_enable(cec);

	/* Setup local address */
	cec_set_local_address(dev, config->local_address);

	if (!ret)
		cec_update_state(dev, CEC_STATE_NORMAL);

	return ret;
}

static int acts_cec_write(const struct device *dev, const struct cec_msg *msg, uint32_t timeout_ms)
{
	const struct acts_cec_config *config = dev->config;
	struct cec_acts_controller *cec = config->base;
	struct acts_cec_data *data = dev->data;
	uint8_t i;
	int ret;

	if ((!msg) || (msg->len > CEC_TRANSFER_MAX_SIZE)
		|| (!msg->len)) {
		LOG_ERR("invalid msg");
		return -EINVAL;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	cec_update_state(dev, CEC_STATE_NORMAL);

	/* disable rx mode */
	cec_rx_disable(cec);
	cec_tx_reset(cec);

	for (i = 0; i < msg->len; i++)
		cec->txdr = msg->buf[i];

	/* config destination address */
	cec->txcr &= ~CEC_DEST_ADDR_MASK;
	cec->txcr |= CEC_DEST_ADDR(msg->destination);

	/* config initiator address */
	cec_set_local_address(dev, msg->initiator);

	cec_tx_enable(cec);

	/* wait cec tx data transfer is done */
	ret = k_sem_take(&data->tx_done, K_MSEC(timeout_ms));
	if (ret) {
		LOG_ERR("wait tx timeout");
		ret = -EIO;
		goto out;
	}

	if (data->state == CEC_STATE_ERROR) {
		LOG_ERR("cec tx error");
		ret = -EFAULT;
	}

out:
	if (ret)
		cec_acts_dump_regs(cec);
	/* disable cec tx */
	cec_tx_disable(cec);
	/* enable cec rx */
	cec_rx_enable(cec);
	k_mutex_unlock(&data->mutex);
	return ret;
}

static int acts_cec_read(const struct device *dev, struct cec_msg *msg, uint32_t timeout_ms)
{
	const struct acts_cec_config *config = dev->config;
	struct cec_acts_controller *cec = config->base;
	struct acts_cec_data *data = dev->data;
	uint8_t i, rx_cnt;
	int ret;

	if (!msg) {
		LOG_ERR("invalid msg");
		return -EINVAL;
	}
	k_mutex_lock(&data->mutex, K_FOREVER);

	cec_update_state(dev, CEC_STATE_NORMAL);

	/* wait cec rx data transfer is done */
	ret = k_sem_take(&data->rx_done, K_MSEC(timeout_ms));
	if (ret) {
		LOG_ERR("wait rx timeout");
		ret = -EIO;
		goto out;
	}

	if (data->state == CEC_STATE_ERROR) {
		LOG_ERR("cec rx error");
		ret = -EFAULT;
		goto out;
	}

	msg->initiator = (cec->rxcr & CEC_INIT_ADDR_MASK) >> CEC_INIT_ADDR_SHIFT;

	/* check if a broadcast message */
	if (cec->rxcr & CEC_RX_MTYPE)
		msg->destination = 0xF;
	else
		msg->destination = (cec->cr & CEC_LOGICAL_ADDR_MASK) >> CEC_LOGICAL_ADDR_SHIFT;

	rx_cnt = cec->rxcr & CEC_RX_FIFO_CNT_MASK;
	for (i = 0; i < rx_cnt; i++) {
		if (i >= CEC_TRANSFER_MAX_SIZE) {
			LOG_ERR("invalid rx fifo count %d", rx_cnt);
			ret = -EINVAL;
			goto out;
		}

		msg->buf[i] = cec->rxdr;
	}
	msg->len = rx_cnt;

out:
	if (ret)
		cec_acts_dump_regs(cec);
	/* rx reset */
	cec_rx_reset(cec);
	/* enable cec rx */
	cec_rx_enable(cec);
	k_mutex_unlock(&data->mutex);
	return ret;
}

int acts_cec_init(const struct device *dev)
{
	const struct acts_cec_config *config = dev->config;
	struct acts_cec_data *data = dev->data;

	/* enable cec controller clock */
	acts_clock_peripheral_enable(config->clock_id);

	/* reset cec controller */
	acts_reset_peripheral(config->reset_id);

	k_mutex_init(&data->mutex);
	k_sem_init(&data->rx_done, 0, 1);
	k_sem_init(&data->tx_done, 0, 1);

	config->irq_config_func();

	cec_control_init(dev);

	printk("cec initialized\n");

	return 0;
}

const struct cec_driver_api cec_acts_driver_api = {
	.config = cec_set_local_address,
	.write = acts_cec_write,
	.read = acts_cec_read,
};

void acts_cec_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	const struct acts_cec_config *config = dev->config;
	struct acts_cec_data *data = dev->data;
	struct cec_acts_controller *cec = config->base;

	LOG_DBG("cr:0x%x txcr:0x%x rxcr:0x%x", cec->cr, cec->txcr, cec->rxcr);

	if (cec->txcr & CEC_TX_INT) {
		/* check tx eom */
		if (!(cec->txcr & CEC_TX_EOM))
			data->state = CEC_STATE_ERROR;
		/* check ack status */
		if (!(cec->cr & CEC_ACKSTATUS))
			data->state = CEC_STATE_ERROR;

		cec->txcr |= CEC_TX_INT;
		k_sem_give(&data->tx_done);
	}

	if (cec->rxcr & CEC_RX_INT) {
		/* check rx fifo overflow */
		if (cec->rxcr & CEC_RX_FIFO_OV)
			data->state = CEC_STATE_ERROR;
		cec->rxcr |= CEC_RX_INT;
		k_sem_give(&data->rx_done);
	}
}

#if IS_ENABLED(CONFIG_CEC)
static void cec_acts_config_func_0(void);

static const struct acts_cec_config acts_cec_config_0 = {
	.base = (struct cec_acts_controller *)CEC_REG_BASE,
	.irq_config_func = cec_acts_config_func_0,
	.clock_id = CLOCK_ID_CEC,
	.reset_id = RESET_ID_CEC,
	.local_address = CONFIG_CEC_0_LOCAL_ADDRESS,
};

static struct acts_cec_data acts_cec_data_0;

#if IS_ENABLED(CONFIG_CEC)
DEVICE_DEFINE(cec0, CONFIG_CEC_DEV_NAME, &acts_cec_init, NULL,
	      &acts_cec_data_0, &acts_cec_config_0,
	      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
	      &cec_acts_driver_api);
#endif

static void cec_acts_config_func_0(void)
{
	IRQ_CONNECT(IRQ_ID_CEC, CONFIG_CEC_0_IRQ_PRI,
		    acts_cec_isr, DEVICE_GET(cec0), 0);

	irq_enable(IRQ_ID_CEC);
}
#endif
