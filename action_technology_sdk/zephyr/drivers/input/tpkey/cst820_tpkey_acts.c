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
#include <stdbool.h>
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

LOG_MODULE_REGISTER(tpkey, 4);

#define tp_slaver_addr             (0x2A >> 1) // 0x15

#define REG_LEN_1B  1
#define REG_LEN_2B  2

#ifndef CONFIG_MERGE_WORK_Q
#define CONFIG_USED_TP_WORK_QUEUE 1
#endif

#define POINT_REPORT_MODE  1
#define GESTURE_REPORT_MODE  2
#define GESTURE_AND_POINT_REPORT_MODE 3

static const struct gpio_cfg reset_gpio_cfg = CONFIG_TPKEY_RESET_GPIO;
#ifdef CONFIG_TPKEY_POWER_GPIO
static const struct gpio_cfg power_gpio_cfg = CONFIG_TPKEY_POWER_GPIO;
#endif
static const struct gpio_cfg isr_gpio_cfg = CONFIG_TPKEY_ISR_GPIO;

#ifdef CONFIG_USED_TP_WORK_QUEUE
#define CONFIG_TP_WORK_Q_STACK_SIZE 1280
struct k_work_q tp_drv_q;
K_THREAD_STACK_DEFINE(tp_work_q_stack, CONFIG_TP_WORK_Q_STACK_SIZE);
#endif


struct acts_tpkey_data {
	input_notify_t notify;
	const  struct device *i2c_dev;
	const  struct device *gpio_dev;
	const  struct device *this_dev;
	struct gpio_callback key_gpio_cb;
	struct k_work init_timer;
	bool inited;
#ifdef CONFIG_PM_DEVICE
	uint32_t pm_state;
#endif
};

struct acts_tpkey_config {
	uint16_t poll_interval_ms;
	uint32_t poll_total_ms;
};

uint16_t tp_crc[2] __attribute((used)) = {0};

static struct acts_tpkey_data tpkey_acts_ddata;

static void _cst820_read_touch(const struct device *i2c_dev, struct input_value *val);

extern int tpkey_put_point(struct input_value *value, uint32_t timestamp);

extern int tpkey_get_last_point(struct input_value *value, uint32_t timestamp);
#define mode_pin (GPIO_INPUT | GPIO_INT_DEBOUNCE)

static void _cst820_poweron(const struct device *dev, bool is_on)
{
#ifdef CONFIG_TPKEY_POWER_GPIO
	const struct device *gpios_power = device_get_binding(power_gpio_cfg.gpio_dev_name);

	gpio_pin_configure(gpios_power, power_gpio_cfg.gpion, GPIO_OUTPUT| GPIO_PULL_UP);

	gpio_pin_set_raw(gpios_power, power_gpio_cfg.gpion, is_on ? 1 : 0);

	printk("power_gpio_cfg.gpion %d \n",power_gpio_cfg.gpion);
#endif
}

static void _cst820_pmctl_io(const struct device *dev, bool is_suspend)
{
	static uint32_t isr_ctl;

	/* scl/sda pin suspend/resume is not here and handled in soc_sleep. */
	if (is_suspend) {
		isr_ctl = sys_read32(GPION_CTL(CONFIG_TPKEY_GPIO_ISR_NUM));

		sys_write32(0x1000, GPION_CTL(CONFIG_TPKEY_GPIO_ISR_NUM));
	} else {
		sys_write32(isr_ctl, GPION_CTL(CONFIG_TPKEY_GPIO_ISR_NUM));
	}
}

static void _tpkey_irq_callback(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct acts_tpkey_data *tpkey = &tpkey_acts_ddata;
	struct input_value val;

	sys_trace_void(SYS_TRACE_ID_TP_IRQ);

#if 0
	static int time_stame = 0;
	printk("irq duration : %d \n",k_cyc_to_ns_floor32(k_cycle_get_32() - time_stame));
	time_stame = k_cycle_get_32();
#endif

	if(tpkey->inited) {
		_cst820_read_touch(tpkey->i2c_dev, &val);
	}

	sys_trace_end_call(SYS_TRACE_ID_TP_IRQ);
}


static void _cst820_irq_config(const struct device *dev, bool is_on)
{
	struct acts_tpkey_data *tpkey = dev->data;

	gpio_pin_configure(tpkey->gpio_dev , isr_gpio_cfg.gpion, mode_pin);

	gpio_init_callback(&tpkey->key_gpio_cb , _tpkey_irq_callback, BIT(isr_gpio_cfg.gpion));

	if (is_on) {
		gpio_add_callback(tpkey->gpio_dev , &tpkey->key_gpio_cb);
	} else {
		gpio_remove_callback(tpkey->gpio_dev , &tpkey->key_gpio_cb);
	}

	printk("isr_gpio_cfg.gpion %d \n",isr_gpio_cfg.gpion);

}

static void _cst820_reset(const struct device *dev)
{
	const struct device *gpios_reset = device_get_binding(reset_gpio_cfg.gpio_dev_name);

	if (gpios_reset == NULL)
		return ;

	gpio_pin_configure(gpios_reset, reset_gpio_cfg.gpion, GPIO_OUTPUT | GPIO_PULL_UP);

	gpio_pin_set_raw(gpios_reset, reset_gpio_cfg.gpion, 1);
	k_msleep(50);
	gpio_pin_set_raw(gpios_reset, reset_gpio_cfg.gpion, 0);
	k_msleep(100);
	gpio_pin_set_raw(gpios_reset, reset_gpio_cfg.gpion, 1);
	k_msleep(10);
}

static int i2c_async_write_cb(void *cb_data, struct i2c_msg *msgs,
					uint8_t num_msgs, bool is_err)
{
	if (is_err) {
		LOG_ERR("i2c write err\n");
	}
	return 0;
}

static int i2c_async_read_cb(void *cb_data, struct i2c_msg *msgs,
					uint8_t num_msgs, bool is_err)
{
	if (!is_err) {
		struct input_value val;
		//int temp_read = k_cycle_get_32();
		val.point.loc_x = (((uint16_t)(msgs->buf[2]&0x0f))<<8) | msgs->buf[3];
		val.point.loc_y = (((uint16_t)(msgs->buf[4]&0x0f))<<8) | msgs->buf[5];
		val.point.pessure_value = msgs->buf[1];
		switch(msgs->buf[0]) {
						case 1:
										msgs->buf[0] = 2;
										break;
						case 2:
										msgs->buf[0] = 1;
										break;
		}
		val.point.gesture = msgs->buf[0];

		tpkey_put_point(&val, k_cycle_get_32());
#if 0
		printk("finger_num = %d, gesture = %d,local:(%d,%d)\n",
			msgs->buf[1], val.point.gesture,val.point.loc_x, val.point.loc_y);
#endif
	} else {
		LOG_ERR("i2c read err\n");
	}
	return 0;
}

static void _cst820_read_touch(const struct device *i2c_dev, struct input_value *val)
{
	static uint8_t write_cmd[1] = {0};
	static uint8_t read_cmd[6] = {0};
	int ret = 0;

	sys_trace_void(SYS_TRACE_ID_TP_READ);

	write_cmd[0] = 0x01;
	ret = i2c_write_async(i2c_dev, write_cmd, 1, tp_slaver_addr, i2c_async_write_cb, NULL);
	if (ret == -1)
		goto exit;

	ret = i2c_read_async(i2c_dev, read_cmd, 6, tp_slaver_addr, i2c_async_read_cb, NULL);
	if (ret == -1)
		goto exit;

exit:
	sys_trace_end_call(SYS_TRACE_ID_TP_READ);
}

static void _cst820_enter_low_power_mode(const struct device *dev, bool low_power)
{
	uint8_t cmd[6] = {0};

	cmd[0] = 0xfe;
	if (low_power) {
		cmd[1] = 0;
	} else {
		cmd[1] = 1;
	}

	i2c_write(tpkey_acts_ddata.i2c_dev, cmd, 2, tp_slaver_addr);

}

static void _cst820_set_report_mode(const struct device *dev, int mode)
{
	uint8_t cmd[6] = {0};

	cmd[0] = 0xFA;

	if (mode == POINT_REPORT_MODE) {
		cmd[1] = 0x60;
	} else if (mode == GESTURE_REPORT_MODE) {
		cmd[1] = 0x11;
	} else if (mode == GESTURE_AND_POINT_REPORT_MODE) {
		cmd[1] = 0x71;
	}

	i2c_write(tpkey_acts_ddata.i2c_dev, cmd, 2, tp_slaver_addr);

}
#if 0
static void _cst820_turn_off_auto_reset()
{
	uint8_t cmd[6] = {0};

	cmd[0] = 0xfb;
	cmd[1] = 0;
	i2c_write(tpkey_acts_ddata.i2c_dev, cmd, 2, tp_slaver_addr);

	cmd[0] = 0xfc;
	cmd[1] = 0;
	i2c_write(tpkey_acts_ddata.i2c_dev, cmd, 2, tp_slaver_addr);

}
#endif

static const struct acts_tpkey_config _tpkey_acts_cdata = {
	.poll_interval_ms = 0,
	.poll_total_ms = 0,
};

static void _tpkey_acts_enable(const struct device *dev)
{
	struct acts_tpkey_data *tpkey = dev->data;
	//const struct acts_tpkey_config *cfg = dev->config;

	printk("isr_gpio_cfg.gpion %x ",isr_gpio_cfg.gpion);

	gpio_pin_interrupt_configure(tpkey->gpio_dev , isr_gpio_cfg.gpion, GPIO_INT_EDGE_FALLING);//GPIO_INT_DISABLE
}

static void _tpkey_acts_disable(const struct device *dev)
{
	struct acts_tpkey_data *tpkey = dev->data;
	//const struct acts_tpkey_config *cfg = dev->config;

	gpio_pin_interrupt_configure(tpkey->gpio_dev , isr_gpio_cfg.gpion, GPIO_INT_DISABLE);//GPIO_INT_DISABLE

	LOG_DBG("disable tpkey");

}

static void _tpkey_acts_inquiry(const struct device *dev, struct input_value *val)
{
	//struct acts_tpkey_data *tpkey = dev->data;

	tpkey_get_last_point(val, k_cycle_get_32());
}

static void _tpkey_acts_register_notify(const struct device *dev, input_notify_t notify)
{
	struct acts_tpkey_data *tpkey = dev->data;

	LOG_DBG("register notify 0x%x", (uint32_t)notify);

	tpkey->notify = notify;

}

static void _tpkey_acts_unregister_notify(const struct device *dev, input_notify_t notify)
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

static void _tpkey_init_work(struct k_work *work)
{
	struct acts_tpkey_data *tpkey = &tpkey_acts_ddata;

	if (tpkey->inited)
		_cst820_pmctl_io(tpkey->this_dev, false);
	_cst820_poweron(tpkey->this_dev, true);

	_cst820_reset(tpkey->this_dev);

	k_msleep(50);

	_cst820_irq_config(tpkey->this_dev, true);

	//_cst820_enter_low_power_mode(tpkey->this_dev,false);
	_cst820_set_report_mode(tpkey->this_dev,POINT_REPORT_MODE);

	tpkey->inited = true;
}


static int _tpkey_acts_init(const struct device *dev)
{
	struct acts_tpkey_data *tpkey = dev->data;

	tpkey->this_dev = (struct device *)dev;

	tpkey->i2c_dev = (struct device *)device_get_binding(CONFIG_TPKEY_I2C_NAME);
	if (!tpkey->i2c_dev) {
		printk("can not access right i2c device\n");
		return -1;
	}
	tpkey->gpio_dev = device_get_binding(isr_gpio_cfg.gpio_dev_name);

	tpkey->inited = false;

	k_work_init(&tpkey->init_timer, _tpkey_init_work);

#ifdef CONFIG_USED_TP_WORK_QUEUE
	k_work_queue_start(&tp_drv_q, tp_work_q_stack, K_THREAD_STACK_SIZEOF(tp_work_q_stack), 7, NULL);
	k_work_submit_to_queue(&tp_drv_q, &tpkey->init_timer);
#else
	k_work_submit(&tpkey->init_timer);
#endif
	return 0;
}

static bool low_power_mode = false;

void tpkey_acts_dump(void)
{
	struct acts_tpkey_data *tpkey = &tpkey_acts_ddata;
	_cst820_reset(tpkey->this_dev);
	k_busy_wait(30*1000);
	if (low_power_mode) {
		_cst820_enter_low_power_mode(tpkey->this_dev,false);
		low_power_mode = false;
	} else {
		low_power_mode = true;
		_cst820_enter_low_power_mode(tpkey->this_dev,true);
	}

}

static const struct input_dev_driver_api _tpkey_acts_driver_api = {
	.enable = _tpkey_acts_enable,
	.disable = _tpkey_acts_disable,
	.inquiry = _tpkey_acts_inquiry,
	.register_notify = _tpkey_acts_register_notify,
	.unregister_notify = _tpkey_acts_unregister_notify,
	.get_capabilities = _tpkey_acts_get_capabilities,
};

#ifdef CONFIG_PM_DEVICE
static void _cst820_suspend(const struct device *dev)
{
	struct acts_tpkey_data *tpkey = (struct acts_tpkey_data *)dev->data;
	const struct device *gpios_reset = device_get_binding(reset_gpio_cfg.gpio_dev_name);

	_cst820_irq_config(tpkey->this_dev, false);
	_cst820_poweron(tpkey->this_dev, false);
	_cst820_pmctl_io(tpkey->this_dev, true);

	if (gpios_reset != NULL)
		gpio_pin_set_raw(gpios_reset, reset_gpio_cfg.gpion, 0);
}
static void _cst820_resume(const struct device *dev)
{
	struct acts_tpkey_data *tpkey = (struct acts_tpkey_data *)dev->data;

#ifdef CONFIG_USED_TP_WORK_QUEUE
	k_work_submit_to_queue(&tp_drv_q, &tpkey->init_timer);
#else
	k_work_submit(&tpkey->init_timer);
#endif

}
static int _cst820_pm_control(const struct device *dev,  enum pm_device_action action)
{
	int ret = 0;
	//struct acts_tpkey_data *data = (struct acts_tpkey_data *)dev->data;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:

		break;
	case PM_DEVICE_ACTION_RESUME:
		break;
	case PM_DEVICE_ACTION_EARLY_SUSPEND:
		_cst820_suspend(dev);
		break;
	case PM_DEVICE_ACTION_LATE_RESUME:
		_cst820_resume(dev);
		break;
	default:
		break;
	}

	return ret;
}
#else /* CONFIG_PM_DEVICE */
static int _cst820_pm_control(const struct device *dev, uint32_t ctrl_command,
				 void *context, device_pm_cb cb, void *arg)
{

}
#endif

#if IS_ENABLED(CONFIG_TPKEY)
DEVICE_DEFINE(tpkey, CONFIG_TPKEY_DEV_NAME, _tpkey_acts_init,
			_cst820_pm_control, &tpkey_acts_ddata, &_tpkey_acts_cdata, POST_KERNEL,
			60, &_tpkey_acts_driver_api);
#endif
