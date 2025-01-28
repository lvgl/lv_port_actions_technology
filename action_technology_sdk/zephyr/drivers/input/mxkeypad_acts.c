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
#include <drivers/i2c.h>

LOG_MODULE_REGISTER(mxkeypad, CONFIG_SYS_LOG_INPUT_DEV_LEVEL);

#define DT_DRV_COMPAT actions_mxkeypad

/*key ctrl0 reg*/

#define KEY_CTRL0_IRP            (0x1 << 31)
#define KEY_CTRL0_IREN           (0x1 << 30)
#define KEY_CTRL0_KOD            (0x1 << 24)
#define KEY_CTRL0_KPDEN          (0x1 << 23)
#define KEY_CTRL0_KPUEN          (0x1 << 22)
#define KEY_CTRL0_KIE            (0x1 << 21)
#define KEY_CTRL0_KOE            (0x1 << 20)
#define KEY_CTRL0_PENM(X)        (X << 4)
#define KEY_CTRL0_KRS(X)         (X << 2)
#define KEY_CTRL0_KMS            (1 << 1)
#define KEY_CTRL0_KEN            (1 << 0)

/*key ctrl1 reg*/

#define KEY_CTRL0_KST(X)         (X << 16)
#define KEY_CTRL0_DTS(X)         (X << 0)

/* mxkeypad controller */
struct mxkeypad_acts_controller {
	volatile uint32_t ctl0;
	volatile uint32_t ctl1;
	volatile uint32_t info0;
	volatile uint32_t info1;
	volatile uint32_t info2;
	volatile uint32_t info3;
	volatile uint32_t info4;
	volatile uint32_t info5;
	volatile uint32_t info6;
};

struct acts_mxkeypad_data {
	input_notify_t notify;
	u32_t key_bit_map[2];
};

struct acts_mxkeypad_config {
	struct mxkeypad_acts_controller *base;
	void (*irq_config_func)(void);
	uint8_t clock_id;
	uint8_t reset_id;
	uint16_t penm;
	uint8_t pinmux_size;
	const struct acts_pin_config *pinmux;

};

static struct acts_mxkeypad_data mxkeypad_acts_ddata;

static const struct acts_pin_config pins_mxkeypad[] = {FOREACH_PIN_MFP(0)};

static void mxkeypad_acts_irq_config(void);

static const struct acts_mxkeypad_config mxkeypad_acts_cdata = {
	.base = (struct mxkeypad_acts_controller *)DT_INST_REG_ADDR(0),
	.irq_config_func = mxkeypad_acts_irq_config,
	.pinmux = pins_mxkeypad,
	.pinmux_size = ARRAY_SIZE(pins_mxkeypad),
	.penm = DT_INST_PROP(0, penm),
	.clock_id = DT_INST_PROP(0, clkid),
	.reset_id = DT_INST_PROP(0, rstid),

};

static void mxkeypad_acts_report_key( struct acts_mxkeypad_data *keypad,
					 int key_code, int value)
{
	struct input_value val;

	if (keypad->notify) {
		val.keypad.type = EV_KEY;
		val.keypad.code = key_code;
		val.keypad.value = value;

		LOG_DBG("type:0x%x, code:0x%x, value:)0x%x\n", val.keypad.type, val.keypad.code, val.keypad.value);

		if(keypad->notify)
			keypad->notify(NULL, &val);
	}
}

static void mxkeypad_acts_scan_key(const struct acts_mxkeypad_config *cfg,
					 struct acts_mxkeypad_data *keypad)
{
	struct mxkeypad_acts_controller *keyctrl = cfg->base;
	u32_t key_modify_map;

	key_modify_map = keypad->key_bit_map[0] ^ keyctrl->info2;

	for(int i = 0; i < 32; i++) {
		if((1 << i) & (key_modify_map)) {
			if((1 << i) & keypad->key_bit_map[0])
				mxkeypad_acts_report_key(keypad, i, 0);
			else
				mxkeypad_acts_report_key(keypad, i, 1);
		}
	}

	key_modify_map = keypad->key_bit_map[1] ^ keyctrl->info3;

	for(int i = 0; i < 32; i++) {
		if((1 << i) & (key_modify_map)) {
			if((1 << i) & keypad->key_bit_map[1])
				mxkeypad_acts_report_key(keypad, i + 32, 0);
			else
				mxkeypad_acts_report_key(keypad, i + 32, 1);
		}
	}

	keypad->key_bit_map[0] = keyctrl->info2;
	keypad->key_bit_map[1] = keyctrl->info3;
	LOG_DBG("key_bit_map[0]:0x%x, keypad->key_bit_map[1]:0x%x\n", keypad->key_bit_map[0], keypad->key_bit_map[1]);
}

void mxkeypad_acts_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct acts_mxkeypad_data *mxkeypad = dev->data;
	const struct acts_mxkeypad_config *cfg = dev->config;
	struct mxkeypad_acts_controller *keyctrl = cfg->base;

	/* disable irq */
	keyctrl->ctl0 &= ~KEY_CTRL0_IREN;

	/* scan key */
	mxkeypad_acts_scan_key(cfg, mxkeypad);

	keyctrl->ctl0 |= KEY_CTRL0_IREN | KEY_CTRL0_IRP;


}

static void mxkeypad_acts_set_clk(const struct acts_mxkeypad_config *cfg, uint32_t freq_khz)
{
#if 1
	clk_set_rate(cfg->clock_id, freq_khz);
	k_busy_wait(100);
#endif
}

static void mxkeypad_acts_enable(struct device *dev)
{
	const struct acts_mxkeypad_config *cfg = dev->config;
	struct mxkeypad_acts_controller *keyctrl = cfg->base;

	keyctrl->ctl1 = KEY_CTRL0_KST(0x140) | KEY_CTRL0_DTS(0x280);//time=ClockTime*count  1/32K*0x280=19.5ms
	keyctrl->ctl0 = KEY_CTRL0_IREN | KEY_CTRL0_KPDEN | KEY_CTRL0_KOE | KEY_CTRL0_PENM(cfg->penm) | KEY_CTRL0_KRS(0x3) | KEY_CTRL0_KEN;

	LOG_DBG("enable mxkeypad");

}

static void mxkeypad_acts_disable(struct device *dev)
{
	const struct acts_mxkeypad_config *cfg = dev->config;
	struct mxkeypad_acts_controller *keyctrl = cfg->base;

	keyctrl->ctl1 = 0;
	keyctrl->ctl0 = 0;

	LOG_DBG("disable mxkeypad");

}

static void mxkeypad_acts_inquiry(struct device *dev, struct input_value *val)
{
	struct acts_mxkeypad_data *mxkeypad = dev->data;

	LOG_DBG("inquiry mxkeypad");

}

static void mxkeypad_acts_register_notify(struct device *dev, input_notify_t notify)
{
	struct acts_mxkeypad_data *mxkeypad = dev->data;

	LOG_DBG("register notify 0x%x", (uint32_t)notify);

	mxkeypad->notify = notify;

}

static void mxkeypad_acts_unregister_notify(struct device *dev, input_notify_t notify)
{
	struct acts_mxkeypad_data *mxkeypad = dev->data;

	LOG_DBG("unregister notify 0x%x", (uint32_t)notify);

	mxkeypad->notify = NULL;

}

const struct input_dev_driver_api mxkeypad_acts_driver_api = {
	.enable = mxkeypad_acts_enable,
	.disable = mxkeypad_acts_disable,
	.inquiry = mxkeypad_acts_inquiry,
	.register_notify = mxkeypad_acts_register_notify,
	.unregister_notify = mxkeypad_acts_unregister_notify,

};
static void notify(struct device *dev, struct input_value *val)
{
	printk("the key change: key[%d]:%d\n", val->keypad.code, val->keypad.value);
}

static void mxkeypad_test(const struct device *dev)
{
	mxkeypad_acts_register_notify(dev, notify);

	mxkeypad_acts_enable(dev);

	while(1);

}

int mxkeypad_acts_init(const struct device *dev)
{
	struct acts_mxkeypad_data *mxkeypad = dev->data;
	const struct acts_mxkeypad_config *cfg = dev->config;
	const struct acts_pin_config *pinconf = cfg->pinmux;

	acts_pinmux_setup_pins(cfg->pinmux, cfg->pinmux_size);

	mxkeypad->key_bit_map[1] = mxkeypad->key_bit_map[0] = 0;

	mxkeypad_acts_set_clk(cfg, 32);

	/* enable key controller clock */
	acts_clock_peripheral_enable(cfg->clock_id);

	/* reset key controller */
	acts_reset_peripheral(cfg->reset_id);

	cfg->irq_config_func();


//	mxkeypad_test(dev);

	return 0;

}

#if DT_NODE_HAS_STATUS(DT_DRV_INST(0), okay)

DEVICE_DEFINE(mxkeypad, DT_INST_LABEL(0),
		    mxkeypad_acts_init, NULL,
		    &mxkeypad_acts_ddata, &mxkeypad_acts_cdata,
		    POST_KERNEL, 60,
		    &mxkeypad_acts_driver_api);

static void mxkeypad_acts_irq_config(void)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
			mxkeypad_acts_isr,
			DEVICE_GET(mxkeypad), 0);
	irq_enable(DT_INST_IRQN(0));

	//IRQ_CONNECT(IRQ_ID_KEY_WAKEUP, 1,
	//	mxkeypad_acts_wakeup_isr, DEVICE_GET(mxkeypad_acts), 0);
}

#endif // DT_NODE_HAS_STATUS
