/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os_common_api.h>
#include <sys_event.h>
#include <native_window.h>

#define LONG_PRESS_TIMER           10
#define SUPER_LONG_PRESS_TIMER     50    /* time */
#define SUPER_LONG_PRESS_6S_TIMER  150   /* time */

typedef struct {
	uint32_t press_type;
	uint32_t press_code;
	uint16_t press_timer : 12;
	event_trigger event_cb;
} keypad_t;

/* static prototypes */
static void _keypad_handler(void *, void *, void *);

/* static variables */
static keypad_t keypad;

int input_keypad_device_init(event_trigger event_cb)
{
	keypad.event_cb = event_cb;

	os_thread_create(NULL, 0, _keypad_handler, NULL, NULL,
			 NULL, 0, 0, 0);

	return 0;
}

void input_keypad_set_init_param(input_dev_init_param *data)
{
}

static void _keypad_handler(void *p1, void *p2, void *p3)
{
	input_dev_data_t input_data;
	bool need_report;

	do {
		/* detect key every 40 ms */
        os_sleep(40);

		need_report = false;

		native_window_get_keypad_state(&input_data);

		if (input_data.state == INPUT_DEV_STATE_REL) {
			keypad.press_code = input_data.key;
			switch (keypad.press_type) {
			case KEY_TYPE_LONG_DOWN:
			case KEY_TYPE_LONG:
			case KEY_TYPE_LONG6S:
				keypad.press_type = KEY_TYPE_LONG_UP;
				break;
			default:
				keypad.press_type = KEY_TYPE_SHORT_UP;
				break;
			}

			if (keypad.press_timer > 0) {
				keypad.press_timer = 0;
				need_report = true;
			}
		} else {
			keypad.press_timer++;

			if (keypad.press_timer >= SUPER_LONG_PRESS_6S_TIMER) {
				if (keypad.press_type != KEY_TYPE_LONG6S) {
					keypad.press_type = KEY_TYPE_LONG6S;
					need_report = true;
				}
			} else if (keypad.press_timer >= SUPER_LONG_PRESS_TIMER) {
				if (keypad.press_type != KEY_TYPE_LONG) {
					keypad.press_type = KEY_TYPE_LONG;
					need_report = true;
				}
			} else if (keypad.press_timer >= LONG_PRESS_TIMER)	{
				if (keypad.press_type != KEY_TYPE_LONG_DOWN) {
					keypad.press_type = KEY_TYPE_LONG_DOWN;
					need_report = true;
				}
			}
		}

		if (need_report) {
			uint32_t report_key = keypad.press_type | keypad.press_code;

			if (keypad.event_cb) {
				keypad.event_cb(report_key, EV_KEY);
			}

			if (input_manager_islock()) {
				SYS_LOG_INF("input manager locked");
				continue;
			}

			if (msg_pool_get_free_msg_num() <= (CONFIG_NUM_MBOX_ASYNC_MSGS / 2)) {
				SYS_LOG_INF("drop input msg ... %d", msg_pool_get_free_msg_num());
				continue;
			}

			sys_event_report_input(report_key);
		}
	} while (!native_window_is_closed());
}
