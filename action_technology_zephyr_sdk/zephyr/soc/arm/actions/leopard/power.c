/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <pm/pm.h>
#include <soc.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>



/* Invoke Low Power/System Off specific Tasks */
void pm_power_state_set(struct pm_state_info info)
{

	if(!pm_is_sleep_state(info.state))
		soc_enter_deep_sleep();
	else
		soc_enter_light_sleep();
	
}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_power_state_exit_post_ops(struct pm_state_info info)
{
	irq_unlock(0);
}
