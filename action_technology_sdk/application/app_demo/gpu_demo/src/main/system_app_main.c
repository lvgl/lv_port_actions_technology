/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system app main
 */

#include <os_common_api.h>
#include <app_manager.h>
#include <app_switch.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <srv_manager.h>
#include <sys_manager.h>
#include <thread_timer.h>
#include <app_ui.h>
#include "system_app.h"

/*share stack for app thread */
char __aligned(ARCH_STACK_PTR_ALIGN) share_stack_area[CONFIG_APP_STACKSIZE];

void main_msg_proc(void *parama1, void *parama2, void *parama3)
{
	struct app_msg msg = {0};
	int result = 0;

	if (receive_msg(&msg, thread_timer_next_timeout())) {
		switch (msg.type) {
	#ifdef CONFIG_INPUT_MANAGER
		case MSG_SR_INPUT:
			system_sr_input_event_handle(msg.ptr);
			break;
		case MSG_KEY_INPUT:
			/**input event means esd proecess finished*/
			system_key_event_handle(msg.value);
			break;
	#endif

		case MSG_START_APP:
			app_switch((char *)msg.ptr, msg.reserve, true);
			break;
		case MSG_EXIT_APP:
			app_manager_exit_app((char *)msg.ptr, true);
			break;

		default:
			break;
		}

		if (msg.callback)
			msg.callback(&msg, result, NULL);
	}

	thread_timer_handle_expired();
}

#ifdef CONFIG_SIMULATOR
int bt_watch_main(void)
#else
int main(void)
#endif
{
#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "main");
#endif

	mem_manager_init();

#ifdef CONFIG_ACTLOG
	act_log_init();
#endif

	/* input manager must initialize before ui service */
	system_input_handle_init();
	system_init();

#ifdef CONFIG_UI_MANAGER
	app_ui_init();
#endif

	system_app_launch_init();

	system_ready();
	system_app_launch(SYS_INIT_NORMAL_MODE);

	while (1) {
		main_msg_proc(NULL, NULL, NULL);
	}
}

extern char z_main_stack[CONFIG_MAIN_STACK_SIZE];

APP_DEFINE(main, z_main_stack, CONFIG_MAIN_STACK_SIZE,\
			APP_PRIORITY, RESIDENT_APP, NULL, NULL, NULL, NULL, NULL);
