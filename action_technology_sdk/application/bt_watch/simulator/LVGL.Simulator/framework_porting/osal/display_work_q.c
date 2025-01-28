/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef CONFIG_SIMULATOR
#include <kernel.h>
#include <init.h>
#endif

#include "os_common_api.h"

#ifdef CONFIG_DISPLAY_WORK_Q

static struct k_work_q display_workq;

static __aligned(Z_THREAD_MIN_STACK_ALIGN) __in_section_unique(ram.noinit.stack)
uint8_t display_workq_stack[CONFIG_DISPLAY_WORK_Q_STACK_SIZE];

static int display_work_q_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	k_work_q_start(&display_workq,
		(k_thread_stack_t *)&display_workq_stack[sizeof(struct k_thread)],
		CONFIG_DISPLAY_WORK_Q_STACK_SIZE - sizeof(struct k_thread),
		CONFIG_DISPLAY_WORK_Q_PRIORITY);
	k_thread_name_set(&display_workq.thread, "displayworkq");

	return 0;
}

os_work_q *os_get_display_work_queue(void)
{
	return &display_workq;
}

SYS_INIT(display_work_q_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#else /* CONFIG_DISPLAY_WORK_Q */

os_work_q *os_get_display_work_queue(void)
{
	return NULL;
}

#endif /* CONFIG_DISPLAY_WORK_Q */
