/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system manager
 */

#include <os_common_api.h>
#include <msg_manager.h>
#include <app_manager.h>
#include <srv_manager.h>
#ifdef CONFIG_FS_MANAGER
#include <fs_manager.h>
#endif
#ifdef CONFIG_SDFS_MANAGER
#include <sdfs_manager.h>
#endif
#ifdef CONFIG_UI_MANAGER
#include <ui_manager.h>
#endif
#include <hotplug_manager.h>
#include <esd_manager.h>
#ifdef CONFIG_AUDIO
#include <audio_system.h>
#endif
#ifdef CONFIG_SENSOR_MANAGER
#include <sensor_manager.h>
#endif
#ifdef CONFIG_GPS_MANAGER
#include <gps_manager.h>
#endif
#include <sys_monitor.h>
#include <power_manager.h>
#include <property_manager.h>
#ifdef CONFIG_PLAYTTS
#include <tts_manager.h>
#endif

#ifdef CONFIG_MEDIA_SERVICE
#include <media_service.h>
#endif

#include <sys_wakelock.h>

#ifdef CONFIG_FM
#include <fm_manager.h>
#endif

void system_ready(void)
{
	struct sys_monitor_t *sys_monitor = sys_monitor_get_instance();

	if (sys_monitor)
		sys_monitor->system_ready = 1;
	
#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_unlock(FULL_WAKE_LOCK);
#endif
}

bool system_is_ready(void)
{
	struct sys_monitor_t *sys_monitor = sys_monitor_get_instance();

	if (sys_monitor)
		return (sys_monitor->system_ready == 1);

	return false;
}

int system_restore_factory_config(void)
{

#ifdef CONFIG_PROPERTY
	property_flush(NULL);
#endif

	return 0;
}

void system_init(void)
{
#ifdef CONFIG_FS_MANAGER
	fs_manager_init();
#endif

#ifdef CONFIG_FILE_SYSTEM_LITTLEFS
	littlefs_manager_init();
#endif

#ifdef CONFIG_SDFS_MANAGER
	sdfs_manager_init();
#endif

	msg_manager_init();

	srv_manager_init();

	app_manager_init();

#ifdef CONFIG_MEDIA_SERVICE
	media_service_init();
#endif

	sys_monitor_init();

#ifdef CONFIG_ESD_MANAGER
	esd_manager_init();
#endif

#ifdef CONFIG_MEDIA
	aduio_system_init();
#endif

#ifdef CONFIG_PLAYTTS
	tts_manager_init();
#endif

#ifdef CONFIG_POWER_MANAGER
	power_manager_init();
#endif

#ifdef CONFIG_UI_MANAGER
	ui_manager_init();
#endif

#ifdef CONFIG_HOTPLUG_MANAGER
	hotplug_manager_init();
#endif

#ifdef CONFIG_FM
	fm_manager_init();
#endif

#ifdef CONFIG_SENSOR_MANAGER
	sensor_manager_init();
#endif

#ifdef CONFIG_GPS_MANAGER
	gps_manager_init();
#endif

#ifdef CONFIG_FS_MANAGER_INIT_CHECK
	fs_manager_check();
#endif

	sys_monitor_start();

}

void system_deinit(void)
{
	sys_monitor_stop();

#ifdef CONFIG_UI_MANAGER
	ui_manager_exit();
#endif

#ifdef CONFIG_PLAYTTS
	tts_manager_deinit();
#endif

#ifdef CONFIG_ESD_MANAGER
	esd_manager_deinit();
#endif

#ifdef CONFIG_FM
	fm_manager_deinit();
#endif
}
