/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief alarm fun
 */


#include "alarm.h"
#include <drivers/rtc.h>


#define CLOCK_DISPLAY_TIMEOUT	20000/* 20s */

int alarm_get_status(void)
{
	struct alarm_app_t *alarm = alarm_get_app();

	if (!alarm)
		return ALARM_STATUS_NULL;

	if (alarm->mode)
		return alarm->mode;
	else
		return ALARM_STATUS_CLOCK_DISPLAY;
}


void alarm_set_clock(struct alarm_app_t *alarm)
{
	alarm->tm.tm_sec = 0;
	alarm_manager_set_time(&alarm->tm);
	SYS_LOG_INF("\n");
}

void alarm_set_alarm(struct alarm_app_t *alarm)
{
	alarm->tm.tm_sec = 0;
	alarm_manager_set_alarm(&alarm->tm, false);
	SYS_LOG_INF("\n");
}
void alarm_set_snooze(struct alarm_app_t *alarm, uint32_t snooze_min)
{
	alarm_manager_get_time(&alarm->tm);
	alarm->tm.tm_min += snooze_min;
	if (alarm->tm.tm_min >= 60) {
		alarm->tm.tm_min -= 60;
		alarm->tm.tm_hour += 1;
		if (alarm->tm.tm_hour >= 24) {
			alarm->tm.tm_hour -= 24;
			alarm->tm.tm_mday += 1;
			if (alarm->tm.tm_mday > rtc_month_days(alarm->tm.tm_mon - 1, alarm->tm.tm_year - 1900)) {
				/* alarm day 1-31 */
				alarm->tm.tm_mday = 1;
				alarm->tm.tm_mon += 1;
				if (alarm->tm.tm_mon >= 13) {
					/* alarm mon 1-12 */
					alarm->tm.tm_mon = 1;
					alarm->tm.tm_year += 1;
				}
			}
		}
	}
	find_and_set_alarm();
	alarm_manager_set_alarm(&alarm->tm, true);
	SYS_LOG_INF("\n");
}
void alarm_delete_alarm(struct alarm_app_t *alarm)
{
	alarm_manager_delete_alarm(&alarm->tm);
	SYS_LOG_INF("\n");
}

void alarm_set_calendar(struct alarm_app_t *alarm)
{
	struct rtc_time tm = {0};

	alarm_manager_get_time(&tm);
	/*update hour/min/sec*/
	alarm->tm.tm_hour = tm.tm_hour;
	alarm->tm.tm_min = tm.tm_min;
	alarm->tm.tm_sec = tm.tm_sec;

	alarm_manager_set_time(&alarm->tm);
	SYS_LOG_INF("\n");
}
void alarm_onoff_adjust(struct alarm_app_t *alarm)
{
	alarm->alarm_is_on = !alarm->alarm_is_on;
#ifdef CONFIG_UI_MANAGER
	alarm_onoff_display(alarm, true);
#endif
}
void alarm_time_hour_adjust(struct alarm_app_t *alarm, uint8_t add)
{
	if (add) {
		if (alarm->tm.tm_hour >= 23) {
			alarm->tm.tm_hour = 0;
		} else {
			alarm->tm.tm_hour++;
		}
	} else {
		if (alarm->tm.tm_hour <= 0) {
			alarm->tm.tm_hour = 23;
		} else {
			alarm->tm.tm_hour--;
		}
	}
#ifdef CONFIG_UI_MANAGER
	alarm_clock_set_display(alarm);
#endif
}
void alarm_time_min_adjust(struct alarm_app_t *alarm, uint8_t add)
{
	if (add) {
		if (alarm->tm.tm_min >= 59) {
			alarm->tm.tm_min = 0;
		} else {
			alarm->tm.tm_min++;
		}
	} else {
		if (alarm->tm.tm_min <= 0) {
			alarm->tm.tm_min = 59;
		} else {
			alarm->tm.tm_min--;
		}
	}
#ifdef CONFIG_UI_MANAGER
	alarm_clock_set_display(alarm);
#endif
}

void alarm_calendar_year_adjust(struct alarm_app_t *alarm, uint8_t add)
{
	if (add) {
		if (alarm->tm.tm_year >= 2099) {
			alarm->tm.tm_year = 2000;
		} else {
			alarm->tm.tm_year++;
		}
	} else {
		if (alarm->tm.tm_year <= 2000) {
			alarm->tm.tm_year = 2099;
		} else {
			alarm->tm.tm_year--;
		}
	}
#ifdef CONFIG_UI_MANAGER
	alarm_calendar_year_display(alarm, true);
#endif
}
void alarm_calendar_month_adjust(struct alarm_app_t *alarm, uint8_t add)
{
	if (add) {
		if (alarm->tm.tm_mon >= 12) {
			alarm->tm.tm_mon = 1;
		} else {
			alarm->tm.tm_mon++;
		}
	} else {
		if (alarm->tm.tm_mon <= 1) {
			alarm->tm.tm_mon = 12;
		} else {
			alarm->tm.tm_mon--;
		}
	}
#ifdef CONFIG_UI_MANAGER
	alarm_calendar_mon_day_display(alarm, true);
#endif
}
void alarm_calendar_day_adjust(struct alarm_app_t *alarm, uint8_t add)
{
	if (add) {
		if (alarm->tm.tm_mday >= rtc_month_days(alarm->tm.tm_mon - 1, alarm->tm.tm_year - 1900)) {
			alarm->tm.tm_mday = 1;
		} else {
			alarm->tm.tm_mday++;
		}
	} else {
		if (alarm->tm.tm_mday <= 1) {
			alarm->tm.tm_mday = rtc_month_days(alarm->tm.tm_mon - 1, alarm->tm.tm_year - 1900);
		} else {
			alarm->tm.tm_mday--;
		}
	}
#ifdef CONFIG_UI_MANAGER
	alarm_calendar_mon_day_display(alarm, true);
#endif
}
static void _alarm_monitor_timer(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct alarm_app_t *alarm = alarm_get_app();

	if (!alarm)
		return;
	alarm->reflash_counter++;
#ifdef CONFIG_UI_MANAGER
	if (alarm->mode == ALARM_STATUS_CLOCK_DISPLAY || alarm->set_ok) {
		alarm_clock_display(alarm, true);
	} else if (alarm->mode == ALARM_STATUS_ALARM_SET_ONOFF) {
		if (alarm->reflash_counter % 2) {
			alarm_onoff_display(alarm, false);
		} else {
			alarm_onoff_display(alarm, true);
		}
	} else if (alarm->mode == ALARM_STATUS_ALARM_SET_HOUR || alarm->mode == ALARM_STATUS_CLOCK_SET_HOUR) {
		if (alarm->reflash_counter % 2) {
			alarm_clock_hour_flash(alarm, false);
		} else {
			alarm_clock_hour_flash(alarm, true);
		}
	} else if (alarm->mode == ALARM_STATUS_ALARM_SET_MIN || alarm->mode == ALARM_STATUS_CLOCK_SET_MIN) {
		if (alarm->reflash_counter % 2) {
			alarm_clock_min_flash(alarm, false);
		} else {
			alarm_clock_min_flash(alarm, true);
		}
	}  else if (alarm->mode == ALARM_STATUS_CALENDAR_SET_YEAR) {
		if (alarm->reflash_counter % 2) {
			alarm_calendar_year_display(alarm, false);
		} else {
			alarm_calendar_year_display(alarm, true);
		}
	}  else if (alarm->mode == ALARM_STATUS_CALENDAR_SET_MONTH) {
		if (alarm->reflash_counter % 2) {
			alarm_calendar_mon_flash(alarm, false);
		} else {
			alarm_calendar_mon_flash(alarm, true);
		}
	}  else if (alarm->mode == ALARM_STATUS_CALENDAR_SET_DAY) {
		if (alarm->reflash_counter % 2) {
			alarm_calendar_day_flash(alarm, false);
		} else {
			alarm_calendar_day_flash(alarm, true);
		}
	} else if (alarm->mode == ALARM_STATUS_RING_PLAYING || alarm->mode == ALARM_STATUS_NULL) {
		if (alarm->reflash_counter % 2) {
			alarm_ringing_clock_flash(alarm, false);
		} else {
			alarm_ringing_clock_flash(alarm, true);
		}
	}
#endif

	if ((alarm->mode != ALARM_STATUS_RING_PLAYING) && (alarm->mode != ALARM_STATUS_RING_STOP)
		&& (alarm->reflash_counter * REFLASH_CLOCK_PERIOD >= CLOCK_DISPLAY_TIMEOUT)) {
		struct app_msg msg = { 0 };

		msg.type = MSG_INPUT_EVENT;
		msg.cmd = MSG_ALARM_ENTRY_EXIT;
		msg.reserve = 0x01;
		send_async_msg(APP_ID_MAIN, &msg);
		SYS_LOG_INF("display timeout\n");
	}
}
static void _alarm_disk_check(struct alarm_app_t *alarm)
{
#ifdef CONFIG_MUTIPLE_VOLUME_MANAGER
	struct app_msg msg = {0};

	if (++alarm->check_disk_times < 10) {
		if (fs_manager_get_volume_state("/USB:")) {
			strncpy(alarm->dir, "/USB:/", sizeof(alarm->dir));
			goto check_finish;
		} else if (fs_manager_get_volume_state("/SD:")) {
			strncpy(alarm->dir, "/SD:/", sizeof(alarm->dir));
			goto check_finish;
		} else if (fs_manager_get_volume_state("/NAND:")) {
			strncpy(alarm->dir, "/NAND:/", sizeof(alarm->dir));
			goto check_finish;
		} else if (fs_manager_get_volume_state("/NOR:")) {
			strncpy(alarm->dir, "/NOR:/", sizeof(alarm->dir));
			goto check_finish;
		} else {
		#ifdef CONFIG_THREAD_TIMER
			thread_timer_start(&alarm->play_timer, 1000, 0);
		#endif
		}
	} else {
		SYS_LOG_ERR("timer out\n");
		goto exit;
	}
#else
	alarm->is_disk_check = 0;
	goto exit;
#endif
	return;
exit:
	alarm_exit_msg_send();
	return;
check_finish:
	msg.type = MSG_ALARM_EVENT;
	msg.cmd = MSG_ALARM_RING_PLAY_INIT;
	send_async_msg(APP_ID_ALARM, &msg);
	alarm->is_disk_check = 0;
	alarm->check_disk_times = 0;
}

extern uint8_t alarm_snooze_times;
static void _alarm_ring_play_timer(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct alarm_app_t *alarm = alarm_get_app();

	if (!alarm)
		return;
	if (alarm->is_disk_check) {
		_alarm_disk_check(alarm);
		return;
	}
	alarm_ring_stop(alarm, false);
	alarm_snooze_times = 0;
	alarm_exit_msg_send();
	SYS_LOG_INF("play timeout\n");
}
void alarm_thread_timer_init(void)
{
	struct alarm_app_t *alarm = alarm_get_app();

	if (!alarm)
		return;
	thread_timer_init(&alarm->monitor_timer, _alarm_monitor_timer, NULL);
	thread_timer_init(&alarm->play_timer, _alarm_ring_play_timer, NULL);
}

void alarm_disk_check_timer_start(struct alarm_app_t *alarm)
{
#ifdef CONFIG_THREAD_TIMER
	thread_timer_start(&alarm->play_timer, 1000, 0);
#endif
	alarm->is_disk_check = 1;
#ifdef CONFIG_UI_MANAGER
	alarm_clock_display(alarm, true);
#endif
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_manager_set_timeout_event(0, NULL);
#endif
	alarm->set_ok = 0;
#ifdef CONFIG_THREAD_TIMER
	thread_timer_start(&alarm->monitor_timer, REFLASH_CLOCK_PERIOD, REFLASH_CLOCK_PERIOD);
#endif
}

