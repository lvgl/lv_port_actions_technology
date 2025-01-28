/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TP Keyboard driver for Actions SoC
 */

#include <errno.h>
#include <kernel.h>
#include <string.h>
#include <init.h>
#include <irq.h>
#include <drivers/adc.h>
#include <drivers/input/input_dev.h>
#include <sys/util.h>
#include <sys/byteorder.h>
#include <board.h>
#include <soc_pmu.h>
#include <logging/log.h>
#include <device.h>
#include <drivers/gpio.h>
#include <soc.h>
#include <string.h>

LOG_MODULE_REGISTER(quad_decoder, CONFIG_SYS_LOG_INPUT_DEV_LEVEL);

#define DT_DRV_COMPAT actions_capture
/* capture ctl reg */
#define CAP_CTL_FIFO_LEV_IRQ_SEL_(X)                 (X << 31)
#define CAP_CTL_FIFO_LEV_IRQ_SEL_4BYTE               CAP_CTL_FIFO_LEV_IRQ_SEL_(1)
#define CAP_CTL_FIFO_LEV_IRQ_SEL_1BYTE               CAP_CTL_FIFO_LEV_IRQ_SEL_(0)

#define CAP_CTL_FIFO_LEV_IRQ_EN                      (1 << 30)
#define CAP_CTL_CC_IRQ_EN                            (1 << 26)
#define CAP_CTL_INLEVEL                              (1 << 24)
#define CAP_CTL_SS                                   (1 << 5)

#define CAP_CTL_ES_SEL(X)                            (X << 3)
#define CAP_CTL_RISE_EDGE                            CAP_CTL_ES_SEL(0)
#define CAP_CTL_FALLING_EDGE                         CAP_CTL_ES_SEL(1)
#define CAP_CTL_BOTH_EDGE                            CAP_CTL_ES_SEL(2)

#define CAP_CTL_MS_SEL(X)                            (X << 1)
#define CAP_CTL_COUNTER_MODE                         CAP_CTL_MS_SEL(0)
#define CAP_CTL_CAPTURE_MODE                         CAP_CTL_MS_SEL(1)
#define CAP_CTL_TIMER_MODE                           CAP_CTL_MS_SEL(2)

#define CAP_CTL_EN                                   (1 << 0)

/* capture cnt reg */
#define CAP_CNT_RE_SEL_(X)                           (X << 15)
#define CAP_CNT_FALLING_EDGE                         CAP_CNT_RE_SEL_(0)
#define CAP_CNT_RISEING_EDGE                         CAP_CNT_RE_SEL_(1)

#define CAP_CNT_CNT(X)                               (X << 0)

/* capture sta reg */
#define CAP_STA_FIFO1_ERR                            (1 << 24)
#define CAP_STA_C1_OVF                               (1 << 23)
#define CAP_STA_RXFL1                                (20)//20~22 READ
#define CAP_STA_RXFF1                                (1 << 19)
#define CAP_STA_RXFE1                                (1 << 18)
#define CAP_STA_FF_LEV_PD1                           (1 << 17)
#define CAP_STA_CT_PD1                               (1 << 16)
#define CAP_STA_FIFO0_ERR                            (1 << 8)
#define CAP_STA_C0_OVF                               (1 << 7)
#define CAP_STA_RXFL0                                (4)//4~6 READ
#define CAP_STA_RXFF0                                (1 << 3)
#define CAP_STA_RXFE0                                (1 << 2)
#define CAP_STA_FF_LEV_PD0                           (1 << 1)
#define CAP_STA_CT_PD0                               (1 << 0)

/* capture dbc ctl */
#define CAP_DBC_CTL_DBC1(X)                          (X << 24)
#define CAP_DBC_CTL_DB_EN1                           (1 << 16)
#define CAP_DBC_CTL_DBC0(X)                          (X << 8)
#define CAP_DBC_CTL_DB_EN0                           (1 << 0)

/* capture demod ctl */
#define CAP_DEM_CTL_INPUT_INV                        (1 << 10)
#define CAP_DEM_CTL_DC_IRQ_EN                        (1 << 9)
#define CAP_DEM_CTL_SPACE_ERR_IRQ_EN                 (1 << 8)
#define CAP_DEM_CTL_MC(X)                            (X << 3)
#define CAP_DEM_CTL_MC_VAL(val)                      ((val & 0x78) >> 3)
#define CAP_DEM_CTL_LC(X)                            (X << 1)
#define CAP_DEM_CTL_EN                               (1 << 0)

/* capture demod sta */
#define CAP_DEM_STA_DC_PD0                           (1 << 1)
#define CAP_DEM_STA_SPACE_ERR_PD0                    (1 << 0)

struct capture_acts_controller {
	volatile uint32_t capture0_ctl;
	volatile uint32_t capture0_cnt;
	volatile uint32_t capture0_val;
	volatile uint32_t reserve_1[5];
	volatile uint32_t capture1_ctl;
	volatile uint32_t capture1_cnt;
	volatile uint32_t capture1_val;
	volatile uint32_t reserve_2[1];
	volatile uint32_t capture_sta;
	volatile uint32_t reserve_3[3];
	volatile uint32_t capture_dbc_ctl;
	volatile uint32_t capture_demod_ctl;
	volatile uint32_t capture_demod_sta;
	volatile uint32_t capture_demod_timeout;
	volatile uint32_t capture_measure_cnt;
	volatile uint32_t capture_measure_high_cnt;
};

struct acts_capture_data {
	input_notify_t notify;
	struct k_delayed_work timer;
	const struct device *this_dev;

	u32_t capture_data[100];
	u32_t byte_cnt;
	u32_t carrier_rate;//khz
};

struct acts_capture_config {
	struct capture_acts_controller *base;
	void (*irq_config_func)(void);
	uint8_t clock_id;
	uint8_t reset_id;
	uint8_t pinmux_size;
	uint32_t poll_total_ms;
	uint16_t poll_interval_ms;
	const struct acts_pin_config *pinmux;
};

static struct acts_capture_data capture_acts_ddata;

static const struct acts_pin_config pins_capture[] = {FOREACH_PIN_MFP(0)};

static void capture_acts_irq_config(void);

static const struct acts_capture_config capture_acts_cdata = {
	.base = (struct capture_acts_controller *)DT_INST_REG_ADDR(0),
	.irq_config_func = capture_acts_irq_config,
	.pinmux = pins_capture,
	.pinmux_size = ARRAY_SIZE(pins_capture),
	.clock_id = DT_INST_PROP(0, clkid),
	.reset_id = DT_INST_PROP(0, rstid),
	.poll_interval_ms = DT_INST_PROP(0, poll_interval_ms),
	.poll_total_ms = DT_INST_PROP(0, poll_total_ms),
};

static void capture_acts_disable(struct device *dev);

static void capture_acts_enable(struct device *dev);

static void capture_acts_reg_dump(struct device *dev)
{
	const struct acts_capture_config *cfg = dev->config;
	struct capture_acts_controller *capture = cfg->base;

	LOG_DBG("enable capture");

	printk("capture0_cnt:0x%x\n", capture->capture0_cnt);
	printk("capture0_ctl:0x%x\n", capture->capture0_ctl);
	printk("capture0_val:0x%x\n", capture->capture0_val);
	printk("capture1_cnt:0x%x\n", capture->capture1_cnt);
	printk("capture1_ctl:0x%x\n", capture->capture1_ctl);
	printk("capture1_val:0x%x\n", capture->capture1_val);
	printk("capture_dbc_ctl:0x%x\n", capture->capture_dbc_ctl);
	printk("capture_demod_ctl:0x%x\n", capture->capture_demod_ctl);
	printk("capture_demod_sta:0x%x\n", capture->capture_demod_sta);
	printk("capture_demod_timeout:0x%x\n", capture->capture_demod_timeout);
	printk("capture_measure_cnt:0x%x\n", capture->capture_measure_cnt);
	printk("capture_measure_high_cnt:0x%x\n", capture->capture_measure_high_cnt);
	printk("capture_sta:0x%x\n", capture->capture_sta);

}

static void capture_acts_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct acts_capture_data *data = dev->data;
	const struct acts_capture_config *cfg = dev->config;
	struct capture_acts_controller *capture = cfg->base;

	data->carrier_rate = 6000000 * CAP_DEM_CTL_MC_VAL(capture->capture_demod_ctl)/capture->capture_measure_cnt/1000;

//	printk("high cnt:%d, mc:%d\n", capture->capture_measure_high_cnt, capture->capture_measure_cnt);

	while((capture->capture_sta & CAP_STA_RXFE0) == 0) {
		data->capture_data[data->byte_cnt] = capture->capture0_cnt;
		data->byte_cnt++;
		if(data->byte_cnt >= 100)
			data->byte_cnt = 99;
	}

	capture->capture_sta = capture->capture_sta;

	while(capture->capture_sta & (CAP_STA_CT_PD1 | CAP_STA_CT_PD0));

	if(data->byte_cnt == 99) {
		capture_acts_disable(dev);
		k_delayed_work_submit(&data->timer, K_NO_WAIT);
	}
}


static void capture_acts_set_clk(const struct acts_capture_config *cfg, uint32_t freq_hz)
{
#if 1
	clk_set_rate(cfg->clock_id, freq_hz);
	k_busy_wait(100);
#endif
}

static void capture_acts_poll(struct k_work *work)
{
	struct acts_capture_data *data = CONTAINER_OF(work, struct acts_capture_data, timer);
	const struct device *dev = data->this_dev;
	const struct acts_capture_config *cfg = dev->config;
	struct capture_acts_controller *capture = cfg->base;
	int ret;
	struct input_value val;

	val.ir.protocol.carry_rate = data->carrier_rate;
	val.ir.protocol.data = data->capture_data;

	if(data->notify != NULL)
		data->notify(NULL, &val);

}


static void capture_acts_enable(struct device *dev)
{
	const struct acts_capture_config *cfg = dev->config;
	struct capture_acts_controller *capture = cfg->base;

	LOG_DBG("enable capture");

	capture->capture_demod_timeout = 0x1770;
	capture->capture_demod_ctl |= CAP_DEM_CTL_INPUT_INV;//depend on peripheral circuit

	capture->capture_demod_ctl = capture->capture_demod_ctl & (~(CAP_DEM_CTL_MC(0xf)));

	capture->capture_demod_ctl |= CAP_DEM_CTL_EN | CAP_DEM_CTL_MC(3);

	capture->capture0_ctl = CAP_CTL_FIFO_LEV_IRQ_EN | CAP_CTL_CC_IRQ_EN | CAP_CTL_SS | CAP_CTL_CAPTURE_MODE | CAP_CTL_EN;

}

static void capture_acts_disable(struct device *dev)
{
	const struct acts_capture_config *cfg = dev->config;
	struct capture_acts_controller *capture = cfg->base;

	LOG_DBG("disable capture");

	capture->capture0_ctl = 0;

}

static void capture_acts_inquiry(struct device *dev, struct input_value *val)
{

	LOG_DBG("inquiry capture");

}

static void capture_acts_register_notify(struct device *dev, input_notify_t notify)
{

	LOG_DBG("register notify 0x%x", (uint32_t)notify);

	struct acts_capture_data *capture = dev->data;

	capture->notify = notify;

}

static void capture_acts_unregister_notify(struct device *dev, input_notify_t notify)
{

	LOG_DBG("unregister notify 0x%x", (uint32_t)notify);

	struct acts_capture_data *capture = dev->data;

	capture->notify = NULL;

}

const struct input_dev_driver_api capture_acts_driver_api = {
	.enable = capture_acts_enable,
	.disable = capture_acts_disable,
	.inquiry = capture_acts_inquiry,
	.register_notify = capture_acts_register_notify,
	.unregister_notify = capture_acts_unregister_notify,

};

static int capture_acts_init(const struct device *dev)
{
	struct acts_capture_data *data = dev->data;
	const struct acts_capture_config *cfg = dev->config;
	const struct acts_pin_config *pinconf = cfg->pinmux;
	struct capture_acts_controller *capture = cfg->base;

	acts_pinmux_setup_pins(cfg->pinmux, cfg->pinmux_size);

	capture_acts_set_clk(cfg, 4000);

	/* enable capture controller clock */
	acts_clock_peripheral_enable(cfg->clock_id);

	/* reset capture controller */
	acts_reset_peripheral(cfg->reset_id);

	data->this_dev = dev;
	data->byte_cnt = 0;

	cfg->irq_config_func();

	k_delayed_work_init(&data->timer, capture_acts_poll);

//	capture_test(dev);

	return 0;

}

#if DT_NODE_HAS_STATUS(DT_DRV_INST(0), okay)

DEVICE_DEFINE(capture, DT_INST_LABEL(0),
		    capture_acts_init, NULL,
		    &capture_acts_ddata, &capture_acts_cdata,
		    POST_KERNEL, 60,
		    &capture_acts_driver_api);

static void capture_acts_irq_config(void)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
			capture_acts_isr,
			DEVICE_GET(capture), 0);
	irq_enable(DT_INST_IRQN(0));
}

#endif // DT_NODE_HAS_STATUS
