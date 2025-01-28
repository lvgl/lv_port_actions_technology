/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file task wdt manager interface
 */

#ifndef __TASK_WDT_MANAGER_H__
#define __TASK_WDT_MANAGER_H__

#include <os_common_api.h>
#include <task_wdt/task_wdt.h>

/**
 * @brief task_wdt_manager_init
 *
 * This routine task_wdt_manager_init
 *
 * @return true  init success
 * @return false init failed
 */
int task_wdt_manager_init(void);
/**
 * @brief start task wdt for current task
 *
 * This routine add on soft task wdt for current task
 *
 * @param reload_period the period for task wdt
 *
 * @retval channel_id If successful, a non-negative value indicating the index
 *                    of the channel to which the timeout was assigned. This
 *                    ID is supposed to be used as the parameter in calls to
 *                    task_wdt_feed().
 * @retval -EINVAL If the reload_period is invalid.
 * @retval -ENOMEM If no more timeouts can be installed.
 */
int task_wdt_start(uint32_t reload_period);
/**
 * @brief Delete task watchdog channel for current task.
 *
 * Deletes the current task's  wdt channel from the list of task watchdog channels. The
 * channel is now available again for other tasks via task_wdt_add() function.
 *
 * @retval 0 If successful.
 * @retval -EINVAL If there is no installed timeout for supplied channel.
 */
int task_wdt_stop(void);
/**
 * @brief Feed the current task's  watchdog channel for current task.
 *
 * This function loops through all installed task watchdogs and updates the
 * internal kernel timer used as for the software watchdog with the next due
 * timeout.
 *
 * @retval 0 If successful.
 * @retval -EINVAL If there is no installed timeout for supplied channel.
 */
int task_wdt_user_feed(void);
#endif
