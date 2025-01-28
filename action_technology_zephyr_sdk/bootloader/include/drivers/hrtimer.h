
/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief hr timer interface
 */
#ifndef HRTIMER_H_
#define HRTIMER_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_ACTS_HRTIMER

struct hrtimer;

/**
 * @typedef hrtimer_expiry_t
 * @brief hrtimer expiry function type.
 *
 * A timer's expiry function is executed by the thread message looper
 * each time the timer expires.
 *
 * @param ttimer          Address of hrtimer.
 * @param expiry_fn_arg   Argument that set by init function.
 *
 * @return N/A
 */
typedef void (*hrtimer_expiry_t)(struct hrtimer *ttimer, void *expiry_fn_arg);

struct hrtimer
{
    sys_dlist_t node;
	s32_t duration;
	s32_t period;
	u64_t expiry_time;
	hrtimer_expiry_t expiry_fn;
	void *expiry_fn_arg;
};

/**
 * @brief Initialize a hrtimer.
 *
 * This routine initializes a timer, prior to its first use.
 *
 * @param timer           Address of hrtimer.
 * @param expiry_fn       Function to invoke each time the thread timer expires.
 * @param expiry_fn_arg   Argument that need by expiry function.
 *
 * @return N/A
 */
extern void hrtimer_init(struct hrtimer *ttimer, hrtimer_expiry_t expiry_fn,
                   void *expiry_fn_arg);


/**
 * @brief Start a hrtimer.
 *
 * This routine starts a hrtimer.
 *
 * Attempting to start a hrtimer that is already running is permitted.
 * The timer's duration and period values is reset to use the new duration
 * and period values.
 *
 * @param timer     Address of hrtimer.
 * @param duration  Initial timer duration (in us).
 * @param period if Timer period (in us).
 *
 * @return N/A
 */
extern void hrtimer_start(struct hrtimer *ttimer, s32_t duration, s32_t period);


/**
 * @brief Stop a hrtimer.
 *
 * This routine stops a running hrtimer prematurely.
 *
 * Attempting to stop a hrtimer that is not running is permitted, but has no
 * effect on the thread timer.
 *
 *
 * @param timer     Address of hrtimer.
 *
 * @return N/A
 */
extern void hrtimer_stop(struct hrtimer* timer);

/**
 * @brief restart a hrtimer.
 *
 * This routine restart a hrtimer. The timer must be initialized and started before.
 *
 * @param ttimer          Address of hrtimer.
 *
 * @return N/A
 */
static inline void hrtimer_restart(struct hrtimer *ttimer)
{
	hrtimer_start(ttimer, ttimer->duration, ttimer->period);
}

/**
 * @brief hrtimer is running or not.
 *
 * This routine get the status of a hrtimer.
 *
 * @param ttimer          Address of hrtimer.
 *
 * @return true if the hrtimer is in hrtimer list, otherwise return false
 */
extern bool hrtimer_is_running(struct hrtimer *ttimer);

/**
 * @brief init hrtimer runtime data
 *
 * This routine init hrtimer runtime data
 *
 * @param N/A
 *
 * @return N/A
 */
//extern void hrtimer_runtime_init(void);

#endif

#ifdef __cplusplus
}
#endif

#endif /* HRTIMER_H_ */
