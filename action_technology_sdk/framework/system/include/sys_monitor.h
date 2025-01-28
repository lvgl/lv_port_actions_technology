/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system monitor
 */

#ifndef _SYS_MONITOR_H
#define _SYS_MONITOR_H
#include <os_common_api.h>
#include <thread_timer.h>
/**
 * @defgroup sys_monitor_apis App system monitor APIs
 * @ingroup system_apis
 * @{
 */
void sys_monitor_init(void);
/**
 * @brief start sysmte monitor
 *
 * @details start system monitor , this rontine make system monitor work
 * can excute in system app context, system monitor used thread timer as 
 * executor.
 *
 * @return N/A
 */

void sys_monitor_start(void);
/**
 * @brief stop sysmte monitor
 *
 * @details stop system monitor, all system monitor work will not excute.
 *
 * @return N/A
 */

void sys_monitor_stop(void);
/**
 * @brief deinit system monitor
 *
 * @details deinit system monitor ,clear system monitor resource.
 *
 * @return N/A
 */

void sys_monitor_deinit(void);
/**
 * @cond INTERNAL_HIDDEN
 */
#define MAX_MONITOR_WORK_NUM 5

/**
 * @brief system monitor work handle
 *
 * @details system monitor work handle, add to system monitor by user.
 * and excute int system monitor context(system app context), excute by
 * thread timer or system work.
 *
 * @return 0 excute success .
 * @return others excute failed .
 */
typedef int (*monitor_work_handle)(void);

/** system monitor structure */
struct sys_monitor_t
{
	/** monitor stoped flag */
	uint32_t monitor_stoped:1;
	/** system ready flag */
	uint32_t system_ready:1;

	/** monitor works , register by other user*/
	monitor_work_handle monitor_work[MAX_MONITOR_WORK_NUM];

	/** monitor excutor, default config to thread timer , if not support thread timer, used delay work*/
#ifdef CONFIG_THREAD_TIMER
	struct thread_timer sys_monitor_timer;
#else
	os_delayed_work sys_monitor_work;
#endif

};

/**
 * @brief get system monitor instance
 *
 * @details system monitor is singleton mode. if system monitor not init
 * this funciton init a system monitor and return this instance. otherwise
 * system monitor is already inited, return this instance dirctory.
 *
 * @return sys_monitor_t monitor instance 
 * @return NULL excute failed.
 */

struct sys_monitor_t *sys_monitor_get_instance(void);

/**
 * @brief add system monitor work to system monitor
 *
 * @details add work to system monitor, when system monitor start ,
 * the new work maybe excute by system monitor
 * @param monitor_work new work want to add to system monitor
 *
 * @return 0 excute success
 * @return others excute failed
 */

int sys_monitor_add_work(monitor_work_handle monitor_work);
/**
 * @brief system monitor init
 *
 * @details init system monitor , malloc system moitor resource
 * init thread timer as sysem monitor executor.
 *
 * @return N/A
 */
/**
 * INTERNAL_HIDDEN @endcond
 */
/**
 * @} end defgroup sys_monitor_apis
 */
#endif


