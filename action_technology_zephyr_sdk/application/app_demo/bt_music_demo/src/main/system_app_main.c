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
#ifdef CONFIG_BT_MANAGER
#include <bt_manager.h>
#endif
#include <property_manager.h>
#include <app_manager.h>

#include <system_app.h>

#include <app_defines.h>


char __aligned(ARCH_STACK_PTR_ALIGN) share_stack_area[CONFIG_APP_STACKSIZE];

int main(void)
{
	char bt_name[48];

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

	#ifdef CONFIG_BT_MANAGER
	bt_manager_init();
	#endif

	#ifdef CONFIG_AUDIO
	system_audio_policy_init();
	#endif

	app_manager_init();
	app_manager_active_app(APP_ID_DEFAULT);

	while (1) {
		memset(bt_name, 0, sizeof(bt_name));
		property_get(CFG_BT_NAME, bt_name, sizeof(bt_name));
		SYS_LOG_INF("bt_name:%s", bt_name);
		os_sleep(2000);
	}
}


