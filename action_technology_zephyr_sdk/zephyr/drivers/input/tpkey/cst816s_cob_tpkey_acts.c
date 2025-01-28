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

#include "capacitive_hynitron_cst816t_update.h"

LOG_MODULE_REGISTER(tpkey, 2);

#define tp_slaver_addr             (0x2A>>1) // 0x15
#define tp_slaver_boot_addr        (0x6A)

#define REG_LEN_1B  1
#define REG_LEN_2B  2

#ifndef CONFIG_MERGE_WORK_Q
#define CONFIG_USED_TP_WORK_QUEUE 1
#endif

#define POINT_REPORT_MODE  1
#define GESTURE_REPORT_MODE  2
#define GESTURE_AND_POINT_REPORT_MODE 3

static const struct gpio_cfg reset_gpio_cfg = CONFIG_TPKEY_RESET_GPIO;
static const struct gpio_cfg power_gpio_cfg = CONFIG_TPKEY_POWER_GPIO;
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

static void _cst816t_read_touch(const struct device *i2c_dev, struct input_value *val);

extern int tpkey_put_point(struct input_value *value, uint32_t timestamp);

extern int tpkey_get_last_point(struct input_value *value, uint32_t timestamp);

#define mode_pin (GPIO_PULL_UP | GPIO_INPUT | GPIO_INT_DEBOUNCE)

static void _cst816t_poweron(const struct device *dev, bool is_on)
{
	const struct device *gpios_power = device_get_binding(power_gpio_cfg.gpio_dev_name);

	gpio_pin_configure(gpios_power, power_gpio_cfg.gpion, GPIO_OUTPUT| GPIO_PULL_UP);

	gpio_pin_set_raw(gpios_power, power_gpio_cfg.gpion, is_on ? 0 : 1);
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
		_cst816t_read_touch(tpkey->i2c_dev, &val);
	}

	sys_trace_end_call(SYS_TRACE_ID_TP_IRQ);
}


static void _cst816t_irq_config(const struct device *dev, bool is_on)
{
	struct acts_tpkey_data *tpkey = dev->data;

	gpio_pin_configure(tpkey->gpio_dev , isr_gpio_cfg.gpion, mode_pin);

	gpio_init_callback(&tpkey->key_gpio_cb , _tpkey_irq_callback, BIT(isr_gpio_cfg.gpion));

	if (is_on) {
		gpio_add_callback(tpkey->gpio_dev , &tpkey->key_gpio_cb);
	} else {
		gpio_remove_callback(tpkey->gpio_dev , &tpkey->key_gpio_cb);
	}

}

static void _cst816t_reset(const struct device *dev)
{
	const struct device *gpios_reset = device_get_binding(reset_gpio_cfg.gpio_dev_name);

	gpio_pin_configure(gpios_reset , reset_gpio_cfg.gpion, GPIO_OUTPUT | GPIO_PULL_UP);

	gpio_pin_set_raw(gpios_reset , reset_gpio_cfg.gpion, 1);
	k_msleep(50);
	gpio_pin_set_raw(gpios_reset , reset_gpio_cfg.gpion, 0);
	k_msleep(10);
	gpio_pin_set_raw(gpios_reset , reset_gpio_cfg.gpion, 1);
	k_msleep(5);
}
static int8_t _cst816t_enter_bootmode(const struct device *dev)
{
	struct acts_tpkey_data *tpkey = dev->data;
	uint8_t retrycnt = 10;

	_cst816t_reset(dev);

	while(retrycnt--){
		uint8_t cmd[4] = {0};

		cmd[0] = 0xA0;
		cmd[1] = 0x01;
		cmd[2] = 0xab;
		if(-1 == i2c_write(tpkey->i2c_dev, cmd, 3, tp_slaver_boot_addr)) {
			k_msleep(4);
			continue;
		}

		cmd[0] = 0xA0;
		cmd[1] = 0x03;
		if(-1 == i2c_write(tpkey->i2c_dev, cmd, 2, tp_slaver_boot_addr)) {
			k_msleep(4);
			continue;
		}

		if(-1 == i2c_read(tpkey->i2c_dev, cmd, 1, tp_slaver_boot_addr)) {
			k_msleep(2);
			continue;
		} else {
			if(cmd[0] != 0xC1) {
				k_msleep(2);
				continue;
			} else {
				return 0;
			}
		}
	}

	return -1;

}

static bool _cst816t_write_bytes(const struct device *dev, uint16_t reg,
					uint8_t *data, uint16_t len, uint8_t regLen)
{
	int i2c_op_ret = 0;
	struct acts_tpkey_data *tpkey = dev->data;
	uint8_t retrycnt = 10;
	uint8_t sendBuf[140];
	int lenT=0;

	if (regLen == 2) {
		sendBuf[0] = reg / 0x100;
		sendBuf[1] = reg % 0x100;
		lenT = 2;
	} else	{
		sendBuf[0]=reg % 0x100;
		lenT = 1;
	}

	memcpy(sendBuf + lenT, data, len);
	lenT += len;

	while (retrycnt--)
	{
		i2c_op_ret = i2c_write(tpkey->i2c_dev, sendBuf, lenT, tp_slaver_boot_addr);
		if (0 != i2c_op_ret) {
			k_msleep(4);
			LOG_ERR("err = %d\n", i2c_op_ret);
		} else {
			return true;
		}
	}

	return false;
}

static bool _cst816t_read_bytes(const struct device *dev, uint16_t reg, uint8_t *value, uint16_t len,uint8_t regLen)
{
	int i2c_op_ret = 0;
	struct acts_tpkey_data *tpkey = dev->data;
	uint8_t retrycnt = 10;

	uint8_t sendBuf[4];
	int lenT=0;

	if (regLen==2) {
		sendBuf[0] = reg / 0x100;
		sendBuf[1] = reg % 0x100;
		lenT=2;
	} else {
		sendBuf[0] = reg % 0x100;
		lenT=1;
	}

  while (retrycnt--)
	{
		i2c_op_ret = i2c_write(tpkey->i2c_dev, sendBuf, lenT, tp_slaver_boot_addr);
		if(0 != i2c_op_ret) {
			k_msleep(4);
			LOG_ERR("err = %d\n", i2c_op_ret);
			if (retrycnt == 1)
			{
				return false;
			}
		}
		else
		{
			break;
		}
	}

	retrycnt = 10;

	while (retrycnt--)
	{
		i2c_op_ret = i2c_read(tpkey->i2c_dev, value, len, tp_slaver_boot_addr);
		if(0 != i2c_op_ret) {
			k_msleep(4);
			LOG_ERR("err = %d\n", i2c_op_ret);
		}
		else
		{
			return true;
		}
	}

  return false;
}

static int32_t cst816t_read_checksum(const struct device *dev, uint16_t startAddr,uint16_t len)
{
	union {
		uint32_t sum;
		uint8_t buf[4];
	} checksum;


	uint8_t cmd[3] = {0};
	memset(&checksum, 0, sizeof(checksum));
	//uint8_t readback[4] = {0};

	if (_cst816t_enter_bootmode(dev) == -1) {
		return -1;
	}

	cmd[0] = 0;
	if (false == _cst816t_write_bytes(dev, 0xA003, cmd, 1, REG_LEN_2B)) {
		return -1;
	}

	k_msleep(500);

	checksum.sum = 0;

	if (false == _cst816t_read_bytes(dev, 0xA008, checksum.buf, 2, REG_LEN_2B)) {
		return -1;
	}

	return checksum.sum;
}

static int _cst816t_update(const struct device *dev, uint16_t startAddr, uint16_t len, const uint8_t* src)
{
   uint16_t sum_len;
   uint8_t cmd[10];

   if (_cst816t_enter_bootmode(dev) == -1) {
      return -1;
   }
   sum_len = 0;

   #define PER_LEN 512
   do
   {
     if (sum_len >= len)
     {
       return 0;
     }

     // send address
     cmd[0] = startAddr&0xFF;
     cmd[1] = startAddr>>8;
     _cst816t_write_bytes(dev, 0xA014, cmd, 2, REG_LEN_2B);

		 int piece_len = 128;
     for(int x=0;x<512;x=x+piece_len)
     {
       _cst816t_write_bytes(dev, 0xA018 + x, (uint8_t*)src + x, piece_len, REG_LEN_2B);
     }

     cmd[0] = 0xEE;
     _cst816t_write_bytes(dev, 0xA004, cmd, 1, REG_LEN_2B);
     k_msleep(100);

     {
       uint8_t retrycnt = 50;
       while(retrycnt--)
       {
         cmd[0] = 0;
         _cst816t_read_bytes(dev, 0xA005, cmd, 1, REG_LEN_2B);
         if (cmd[0] == 0x55)
         {
           // success
           break;
         }
         k_msleep(10);
       }
     }

     startAddr += PER_LEN;
     src += PER_LEN;
     sum_len += PER_LEN;

   } while(len);

   // exit program mode
   cmd[0] = 0x00;
   _cst816t_write_bytes(dev, 0xA003, cmd, 1, REG_LEN_2B);

   return 0;
}

static bool _cst816t_hynitron_update(const struct device *dev, bool fource_update)
{
	if (_cst816t_enter_bootmode(dev) == 0) {
		if(sizeof(app_bin) > 10) {
			uint16_t startAddr = app_bin[1];
			uint16_t length = app_bin[3];
			uint16_t checksum = app_bin[5];
			startAddr <<= 8;
			startAddr |= app_bin[0];
			length <<= 8;
			length |= app_bin[2];
			checksum <<= 8;
			checksum |= app_bin[4];

			tp_crc[0] = cst816t_read_checksum(dev, startAddr, length);

			tp_crc[0] = cst816t_read_checksum(dev, startAddr, length);

			tp_crc[0] = cst816t_read_checksum(dev, startAddr, length);

			tp_crc[1] = checksum;
			if ((tp_crc[0] != checksum) || fource_update) {
				int update_ret = -1;
				//LCD_Init();
				//tPUpdatePage(0);
				update_ret = _cst816t_update(dev, startAddr, length, app_bin+6);
				uint32_t crcR=cst816t_read_checksum(dev, startAddr, length);
				k_msleep(2000);
				LOG_INF("crc_now:0x%x, crc_fw:0x%x\n", crcR, checksum);
			} else {
				LOG_INF("cst816s checksum, crc_now=%x, crc_fw=%x, no need to update tp firmware\n", tp_crc[0], checksum);
			}
		}

		return true;
	} else {
		printk("_cst816t_enter_bootmode failed\n");
	}

	return false;
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
		val.point.gesture = msgs->buf[0];
		tpkey_put_point(&val, k_cycle_get_32());
#if 0
		LOG_DBG("finger_num = %d, gesture = %d,local:(%d,%d)\n",
			msgs->buf[1], val.point.gesture,val.point.loc_x, val.point.loc_y);
#endif
	} else {
		LOG_ERR("i2c read err\n");
	}
	return 0;
}

static void _cst816t_read_touch(const struct device *i2c_dev, struct input_value *val)
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

static void _cst816t_enter_low_power_mode(const struct device *dev, bool low_power)
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
#if 0
static void _cst816t_set_report_mode(const struct device *dev, int mode)
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

static void _cst816t_turn_off_auto_reset()
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

	gpio_pin_interrupt_configure(tpkey->gpio_dev , isr_gpio_cfg.gpion, GPIO_INT_EDGE_FALLING);//GPIO_INT_DISABLE

	LOG_DBG("enable tpkey");

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

	if (!tpkey->inited) {
		_cst816t_poweron(tpkey->this_dev, true);

		_cst816t_reset(tpkey->this_dev);

		_cst816t_hynitron_update(tpkey->this_dev, false);

		_cst816t_reset(tpkey->this_dev);

		_cst816t_irq_config(tpkey->this_dev, true);

		tpkey->inited = true;
	} else {

		_cst816t_poweron(tpkey->this_dev, true);

		_cst816t_reset(tpkey->this_dev);

		_cst816t_irq_config(tpkey->this_dev, true);

		LOG_INF("ok\n");
	}
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
	_cst816t_reset(tpkey->this_dev);
	k_msleep(30);
	if (low_power_mode) {
		_cst816t_enter_low_power_mode(tpkey->this_dev,false);
		low_power_mode = false;
	} else {
		low_power_mode = true;
		_cst816t_enter_low_power_mode(tpkey->this_dev,true);
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
static void _cst816t_suspend(const struct device *dev)
{
	struct acts_tpkey_data *tpkey = (struct acts_tpkey_data *)dev->data;
	const struct device *gpios_reset = device_get_binding(reset_gpio_cfg.gpio_dev_name);

	_cst816t_irq_config(tpkey->this_dev, false);
	_cst816t_poweron(tpkey->this_dev, false);

	gpio_pin_set_raw(gpios_reset , reset_gpio_cfg.gpion, 0);

	LOG_INF("ok\n");
}
static void _cst816t_resume(const struct device *dev)
{
	struct acts_tpkey_data *tpkey = (struct acts_tpkey_data *)dev->data;

#ifdef CONFIG_USED_TP_WORK_QUEUE
	k_work_submit_to_queue(&tp_drv_q, &tpkey->init_timer);
#else
	k_work_submit(&tpkey->init_timer);
#endif

}
static int _cst816t_pm_control(const struct device *dev, enum pm_device_action action)
{
	int ret = 0;
	//struct acts_tpkey_data *data = (struct acts_tpkey_data *)dev->data;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		break;
	case PM_DEVICE_ACTION_RESUME:
		break;
	case PM_DEVICE_ACTION_EARLY_SUSPEND:
		_cst816t_suspend(dev);
		break;
	case PM_DEVICE_ACTION_LATE_RESUME:
		_cst816t_resume(dev);
		break;
	default:
		break;
	}

	return ret;
}
#else /* CONFIG_PM_DEVICE */
static int _cst816t_pm_control(const struct device *dev, uint32_t ctrl_command,
				 void *context, device_pm_cb cb, void *arg)
{

}
#endif

#if IS_ENABLED(CONFIG_TPKEY)
DEVICE_DEFINE(tpkey, CONFIG_TPKEY_DEV_NAME, _tpkey_acts_init,
			_cst816t_pm_control, &tpkey_acts_ddata, &_tpkey_acts_cdata, POST_KERNEL,
			60, &_tpkey_acts_driver_api);
#endif
