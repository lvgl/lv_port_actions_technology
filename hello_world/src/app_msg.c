/*
 * Copyright (c) 2019 Actions Semiconductor Co, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os_common_api.h>
#include "app_msg.h"

int app_msg_init(void)
{
	bool ret;
	const char *name = k_thread_name_get(os_current_get());

	ret = msg_manager_add_listener((char*)name, os_current_get());

	return (ret ? 0 : -1);
}

int app_msg_send(const char* receiver, uint8_t type, uint8_t cmd)
{
	bool ret;
	struct app_msg msg;

	memset(&msg, 0, sizeof(msg));
	msg.type = type;
	msg.cmd = cmd;
	ret = send_async_msg((char*)receiver, &msg);

	return (ret ? 0 : -1);
}

