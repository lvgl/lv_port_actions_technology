/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file input manager interface
 */


#include <os_common_api.h>
#include <string.h>
#include <key_hal.h>
#include <app_manager.h>
#include <mem_manager.h>
#include <input_manager.h>
#include <msg_manager.h>
#include <sys_event.h>
#include <srv_manager.h>
#include <property_manager.h>
#include <input_manager_inner.h>

#ifndef CONFIG_SIMULATOR
#include <board.h>
#endif

//K_MSGQ_DEFINE(keypad_scan_msgq, sizeof(input_dev_data_t), 10, 4);

#define MAX_KEY_SUPPORT			10
#define MAX_HOLD_KEY_SUPPORT        4
#define MAX_MUTIPLE_CLICK_KEY_SUPPORT        6

#define LONG_PRESS_TIMER 10 * 40
#define SUPER_LONG_PRESS_TIMER             50 * 40   /* time */
#define SUPER_LONG_PRESS_6S_TIMER         150 * 40  /* time */
#define QUICKLY_CLICK_DURATION 300 /* ms */
#define KEY_EVENT_CANCEL_DURATION 50 /* ms */
#define HOLD_DELAY_TIME 500 /* ms */
#define HOLD_INTERVAL_DURATION 200 /* ms */

struct input_keypad_info {
	struct k_delayed_work report_work;
	struct k_delayed_work hold_key_work;
	struct k_delayed_work callback_work;
	event_trigger event_cb;
	uint32_t press_type;
	uint16_t press_code;
	uint16_t report_press_code;
	uint32_t report_key_value;
	uint32_t last_report_key_value;
	uint32_t callback_key_value;	
	int64_t report_stamp;
	uint32_t press_timer;
	uint16_t click_num: 4;
	uint16_t key_hold:1;
	uint16_t filter_itself:1;   /*过滤当前按键后续所有的事件*/
	uint16_t combo_mode:1;
	uint16_t combo_state_0:1;
	uint16_t combo_state_1:1;
	void* onoff_handle;
	void* adckey_handle;
	void* gpiokey_handle;
	uint16_t long_press_timer;
	uint16_t super_long_press_timer;
	uint16_t super_long_press_6s_timer;

	uint16_t quickly_click_duration ;
	uint16_t key_event_cancel_duration;
	uint16_t hold_delay_time;
	uint16_t hold_interval_time;
};

static struct input_keypad_info input_keypad;

static struct input_keypad_info *keypad;

#ifdef HOLDABLE_KEY
const char support_hold_key[MAX_HOLD_KEY_SUPPORT] = HOLDABLE_KEY;
#else
const char support_hold_key[MAX_HOLD_KEY_SUPPORT] = {0};
#endif

#ifdef MUTIPLE_CLIK_KEY
const char support_mutiple_key[MAX_MUTIPLE_CLICK_KEY_SUPPORT] = MUTIPLE_CLIK_KEY;
#else
const char support_mutiple_key[MAX_MUTIPLE_CLICK_KEY_SUPPORT] = {0};
#endif

void report_key_event_work_handle(struct k_work *work)
{
	struct input_keypad_info *input = CONTAINER_OF(work, struct input_keypad_info, report_work);

	if (input->filter_itself) {
		if ((input->last_report_key_value & KEY_TYPE_SHORT_UP)
			|| (input->last_report_key_value & KEY_TYPE_DOUBLE_CLICK)
			|| (input->last_report_key_value & KEY_TYPE_TRIPLE_CLICK)
			|| (input->last_report_key_value & KEY_TYPE_QUAD_CLICK)
			|| (input->last_report_key_value & KEY_TYPE_QUINT_CLICK)
			|| (input->last_report_key_value & KEY_TYPE_LONG_UP)) {
			input->filter_itself = false;
		}
		return;
	}

	input->click_num = 0;
	//os_printk("*******************\n\n\n\n report input 0x%x \n\n\n\n*********************\n",input->last_report_key_value);
	sys_event_report_input(input->last_report_key_value);
}

void callback_key_event_work_handle(struct k_work *work)
{
	struct input_keypad_info *input = CONTAINER_OF(work, struct input_keypad_info, callback_work);

	if (input->event_cb) {
		input->event_cb(input->callback_key_value,EV_KEY);
	}

}

static bool is_support_hold(int key_code)
{
	for (int i = 0 ; i < MAX_HOLD_KEY_SUPPORT; i++) {
		if (support_hold_key[i] == key_code)	{
			return true;
		}
	}
	return false;
}

#ifdef CONFIG_INPUT_MUTIPLE_CLICK
static bool is_support_mutiple_click(int key_code)
{
	for (int i = 0 ; i < MAX_MUTIPLE_CLICK_KEY_SUPPORT; i++) {
		if (support_mutiple_key[i] == key_code)	{
			return true;
		}
	}
	return false;
}
#endif

static void check_hold_key_work_handle(struct k_work *work)
{
	struct input_keypad_info *keypad = CONTAINER_OF(work, struct input_keypad_info, hold_key_work);
	int key_value = 0;

	if (keypad->key_hold) {
		os_delayed_work_submit(&keypad->hold_key_work, keypad->hold_interval_time);
		if (!input_manager_islock()) {
			key_value = KEY_TYPE_HOLD | keypad->press_code;
		}
	} else {
		if (!input_manager_islock()) {
			key_value = KEY_TYPE_HOLD_UP | keypad->press_code;
		}
	}

	if (key_value) {
		if (keypad->event_cb && !input_manager_islock()) {
			keypad->event_cb(key_value, EV_KEY);
		}

		if (keypad->filter_itself) {
			return;
		}

		sys_event_report_input(key_value);
	}

}

static void keypad_scan_callback(struct device *dev, struct input_value *val)
{
	static input_dev_state_t last_value = KEY_VALUE_UP;
	bool need_report = false;
    int32_t time_ms;

	if (val->keypad.type != EV_KEY) {
		SYS_LOG_ERR("input type %d not support\n", val->keypad.type);
		return;
	}

	if (last_value != val->keypad.value) {
		input_manager_boost(val->keypad.value == KEY_VALUE_DOWN);
		last_value = val->keypad.value;
	}

	SYS_LOG_INF("type: %d, code:%d, value:%d\n", val->keypad.type, val->keypad.code, val->keypad.value);

	switch (val->keypad.value) {
	case KEY_VALUE_UP:
	{
		if(!keypad->combo_mode) {
			keypad->press_code = val->keypad.code;
			keypad->report_press_code = keypad->press_code;
			keypad->press_code = 0;
		} else {
			if ((keypad->press_code & 0xff) == val->keypad.code) {
				keypad->combo_state_0 = KEY_VALUE_UP; 
			} else if(((keypad->press_code >> 8) & 0xff) == val->keypad.code) {
				keypad->combo_state_1 = KEY_VALUE_UP; 
			}
			keypad->report_press_code = keypad->press_code;
			if (keypad->combo_state_0 == keypad->combo_state_1
				&& keypad->combo_state_0 == KEY_VALUE_UP) {
				keypad->combo_mode = 0;
				keypad->press_code = 0;
			} else {
				return;
			}
		}
		keypad->key_hold = false;
		if (keypad->press_type == KEY_TYPE_LONG_DOWN
			 || keypad->press_type == KEY_TYPE_LONG) {
			keypad->press_type = KEY_TYPE_LONG_UP;
		}else if(keypad->press_type == KEY_TYPE_LONG6S){
		    keypad->press_type = KEY_TYPE_LONG6S_UP;
		} else {
		#ifdef CONFIG_INPUT_MUTIPLE_CLICK
			if (is_support_mutiple_click(keypad->report_press_code)) {
				if (keypad->last_report_key_value == (keypad->press_type | keypad->report_press_code)) {
					if(keypad->click_num) {
						os_delayed_work_cancel(&keypad->report_work);
					}
					keypad->click_num++;

					keypad->report_stamp = k_uptime_get();
				} else {
					if(keypad->click_num) {
						os_delayed_work_cancel(&keypad->report_work);
					}
					keypad->click_num = 1;
					keypad->report_stamp = k_uptime_get();
				}

				switch (keypad->click_num) {
				case 1:
					keypad->press_type = KEY_TYPE_SHORT_UP;
					break;
				case 2:
					keypad->press_type = KEY_TYPE_DOUBLE_CLICK;
					break;
				case 3:
					keypad->press_type = KEY_TYPE_TRIPLE_CLICK;
					break;
				case 4:
					keypad->press_type = KEY_TYPE_QUAD_CLICK;
					break;
				case 5:
					keypad->press_type = KEY_TYPE_QUINT_CLICK;
					break;
				}
			} else {
				keypad->press_type = KEY_TYPE_SHORT_UP;
			}
		#else
			keypad->press_type = KEY_TYPE_SHORT_UP;
		#endif
		}
		//keypad->press_timer = 0;
        keypad->press_timer = k_cyc_to_us_near32(k_cycle_get_32()) / 1000;
		need_report = true;
		break;
	}
	case KEY_VALUE_DOWN:
	{
		if (val->keypad.code != (keypad->press_code & 0xff)
			&& val->keypad.code != ((keypad->press_code >> 8) & 0xff)) {
			if ((keypad->press_code & 0xff) == 0) {
				keypad->press_code = val->keypad.code;
				keypad->combo_mode = 0;
				keypad->combo_state_0 = KEY_VALUE_DOWN;
			} else if(((keypad->press_code >> 8) & 0xff) == 0) {
				keypad->press_code |= (val->keypad.code << 8);
				keypad->combo_mode = 1;
				keypad->combo_state_1 = KEY_VALUE_DOWN;
			}
			//keypad->press_timer = 0;
            keypad->press_timer = k_cyc_to_us_near32(k_cycle_get_32()) / 1000;
            
			keypad->filter_itself = false;

            keypad->report_press_code = keypad->press_code;
            keypad->press_type = KEY_TYPE_SHORT_DOWN;
            need_report = true;
		} else {
			//keypad->press_timer++;
            time_ms = (k_cyc_to_us_near32(k_cycle_get_32()) / 1000) - keypad->press_timer;
			if (time_ms >= keypad->super_long_press_6s_timer) {
				if (keypad->press_type != KEY_TYPE_LONG6S) {
					keypad->report_press_code = keypad->press_code;
					keypad->press_type = KEY_TYPE_LONG6S;
					need_report = true;
				}
			} else if (time_ms >= keypad->super_long_press_timer) {
				if (keypad->press_type != KEY_TYPE_LONG) {
					keypad->report_press_code = keypad->press_code;
					keypad->press_type = KEY_TYPE_LONG;
					need_report = true;
				}
			} else if (time_ms >= keypad->long_press_timer)	{
				if (keypad->press_type != KEY_TYPE_LONG_DOWN) {
					keypad->report_press_code = keypad->press_code;
					keypad->press_type = KEY_TYPE_LONG_DOWN;
					if (is_support_hold(keypad->press_code)) {
						keypad->key_hold = true;
						os_delayed_work_submit(&keypad->hold_key_work, keypad->hold_delay_time);
					}
					need_report = true;
				}
			}

		}
		break;
	}
	default:
		break;
	}

	if (keypad->event_cb) {
		keypad->callback_key_value = ((val->keypad.value<<31)|val->keypad.code);
		os_delayed_work_submit(&keypad->callback_work, 0);
	}

	if(val->keypad.value == KEY_VALUE_DOWN && keypad->click_num) {
		os_delayed_work_cancel(&keypad->report_work);
		os_delayed_work_submit(&keypad->report_work, keypad->quickly_click_duration + keypad->key_event_cancel_duration);
	}

	if (need_report) {
		keypad->report_key_value = keypad->press_type
									| keypad->report_press_code;
		keypad->last_report_key_value = keypad->report_key_value;

		if (keypad->event_cb) {
			keypad->event_cb(keypad->last_report_key_value, EV_KEY);
		}

		if (input_manager_islock()) {
			SYS_LOG_INF("input manager locked");
			keypad->click_num = 0;
			return;
		}

		if (msg_pool_get_free_msg_num() <= (CONFIG_NUM_MBOX_ASYNC_MSGS / 2)) {
			SYS_LOG_INF("drop input msg ... %d", msg_pool_get_free_msg_num());
			return;
		}

	#ifdef CONFIG_INPUT_MUTIPLE_CLICK 
		if (is_support_mutiple_click(keypad->report_press_code)) {
			keypad->report_stamp = k_uptime_get();
			if(keypad->press_type == KEY_TYPE_LONG_DOWN || keypad->press_type == KEY_TYPE_LONG || keypad->press_type == KEY_TYPE_LONG6S)
				os_delayed_work_submit(&keypad->report_work, 0);
			else
				os_delayed_work_submit(&keypad->report_work, QUICKLY_CLICK_DURATION + KEY_EVENT_CANCEL_DURATION);
		} else {
			os_delayed_work_submit(&keypad->report_work, 0);
		}
	#else
		os_delayed_work_submit(&keypad->report_work, 0);
	#endif
	}

}

int input_keypad_device_init(event_trigger event_cb)
{
	keypad = &input_keypad;
	memset(keypad, 0, sizeof(struct input_keypad_info));
	keypad->event_cb = event_cb;
	
	os_delayed_work_init(&keypad->report_work, report_key_event_work_handle);
	os_delayed_work_init(&keypad->hold_key_work, check_hold_key_work_handle);
	os_delayed_work_init(&keypad->callback_work, callback_key_event_work_handle);

	keypad->adckey_handle = key_device_open(&keypad_scan_callback, ADC_KEY_DEVICE_NAME);
	if (!keypad->adckey_handle) {
		SYS_LOG_WRN("adckey devices open failed");
	}

//	if (!key_device_open(&keypad_scan_callback, TAP_KEY_DEVICE_NAME)) {
//		SYS_LOG_ERR("tapkey devices open failed");
//	}

	keypad->onoff_handle = key_device_open(&keypad_scan_callback, ONOFF_KEY_DEVICE_NAME);
	if (!keypad->onoff_handle) {
		SYS_LOG_WRN("onoffkey devices open failed");
	}

	keypad->gpiokey_handle = key_device_open(&keypad_scan_callback, GPIO_KEY_DEVICE_NAME);
	if (!keypad->gpiokey_handle) {
		SYS_LOG_WRN("gpiokey devices open failed");
	}

	keypad->long_press_timer = LONG_PRESS_TIMER;
	keypad->super_long_press_timer = SUPER_LONG_PRESS_TIMER;
	keypad->super_long_press_6s_timer = SUPER_LONG_PRESS_6S_TIMER;

	keypad->quickly_click_duration = QUICKLY_CLICK_DURATION;
	keypad->key_event_cancel_duration = KEY_EVENT_CANCEL_DURATION;
	keypad->hold_delay_time = HOLD_DELAY_TIME;
	keypad->hold_interval_time = HOLD_INTERVAL_DURATION;
	return 0;
}

void input_keypad_set_init_param(input_dev_init_param *data)
{
	keypad->long_press_timer = data->long_press_timer;
	keypad->super_long_press_timer = data->super_long_press_timer;
	keypad->super_long_press_6s_timer = data->super_long_press_6s_timer;

	keypad->quickly_click_duration = data->quickly_click_duration;
	keypad->key_event_cancel_duration = data->key_event_cancel_duration;
	keypad->hold_delay_time = data->hold_delay_time;
	keypad->hold_interval_time = data->hold_interval_time;
}


int input_keypad_onoff_inquiry_enable(bool inner_onoff, bool enable)
{
	if (!keypad)
		return -1;

	void* handle;

	if (inner_onoff)
		handle = keypad->onoff_handle;
	else
		handle = keypad->adckey_handle;
		
	if(!handle)
		return -1;

	keypad->press_type = 0;
	keypad->press_code = 0;
	keypad->report_press_code = 0;
	keypad->report_key_value = 0;
	keypad->callback_key_value = 0;	

	keypad->press_timer = 0;
	keypad->click_num = 0;
	keypad->key_hold = 0;

	if (enable){
		key_device_abort_cb(handle);
	}else{
		key_device_enable_cb(handle);
	}

	return 0;
}


int input_keypad_onoff_inquiry(bool inner_onoff, int keycode)
{
	if (!keypad)
		return -1;

	void* handle;
	if (inner_onoff)
		handle = keypad->onoff_handle;
	else
		handle = keypad->adckey_handle;

	if(!handle)
		return -1;

	struct input_value val = {0};

	if (!inner_onoff)
	{
		memset(&val, 0, sizeof(struct input_value));
		val.keypad.code = keycode;
	}

	key_device_inquiry(handle, &val);

	if (!inner_onoff)
	{
		SYS_LOG_ERR("keycode:0x%x, rkeycode:0x%x, value:%d",keycode,val.keypad.code,val.keypad.value);
	}

	if (val.keypad.code == keycode)
	{
		return val.keypad.value;
	}

	return KEY_VALUE_UP;
}


