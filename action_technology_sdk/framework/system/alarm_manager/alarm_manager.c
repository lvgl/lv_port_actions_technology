/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file alarm manager interface
 */

#if defined(CONFIG_SYS_LOG)
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "alarm_manager"
#include <logging/sys_log.h>
#endif

#ifndef CONFIG_SIMULATOR
#include <drivers/rtc.h>
#include <soc.h>
#include <drivers/alarm.h>
#endif

#include <os_common_api.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "stream.h"
#include "mem_manager.h"
#include <alarm_manager.h>

#ifdef CONFIG_PROPERTY
#include "property_manager.h"
#endif


#define ALARM_DEFAULF_TIME	(7*60*60)/*default alarm time 07:00:00*/
#define ALARM_EXIST	(1)
#define SN_ALARM_EXIST	(2)

static struct alarm_manager al_manager;
alarm_callback call_back;
static uint8_t get_alarm_times;

static void alarm_manager_callback(const void *cb_data);
static int system_add_alarm(const struct device *dev, struct rtc_time *tm, bool is_snooze);
static int system_delete_alarm(const struct device *dev, struct rtc_time *tm);
static int system_get_alarm(struct rtc_time *tm, bool *is_on);
#if CONFIG_ALARM8HZ_ACTS
static int alarm_set_alarm8hz(const struct device *rtc_dev, struct rtc_time *tm);
#else
static int alarm_set_rtc(const struct device *dev, struct rtc_time *tm);
#endif
#ifdef CONFIG_RTC_ACTS
static void tm_to_time(struct rtc_time *tm, uint32_t *time)
{
	*time = (tm->tm_hour * 60 + tm->tm_min) * 60 + tm->tm_sec;
}

static void tm_add_one_day(struct rtc_time *tm)
{
	tm->tm_mday += 1;
	if (tm->tm_mday > rtc_month_days(tm->tm_mon, tm->tm_year)) {
		/* alarm day 1-31 */
		tm->tm_mday = 1;
		tm->tm_mon += 1;
		if (tm->tm_mon >= 13) {
			/* alarm mon 1-12 */
			tm->tm_mon = 1;
			tm->tm_year += 1;
		}
	}
}
static void time_to_tm(struct rtc_time *tm, uint32_t time)
{
	tm->tm_hour = time / 3600;
	tm->tm_min = time / 60 - tm->tm_hour * 60;
	tm->tm_sec = time % 60;
}
#endif

int find_and_set_alarm(void)
{
#ifdef CONFIG_RTC_ACTS
	int ret = 0;
	int i = 0;
	int earliest_alarm = -1;
	struct alarm_info *alarm = NULL;
	struct alarm_info *sn_alarm = NULL;
	const struct device *rtc = device_get_binding(CONFIG_RTC_0_NAME);
	uint32_t cur_time = 0;
	struct rtc_time tm = {0};

	if (!rtc) {
		SYS_LOG_ERR("no alarm\n");
		return -1;
	}
	ret = rtc_get_time(rtc, &tm);
	print_rtc_time(&tm);
	if (ret) {
		SYS_LOG_ERR("get curtime error\n");
		return -1;
	}
	tm_to_time(&tm, &cur_time);
	/*need to free current alarm*/
	for (i = 0; i < MAX_ALARM_SUPPORT; i++) {
		if (al_manager.alarm[i].state == ALARM_STATE_OK) {
			al_manager.alarm[i].state = ALARM_STATE_FREE;
			break;
		}

		if (al_manager.sn_alarm[i].state == ALARM_STATE_OK) {
			if ((al_manager.sn_alarm[i].alarm_time > cur_time) &&
				(al_manager.sn_alarm[i].alarm_time - cur_time < 36000)) {/* fix snooze time is 11:59:00,cur time is 00:00:00*/
				al_manager.sn_alarm[i].state = ALARM_STATE_FREE;
			} else {
				al_manager.sn_alarm[i].state = ALARM_STATE_NULL;
			}
			break;
		}
	}

	for (i = 0; i < MAX_ALARM_SUPPORT; i++) {
		alarm = &al_manager.alarm[i];

		if ((alarm->state == ALARM_STATE_FREE)
				&& (alarm->alarm_time > cur_time)) {
			if (earliest_alarm < 0) {
				earliest_alarm = alarm->alarm_time;
			} else if (earliest_alarm > alarm->alarm_time) {
				earliest_alarm = alarm->alarm_time;
			}
		}
		sn_alarm = &al_manager.sn_alarm[i];
		if ((sn_alarm->state == ALARM_STATE_FREE)
				&& (sn_alarm->alarm_time > cur_time)) {
			if (earliest_alarm < 0) {
				earliest_alarm = sn_alarm->alarm_time;
			} else if (earliest_alarm > sn_alarm->alarm_time) {
				earliest_alarm = sn_alarm->alarm_time;
			}
		}
	}
	if (earliest_alarm < 0) {
		/*if all alarm is earlier than current time*/

		for (i = 0; i < MAX_ALARM_SUPPORT; i++) {
			alarm = &al_manager.alarm[i];
			if ((alarm->state == ALARM_STATE_FREE)
					&& (alarm->alarm_time <= cur_time)) {
				if (earliest_alarm < 0) {
					earliest_alarm = alarm->alarm_time;
				} else if (earliest_alarm > alarm->alarm_time) {
					earliest_alarm = alarm->alarm_time;
				}
			}

			sn_alarm = &al_manager.sn_alarm[i];
			if ((sn_alarm->state == ALARM_STATE_FREE)
					&& (sn_alarm->alarm_time <= cur_time)) {
				if (earliest_alarm < 0) {
					earliest_alarm = sn_alarm->alarm_time;
				} else if (earliest_alarm > sn_alarm->alarm_time) {
					earliest_alarm = sn_alarm->alarm_time;
				}
			}
		}
	}
	if (earliest_alarm >= 0) {
		time_to_tm(&tm, earliest_alarm);
		if (earliest_alarm <= cur_time) {
			tm_add_one_day(&tm);
		}
	#if CONFIG_ALARM8HZ_ACTS
		ret = alarm_set_alarm8hz(rtc, &tm);
	#else
		ret = alarm_set_rtc(rtc, &tm);
	#endif

		if (ret) {
			SYS_LOG_ERR("set alarm error ret=%d\n", ret);
			return -1;
		}

		for (i = 0; i < MAX_ALARM_SUPPORT; i++) {
			alarm = &al_manager.alarm[i];
			if ((alarm->state == ALARM_STATE_FREE)
				&& (alarm->alarm_time == earliest_alarm)) {
				alarm->state = ALARM_STATE_OK;
				break;
			}

			sn_alarm = &al_manager.sn_alarm[i];
			if ((sn_alarm->state == ALARM_STATE_FREE)
				&& (sn_alarm->alarm_time == earliest_alarm)) {
				sn_alarm->state = ALARM_STATE_OK;
				break;
			}
		}
	}
#ifdef CONFIG_PROPERTY
	ret = property_set(CFG_ALARM_INFO,
				 (char *)&al_manager, sizeof(struct alarm_manager));
#endif
	if (ret < 0) {
		SYS_LOG_ERR("failed to set config %s, ret %d\n",
						 CFG_ALARM_INFO, ret);
		return -1;
	}
#endif
	return 0;
}
static int is_alarm_full(bool is_snooze)
{
	int i = 0;

	for (i = 0; i < MAX_ALARM_SUPPORT; i++) {
		if (is_snooze) {
			if (al_manager.sn_alarm[i].state <= ALARM_STATE_OFF) {
				break;
			}
		} else {
			if (al_manager.alarm[i].state <= ALARM_STATE_OFF) {
				break;
			}
		}
	}

	if (i == MAX_ALARM_SUPPORT) {
		return -2;
	}
	return 0;
}
static int is_alarm_exist(uint32_t time)
{
	for (int i = 0; i < MAX_ALARM_SUPPORT; i++) {
		if ((al_manager.sn_alarm[i].alarm_time == time)
				&& (al_manager.sn_alarm[i].state > ALARM_STATE_OFF)) {
			return SN_ALARM_EXIST;
		}

		if ((al_manager.alarm[i].alarm_time == time)
				&& (al_manager.alarm[i].state > ALARM_STATE_OFF)) {
			return ALARM_EXIST;
		}
	}
	return 0;
}

static int delect_sn_alarm(uint32_t time)
{
	for (int i = 0; i < MAX_ALARM_SUPPORT; i++) {
		if ((al_manager.sn_alarm[i].alarm_time == time)
				&& (al_manager.sn_alarm[i].state > ALARM_STATE_OFF)) {
			al_manager.sn_alarm[i].state = ALARM_STATE_NULL;
			break;
		}
	}
	SYS_LOG_INF("time %d\n", time);
	return 0;
}

static int is_need_set_alarm(const struct device *dev, uint32_t time)
{
#ifdef CONFIG_RTC_ACTS
	int i = 0;
	uint32_t cur_time = 0;
	struct rtc_time tm = {0};
	bool is_snooze_alarm_set = false;

	for (i = 0; i < MAX_ALARM_SUPPORT; i++) {
		if (al_manager.alarm[i].state == ALARM_STATE_OK) {
			break;
		}
		if (al_manager.sn_alarm[i].state == ALARM_STATE_OK) {
			is_snooze_alarm_set = true;
			break;
		}
	}
	if (i == MAX_ALARM_SUPPORT) {
		return 1;
	}
	int ret = rtc_get_time(dev, &tm);

	if (ret) {
		SYS_LOG_ERR("get curtime error\n");
		return 0;
	}
	tm_to_time(&tm, &cur_time);
	if (is_snooze_alarm_set) {
		if (time > cur_time) {
			for (i = 0; i < MAX_ALARM_SUPPORT; i++) {
				if (al_manager.sn_alarm[i].state == ALARM_STATE_OK) {
					if (time < al_manager.sn_alarm[i].alarm_time) {
						al_manager.sn_alarm[i].state = ALARM_STATE_FREE;
						return 1;
					} else if (al_manager.sn_alarm[i].alarm_time < cur_time) {
						al_manager.sn_alarm[i].state = ALARM_STATE_FREE;
						return 1;
					}
					break;
				}
			}
		} else if (time < cur_time) {
			for (i = 0; i < MAX_ALARM_SUPPORT; i++) {
				if (al_manager.sn_alarm[i].state == ALARM_STATE_OK) {
					if ((time < al_manager.sn_alarm[i].alarm_time)
						&& (al_manager.sn_alarm[i].alarm_time < cur_time)) {
						al_manager.sn_alarm[i].state = ALARM_STATE_FREE;
						return 1;
					}
					break;
				}
			}
		}

	} else {
		if (time > cur_time) {
			for (i = 0; i < MAX_ALARM_SUPPORT; i++) {
				if (al_manager.alarm[i].state == ALARM_STATE_OK) {
					if (time < al_manager.alarm[i].alarm_time) {
						al_manager.alarm[i].state = ALARM_STATE_FREE;
						return 1;
					} else if (al_manager.alarm[i].alarm_time < cur_time) {
						al_manager.alarm[i].state = ALARM_STATE_FREE;
						return 1;
					}
					break;
				}
			}
		} else if (time < cur_time) {
			for (i = 0; i < MAX_ALARM_SUPPORT; i++) {
				if (al_manager.alarm[i].state == ALARM_STATE_OK) {
					if ((time < al_manager.alarm[i].alarm_time)
						&& (al_manager.alarm[i].alarm_time < cur_time)) {
						al_manager.alarm[i].state = ALARM_STATE_FREE;
						return 1;
					}
					break;
				}
			}
		}
	}
#endif
	return 0;
}

static int check_alarm_time(int time)
{
	if (time < 0 || time >= 86400)
		return -EINVAL;
	return 0;
}
static void alarm_manager_callback(const void *cb_data)
{
	if (call_back != NULL) {
		call_back();
	}
}

int alarm_manager_init(void)
{
#ifdef CONFIG_RTC_ACTS
	const struct device *rtc = device_get_binding(CONFIG_RTC_0_NAME);

	if (!rtc) {
		SYS_LOG_ERR("no alarm\n");
		return -1;
	}
	rtc_enable(rtc);
#if 0
	struct alarm_time tm;
	uint32_t time_stamp = system_clock_init();

	alarm_time_to_tm(time_stamp, &tm)

	if (alarm_set_time(alarm, &tm)) {
		SYS_LOG_ERR("Failed to config RTC alarm\n");
		return -1;
	}
#endif
	memset(&al_manager, 0, sizeof(struct alarm_manager));
#ifdef CONFIG_PROPERTY
	int ret = property_get(CFG_ALARM_INFO,
				(char *)&al_manager, sizeof(struct alarm_manager));
#endif
	for (int i = 0; i < MAX_ALARM_SUPPORT; i++) {
		if (al_manager.alarm[i].state == ALARM_STATE_NULL &&
			al_manager.alarm[i].alarm_time != ALARM_DEFAULF_TIME) {
			al_manager.alarm[i].alarm_time = ALARM_DEFAULF_TIME;
		}
		SYS_LOG_INF("alarm[%d]:state=%d,alarm_time=%d\n",
			i, al_manager.alarm[i].state, al_manager.alarm[i].alarm_time);
	}

	if (ret < 0) {
		SYS_LOG_ERR("failed to get config %s, ret %d\n", CFG_ALARM_INFO, ret);
		return -1;
	}
#endif
	return 0;
}

int alarm_manager_get_time(struct rtc_time *tm)
{
#ifdef CONFIG_RTC_ACTS
	const struct device *rtc = device_get_binding(CONFIG_RTC_0_NAME);
	if (!rtc)
		return -ENODEV;

	int ret = rtc_get_time(rtc, tm);

	if (ret) {
		SYS_LOG_ERR("get time error ret=%d\n", ret);
		return -1;
	}
	tm->tm_year += 1900;
	tm->tm_mon += 1;
	return ret;
#else
	return 0;
#endif
}
int alarm_manager_set_time(struct rtc_time *tm)
{
#ifdef CONFIG_RTC_ACTS
	const struct device *rtc = device_get_binding(CONFIG_RTC_0_NAME);

	if (!rtc) {
		SYS_LOG_ERR("no alarm device\n");
		return -1;
	}
	tm->tm_year -= 1900;
	tm->tm_mon -= 1;
	int ret = rtc_set_time(rtc, tm);

	if (ret) {
		SYS_LOG_ERR("set time error ret=%d\n", ret);
		return -1;
	}
	print_rtc_time(tm);
	return ret;
#else
	return 0;
#endif
}
int alarm_manager_get_alarm(struct rtc_time *tm, bool *is_on)
{
	int ret = system_get_alarm(tm, is_on);

	if (ret) {
		SYS_LOG_ERR("get alarm error ret=%d\n", ret);
		return -1;
	}
	return ret;
}
int alarm_manager_set_alarm(struct rtc_time *tm, bool is_snooze)
{
#ifdef CONFIG_RTC_ACTS
	const struct device *rtc = device_get_binding(CONFIG_RTC_0_NAME);

	if (!rtc) {
		SYS_LOG_ERR("no alarm device\n");
		return -1;
	}
	int ret = system_add_alarm(rtc, tm, is_snooze);

	if (ret) {
		SYS_LOG_ERR("set alarm error ret=%d\n", ret);
		return -1;
	}
	get_alarm_times--;
	return ret;
#else
	return 0;
#endif
}
int alarm_manager_delete_alarm(struct rtc_time *tm)
{
#ifdef CONFIG_RTC_ACTS
	const struct device *rtc = device_get_binding(CONFIG_RTC_0_NAME);

	if (!rtc) {
		SYS_LOG_ERR("no alarm device\n");
		return -1;
	}
	int ret = system_delete_alarm(rtc, tm);

	if (ret) {
		SYS_LOG_ERR("delete alarm error ret=%d\n", ret);
		return -1;
	}
	print_rtc_time(tm);
	return ret;
#else
	return 0;
#endif
}
static int alarm_set_state(bool is_snooze, uint32_t time, uint8_t alarm_state)
{
	int i = 0;

	for (i = 0; i < MAX_ALARM_SUPPORT; i++) {
		if (is_snooze) {
			if (al_manager.sn_alarm[i].state <= ALARM_STATE_OFF) {
				al_manager.sn_alarm[i].state = alarm_state;
				al_manager.sn_alarm[i].alarm_time = time;
				break;
			}
		} else {
			if (al_manager.alarm[i].state <= ALARM_STATE_OFF) {
				al_manager.alarm[i].state = alarm_state;
				al_manager.alarm[i].alarm_time = time;
				break;
			}
		}
	}
	return 0;
}

#if CONFIG_ALARM8HZ_ACTS
static int alarm_set_alarm8hz(const struct device *rtc_dev, struct rtc_time *tm)
{
	const struct device *dev = device_get_binding(CONFIG_ALARM8HZ_0_NAME);
	struct alarm_config config = {0};
	int ret = 0;
	uint32_t cur_time = 0;
	uint32_t alarm_time = 0;
	struct rtc_time cur_tm = {0};
	uint32_t alarm_msec = 0;

	if (!dev) {
		printk("failed to get alarm8hz device:%s\n", CONFIG_ALARM8HZ_0_NAME);
		return -ENXIO;
	}
	ret = rtc_get_time(rtc_dev, &cur_tm);
	if (ret) {
		SYS_LOG_ERR("get curtime error\n");
		return ret;
	}
	rtc_tm_to_time(&cur_tm, &cur_time);
	rtc_tm_to_time(tm, &alarm_time);
	if ((alarm_time < cur_time) || (alarm_time - cur_time > 4294967)/*0xffffffff / 1000*/) {
		SYS_LOG_WRN("time invalid cur_time %d alarm %d\n", cur_time, alarm_time);
		return -EINVAL;
	}
	alarm_msec = (alarm_time - cur_time) * 1000;/*Turning milliseconds*/

	config.alarm_msec = alarm_msec;
	config.cb_fn = alarm_manager_callback;
	config.cb_data = dev;
	acts_alarm_set_alarm(dev, &config, true);

	SYS_LOG_INF(" cur_time %d alarm %d\n", cur_time, alarm_time);

	return 0;
}

#else /* CONFIG_ALARM8HZ_ACTS */

static int alarm_set_rtc(const struct device *dev, struct rtc_time *tm)
{
#ifdef CONFIG_RTC_ACTS
	struct rtc_alarm_config config = {0};
	int ret = 0;

	config.alarm_time.tm_year = tm->tm_year;
	config.alarm_time.tm_mon = tm->tm_mon;/*don't need to -1*/
	config.alarm_time.tm_mday = tm->tm_mday;
	config.alarm_time.tm_hour = tm->tm_hour;
	config.alarm_time.tm_min = tm->tm_min;
	config.alarm_time.tm_sec = tm->tm_sec;
	config.cb_fn = alarm_manager_callback;
	config.cb_data = NULL;

	SYS_LOG_INF("hour=%d,min=%d\n",
				config.alarm_time.tm_hour, config.alarm_time.tm_min);

	ret = rtc_set_alarm(dev, &config, true);
	if (ret) {
		SYS_LOG_ERR("set alarm error ret=%d\n", ret);
		return -1;
	}
#endif
}
#endif /* CONFIG_ALARM8HZ_ACTS */

static int system_add_alarm(const struct device *dev, struct rtc_time *tm, bool is_snooze)
{
#ifdef CONFIG_RTC_ACTS
	int ret = 0;
	uint32_t time = 0;

	tm->tm_mon--;/*need to - 1*/
	tm->tm_year -= 1900;/*need to - 1900*/
	if (rtc_valid_tm(tm)) {
		SYS_LOG_ERR("Bad time structure\n");
		return -ENOEXEC;
	}

	if (is_alarm_full(is_snooze)) {
		SYS_LOG_WRN("alarm full!\n");
		return -2;
	}
	tm_to_time(tm, &time);
	ret = is_alarm_exist(time);
	if (ret) {
		SYS_LOG_WRN("alarm existed!\n");
		if (ret == SN_ALARM_EXIST && !is_snooze) {
			delect_sn_alarm(time);
		} else {
			return -3;
		}
	}
	if (is_need_set_alarm(dev, time)) {
	#if CONFIG_ALARM8HZ_ACTS
		ret = alarm_set_alarm8hz(dev, tm);
	#else
		ret = alarm_set_rtc(dev, tm);
	#endif
		if (ret)
			return -1;
		alarm_set_state(is_snooze, time, ALARM_STATE_OK);
	} else {
		alarm_set_state(is_snooze, time, ALARM_STATE_FREE);
		SYS_LOG_INF("add alarm:hour=%d, min=%d\n", tm->tm_hour, tm->tm_min);
	}

#ifdef CONFIG_PROPERTY
	ret = property_set(CFG_ALARM_INFO,
				(char *)&al_manager, sizeof(struct alarm_manager));
	if (ret < 0) {
		SYS_LOG_ERR("failed to set config %s, ret %d\n", CFG_ALARM_INFO, ret);
		return -1;
	}
#endif
#endif
	return 0;
}

static int system_delete_alarm(const struct device *dev, struct rtc_time *tm)
{
#ifdef CONFIG_RTC_ACTS
	int i = 0;
	int ret = 0;
	uint32_t time = 0;
	bool need_delete = false;

	tm_to_time(tm, &time);
	SYS_LOG_INF("delect time=%d\n", time);
	for (int j = 0; j < MAX_ALARM_SUPPORT; j++) {
		SYS_LOG_DBG("al_manager.alarm[%d].state=%d"
					",al_manager.alarm[%d].alarm_time=%d\n",
			j, al_manager.alarm[j].state, j, al_manager.alarm[j].alarm_time);
	}

	for (i = 0; i < MAX_ALARM_SUPPORT; i++) {
		if ((al_manager.alarm[i].state > ALARM_STATE_OFF)
				&& (al_manager.alarm[i].alarm_time == time)) {
			need_delete = true;
			break;
		}
	}
	if (need_delete) {
		if (al_manager.alarm[i].state == ALARM_STATE_OK) {
			/*need remove alarm from alarm*/
		#if CONFIG_ALARM8HZ_ACTS
			const struct device *alarm_8hzdev = device_get_binding(CONFIG_ALARM8HZ_0_NAME);

			if (alarm_8hzdev == NULL) {
				SYS_LOG_ERR("alarm_8hzdev null\n");
				return -1;
			}
			ret = acts_alarm_set_alarm(alarm_8hzdev, NULL, false);
		#else
			ret = rtc_set_alarm(dev, NULL, false);
		#endif
			if (ret) {
				SYS_LOG_ERR("set alarm error ret=%d\n", ret);
				return -1;
			}
			al_manager.alarm[i].state = ALARM_STATE_OFF;
			if (!find_and_set_alarm())
				return 0;
		} else {
			al_manager.alarm[i].state = ALARM_STATE_OFF;
		}
	#ifdef CONFIG_PROPERTY
		ret = property_set(CFG_ALARM_INFO,
				(char *)&al_manager, sizeof(struct alarm_manager));
		if (ret < 0) {
		SYS_LOG_ERR("failed to set config %s, ret %d\n",
					CFG_ALARM_INFO, ret);
			return -1;
		}
	#endif
	}
#endif
	return 0;
}

static int system_get_alarm(struct rtc_time *tm, bool *is_on)
{
#ifdef CONFIG_RTC_ACTS
	for (int i = 0; i < MAX_ALARM_SUPPORT; i++) {
		if (get_alarm_times >= MAX_ALARM_SUPPORT)
			get_alarm_times = 0;
		SYS_LOG_INF("get_alarm_times=%d,alarm_time=%d\n",
			get_alarm_times, al_manager.alarm[get_alarm_times].alarm_time);
		if (check_alarm_time(al_manager.alarm[get_alarm_times].alarm_time)) {
			get_alarm_times++;
		} else {
			time_to_tm(tm, al_manager.alarm[get_alarm_times].alarm_time);
			SYS_LOG_INF("hour=%d,min=%d\n", tm->tm_hour, tm->tm_min);
			if (al_manager.alarm[get_alarm_times].state) {
				*is_on = true;
			} else {
				*is_on = false;
			}
			get_alarm_times++;
			break;
		}
	}
#endif
	return 0;
}

int system_registry_alarm_callback(alarm_callback callback)
{
	call_back = callback;
	return 0;
}
extern int sys_pm_get_wakeup_source(union sys_pm_wakeup_src *src);
bool alarm_wakeup_source_check(void)
{
#ifdef CONFIG_RTC_ACTS
	union sys_pm_wakeup_src src = {0};

	if (!sys_pm_get_wakeup_source(&src)) {
		SYS_LOG_INF("alarm=%d\n", src.t.alarm);
		if (src.t.alarm) {
			return true;
		}
	}
#endif
	return false;
}
struct alarm_manager *alarm_manager_get_exist_alarm(int *alarm_count)
{
	struct alarm_manager *alarm_manager = &al_manager;

	*alarm_count = MAX_ALARM_SUPPORT;
	return alarm_manager;
}
int alarm_manager_update_alarm(uint32_t time_sec, uint8_t alarm_index, int alarm_state)
{
#ifdef CONFIG_RTC_ACTS
	struct rtc_time tm;
	int ret = 0;

	if ((alarm_index >= MAX_ALARM_SUPPORT) || (alarm_state == ALARM_STATE_NULL))
		return -EINVAL;
	if (alarm_state == ALARM_STATE_OFF) {
		if (al_manager.alarm[alarm_index].state <= ALARM_STATE_OFF) {
			al_manager.alarm[alarm_index].alarm_time = time_sec;
			al_manager.alarm[alarm_index].state = alarm_state;
		#ifdef CONFIG_PROPERTY
			ret = property_set(CFG_ALARM_INFO,
					(char *)&al_manager, sizeof(struct alarm_manager));
			if (ret < 0) {
				SYS_LOG_ERR("failed to set config %s, ret %d\n", CFG_ALARM_INFO, ret);
				return -1;
			}
		#endif

		} else {
			time_to_tm(&tm, time_sec);
			al_manager.alarm[alarm_index].alarm_time = time_sec;
			alarm_manager_delete_alarm(&tm);
		}
	} else {
		al_manager.alarm[alarm_index].alarm_time = time_sec;
		al_manager.alarm[alarm_index].state = ALARM_STATE_FREE;
		find_and_set_alarm();
	}
#endif
	return 0;
}
