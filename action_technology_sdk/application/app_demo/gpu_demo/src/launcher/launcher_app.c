/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file launcher_app.c
 */

/*********************
 *      INCLUDES
 *********************/
#include <os_common_api.h>
#include <app_manager.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <srv_manager.h>
#include <thread_timer.h>
#include <app_defines.h>
#include "launcher_app.h"
#include "launcher_ui.h"

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void _launcher_key_event_handle(uint16_t view_id, uint32_t event);

/**********************
 *   STATIC FUNCTIONS
 **********************/
static int _launcher_app_init(void)
{
	ui_manager_set_keyevent_callback(_launcher_key_event_handle);
	app_ui_stack_forward(MAIN_UI, APP_UI_LOAD_ANIM_NONE);
	return 0;
}

static void _launcher_app_exit(void)
{
	app_manager_thread_exit(APP_ID_LAUNCHER);
}

static void _launcher_key_event_handle(uint16_t view_id, uint32_t event)
{
	SYS_LOG_INF("view %u, key 0x%x", view_id, event);

	os_strace_u32(SYS_TRACE_ID_KEY_READ, event);

	if (KEY_VALUE(event) == KEY_GESTURE_RIGHT) {
		app_ui_stack_back_gestured(DEF_UI_LOAD_ANIM_TYPE);
	}
}

void launcher_app_loop(void *p1, void *p2, void *p3)
{
	struct app_msg msg = { 0 };
	bool terminated = false;

	SYS_LOG_INF(APP_ID_LAUNCHER " enter");

	if (_launcher_app_init()) {
		SYS_LOG_ERR(APP_ID_LAUNCHER " init failed");
		_launcher_app_exit();
		goto out_exit;
	}

	while (!terminated) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			switch (msg.type) {
			case MSG_EXIT_APP:
				_launcher_app_exit();
				terminated = true;
				break;
			case MSG_KEY_INPUT:
				_launcher_key_event_handle(VIEW_INVALID_ID, msg.value);
				break;
			default:
				break;
			}

			if (msg.callback != NULL)
				msg.callback(&msg, 0, NULL);
		}

		thread_timer_handle_expired();
	}

out_exit:
	SYS_LOG_INF(APP_ID_LAUNCHER " exit");
}

APP_DEFINE(launcher, share_stack_area, sizeof(share_stack_area),
		CONFIG_APP_PRIORITY, DEFAULT_APP | FOREGROUND_APP, NULL, NULL, NULL,
		launcher_app_loop, NULL);
