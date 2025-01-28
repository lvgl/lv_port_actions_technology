/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system wakelock
 */

#ifndef _SYS_WAKELOCK_H
#define _SYS_WAKELOCK_H
#include <stdint.h>
#include <stdbool.h>
/**
 * @defgroup sys_wakelock_apis App system wakelock APIs
 * @ingroup system_apis
 * @{
 */
enum
{
	/**Keep the CPU running normally, but the screen and tp may be off*/
	PARTIAL_WAKE_LOCK,
	/**Keep the CPU running normally, keep the screen highlighted, and the tp  also keep work well*/
	FULL_WAKE_LOCK,

	MAX_WAKE_LOCK_TYPE,
};

enum
{
	BT_WAKE_LOCK_USER,
	MEDIA_WAKE_LOCK_USER,
	USB_WAKE_LOCK_USER,
	POWER_WAKE_LOCK_USER,
	SYS_WAKE_LOCK_USER,
	APP_WAKE_LOCK_USER,
	MAX_WAKE_LOCK_USER,
};

/**
 * @brief hold system wakelock
 *
 * @details hold system wake lock Prevent the system entering sleep
 * @param wake_lock_type which wake lock type lock
 *
 * @return 0 excute success .
 * @return others excute failed .
 */
static inline int sys_wake_lock(int wake_lock_type) { return 0; }
static inline int sys_wake_lock_ext(int wake_lock_type, int wake_lock_user) { return 0; }

/**
 * @brief release system wakelock
 *
 * @details release system wake lock allowed the system entering sleep
 * @param wake_lock_type which wake lock type unlock
 *
 * @return 0 excute success .
 * @return others excute failed .
 */
static inline int sys_wake_unlock(int wake_lock_type) { return 0; }
static inline int sys_wake_unlock_ext(int wake_lock_type, int wake_lock_user) { return 0; }

/**
 * @brief check system wakelock state
 *
 * @details check system wakelock state , wakelock hold by user or not.
 * @param wake_lock_type which wake lock type
 *
 * @return 0 no wakelock holded by user.
 * @return others wakelock holded by user .
 */

static inline int sys_wakelocks_check(int wake_lock_type) { return 0; }

/**
 * @brief Return to the time difference between when the system
 *  wakelock is released to the present
 * @param wake_lock_type which wake lock type
 *
 * @return duration of  wakelock all released
 */
static inline uint32_t sys_wakelocks_get_free_time(int wake_lock_type) { return 0; }

/**
 * @brief Pend the current thread on wakelock
 *
 * @details pend the current thread on wakelock.
 * @param wake_lock_type which wake lock type
 *
 * @return 0 excute success .
 * @return others excute failed .
 */

static inline int sys_wakelocks_wait(int wake_lock_type) { return 0; }

/**
 * @brief Wake the thread pending on wakelock
 *
 * @details wake the thread pending on wakelock.
 * @param wake_lock_type which wake lock type
 *
 * @return 0 excute success .
 * @return others excute failed .
 */

static inline int sys_wakelocks_wake(int wake_lock_type) { return 0; }

/**
 * @brief dump wakelocks info
 *
 * @details dump wakelocks info.
 *
 * @return  N/A.
 */

static inline void sys_wakelocks_dump(void) {}

/**
 * @} end defgroup sys_wakelock_apis
 */
#endif


