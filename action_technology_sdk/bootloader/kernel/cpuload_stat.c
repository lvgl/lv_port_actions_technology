/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief cpu load statistic
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <string.h>
#include <ksched.h>
#include <wait_q.h>
#include <sys/dlist.h>
#include <init.h>



/* start flag */
static int cpuload_started;

/* cpu load poll interval, unit: ms */
static int cpuload_interval;

struct k_delayed_work cpuload_stat_work;


void k_thread_runtime_clear(void);

static void cpuload_stat_clear(void)
{
	k_thread_runtime_clear();
}


void cpuload_stat(uint32_t interval)
{
	k_thread_runtime_stats_t rt_stats_thread;
	k_thread_runtime_stats_t rt_stats_all;

	struct k_thread *thread_list = NULL;
	unsigned int key, ratio, running_cycles, time_us;
	uint32_t ms_all;
	int is_curr_thread, curr_prio;

	k_thread_runtime_stats_all_get(&rt_stats_all);
	ms_all = SYS_CLOCK_HW_CYCLES_TO_NS_AVG(rt_stats_all.execution_cycles, 1000000);
	printk("stat interval %d, %d\n", interval, ms_all);
	interval = ms_all;
	printk("\t\t\t\t prio \t run(us)\t %%cpu\n");

	key = irq_lock();

	thread_list = (struct k_thread *)(_kernel.threads);
	while (thread_list != NULL) {
		k_thread_runtime_stats_get(thread_list, &rt_stats_thread);
		running_cycles = rt_stats_thread.execution_cycles;
		is_curr_thread = thread_list == k_current_get();
		curr_prio = k_thread_priority_get(thread_list);		
		irq_unlock(key);		

		time_us = SYS_CLOCK_HW_CYCLES_TO_NS_AVG(running_cycles, 1000);
		ratio = time_us / 10 / interval;

		printk("%s%s[%p]:\n\t\t\t\t  %d  \t %d \t\t %d\n",
		       is_curr_thread ? "*" : " ",
		       k_thread_name_get(thread_list)? k_thread_name_get(thread_list):"NULL",
		       thread_list,
		       curr_prio,
		       time_us,
		       ratio);

		key = irq_lock();	
		thread_list = (struct k_thread *)thread_list->next_thread;
	}

	irq_unlock(key);
	cpuload_stat_clear();
}

static void cpuload_stat_callback(struct k_work *work)
{
    cpuload_stat(cpuload_interval);

	k_delayed_work_submit(&cpuload_stat_work, Z_TIMEOUT_MS(cpuload_interval));
}

void cpuload_stat_start(int interval_ms)
{
	if (!interval_ms)
		return;

	if (cpuload_started)
		k_delayed_work_cancel(&cpuload_stat_work);

	cpuload_stat_clear();

	cpuload_interval = interval_ms;
	cpuload_started = 1;

	k_delayed_work_init(&cpuload_stat_work, cpuload_stat_callback);
	k_delayed_work_submit(&cpuload_stat_work, Z_TIMEOUT_MS(interval_ms));
}

void cpuload_stat_stop(void)
{
	k_delayed_work_cancel(&cpuload_stat_work);
	cpuload_started = 0;
}


