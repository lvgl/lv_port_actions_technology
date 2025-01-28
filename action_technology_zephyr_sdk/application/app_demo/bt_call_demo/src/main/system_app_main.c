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
#include <app_manager.h>
#include <property_manager.h>

#include <system_app.h>
#include <app_defines.h>
#include <btcall.h>


char __aligned(ARCH_STACK_PTR_ALIGN) share_stack_area[CONFIG_APP_STACKSIZE];
void main_msg_proc(void *parama1, void *parama2, void *parama3)
{
	struct app_msg msg = {0};
	int result = 0;

	if (receive_msg(&msg, thread_timer_next_timeout())) {
		SYS_LOG_INF("msg type:%d cmd:%d", msg.type, msg.cmd);
		if (msg.callback)
			msg.callback(&msg, result, NULL);
	}

	thread_timer_handle_expired();
}


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

	bool init_ret = app_manager_init();

	SYS_LOG_INF("APP init_ret:%d", init_ret);

	init_ret =  app_manager_active_app(APP_ID_BTCALL);
	SYS_LOG_INF("APP active_ret:%d", init_ret);

	memset(bt_name, 0, sizeof(bt_name));
	property_get(CFG_BT_NAME, bt_name, sizeof(bt_name));
	SYS_LOG_INF("bt_name:%s", bt_name);

	while (1) {
		main_msg_proc(NULL, NULL, NULL);
	}
}

extern char z_main_stack[CONFIG_MAIN_STACK_SIZE];

APP_DEFINE(main, z_main_stack, CONFIG_MAIN_STACK_SIZE,\
			APP_PRIORITY, RESIDENT_APP,\
			NULL, NULL, NULL,\
			NULL, NULL);



