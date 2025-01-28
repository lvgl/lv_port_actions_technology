/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system app launcher
 */

#include <os_common_api.h>
#include <string.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <app_manager.h>
#include <hotplug_manager.h>
#ifdef CONFIG_UI_MANAGER
#include <ui_manager.h>
#endif
#include "app_ui.h"
#include "app_switch.h"
#include "app_defines.h"
#include "system_app.h"
#ifdef CONFIG_SD_FS
#include <sdfs.h>
#endif

#if defined(CONFIG_SYS_LOG)
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "system app launcher"
#endif
extern int ota_app_init(void);
#ifdef CONFIG_OTA_FOREGROUND
extern bool ota_is_already_running(void);
extern int res_inc_file_check(void);
#endif

static int res_check_failed = 0;

static int _res_file_check(void)
{
	/** check res resource */
	res_check_failed = sdfs_verify(CONFIG_APP_UI_DISK);
	if (!res_check_failed) {
		/** check fonts resource */
		res_check_failed = sdfs_verify(CONFIG_APP_FONT_DISK);
	}
	
#ifdef CONFIG_OTA_FOREGROUND
	/** check inc ui resource */
	if (!res_check_failed) {
		res_check_failed = res_inc_file_check();
	}
#endif

	return res_check_failed;
}

int system_app_launch(uint8_t mode)
{
	char *default_app = APP_ID_DEFAULT;

	/*waku up by alarm.switch to alarm app*/
	if (mode == SYS_INIT_ALARM_MODE) {
		app_switch_add_app(APP_ID_LAUNCHER);
		app_switch(APP_ID_LAUNCHER, APP_SWITCH_NEXT, false);
		//app_switch_lock(1);
		return 0;
	}

	app_switch_add_app(APP_ID_LAUNCHER);
	/* Check the integrity of the resource file,if no integrity,don't visit resource file,switch to ota app */
#ifdef CONFIG_OTA_FOREGROUND
	if (ota_is_already_running() || res_check_failed) {
		app_switch(APP_ID_OTA, APP_SWITCH_CURR, true);
		return 0;
	}
#endif

	SYS_LOG_INF("default_app: %s\n", default_app);
	if (app_switch(default_app, APP_SWITCH_CURR, false) == 0xff) {
		app_switch(APP_ID_LAUNCHER, APP_SWITCH_CURR, false);
	}

#if defined(CONFIG_OTA_PRODUCT_SUPPORT) || defined(CONFIG_OTA_BLE_MASTER_SUPPORT)
	struct app_msg msg = {0};
	msg.type = MSG_START_APP;
	msg.ptr = APP_ID_OTA_PRODUCT;
	msg.reserve = APP_SWITCH_CURR;
	send_async_msg(APP_ID_MAIN, &msg);
#endif
	return 0;
}

int system_app_launch_init(void)
{
	const char *app_id_list_array[] = app_id_list;
	int prio = os_thread_priority_get(os_current_get());

	if (app_switch_init(app_id_list_array, ARRAY_SIZE(app_id_list_array)))
		return -EINVAL;

#ifndef CONFIG_SIMULATOR
#ifdef CONFIG_UI_MANAGER
	ui_view_create(WELCOME_VIEW, NULL, UI_CREATE_FLAG_SHOW | UI_CREATE_FLAG_NO_PRELOAD);
#endif

	/**lower priority */
	os_thread_priority_set(os_current_get(), prio + 5);

	_res_file_check();

#ifdef CONFIG_OTA_APP
	ota_app_init();
#endif

	/**restore priority */
	os_thread_priority_set(os_current_get(), prio);

	os_sleep(1000);

#ifdef CONFIG_UI_MANAGER
	ui_view_delete(WELCOME_VIEW);
#endif
#endif /* CONFIG_SIMULATOR */

	return 0;
}
