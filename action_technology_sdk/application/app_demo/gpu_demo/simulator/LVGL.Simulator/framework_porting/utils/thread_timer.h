/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 /**
 * @file Thread timer interface
 *
 * NOTE: All Thread timer functions cannot be called in interrupt context.
 */

#ifndef _THREAD_TIMER__H_
#define _THREAD_TIMER__H_

#ifndef CONFIG_SIMULATOR
#include <sys_clock.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_THREAD_TIMER

struct thread_timer;

/**
 * @typedef thread_timer_expiry_t
 * @brief Thread timer expiry function type.
 *
 * A timer's expiry function is executed by the thread message looper
 * each time the timer expires.
 *
 * @param ttimer          Address of timer.
 * @param expiry_fn_arg   Argument that set by init function.
 *
 * @return N/A
 */
typedef void (*thread_timer_expiry_t)(struct thread_timer *ttimer, void *expiry_fn_arg);

struct thread_timer {
	sys_dlist_t node;
	int32_t duration;
	int32_t period;
	uint32_t expiry_time;
	thread_timer_expiry_t expiry_fn;
	void *expiry_fn_arg;
};

/**
 * @brief Initialize a thread timer.
 *
 * This routine initializes a timer, prior to its first use.
 *
 * @param ttimer          Address of thread timer.
 * @param expiry_fn       Function to invoke each time the thread timer expires.
 * @param arg             Argument that need by expiry function.
 *
 * @return N/A
 */
extern void thread_timer_init(struct thread_timer *ttimer,
			      thread_timer_expiry_t expiry_fn,
			      void *arg);

/**
 * @brief Start a thread timer.
 *
 * This routine starts a thread timer.
 *
 * Attempting to start a thread timer that is already running is permitted.
 * The timer's duration and period values is reset to use the new duration
 * and period values.
 *
 * @param ttimer    Address of timer.
 * @param duration  Initial timer duration (in milliseconds).
 * @param period    Timer period (in milliseconds).
 *
 * @return N/A
 */
extern void thread_timer_start(struct thread_timer *ttimer, int32_t duration, int32_t period);

/**
 * @brief Stop a thread timer.
 *
 * This routine stops a running thread timer prematurely.
 *
 * Attempting to stop a thread timer that is not running is permitted, but has no
 * effect on the thread timer.
 *
 *
 * @param ttimer    Address of timer.
 *
 * @return N/A
 */
extern void thread_timer_stop(struct thread_timer *ttimer);

/**
 * @brief restart a thread timer.
 *
 * This routine restart a timer. The timer must be initialized and started before.
 *
 * @param ttimer          Address of thread timer.
 *
 * @return N/A
 */
static inline void thread_timer_restart(struct thread_timer *ttimer)
{
	thread_timer_start(ttimer, ttimer->duration, ttimer->period);
}

/**
 * @brief restart a thread timer status.
 *
 * This routine get the status of a thread timer.
 *
 * @param ttimer          Address of thread timer.
 *
 * @return true if the thread timer is in thread timer list, otherwise return false
 */
extern bool thread_timer_is_running(struct thread_timer *ttimer);

/**
 * @brief get the next expiry thread time interval of current thread.
 *
 * This routine get the expiry time thread time interval of current threadt.
 *
 * @param N/A
 *
 * @return    the next expiry time interval
 *            K_FOREVER, if no actived thread timer in current thread
 */
extern int thread_timer_next_timeout(void);

/**
 * @brief handle the expired thread timers of current thread.
 *
 * This routine get the status of a thread timer.
 *
 * @param N/A
 *
 * @return N/A
 */
extern void thread_timer_handle_expired(void);

#else

#define thread_timer_next_timeout()		(-1)

#define thread_timer_handle_expired()		do { } while (0)

#endif

#ifdef __cplusplus
}
#endif

#endif /* _THREAD_TIMER__H_ */
