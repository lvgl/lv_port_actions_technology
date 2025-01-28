/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TP Keyboard driver for Actions SoC
 */

/*****************************************************************************
* Included header files
*****************************************************************************/
#include <errno.h>
#include <string.h>
#include <device.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include <drivers/input/input_dev.h>
#include <board.h>
#include <logging/log.h>
#include <board_cfg.h>

LOG_MODULE_REGISTER(tpkey, LOG_LEVEL_DBG);

/*****************************************************************************
* Macro using #define
*****************************************************************************/
#define CONFIG_DEBUG_TP_REPORT_RATE  0
#define	TPKEY_SLAVE_ADDR             (0x70 >> 1)

/*****************************************************************************
* Private constant and macro definitions using #define
*****************************************************************************/
#define INTERVAL_READ_REG                   		200  /* unit:ms */

#define FTS_CMD_READ_ID                     		0x90

/* chip id */
#define FTS_CHIP_IDH								0x64 
#define FTS_CHIP_IDL								0x56 

/* register address */
#define FTS_REG_CHIP_ID                     		0xA3
#define FTS_REG_CHIP_ID2                    		0x9F
#define FTS_REG_FW_VER                      		0xA6
#define FTS_REG_UPGRADE                         	0xFC

#define FTS_CMD_START_DELAY                 		12

/* register address */
#define FTS_REG_WORKMODE                    		0x00
#define FTS_REG_WORKMODE_FACTORY_VALUE      		0x40
#define FTS_REG_WORKMODE_SCAN_VALUE					0xC0
#define FTS_REG_FLOW_WORK_CNT               		0x91
#define FTS_REG_POWER_MODE                  		0xA5
#define FTS_REG_GESTURE_EN                  		0xD0
#define FTS_REG_GESTURE_ENABLE              		0x01
#define FTS_REG_GESTURE_OUTPUT_ADDRESS      		0xD3

#define CONFIG_USED_TP_WORK_QUEUE 1
#ifdef CONFIG_USED_TP_WORK_QUEUE
#define CONFIG_TP_WORK_Q_STACK_SIZE 1280
struct k_work_q tp_drv_q;
K_THREAD_STACK_DEFINE(tp_work_q_stack, CONFIG_TP_WORK_Q_STACK_SIZE);
#endif
/*Max point numbers of gesture trace*/
#define MAX_POINTS_GESTURE_TRACE                	6
/*Length of gesture information*/
#define MAX_LEN_GESTURE_INFO            			(MAX_POINTS_GESTURE_TRACE * 4 + 2)

/*Max point numbers of touch trace*/
#define MAX_POINTS_TOUCH_TRACE                  	1
/*Length of touch information*/
#define MAX_LEN_TOUCH_INFO            			    (MAX_POINTS_TOUCH_TRACE * 6 + 2)

/*Max touch points that touch controller supports*/
#define	FTS_MAX_POINTS_SUPPORT						10

/*****************************************************************************
* Private enumerations, structures and unions using typedef
*****************************************************************************/
struct tpkey_data {
	input_notify_t notify;

	const struct device *bus;
	const struct device *int_gpio;
	const struct device *power_gpio;
	const struct device *reset_gpio;

	struct gpio_callback int_gpio_cb;
	struct k_work init_work;

	int8_t enable_cnt;
	uint8_t inited : 1;
};

struct tpkey_config {
	struct gpio_cfg int_cfg;
	struct gpio_cfg power_cfg;
	struct gpio_cfg reset_cfg;

	uint8_t slave_addr;
};

/*****************************************************************************
* Private variables/functions
*****************************************************************************/

/*****************************************************************************
* Global variable or extern global variabls/functions
*****************************************************************************/
extern int tpkey_put_point(struct input_value *value, uint32_t timestamp);
extern int tpkey_get_last_point(struct input_value *value, uint32_t timestamp);

static void _fts_read_touch(const struct device *dev);

/*Functions*/
/*****************************************************************************
* power on
*****************************************************************************/
static void _tpkey_poweron(const struct device *dev, bool is_on)
{
	const struct tpkey_config *tpcfg = dev->config;
	struct tpkey_data *tpkey = dev->data;

	if (is_on) {
		gpio_pin_set(tpkey->power_gpio, tpcfg->power_cfg.gpion, 1);
	} else {
		gpio_pin_set(tpkey->power_gpio, tpcfg->power_cfg.gpion, 0);
		gpio_pin_set(tpkey->reset_gpio, tpcfg->reset_cfg.gpion, 1);
	}
}

/*****************************************************************************
* reset
*****************************************************************************/
static void _tpkey_reset(const struct device *dev)
{
	const struct tpkey_config *tpcfg = dev->config;
	struct tpkey_data *tpkey = dev->data;

	gpio_pin_set(tpkey->reset_gpio, tpcfg->reset_cfg.gpion, 1);
	k_msleep(10); // 5
	gpio_pin_set(tpkey->reset_gpio, tpcfg->reset_cfg.gpion, 0);
	k_msleep(20); // 80
}

/*****************************************************************************
* control interrupt
*****************************************************************************/
static void _tpkey_irq_enable(const struct device *dev, bool enabled)
{
	const struct tpkey_config *tpcfg = dev->config;
	struct tpkey_data *tpkey = dev->data;
	unsigned key = irq_lock();

	if (enabled) {
		if (++tpkey->enable_cnt == 1) {
			gpio_pin_interrupt_configure(tpkey->int_gpio,
					tpcfg->int_cfg.gpion, GPIO_INT_EDGE_TO_ACTIVE);
		}
	} else {
		if (--tpkey->enable_cnt == 0) {
			gpio_pin_interrupt_configure(tpkey->int_gpio,
					tpcfg->int_cfg.gpion, GPIO_INT_DISABLE);
		}
	}

	irq_unlock(key);
}

/*****************************************************************************
* interrupt callback
*****************************************************************************/
DEVICE_DECLARE(tpkey);

static void _tpkey_irq_callback(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	const struct device *dev = DEVICE_GET(tpkey);
	struct tpkey_data *tpkey = dev->data;

	sys_trace_void(SYS_TRACE_ID_TP_IRQ);

	if (tpkey->inited) {
		_fts_read_touch(dev);
	}

	sys_trace_end_call(SYS_TRACE_ID_TP_IRQ);
}

/*****************************************************************************
* read/write functons that might sleep
*****************************************************************************/

static int _tpkey_i2c_write(uint16_t reg, uint8_t reglen, uint8_t *data, uint16_t datalen)
{
	const struct device *dev = DEVICE_GET(tpkey);
	const struct tpkey_config *tpcfg = dev->config;
	struct tpkey_data *tpkey = dev->data;
	uint8_t retrycnt = 10;
	uint8_t txbuf[140];
	uint16_t txlen = reglen;
	int ret = 0;

	if (reglen == 2) {
		txbuf[0] = reg >> 8;
		txbuf[1] = reg & 0xFF;
	} else	{
		txbuf[0] = reg & 0xFF;
	}

	if (data && datalen) {
		memcpy(&txbuf[txlen], data, datalen);
		txlen += datalen;
	}

	for (; retrycnt > 0; retrycnt--) {
		ret = i2c_write(tpkey->bus, txbuf, txlen, tpcfg->slave_addr);
		if (ret) {
			LOG_ERR("i2c wr err %d\n", ret);
			k_msleep(4);
		} else {
			break;
		}
	}

	return ret;
}

static int _tpkey_i2c_read(uint16_t reg, uint8_t reglen, uint8_t *data, uint16_t datalen)
{
	const struct device *dev = DEVICE_GET(tpkey);
	const struct tpkey_config *tpcfg = dev->config;
	struct tpkey_data *tpkey = dev->data;
	uint8_t regbuf[2];
	uint8_t retrycnt = 10;
	int ret = 0;

	if (reglen == 2) {
		regbuf[0] = reg >> 8;
		regbuf[1] = reg & 0xFF;
	} else {
		regbuf[0] = reg & 0xFF;
	}
	printk("tpcfg->slave_addr:%02x\n",tpcfg->slave_addr);
	for (; retrycnt > 0; retrycnt--) {
		ret = i2c_write(tpkey->bus, regbuf, reglen, tpcfg->slave_addr);
		if (ret) {
			LOG_ERR("i2c wr err %d\n", ret);
			k_msleep(4);
			continue;
		}

		ret = i2c_read(tpkey->bus, data, datalen, tpcfg->slave_addr);
		if (ret) {
			LOG_ERR("i2c rd err %d\n", ret);
			k_msleep(4);
			continue;
		}

		break;
	}

	return ret;
}

/*****************************************************************************
* fts porting functions
*****************************************************************************/
int fts_write(uint8_t addr, uint8_t *data, uint16_t datalen)
{
	return _tpkey_i2c_write(addr, 1, data, datalen);
}

static int fts_read(uint8_t addr, uint8_t *data, uint16_t datalen)
{
	return _tpkey_i2c_read(addr, 1, data, datalen);
}

static int fts_write_reg(uint8_t addr, uint8_t val)
{
	return fts_write(addr, &val, 1);
}

static int fts_read_reg(uint8_t addr, uint8_t *val)
{
	return fts_read(addr, val, 1);
}

/*delay, unit: millisecond */
static void fts_msleep(unsigned int msec)
{
	k_msleep(msec);
}

static int _i2c_async_write_cb(void *cb_data, struct i2c_msg *msgs,
					uint8_t num_msgs, bool is_err)
{
	if (is_err) {
		LOG_ERR("i2c write err\n");
	}

	return 0;
}

static void _debug_report_rate(uint32_t timestamp)
{
#if CONFIG_DEBUG_TP_REPORT_RATE
	static uint32_t report_timestamp;
	static uint8_t report_cnt;

	if (++report_cnt >= 64) {
		LOG_INF("TP period %u us\n",
				k_cyc_to_us_floor32((timestamp - report_timestamp) / report_cnt));
		report_timestamp = timestamp;
		report_cnt = 0;
	}
#endif
}

#define X_COMPENSATE	(20)
#define Y_COMPENSATE	(0)
static int _i2c_async_read_cb(void *cb_data, struct i2c_msg *msgs,
					uint8_t num_msgs, bool is_err)
{
	struct input_value val;
	uint32_t timestamp;

	if (is_err) {
		LOG_ERR("i2c read err\n");
		goto exit;
	}

	timestamp = k_cycle_get_32();

	_debug_report_rate(timestamp);

	val.point.loc_x = ((msgs->buf[2] & 0x0F) << 8) + msgs->buf[3] + X_COMPENSATE;
	val.point.loc_y = ((msgs->buf[4] & 0x0F) << 8) + msgs->buf[5] + Y_COMPENSATE;

	val.point.pessure_value = (((msgs->buf[2] >> 6) & 0x03) == 1) ? 0 : 1;//msgs->buf[1];
	val.point.gesture = 0;//msgs->buf[0];	//
	tpkey_put_point(&val, timestamp);
	
	LOG_INF("pressed %d, loc (%d %d)\n",
			val.point.pessure_value, val.point.loc_x, val.point.loc_y);

exit:
	return 0;
}

static void _fts_read_touch(const struct device *dev)
{
	const struct tpkey_config *tpcfg = dev->config;
	struct tpkey_data *tpkey = dev->data;

	static uint8_t reg[1] = { 0x1 };
	static uint8_t buf[MAX_LEN_TOUCH_INFO];/*A maximum of two points are supported*/
	int ret = 0;

	sys_trace_void(SYS_TRACE_ID_TP_READ);

	ret = i2c_write_async(tpkey->bus, reg, 1, tpcfg->slave_addr, _i2c_async_write_cb, NULL);
	if (ret)
		goto exit;

	ret = i2c_read_async(tpkey->bus, buf, sizeof(buf), tpcfg->slave_addr, _i2c_async_read_cb, NULL);
	if (ret)
		goto exit;

exit:
	sys_trace_end_call(SYS_TRACE_ID_TP_READ);
}

/*****************************************************************************
* Name: fts_check_id
* Brief:
*   The function is used to check id.
* Input:
* Output:
* Return:
*   return 0 if check id successfully, otherwise error code.
*****************************************************************************/
static int _fts_check_id(void)
{
	int ret = 0;
	uint8_t chip_id[2] = { 0 };
	uint8_t val;

	/*get chip id*/
	fts_read_reg(FTS_REG_CHIP_ID, &chip_id[0]);
	fts_read_reg(FTS_REG_CHIP_ID2, &chip_id[1]);
	printk("ft3168 get ic information, chip id = 0x%02x%02x",  chip_id[0], chip_id[1]);
	if ((FTS_CHIP_IDH == chip_id[0]) && (FTS_CHIP_IDL == chip_id[1])) {
		LOG_INF("get ic information, chip id = 0x%02x%02x",  chip_id[0], chip_id[1]);
		return 0;
	}

	/*get boot id*/
	LOG_INF("fw is invalid, need read boot id");
	val = 0xAA;
	ret = fts_write_reg(0x55, 0xAA);
	if (ret) {
		LOG_ERR("start cmd write fail");
		return ret;
	}

	fts_msleep(FTS_CMD_START_DELAY);

	ret = fts_read(FTS_CMD_READ_ID, chip_id, 2);
	if ((ret == 0) && ((FTS_CHIP_IDH == chip_id[0]) && (FTS_CHIP_IDL == chip_id[1]))) {
		LOG_INF("get ic information, boot id = 0x%02x%02x", chip_id[0], chip_id[1]);
	} else {
		LOG_ERR("read boot id fail, read:0x%02x%02x", chip_id[0], chip_id[1]);
		ret = -1;
	}

	return ret;
}

static void _tpkey_init_work(struct k_work *work)
{
	const struct device *dev = DEVICE_GET(tpkey);
	struct tpkey_data *tpkey = dev->data;

	_tpkey_poweron(dev, true);
	_tpkey_reset(dev);
	_tpkey_irq_enable(dev, true);

	/*delay 200ms,wait fw*/
	k_msleep(200);
	
	if (!tpkey->inited) {
		if (_fts_check_id()) {
			LOG_ERR("tpkey init failed\n");
			return;
		}

		tpkey->inited = 1;
	}

	LOG_INF("tpkey init ok\n");
}

/*****************************************************************************
* device API implemention
*****************************************************************************/

static void _tpkey_dev_enable(const struct device *dev)
{
	_tpkey_irq_enable(dev, true);
}

static void _tpkey_dev_disable(const struct device *dev)
{
	_tpkey_irq_enable(dev, false);
}

static void _tpkey_dev_inquiry(const struct device *dev, struct input_value *val)
{
	tpkey_get_last_point(val, k_cycle_get_32());
}

static void _tpkey_dev_register_notify(const struct device *dev, input_notify_t notify)
{
	struct tpkey_data *tpkey = dev->data;

	tpkey->notify = notify;
}

static void _tpkey_dev_unregister_notify(const struct device *dev, input_notify_t notify)
{
	struct tpkey_data *tpkey = dev->data;

	tpkey->notify = NULL;
}

static void _tpkey_acts_get_capabilities(const struct device *dev,
					struct input_capabilities *capabilities)
{
	capabilities->pointer.supported_gestures = 0;
}

static int _tpkey_dev_init(const struct device *dev)
{
	printk("ft3168_t init \n");
	const struct tpkey_config *tpcfg = dev->config;
	struct tpkey_data *tpkey = dev->data;

	tpkey->bus = (struct device *)device_get_binding(CONFIG_TPKEY_I2C_NAME);
	if (tpkey->bus == NULL) {
		LOG_ERR("can not access i2c dev\n");
		return -1;
	}

	tpkey->int_gpio = device_get_binding(tpcfg->int_cfg.gpio_dev_name);
	if (tpkey->int_gpio == NULL) {
		LOG_ERR("can not access gpio dev %s\n", tpcfg->int_cfg.gpio_dev_name);
		return -1;
	}

	tpkey->power_gpio = device_get_binding(tpcfg->power_cfg.gpio_dev_name);
	if (tpkey->power_gpio == NULL) {
		LOG_ERR("can not access gpio dev %s\n", tpcfg->power_cfg.gpio_dev_name);
		return -1;
	}

	tpkey->reset_gpio = device_get_binding(tpcfg->reset_cfg.gpio_dev_name);
	if (tpkey->reset_gpio == NULL) {
		LOG_ERR("can not access gpio dev %s\n", tpcfg->reset_cfg.gpio_dev_name);
		return -1;
	}

	tpkey->inited = 0;
	tpkey->enable_cnt = 0;

	gpio_pin_configure(tpkey->power_gpio, tpcfg->power_cfg.gpion,
			GPIO_OUTPUT_INACTIVE | tpcfg->power_cfg.flag);
	gpio_pin_configure(tpkey->reset_gpio, tpcfg->reset_cfg.gpion,
			GPIO_OUTPUT_ACTIVE | tpcfg->reset_cfg.flag);

	/* configure interrupt */
	gpio_pin_configure(tpkey->int_gpio, tpcfg->int_cfg.gpion,
			GPIO_INPUT | GPIO_INT_DEBOUNCE | tpcfg->int_cfg.flag);
	gpio_init_callback(&tpkey->int_gpio_cb, _tpkey_irq_callback, BIT(tpcfg->int_cfg.gpion));
	gpio_add_callback(tpkey->int_gpio, &tpkey->int_gpio_cb);

	k_work_init(&tpkey->init_work, _tpkey_init_work);

#ifdef CONFIG_USED_TP_WORK_QUEUE
	k_work_queue_start(&tp_drv_q, tp_work_q_stack, K_THREAD_STACK_SIZEOF(tp_work_q_stack), 7, NULL);
	k_work_submit_to_queue(&tp_drv_q, &tpkey->init_work);
#else
	k_work_submit(&tpkey->init_work);
#endif
	return 0;
}

#ifdef CONFIG_PM_DEVICE
static void _tpkey_dev_suspend(const struct device *dev)
{
	_tpkey_irq_enable(dev, false);
	_tpkey_poweron(dev, false);

	LOG_INF("tpkey suspend\n");
}

static void _tpkey_dev_resume(const struct device *dev)
{
	struct tpkey_data *tpkey = dev->data;

	LOG_INF("tpkey resume\n");
#ifdef CONFIG_USED_TP_WORK_QUEUE
	k_work_submit_to_queue(&tp_drv_q, &tpkey->init_work);
#else
	k_work_submit(&tpkey->init_work);
#endif
}

static int _tpkey_dev_pm_control(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_EARLY_SUSPEND:
		_tpkey_dev_suspend(dev);
		break;
	case PM_DEVICE_ACTION_LATE_RESUME:
		_tpkey_dev_resume(dev);
		break;
	default:
		break;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static struct tpkey_data tpkey_data;

static const struct tpkey_config tpkey_config = {
	.slave_addr = TPKEY_SLAVE_ADDR,

	.int_cfg = CONFIG_TPKEY_ISR_GPIO,
	.power_cfg = CONFIG_TPKEY_POWER_GPIO,
	.reset_cfg = CONFIG_TPKEY_RESET_GPIO,
};

static const struct input_dev_driver_api tpkey_driver_api = {
	.enable = _tpkey_dev_enable,
	.disable = _tpkey_dev_disable,
	.inquiry = _tpkey_dev_inquiry,
	.register_notify = _tpkey_dev_register_notify,
	.unregister_notify = _tpkey_dev_unregister_notify,
	.get_capabilities = _tpkey_acts_get_capabilities,
};

#if IS_ENABLED(CONFIG_TPKEY)
DEVICE_DEFINE(tpkey, CONFIG_TPKEY_DEV_NAME, _tpkey_dev_init,
			_tpkey_dev_pm_control, &tpkey_data, &tpkey_config, POST_KERNEL,
			60, &tpkey_driver_api);
#endif
