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

#define TP_SLAVER_ADDR              (0x70 >> 1) // 0x15
//#define tp_slaver_boot_addr        (0x70)

#define REG_LEN_1B  1
#define REG_LEN_2B  2


#define POINT_REPORT_MODE  1
#define GESTURE_REPORT_MODE  2
#define GESTURE_AND_POINT_REPORT_MODE 3

static const struct gpio_cfg reset_gpio_cfg = CONFIG_TPKEY_RESET_GPIO;
static const struct gpio_cfg power_gpio_cfg = CONFIG_TPKEY_RESET_GPIO;
static const struct gpio_cfg isr_gpio_cfg = CONFIG_TPKEY_RESET_GPIO;


struct acts_tpkey_data {
	input_notify_t notify;
	const  struct device *i2c_dev;
	const  struct device *gpio_dev;
	const  struct device *this_dev;
	struct gpio_callback key_gpio_cb;
	struct k_work init_timer;
    struct k_timer poll_timer;
};

struct acts_tpkey_config {
	uint16_t poll_interval_ms;
	uint32_t poll_total_ms;
};

uint16_t tp_crc[2] __attribute((used)) = {0};

static struct acts_tpkey_data tpkey_acts_ddata;
static void _ft5x0x_read_touch(struct device *i2c_dev, struct input_value *val);

#define mode_pin (GPIO_PULL_UP | GPIO_INPUT | GPIO_INT_DEBOUNCE)

static void _ft5x0x_poweron(const struct device *dev)
{
	//const struct device *gpios_power = device_get_binding(power_gpio_cfg.gpio_dev_name);

	//gpio_pin_configure(gpios_power, power_gpio_cfg.gpion, GPIO_OUTPUT| GPIO_PULL_UP);

	//gpio_pin_set_raw(gpios_power, power_gpio_cfg.gpion, 0);
}

#if 0
static void _tpkey_irq_callback(struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct acts_tpkey_data *tpkey = &tpkey_acts_ddata;
	struct input_value val;


#if 0
	static int time_stame = 0;
	printk("irq duration : %d \n",k_cyc_to_ns_floor32(k_cycle_get_32() - time_stame));
	time_stame = k_cycle_get_32();
#endif

	_ft5x0x_read_touch(tpkey->i2c_dev, &val);

}
#endif


static void _ft5x0x_irq_config(const struct device *dev)
{
    #if 0
	struct acts_tpkey_data *tpkey = dev->data;

	tpkey->gpio_dev = device_get_binding(isr_gpio_cfg.gpio_dev_name);

	gpio_pin_configure(tpkey->gpio_dev , isr_gpio_cfg.gpion, mode_pin);

	gpio_init_callback(&tpkey->key_gpio_cb , _tpkey_irq_callback, BIT(isr_gpio_cfg.gpion));

	gpio_add_callback(tpkey->gpio_dev , &tpkey->key_gpio_cb);
    #endif

}

static void _ft5x0x_reset(const struct device *dev)
{
	const struct device *gpios_reset = device_get_binding(reset_gpio_cfg.gpio_dev_name);

	gpio_pin_configure(gpios_reset , reset_gpio_cfg.gpion, GPIO_OUTPUT | GPIO_PULL_UP);
	gpio_pin_set_raw(gpios_reset , reset_gpio_cfg.gpion, 1);
	k_busy_wait(50*1000);
	gpio_pin_set_raw(gpios_reset , reset_gpio_cfg.gpion, 0);
	k_busy_wait(200*1000);
	gpio_pin_set_raw(gpios_reset , reset_gpio_cfg.gpion, 1);
	k_busy_wait(100*1000);
}

static bool _ft5x0x_write_bytes(const struct device *dev, uint16_t reg,
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
		i2c_op_ret = i2c_write(tpkey->i2c_dev, sendBuf, lenT, TP_SLAVER_ADDR);
		if (0 != i2c_op_ret) {
			k_busy_wait(4*1000);
			LOG_ERR("err = %d\n", i2c_op_ret);
		} else {
			return true;
		}
	}

	return false;
}

static bool _ft5x0x_read_bytes(const struct device *dev, uint16_t reg, uint8_t *value, uint16_t len,uint8_t regLen)
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
		i2c_op_ret = i2c_write(tpkey->i2c_dev, sendBuf, lenT, TP_SLAVER_ADDR);
		if(0 != i2c_op_ret) {
			k_busy_wait(4*1000);
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
		i2c_op_ret = i2c_read(tpkey->i2c_dev, value, len, TP_SLAVER_ADDR);
		if(0 != i2c_op_ret) {
			k_busy_wait(4*1000);
			LOG_ERR("err = %d\n", i2c_op_ret);
		}
		else
		{
			return true;
		}
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
#if 1
	if (!is_err) {
		struct input_value val;
        #if 1
		int temp_read = k_cycle_get_32();
		val.point.loc_x = (((uint16_t)(msgs->buf[1]&0x0f))<<8) | msgs->buf[2];
		val.point.loc_y = (((uint16_t)(msgs->buf[3]&0x0f))<<8) | msgs->buf[4];
		//val.point.pessure_value = msgs->buf[1];
		val.point.gesture = msgs->buf[0];
        #endif

        //printk("%d, %d, %d, %d. %d\n", msgs->buf[0], msgs->buf[1], msgs->buf[2], msgs->buf[3], msgs->buf[3]);
        if((val.point.gesture != 0) && (val.point.gesture != 0xff))
        {
		    tpkey_put_point(&val, k_cycle_get_32());
        }
#if 1
		LOG_DBG("finger_num = %d, gesture = %d, local:(%d,%d)\n",
			msgs->buf[0], val.point.gesture,val.point.loc_x, val.point.loc_y);
#endif
	} else {
		LOG_ERR("i2c read err\n");
	}
#endif
	return 0;
}

static void _ft5x0x_read_touch(struct device *i2c_dev, struct input_value *val)
{
    #if 1
	static uint8_t write_cmd[1] = {0};
	static uint8_t read_cmd[5] = {0};
	int ret = 0;

	sys_trace_void(SYS_TRACE_ID_TP_READ);
    //sys_write32(0x3908, 0x40068064);
    //printk("\t +++++++++++GPIO57_CTL: 0x%08x\n", sys_read32(0x400680E8));
    //printk("\t +++++++++++GPIO58_CTL: 0x%08x\n", sys_read32(0x400680EC));
    //printk("\t +++++++++++GPIO24_CTL: 0x%08x\n", sys_read32(0x40068064));
    //printk("\t +++++++++++GPIO25_CTL: 0x%08x\n", sys_read32(0x40068068));
    //printk("\t +++++++++++GPIO26_CTL: 0x%08x\n", sys_read32(0x4006806C));

	write_cmd[0] = 0x02;
	ret = i2c_write_async(i2c_dev, write_cmd, 1, TP_SLAVER_ADDR, i2c_async_write_cb, NULL);
	if (ret == -1)
		goto exit;

	ret = i2c_read_async(i2c_dev, read_cmd, 5, TP_SLAVER_ADDR, i2c_async_read_cb, NULL);
	if (ret == -1)
		//goto exit;
exit:
	sys_trace_end_call(SYS_TRACE_ID_TP_READ);
	return;
    #endif
}


static const struct acts_tpkey_config _tpkey_acts_cdata = {
	.poll_interval_ms = 0,
	.poll_total_ms = 0,
};

static void _tpkey_acts_enable(struct device *dev)
{
	struct acts_tpkey_data *tpkey = dev->data;
	const struct acts_tpkey_config *cfg = dev->config;
    #if 0
	gpio_pin_interrupt_configure(tpkey->gpio_dev , isr_gpio_cfg.gpion, GPIO_INT_EDGE_FALLING);//GPIO_INT_DISABLE
    #endif
	LOG_DBG("enable tpkey");

}

static void _tpkey_acts_disable(struct device *dev)
{
	struct acts_tpkey_data *tpkey = dev->data;
	const struct acts_tpkey_config *cfg = dev->config;
    #if 0
	gpio_pin_interrupt_configure(tpkey->gpio_dev , isr_gpio_cfg.gpion, GPIO_INT_DISABLE);//GPIO_INT_DISABLE
    #endif
	LOG_DBG("disable tpkey");

}

static void _tpkey_acts_inquiry(struct device *dev, struct input_value *val)
{
	struct acts_tpkey_data *tpkey = dev->data;
	tpkey_get_last_point(val, k_cycle_get_32());
}

static void _tpkey_acts_register_notify(struct device *dev, input_notify_t notify)
{
	struct acts_tpkey_data *tpkey = dev->data;

	LOG_DBG("register notify 0x%x", (uint32_t)notify);

	tpkey->notify = notify;

}

static void _tpkey_acts_unregister_notify(struct device *dev, input_notify_t notify)
{
	struct acts_tpkey_data *tpkey = dev->data;

	LOG_DBG("unregister notify 0x%x", (uint32_t)notify);

	tpkey->notify = NULL;

}

static void tpkey_poll(struct k_timer *timer)
{
	struct acts_tpkey_data *tpkey = &tpkey_acts_ddata;
	struct input_value val;

	_ft5x0x_read_touch(tpkey->i2c_dev, &val);

}

static void _tpkey_init_work(struct k_work *work)
{
	struct acts_tpkey_data *tpkey = &tpkey_acts_ddata;
	struct input_value val;

    printk("+++++++++%s, %d++++++++++\n", __func__, __LINE__);
	_ft5x0x_poweron(tpkey->this_dev);

	_ft5x0x_irq_config(tpkey->this_dev);

	_ft5x0x_reset(tpkey->this_dev);
	_ft5x0x_read_touch(tpkey->i2c_dev, &val);

    k_timer_init(&tpkey->poll_timer, tpkey_poll, NULL);
	k_timer_user_data_set(&tpkey->poll_timer, (void *)tpkey);
    k_timer_start(&tpkey->poll_timer, K_MSEC(20), K_MSEC(20));


    //printk("\t +++++++++++GPIO24_CTL: 0x%08x\n", sys_read32(0x40068064));
    //printk("\t +++++++++++GPIO25_CTL: 0x%08x\n", sys_read32(0x40068068));
    //printk("\t +++++++++++GPIO26_CTL: 0x%08x\n", sys_read32(0x4006806C));


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

	k_work_init(&tpkey->init_timer, _tpkey_init_work);

	k_work_submit(&tpkey->init_timer);

	return 0;
}

void tpkey_acts_dump(void)
{

}

static const struct input_dev_driver_api _tpkey_acts_driver_api = {
	.enable = _tpkey_acts_enable,
	.disable = _tpkey_acts_disable,
	.inquiry = _tpkey_acts_inquiry,
	.register_notify = _tpkey_acts_register_notify,
	.unregister_notify = _tpkey_acts_unregister_notify,
};

#if IS_ENABLED(CONFIG_TPKEY)
DEVICE_DEFINE(tpkey, CONFIG_TPKEY_DEV_NAME, _tpkey_acts_init,
			NULL, &tpkey_acts_ddata, &_tpkey_acts_cdata, POST_KERNEL,
			60, &_tpkey_acts_driver_api);
#endif
