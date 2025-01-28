/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system monitor
 */

#include <os_common_api.h>
#include <msg_manager.h>
#include <power_manager.h>
#include <property_manager.h>
#include <esd_manager.h>
#include <tts_manager.h>
#include <string.h>
#include <sys_event.h>
#include <sys_monitor.h>

#ifdef CONFIG_WATCHDOG
#include <watchdog_hal.h>
#endif

#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "sys_monitor"

#define CONFIG_MONITOR_PERIOD 100

static struct sys_monitor_t g_monitor;

struct sys_monitor_t *sys_monitor_get_instance(void)
{
	return &g_monitor;
}

#ifdef CONFIG_THREAD_TIMER
static void _sys_monitor_timer_handle(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	int system_event = SYS_EVENT_NONE;

	struct sys_monitor_t *sys_monitor =
		CONTAINER_OF(ttimer, struct sys_monitor_t, sys_monitor_timer);

	/**clear watchdog */
#ifdef CONFIG_WATCHDOG
	watchdog_clear();
#endif

	if (!sys_monitor || sys_monitor->monitor_stoped)
		return;

	for (int i = 0 ; i < MAX_MONITOR_WORK_NUM; i++) {
		if (!sys_monitor->monitor_work[i]) {
			continue;
		}

		system_event = sys_monitor->monitor_work[i]();
		if (system_event != SYS_EVENT_NONE) {
			sys_event_notify(system_event);
		}
	}
}
#else

static void _sys_monitor_timer_work(os_work *work)
{
	int system_event = SYS_EVENT_NONE;

	struct sys_monitor_t *sys_monitor =
		CONTAINER_OF(work, struct sys_monitor_t, sys_monitor_work);

	if (!sys_monitor || sys_monitor->monitor_stoped)
		return;

	for (int i = 0 ; i < MAX_MONITOR_WORK_NUM; i++) {
		if (!sys_monitor->monitor_work[i]) {
			continue;
		}
		system_event = sys_monitor->monitor_work[i]();
		if (system_event != SYS_EVENT_NONE) {
			sys_event_notify(system_event);
		}
	}

	os_delayed_work_submit(&sys_monitor->sys_monitor_work, CONFIG_MONITOR_PERIOD);

}
#endif

extern int sys_standby_init(void);

void sys_monitor_init(void)
{
	struct sys_monitor_t *sys_monitor = sys_monitor_get_instance();

	memset(sys_monitor, 0, sizeof(struct sys_monitor_t));

	sys_monitor->monitor_stoped = 0;

#ifdef CONFIG_THREAD_TIMER
	thread_timer_init(&sys_monitor->sys_monitor_timer, _sys_monitor_timer_handle, NULL);
#else
	os_delayed_work_init(&sys_monitor->sys_monitor_work, _sys_monitor_timer_work);
#endif

#ifdef CONFIG_SYS_STANDBY
	sys_standby_init();
#endif
}

int sys_monitor_add_work(monitor_work_handle monitor_work)
{
	int ret = -ESRCH;
	struct sys_monitor_t *sys_monitor = sys_monitor_get_instance();

	for (int i = 0 ; i < MAX_MONITOR_WORK_NUM; i++) {
		if (!sys_monitor->monitor_work[i]) {
			sys_monitor->monitor_work[i] = monitor_work;
			ret = 0;
			break;
		}
	}
	if (ret) {
		SYS_LOG_ERR(" err %d\n", ret);
	}
	return ret;
}

void sys_monitor_start(void)
{
	struct sys_monitor_t *sys_monitor = sys_monitor_get_instance();

#ifdef CONFIG_THREAD_TIMER
	thread_timer_start(&sys_monitor->sys_monitor_timer, CONFIG_MONITOR_PERIOD, CONFIG_MONITOR_PERIOD);
#else
	os_delayed_work_submit(&sys_monitor->sys_monitor_work, CONFIG_MONITOR_PERIOD);
#endif
#ifdef CONFIG_WATCHDOG
	watchdog_start(CONFIG_WDT_ACTS_OVERFLOW_TIME);
#endif
}

void sys_monitor_stop(void)
{
	struct sys_monitor_t *sys_monitor = sys_monitor_get_instance();

	sys_monitor->monitor_stoped = 1;

#ifdef CONFIG_THREAD_TIMER
	thread_timer_stop(&sys_monitor->sys_monitor_timer);
#else
	os_delayed_work_cancel(&sys_monitor->sys_monitor_work);
#endif

#ifdef CONFIG_WATCHDOG
	watchdog_stop();
#endif

}

void sys_monitor_deinit(void)
{

}

