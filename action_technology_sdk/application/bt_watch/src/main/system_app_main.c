/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system app main
 */
#include <app_manager.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <thread_timer.h>
#include <sys_event.h>
#include "app_switch.h"
#include "system_app.h"
#include "app_ui.h"
#include <stream.h>
#include <property_manager.h>
#ifdef CONFIG_PLAYTTS
#include "tts_manager.h"
#endif

#ifdef CONFIG_BLUETOOTH
#include "mem_manager.h"
#include "bt_manager.h"
#endif

#ifdef CONFIG_TOOL
#include "tool_app.h"
#endif

#ifdef CONFIG_SENSOR_MANAGER
#include <sensor_manager.h>
#endif

#ifndef CONFIG_SIMULATOR
#include <drivers/power_supply.h>
#endif

#ifdef CONFIG_ACTLOG
#include <logging/act_log.h>
#endif

#ifdef CONFIG_BT_LOG_SERVICE
#include <bt_log/log_service.h>
#endif

#ifdef CONFIG_UI_MANAGER
#include "alipay/alipay_ui.h"
#endif

#ifdef CONFIG_SYS_WAKELOCK
#include "sys_wakelock.h"
#endif

#ifdef CONFIG_GLYPHIX
#include "glyphix_ats.h"
#include "gx_lvgx.h"
#endif

#ifdef CONFIG_USING_AICXTEK_LIB 
extern void aic_srv_init(void);
#endif
#ifdef CONFIG_AWK_LIB
extern int awk_service_start(void);
#endif

/*share stack for app thread */
char __aligned(ARCH_STACK_PTR_ALIGN) share_stack_area[CONFIG_APP_STACKSIZE];

extern void system_library_version_dump(void);
extern void trace_init(void);
extern int trace_dma_print_set(unsigned int dma_enable);
extern int card_reader_mode_check(void);
extern bool alarm_wakeup_source_check(void);
extern int ota_app_init_sdcard(void);
extern int ota_app_init_bluetooth(void);
extern bool ota_is_already_done(void);
extern int vendor_app_init(void);
extern void tool_sysrq_init();
extern int charger_mode_check(void);

static bool att_enter_bqb_flag = false;
static uint8_t super_enter_bqb_mode = 0;
static uint8_t boot_sys_event = SYS_EVENT_NONE;

#ifdef CONFIG_SPP_TEST_SUPPORT
extern int spp_test_app_init(void);
#endif

#ifdef CONFIG_NSM_APP
extern int nsm_test_app_init(void);
#endif

void main_msg_proc(void *parama1, void *parama2, void *parama3)
{
	struct app_msg msg = {0};
	int result = 0;

	if (receive_msg(&msg, thread_timer_next_timeout())) {

		switch (msg.type) {
#ifndef CONFIG_SIMULATOR
		case MSG_BAT_CHARGE_EVENT:
			if ((msg.cmd == BAT_CHG_EVENT_DC5V_IN)
				|| (msg.cmd == BAT_CHG_EVENT_DC5V_OUT)) {
				send_async_msg(app_manager_get_current_app(), &msg);
			} else if (msg.cmd == BAT_CHG_EVENT_BATTERY_TOO_LOW) {
				system_power_off();
            }
			break;
#endif
		case MSG_SYS_EVENT:
			if (msg.cmd == SYS_EVENT_BATTERY_TOO_LOW) {
				system_power_off();
			}else if (msg.cmd == SYS_EVENT_BATTERY_LOW) {
				result = send_async_msg(app_manager_get_current_app(), &msg);
				if (!result) {
					boot_sys_event = SYS_EVENT_BATTERY_LOW;
				}
			} else {
				system_bluetooth_event_handle(&msg);
				sys_event_process(msg.cmd);
			}
			break;

		#ifdef CONFIG_INPUT_MANAGER
		case MSG_SR_INPUT:
			system_sr_input_event_handle(msg.ptr);
			break;
		#endif

		#ifdef CONFIG_PLAYTTS
		case MSG_TTS_EVENT:
			if (msg.cmd == TTS_EVENT_START_PLAY) {
				tts_manager_play_process();
			}
			break;
		#endif
		#ifdef CONFIG_INPUT_MANAGER
		case MSG_KEY_INPUT:
			/**input event means esd proecess finished*/
			system_input_event_handle(msg.value);
			break;
		#endif

		case MSG_INPUT_EVENT:
			system_key_event_handle(&msg);
			break;

		case MSG_HOTPLUG_EVENT:
			system_hotplug_event_handle(&msg);
			break;

		case MSG_VOLUME_CHANGED_EVENT:
			//system_app_volume_show(&msg);
			break;

		case MSG_POWER_OFF:
		#ifdef CONFIG_UI_MANAGER
			ui_view_create(GOODBYE_VIEW, NULL, UI_CREATE_FLAG_SHOW | UI_CREATE_FLAG_NO_PRELOAD);
		#endif
			os_sleep(500);
			#ifdef CONFIG_GLYPHIX
			system_app_ui_deinit();
			glyphix_ats_finish();
			#endif
			system_power_off();
			break;
		case MSG_REBOOT:
			#ifdef CONFIG_GLYPHIX
			system_app_ui_deinit();
			glyphix_ats_finish();
			#endif
			system_power_reboot(msg.cmd);
			break;
		case MSG_NO_POWER:
			sys_event_notify(SYS_EVENT_POWER_OFF);
			break;
		case MSG_BT_ENGINE_READY:
		#ifdef CONFIG_BT_LOG_SERVICE
			bt_log_service_start();
		#endif
		#ifdef CONFIG_ALARM_APP
			if (alarm_wakeup_source_check()) {
				system_app_launch(SYS_INIT_ALARM_MODE);
			} else
		#endif
			{
				system_app_launch(SYS_INIT_NORMAL_MODE);
			}

		#ifdef CONFIG_OTA_BACKEND_BLUETOOTH
		#ifndef CONFIG_OTA_PRODUCT_SUPPORT
		#ifndef CONFIG_OTA_BLE_MASTER_SUPPORT
			ota_app_init_bluetooth();
		#endif
		#endif
		#endif
		#ifdef CONFIG_BT_VENDOR
			vendor_app_init();
		#endif

		#ifdef CONFIG_UI_MANAGER
			alipay_ui_init();
			wxpay_ui_init();
		#endif
		#ifdef CONFIG_AWK_LIB
			awk_service_start();
		#endif

		#ifdef CONFIG_SENSOR_MANAGER
			sensor_manager_enable(ALGO_ACTIVITY_OUTPUT, 0);
			sensor_manager_enable(ALGO_HANDUP, 0);
		#endif

			if (att_enter_bqb_flag == true) {
				SYS_LOG_INF("ATT Goto BQB TEST");
				os_thread_priority_set(os_current_get(), 0);
			}
			if (boot_sys_event != SYS_EVENT_NONE) {
				msg.type = MSG_SYS_EVENT;
				msg.cmd = boot_sys_event;
				send_async_msg(app_manager_get_current_app(), &msg);
				boot_sys_event = SYS_EVENT_NONE;
			}

			#ifdef CONFIG_SPP_TEST_SUPPORT
			spp_test_app_init();
			#endif

			#ifdef CONFIG_NSM_APP
			nsm_test_app_init();
			#endif

			#ifdef CONFIG_GLYPHIX
			glyphix_ats_start();
			#endif
			break;

		case MSG_START_APP:
			SYS_LOG_INF("start %s\n", (char *)msg.ptr);
			app_switch((char *)msg.ptr, msg.reserve, true);
			break;

		case MSG_EXIT_APP:
			SYS_LOG_DBG("exit %s\n", (char *)msg.ptr);
			app_manager_exit_app((char *)msg.ptr, true);
			break;

#ifdef CONFIG_BT_BLE
#ifdef CONFIG_BT_ANCS_AMS
		case MSG_BLE_ANCS_AMS_SERVICE:
            bt_manager_ble_ancs_ams_handle(msg.cmd,(void *)msg.value);
		    break;
#endif
#endif

#ifdef CONFIG_BT_ACTIONS_SUPER_SERVICE
		case MSG_APP_BLE_SUPER_BQB:
			system_app_launch(SYS_INIT_NORMAL_MODE);
			if (super_enter_bqb_mode) {
				super_enter_bqb_mode = 0;
				os_thread_priority_set(os_current_get(), 0);
			}
			break;
#endif

#ifdef CONFIG_TOOL
		case MSG_APP_TOOL_INIT:
			tool_sysrq_init();
			break;
#endif

		case MSG_BT_PAIRING_EVENT:
			SYS_LOG_INF(" MSG_BT_PAIRING_EVENT: cmd:%d value:%d", msg.cmd,msg.value);
			break;

#ifdef CONFIG_BT_MANAGER
		case MSG_BT_DEVICE_ERROR:
			bt_manager_reset_btdev();
			break;
#endif
		default:
			SYS_LOG_ERR(" error: %d\n", msg.type);
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
	bool play_welcome = true;
	bool init_bt_manager = true;
	uint16_t reboot_type = 0;
	uint8_t reason = 0;

	system_power_get_reboot_reason(&reboot_type, &reason);

	system_library_version_dump();

	mem_manager_init();

#ifdef CONFIG_ACTLOG
	act_log_init();
#endif

	/* input manager must initialize before ui service */
	system_input_handle_init();

	system_init();

#ifdef CONFIG_USING_AICXTEK_LIB
	aic_srv_init();
#endif

#ifdef CONFIG_UI_MANAGER
	system_app_ui_init();
#endif

#ifdef CONFIG_CHARGER_APP
	if (charger_mode_check()) {

	}
#endif
#ifdef CONFIG_AUDIO
	system_audio_policy_init();
#endif

	system_tts_policy_init();

	system_event_map_init();

#ifdef CONFIG_PLAYTTS
	if (!ota_is_already_done()) {
#ifndef CONFIG_BT_FCC_TEST
		tts_manager_play("welcome.act", PLAY_IMMEDIATELY);
#endif
	}
#endif

	system_app_launch_init();

#ifdef CONFIG_ALARM_APP
	if (alarm_wakeup_source_check()) {
		play_welcome = false;
	}
#endif

	if ((reboot_type == REBOOT_TYPE_SF_RESET) && (reason == REBOOT_REASON_GOTO_BQB_ATT)) {
		play_welcome = false;
		att_enter_bqb_flag = true;
	}

	if (!play_welcome) {
	#ifdef CONFIG_PLAYTTS
		tts_manager_lock();
	#endif
	} else if(!(reboot_type == REBOOT_TYPE_SF_RESET && reason == REBOOT_REASON_OTA_FINISHED)) {
		bool enter_stub_tool = false;

		if (enter_stub_tool == false) {
		#ifdef CONFIG_CARD_READER_APP
			if (usb_hotplug_device_mode()
				&& !(reason == REBOOT_REASON_NORMAL)) {
#if defined(CONFIG_OTA_PRODUCT_SUPPORT) || defined(CONFIG_OTA_BLE_MASTER_SUPPORT)
				if ((reason == REBOOT_REASON_HCI_TIMEOUT) || (reason == REBOOT_REASON_SYSTEM_EXCEPTION)) {
					SYS_LOG_INF("ota product auto reboot.");
					os_sleep(100);
				} else
#endif
				if (card_reader_mode_check() == NO_NEED_INIT_BT) {
					init_bt_manager = false;
				}
			}
		#endif
		}
	}
//	move glyphix init after cardreader mode avoid file access error
	system_ready();

#ifdef CONFIG_GLYPHIX
        glyphix_ats_init();
        lvgx_set_applet_view(GLYPHIX_APPLET_VIEW);
        lvgx_view_system_init();
#endif

#ifdef CONFIG_OTA_BACKEND_SDCARD
	ota_app_init_sdcard();
#endif

#ifndef CONFIG_DEEP_SUSPEND
#ifndef CONFIG_SIMULATOR
	sys_wake_lock(PARTIAL_WAKE_LOCK);
#endif
#endif

#ifdef CONFIG_ACTIONS_TRACE
#ifdef CONFIG_TOOL
	if (tool_get_dev_type() == TOOL_DEV_TYPE_UART0) {
		/* stub uart and trace both use uart0, forbidden trace dma mode */
		trace_dma_print_set(false);
	}
#endif
	trace_init();
#endif

#ifdef CONFIG_BT_CONTROLER_RF_FCC
	if ((reboot_type == REBOOT_TYPE_SF_RESET) && (reason == REBOOT_REASON_GOTO_BQB)) {
		SYS_LOG_INF("Goto BQB TEST");
	} else {
		system_enter_FCC_mode();
	}
#endif

#ifdef CONFIG_BT_ACTIONS_SUPER_SERVICE
	if ((reason == REBOOT_REASON_SUPER_BR_BQB) || (reason == REBOOT_REASON_SUPER_LE_BQB)) {
		init_bt_manager = false;
		if(reason == REBOOT_REASON_SUPER_LE_BQB)
			super_enter_bqb_mode = LE_TEST;
		else
			super_enter_bqb_mode = DUT_TEST;
	}
#endif

#ifdef CONFIG_BT_MANAGER

	if(super_enter_bqb_mode){
#ifdef CONFIG_BT_ACTIONS_SUPER_SERVICE
		bt_manager_ble_super_enter_bqb(super_enter_bqb_mode);
#endif
	}
	else{
		if (init_bt_manager) {
#ifdef CONFIG_BT_FCC_TEST
			os_sleep(1000);
			//SYS_LOG_INF("dsp force to stop.");
			//void media_player_force_stop(void);
			//media_player_force_stop();
#endif
			bt_manager_init();
		}
	}
#else
	system_app_launch(SYS_INIT_NORMAL_MODE);
#endif

#ifdef CONFIG_SIMULATOR
#ifdef CONFIG_UI_MANAGER
	alipay_ui_init();
	wxpay_ui_init();
#endif
#endif

	while (1) {
		main_msg_proc(NULL, NULL, NULL);
	}
}

extern char z_main_stack[CONFIG_MAIN_STACK_SIZE];

APP_DEFINE(main, z_main_stack, CONFIG_MAIN_STACK_SIZE,\
			APP_PRIORITY, RESIDENT_APP,\
			NULL, NULL, NULL,\
			NULL, NULL);

