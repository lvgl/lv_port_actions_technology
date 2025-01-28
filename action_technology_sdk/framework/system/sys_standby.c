/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system standby
 */

#include <os_common_api.h>
#include <app_manager.h>
#include <msg_manager.h>
#include <power_manager.h>
#ifdef CONFIG_UI_MANAGER
#include <ui_manager.h>
#include <ui_mem.h>
#endif
#include <property_manager.h>
#include <esd_manager.h>
#include <tts_manager.h>
#include <string.h>

#include <sys_event.h>
#include <sys_manager.h>
#include <mem_manager.h>
#include <sys_monitor.h>
#include <srv_manager.h>
#include <sys_wakelock.h>
#ifdef CONFIG_RES_MANAGER
#include <res_manager_api.h>
#endif
#ifndef CONFIG_SIMULATOR
#include <kernel.h>
#include <pm/pm.h>
#include <device.h>
#include <soc.h>
#endif
//#include <input_dev.h>
#include <audio_hal.h>

#ifdef CONFIG_BLUETOOTH
#include "bt_manager.h"
#endif

#include <watchdog_hal.h>

#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif

#if defined(CONFIG_SYS_LOG)
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "sys_standby"
#endif

/**
 * OTA app does not reponse MSG_APP_EARLY_SUSPEND/LATE_RESUME message during
 * upgrading, so donot send thess message to OTA app in case the system cannot
 * enter or exit STANDBY_S1.
 *
 * TAKE CARE:
 *  The app name must be the same of the real OTA app name.
 */
#ifndef APP_ID_OTA
#  define APP_ID_OTA "ota"
#endif

#define STANDBY_MIN_TIME_SEC (5)

enum STANDBY_STATE_E {
	STANDBY_NORMAL,
	STANDBY_S1,
	STANDBY_S2,
	STANDBY_S3,
};

enum STANDBY_MODE_E {
	STANDBY_SLEEP,
	STANDBY_LIGHT_SLEEP,
	STANDBY_DEEP_SLEEP,
};

struct standby_context_t {
	uint32_t	auto_standby_time;
	uint32_t	auto_powerdown_time;
	uint8_t		standby_state;
	uint8_t		standby_mode;
	uint32_t   wakeup_timestamp;
	uint32_t   wakeup_flag:1;
	uint32_t   force_standby:1;
	void * last_app;
};

struct standby_context_t *standby_context = NULL;

extern void thread_usleep(uint32_t usec);
extern int usb_hotplug_suspend(void);
extern int usb_hotplug_resume(void);

static os_sem wakeup_sem;

static int _sys_standby_check_auto_powerdown(void)
{
	int ret = 0;

	return ret;
}

static int _sys_standby_enter_s1(void)
{
	char *cur_app = app_manager_get_current_app();

	standby_context->standby_state = STANDBY_S1;

	if (!cur_app) {
		app_manager_notify_app("main", MSG_EARLY_SUSPEND_APP);
	}
	else if(strcmp(APP_ID_OTA, cur_app)) {
		app_manager_notify_app(cur_app, MSG_EARLY_SUSPEND_APP);
	}

#ifndef CONFIG_SIMULATOR
	pm_early_suspend();
#endif

	standby_context->last_app = app_manager_get_current_app();
	if (standby_context->last_app) {
		app_manager_notify_app(standby_context->last_app, MSG_SUSPEND_APP);
	}

	srv_manager_notify_service(NULL, MSG_SUSPEND_APP);

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	dvfs_unset_level(DVFS_LEVEL_NORMAL, "standby");
#endif

	/**set ble state stanyby*/
	SYS_LOG_INF("Enter S1");
	return 0;
}

static int _sys_standby_exit_s1(void)
{
	char *cur_app;

	standby_context->standby_state = STANDBY_NORMAL;

	standby_context->wakeup_timestamp = os_uptime_get_32();

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	dvfs_set_level(DVFS_LEVEL_NORMAL, "standby");
#endif
	srv_manager_notify_service(NULL, MSG_RESUME_APP);

	if (standby_context->last_app) {
		app_manager_notify_app(standby_context->last_app, MSG_RESUME_APP);
	}

#ifndef CONFIG_SIMULATOR
	pm_late_resume();
#endif

	cur_app = app_manager_get_current_app();
	if (!cur_app) {
		app_manager_notify_app("main", MSG_LATE_RESUME_APP);
	}
	else if (strcmp(APP_ID_OTA, cur_app)) {
		app_manager_notify_app(cur_app, MSG_LATE_RESUME_APP);
	}

	/**clear ble state stanyby*/
	SYS_LOG_INF("Exit S1");

	return 0;
}

static int _sys_standby_exit_s2(void);

static int _sys_standby_enter_s2(void)
{
	standby_context->standby_state = STANDBY_S2;

	if (standby_context->standby_mode == STANDBY_DEEP_SLEEP) {
		if (sys_wakelocks_check(PARTIAL_WAKE_LOCK)
			 || sys_wakelocks_check(FULL_WAKE_LOCK)) {
			standby_context->standby_state = STANDBY_S1;
		}
	}
	if (standby_context->standby_state == STANDBY_S2) {
		SYS_LOG_INF("Enter S2");
		//msg_manager_lock();
	} else if(standby_context->standby_state == STANDBY_S1){
		_sys_standby_exit_s2();
	}
	return 0;
}

static int _sys_standby_exit_s2(void)
{
	standby_context->standby_state = STANDBY_S1;

	/**TODO: add registry notify*/
	//if (standby_context->standby_mode == STANDBY_DEEP_SLEEP) {
	//#if CONFIG_DISPLAY
	//	ui_memory_init();
	//#endif
	//#ifdef CONFIG_RES_MANAGER
	//	res_manager_init();
	//#endif
	//}
	SYS_LOG_INF("Exit S2");
	return 0;
}

static int _sys_standby_enter_s3(void)
{
	standby_context->standby_state = STANDBY_S3;

	SYS_LOG_INF("Enter S3BT");

	standby_context->wakeup_flag = 0;

	sys_pm_enter_deep_sleep();

	/**wait for wake up*/
	os_sem_take(&wakeup_sem, OS_FOREVER);

	return 0;
}

static int _sys_standby_process_normal(void)
{
	uint32_t wakelocks = sys_wakelocks_check(FULL_WAKE_LOCK);

	/**have sys wake lock*/
	if (wakelocks) {
		SYS_LOG_DBG("wakelocks: 0x%08x\n", wakelocks);
		return 0;
	}

	if (standby_context->force_standby 
		|| sys_wakelocks_get_free_time(FULL_WAKE_LOCK) > standby_context->auto_standby_time)
		_sys_standby_enter_s1();

	return 0;
}

static int _sys_standby_process_s1(void)
{
	uint32_t wakelocks = sys_wakelocks_check(FULL_WAKE_LOCK);
	/**have sys wake lock*/
	if (wakelocks) {
		SYS_LOG_DBG("hold status: 0x%08x\n", wakelocks);
		_sys_standby_exit_s1();
	} else if (sys_wakelocks_get_free_time(FULL_WAKE_LOCK) < standby_context->auto_standby_time && !standby_context->force_standby) {
		_sys_standby_exit_s1();
	} else if (sys_wakelocks_get_free_time(PARTIAL_WAKE_LOCK) > 0) {
		if (!sys_wakelocks_check(PARTIAL_WAKE_LOCK)) {
			_sys_standby_enter_s2();
		}
	} else if (standby_context->force_standby){
		if (!sys_wakelocks_check(PARTIAL_WAKE_LOCK)) {
			_sys_standby_enter_s2();
		}
	}

	return 0;
}

static bool _sys_standby_wakeup_from_s2(void)
{
#if 0
	uint32_t wakelocks = sys_wakelocks_check(FULL_WAKE_LOCK);
	uint32_t pending = sys_pm_get_wakeup_pending();

	if (_sys_standby_check_auto_powerdown()) {
		return true;
	}

	if (pending & STANDBY_VALID_WAKEUP_PD) {
		sys_pm_clear_wakeup_pending();
		SYS_LOG_DBG("wakeup from S2: 0x%x", pending);
		return true;
	}

	if (power_manager_get_dc5v_status()) {
		SYS_LOG_DBG("wakeup from S2 because dc5v \n");
		return true;
	}

	if (wakelocks || sys_wakelocks_get_free_time(FULL_WAKE_LOCK) < standby_context->auto_standby_time) {
		SYS_LOG_DBG("wakelock: 0x%x\n", wakelocks);
		return true;
	}

	if (power_manager_check_is_no_power()) {
		SYS_LOG_INF("NO POWER\n");
		sys_pm_poweroff();
		return true;
	}

	/* FIXME: Here catch a hardware related issue.
	  * There is no wakeup pending of on-off key during s3bt stage,
	  * whereas the pending is happened at s1/s2 stage.
	  */
	if (sys_read32(PMU_ONOFF_KEY) & 0x1) {
		/* wait until ON-OFF key bounce */
		while (sys_read32(PMU_ONOFF_KEY) & 0x1)
			;
		SYS_LOG_INF("wakeup from ON-OFF key");
		return true;
	}
#endif
	return true;
}

static int _sys_standby_process_s2(void)
{
#ifndef CONFIG_SIMULATOR
	SYS_LOG_INF("enter");
	/**clear watchdog */
#ifdef CONFIG_WATCHDOG
	watchdog_clear();
#endif
	while(true) {
		if (sys_wakelocks_check(PARTIAL_WAKE_LOCK)
			|| sys_wakelocks_check(FULL_WAKE_LOCK)) {
			_sys_standby_exit_s2();
			break;
		}

		/**raise priority before deep sleep*/
		os_thread_priority_set(os_current_get(), -2);

		_sys_standby_enter_s3();

		/**have sys wake lock*/
		if (_sys_standby_wakeup_from_s2()) {
			_sys_standby_exit_s2();
			break;
		}
	}

	if ((sys_s3_wksrc_get() != SLEEP_WK_SRC_BT)
		 && (sys_s3_wksrc_get() != SLEEP_WK_SRC_T1)
		 &&	!(soc_get_aod_mode() && sys_s3_wksrc_get() == SLEEP_WK_SRC_RTC)) {
		sys_wake_lock_ext(FULL_WAKE_LOCK, SYS_WAKE_LOCK_USER);
		_sys_standby_exit_s1();
		sys_wake_unlock_ext(FULL_WAKE_LOCK, SYS_WAKE_LOCK_USER);
	}

	/**restore priorpty after wake up*/
	if (os_thread_priority_get(os_current_get()) < 14) {
		os_thread_priority_set(os_current_get(), 14);
	}
#endif
	return 0;
}

static int _sys_standby_work_handle(void)
{
	int ret = _sys_standby_check_auto_powerdown();

	if (ret)
		return ret;

	if (standby_context->standby_state == STANDBY_NORMAL) {
		ret = _sys_standby_process_normal();
	}

	if (standby_context->standby_state == STANDBY_S1) {
		ret = _sys_standby_process_s1();
	}

	if (standby_context->standby_state == STANDBY_S2) {
		ret = _sys_standby_process_s2();
	}

	return ret;
}

#ifdef CONFIG_AUTO_POWEDOWN_TIME_SEC
static bool _sys_standby_is_auto_powerdown(void)
{
	bool auto_powerdown = true;
	char temp[16];
	int ret;

	memset(temp, 0, sizeof(temp));
	ret = property_get(CFG_AUTO_POWERDOWN, temp, 16);
	if (ret > 0) {
		if (strcmp(temp, "false") == 0) {
			auto_powerdown = false;
		}
	}

	return auto_powerdown;
}
#endif

static void _sys_standby_entry_sleep(enum pm_state state)
{
	SYS_LOG_INF("enter \n");
}
static void _sys_standby_exit_sleep(enum pm_state state)
{
	SYS_LOG_INF("enter \n");
	sys_wakelocks_wake(PARTIAL_WAKE_LOCK);
	os_sem_give(&wakeup_sem);
}
#ifndef CONFIG_SIMULATOR
static struct pm_notifier notifier = {
	.state_entry = _sys_standby_entry_sleep,
	.state_exit = _sys_standby_exit_sleep,
};
#endif

static struct standby_context_t global_standby_context;

#define CONFIG_STANDBY_STACKSIZE	1280
static char __aligned(ARCH_STACK_PTR_ALIGN) __in_section_unique(noinit)
standby_thread_stack[CONFIG_STANDBY_STACKSIZE];
static os_tid_t standby_thread_data;

static void standby_thread(void *p1, void *p2, void *p3)
{
#ifndef CONFIG_SIMULATOR
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
#endif

	SYS_LOG_INF("standby thread started");

	while (1) {
		sys_wakelocks_wait(PARTIAL_WAKE_LOCK);
		_sys_standby_work_handle();
		os_yield();
	}
}

static int _sys_standby_check_handle(void)
{
	sys_wakelocks_wake(PARTIAL_WAKE_LOCK);
	return 0;
}

int sys_standby_init(void)
{
	standby_context = &global_standby_context;

	memset(standby_context, 0, sizeof(struct standby_context_t));

#ifdef CONFIG_AUTO_STANDBY_TIME_SEC
	if (0 == CONFIG_AUTO_STANDBY_TIME_SEC) {
		standby_context->auto_standby_time = (-1);
	} else if (CONFIG_AUTO_STANDBY_TIME_SEC < STANDBY_MIN_TIME_SEC) {
		SYS_LOG_WRN("too small, used default");
		standby_context->auto_standby_time = STANDBY_MIN_TIME_SEC * 1000;
	} else {
		standby_context->auto_standby_time = CONFIG_AUTO_STANDBY_TIME_SEC * 1000;
	}
#else
	standby_context->auto_standby_time = (-1);
#endif

#ifdef CONFIG_AUTO_POWEDOWN_TIME_SEC
	if (_sys_standby_is_auto_powerdown()) {
		standby_context->auto_powerdown_time = CONFIG_AUTO_POWEDOWN_TIME_SEC * 1000;
	} else {
		SYS_LOG_WRN("Disable auto powerdown\n");
		standby_context->auto_powerdown_time = (-1);
	}
#else
	standby_context->auto_powerdown_time = (-1);
#endif

	standby_context->standby_state = STANDBY_NORMAL;
	standby_context->standby_mode = STANDBY_DEEP_SLEEP;

	standby_context->wakeup_timestamp = os_uptime_get_32();


	if (sys_monitor_add_work(_sys_standby_check_handle)) {
		SYS_LOG_ERR("add work failed\n");
		return -EFAULT;
	}

#ifndef CONFIG_SIMULATOR
	pm_notifier_register(&notifier);
#endif

	SYS_LOG_INF("standby time : %d", standby_context->auto_standby_time);

	os_sem_init(&wakeup_sem, 0, 1);
	
	standby_thread_data = (os_tid_t)os_thread_create((char*)standby_thread_stack,
			CONFIG_STANDBY_STACKSIZE,
			standby_thread, NULL, NULL, NULL,
			14,
			0, OS_NO_WAIT);
	os_thread_name_set(standby_thread_data, "standby");
	
	return 0;
}

uint32_t system_wakeup_time(void)
{
	uint32_t wakeup_time = (-1);

	/** no need deal uint32_t overflow */
	if (standby_context->wakeup_timestamp) {
		wakeup_time = os_uptime_get_32() - standby_context->wakeup_timestamp;
	}

	SYS_LOG_INF("wakeup_time %d ms\n", wakeup_time);
	return wakeup_time;
}

uint32_t system_boot_time(void)
{
	return os_uptime_get();
}

void system_set_standby_mode(uint8_t sleep_mode)
{
	if (standby_context) {
		standby_context->standby_mode = sleep_mode;
	}
}

void system_set_auto_sleep_time(uint32_t timeout)
{
	if (standby_context) {
		standby_context->auto_standby_time = timeout * 1000;
	}
}

void system_request_fast_standby(void)
{
	if (standby_context) {
		standby_context->force_standby = 1;
	}
}

void system_clear_fast_standby(void)
{
	if (standby_context) {
		standby_context->force_standby = 0;
	}
}

bool system_is_screen_on(void)
{
	bool screen_on = false;
	if (standby_context) {
		screen_on = (standby_context->standby_state == STANDBY_NORMAL);
	}
	return screen_on;
}

