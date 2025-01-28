/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <spinlock.h>
#include <ksched.h>
#include <timeout_q.h>
#include <syscall_handler.h>
#include <drivers/timer/system_timer.h>
#include <sys_clock.h>

static uint64_t curr_tick;

static sys_dlist_t timeout_list = SYS_DLIST_STATIC_INIT(&timeout_list);

#ifdef CONFIG_TIME_SLEEP_COMP
static sys_dlist_t timeout_list_sl = SYS_DLIST_STATIC_INIT(&timeout_list_sl);
static bool b_time_exit_sleep = false;
void acts_exit_sleep(void)
{
	b_time_exit_sleep = true;
}

#endif


static struct k_spinlock timeout_lock;

#define MAX_WAIT (IS_ENABLED(CONFIG_SYSTEM_CLOCK_SLOPPY_IDLE) \
		  ? K_TICKS_FOREVER : INT_MAX)

/* Cycles left to process in the currently-executing sys_clock_announce() */
static int announce_remaining;

#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)
int z_clock_hw_cycles_per_sec = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_sys_clock_hw_cycles_per_sec_runtime_get(void)
{
	return z_impl_sys_clock_hw_cycles_per_sec_runtime_get();
}
#include <syscalls/sys_clock_hw_cycles_per_sec_runtime_get_mrsh.c>
#endif /* CONFIG_USERSPACE */
#endif /* CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME */

static struct _timeout *first(void)
{
	sys_dnode_t *t = sys_dlist_peek_head(&timeout_list);

	return t == NULL ? NULL : CONTAINER_OF(t, struct _timeout, node);
}

static struct _timeout *next(struct _timeout *t)
{
	sys_dnode_t *n = sys_dlist_peek_next(&timeout_list, &t->node);

	return n == NULL ? NULL : CONTAINER_OF(n, struct _timeout, node);
}

static void remove_timeout(struct _timeout *t)
{
	if (next(t) != NULL) {
		next(t)->dticks += t->dticks;
	}

	sys_dlist_remove(&t->node);
}

static int32_t elapsed(void)
{
	return announce_remaining == 0 ? sys_clock_elapsed() : 0U;
}

static int32_t next_timeout(void)
{
	struct _timeout *to = first();
	int32_t ticks_elapsed = elapsed();
	int32_t ret = to == NULL ? MAX_WAIT
		: CLAMP(to->dticks - ticks_elapsed, 0, MAX_WAIT);

#ifdef CONFIG_TIMESLICING
	if (_current_cpu->slice_ticks && _current_cpu->slice_ticks < ret) {
		ret = _current_cpu->slice_ticks;
	}
#endif
	return ret;
}

void z_add_timeout(struct _timeout *to, _timeout_func_t fn,
		   k_timeout_t timeout)
{
	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		return;
	}

#ifdef CONFIG_KERNEL_COHERENCE
	__ASSERT_NO_MSG(arch_mem_coherent(to));
#endif

	__ASSERT(!sys_dnode_is_linked(&to->node), "");
	to->fn = fn;
	#ifdef CONFIG_TIME_SLEEP_COMP
	if(b_time_exit_sleep){// Temporarily add to timeout_list_sl
		to->dticks = timeout.ticks;
		//printk("sl:add time, %p, fun=%p, tick=%d\n", to, fn, (uint32_t)to->dticks);
		LOCKED(&timeout_lock) {
			sys_dlist_append(&timeout_list_sl, &to->node);
		}
		return;
	}
	#endif
	LOCKED(&timeout_lock) {
		struct _timeout *t;

		if (IS_ENABLED(CONFIG_TIMEOUT_64BIT) &&
		    Z_TICK_ABS(timeout.ticks) >= 0) {
			k_ticks_t ticks = Z_TICK_ABS(timeout.ticks) - curr_tick;

			to->dticks = MAX(1, ticks);
		} else {
			to->dticks = timeout.ticks + 1 + elapsed();
		}

		for (t = first(); t != NULL; t = next(t)) {
			if (t->dticks > to->dticks) {
				t->dticks -= to->dticks;
				sys_dlist_insert(&t->node, &to->node);
				break;
			}
			to->dticks -= t->dticks;
		}

		if (t == NULL) {
			sys_dlist_append(&timeout_list, &to->node);
		}

		if (to == first()) {
#if CONFIG_TIMESLICING
			/*
			 * This is not ideal, since it does not
			 * account the time elapsed since the
			 * last announcement, and slice_ticks is based
			 * on that. It means that the time remaining for
			 * the next announcement can be less than
			 * slice_ticks.
			 */
			int32_t next_time = next_timeout();

			if (next_time == 0 ||
			    _current_cpu->slice_ticks != next_time) {
				sys_clock_set_timeout(next_time, false);
			}
#else
			sys_clock_set_timeout(next_timeout(), false);
#endif	/* CONFIG_TIMESLICING */
		}
	}
}

int z_abort_timeout(struct _timeout *to)
{
	int ret = -EINVAL;
#ifdef CONFIG_TIME_SLEEP_COMP
	if(b_time_exit_sleep){
		struct _timeout *t, *next;
		bool b_rm = false;
		if (!sys_dnode_is_linked(&to->node))
			return ret;
		LOCKED(&timeout_lock) {
			SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&timeout_list_sl, t, next, node) {
				if(t == to){ // // if timer in timeout_list_sl, remove
					sys_dlist_remove(&t->node);
					//printk("sl:remove time, %p, fun=%p, tick=%d\n", to, to->fn, (uint32_t)to->dticks);
					b_rm = true;
					break;
				}
			}
		}
		if(b_rm)
			return 0;
	}
#endif

	LOCKED(&timeout_lock) {
		if (sys_dnode_is_linked(&to->node)) {
			remove_timeout(to);
			ret = 0;
		}
	}

	return ret;
}

/* must be locked */
static k_ticks_t timeout_rem(const struct _timeout *timeout)
{
	k_ticks_t ticks = 0;

	if (z_is_inactive_timeout(timeout)) {
		return 0;
	}

	for (struct _timeout *t = first(); t != NULL; t = next(t)) {
		ticks += t->dticks;
		if (timeout == t) {
			break;
		}
	}

	return ticks - elapsed();
}

k_ticks_t z_timeout_remaining(const struct _timeout *timeout)
{
	k_ticks_t ticks = 0;

	LOCKED(&timeout_lock) {
		ticks = timeout_rem(timeout);
	}

	return ticks;
}

k_ticks_t z_timeout_expires(const struct _timeout *timeout)
{
	k_ticks_t ticks = 0;

	LOCKED(&timeout_lock) {
		ticks = curr_tick + timeout_rem(timeout);
	}

	return ticks;
}

int32_t z_get_next_timeout_expiry(void)
{
	int32_t ret = (int32_t) K_TICKS_FOREVER;

	LOCKED(&timeout_lock) {
		ret = next_timeout();
	}
	return ret;
}

void z_set_timeout_expiry(int32_t ticks, bool is_idle)
{
	LOCKED(&timeout_lock) {
		int next_to = next_timeout();
		bool sooner = (next_to == K_TICKS_FOREVER)
			      || (ticks <= next_to);
		bool imminent = next_to <= 1;

		/* Only set new timeouts when they are sooner than
		 * what we have.  Also don't try to set a timeout when
		 * one is about to expire: drivers have internal logic
		 * that will bump the timeout to the "next" tick if
		 * it's not considered to be settable as directed.
		 * SMP can't use this optimization though: we don't
		 * know when context switches happen until interrupt
		 * exit and so can't get the timeslicing clamp folded
		 * in.
		 */
		if (!imminent && (sooner || IS_ENABLED(CONFIG_SMP))) {
			sys_clock_set_timeout(MIN(ticks, next_to), is_idle);
		}
	}
}

void sys_clock_announce(int32_t ticks)
{
#ifdef CONFIG_TIMESLICING
	z_time_slice(ticks);
#endif

	k_spinlock_key_t key = k_spin_lock(&timeout_lock);

	announce_remaining = ticks;

	while (first() != NULL && first()->dticks <= announce_remaining) {
		struct _timeout *t = first();
		int dt = t->dticks;

		curr_tick += dt;
		announce_remaining -= dt;
		t->dticks = 0;
		remove_timeout(t);

		k_spin_unlock(&timeout_lock, key);
		t->fn(t);
		key = k_spin_lock(&timeout_lock);
	}
	#ifdef CONFIG_TIME_SLEEP_COMP
	if(b_time_exit_sleep){//
		struct _timeout *t0, *next;
		b_time_exit_sleep = false;
		if (first() != NULL){
			//printk("t_isr: next tick=%d, cur-t=%d\n\n", (uint32_t)(first()->dticks), ticks);
			first()->dticks -= announce_remaining;
		}else{
			//printk("t_isr: list null,  cur-t=%d\n", ticks);
		}
		curr_tick += announce_remaining;
		announce_remaining = 0;// clear announce_remaining
		SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&timeout_list_sl, t0, next, node) {
			sys_dlist_remove(&t0->node);
			//printk("t_isr:remove time, %p, fun=%p, tick=%d\n", t0, t0->fn, (uint32_t)t0->dticks);
			z_add_timeout(t0, t0->fn, Z_TIMEOUT_TICKS(t0->dticks));
		}
	}
	#endif
	if (first() != NULL) {
		first()->dticks -= announce_remaining;
	}

	curr_tick += announce_remaining;
	announce_remaining = 0;

	sys_clock_set_timeout(next_timeout(), false);

	k_spin_unlock(&timeout_lock, key);
}

int64_t sys_clock_tick_get(void)
{
	uint64_t t = 0U;

	LOCKED(&timeout_lock) {
		t = curr_tick + sys_clock_elapsed();
	}
	return t;
}

uint32_t sys_clock_tick_get_32(void)
{
#ifdef CONFIG_TICKLESS_KERNEL
	return (uint32_t)sys_clock_tick_get();
#else
	return (uint32_t)curr_tick;
#endif
}

int64_t z_impl_k_uptime_ticks(void)
{
	return sys_clock_tick_get();
}

#ifdef CONFIG_USERSPACE
static inline int64_t z_vrfy_k_uptime_ticks(void)
{
	return z_impl_k_uptime_ticks();
}
#include <syscalls/k_uptime_ticks_mrsh.c>
#endif

void z_impl_k_busy_wait(uint32_t usec_to_wait)
{
	SYS_PORT_TRACING_FUNC_ENTER(k_thread, busy_wait, usec_to_wait);
	if (usec_to_wait == 0U) {
		SYS_PORT_TRACING_FUNC_EXIT(k_thread, busy_wait, usec_to_wait);
		return;
	}

#if !defined(CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT)
	uint32_t start_cycles = k_cycle_get_32();

	/* use 64-bit math to prevent overflow when multiplying */
	uint32_t cycles_to_wait = (uint32_t)(
		(uint64_t)usec_to_wait *
		(uint64_t)sys_clock_hw_cycles_per_sec() /
		(uint64_t)USEC_PER_SEC
	);

	for (;;) {
		uint32_t current_cycles = k_cycle_get_32();

		/* this handles the rollover on an unsigned 32-bit value */
		if ((current_cycles - start_cycles) >= cycles_to_wait) {
			break;
		}
	}
#else
	arch_busy_wait(usec_to_wait);
#endif /* CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT */
	SYS_PORT_TRACING_FUNC_EXIT(k_thread, busy_wait, usec_to_wait);
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_busy_wait(uint32_t usec_to_wait)
{
	z_impl_k_busy_wait(usec_to_wait);
}
#include <syscalls/k_busy_wait_mrsh.c>
#endif /* CONFIG_USERSPACE */

/* Returns the uptime expiration (relative to an unlocked "now"!) of a
 * timeout object.  When used correctly, this should be called once,
 * synchronously with the user passing a new timeout value.  It should
 * not be used iteratively to adjust a timeout.
 */
uint64_t sys_clock_timeout_end_calc(k_timeout_t timeout)
{
	k_ticks_t dt;

	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		return UINT64_MAX;
	} else if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		return sys_clock_tick_get();
	} else {

		dt = timeout.ticks;

		if (IS_ENABLED(CONFIG_TIMEOUT_64BIT) && Z_TICK_ABS(dt) >= 0) {
			return Z_TICK_ABS(dt);
		}
		return sys_clock_tick_get() + MAX(1, dt);
	}
}
