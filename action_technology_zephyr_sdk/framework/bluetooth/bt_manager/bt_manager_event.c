/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager event notify.
 */
#define SYS_LOG_NO_NEWLINE
#define SYS_LOG_DOMAIN "bt manager"

#include <os_common_api.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>
#include <msg_manager.h>
#include <mem_manager.h>
#include <app_manager.h>
#include "bt_manager_inner.h"
#include "btservice_api.h"
#include "bt_porting_inner.h"

static void _bt_manager_event_notify_cb(struct app_msg *msg, int result, void *not_used)
{
	if (msg && msg->ptr)
		bt_mem_free(msg->ptr);
}

int bt_manager_event_notify(int event_id, void *event_data, int event_data_size)
{
	struct app_msg  msg = {0};
	char *fg_app = app_manager_get_current_app();

	if (!fg_app)
		return -ENODEV;

	/**ota not deal bt event when process*/
	if (memcmp(fg_app, "ota", strlen("ota")) == 0)
		return 0;

	if (event_data && event_data_size) {
		msg.ptr = bt_mem_malloc(event_data_size + 1);
		if (!msg.ptr)
			return -ENOMEM;
		memset(msg.ptr, 0, event_data_size + 1);
		memcpy(msg.ptr, event_data, event_data_size);
		msg.callback = _bt_manager_event_notify_cb;
	}

	msg.type = MSG_BT_EVENT;
	msg.cmd = event_id;

	return send_async_msg(fg_app, &msg);
}

int bt_manager_event_notify_ext(int event_id, void *event_data, int event_data_size , void* call_cb)
{
	struct app_msg  msg = {0};
	char *fg_app = app_manager_get_current_app();

	if (!fg_app)
		return -ENODEV;

	/**ota not deal bt event when process*/
	if (memcmp(fg_app, "ota", strlen("ota")) == 0)
		return -ENODEV;

	if (event_data && event_data_size) {
		msg.ptr = bt_mem_malloc(event_data_size + 1);
		if (!msg.ptr)
			return -ENOMEM;
		memset(msg.ptr, 0, event_data_size + 1);
		memcpy(msg.ptr, event_data, event_data_size);
		msg.callback = call_cb;
	}

	msg.type = MSG_BT_EVENT;
	msg.cmd = event_id;

	return send_async_msg(fg_app, &msg);
}

int bt_manager_state_notify(int state)
{
	struct app_msg  msg = {0};

	msg.type = MSG_BT_EVENT;
	msg.cmd = state;

	return send_async_msg("main", &msg);
}

