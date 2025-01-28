/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file key tone
 */
#include <os_common_api.h>
#include <audio_system.h>
#include <media_player.h>
#include <buffer_stream.h>
#include <file_stream.h>
#include <app_manager.h>
#include <mem_manager.h>
#include <tts_manager.h>
#include <audio_track.h>
#include <fs/fs.h>

#include <ringbuff_stream.h>
#include <stream.h>

#include <bt_manager.h>
#include <srv_manager.h>
#include <btcall.h>


static void btcall_app_loop(void *p1, void *p2, void *p3)
{
	struct app_msg msg = {0};
	bool terminated = false;

	SYS_LOG_INF(APP_ID_BTCALL " enter");

	while (!terminated) {
		if (receive_msg(&msg, OS_FOREVER)) {
			switch (msg.type) {
			case MSG_EXIT_APP:
				app_manager_thread_exit(APP_ID_BTCALL);
				terminated = true;
				break;
			case MSG_BT_EVENT:
					btcall_bt_event_proc(&msg);
				break;
			case MSG_INPUT_EVENT:
					btcall_input_event_proc(&msg);
				break;
			default:
				break;
			}

			if (msg.callback != NULL)
				msg.callback(&msg, 0, NULL);
		}
	}

	SYS_LOG_INF(APP_ID_BTCALL " exit");
}


APP_DEFINE(btcall, share_stack_area, sizeof(share_stack_area),
		CONFIG_APP_PRIORITY, DEFAULT_APP | FOREGROUND_APP, NULL, NULL, NULL,
	   btcall_app_loop, NULL);


