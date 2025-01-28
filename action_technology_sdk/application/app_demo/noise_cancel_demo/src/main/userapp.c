/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file key tone
 */
#include <os_common_api.h>
#include <app_manager.h>
#include <mem_manager.h>
#include <srv_manager.h>
#include <noise_cancel.h>
#include "app_defines.h"
#include "userapp.h"

void nc_start_cb(struct app_msg* msg, int result, void *reply)
{
	app_mem_free(msg->ptr);
}

void nc_start(char *save_url)
{
	if (!save_url)
		return;

	char *url = app_mem_malloc(strlen(save_url)+1);
	memset(url, 0, strlen(save_url)+1);
	memcpy(url, save_url, strlen(save_url));
	struct app_msg msg = {0};
	msg.cmd = NC_START;
	msg.ptr = url;
	msg.callback = (MSG_CALLBAK)nc_start_cb;
	send_async_msg(APP_ID_USERAPP, &msg);
}

void nc_mp3_start(char *save_url)
{
	if (!save_url)
		return;

	char *url = app_mem_malloc(strlen(save_url)+1);
	memset(url, 0, strlen(save_url)+1);
	memcpy(url, save_url, strlen(save_url));
	struct app_msg msg = {0};
	msg.cmd = NC_MP3_START;
	msg.ptr = url;
	msg.callback = (MSG_CALLBAK)nc_start_cb;
	send_async_msg(APP_ID_USERAPP, &msg);
}

void nc_stop(void)
{
	struct app_msg msg = {0};

	msg.cmd = NC_STOP;
	send_async_msg(APP_ID_USERAPP, &msg);
}

void nc_get_vad(void)
{
	struct app_msg msg = {0};

	msg.cmd = NC_GET_VAD;
	send_async_msg(APP_ID_USERAPP, &msg);
}


static void userapp_loop(void *p1, void *p2, void *p3)
{
	struct app_msg msg = {0};

	SYS_LOG_INF(APP_ID_USERAPP " enter");

	while (1) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			switch (msg.cmd) {
			case NC_START:
				noise_cancel_start(msg.ptr);
				break;
			
			case NC_MP3_START:
				noise_cancel_mp3_start(msg.ptr);
				break;
				
			case NC_STOP:
				noise_cancel_stop();
				break;

			case NC_GET_VAD:
				noise_cancel_get_vad();
				break;
			default:
				SYS_LOG_INF("Unknown massage");
				break;
			}

			if (msg.callback)
				msg.callback(&msg, 0, NULL);

		}

		thread_timer_handle_expired();
	}

	SYS_LOG_INF(APP_ID_USERAPP " exit");
}


APP_DEFINE(userapp, share_stack_area, sizeof(share_stack_area),
		CONFIG_APP_PRIORITY, DEFAULT_APP | FOREGROUND_APP, NULL, NULL, NULL,
	   userapp_loop, NULL);


