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
#include <board_cfg.h>

LOG_MODULE_REGISTER(tpkey, CONFIG_SYS_LOG_INPUT_DEV_LEVEL);

#define tp_slaver_addr             (0x8C>>1)

#ifdef CONFIG_TPKEY_RESET_GPIO
static const struct gpio_cfg reset_gpio_cfg = CONFIG_TPKEY_RESET_GPIO;
#endif
#ifdef CONFIG_TPKEY_POWER_GPIO
static const struct gpio_cfg power_gpio_cfg = CONFIG_TPKEY_POWER_GPIO;
#endif
#ifdef CONFIG_TPKEY_ISR_GPIO
static const struct gpio_cfg isr_gpio_cfg = CONFIG_TPKEY_ISR_GPIO;
#endif
#define CONFIG_USED_TP_WORK_QUEUE 1
#ifdef CONFIG_USED_TP_WORK_QUEUE
#define CONFIG_TP_WORK_Q_STACK_SIZE 1536
struct k_work_q tp_drv_q;
K_THREAD_STACK_DEFINE(tp_work_q_stack, CONFIG_TP_WORK_Q_STACK_SIZE);
#endif
struct acts_tpkey_data {
	input_notify_t notify;
	struct device *i2c_dev;
	struct device *gpio_dev;
	struct device *this_dev;
	struct gpio_callback key_gpio_cb;
	struct k_delayed_work timer;
};

struct acts_tpkey_config {
	uint16_t poll_interval_ms;
	uint32_t poll_total_ms;
};

struct k_timer tpkey_inquiry_timer;
u8_t tpkey_flag_timeout;

static struct acts_tpkey_data tpkey_acts_ddata;

static const struct acts_tpkey_config tpkey_acts_cdata = {
	.poll_interval_ms = 0,
	.poll_total_ms = 0,
};

static void tpkey_acts_enable(struct device *dev);
static void tpkey_acts_disable(struct device *dev);
static void read_touch(struct input_value *val);

void tpkey_detect(struct k_timer *timer)
{
	LOG_DBG("go into tpkey_detect\n");
	tpkey_flag_timeout = 1;
	k_timer_stop(&tpkey_inquiry_timer);
}

static void KEY_IRQ_callback(struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	sys_trace_void(SYS_TRACE_ID_TP_IRQ);

#if CONFIG_TPKEY_LOWPOWER
	LOG_DBG("inquiry mode\n");
	tpkey_flag_timeout = 0;
	k_timer_start(&tpkey_inquiry_timer, K_MSEC(20), K_MSEC(20));
#endif

#if CONFIG_TPKEY_READ_POLL_EN
	k_delayed_work_submit(&tpkey_acts_ddata.timer, K_NO_WAIT);
#endif

	sys_trace_end_call(SYS_TRACE_ID_TP_IRQ);
}

static void tpkey_acts_poll(struct k_work *work)
{
	struct acts_tpkey_data *tpkey = CONTAINER_OF(work, struct acts_tpkey_data, timer);
	struct device *dev = tpkey->this_dev;
	struct input_value val;

	read_touch(&val);
	tpkey->notify(dev,&val);
	k_delayed_work_cancel(&tpkey->timer);

}
static void tpkey_acts_enable(struct device *dev)
{
	struct acts_tpkey_data *tpkey = dev->data;
	const struct acts_tpkey_config *cfg = dev->config;

	gpio_pin_interrupt_configure(tpkey->gpio_dev , isr_gpio_cfg.gpion, GPIO_INT_EDGE_FALLING);//GPIO_INT_DISABLE

	LOG_DBG("enable tpkey");

}

static void tpkey_acts_disable(struct device *dev)
{
	struct acts_tpkey_data *tpkey = dev->data;
	const struct acts_tpkey_config *cfg = dev->config;

	gpio_pin_interrupt_configure(tpkey->gpio_dev , isr_gpio_cfg.gpion, GPIO_INT_DISABLE);//GPIO_INT_DISABLE

	LOG_DBG("disable tpkey");

}

static void tpkey_acts_inquiry(struct device *dev, struct input_value *val)
{
	struct acts_tpkey_data *tpkey = dev->data;

	if(tpkey_flag_timeout)
		return;

	read_touch(val);

	LOG_DBG("inquiry tpkey");

}

static void tpkey_acts_register_notify(struct device *dev, input_notify_t notify)
{
	struct acts_tpkey_data *tpkey = dev->data;

	LOG_DBG("register notify 0x%x", (uint32_t)notify);

	tpkey->notify = notify;

}

static void tpkey_acts_unregister_notify(struct device *dev, input_notify_t notify)
{
	struct acts_tpkey_data *tpkey = dev->data;

	LOG_DBG("unregister notify 0x%x", (uint32_t)notify);

	tpkey->notify = NULL;

}

static void _tpkey_acts_get_capabilities(const struct device *dev,
					struct input_capabilities *capabilities)
{
	capabilities->pointer.supported_gestures = INPUT_GESTURE_DIRECTION;
}

const struct input_dev_driver_api tpkey_acts_driver_api = {
	.enable = tpkey_acts_enable,
	.disable = tpkey_acts_disable,
	.inquiry = tpkey_acts_inquiry,
	.register_notify = tpkey_acts_register_notify,
	.unregister_notify = tpkey_acts_unregister_notify,
	.get_capabilities = _tpkey_acts_get_capabilities,
};

#define mode_pin (GPIO_PULL_UP | GPIO_INPUT | GPIO_INT_DEBOUNCE)

void tp_reset(struct device *dev)
{
	struct device *gpios_temp = NULL;

	gpios_temp = device_get_binding(reset_gpio_cfg.gpio_dev_name);

	gpio_pin_configure(gpios_temp , reset_gpio_cfg.gpion, GPIO_OUTPUT| GPIO_PULL_UP);

	gpio_pin_set_raw(gpios_temp , reset_gpio_cfg.gpion, 1);
	k_busy_wait(1000*1000);
	gpio_pin_set_raw(gpios_temp , reset_gpio_cfg.gpion, 0);
	k_busy_wait(10*1000);
	gpio_pin_set_raw(gpios_temp , reset_gpio_cfg.gpion, 1);

}

static uint8_t tp_enter_bootmode(struct device *dev)
{
	struct acts_tpkey_data *tpkey = dev->data;
	uint8_t retrycnt = 10;

	tp_reset(dev);

	k_busy_wait(5*1000);

	while(retrycnt--){
		uint8_t cmd[4] = {0};

		cmd[0] = 0xA0;
		cmd[1] = 0x01;
		cmd[2] = 0xab;
		if(-1 == i2c_write(tpkey->i2c_dev,cmd, 3, 0X6A)) {
			k_busy_wait(4*1000);
			continue;
		}

		cmd[0] = 0xA0;
		cmd[1] = 0x03;
		if(-1 == i2c_write(tpkey->i2c_dev,cmd, 2, 0X6A)) {
			k_busy_wait(4*1000);
			continue;
		}

		if(-1 == i2c_read(tpkey->i2c_dev,cmd, 1, 0X6A)) {
			k_busy_wait(2*1000);
			continue;
		} else {
			if(cmd[0] != 0x55) {
				k_busy_wait(2*1000);
				continue;
			} else {
				return 0;
			}
		}
	}

	return -1;

}

#if 0
void read_touch(struct input_value *val)
{

	uint8_t cmd[5] = {0};

	cmd[0] = 0x02;

	if(-1 == i2c_write(tpkey_acts_ddata.i2c_dev, cmd, 1, tp_slaver_addr)) {
		return;
	} else if(-1 == i2c_read(tpkey_acts_ddata.i2c_dev, cmd, 5, tp_slaver_addr)) {//tp_slaver_addr
		return;
	}

	val->point.loc_x = (((uint16_t)(cmd[1]&0x0f))<<8) | cmd[2];
	val->point.loc_y = (((uint16_t)(cmd[3]&0x0f))<<8) | cmd[4];
	val->point.pessure_value = cmd[0];

	LOG_DBG("finger_num = %d, local:(%d,%d)\n",cmd[0], val->point.loc_x, val->point.loc_y);

}
#endif

static void read_touch(struct input_value *val)
{
	uint8_t cmd[14] = {0};

	sys_trace_void(SYS_TRACE_ID_TP_READ);

	cmd[0] = 0xE0;

//	if(-1 == i2c_read(tpkey_acts_ddata.i2c_dev, cmd, 14, tp_slaver_addr))
//		return;

	if(-1 == i2c_write(tpkey_acts_ddata.i2c_dev, cmd, 1, tp_slaver_addr)) {
		return;
	} else if(-1 == i2c_read(tpkey_acts_ddata.i2c_dev, cmd, 6, tp_slaver_addr)) {//tp_slaver_addr
		return;
	}

	val->point.loc_x = (((uint16_t)(cmd[3]&0x0f))<<8) | cmd[2];
	val->point.loc_y = (((uint16_t)(cmd[3]&0xf0))<<4) | cmd[4];
	val->point.pessure_value = cmd[1];
	val->point.gesture = cmd[0];

	LOG_DBG("finger_num = %d, gesture = %d,local:(%d,%d)\n",cmd[0], val->point.gesture,val->point.loc_x, val->point.loc_y);

	sys_trace_end_call(SYS_TRACE_ID_TP_READ);
}
static void enter_low_power_mode(const struct device *dev)
{
	uint8_t cmd[6] = {0};

	printk("go into low power mode\n");
	cmd[0] = 0xfe;
	cmd[1] = 0;
	i2c_write(tpkey_acts_ddata.i2c_dev, cmd, 2, tp_slaver_addr);

}

static void turn_off_detector_mode()
{
	uint8_t cmd[6] = {0};

	cmd[0] = 0xfb;
	cmd[1] = 0;
	i2c_write(tpkey_acts_ddata.i2c_dev, cmd, 2, tp_slaver_addr);

	cmd[0] = 0xfc;
	cmd[1] = 0;
	i2c_write(tpkey_acts_ddata.i2c_dev, cmd, 2, tp_slaver_addr);

}

int tpkey_acts_init(const struct device *dev)
{
	struct acts_tpkey_data *tpkey = dev->data;
	const struct acts_tpkey_config *cfg = dev->config;
	struct device *gpios_power = NULL;

#ifdef CONFIG_TPKEY_POWER_GPIO
	gpios_power = device_get_binding(power_gpio_cfg.gpio_dev_name);
	gpio_pin_configure(gpios_power, power_gpio_cfg.gpion, GPIO_OUTPUT| GPIO_PULL_UP);
	gpio_pin_set_raw(gpios_power, reset_gpio_cfg.gpion, 0);
#endif
	tpkey->i2c_dev = (struct device *)device_get_binding(CONFIG_TPKEY_I2C_NAME);
	tpkey->gpio_dev = (struct device *)device_get_binding(isr_gpio_cfg.gpio_dev_name);

	if(tpkey->i2c_dev == NULL) {
		printk("can not access right i2c device\n");
		return -1;
	}

	if(tpkey->gpio_dev == NULL) {
		printk("can not access right gpio device\n");
		return -1;
	}

	tpkey->this_dev = (struct device *)dev;

	gpio_pin_configure(tpkey->gpio_dev , isr_gpio_cfg.gpion, mode_pin);
	gpio_init_callback(&tpkey->key_gpio_cb , KEY_IRQ_callback, BIT(isr_gpio_cfg.gpion));
	gpio_add_callback(tpkey->gpio_dev , &tpkey->key_gpio_cb);

	//if(-1 == tp_enter_bootmode(dev)) {
	//	printk("error return \n");
	//}

	tpkey_flag_timeout = 0;

	tp_reset(dev);
	k_busy_wait(100*1000);//the time waiting too short,and the device can not wake up

#if CONFIG_TPKEY_LOWPOWER
//	enter_low_power_mode(dev);
	k_timer_init(&tpkey_inquiry_timer, tpkey_detect, NULL);
	k_timer_start(&tpkey_inquiry_timer, K_MSEC(20), K_MSEC(20));
#endif

//	turn_off_detector_mode();


	k_delayed_work_init(&tpkey->timer, tpkey_acts_poll);

	return 0;

}

#if IS_ENABLED(CONFIG_TPKEY)
DEVICE_DEFINE(tpkey, CONFIG_TPKEY_DEV_NAME, tpkey_acts_init,
			NULL, &tpkey_acts_ddata, &tpkey_acts_cdata, POST_KERNEL,
			60, &tpkey_acts_driver_api);
#endif
