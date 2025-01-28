/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief GPIO Keyboard driver for Actions SoC
 */

#include <errno.h>
#include <kernel.h>
#include <string.h>
#include <irq.h>
#include <drivers/input/input_dev.h>
#include <sys/util.h>
#include <sys/byteorder.h>
#include <device.h>
#include <drivers/gpio.h>
#include <soc.h>
#include <board.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(gpiokey, CONFIG_LOG_DEFAULT_LEVEL);

#if CONFIG_GPIOKEY_PRESSED_VOLTAGE_LEVEL
#define GPIOKEY_PIN_MODE_DEFAULT (GPIO_INPUT)
#else
#define GPIOKEY_PIN_MODE_DEFAULT (GPIO_INPUT | GPIO_PULL_UP)
#endif

#define GPIOKEY_PIN_MODE_IRQ     (GPIO_INPUT | GPIO_ACTIVE_LOW)

struct acts_gpiokey_data {
	input_notify_t notify;
#ifdef GPIO_KEY_POLL_TIMER
	struct k_timer timer;
#else
	struct k_delayed_work timer;
#endif
	const struct device *this_dev;
	uint32_t time_wait;
	uint8_t scan_count;
	bool irq_trigger;
};

struct acts_gpiokey_config {
	uint8_t pinmux_size;
	const struct acts_pin_config *pinmux;
	uint16_t poll_interval_ms;
	uint32_t poll_total_ms;
	uint8_t sample_filter_cnt;
};

typedef void (*gpio_config_irq_t)(struct device *dev);

struct acts_gpio_config {
	uint32_t base;
	uint32_t irq_num;
	gpio_config_irq_t config_func;
};

static struct acts_gpiokey_data gpiokey_acts_ddata;

static const struct acts_pin_config pins_gpiokey[] = {CONFIG_GPIOKEY_MFP};

static const struct acts_gpiokey_config gpiokey_acts_cdata = {
	.pinmux = pins_gpiokey,
	.poll_interval_ms = CONFIG_GPIOKEY_POLL_INTERVAL_MS,
	.poll_total_ms = CONFIG_GPIOKEY_POLL_TOTAL_MS,
	.sample_filter_cnt = CONFIG_GPIOKEY_SAMPLE_FILTER_CNT,
	.pinmux_size = ARRAY_SIZE(pins_gpiokey),

};
struct gpio_status {
	const struct device *gpio_dev;
	uint8_t past_tatus;
	uint8_t curent_tatus;
};

static void gpiokey_acts_enable(const struct device *dev);
static void gpiokey_acts_disable(const struct device *dev);

struct gpio_status key_gpio_status[ARRAY_SIZE(pins_gpiokey)];
struct gpio_callback key_gpio_cb[ARRAY_SIZE(pins_gpiokey)];
uint8_t disable_cb;

static void KEY_IRQ_callback(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	for( int i = 0; i < ARRAY_SIZE(pins_gpiokey); i++){
		gpio_pin_configure(key_gpio_status[i].gpio_dev, pins_gpiokey[i].pin_num % 32, GPIOKEY_PIN_MODE_DEFAULT);
	}
	gpiokey_acts_ddata.time_wait = k_uptime_get_32();

#ifdef GPIO_KEY_POLL_TIMER
	k_timer_start(&gpiokey_acts_ddata.timer, K_MSEC(gpiokey_acts_cdata.poll_interval_ms), K_NO_WAIT);
#else
	k_delayed_work_submit(&gpiokey_acts_ddata.timer, K_MSEC(gpiokey_acts_cdata.poll_interval_ms));
#endif
	gpiokey_acts_ddata.irq_trigger = true;
}

#ifdef GPIO_KEY_POLL_TIMER
static void gpiokey_acts_poll(struct k_timer *timer)
#else
static void gpiokey_acts_poll(struct k_work *work)
#endif
{
#ifdef GPIO_KEY_POLL_TIMER
	const struct device *dev = k_timer_user_data_get(timer);
	struct acts_gpiokey_data *gpiokey = dev->data;
#else
	struct acts_gpiokey_data *gpiokey = CONTAINER_OF(work, struct acts_gpiokey_data, timer);
	const struct device *dev = gpiokey->this_dev;
#endif
	const struct acts_gpiokey_config *cfg = dev->config;
	const struct acts_pin_config *pinconf = cfg->pinmux;
	int val;
	int i;

	for (i = 0; i < cfg->pinmux_size; i++) {
		val = gpio_pin_get(key_gpio_status[i].gpio_dev, (pinconf[i].pin_num % 32));
		if (val < 0) {
			LOG_DBG("failed to get gpio:%d input state", pinconf[i].pin_num);
			continue;
		}

		LOG_DBG("gpiokey pin:%d state:%d stable_state:%d",
				pinconf[i].pin_num, val, key_gpio_status[i].past_tatus);

		if (val != key_gpio_status[i].past_tatus) {//a key is pressed or released
			if (++gpiokey->scan_count == cfg->sample_filter_cnt) {
				gpiokey->time_wait = k_uptime_get_32();
				key_gpio_status[i].past_tatus = (uint8_t)val;
				if(gpiokey->notify) {
					struct input_value val_notify = {0};
					val_notify.keypad.type = EV_KEY;
#ifdef CONFIG_GPIOKEY_USER_KEYCODE
					val_notify.keypad.code = CONFIG_GPIOKEY_USER_KEYCODE;
#else
					val_notify.keypad.code = KEY_NUM0 + i;
#endif

					if (CONFIG_GPIOKEY_PRESSED_VOLTAGE_LEVEL == val)
						val_notify.keypad.value = 1; /* key pressed */
					else
						val_notify.keypad.value = 0; /* key release */
					gpiokey->notify(NULL, &val_notify);
				}
				gpiokey->scan_count = 0;
				gpiokey_acts_ddata.irq_trigger = false;
			}
			goto out;
		}

		if (key_gpio_status[i].past_tatus == CONFIG_GPIOKEY_PRESSED_VOLTAGE_LEVEL) {
			if(gpiokey->notify) {
				struct input_value val_notify = {0};
				val_notify.keypad.type = EV_KEY;
#ifdef CONFIG_GPIOKEY_USER_KEYCODE
				val_notify.keypad.code = CONFIG_GPIOKEY_USER_KEYCODE;
#else
				val_notify.keypad.code = KEY_NUM0 + i;
#endif
				val_notify.keypad.value = 1;
				gpiokey->notify(NULL, &val_notify);
			}
		}
	}

out:
	if ((k_uptime_get_32() - gpiokey->time_wait) > cfg->poll_total_ms) {
		gpiokey_acts_disable(dev);
		gpiokey->scan_count = 0;
		gpiokey_acts_ddata.irq_trigger = false;
		for(i = 0; i < cfg->pinmux_size; i++) {
			gpio_pin_configure(key_gpio_status[i].gpio_dev, (pinconf[i].pin_num % 32), GPIOKEY_PIN_MODE_IRQ);
            gpio_pin_interrupt_configure(key_gpio_status[i].gpio_dev, (pinconf[i].pin_num % 32), GPIO_INT_EDGE_TO_INACTIVE);
			gpio_init_callback(&key_gpio_cb[i], KEY_IRQ_callback, BIT(pinconf[i].pin_num % 32));
			gpio_add_callback(key_gpio_status[i].gpio_dev, &key_gpio_cb[i]);
			gpio_pin_interrupt_configure(key_gpio_status[i].gpio_dev, (pinconf[i].pin_num % 32), GPIO_INT_EDGE_BOTH);
		}
	} else {
#ifdef GPIO_KEY_POLL_TIMER
		k_timer_start(&gpiokey->timer, K_MSEC(cfg->poll_interval_ms), K_NO_WAIT);
#else
		k_delayed_work_submit(&gpiokey->timer, K_MSEC(cfg->poll_interval_ms));
#endif
	}
}

static void gpiokey_acts_enable(const struct device *dev)
{
	struct acts_gpiokey_data *gpiokey = dev->data;

	gpiokey->time_wait = k_uptime_get_32();

#ifdef GPIO_KEY_POLL_TIMER
	const struct acts_gpiokey_config *cfg = dev->config;
	k_timer_start(&gpiokey->timer, K_MSEC(cfg->poll_interval_ms), K_NO_WAIT);
#else
	k_delayed_work_submit(&gpiokey->timer, K_NO_WAIT);
#endif

	LOG_DBG("enable gpiokey");

}

static void gpiokey_acts_disable(const struct device *dev)
{
	struct acts_gpiokey_data *gpiokey = dev->data;

#ifdef GPIO_KEY_POLL_TIMER
	k_timer_stop(&gpiokey->timer);
#else
	k_delayed_work_cancel(&gpiokey->timer);
#endif

	LOG_DBG("disable gpiokey");

}

static void gpiokey_acts_register_notify(const struct device *dev, input_notify_t notify)
{
	struct acts_gpiokey_data *gpiokey = dev->data;

	LOG_DBG("register notify 0x%x", (uint32_t)notify);

	gpiokey->notify = notify;
}

static void gpiokey_acts_unregister_notify(const struct device *dev, input_notify_t notify)
{
	struct acts_gpiokey_data *gpiokey = dev->data;

	LOG_DBG("unregister notify 0x%x", (uint32_t)notify);

	gpiokey->notify = NULL;
}

const struct input_dev_driver_api gpiokey_acts_driver_api = {
	.enable = gpiokey_acts_enable,
	.disable = gpiokey_acts_disable,
	.register_notify = gpiokey_acts_register_notify,
	.unregister_notify = gpiokey_acts_unregister_notify,
};

#ifdef CONFIG_PM_DEVICE
int gpiokey_pm_control(const struct device *dev, enum pm_device_action action)
{
    struct acts_gpiokey_data *data =  (struct acts_gpiokey_data *const)(dev->data);
	
    switch (action)
    {
        case PM_DEVICE_ACTION_RESUME:
            // data->suspended = 0;
            break;
        case PM_DEVICE_ACTION_SUSPEND:
            if((data->irq_trigger != false))
            {
                LOG_ERR("gpiokey scan keypress, cannot suspend");
                return -1;
            }
            // data->suspended = 1;
            break;
        default:
            return 0;
    }

    return 0;
}
#else
#define gpiokey_pm_control    NULL
#endif

int gpiokey_acts_init(const struct device *dev)
{
	const struct acts_gpiokey_config *cfg = dev->config;
	const struct acts_pin_config *pinconf = cfg->pinmux;
	struct acts_gpiokey_data *gpiokey = dev->data;
	int val, i;

	for(i = 0; i < cfg->pinmux_size; i++) {
		const struct device *gpio_dev = device_get_binding(CONFIG_GPIO_PIN2NAME(pinconf[i].pin_num));
		if (!gpio_dev) {
			LOG_ERR("failed to bind GPIO:%d", pinconf[i].pin_num);
			continue;
		}
		gpio_pin_configure(gpio_dev, (pinconf[i].pin_num % 32), GPIOKEY_PIN_MODE_DEFAULT);
		val = gpio_pin_get(gpio_dev, (pinconf[i].pin_num % 32));
		if (val < 0) {
			LOG_ERR("failed to get gpio:%d input state", pinconf[i].pin_num);
			continue;
		}
		key_gpio_status[i].past_tatus = val;
		key_gpio_status[i].gpio_dev = gpio_dev;
	}

	gpiokey->this_dev = dev;
	gpiokey->scan_count = 0;
	gpiokey_acts_ddata.irq_trigger = false;
#ifdef GPIO_KEY_POLL_TIMER
	k_timer_init(&gpiokey->timer, gpiokey_acts_poll, NULL);
	k_timer_user_data_set(&gpiokey->timer, (void *)dev);
#else
	k_delayed_work_init(&gpiokey->timer, gpiokey_acts_poll);
#endif

	return 0;
}

#if IS_ENABLED(CONFIG_GPIOKEY)
DEVICE_DEFINE(gpiokey, CONFIG_INPUT_DEV_ACTS_GPIOKEY_NAME,
		    gpiokey_acts_init, gpiokey_pm_control,
		    &gpiokey_acts_ddata, &gpiokey_acts_cdata,
		    POST_KERNEL, 60,
		    &gpiokey_acts_driver_api);
#endif
