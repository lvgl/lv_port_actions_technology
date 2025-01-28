/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system event notify
 */
#if defined(CONFIG_SYS_LOG)
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "sys_event"
#endif
#include <os_common_api.h>
#include <srv_manager.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <stdio.h>
#include <string.h>
#include <sys_event.h>
#ifdef CONFIG_UI_MANAGER
#include <ui_manager.h>
#endif

#ifdef CONFIG_TWS
#include <btservice_api.h>
#include <bt_manager.h>
#endif

static const struct sys_event_ui_map *sys_event_map = NULL;
static int sys_event_map_size = 0;

static void sys_event_send_message_cb(struct app_msg *msg, int result, void *not_used)
{
	if (msg && msg->ptr)
		mem_free(msg->ptr);
}

void sys_event_send_message_new(uint32_t message, uint8_t cmd, void* extra_data, int extra_data_size)
{
	struct app_msg  msg = {0};

	if (extra_data && extra_data_size) {
		msg.ptr = mem_malloc(extra_data_size + 1);
		if (!msg.ptr){
			SYS_LOG_ERR("mem_malloc fail");
			return;
		}
		memset(msg.ptr, 0, extra_data_size + 1);
		memcpy(msg.ptr, extra_data, extra_data_size);
		msg.callback = sys_event_send_message_cb;
	}

#ifdef CONFIG_TWS
	#ifndef CONFIG_SNOOP_LINK_TWS
	if (bt_manager_tws_get_dev_role() == BTSRV_TWS_MASTER) {
		bt_manager_tws_send_event(TWS_SYSTEM_EVENT, message);
	}
	#endif
#endif	
	msg.type = message;
	msg.cmd = cmd;
	send_async_msg("main", &msg);
}



void sys_event_report_input(uint32_t key_event)
{
	struct app_msg  msg = {0};

	msg.type = MSG_KEY_INPUT;
	msg.value = key_event;
	send_async_msg("main", &msg);
}

void sys_event_notify(uint32_t event)
{
#ifdef CONFIG_TWS
	#ifndef CONFIG_SNOOP_LINK_TWS
	if (bt_manager_tws_get_dev_role() == BTSRV_TWS_MASTER &&
		event != SYS_EVENT_TWS_CONNECTED &&
		event != SYS_EVENT_TWS_DISCONNECTED) {
		/* Tws master/slave all will receive TWS_CONNECTED/TWS_DISCONNECTED event */
		bt_manager_tws_send_event_sync(TWS_UI_EVENT, event);
		return;
	}
	#endif
#endif
	struct app_msg  msg = {0};

	msg.type = MSG_SYS_EVENT;
	msg.cmd = event;

	send_async_msg("main", &msg);
}

void sys_event_map_register(const struct sys_event_ui_map *event_map, int size, int sys_view_id)
{
	if (!sys_event_map) {
		sys_event_map = event_map;
		sys_event_map_size = size;
	} else {
		SYS_LOG_ERR("failed\n");
	}
}

void sys_event_process(uint32_t event)
{
	int ui_event = 0;

	if (!sys_event_map)
		return;

	for (int i = 0; i < sys_event_map_size; i++) {
		if (sys_event_map[i].sys_event == event) {
			ui_event = sys_event_map[i].ui_event;
			break;
		}
	}

	if (ui_event != 0) {
	#ifdef CONFIG_UI_MANAGER
		SYS_LOG_INF("ui_event %d\n", ui_event);
		ui_manager_send_async(0, MSG_VIEW_PAINT, ui_event);
	#endif
	}
}

