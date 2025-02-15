/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Actions SoC On/Off Key driver
 */

#include <errno.h>
#include <kernel.h>
#include <string.h>
#include <init.h>
#include <irq.h>
#include <soc.h>
#include <board_cfg.h>
#include <drivers/input/input_dev.h>
#include <sys/util.h>
#include <logging/log.h>
#include <debug/ramdump.h>

#ifdef CONFIG_CFG_DRV
#include <config.h>
#include <drivers/cfg_drv/driver_config.h>
#endif

LOG_MODULE_REGISTER(onoffkey, CONFIG_SYS_LOG_INPUT_DEV_LEVEL);

#define ONOFF_KEY_RESERVED (3)
#define ONOFF_KEY_CODE_INVALID (0xFF)

#ifdef CONFIG_DEBUG_RAMDUMP
#define ONOFFKEY_RAMDUMP_TIMEOUT_MS (5000)
#endif

struct onoff_key_info {
	uint32_t poll_interval_ms;
	uint32_t poll_total_ms;
	uint32_t timestamp;
#ifdef CONFIG_ONOFF_KEY_POLL_TIMER
	struct k_timer timer;
#else
	struct k_delayed_work timer;
#endif
	input_notify_t notify;

	uint8_t sample_filter_cnt;
	uint8_t scan_count;

	uint8_t user_key_code;

	uint8_t status_check_on : 1; /* if 1 stands for the onoff key status check worker is on */
	uint8_t prev_key_stable_status : 2; /* previous onoff key stable status */
#ifdef CONFIG_DEBUG_RAMDUMP
	struct k_timer ramd_timer;
#endif
};

static void onoff_key_acts_report_key(struct onoff_key_info *onoff, int value)
{
	struct input_value val = {0};

	if (onoff->notify) {
		val.keypad.code = onoff->user_key_code;

		val.keypad.type = EV_KEY;

		val.keypad.value = value;
		LOG_DBG("report onoff key code:%d value:%d", val.keypad.code, value);
		onoff->notify(NULL, &val);
	}
}

static void onoff_key_acts_enable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static void onoff_key_acts_disable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

void onoff_key_acts_inquiry(const struct device *dev, struct input_value *val)
{
	int value;
	struct onoff_key_info *onoff = dev->data;

	if (!soc_pmu_is_onoff_key_pressed()) {
		value = 0;
	} else {
		value = 1;
	}

	val->keypad.code = onoff->user_key_code;

	val->keypad.type = EV_KEY;

	val->keypad.value = value;
}
static void onoff_key_acts_register_notify(const struct device *dev, input_notify_t notify)
{
	struct onoff_key_info *onoff = dev->data;

	LOG_DBG("register notify 0x%x", (uint32_t)notify);

	onoff->notify = notify;
}

static void onoff_key_acts_unregister_notify(const struct device *dev, input_notify_t notify)
{
	struct onoff_key_info *onoff = dev->data;

	LOG_DBG("unregister notify 0x%x", (uint32_t)notify);

	onoff->notify = NULL;
}

static const struct input_dev_driver_api onoff_key_acts_driver_api = {
	.enable = onoff_key_acts_enable,
	.disable = onoff_key_acts_disable,
	.inquiry = onoff_key_acts_inquiry,
	.register_notify = onoff_key_acts_register_notify,
	.unregister_notify = onoff_key_acts_unregister_notify,
};

#ifdef CONFIG_ONOFF_KEY_POLL_TIMER
static void onoff_key_acts_poll(struct k_timer *timer)
#else
static void onoff_key_acts_poll(struct k_work *work)
#endif
{
#ifdef CONFIG_ONOFF_KEY_POLL_TIMER
	struct device *dev = k_timer_user_data_get(timer);
	struct onoff_key_info *onoff = dev->data;
#else
	struct onoff_key_info *onoff =
			CONTAINER_OF(work, struct onoff_key_info, timer);
#endif

	uint8_t is_onoff_pressed = soc_pmu_is_onoff_key_pressed();

	LOG_DBG("onoff_pressed:%d prev_key_stable_status:%d scan_count:%d(%d)",
			is_onoff_pressed, onoff->prev_key_stable_status,
			onoff->scan_count, onoff->sample_filter_cnt);

	if (is_onoff_pressed) {
		onoff->timestamp = k_uptime_get_32();
		LOG_DBG("new timestamp:%d", onoff->timestamp);
	}

	if (onoff->prev_key_stable_status != is_onoff_pressed) {
		if (++onoff->scan_count == onoff->sample_filter_cnt) {
#ifdef CONFIG_DEBUG_RAMDUMP
			if (is_onoff_pressed) {
				k_timer_start(&onoff->ramd_timer, K_MSEC(ONOFFKEY_RAMDUMP_TIMEOUT_MS), K_FOREVER);
			} else {
				k_timer_stop(&onoff->ramd_timer);
			}
#endif
			onoff->prev_key_stable_status = is_onoff_pressed;
			onoff_key_acts_report_key(onoff, is_onoff_pressed);
			onoff->scan_count = 0;
		}
	}

	if (onoff->prev_key_stable_status == 1)
		onoff_key_acts_report_key(onoff, 1);

	if ((k_uptime_get_32() - onoff->timestamp) > onoff->poll_total_ms) {
		onoff->status_check_on = 0;
		onoff->timestamp = 0;
		onoff->scan_count = 0;
		onoff->prev_key_stable_status = ONOFF_KEY_RESERVED;
	} else {
#ifdef CONFIG_ONOFF_KEY_POLL_TIMER
		k_timer_start(&onoff->timer, K_MSEC(onoff->poll_interval_ms), K_NO_WAIT);
#else
		k_delayed_work_submit(&onoff->timer, K_MSEC(onoff->poll_interval_ms));
#endif
	}
}

#ifdef CONFIG_DEBUG_RAMDUMP
static void onoff_key_ramdump(struct k_timer *timer)
{
#ifdef CONFIG_ACTIONS_PRINTK_DMA
	set_panic_exe(1);
#endif
	ramdump_save(NULL, 0);
#ifdef CONFIG_ACTIONS_PRINTK_DMA
	set_panic_exe(0);
#endif
}
#endif

static void onoff_key_pmu_notify(void *cb_data, int state)
{
	struct device *dev = (struct device *)cb_data;
	struct onoff_key_info *onoff = dev->data;

	LOG_DBG("PMU notify state:%d", state);

	if ((state != PMU_NOTIFY_STATE_PRESSED)
		&& (state != PMU_NOTIFY_STATE_LONG_PRESSED))
		return ;

	if (!soc_pmu_is_onoff_key_pressed())
		return ;

	if (onoff->status_check_on)
		return ;

	onoff->status_check_on = 1;

	onoff->timestamp = k_uptime_get_32();

	onoff_key_acts_report_key(onoff, 1);

#ifdef CONFIG_ONOFF_KEY_POLL_TIMER
	k_timer_start(&onoff->timer, K_MSEC(onoff->poll_interval_ms), K_NO_WAIT);
#else
	k_delayed_work_submit(&onoff->timer, K_MSEC(onoff->poll_interval_ms));
#endif

#ifdef CONFIG_DEBUG_RAMDUMP
	k_timer_start(&onoff->ramd_timer, K_MSEC(ONOFFKEY_RAMDUMP_TIMEOUT_MS), K_FOREVER);
#endif
}

static int onoff_key_acts_init(const struct device *dev)
{
	struct onoff_key_info *onoff = dev->data;
	uint8_t long_press_time;
	uint8_t key_function;
	struct detect_param_t detect_param = {PMU_DETECT_DEV_ONOFF, onoff_key_pmu_notify, (void *)dev};

	LOG_DBG("init on/off key");

	onoff->poll_interval_ms = CONFIG_ONOFFKEY_POLL_INTERVAL_MS;
	onoff->poll_total_ms = CONFIG_ONOFFKEY_POLL_TOTAL_MS;
	onoff->status_check_on = 0;
	onoff->sample_filter_cnt = CONFIG_ONOFFKEY_SAMPLE_FILTER_CNT;
	onoff->scan_count = 0;
	onoff->prev_key_stable_status = ONOFF_KEY_RESERVED;

	long_press_time = CONFIG_ONOFFKEY_LONG_PRESS_TIME;
	key_function = CONFIG_ONOFFKEY_FUNCTION;

#ifdef CONFIG_CFG_DRV
	uint8_t Use_Inner_ONOFF_Key, Reset_Time_Ms, Key_Value;
	uint16_t __long_press_time;
	uint16_t debounce_time_ms;
	if (cfg_get_by_key(ITEM_ONOFF_USE_INNER_ONOFF_KEY,
		&Use_Inner_ONOFF_Key, sizeof(Use_Inner_ONOFF_Key))) {
		if (!Use_Inner_ONOFF_Key) {
			LOG_INF("onoff key exit by config tool");
			return 0;
		}
	}

	if (cfg_get_by_key(ITEM_ONOFF_TIME_LONG_PRESS_RESET,
		&Reset_Time_Ms, sizeof(Reset_Time_Ms))) {
		if (Reset_Time_Ms == 0xFF) {
			key_function = 0;
		} else if (Reset_Time_Ms == 0) {
			soc_pmu_config_onoffkey_reset_time(0);
			key_function = 1;
		} else if (Reset_Time_Ms == 1) {
			soc_pmu_config_onoffkey_reset_time(1);
			key_function = 1;
		}
	}

	if (cfg_get_by_key(ITEM_ONOFF_TIME_PRESS_POWER_ON,
		&__long_press_time, sizeof(__long_press_time))) {
		if (__long_press_time == 250)
			long_press_time = 1;
		else if (__long_press_time == 500)
			long_press_time = 2;
		else if (__long_press_time == 1000)
			long_press_time = 3;
		else if (__long_press_time == 1500)
			long_press_time = 4;
		else if (__long_press_time == 2000)
			long_press_time = 5;
		else if (__long_press_time == 3000)
			long_press_time = 6;
		else if (__long_press_time == 4000)
			long_press_time = 7;
	}

	if (cfg_get_by_key(ITEM_ONOFF_DEBOUNCE_TIME_MS,
		&debounce_time_ms, sizeof(debounce_time_ms))) {
		onoff->sample_filter_cnt = (debounce_time_ms / onoff->poll_interval_ms);
		LOG_DBG("debounce_time_ms:%d poll_interval_ms:%d sample_filter_cnt:%d",
				debounce_time_ms, onoff->poll_interval_ms, onoff->sample_filter_cnt);
	}

	if (cfg_get_by_key(ITEM_ONOFF_KEY_VALUE,&Key_Value, sizeof(Key_Value))) {
		onoff->user_key_code = Key_Value;
	} else {
#ifdef CONFIG_ONOFFKEY_USER_KEYCODE
		onoff->user_key_code = CONFIG_ONOFFKEY_USER_KEYCODE;
#else
		onoff->user_key_code = KEY_POWER;
#endif
	}
#else
#ifdef CONFIG_ONOFFKEY_USER_KEYCODE
		onoff->user_key_code = CONFIG_ONOFFKEY_USER_KEYCODE;
#else
		onoff->user_key_code = KEY_POWER;
#endif
#endif

	soc_pmu_config_onoffkey_function(key_function);
	soc_pmu_config_onoffkey_time(long_press_time);

	if (soc_pmu_register_notify(&detect_param)) {
		LOG_ERR("failed to register pmu notify");
		return -ENXIO;
	}

#ifdef CONFIG_ONOFF_KEY_POLL_TIMER
	k_timer_init(&onoff->timer, onoff_key_acts_poll, NULL);
	k_timer_user_data_set(&onoff->timer, (void *)dev);
#else
	k_delayed_work_init(&onoff->timer, onoff_key_acts_poll);
#endif

#ifdef CONFIG_DEBUG_RAMDUMP
	k_timer_init(&onoff->ramd_timer, onoff_key_ramdump, NULL);
#endif

	return 0;
}

static struct onoff_key_info onoff_key_acts_ddata;

#if IS_ENABLED(CONFIG_ONOFFKEY)
DEVICE_DEFINE(onoff_key_acts, CONFIG_INPUT_DEV_ACTS_ONOFF_KEY_NAME,
			onoff_key_acts_init, NULL,
		    &onoff_key_acts_ddata, NULL,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &onoff_key_acts_driver_api);
#endif
