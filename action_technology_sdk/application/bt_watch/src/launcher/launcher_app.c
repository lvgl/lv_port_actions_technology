/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file launcher_app.c
 */
#include <assert.h>
#include <app_manager.h>
#include <srv_manager.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <power_manager.h>
#include <thread_timer.h>
#include "app_defines.h"
#include "system_app.h"
#include "launcher_app.h"
#include "app_ui.h"

#ifdef CONFIG_UI_MANAGER
#include <msgbox_cache.h>
#include "applist/applist_ui.h"
#include "health/health_ui.h"
#include "alipay/alipay_ui.h"
#include "gps/gps_ui.h"
#include "main_view.h"
#include "clock_view.h"
#include "heart_view.h"
#include "clock_selector/clock_selector.h"
#endif
#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
#include "dvfs.h"
#endif
#ifndef CONFIG_SIMULATOR
#include <soc.h>
#include <drivers/power_supply.h>
#else
#include <native_window.h>
#endif

#ifdef CONFIG_BT_MANAGER
#include <bt_manager.h>
#endif

#ifdef CONFIG_ALARM_MANAGER
#include <alarm_manager.h>
#endif
#ifdef CONFIG_ALARM_APP
#include "../alarm/alarm.h"
#endif
#ifdef CONFIG_SYS_WAKELOCK
#include <sys_wakelock.h>
#endif
#ifdef CONFIG_BT_PLAYER
#include <bt_player/bt_player.h>
#endif
#ifdef CONFIG_LCMUSIC_APP
#include <lcmusic/lcmusic.h>
#endif

#ifdef CONFIG_VIDEO_PLAYER
#include "video_player.h"
#endif
#include "ui_coder/ui_coder.h"

#ifdef CONFIG_LCMUSIC_APP
extern void lcmusic_init(const char *dir);
extern void lcmusic_exit(void);
extern void lcmusic_event_proc(struct app_msg *msg);
extern void lcmusic_input_event_proc(struct app_msg *msg);
extern void lcmusic_restore_music_player_mode(launcher_app_t *app);
#endif /* CONFIG_LCMUSIC_APP */

#ifdef CONFIG_BT_PLAYER
extern void bt_player_bt_event_proc(struct app_msg *msg);
extern void btmusic_view_input_event_proc(struct app_msg *msg);
#endif /* CONFIG_BT_PLAYER */
#ifdef CONFIG_BT_CALL_APP
extern void btcall_input_event_proc(struct app_msg *msg);
extern void btcall_bt_event_proc(struct app_msg *msg);
extern void btcall_tts_event_proc(struct app_msg *msg);
extern bool btcall_key_event_proc(uint32_t event);
extern void btcall_view_resume(void);
#endif/* CONFIG_BT_CALL_APP */
#ifdef CONFIG_ALARM_APP
extern void alarm_event_cb(void);
#endif/* CONFIG_ALARM_APP */
extern void system_create_low_power_view(void);
extern void system_delete_low_power_view(void);
extern void lcmusic_restore_music_player_mode(launcher_app_t *app);

#ifdef CONFIG_BT_TRANSMIT
extern void bt_transmit_bt_event_proc(struct app_msg *msg);
#endif /* CONFIG_BT_TRANSMIT */

static void _launcher_key_event_handle(uint16_t view_id, uint32_t event);

#ifdef CONFIG_UI_MANAGER

extern int aod_clock_view_enter(void);
extern int aod_clock_view_exit(void);

extern const main_view_presenter_t main_view_presenter;
extern const heart_view_presenter_t heart_view_presenter;
extern const clock_view_presenter_t clock_view_presenter;

/*******************************************************************************
 * View cache definition
 ******************************************************************************/

static const uint16_t app_view_id_list[] = {
	SPORT_VIEW, CLOCK_VIEW, HEART_VIEW,
	MUSIC_VIEW, ALIPAY_MAIN_VIEW, WXPAY_MAIN_VIEW,
};

static const void * app_view_presenter_list[] = {
	NULL,
	&clock_view_presenter,
	&heart_view_presenter,
	NULL,
	&alipay_view_presenter,
	&wxpay_view_presenter,
};

static void _view_cache_focus_proc(uint16_t view_id, bool focus);
static void _view_cache_event_proc(uint8_t event);
void video_clock_view_cache_event_callback(uint8_t event);

static const view_cache_dsc_t app_view_cache_dsc = {
	.type = LANDSCAPE,
	.serial_load = 1,
	.rotate = 1,
	.num = ARRAY_SIZE(app_view_id_list),
	.vlist = app_view_id_list,
	.plist = app_view_presenter_list,
	.cross_attached_view = VIEW_INVALID_ID,
	.cross_vlist = { MAIN_VIEW, MSG_VIEW },
	.cross_plist = { &main_view_presenter, NULL },
	.event_cb = _view_cache_event_proc,
	.focus_cb = _view_cache_focus_proc,
};

const view_cache_dsc_t *get_app_view_cache_dsc(void)
{
	return &app_view_cache_dsc;
}
#endif

launcher_app_t g_launcher_app;

#ifdef CONFIG_RTC_ACTS
static void _launcher_app_update_clock_view(os_work *work);

static OS_WORK_DEFINE(clock_update_work, _launcher_app_update_clock_view);

static void _launcher_app_update_clock_view(os_work *work)
{
#ifdef CONFIG_UI_MANAGER
	launcher_app_t *app = launcher_app_get();
	ui_view_paint(app->in_aod_view ? AOD_CLOCK_VIEW : CLOCK_VIEW);
#endif
}

static void _rtc_alarm_period_handler(const void *cb_data)
{
	os_work_submit(&clock_update_work);
}

void launcher_set_rtc_period(bool en, uint16_t period_ms)
{
	launcher_app_t *app = launcher_app_get();

	if (en) {
		app->rtc_config.tm_msec = period_ms % 1000;
		app->rtc_config.tm_sec = period_ms / 1000;
	}

	rtc_set_period_alarm(app->rtc_dev, (void *)&app->rtc_config, en);
}
#else
void launcher_set_rtc_period(bool en, uint16_t period_ms) {}
#endif /* CONFIG_RTC_ACTS*/

#ifdef CONFIG_UI_MANAGER
static void _view_cache_focus_proc(uint16_t view_id, bool focus)
{
	SYS_LOG_DBG("view %d focus %d\n", view_id, focus);
}

static void _view_cache_event_proc(uint8_t event)
{
	launcher_app_t *app = launcher_app_get();
	(void)app;
	SYS_LOG_DBG("event %d\n", event);
#ifdef CONFIG_VIDEO_APP
	if (app->clock_id == VIDEO_CLOCK_ID) {
		video_clock_view_cache_event_callback(event);
	}
#endif
}
#endif

#ifdef CONFIG_SENSOR_MANAGER
static int _sensor_res_init(void)
{
	launcher_app_t *app = launcher_app_get();
	sensor_res_t *res = &app->sensor_res;
	int ret;

	ret = sensor_manager_get_result(res);
	if (ret == 0) {
		app->heart_rate = (uint32_t)res->heart_rate;
		app->step_count = (uint32_t)res->pedometer.total_steps;
		SYS_LOG_INF("heart_rate: %d", app->heart_rate);
		SYS_LOG_INF("step_count: %d", app->step_count);
	}

	return 0;
}

static int _sensor_res_callback(int evt_id, sensor_res_t *res)
{
	launcher_app_t *app = launcher_app_get();

	if (app->heart_rate != (uint32_t)res->heart_rate) {
		app->heart_rate = (uint32_t)res->heart_rate;
		SYS_LOG_INF("heart_rate: %d", app->heart_rate);
#ifdef CONFIG_UI_MANAGER
		ui_view_paint(HEART_VIEW);
#endif
	}

	if (app->step_count != (uint32_t)res->pedometer.total_steps) {
		app->step_count = (uint32_t)res->pedometer.total_steps;
		SYS_LOG_INF("step_count: %d", app->step_count);
#ifdef CONFIG_UI_MANAGER
		ui_view_paint(CLOCK_VIEW);
#endif
	}

	if (app->bearing != (uint32_t)res->orientation) {
		app->bearing = (uint32_t)res->orientation;
		SYS_LOG_INF("bearing: %d", app->bearing);
#ifdef CONFIG_UI_MANAGER
		ui_view_paint(COMPASS_VIEW);
#endif
	}

	return 0;
}
#endif /* CONFIG_SENSOR_MANAGER */

void launcher_apply_clock_id(uint8_t clock_id)
{
	launcher_app_t *app = launcher_app_get();

	app->clock_id = clock_id;
}

static int _launcher_app_init(void)
{
	launcher_app_t *app = launcher_app_get();

	memset(app, 0, sizeof(*app));
	app->step_count = 3456;
	app->heart_rate = 120;
	app->bearing = 200;

#ifdef CONFIG_RTC_ACTS
	app->rtc_dev = device_get_binding(CONFIG_RTC_0_NAME);
	assert(app->rtc_dev != NULL);
	app->rtc_config.cb_fn = _rtc_alarm_period_handler;
	app->rtc_config.cb_data = NULL;
#endif

#ifdef CONFIG_PROPERTY
#ifdef CONFIG_UI_MANAGER
	if (clocksel_get_aod_clock_dsc() != NULL) {
		soc_set_aod_mode((uint8_t)property_get_int(CFG_AOD_MODE, 0));
	}
#endif

	app->clock_id = (uint8_t)property_get_int(CFG_CLOCK_ID, 0);

	app->scroll_effects_mode = (uint8_t)property_get_int(CFG_SCROLL_MODE, UI_SCROLL_EFFECT_CUBE);
#ifdef CONFIG_VG_LITE
	app->switch_effect_mode = (uint8_t)property_get_int(CFG_SWITCH_MODE, UI_SWITCH_EFFECT_PAGE);
#else
	app->switch_effect_mode = (uint8_t)property_get_int(CFG_SWITCH_MODE, UI_SWITCH_EFFECT_SCALE);
#endif /*CONFIG_VG_LITE*/
#endif

#ifdef CONFIG_SENSOR_MANAGER
	_sensor_res_init();
	sensor_manager_add_callback(_sensor_res_callback);
#endif

#ifdef CONFIG_UI_MANAGER
	ui_manager_set_keyevent_callback(_launcher_key_event_handle);
	ui_switch_effect_set_type(app->switch_effect_mode);
	ui_scroll_effect_set_type(app->scroll_effects_mode);
	ui_vscroll_effect_set_type(DEF_UI_VSCROLL_EFFECT);

	view_stack_push_cache(&app_view_cache_dsc, CLOCK_VIEW);

	gps_ui_init();
#endif /* CONFIG_UI_MANAGER */

#ifdef CONFIG_ALARM_MANAGER
#ifdef CONFIG_ALARM_APP
	system_registry_alarm_callback(alarm_event_cb);
#endif
	alarm_manager_init();
	find_and_set_alarm();
#endif
#ifdef CONFIG_LCMUSIC_APP
#ifdef CONFIG_UI_MANAGER
	lcmusic_restore_music_player_mode(app);
#else
	app->cur_player = BTMUSIC_PLAYER;
#endif
	lcmusic_init(CONFIG_APP_FAT_DISK"/");
#endif
	return 0;
}

static void _music_player_exit(void)
{
	launcher_app_t *app = launcher_app_get();
	struct app_msg msg = { 0 };

	if (app->cur_player == LCMUSIC_PLAYER) {
	#ifdef CONFIG_LCMUSIC_APP
		lcmusic_exit();
	#endif
	} else if (app->cur_player == BTMUSIC_PLAYER) {
	/*stop btmusic player*/
	#ifdef CONFIG_BT_PLAYER
		msg.cmd = BT_A2DP_STREAM_SUSPEND_EVENT;
		bt_player_bt_event_proc(&msg);
	#endif
	} else if (app->cur_player == ALARM_PLAYER) {
	#ifdef CONFIG_ALARM_APP
		alarm_exit(false);
	#endif
	} else if (app->cur_player == BTCALL_PLAYER) {
	#ifdef CONFIG_BT_CALL_APP
		msg.cmd = BT_HFP_CALL_STATE_EXIT;
		btcall_bt_event_proc(&msg);
	#endif
	} else if (app->cur_player == VIDEO_PLAYER) {
	#ifdef CONFIG_VIDEO_PLAYER
		video_player_pause(true, false);
	#endif
	}

	(void)msg;
}

#ifdef CONFIG_VIDEO_APP_AUDIO_ENABLE
#ifdef CONFIG_VIDEO_PLAYER
void launcher_video_control(int cmd)
{
	struct app_msg msg = {
		.type = MSG_VIDEO_EVENT,
		.cmd = cmd,
	};

	send_async_msg(app_manager_get_current_app(), &msg);
}

static void _launcher_video_player_handle(struct app_msg *msg)
{
	launcher_app_t *app = launcher_app_get();
	struct app_msg tmp_msg = { 0 };
	static bool focused = false;

	SYS_LOG_INF("video cmd %d, cur player %d, focused %d\n", msg->cmd, app->cur_player, focused);

	switch (msg->cmd) {
		case MSG_VIDEO_VIEW_FOCUS:
		case MSG_VIDEO_RESUME_APP:
			if (msg->cmd == MSG_VIDEO_VIEW_FOCUS) {
				if (focused == true) {
					break;
				}
				focused = true;
			} else if (msg->cmd == MSG_VIDEO_RESUME_APP) {
				if (focused == false)
					break;
			}

			if (app->cur_player == LCMUSIC_PLAYER) {
				//exit current player
			#ifdef CONFIG_LCMUSIC_APP
				lcmusic_exit();
				launcher_suspend_player(app->cur_player);
			#endif
				app->cur_player = VIDEO_PLAYER;
				video_player_play();
			} else if (app->cur_player == BTMUSIC_PLAYER) {
				/*stop btmusic player*/
			#ifdef CONFIG_BT_PLAYER
				tmp_msg.cmd = MSG_BT_PLAY_STOP_PLAYER;
				btmusic_view_input_event_proc(&tmp_msg);
				launcher_suspend_player(app->cur_player);
			#endif
				app->cur_player = VIDEO_PLAYER;
				video_player_play();
			} else if (app->cur_player == BTCALL_PLAYER || app->cur_player == ALARM_PLAYER){
				launcher_suspend_player(VIDEO_PLAYER);
			}
			break;
		case MSG_VIDEO_VIEW_DEFOCUS:
		case MSG_VIDEO_SUSPEND_APP:
			if (msg->cmd == MSG_VIDEO_VIEW_DEFOCUS) {
				if (focused == false) {
					break;
				}
				focused = false;
			} else if (msg->cmd == MSG_VIDEO_SUSPEND_APP) {
				if (focused == false)
					break;
			}

			if (app->cur_player == VIDEO_PLAYER) {
				video_player_pause(true, false);
				launcher_restore_last_player();
			} else if (app->cur_player == BTCALL_PLAYER || app->cur_player == ALARM_PLAYER) {
				launcher_cancel_player(VIDEO_PLAYER);
			}
			break;
		default:
			break;
	}

	(void)tmp_msg;
}
#endif
#endif

void launcher_switch_player(uint8_t player)
{
	struct app_msg msg = {
		.type = MSG_SWITCH_PLAYER,
		.cmd = player,
	};

	send_async_msg(app_manager_get_current_app(), &msg);
}

static void _launcher_app_exit(void)
{
#ifdef CONFIG_SENSOR_MANAGER
	sensor_manager_remove_callback(_sensor_res_callback);
#endif
	_music_player_exit();

#ifdef CONFIG_UI_MANAGER
	ui_switch_effect_set_type(UI_SWITCH_EFFECT_NONE);
	view_stack_clean();
#endif
	app_manager_thread_exit(APP_ID_LAUNCHER);
}

static void _launcher_app_early_suspend(void)
{
	launcher_app_t *app = launcher_app_get();

#ifdef CONFIG_VIDEO_PLAYER
#ifdef CONFIG_VIDEO_APP_AUDIO_ENABLE
	struct app_msg tmp_msg = { 0 };
	tmp_msg.cmd = MSG_VIDEO_SUSPEND_APP;
	_launcher_video_player_handle(&tmp_msg);
#endif
#endif
	app->suspended = 1;

#ifndef CONFIG_SIMULATOR
	if (soc_get_aod_mode()) {
		SYS_LOG_INF("AOD enter\n");

		if (!aod_clock_view_enter())
			app->in_aod_view = 1;
	}
#endif /* CONFIG_SIMULATOR */

	SYS_LOG_INF("launcher early-suspend\n");
}

static void _launcher_app_late_resume(void)
{
	launcher_app_t *app = launcher_app_get();

#ifndef CONFIG_SIMULATOR
	if (app->in_aod_view) {
		aod_clock_view_exit();
		app->in_aod_view = 0;

		SYS_LOG_INF("AOD exit\n");
	}
#endif /* CONFIG_SIMULATOR */

#ifdef CONFIG_UI_MANAGER
	if (view_stack_get_num() == 0) {
		view_stack_push_cache(&app_view_cache_dsc, CLOCK_VIEW);
	}

	app->suspended = 0;

#ifdef CONFIG_BT_CALL_APP
	if (app->cur_player == BTCALL_PLAYER) {
		btcall_view_resume();
	}
#endif
#endif

#ifdef CONFIG_ALARM_APP
	if (app->alarm_en) {
		alarm_start_msg_send();
		app->alarm_en = 0;
	}
#endif

#ifdef CONFIG_VIDEO_PLAYER
#ifdef CONFIG_VIDEO_APP_AUDIO_ENABLE
	struct app_msg tmp_msg = { 0 };
	tmp_msg.cmd = MSG_VIDEO_RESUME_APP;
	_launcher_video_player_handle(&tmp_msg);
#endif
#endif
	SYS_LOG_INF("launcher late-resume\n");
}

static void _launcher_app_suspend(void)
{
	launcher_app_t *app = launcher_app_get();

#ifdef CONFIG_SYS_WAKELOCK
	if (sys_wakelocks_check(PARTIAL_WAKE_LOCK)
		|| sys_wakelocks_check(FULL_WAKE_LOCK)) {
		SYS_LOG_INF("wake locked\n");
		return;
	}
#endif

	app->suspended = 2;

	SYS_LOG_INF("launcher suspend\n");
}

static void _launcher_app_resume(void)
{
	launcher_app_t *app = launcher_app_get();

	app->suspended = 1;

	SYS_LOG_INF("launcher resume\n");
}

#ifdef CONFIG_BT_MANAGER

#ifdef CONFIG_BT_CALL_APP
static void btcall_start_exit_event(struct app_msg *msg)
{
	launcher_app_t *app = launcher_app_get();

	if (msg->cmd == BT_HFP_CALL_STATE_START) {
		if (app->cur_player == BTCALL_PLAYER) {
			SYS_LOG_INF("btcall is cur player\n");
		} else if (app->cur_player == ALARM_PLAYER) {
			SYS_LOG_INF("switch player alarm -> btcall\n");
		#ifdef CONFIG_ALARM_MANAGER
			alarm_snooze(false);
		#endif
			app->cur_player = BTCALL_PLAYER;
			btcall_bt_event_proc(msg);
		} else if (app->cur_player == LCMUSIC_PLAYER || app->cur_player == BTMUSIC_PLAYER
			|| app->cur_player == VIDEO_PLAYER) {
			SYS_LOG_INF("switch player %d -> btcall\n", app->cur_player);
			_music_player_exit();
			launcher_suspend_player(app->cur_player);
			app->cur_player = BTCALL_PLAYER;
			btcall_bt_event_proc(msg);
		}
	} else if (msg->cmd == BT_HFP_CALL_STATE_EXIT) {
		btcall_bt_event_proc(msg);
	} else {
		btcall_bt_event_proc(msg);
	}
}
#endif

static void _launcher_bt_event_handle(struct app_msg *msg)
{
	launcher_app_t *app = launcher_app_get();

	switch (msg->cmd) {
#ifdef CONFIG_BT_CALL_APP
	case BT_HFP_CALL_STATE_START:
	case BT_HFP_CALL_STATE_EXIT:
		btcall_start_exit_event(msg);
		return;
#endif

#ifdef CONFIG_BT_PLAYER
	case BT_A2DP_STREAM_SUSPEND_EVENT:
	case BT_AVRCP_PLAYBACK_STATUS_CHANGED_EVENT:
	case BT_AVRCP_DISCONNECTION_EVENT:
	case BT_DISCONNECTION_EVENT:
		bt_player_bt_event_proc(msg);
		return;
#endif

#ifdef CONFIG_BT_TRANSMIT
	case BT_TRS_A2DP_STREAM_READY_EVENT:
	case BT_TRS_A2DP_STREAM_CLOSE_EVENT:
		bt_transmit_bt_event_proc(msg);
		return;
#endif

	default:
		break;
	}

#ifdef CONFIG_BT_CALL_APP
	if (app->cur_player == BTCALL_PLAYER) {
		btcall_start_exit_event(msg);
		return;
	}
#endif

#ifdef CONFIG_BT_PLAYER
	if (app->cur_player == BTMUSIC_PLAYER) {
		bt_player_bt_event_proc(msg);
		return;
	}
#endif

#ifdef CONFIG_BT_TRANSMIT
	if (app->cur_player == LCMUSIC_PLAYER) {
		bt_transmit_bt_event_proc(msg);
		return;
	}
#endif
	(void)app;
}

static void _launcher_bt_mgr_event_handle(struct app_msg *msg)
{
#ifdef CONFIG_ALARM_MANAGER
	int ret;
#endif
	switch (msg->cmd) {
#ifdef CONFIG_ALARM_MANAGER
	case BT_MAP_SET_TIME_EVENT:
		ret = alarm_manager_set_time((struct rtc_time *)msg->ptr);
		if (ret) {
			SYS_LOG_ERR("set time error ret=%d\n", ret);
		} else {
			find_and_set_alarm();
		}

#ifdef CONFIG_UI_MANAGER
		{
			launcher_app_t *app = launcher_app_get();
			ui_view_paint(MAIN_VIEW);
			ui_view_paint(app->in_aod_view ? AOD_CLOCK_VIEW : CLOCK_VIEW);
			// sync time for alipay
			alipay_ui_sync_time();
		}
#endif
		break;
#endif /* CONFIG_ALARM_MANAGER */

	default:
		SYS_LOG_ERR("error: 0x%x!\n", msg->cmd);
		break;
	}
}
#endif /* CONFIG_BT_MANAGER */

#ifdef CONFIG_ALARM_APP
static void _launcher_alarm_event_handle(struct app_msg *msg)
{
	launcher_app_t *app = launcher_app_get();

	if (app->cur_player == BTCALL_PLAYER) {
		SYS_LOG_INF("btcall going");
		alarm_snooze(false);
		return;
	}

	if (msg->cmd == MSG_ALARM_PROC_START) {
		if (app->suspended != 0) {
		#ifdef CONFIG_SYS_WAKELOCK
			sys_wake_lock(FULL_WAKE_LOCK);
			sys_wake_unlock(FULL_WAKE_LOCK);
		#endif
			app->alarm_en = 1;
		}

		if (app->cur_player != ALARM_PLAYER) {
			_music_player_exit();
			launcher_suspend_player(app->cur_player);
			app->cur_player = ALARM_PLAYER;
		}

		if (app->suspended == 0) {
			alarm_event_proc(msg);
		}
	} else if (app->cur_player == ALARM_PLAYER) {
		alarm_event_proc(msg);
	}
}
#endif /* CONFIG_ALARM_APP */

static void _launcher_key_event_handle(uint16_t view_id, uint32_t event)
{
	launcher_app_t *app = launcher_app_get();

	SYS_LOG_INF("view %u, key 0x%x", view_id, event);

	if (app->suspended) /* bypass key event until launcher late-resume */
		return;

	os_strace_u32(SYS_TRACE_ID_KEY_READ, event);

#ifdef CONFIG_BT_CALL_APP
	/* if (app->cur_player == BTCALL_PLAYER) */ {
		if (btcall_key_event_proc(event) == true)
			return;
	}
#endif

#if defined(CONFIG_OTA_PRODUCT_SUPPORT) || defined(CONFIG_OTA_BLE_MASTER_SUPPORT)
	bool btota_product_key_event_proc(uint32_t event);
	if (btota_product_key_event_proc(event) == true)
		return;
#endif

#ifdef CONFIG_ALARM_APP
		if (app->cur_player == ALARM_PLAYER) {
			if (alarm_key_event_proc(event) == true)
				return;
		}
#endif /* CONFIG_ALARM_APP */

#ifdef CONFIG_UI_MANAGER
	if (msgbox_cache_num_popup_get() > 0) {
		return;
	}

	if (event == (KEY_POWER | KEY_TYPE_LONG_DOWN)) {/* enter power view */
		if (view_stack_get_top() != POWER_VIEW) {
			view_stack_push_view(POWER_VIEW, NULL);
		}
		return;
	}

	if (event == (KEY_POWER | KEY_TYPE_SHORT_UP)) {
		if (view_stack_get_top_cache() == &app_view_cache_dsc &&
			view_cache_get_focus_view() != CLOCK_VIEW) {
			view_cache_set_focus_view(CLOCK_VIEW);
			return;
		}

		if (view_stack_get_num() == 1) {
			applist_ui_enter();
			return;
		}

		if (view_stack_get_top() == APPLIST_VIEW) {
			ui_vscroll_effect_set_type(UI_VSCROLL_EFFECT_NONE);
			health_ui_enter();
			return;
		}

		ui_vscroll_effect_set_type(DEF_UI_VSCROLL_EFFECT);
		view_stack_pop_home(); /* go back to the CLOCK_VIEW */
		return;
	}

#ifdef CONFIG_INPUT_DEV_ACTS_KNOB
	if(event == KEY_KONB_CLOCKWISE) {
		_coder_roll_execute(1);
		SYS_LOG_INF("KEY_KONB_CLOCKWISE");
	} else if(event == KEY_KONB_ANTICLOCKWISE) {
		_coder_roll_execute(0);
		SYS_LOG_INF("KEY_KONB_ANTICLOCKWISE");
	}
#endif

	if (KEY_VALUE(event) == KEY_GESTURE_RIGHT) {
		switch (view_id) {
		default:
			view_stack_pop();
			break;
		}
	}
#endif
}

static void _launcher_input_event_handle(struct app_msg *msg)
{
	uint8_t target_player = msg->reserve;

	switch(target_player) {
	#ifdef CONFIG_LCMUSIC_APP
		case LCMUSIC_PLAYER:
			lcmusic_input_event_proc(msg);
			break;
	#endif
	#ifdef CONFIG_BT_PLAYER
		case BTMUSIC_PLAYER:
			btmusic_view_input_event_proc(msg);
			break;
	#endif
	#ifdef CONFIG_BT_CALL_APP
		case BTCALL_PLAYER:
			btcall_input_event_proc(msg);
			break;
	#endif
	#ifdef CONFIG_ALARM_APP
		case ALARM_PLAYER:
			alarm_input_event_proc(msg);
			break;
	#endif
		default:
			SYS_LOG_INF("type:%d, cmd:%d, reserve:%d", msg->type, msg->cmd, msg->reserve);
			break;
	}
}
static void _launcher_tts_event_handle(struct app_msg *msg)
{
#ifdef CONFIG_BT_CALL_APP
	launcher_app_t *app = launcher_app_get();

	if (app->cur_player == BTCALL_PLAYER) {
		btcall_tts_event_proc(msg);
		return;
	}
#endif
}

void launcher_restore_last_player(void)
{
	launcher_app_t *app = launcher_app_get();
	t_player_node *player_node;

	if (app->cur_player != LCMUSIC_PLAYER && app->cur_player != BTMUSIC_PLAYER) {
		player_node = app->suspend_player_list;
		if (player_node) {
			app->cur_player = player_node->type;
			app->suspend_player_list = player_node->next;
			app_mem_free(player_node);
		} else {
			SYS_LOG_INF("restore wrong");
			app->cur_player = LCMUSIC_PLAYER;
		}

		SYS_LOG_INF("restore player %d\n", app->cur_player);

		if (app->cur_player == LCMUSIC_PLAYER) {
	#ifdef CONFIG_LCMUSIC_APP
			lcmusic_init(CONFIG_APP_FAT_DISK"/");
	#endif
		}
		if (app->cur_player == BTMUSIC_PLAYER) {
	#ifdef CONFIG_BT_PLAYER
			btmusic_check_remote_state();
	#endif
		}
		if (app->cur_player == VIDEO_PLAYER) {
	#ifdef CONFIG_VIDEO_PLAYER
			video_player_play();
	#endif
		}
	}
}

void launcher_suspend_player(uint8_t suspend_player)
{
	launcher_app_t *app = launcher_app_get();

	t_player_node *player_node;
	for (player_node = app->suspend_player_list; player_node != NULL; ) {
		if (player_node->type == suspend_player) {
			SYS_LOG_INF("player %d suspended\n", suspend_player);
			return;
		}
		if (player_node->next == NULL) {
			break;
		} else {
			player_node = player_node->next;
		}
	}

	SYS_LOG_INF("suspend player %d\n", suspend_player);
	player_node = app_mem_malloc(sizeof(t_player_node));
	player_node->type = suspend_player;
	player_node->next = app->suspend_player_list;
	app->suspend_player_list = player_node;
}

void launcher_cancel_player(uint8_t target_player)
{
	launcher_app_t *app = launcher_app_get();
	t_player_node *pre_player_node = NULL;
	t_player_node *cur_player_node = NULL;

	for (cur_player_node = app->suspend_player_list; cur_player_node != NULL; ) {
		if (cur_player_node->type == target_player) {
			SYS_LOG_INF("cancel player %d\n", target_player);
			if (pre_player_node != NULL) {
				pre_player_node->next = cur_player_node->next;
			} else {
				app->suspend_player_list = cur_player_node->next;
			}
			app_mem_free(cur_player_node);
			return;
		}

		if (cur_player_node->next == NULL) {
			return;
		} else {
			cur_player_node = cur_player_node->next;
		}
	}

}


void _launcher_app_loop(void *p1, void *p2, void *p3)
{
	launcher_app_t *app = NULL;
	struct app_msg msg = {0};
	struct app_msg tmp_msg = {0};
	bool terminated = false;
	int timeout;

	SYS_LOG_INF(APP_ID_LAUNCHER " enter");

	if (_launcher_app_init()) {
		SYS_LOG_ERR(APP_ID_LAUNCHER " init failed");
		_launcher_app_exit();
		goto out_exit;
	}

#ifdef CONFIG_ALARM_APP
	if (alarm_wakeup_source_check()) {
		alarm_start_msg_send();
	}
#endif
#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "init");
	dvfs_set_level(DVFS_LEVEL_NORMAL, "launcher");
#endif
	app = launcher_app_get();

	while (!terminated) {
		timeout = (app->suspended == 2) ?
				OS_FOREVER : thread_timer_next_timeout();

		if (receive_msg(&msg, timeout)) {
			switch (msg.type) {
			case MSG_EXIT_APP:
				_launcher_app_exit();
				terminated = true;
				break;

			case MSG_EARLY_SUSPEND_APP:
				_launcher_app_early_suspend();
				break;
			case MSG_LATE_RESUME_APP:
				_launcher_app_late_resume();
				break;
			case MSG_SUSPEND_APP:
				_launcher_app_suspend();
				break;
			case MSG_RESUME_APP:
				_launcher_app_resume();
				break;
			case MSG_KEY_INPUT:
				_launcher_key_event_handle(VIEW_INVALID_ID, msg.value);
				break;
			case MSG_INPUT_EVENT:
				_launcher_input_event_handle(&msg);
				break;

			#ifdef CONFIG_LCMUSIC_APP
			case MSG_APP_MESSAGE_START:
				if (app->cur_player != LCMUSIC_PLAYER)
					break;

				lcmusic_event_proc(&msg);
				break;
			#endif
			case MSG_TTS_EVENT:
				_launcher_tts_event_handle(&msg);
				break;

			#ifdef CONFIG_ALARM_APP
			case MSG_ALARM_EVENT:
				_launcher_alarm_event_handle(&msg);
				break;
			#endif
			#ifdef CONFIG_VIDEO_PLAYER
			#ifdef CONFIG_VIDEO_APP_AUDIO_ENABLE
			case MSG_VIDEO_EVENT:
				_launcher_video_player_handle(&msg);
				break;
			#endif
			#endif
			#ifndef CONFIG_SIMULATOR
			case MSG_SYS_EVENT:
				if (msg.cmd == SYS_EVENT_BATTERY_LOW) {
#ifdef CONFIG_UI_MANAGER
					system_create_low_power_view();
#endif
				}
				break;
			case MSG_BAT_CHARGE_EVENT:
				if (msg.cmd == BAT_CHG_EVENT_DC5V_IN) {
#ifdef CONFIG_UI_MANAGER
					system_delete_low_power_view();
#endif
				}
				break;
			#endif /* CONFIG_SIMULATOR */

			#ifdef CONFIG_BT_MANAGER
			case MSG_BT_EVENT:
				_launcher_bt_event_handle(&msg);
				break;
			case MSG_BT_MGR_EVENT:
				_launcher_bt_mgr_event_handle(&msg);
				break;
			#endif /* CONFIG_BT_MANAGER */
			case MSG_SWITCH_PLAYER:
				if (app->cur_player == msg.cmd)
					break;

				if (app->cur_player == LCMUSIC_PLAYER) {
				#ifdef CONFIG_LCMUSIC_APP
					tmp_msg.cmd = MSG_LCMPLAYER_PLAY_PAUSE;
					lcmusic_input_event_proc(&tmp_msg);
				#endif
				} else if (app->cur_player == BTMUSIC_PLAYER) {
				#ifdef CONFIG_BT_PLAYER
					tmp_msg.cmd = MSG_BT_PLAY_PAUSE;
					tmp_msg.value = true;
					btmusic_view_input_event_proc(&tmp_msg);
				#endif
				}
				app->cur_player = msg.cmd;
				SYS_LOG_INF("player switch to %d\n", app->cur_player);
				break;
			default:
				break;
			}

			if (msg.callback != NULL)
				msg.callback(&msg, 0, NULL);
		}

		thread_timer_handle_expired();
	}

out_exit:
	SYS_LOG_INF(APP_ID_LAUNCHER " exit");
}

APP_DEFINE(launcher, share_stack_area, sizeof(share_stack_area),
		CONFIG_APP_PRIORITY, DEFAULT_APP | FOREGROUND_APP, NULL, NULL, NULL,
	   _launcher_app_loop, NULL);
