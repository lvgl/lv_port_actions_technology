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

#ifdef CONFIG_SDFS_MANAGER
#include <sdfs_manager.h>
#endif

#ifdef CONFIG_AUDIO
#include <audio_system.h>
#endif

#ifdef CONFIG_MEDIA_SERVICE
#include <media_service.h>
#endif

#include <app_manager.h>
#include <srv_manager.h>
#include <hotplug_manager.h>
#include <app_defines.h>
#include <system_app.h>
#include <sys_monitor.h>
#include <input_manager.h>
#include <fs_manager.h>


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

#ifdef CONFIG_SDFS_MANAGER
	sdfs_manager_init();
#endif

#ifdef CONFIG_MEDIA
	aduio_system_init();
#endif

#ifdef CONFIG_AUDIO
	system_audio_policy_init();
#endif

#ifdef CONFIG_MEDIA_SERVICE
	media_service_init();
#endif

#ifdef CONFIG_INPUT_MANAGER
	input_manager_init(NULL);
	/** input init is locked ,so we must unlock*/
	input_manager_unlock();
#endif

	sys_monitor_init();
	sys_monitor_start();

	#ifdef CONFIG_HOTPLUG_MANAGER
	hotplug_manager_init();
	#endif
	
	app_manager_init();


	if (usb_hotplug_device_mode()) {
		card_reader_mode_check();
	}
	else
	{
#ifdef CONFIG_FS_MANAGER
		fs_manager_disk_init(CONFIG_APP_FAT_DISK);
#endif
	}

	app_manager_active_app(APP_ID_USERAPP);

	while(1)
	{
		os_sleep(1000);
	}
	
	return 0;
}

extern char z_main_stack[CONFIG_MAIN_STACK_SIZE];

APP_DEFINE(main, z_main_stack, CONFIG_MAIN_STACK_SIZE,\
			APP_PRIORITY, RESIDENT_APP,\
			NULL, NULL, NULL,\
			NULL, NULL);

