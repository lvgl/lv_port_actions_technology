/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Kernel thread timer support
 *
 * This module provides thread timer support.
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <thread_timer.h>
#include <string.h>


#undef THREAD_TIMER_DEBUG
//#define CONFIG_THREAD_TIMER_DEBUG
//#define THREAD_TIMER_DEBUG

#ifdef THREAD_TIMER_DEBUG
#include <sys/printk.h>
#define TT_DEBUG(fmt, ...) printk("[%d] thread %p: " fmt, k_uptime_get_32(), \
				(void *)_current, ##__VA_ARGS__)
#else
#define TT_DEBUG(fmt, ...)
#endif

#define compare_time(a, b) ((int)((u32_t)(a) - (u32_t)(b)))

#ifdef CONFIG_THREAD_TIMER_DEBUG
static void _dump_thread_timer(struct thread_timer *ttimer)
{
	printk("timer %p, prev: %p, next: %p\n",
		ttimer, ttimer->node.prev, ttimer->node.next);

	printk("\tthread: %p, period %d ms, delay %d ms\n",
		_current, ttimer->period, ttimer->duration);

	printk("\texpiry_time: %u, expiry_fn: %p, arg %p\n\n",
		ttimer->expiry_time,
		ttimer->expiry_fn, ttimer->expiry_fn_arg);
}

void _dump_thread_timer_q(void)
{
	sys_dlist_t *thread_timer_q = &_current->thread_timer_q;
	struct thread_timer *ttimer;

	printk("thread: %p, thread_timer_q: %p, head: %p, tail: %p\n",
		_current, thread_timer_q, thread_timer_q->head,
		thread_timer_q->tail);

	SYS_DLIST_FOR_EACH_CONTAINER(thread_timer_q, ttimer, node) {
		_dump_thread_timer(ttimer);
	}
}
#endif

static void _thread_timer_remove(struct k_thread *thread, struct thread_timer *ttimer)
{
	struct thread_timer *in_q, *next;

	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&thread->thread_timer_q, in_q, next, node) {
		if (ttimer == in_q) {
			sys_dlist_remove(&in_q->node);
			return;
		}
	}
}

static void _thread_timer_insert(struct k_thread *thread, struct thread_timer *ttimer,
				 u32_t expiry_time)
{
	sys_dlist_t *thread_timer_q = &thread->thread_timer_q;
	struct thread_timer *in_q;

	SYS_DLIST_FOR_EACH_CONTAINER(thread_timer_q, in_q, node) {
		if (compare_time(ttimer->expiry_time, in_q->expiry_time) < 0) {
			sys_dlist_insert(&in_q->node, &ttimer->node);
			return;
		}
	}

	sys_dlist_append(thread_timer_q, &ttimer->node);
}

void thread_timer_init(struct thread_timer *ttimer, thread_timer_expiry_t expiry_fn,
		       void *expiry_fn_arg)
{
	__ASSERT(ttimer != NULL, "");

	if (!ttimer) {
		return;
	}

	TT_DEBUG("timer %p: init func %p arg 0x%x\n", ttimer,
		ttimer->expiry_fn, ttimer->expiry_fn_arg);

	/* remove thread timer if already submited */
	_thread_timer_remove(_current, ttimer);

	memset(ttimer, 0, sizeof(struct thread_timer));
	ttimer->expiry_fn = expiry_fn;
	ttimer->expiry_fn_arg = expiry_fn_arg;
	sys_dlist_init(&ttimer->node);
}

void thread_timer_start(struct thread_timer *ttimer, s32_t duration, s32_t period)
{
	u32_t cur_time;

	if (!ttimer) {
		return;
	}

	__ASSERT((ttimer != NULL) && (ttimer->expiry_fn != NULL), "");

	_thread_timer_remove(_current, ttimer);

	cur_time = k_uptime_get_32();
	ttimer->expiry_time = cur_time + duration;
	ttimer->period = period;
	ttimer->duration = duration;

	TT_DEBUG("timer %p: start duration %d period %d, expiry_time %d\n",
		ttimer, duration, period, ttimer->expiry_time);

	_thread_timer_insert(_current, ttimer, ttimer->expiry_time);
}

void thread_timer_stop(struct thread_timer *ttimer)
{
	__ASSERT(ttimer != NULL, "");

	TT_DEBUG("timer %p: stop\n", ttimer);

	_thread_timer_remove(_current, ttimer);
}

bool thread_timer_is_running(struct thread_timer *ttimer)
{
	struct thread_timer *in_q;

	__ASSERT(ttimer != NULL, "");

	SYS_DLIST_FOR_EACH_CONTAINER(&_current->thread_timer_q, in_q, node) {
		if (ttimer == in_q)
			return true;
	}

	return false;
}

int thread_timer_next_timeout(void)
{
	struct thread_timer *ttimer;
	int timeout;

	ttimer = SYS_DLIST_PEEK_HEAD_CONTAINER(&_current->thread_timer_q, ttimer, node);
	if (ttimer) {
		timeout = (int)((u32_t)ttimer->expiry_time - k_uptime_get_32());
		return (timeout < 0) ? 0 : timeout;
	}

	return -1;
}

void thread_timer_handle_expired(void)
{
	struct thread_timer *ttimer, *next;
	u32_t cur_time;

	cur_time = k_uptime_get_32();

	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&_current->thread_timer_q, ttimer, next, node) {
		if (compare_time(ttimer->expiry_time, cur_time) > 0) {
			/* no expired thread timer */
			return;
		}

		/* remove this expiry thread timer */
		sys_dlist_remove(&ttimer->node);

		if (ttimer->expiry_fn) {
			/* resubmit this thread timer if it is a period timer */
			if (ttimer->period != 0) {
				thread_timer_start(ttimer, ttimer->period,
						   ttimer->period);
			}

			TT_DEBUG("timer %p: call %p\n", ttimer, ttimer->expiry_fn);

			/* invoke thread timer expiry function */
			ttimer->expiry_fn(ttimer, ttimer->expiry_fn_arg);
		}
	}
}
