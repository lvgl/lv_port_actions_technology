/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief alarm app.h.
 */
#ifndef _ALARM_H
#define _ALARM_H


#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "alarm"
#ifdef SYS_LOG_LEVEL
#undef SYS_LOG_LEVEL
#endif
#include <mem_manager.h>
#include <app_manager.h>
#include <srv_manager.h>
#include <msg_manager.h>
#include <thread_timer.h>
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <thread_timer.h>
#include "app_defines.h"
#include "app_ui.h"
#include "app_switch.h"
#include <alarm_manager.h>
#ifdef CONFIG_UI_MANAGER
#include <seg_led_manager.h>
#endif
#include "../local_player/local_player.h"
#include <iterator/file_iterator.h>

#ifdef CONFIG_FS_MANAGER
#include <fs_manager.h>
#endif
#ifdef CONFIG_LAUNCHER_APP
#include "../launcher/launcher_app.h"
#endif

#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif

#define REFLASH_CLOCK_PERIOD	(500)/* 0.5s */
#define ALARM_PLAY_TIMEOUT (5*60*1000)/* 5min */
#define ALARM_SNOOSE_TIMS	(3)
#define ALARM_SNOOSE_TIMEOUT	(10)/* 10min */

enum alarm_player_state {
	ALARM_MPLAYER_STATE_INIT = 0,
	ALARM_MPLAYER_STATE_NORMAL,
	ALARM_MPLAYER_STATE_ERROR,
};
#ifndef CONFIG_LAUNCHER_APP
enum alarm_msg_even_type {
	MSG_ALARM_EVENT = MSG_APP_MESSAGE_START,
};
#endif
enum alarm_msg_even_cmd {
	MSG_ALARM_CHANGE_MODE = MSG_APP_MESSAGE_MAX_INDEX, //213
	MSG_ALARM_SETTING_CONFIRM,
	MSG_ALARM_VALUE_ADD,
	MSG_ALARM_VALUE_SUB,
	MSG_ALARM_PROC_START,
	MSG_ALARM_PROC_EXIT,
	MSG_ALARM_RING_PLAY_INIT,
	MSG_ALARM_RING_PLAY_CUR,
	MSG_ALARM_RING_PLAY_NEXT, //221
	MSG_ALARM_RING_STOP,
	MSG_ALARM_RING_SNOOZE,

	MSG_ALARM_KEY_SNOOZE, /* key triggered */
};

enum alarm_status {
	ALARM_STATUS_NULL		= 0x0000,
	ALARM_STATUS_CLOCK_DISPLAY	= 0x0001,
	ALARM_STATUS_ALARM_SET_ONOFF	= 0x0002,
	ALARM_STATUS_ALARM_SET_HOUR	= 0x0004,
	ALARM_STATUS_ALARM_SET_MIN	= 0x0008,
	ALARM_STATUS_CLOCK_SET_HOUR	= 0x0010,
	ALARM_STATUS_CLOCK_SET_MIN	= 0x0020,
	ALARM_STATUS_CALENDAR_SET_YEAR	= 0x0040,
	ALARM_STATUS_CALENDAR_SET_MONTH	= 0x0080,
	ALARM_STATUS_CALENDAR_SET_DAY	= 0x0100,
	ALARM_STATUS_RING_PLAYING	= 0x0200,
	ALARM_STATUS_RING_ERROR	= 0x0400,
	ALARM_STATUS_RING_STOP  = 0x0800,
};

#define ALARM_CHANGE_MODE_STATUS (ALARM_STATUS_CLOCK_DISPLAY\
	| ALARM_STATUS_ALARM_SET_ONOFF\
	| ALARM_STATUS_ALARM_SET_HOUR\
	| ALARM_STATUS_ALARM_SET_MIN \
	| ALARM_STATUS_CLOCK_SET_HOUR\
	| ALARM_STATUS_CLOCK_SET_MIN\
	| ALARM_STATUS_CALENDAR_SET_YEAR\
	| ALARM_STATUS_CALENDAR_SET_MONTH\
	| ALARM_STATUS_CALENDAR_SET_DAY)

#define ALARM_SPECIFY_DIR_NAME 15

struct alarm_app_t {
	struct thread_timer monitor_timer;
	struct thread_timer play_timer;
	struct rtc_time tm;
	uint32_t mode;
	uint32_t reflash_counter : 8;
	uint32_t set_ok : 1;
	uint32_t alarm_is_on : 1;
	uint32_t is_disk_check : 1;
	uint32_t need_resume_play : 1;
	uint32_t check_disk_times : 4;
	uint8_t mplayer_state;
	char dir[ALARM_SPECIFY_DIR_NAME + 1];
	char url[MAX_URL_LEN + 1];
	media_breakpoint_info_t bp_info;
	struct local_player_t *lcplayer;
};


struct alarm_app_t *alarm_get_app(void);
int alarm_get_status(void);
void alarm_tts_event_proc(struct app_msg *msg);
int alarm_input_event_proc(struct app_msg *msg);
void alarm_event_proc(struct app_msg *msg);
bool alarm_key_event_proc(uint32_t event);

void alarm_set_alarm(struct alarm_app_t *alarm);
void alarm_delete_alarm(struct alarm_app_t *alarm);
void alarm_set_snooze(struct alarm_app_t *alarm, uint32_t snooze_min);
void alarm_set_clock(struct alarm_app_t *alarm);
void alarm_set_calendar(struct alarm_app_t *alarm);

void alarm_time_hour_adjust(struct alarm_app_t *alarm, uint8_t add);
void alarm_time_min_adjust(struct alarm_app_t *alarm, uint8_t add);
void alarm_calendar_year_adjust(struct alarm_app_t *alarm, uint8_t add);
void alarm_calendar_month_adjust(struct alarm_app_t *alarm, uint8_t add);
void alarm_calendar_day_adjust(struct alarm_app_t *alarm, uint8_t add);
void alarm_onoff_adjust(struct alarm_app_t *alarm);
void alarm_thread_timer_init(void);
void alarm_ring_stop(struct alarm_app_t *alarm, bool need_updata_bp);
int alarm_init_iterator(struct alarm_app_t *alarm);
void alarm_exit_iterator(void);
const char *alarm_ring_play_next_url(struct alarm_app_t *alarm);

void alarm_view_init(void);
void alarm_view_deinit(void);

void alarm_calendar_year_display(struct alarm_app_t *alarm, bool is_light);
void alarm_calendar_mon_day_display(struct alarm_app_t *alarm, bool is_light);
void alarm_calendar_mon_flash(struct alarm_app_t *alarm, bool is_light);
void alarm_calendar_day_flash(struct alarm_app_t *alarm, bool is_light);

void alarm_clock_display(struct alarm_app_t *alarm, bool need_update);
void alarm_clock_set_display(struct alarm_app_t *alarm);
void alarm_clock_hour_flash(struct alarm_app_t *alarm, bool is_light);
void alarm_clock_min_flash(struct alarm_app_t *alarm, bool is_light);

void alarm_onoff_display(struct alarm_app_t *alarm, bool is_light);
void alarm_ringing_display(struct alarm_app_t *alarm);
void alarm_ringing_clock_flash(struct alarm_app_t *alarm, bool is_light);
void alarm_disk_check_timer_start(struct alarm_app_t *alarm);

void alarm_view_stop_event(void);
void alarm_view_snooze_event(void);
void alarm_switch_to_last(void);
void alarm_exit(bool need_switch_last);
void alarm_snooze(bool need_switch_last);
void alarm_start_msg_send(void);
void alarm_exit_msg_send(void);
void alarm_proc_start(void);
#endif  /* _ALARM_H */

