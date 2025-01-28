/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <pm/pm.h>
#include "soc.h"

static struct pm_state_info  g_pm_stat_info = {
	.state = PM_STATE_ACTIVE,
	.substate_id = 0,
	.min_residency_us = 0,
};

static volatile bool b_enter_sleep;

#if 0
static void _sys_pm_enter_sleep(int deep)
{
#if defined(CONFIG_PM_DIRECT_FORCE_MODE) 
	struct pm_state_info  pm_stat;
	if(deep) {
		pm_stat.state = PM_STATE_SUSPEND_TO_RAM;
		pm_power_state_force(pm_stat);
	}else{
		pm_stat.state = PM_STATE_STANDBY;
		pm_power_state_force(pm_stat);
	}
#endif
}
#endif

void sys_pm_enter_deep_sleep(void)
{
	sl_dbg("enter deep sleep\n");
	b_enter_sleep = true;
}

#if 0
void sys_pm_enter_light_sleep(void)
{
	_sys_pm_enter_sleep(0);
}
#endif

#ifdef CONFIG_SYS_WAKELOCK
#include <sys_wakelock.h>
extern void pm_abort_enter_sleep(void); //power.c
static int check_wake_lock(void)
{
	if (sys_wakelocks_check(PARTIAL_WAKE_LOCK)
		|| sys_wakelocks_check(FULL_WAKE_LOCK)) {
		pm_abort_enter_sleep();
		sl_dbg("wake lock,not s3\n");
		return 1;
	}
	return 0;
}
#else
static int check_wake_lock(void)
{
	return 0;
}
#endif


struct pm_state_info pm_policy_next_state(int32_t ticks)
{
	g_pm_stat_info.state = PM_STATE_ACTIVE;
	if(b_enter_sleep){
		sys_s3_wksrc_init();
		b_enter_sleep = false;
		if(!check_wake_lock()) {
			g_pm_stat_info.state = PM_STATE_SUSPEND_TO_RAM;
			//g_pm_stat_info.state = PM_STATE_STANDBY;
			sl_dbg("deep sleep, ticks=%d\n", ticks);
		}
	}

	return g_pm_stat_info;
}

bool pm_policy_low_power_devices(enum pm_state state)
{
	return pm_is_sleep_state(state);
}

