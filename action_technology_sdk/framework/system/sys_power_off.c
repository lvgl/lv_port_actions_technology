/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system power off
 */

#include <os_common_api.h>
#include <app_manager.h>
#include <msg_manager.h>
#include <power_manager.h>
#include <property_manager.h>
#include <esd_manager.h>
#include <tts_manager.h>
#include <string.h>
#include <sys_event.h>
#include <sys_manager.h>
#include <sys_monitor.h>
#include <audio_hal.h>

#ifdef CONFIG_INPUT_MANAGER
#include <input_manager.h>
#endif

#ifdef CONFIG_TTS_MANAGER
#include <tts_manager.h>
#endif

#ifdef CONFIG_BLUETOOTH
#include <bt_manager.h>
#endif

#if defined(CONFIG_SYS_LOG)
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "sys_monitor"
#endif
#ifndef CONFIG_SIMULATOR
#include <soc.h>
#endif
void system_power_off(void)
{
#ifdef CONFIG_INPUT_MANAGER
	input_manager_lock();
#endif

	app_manager_active_app(APP_ID_ILLEGAL);

#ifdef CONFIG_BT_MANAGER
	bt_manager_deinit();
#endif

#ifndef CONFIG_SIMULATOR
	power_manager_save_last_voltage(0);
#endif

	sys_monitor_stop();

#ifdef CONFIG_PROPERTY
	property_flush(NULL);
#endif

	system_deinit();

#ifdef CONFIG_AUDIO
	hal_aout_close_pa();
#endif

#ifdef CONFIG_POWER
	if (!power_manager_get_dc5v_status()) {
	#ifdef BOARD_POWER_LOCK_GPIO
		board_power_lock_enable(false);
	#endif
		SYS_LOG_DBG("poweroff");
		power_manager_set_before_poweroff();
		sys_pm_poweroff();
	} else {	
		SYS_LOG_DBG("reboot");
		sys_pm_reboot(REBOOT_TYPE_GOTO_SYSTEM | REBOOT_REASON_NORMAL);
	}
#endif
}

void system_power_reboot(int reason)
{
#if 0
#ifndef CONFIG_SIMULATOR
	u32_t bat_vol_saved = power_manager_get_battery_vol()/1000;
	soc_pstore_set(SOC_PSTORE_TAG_CAPACITY, bat_vol_saved);
	soc_pstore_set(SOC_PSTORE_TAG_FLAG_CAP, 1);
	SYS_LOG_INF("bat_vol_saved:	%d\n", bat_vol_saved);
#endif
#endif

#ifdef CONFIG_INPUT_MANAGER
	input_manager_lock();
#endif

#ifdef CONFIG_TTS_MANAGER
	tts_manager_lock();
#endif

	app_manager_active_app(APP_ID_ILLEGAL);

#ifdef CONFIG_BT_MANAGER
	bt_manager_deinit();
#endif

#ifdef CONFIG_POWER
	power_manager_save_last_voltage(0);
#endif

	sys_monitor_stop();

#ifdef CONFIG_PROPERTY
	property_flush(NULL);
#endif

	system_deinit();

#ifndef CONFIG_SIMULATOR
	sys_pm_reboot(reason);
#endif

}

void system_power_get_reboot_reason(uint16_t *reboot_type, uint8_t *reason)
{
#ifndef CONFIG_SIMULATOR
	union sys_pm_wakeup_src src = {0};

	if (!sys_pm_get_wakeup_source(&src)) {
		*reason = 0;

		if (src.t.alarm) {
			*reboot_type = REBOOT_TYPE_ALARM;
		} else if (src.t.watchdog) {
			*reboot_type = REBOOT_TYPE_WATCHDOG;
		} else if (src.t.onoff_reset) {
			*reboot_type = REBOOT_TYPE_ONOFF_RESET;
		} else if (sys_pm_get_reboot_reason(reboot_type, reason) == 0) {
			*reboot_type = REBOOT_TYPE_SF_RESET;
		} else {
			*reboot_type = REBOOT_TYPE_HW_RESET;
			if (src.t.short_onoff) {
				*reason = HW_RESET_REASON_ONOFF_SHORT;
			} else if (src.t.long_onoff) {
				*reason = HW_RESET_REASON_ONOFF_LONG;
			} else if (src.t.wio) {
				*reason = HW_RESET_REASON_WIO;
			} else if (src.t.remote) {
				*reason = HW_RESET_REASON_REMOTE;
			} else if (src.t.dc5vin) {
				*reason = HW_RESET_REASON_DC5V;
			} else {
				*reason = HW_RESET_REASON_BAT;
			}
		}
	}

	SYS_LOG_INF("reboot_type: 0x%x, reason %d\n", *reboot_type, *reason);
#endif
	return;
}

