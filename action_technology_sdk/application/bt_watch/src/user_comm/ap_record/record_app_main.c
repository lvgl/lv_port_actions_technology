/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <mem_manager.h>
#include <app_manager.h>
#include <srv_manager.h>
#include <volume_manager.h>
#include <msg_manager.h>
#include <thread_timer.h>
#include <media_player.h>
#include <audio_system.h>
#include <audio_policy.h>
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>
#include "app_defines.h"
#include "sys_manager.h"
#include "app_ui.h"
	
#include <user_comm/ap_record.h>
#include "tts_manager.h"
#include "buffer_stream.h"
#include "ringbuff_stream.h"
#include "media_mem.h"


#ifdef CONFIG_PROPERTY
#include "property_manager.h"
#endif

#ifdef CONFIG_TOOL
#include "tool_app.h"
#endif

#include "ap_record_private.h"


static struct record_app_t *p_record = NULL;

extern uint8_t record_app_sent_record_start(void)
{
	struct app_msg msg = { 0 };

	msg.type = MSG_INPUT_EVENT;
	msg.cmd = MSG_AP_RECORD_START;
	send_async_msg(APP_ID_AP_RECORD, &msg);
	return 0;
}

static int _record_init(void)
{
	if (p_record) {
		return 0;
	}

	p_record = app_mem_malloc(sizeof(struct record_app_t));
	if (!p_record) {
		SYS_LOG_ERR("malloc failed!\n");
		return -ENOMEM;
	}

	memset(p_record, 0, sizeof(struct record_app_t));

	SYS_LOG_INF("ok\n");
	return 0;
}

void _record_exit(void)
{
	if (!p_record) {
		goto exit;
	}
	if (p_record->playing) {
		record_stop_record();
	}

	app_mem_free(p_record);
	p_record = NULL;

#ifdef CONFIG_PROPERTY
	property_flush(NULL);
#endif

exit:
	app_manager_thread_exit(app_manager_get_current_app());
	SYS_LOG_INF("ok\n");

}

struct record_app_t *record_get_app(void)
{
	return p_record;
}

static void _record_main_loop(void *parama1, void *parama2, void *parama3)
{
	struct app_msg msg = { 0 };

	bool terminated = false;

	if (_record_init()) {
		SYS_LOG_ERR("init failed");
		_record_exit();
		return;
	}

	while (!terminated) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			SYS_LOG_INF("type %d,cmd %d value %d\n", msg.type, msg.cmd, msg.value);
			switch (msg.type) {
			case MSG_INPUT_EVENT:
				record_input_event_proc(&msg);
				break;

			case MSG_TTS_EVENT:
				record_tts_event_proc(&msg);
				break;

			case MSG_AP_RECORD_EVENT:
				record_event_proc(&msg);
				break;

			case MSG_EXIT_APP:
				_record_exit();
				terminated = true;
				break;
			default:
				break;
			}
			if (msg.callback)
				msg.callback(&msg, 0, NULL);
		}
		if (!terminated)
			thread_timer_handle_expired();
	}
}

APP_DEFINE(ap_record, share_stack_area, sizeof(share_stack_area),
	CONFIG_APP_PRIORITY, FOREGROUND_APP, NULL, NULL, NULL,
	_record_main_loop, NULL);

