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

#define DT_DRV_COMPAT actions_qd

/* qd en reg */

#define QD_EN_ZRDIE            (0x1 << 8)
#define QD_EN_ZFDIE            (0x1 << 7)
#define QD_EN_YRDIE            (0x1 << 6)
#define QD_EN_YFDIE            (0x1 << 5)
#define QD_EN_XRDIE            (0x1 << 4)
#define QD_EN_XFDIE            (0x1 << 3)
#define QD_EN_QDZE             (0x1 << 2)
#define QD_EN_QDYE             (0x1 << 1)
#define QD_EN_QDXE             (0x1 << 0)

/* qd ctl reg */

#define QD_CTL_DT(X)             (X << 9)
#define QD_CTL_DT_MASK           QD_CTL_DT(0X7)

#define QD_CTL_ZCM(X)            (X << 7)
#define QD_CTL_YCM(X)            (X << 5)
#define QD_CTL_XCM(X)            (X << 3)
#define QD_CTL_ZCDC              (1 << 2)
#define QD_CTL_YCDC              (1 << 1)
#define QD_CTL_XCDC              (1 << 0)

/* qd state reg */

#define QD_STATE_ZRDIP             (1 << 8)
#define QD_STATE_ZFDIP             (1 << 7)
#define QD_STATE_YRDIP             (1 << 6)
#define QD_STATE_YFDIP             (1 << 5)
#define QD_STATE_XRDIP             (1 << 4)
#define QD_STATE_XFDIP             (1 << 3)
#define QD_STATE_CLEAR_PENDING     (QD_STATE_ZRDIP | QD_STATE_ZFDIP | QD_STATE_YRDIP | QD_STATE_YFDIP | QD_STATE_XRDIP | QD_STATE_XFDIP)
#define QD_STATE_ZTER              (1 << 2)
#define QD_STATE_YTER              (1 << 1)
#define QD_STATE_XTER              (1 << 0)


struct quad_decoder_acts_controller {
	volatile uint32_t qd_en;
	volatile uint32_t qd_ctl;
	volatile uint32_t qd_state;
	volatile uint32_t x_forward_counter;
	volatile uint32_t x_reverse_counter;
	volatile uint32_t y_forward_counter;
	volatile uint32_t y_reverse_counter;
	volatile uint32_t z_forward_counter;
	volatile uint32_t z_reverse_counter;
};

struct acts_quad_decoder_data {
	input_notify_t notify;
	struct k_delayed_work timer;
	const struct device *this_dev;
	uint32_t timestamp;
	uint16_t prev_x_forward;
	uint16_t prev_x_reserve;
	uint16_t prev_y_forward;
	uint16_t prev_y_reserve;
	uint16_t prev_z_forward;
	uint16_t prev_z_reserve;
};

struct acts_quad_decoder_config {
	struct mxkeypad_acts_controller *base;
	void (*irq_config_func)(void);
	uint8_t clock_id;
	uint8_t reset_id;
	uint8_t pinmux_size;
	uint32_t poll_total_ms;
	uint16_t poll_interval_ms;
	const struct acts_pin_config *pinmux;
};

static struct acts_quad_decoder_data quad_decoder_acts_ddata;

static const struct acts_pin_config pins_quad_decoder[] = {FOREACH_PIN_MFP(0)};

static void quad_decoder_acts_irq_config(void);

static void quad_decoder_acts_disable(struct device *dev);

static const struct acts_quad_decoder_config quad_decoder_acts_cdata = {
	.base = (struct quad_decoder_acts_controller *)DT_INST_REG_ADDR(0),
	.irq_config_func = quad_decoder_acts_irq_config,
	.pinmux = pins_quad_decoder,
	.pinmux_size = ARRAY_SIZE(pins_quad_decoder),
	.clock_id = DT_INST_PROP(0, clkid),
	.reset_id = DT_INST_PROP(0, rstid),
	.poll_interval_ms = DT_INST_PROP(0, poll_interval_ms),
	.poll_total_ms = DT_INST_PROP(0, poll_total_ms),
};

static void quad_decoder_acts_irq_disable(struct quad_decoder_acts_controller *qd)
{
	if(DT_INST_PROP(0, x_direct))
		qd->qd_en = qd->qd_en&(~(QD_EN_XRDIE | QD_EN_XFDIE));

	if(DT_INST_PROP(0, y_direct))
		qd->qd_en = qd->qd_en&(~(QD_EN_YRDIE | QD_EN_YFDIE));

	if(DT_INST_PROP(0, z_direct))
		qd->qd_en = qd->qd_en&(~(QD_EN_ZRDIE | QD_EN_ZFDIE));

	return;
}

static void quad_decoder_acts_irq_enable(struct quad_decoder_acts_controller *qd)
{
	if(DT_INST_PROP(0, x_direct))
		qd->qd_en |= QD_EN_XRDIE | QD_EN_XFDIE;

	if(DT_INST_PROP(0, y_direct))
		qd->qd_en |= QD_EN_YRDIE | QD_EN_YFDIE;

	if(DT_INST_PROP(0, z_direct))
		qd->qd_en |= QD_EN_ZRDIE | QD_EN_ZFDIE;

	return;
}


static void quad_decoder_acts_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct acts_quad_decoder_data *data = dev->data;
	const struct acts_quad_decoder_config *cfg = dev->config;
	struct quad_decoder_acts_controller *qd = cfg->base;

	quad_decoder_acts_irq_enable(qd);


	qd->qd_state |= QD_STATE_CLEAR_PENDING;
	data->timestamp = k_uptime_get_32();
	k_delayed_work_submit(&data->timer, K_NO_WAIT);//K_MSEC(cfg->poll_interval_ms));

}

static void quad_decoder_acts_report_key(struct acts_quad_decoder_data *data,
				   int key_code, int value)
{
	struct input_value val;

	if (data->notify) {
		val.keypad.type = EV_KEY;
		val.keypad.code = key_code;
		val.keypad.value = value;

		LOG_DBG("report key_code %d value %d",
			key_code, value);

		data->notify(NULL, &val);
	}
}

static void quad_decoder_acts_set_clk(const struct acts_quad_decoder_config *cfg, uint32_t freq_hz)
{
#if 1
	clk_set_rate(cfg->clock_id, freq_hz);
	k_busy_wait(100);
#endif
}

static void quad_decoder_acts_poll(struct k_work *work)
{
	struct acts_quad_decoder_data *data = CONTAINER_OF(work, struct acts_quad_decoder_data, timer);
	const struct device *dev = data->this_dev;
	const struct acts_quad_decoder_config *cfg = dev->config;
	struct quad_decoder_acts_controller *qd = cfg->base;
	uint8_t time_update;
	signed long int decoder_value;

	time_update = 0;

	if(data->prev_x_forward != qd->x_forward_counter) {//x_f
		decoder_value = qd->x_forward_counter - data->prev_x_forward;
		decoder_value = (decoder_value > 0) ? decoder_value : (decoder_value + 65535);
		quad_decoder_acts_report_key(data, 1, decoder_value);
		data->prev_x_forward = qd->x_forward_counter;
		time_update = 1;
	}

	if(data->prev_x_reserve != qd->x_reverse_counter) {//x_r
		decoder_value = qd->x_reverse_counter - data->prev_x_reserve;
		decoder_value = (decoder_value > 0) ? decoder_value : (decoder_value + 65535);
		quad_decoder_acts_report_key(data, 2, decoder_value);
		data->prev_x_reserve = qd->x_reverse_counter;
		time_update = 1;
	}

	if(data->prev_y_forward != qd->y_forward_counter) {//y_f
		decoder_value = qd->y_forward_counter - data->prev_y_forward;
		decoder_value = (decoder_value > 0) ? decoder_value : (decoder_value + 65535);
		quad_decoder_acts_report_key(data, 3, decoder_value);
		data->prev_y_forward = qd->y_forward_counter;
		time_update = 1;
	}

	if(data->prev_y_reserve != qd->y_reverse_counter) {//y_r
		decoder_value = qd->y_reverse_counter - data->prev_y_reserve;
		decoder_value = (decoder_value > 0) ? decoder_value : (decoder_value + 65535);
		quad_decoder_acts_report_key(data, 4, decoder_value);
		data->prev_y_reserve = qd->y_reverse_counter;
		time_update = 1;
	}

	if(data->prev_z_forward != qd->z_forward_counter) {//z_f
		decoder_value = qd->z_forward_counter - data->prev_z_forward;
		decoder_value = (decoder_value > 0) ? decoder_value : (decoder_value + 65535);
		quad_decoder_acts_report_key(data, 5, decoder_value);
		data->prev_z_forward = qd->z_forward_counter;
		time_update = 1;
	}

	if(data->prev_z_reserve != qd->z_reverse_counter) {//z_r
		decoder_value = qd->z_reverse_counter - data->prev_z_reserve;
		decoder_value = (decoder_value > 0) ? decoder_value : (decoder_value + 65535);
		quad_decoder_acts_report_key(data, 6, decoder_value);
		data->prev_z_reserve = qd->z_reverse_counter;
		time_update = 1;
	}

	if(time_update)
		data->timestamp = k_uptime_get_32();

out:
	if ((k_uptime_get_32() - data->timestamp) > cfg->poll_total_ms) {

		quad_decoder_acts_irq_enable(qd);

	} else {
		k_delayed_work_submit(&data->timer, K_MSEC(cfg->poll_interval_ms));
	}
}


static void quad_decoder_acts_enable(struct device *dev)
{
	const struct acts_quad_decoder_config *cfg = dev->config;
	struct quad_decoder_acts_controller *qd = cfg->base;

	LOG_DBG("enable quad_decoder");

	if(DT_INST_PROP(0, x_direct)) {
		qd->qd_en |= (qd->qd_en & (~QD_EN_QDXE)) | QD_EN_QDXE;
		qd->qd_en |= QD_EN_XRDIE | QD_EN_XFDIE;
		qd->x_forward_counter = 0;
		qd->x_reverse_counter = 0;
	}

	if(DT_INST_PROP(0, y_direct)) {
		qd->qd_en |= (qd->qd_en & (~QD_EN_QDYE)) | QD_EN_QDYE;
		qd->qd_en |= QD_EN_YRDIE | QD_EN_YFDIE;
		qd->y_forward_counter = 0;
		qd->y_reverse_counter = 0;
	}

	if(DT_INST_PROP(0, z_direct)) {
		qd->qd_en |= (qd->qd_en & (~QD_EN_QDZE)) | QD_EN_QDZE;
		qd->qd_en |= QD_EN_ZRDIE | QD_EN_ZFDIE;
		qd->z_forward_counter = 0;
		qd->z_reverse_counter = 0;
	}

	qd->qd_ctl = (qd->qd_ctl & (~ QD_CTL_DT_MASK)) | QD_CTL_DT(2);//DT3
}

static void quad_decoder_acts_disable(struct device *dev)
{
	const struct acts_quad_decoder_config *cfg = dev->config;
	struct quad_decoder_acts_controller *qd = cfg->base;

	LOG_DBG("disable quad_decoder");

	qd->qd_en = 0;

	qd->qd_ctl = (qd->qd_ctl & (~ QD_CTL_DT_MASK)) | QD_CTL_DT(0);//DT1

}

static void quad_decoder_acts_inquiry(struct device *dev, struct input_value *val)
{

	LOG_DBG("inquiry quad_decoder");

}

static void quad_decoder_acts_register_notify(struct device *dev, input_notify_t notify)
{

	LOG_DBG("register notify 0x%x", (uint32_t)notify);

	struct acts_quad_decoder_data *quad_decoder = dev->data;

	quad_decoder->notify = notify;

}

static void quad_decoder_acts_unregister_notify(struct device *dev, input_notify_t notify)
{

	LOG_DBG("unregister notify 0x%x", (uint32_t)notify);

	struct acts_quad_decoder_data *quad_decoder = dev->data;

	quad_decoder->notify = NULL;

}

const struct input_dev_driver_api quad_decoder_acts_driver_api = {
	.enable = quad_decoder_acts_enable,
	.disable = quad_decoder_acts_disable,
	.inquiry = quad_decoder_acts_inquiry,
	.register_notify = quad_decoder_acts_register_notify,
	.unregister_notify = quad_decoder_acts_unregister_notify,

};

static void notify(struct device *dev, struct input_value *val)
{
	printk("the key change: key[%d]:%d\n", val->keypad.code, val->keypad.value);
}

static void qd_test(const struct device *dev)
{
	//sys_write32(0x18, 0x40068200);
	//*((REG32)(DEBUGOE))  = 0x3ff4f0;
	//sys_write32(0x30480, 0x40068208);

	quad_decoder_acts_register_notify(dev, notify);

	quad_decoder_acts_enable(dev);

	while(1);

}

static int quad_decoder_acts_init(const struct device *dev)
{
	struct acts_quad_decoder_data *data = dev->data;
	const struct acts_quad_decoder_config *cfg = dev->config;
	const struct acts_pin_config *pinconf = cfg->pinmux;
	struct quad_decoder_acts_controller *qd = cfg->base;

	acts_pinmux_setup_pins(cfg->pinmux, cfg->pinmux_size);

	quad_decoder_acts_set_clk(cfg, 4000);

	/* enable QD controller clock */
	acts_clock_peripheral_enable(cfg->clock_id);

	/* reset QD controller */
	acts_reset_peripheral(cfg->reset_id);

	data->this_dev = dev;

	qd->qd_en = 0;

	data->prev_x_forward = qd->x_forward_counter;
	data->prev_x_reserve = qd->x_reverse_counter;
	data->prev_y_forward = qd->y_forward_counter;
	data->prev_y_reserve = qd->y_reverse_counter;
	data->prev_z_forward = qd->z_forward_counter;
	data->prev_z_reserve = qd->z_reverse_counter;

	qd->qd_ctl = QD_CTL_DT(0);//debounce config

	if(DT_INST_PROP(0, x_direct)) {
		qd->qd_ctl |= QD_CTL_XCDC;//init x direct
		qd->qd_ctl |= QD_CTL_XCM(2);// (0:0x1 mode, 1:0x2 mode, 2 : 0x4 mode)
	}

	if(DT_INST_PROP(0, y_direct)) {
		qd->qd_ctl |= QD_CTL_YCDC;//init y direct
		qd->qd_ctl |= QD_CTL_YCM(0);// (0:0x1 mode, 1:0x2 mode, 2 : 0x4 mode)
	}

	if(DT_INST_PROP(0, z_direct)) {
		qd->qd_ctl |= QD_CTL_ZCDC;//init z direct
		qd->qd_ctl |= QD_CTL_ZCM(0);// (0:0x1 mode, 1:0x2 mode, 2 : 0x4 mode)
	}

	cfg->irq_config_func();

	k_delayed_work_init(&data->timer, quad_decoder_acts_poll);

//	qd_test(dev);

	return 0;

}

#if DT_NODE_HAS_STATUS(DT_DRV_INST(0), okay)

DEVICE_DEFINE(quad_decoder, DT_INST_LABEL(0),
		    quad_decoder_acts_init, NULL,
		    &quad_decoder_acts_ddata, &quad_decoder_acts_cdata,
		    POST_KERNEL, 60,
		    &quad_decoder_acts_driver_api);

static void quad_decoder_acts_irq_config(void)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
			quad_decoder_acts_isr,
			DEVICE_GET(quad_decoder), 0);
	irq_enable(DT_INST_IRQN(0));
}

#endif // DT_NODE_HAS_STATUS
