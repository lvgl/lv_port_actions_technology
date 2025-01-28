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

#ifdef __cplusplus
extern "C" {
#endif
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
	DISPLAY_WAKE_LOCK_USER,
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
int sys_wake_lock(int wake_lock_type);
int sys_wake_lock_ext(int wake_lock_type, int wake_lock_user);
/**
 * @brief release system wakelock
 *
 * @details release system wake lock allowed the system entering sleep
 * @param wake_lock_type which wake lock type unlock
 *
 * @return 0 excute success .
 * @return others excute failed .
 */
int sys_wake_unlock(int wake_lock_type);
int sys_wake_unlock_ext(int wake_lock_type, int wake_lock_user);

/**
 * @brief check system wakelock state
 *
 * @details check system wakelock state , wakelock hold by user or not.
 * @param wake_lock_type which wake lock type
 *
 * @return 0 no wakelock holded by user.
 * @return others wakelock holded by user .
 */

int sys_wakelocks_check(int wake_lock_type);

/**
 * @brief Return to the time difference between when the system
 *  wakelock is released to the present
 * @param wake_lock_type which wake lock type
 *
 * @return duration of  wakelock all released
 */


uint32_t sys_wakelocks_get_free_time(int wake_lock_type);

/**
 * @brief Pend the current thread on wakelock
 *
 * @details pend the current thread on wakelock.
 * @param wake_lock_type which wake lock type
 *
 * @return 0 excute success .
 * @return others excute failed .
 */

int sys_wakelocks_wait(int wake_lock_type);

/**
 * @brief Wake the thread pending on wakelock
 *
 * @details wake the thread pending on wakelock.
 * @param wake_lock_type which wake lock type
 *
 * @return 0 excute success .
 * @return others excute failed .
 */

int sys_wakelocks_wake(int wake_lock_type);

/**
 * @brief dump wakelocks info
 *
 * @details dump wakelocks info.
 *
 * @return  N/A.
 */

void sys_wakelocks_dump(void);

/**
 * @brief enable/disable wakelocks
 *
 * @details enable/disable wakelocks.
 * @param enable 0/1
 *
 * @return  N/A.
 */

void sys_wakelocks_enable(int enable);

/**
 * @} end defgroup sys_wakelock_apis
 */
#ifdef __cplusplus
}
#endif

#endif


