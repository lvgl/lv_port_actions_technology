/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief alarm app event
 */

#include "alarm.h"
#include "tts_manager.h"

uint8_t alarm_snooze_times = 0;

static void _alarm_ring_force_play_next(struct alarm_app_t *alarm, bool force_switch)
{
	struct app_msg msg = {
		.type = MSG_ALARM_EVENT,
		.cmd = MSG_ALARM_RING_PLAY_CUR,
	};

	SYS_LOG_INF("\n");
	if (force_switch)
		msg.cmd = MSG_ALARM_RING_PLAY_NEXT;

	os_sleep(20);
	send_async_msg(app_manager_get_current_app(), &msg);
}

static void _alarm_ring_play_event_callback(uint32_t event, void *data, uint32_t len, void *user_data)
{
	struct alarm_app_t *alarm = alarm_get_app();

	if (!alarm)
		return;

	SYS_LOG_INF("event %d\n", event);

	switch (event) {
	case PLAYBACK_EVENT_STOP_ERROR:
		if (alarm->mplayer_state != ALARM_MPLAYER_STATE_NORMAL)
			alarm->mplayer_state = ALARM_MPLAYER_STATE_ERROR;
		alarm->mode = ALARM_STATUS_RING_ERROR;
		_alarm_ring_force_play_next(alarm, true);
		break;
	case PLAYBACK_EVENT_STOP_COMPLETE:
	//case PARSER_EVENT_STOP_COMPLETE:
		alarm->mplayer_state = ALARM_MPLAYER_STATE_NORMAL;
		_alarm_ring_force_play_next(alarm, false);
		break;
	default:

		break;
	}
}
static void _alarm_clear_breakpoint(struct alarm_app_t *alarm)
{
	alarm->bp_info.file_offset = 0;
	alarm->bp_info.time_offset = 0;
}

void alarm_ring_stop(struct alarm_app_t *alarm, bool need_updata_bp)
{
	if (!alarm || !alarm->lcplayer)
		return;
	if (need_updata_bp) {
#ifdef CONFIG_LOCAL_PLAYER
		mplayer_update_breakpoint(alarm->lcplayer, &alarm->bp_info);
#endif
	} else {
		_alarm_clear_breakpoint(alarm);
	}
#ifdef CONFIG_LOCAL_PLAYER
	mplayer_stop_play(alarm->lcplayer);
#endif
	alarm->lcplayer = NULL;
	alarm->mode = ALARM_STATUS_RING_STOP;
}

static void _alarm_ring_start_play(struct alarm_app_t *alarm, const char *url)
{
	struct lcplay_param play_param;

	memset(&play_param, 0, sizeof(struct lcplay_param));
	play_param.url = url;
	play_param.play_event_callback = _alarm_ring_play_event_callback;
	play_param.bp.time_offset = alarm->bp_info.time_offset;
	play_param.bp.file_offset = alarm->bp_info.file_offset;
#ifdef CONFIG_LOCAL_PLAYER
	alarm->lcplayer = mplayer_start_play(&play_param);
#endif
	if (!alarm->lcplayer) {
		alarm->mode = ALARM_STATUS_RING_ERROR;
		if (alarm->mplayer_state != ALARM_MPLAYER_STATE_NORMAL)
			alarm->mplayer_state = ALARM_MPLAYER_STATE_ERROR;
		_alarm_ring_force_play_next(alarm, true);
	} else {
		alarm->mode = ALARM_STATUS_RING_PLAYING;
	}
}
static void _alarm_switch_mode(struct alarm_app_t *alarm)
{
#ifdef CONFIG_THREAD_TIMER
	if (thread_timer_is_running(&alarm->monitor_timer)) {
		thread_timer_stop(&alarm->monitor_timer);
		alarm->reflash_counter = 0;
	}
#endif
	alarm->set_ok = 0;
	if (!alarm->mode || alarm->mode >= ALARM_STATUS_CALENDAR_SET_YEAR) {
		alarm->mode = ALARM_STATUS_CLOCK_DISPLAY;
#ifdef CONFIG_UI_MANAGER
		alarm_clock_display(alarm, true);
#endif
	} else if (alarm->mode == ALARM_STATUS_CLOCK_DISPLAY) {
		alarm->mode = ALARM_STATUS_ALARM_SET_ONOFF;
		bool is_on = false;

		alarm_manager_get_alarm(&alarm->tm, &is_on);
		alarm->alarm_is_on = is_on;
#ifdef CONFIG_UI_MANAGER
		alarm_onoff_display(alarm, true);
#endif
		SYS_LOG_INF(" alarm set mode\n");
	} else if (alarm->mode < ALARM_STATUS_CLOCK_SET_HOUR) {
		alarm->mode = ALARM_STATUS_CLOCK_SET_HOUR;
#ifdef CONFIG_UI_MANAGER
		alarm_clock_display(alarm, true);
#endif
		SYS_LOG_INF(" clock set mode\n");
	} else if (alarm->mode < ALARM_STATUS_CALENDAR_SET_YEAR) {
		alarm->mode = ALARM_STATUS_CALENDAR_SET_YEAR;
#ifdef CONFIG_UI_MANAGER
		alarm_calendar_year_display(alarm, true);
#endif
		SYS_LOG_INF(" calendar set mode\n");
	} else {
		;
	}
#ifdef CONFIG_THREAD_TIMER
	thread_timer_start(&alarm->monitor_timer, REFLASH_CLOCK_PERIOD, REFLASH_CLOCK_PERIOD);
#endif
}
static void _alarm_clock_value_adjust(struct alarm_app_t *alarm, bool add)
{
#ifdef CONFIG_THREAD_TIMER
	if (thread_timer_is_running(&alarm->monitor_timer)) {
		thread_timer_stop(&alarm->monitor_timer);
		alarm->reflash_counter = 0;
	}
#endif
	if (alarm->set_ok) {
	#ifdef CONFIG_THREAD_TIMER
		thread_timer_start(&alarm->monitor_timer, REFLASH_CLOCK_PERIOD, REFLASH_CLOCK_PERIOD);
	#endif
		return;
	}

	if (alarm->mode == ALARM_STATUS_ALARM_SET_ONOFF) {
		alarm_onoff_adjust(alarm);
	} else if (alarm->mode == ALARM_STATUS_ALARM_SET_HOUR) {
		alarm_time_hour_adjust(alarm, add);
	} else if (alarm->mode == ALARM_STATUS_ALARM_SET_MIN) {
		alarm_time_min_adjust(alarm, add);
	} else if (alarm->mode == ALARM_STATUS_CLOCK_SET_HOUR) {
		alarm_time_hour_adjust(alarm, add);
	} else if (alarm->mode == ALARM_STATUS_CLOCK_SET_MIN) {
		alarm_time_min_adjust(alarm, add);
	} else if (alarm->mode == ALARM_STATUS_CALENDAR_SET_YEAR) {
		alarm_calendar_year_adjust(alarm, add);
	} else if (alarm->mode == ALARM_STATUS_CALENDAR_SET_MONTH) {
		alarm_calendar_month_adjust(alarm, add);
	} else if (alarm->mode == ALARM_STATUS_CALENDAR_SET_DAY) {
		alarm_calendar_day_adjust(alarm, add);
	} else {
		;
	}
#ifdef CONFIG_THREAD_TIMER
	thread_timer_start(&alarm->monitor_timer, REFLASH_CLOCK_PERIOD, REFLASH_CLOCK_PERIOD);
#endif
}

static void _alarm_confirm_setting(struct alarm_app_t *alarm)
{
#ifdef CONFIG_THREAD_TIMER
	if (thread_timer_is_running(&alarm->monitor_timer)) {
		thread_timer_stop(&alarm->monitor_timer);
		alarm->reflash_counter = 0;
	}
#endif
	if (alarm->mode == ALARM_STATUS_ALARM_SET_ONOFF) {
		if (alarm->alarm_is_on) {
			alarm->mode = ALARM_STATUS_ALARM_SET_HOUR;
#ifdef CONFIG_UI_MANAGER
			alarm_clock_display(alarm, false);
#endif
		} else {
			alarm_delete_alarm(alarm);
#ifdef CONFIG_UI_MANAGER
			alarm_clock_display(alarm, true);
#endif
			alarm->set_ok = 1;
		}
	} else if (alarm->mode == ALARM_STATUS_ALARM_SET_HOUR) {
		alarm->mode = ALARM_STATUS_ALARM_SET_MIN;
#ifdef CONFIG_UI_MANAGER
		alarm_clock_display(alarm, false);
#endif
	} else if (alarm->mode == ALARM_STATUS_ALARM_SET_MIN) {
		alarm_set_alarm(alarm);
		alarm->set_ok = 1;
#ifdef CONFIG_UI_MANAGER
		alarm_clock_display(alarm, true);
#endif
	} else if (alarm->mode == ALARM_STATUS_CLOCK_SET_HOUR) {
		alarm->mode = ALARM_STATUS_CLOCK_SET_MIN;
#ifdef CONFIG_UI_MANAGER
		alarm_clock_display(alarm, false);
#endif
	} else if (alarm->mode == ALARM_STATUS_CLOCK_SET_MIN) {
		alarm_set_clock(alarm);
		alarm->set_ok = 1;
#ifdef CONFIG_UI_MANAGER
		alarm_clock_display(alarm, true);
#endif
	} else if (alarm->mode == ALARM_STATUS_CALENDAR_SET_YEAR) {
		alarm->mode = ALARM_STATUS_CALENDAR_SET_MONTH;
#ifdef CONFIG_UI_MANAGER
		alarm_calendar_mon_day_display(alarm, true);
#endif
	} else if (alarm->mode == ALARM_STATUS_CALENDAR_SET_MONTH) {
		alarm->mode = ALARM_STATUS_CALENDAR_SET_DAY;
#ifdef CONFIG_UI_MANAGER
		alarm_calendar_mon_day_display(alarm, true);
#endif
	} else if (alarm->mode == ALARM_STATUS_CALENDAR_SET_DAY) {
		alarm_set_calendar(alarm);
		alarm->set_ok = 1;
#ifdef CONFIG_UI_MANAGER
		alarm_clock_display(alarm, true);
#endif
	} else {
		;
	}
#ifdef CONFIG_THREAD_TIMER
	thread_timer_start(&alarm->monitor_timer, REFLASH_CLOCK_PERIOD, REFLASH_CLOCK_PERIOD);
#endif
}

void alarm_switch_to_last(void)
{
#ifdef CONFIG_LAUNCHER_APP
	alarm_exit(false);

	launcher_restore_last_player();
#else
	struct app_msg msg = {0};

	app_switch_unlock(1);
	msg.type = MSG_START_APP;
	msg.ptr = NULL;
	msg.reserve = APP_SWITCH_LAST;
	send_async_msg(APP_ID_MAIN, &msg);
#endif
}
#ifdef CONFIG_LOCAL_PLAYER
static int _alarm_switch_disk(struct alarm_app_t *alarm)
{
	struct app_msg msg = {
		.type = MSG_ALARM_EVENT,
		.cmd = MSG_ALARM_RING_PLAY_INIT,
	};

	if (memcmp(alarm->dir, "/USB:/", strlen("/USB:/")) == 0) {
		if (fs_manager_get_volume_state("/SD:")) {
			strncpy(alarm->dir, "/SD:/", sizeof(alarm->dir));
		} else if (fs_manager_get_volume_state("/NAND:")) {
			strncpy(alarm->dir, "/NAND:/", sizeof(alarm->dir));
		} else {
			alarm_switch_to_last();
			return -1;
		}
	} else if (memcmp(alarm->dir, "/SD:/", strlen("/SD:/")) == 0) {
	#ifdef CONFIG_DISK_MUSIC_APP
		strncpy(alarm->dir, "/NOR:", sizeof(alarm->dir));
	#else
		alarm_switch_to_last();
		return -1;
	#endif
	} else {
		alarm_switch_to_last();
		return -1;
	}
	send_async_msg(APP_ID_ALARM, &msg);
	return 0;
}
#endif

void alarm_tts_event_proc(struct app_msg *msg)
{
	struct alarm_app_t *alarm = alarm_get_app();

	if (!alarm)
		return;
	SYS_LOG_INF("msg->value=%d\n", msg->value);
	switch (msg->value) {
	case TTS_EVENT_START_PLAY:
		if (alarm->mode == ALARM_STATUS_RING_PLAYING)
			alarm->need_resume_play = 1;
		alarm_ring_stop(alarm, true);
		break;
	case TTS_EVENT_STOP_PLAY:
		if (alarm->need_resume_play) {
			alarm->need_resume_play = 0;
			_alarm_ring_start_play(alarm, alarm->url);
		} else {
			alarm->need_resume_play = 0;
		}
		break;
	default:
		break;
	}
}

int alarm_input_event_proc(struct app_msg *msg)
{
	struct alarm_app_t *alarm = alarm_get_app();

	if (!alarm)
		return -1;

	switch (msg->cmd) {
	case MSG_ALARM_CHANGE_MODE:
		_alarm_switch_mode(alarm);
		break;
	case MSG_ALARM_VALUE_ADD:
		_alarm_clock_value_adjust(alarm, true);
		break;
	case MSG_ALARM_VALUE_SUB:
		_alarm_clock_value_adjust(alarm, false);
		break;
	case MSG_ALARM_SETTING_CONFIRM:
		_alarm_confirm_setting(alarm);
		break;

	case MSG_ALARM_RING_STOP:
		alarm_snooze_times = 0;
		alarm_ring_stop(alarm, false);
		alarm_switch_to_last();
		break;
	case MSG_ALARM_RING_SNOOZE:
		alarm_ring_stop(alarm, false);
		if (alarm_snooze_times < ALARM_SNOOSE_TIMS) {
			alarm_snooze_times++;
			alarm_set_snooze(alarm, ALARM_SNOOSE_TIMEOUT);
		}
		alarm_switch_to_last();
		break;
	case MSG_ALARM_KEY_SNOOZE:
		alarm_snooze(true);
		break;
	default:
		return -1;
	}
	return 0;
}

bool alarm_key_event_proc(uint32_t event)
{
	struct app_msg msg = {
		.type = MSG_INPUT_EVENT,
		.cmd = MSG_ALARM_KEY_SNOOZE,
		.reserve = ALARM_PLAYER,
	};

	return send_async_msg(APP_ID_LAUNCHER, &msg);
}

void alarm_event_proc(struct app_msg *msg)
{
	struct alarm_app_t *alarm = alarm_get_app();

	if (!alarm && msg->cmd != MSG_ALARM_PROC_START)
		return;

	SYS_LOG_INF("alarm cmd %d", msg->cmd);

	switch (msg->cmd) {
#ifdef CONFIG_LAUNCHER_APP
	case MSG_ALARM_PROC_START:
		alarm_proc_start();
		break;
#endif
	case MSG_ALARM_PROC_EXIT:
		alarm_switch_to_last();
		break;
	case MSG_ALARM_RING_PLAY_INIT:
	#ifdef CONFIG_THREAD_TIMER
		if (thread_timer_is_running(&alarm->monitor_timer)) {
			thread_timer_stop(&alarm->monitor_timer);
			alarm->reflash_counter = 0;
		}

		if (thread_timer_is_running(&alarm->play_timer)) {
			thread_timer_stop(&alarm->play_timer);
		}
	#endif
	#ifdef CONFIG_LOCAL_PLAYER
		if (alarm_init_iterator(alarm)) {
		/*need switch disk or exit app*/
			SYS_LOG_ERR("init iterator failed\n");
			if (_alarm_switch_disk(alarm)) {
				alarm_snooze_times = 0;
				SYS_LOG_ERR("no disk\n");
			}
			break;
		}

		if (alarm_ring_play_next_url(alarm)) {
		#ifdef CONFIG_UI_MANAGER
			alarm_ringing_display(alarm);
		#endif
			_alarm_ring_start_play(alarm, alarm->url);
		#ifdef CONFIG_THREAD_TIMER
			thread_timer_start(&alarm->play_timer, ALARM_PLAY_TIMEOUT, 0);
		#endif
		} else {
		/*need switch disk or exit app*/
			if (_alarm_switch_disk(alarm)) {
				alarm_snooze_times = 0;
				SYS_LOG_ERR("no disk\n");
			}
		}
	#endif
		break;
	case MSG_ALARM_RING_PLAY_CUR:
		alarm_ring_stop(alarm, false);
		_alarm_ring_start_play(alarm, alarm->url);
		break;
	case MSG_ALARM_RING_PLAY_NEXT:
	#ifdef CONFIG_LOCAL_PLAYER
		alarm_ring_stop(alarm, false);
		if (alarm_ring_play_next_url(alarm)) {
			_alarm_ring_start_play(alarm, alarm->url);
		} else {
		/*alarm music play error,exit app*/
			alarm_switch_to_last();
			SYS_LOG_WRN("play error,exit app\n");
		}
	#endif
		break;
	default:
		break;
	}
}

void alarm_view_stop_event(void)
{
	struct app_msg msg = {
		.type = MSG_INPUT_EVENT,
		.cmd = MSG_ALARM_RING_STOP,
		.reserve = ALARM_PLAYER,
	};
	SYS_LOG_INF("ring stop\n");

	send_async_msg(app_manager_get_current_app(), &msg);
}

void alarm_view_snooze_event(void)
{
	struct app_msg msg = {
		.type = MSG_INPUT_EVENT,
		.cmd = MSG_ALARM_RING_SNOOZE,
		.reserve = ALARM_PLAYER,
	};
	SYS_LOG_INF("ring snooze\n");

	send_async_msg(app_manager_get_current_app(), &msg);
}

