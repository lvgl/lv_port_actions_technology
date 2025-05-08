/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2007-01-10     Bernard      the first version
 * 2008-07-12     Bernard      remove all rt_int8, rt_uint32_t etc typedef
 * 2010-10-26     yi.qiu       add module support
 * 2010-11-10     Bernard      add cleanup callback function in thread exit.
 * 2011-05-09     Bernard      use builtin va_arg in GCC 4.x
 * 2012-11-16     Bernard      change RT_NULL from ((void*)0) to 0.
 * 2012-12-29     Bernard      change the RT_USING_MEMPOOL location and add
 *                             RT_USING_MEMHEAP condition.
 * 2012-12-30     Bernard      add more control command for graphic.
 * 2013-01-09     Bernard      change version number.
 * 2015-02-01     Bernard      change version number to v2.1.0
 * 2017-08-31     Bernard      change version number to v3.0.0
 * 2017-11-30     Bernard      change version number to v3.0.1
 * 2017-12-27     Bernard      change version number to v3.0.2
 * 2018-02-24     Bernard      change version number to v3.0.3
 * 2018-04-25     Bernard      change version number to v3.0.4
 * 2018-05-31     Bernard      change version number to v3.1.0
 * 2018-09-04     Bernard      change version number to v3.1.1
 * 2018-09-14     Bernard      apply Apache License v2.0 to RT-Thread Kernel
 * 2018-10-13     Bernard      change version number to v4.0.0
 * 2018-10-02     Bernard      add 64bit arch support
 * 2018-11-22     Jesven       add smp member to struct rt_thread
 *                             add struct rt_cpu
 *                             add smp relevant macros
 * 2019-01-27     Bernard      change version number to v4.0.1
 * 2019-05-17     Bernard      change version number to v4.0.2
 * 2019-12-20     Bernard      change version number to v4.0.3
 * 2020-08-10     Meco Man     add macro for struct rt_device_ops
 * 2020-10-23     Meco Man     define maximum value of ipc type
 * 2021-03-19     Meco Man     add security devices
 * 2021-05-10     armink       change version number to v4.0.4
 * 2021-11-19     Meco Man     change version number to v4.1.0
 * 2021-12-21     Meco Man     re-implement RT_UNUSED
 * 2022-01-01     Gabriel      improve hooking method
 * 2022-01-07     Gabriel      move some __on_rt_xxxxx_hook to dedicated c source files
 * 2022-01-12     Meco Man     remove RT_THREAD_BLOCK
 * 2022-04-20     Meco Man     change version number to v4.1.1
 * 2022-04-21     THEWON       add macro RT_VERSION_CHECK
 * 2022-06-29     Meco Man     add RT_USING_LIBC and standard libc headers
 * 2022-08-16     Meco Man     change version number to v5.0.0
 * 2022-09-12     Meco Man     define rt_ssize_t
 * 2022-12-20     Meco Man     add const name for rt_object
 * 2023-04-01     Chushicheng  change version number to v5.0.1
 * 2023-05-20     Bernard      add stdc atomic detection.
 * 2023-09-15     xqyjlj       perf rt_hw_interrupt_disable/enable
 * 2023-10-10     Chushicheng  change version number to v5.1.0
 * 2023-10-11     zmshahaha    move specific devices related and driver to components/drivers
 * 2023-11-21     Meco Man     add RT_USING_NANO macro
 * 2023-11-17     xqyjlj       add process group and session support
 * 2023-12-01     Shell        Support of dynamic device
 * 2023-12-18     xqyjlj       add rt_always_inline
 * 2023-12-22     Shell        Support hook list
 * 2024-01-18     Shell        Seperate basical types to a rttypes.h
 *                             Seperate the compiler portings to rtcompiler.h
 * 2024-03-30     Meco Man     update version number to v5.2.0
 */

#ifndef __RT_DEF_H__
#define __RT_DEF_H__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "rttypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup BasicDef
 */

/**@{*/

/* RT-Thread version information */
#define RT_VERSION_MAJOR                5               /**< Major version number (X.x.x) */
#define RT_VERSION_MINOR                2               /**< Minor version number (x.X.x) */
#define RT_VERSION_PATCH                0               /**< Patch version number (x.x.X) */

/* e.g. #if (RTTHREAD_VERSION >= RT_VERSION_CHECK(4, 1, 0) */
#define RT_VERSION_CHECK(major, minor, revise)          ((major * 10000U) + (minor * 100U) + revise)

/* RT-Thread version */
#define RTTHREAD_VERSION                RT_VERSION_CHECK(RT_VERSION_MAJOR, RT_VERSION_MINOR, RT_VERSION_PATCH)

/**@}*/

/* maximum value of base type */
#ifdef RT_USING_LIBC
#define RT_UINT8_MAX                    UINT8_MAX       /**< Maximum number of UINT8 */
#define RT_UINT16_MAX                   UINT16_MAX      /**< Maximum number of UINT16 */
#define RT_UINT32_MAX                   UINT32_MAX      /**< Maximum number of UINT32 */
#define RT_UINT64_MAX                   UINT64_MAX      /**< Maximum number of UINT64 */
#else
#define RT_UINT8_MAX                    0xFFU                 /**< Maximum number of UINT8 */
#define RT_UINT16_MAX                   0xFFFFU               /**< Maximum number of UINT16 */
#define RT_UINT32_MAX                   0xFFFFFFFFUL          /**< Maximum number of UINT32 */
#define RT_UINT64_MAX                   0xFFFFFFFFFFFFFFFFULL /**< Maximum number of UINT64 */
#endif /* RT_USING_LIBC */

#define RT_TICK_MAX                     RT_UINT32_MAX   /**< Maximum number of tick */

/* maximum value of ipc type */
#define RT_SEM_VALUE_MAX                RT_UINT16_MAX   /**< Maximum number of semaphore .value */
#define RT_MUTEX_VALUE_MAX              RT_UINT16_MAX   /**< Maximum number of mutex .value */
#define RT_MUTEX_HOLD_MAX               RT_UINT8_MAX    /**< Maximum number of mutex .hold */
#define RT_MB_ENTRY_MAX                 RT_UINT16_MAX   /**< Maximum number of mailbox .entry */
#define RT_MQ_ENTRY_MAX                 RT_UINT16_MAX   /**< Maximum number of message queue .entry */

/* Common Utilities */

#define RT_UNUSED(x)                   ((void)(x))

/* compile time assertion */
#define RT_STATIC_ASSERT(name, expn) typedef char _static_assert_##name[(expn)?1:-1]

/* Compiler Related Definitions */
#include "rtcompiler.h"

/**
 * @ingroup BasicDef
 *
 * @def RT_IS_ALIGN(addr, align)
 * Return true(1) or false(0).
 *     RT_IS_ALIGN(128, 4) is judging whether 128 aligns with 4.
 *     The result is 1, which means 128 aligns with 4.
 * @note If the address is NULL, false(0) will be returned
 */
#define RT_IS_ALIGN(addr, align) ((!(addr & (align - 1))) && (addr != RT_NULL))

/**
 * @ingroup BasicDef
 *
 * @def RT_ALIGN(size, align)
 * Return the most contiguous size aligned at specified width. RT_ALIGN(13, 4)
 * would return 16.
 */
#define RT_ALIGN(size, align)           (((size) + (align) - 1) & ~((align) - 1))

/**
 * @ingroup BasicDef
 *
 * @def RT_ALIGN_DOWN(size, align)
 * Return the down number of aligned at specified width. RT_ALIGN_DOWN(13, 4)
 * would return 12.
 */
#define RT_ALIGN_DOWN(size, align)      ((size) & ~((align) - 1))

/**
 * @addtogroup Error
 */

/**@{*/

/* RT-Thread error code definitions */
#define RT_EOK                          0               /**< There is no error */
#define RT_ERROR                        1               /**< A generic/unknown error happens */
#define RT_ETIMEOUT                     2               /**< Timed out */
#define RT_EFULL                        3               /**< The resource is full */
#define RT_EEMPTY                       4               /**< The resource is empty */
#define RT_ENOMEM                       5               /**< No memory */
#define RT_ENOSYS                       6               /**< Function not implemented */
#define RT_EBUSY                        7               /**< Busy */
#define RT_EIO                          8               /**< IO error */
#define RT_EINTR                        9               /**< Interrupted system call */
#define RT_EINVAL                       10              /**< Invalid argument */
#define RT_ENOENT                       11              /**< No entry */
#define RT_ENOSPC                       12              /**< No space left */
#define RT_EPERM                        13              /**< Operation not permitted */
#define RT_ETRAP                        14              /**< Trap event */
#define RT_EFAULT                       15              /**< Bad address */
#define RT_ENOBUFS                      16              /**< No buffer space is available */
#define RT_ESCHEDISR                    17              /**< scheduler failure in isr context */
#define RT_ESCHEDLOCKED                 18              /**< scheduler failure in critical region */

/**@}*/

/**
 * rt_container_of - return the start address of struct type, while ptr is the
 * member of struct type.
 */
#define rt_container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

/**
 * @brief initialize a list object
 */
#define RT_LIST_OBJECT_INIT(object) { &(object), &(object) }

/**
 * @brief initialize a list
 *
 * @param l list to be initialized
 */
rt_inline void rt_list_init(rt_list_t *l)
{
    l->next = l->prev = l;
}

/**
 * @brief insert a node after a list
 *
 * @param l list to insert it
 * @param n new node to be inserted
 */
rt_inline void rt_list_insert_after(rt_list_t *l, rt_list_t *n)
{
    l->next->prev = n;
    n->next = l->next;

    l->next = n;
    n->prev = l;
}

/**
 * @brief insert a node before a list
 *
 * @param n new node to be inserted
 * @param l list to insert it
 */
rt_inline void rt_list_insert_before(rt_list_t *l, rt_list_t *n)
{
    l->prev->next = n;
    n->prev = l->prev;

    l->prev = n;
    n->next = l;
}

/**
 * @brief remove node from list.
 * @param n the node to remove from the list.
 */
rt_inline void rt_list_remove(rt_list_t *n)
{
    n->next->prev = n->prev;
    n->prev->next = n->next;

    n->next = n->prev = n;
}

/**
 * @brief tests whether a list is empty
 * @param l the list to test.
 */
rt_inline int rt_list_isempty(const rt_list_t *l)
{
    return l->next == l;
}

/**
 * @brief get the list length
 * @param l the list to get.
 */
rt_inline unsigned int rt_list_len(const rt_list_t *l)
{
    unsigned int len = 0;
    const rt_list_t *p = l;
    while (p->next != l)
    {
        p = p->next;
        len ++;
    }

    return len;
}

/**
 * @brief get the struct for this entry
 * @param node the entry point
 * @param type the type of structure
 * @param member the name of list in structure
 */
#define rt_list_entry(node, type, member) \
    rt_container_of(node, type, member)


/* RT-Thread porting */
#define LOG_D                   LOG_DBG
#define LOG_I                   LOG_INF
#define LOG_W                   LOG_WRN
#define LOG_E                   LOG_ERR

#define rt_strcmp               strcmp
#define rt_strncmp              strncmp
#define rt_strlen               strlen
#define rt_strcpy               strcpy
#define rt_strncpy              strncpy
#define rt_strcat               strcat
#define rt_memcmp               memcmp
#define rt_memset               memset
#define rt_memcpy               memcpy
#define rt_snprintf             snprintf
#define rt_kprintf              LOG_I

#define rt_malloc               dlheap_malloc
#define rt_free                 dlheap_free

#define rt_enter_critical       k_sched_lock
#define rt_exit_critical        k_sched_unlock

#define RT_ASSERT(x)            __ASSERT(x,"RT_ASSERT failed!")
#define RT_DEBUG_NOT_IN_INTERRUPT

#ifdef __cplusplus
}
#endif


#endif /* __RT_DEF_H__ */
