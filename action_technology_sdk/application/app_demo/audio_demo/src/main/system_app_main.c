/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system app main
 */
#include <mem_manager.h>
#include <msg_manager.h>
#ifdef CONFIG_FS_MANAGER
#include <fs_manager.h>
#endif
#ifdef CONFIG_AUDIO
#include <audio_system.h>
#endif

#include <app_manager.h>
#include <srv_manager.h>
#include <hotplug_manager.h>
#include <app_defines.h>
#include <system_app.h>
#include <sys_monitor.h>

void system_audio_policy_init(void);
char __aligned(ARCH_STACK_PTR_ALIGN) share_stack_area[CONFIG_APP_STACKSIZE];
extern int card_reader_mode_check(void);
extern bool usb_hotplug_device_mode(void);

int main(void)
{
	mem_manager_init();
	msg_manager_init();
	#ifdef CONFIG_ACTLOG
	act_log_init();
	#endif

	#ifdef CONFIG_FS_MANAGER
	fs_manager_init();
	#endif

	#ifdef CONFIG_MEDIA
	aduio_system_init();
	#endif

	system_audio_policy_init();


	sys_monitor_init();
	sys_monitor_start();
	#ifdef CONFIG_HOTPLUG_MANAGER
	hotplug_manager_init();
	#endif
	app_manager_init();

	if (usb_hotplug_device_mode()) {
		card_reader_mode_check();
	}
	app_manager_active_app(APP_ID_DEFAULT);

	while (1) {
		k_sleep(K_MSEC(10000));
	}
}

extern char z_main_stack[CONFIG_MAIN_STACK_SIZE];

APP_DEFINE(main, z_main_stack, CONFIG_MAIN_STACK_SIZE,\
			APP_PRIORITY, RESIDENT_APP,\
			NULL, NULL, NULL,\
			NULL, NULL);

