/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file alarm manager interface
 */

#ifndef __ALARM_MANAGER_H__
#define __ALARM_MANAGER_H__

/**
 * @defgroup alarm_manager_apis alarm manager APIs
 * @ingroup system_apis
 * @{
 */
/**
 * @cond INTERNAL_HIDDEN
 */
#ifdef CONFIG_RTC_ACTS
#include <drivers/rtc.h>
#endif


#define MAX_ALARM_SUPPORT 3

enum alarm_state {
	ALARM_STATE_NULL = 0,
	ALARM_STATE_OFF,
	ALARM_STATE_FREE,
	ALARM_STATE_OK,
};

struct alarm_info
{
	int alarm_time;/*hour*3600+min*60+sec*/
	int state;
};

struct alarm_manager
{
	struct alarm_info alarm[MAX_ALARM_SUPPORT];
	struct alarm_info sn_alarm[MAX_ALARM_SUPPORT];
};

typedef void (*alarm_callback)(void);
int find_and_set_alarm(void);
int system_registry_alarm_callback(alarm_callback callback);
int alarm_manager_init(void);
int alarm_manager_get_time(struct rtc_time *tm);
int alarm_manager_set_time(struct rtc_time *tm);
int alarm_manager_get_alarm(struct rtc_time *tm, bool *is_on);
int alarm_manager_set_alarm(struct rtc_time *tm, bool is_snooze);
int alarm_manager_delete_alarm(struct rtc_time *tm);
bool alarm_wakeup_source_check(void);
struct alarm_manager *larm_manager_get_exist_alarm(int *alarm_count);
int alarm_manager_update_alarm(uint32_t time_sec, uint8_t alarm_index, int alarm_state);
struct alarm_manager *alarm_manager_get_exist_alarm(int *alarm_count);

/**
 * INTERNAL_HIDDEN @endcond
 */
/**
 * @} end defgroup alarm_manager_apis
 */
#endif
