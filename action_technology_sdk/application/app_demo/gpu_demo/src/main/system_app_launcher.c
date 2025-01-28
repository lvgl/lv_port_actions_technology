/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system app launcher
 */

#include <os_common_api.h>
#include <app_switch.h>
#include "app_defines.h"

int system_app_launch(uint8_t mode)
{
	char *default_app = APP_ID_LAUNCHER;

	app_switch_add_app(APP_ID_LAUNCHER);

	SYS_LOG_INF("default_app: %s\n", default_app);
	app_switch(default_app, APP_SWITCH_CURR, false);

	return 0;
}

int system_app_launch_init(void)
{
	const char *app_id_list_array[] = app_id_list;

	if (app_switch_init(app_id_list_array, ARRAY_SIZE(app_id_list_array)))
		return -1;

	return 0;
}

