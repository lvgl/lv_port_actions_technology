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

#include <system_app.h>
#include <srv_manager.h>
#include <app_defines.h>


char __aligned(ARCH_STACK_PTR_ALIGN) share_stack_area[CONFIG_APP_STACKSIZE];

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

	#ifdef CONFIG_MEDIA_SERVICE
	media_service_init();
	#endif

	#ifdef CONFIG_AUDIO
	system_audio_policy_init();
	#endif

	bool init_ret = app_manager_init();

	SYS_LOG_INF("APP init_ret:%d", init_ret);

	init_ret =  app_manager_active_app(APP_ID_LCMUSIC);
	SYS_LOG_INF("APP active_ret:%d", init_ret);

	while (1) {
		os_sleep(5000);
	}
}

extern char z_main_stack[CONFIG_MAIN_STACK_SIZE];

APP_DEFINE(main, z_main_stack, CONFIG_MAIN_STACK_SIZE,\
			APP_PRIORITY, RESIDENT_APP,\
			NULL, NULL, NULL,\
			NULL, NULL);



