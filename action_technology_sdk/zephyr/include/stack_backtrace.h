/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief cpu load statistic
 */

#ifndef __INCLUDE_STACK_BACKTRACE_H__
#define __INCLUDE_STACK_BACKTRACE_H__

#include <kernel.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifdef CONFIG_ARM_UNWIND

void dump_stack(void);
void show_thread_stack(struct k_thread *thread);
void show_all_threads_stack(void);

#else /* !CONFIG_STACK_BACKTRACE */

static inline void dump_stack(void)
{
}

static inline void show_thread_stack(struct k_thread *thread)
{
	ARG_UNUSED(thread);
}

static inline void show_all_threads_stack(void)
{
}

#endif /* !CONFIG_STACK_BACKTRACE */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __INCLUDE_STACK_BACKTRACE_H__ */
