/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <init.h>
#include "os_common_api.h"

#ifdef CONFIG_DISPLAY_WORK_Q

static K_THREAD_STACK_DEFINE(display_workq_stack, CONFIG_DISPLAY_WORK_Q_STACK_SIZE);

static struct k_work_q display_workq;

static int display_work_q_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	k_work_queue_start(&display_workq, display_workq_stack,
		K_THREAD_STACK_SIZEOF(display_workq_stack),
		CONFIG_DISPLAY_WORK_Q_PRIORITY, NULL);
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
