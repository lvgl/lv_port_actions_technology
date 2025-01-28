/******************************************************************************/
/*                                                                            */
/*    Copyright 2023 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.    */
/*                                                                            */
/******************************************************************************/

/**
 *  DESCRIPTION
 *
 *    This file defines the aic portable Interface.
 */

#ifndef __AIC_PORTABLE_H__
#define __AIC_PORTABLE_H__

#include <irq.h>
#include <kernel.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/printk.h>
#include <posix/unistd.h>

#define AIC_SRV_SW_BRANCH       "juxin"

#define aos_malloc(size)    k_malloc(size)
#define aos_zalloc(size)    k_calloc(1, size)
#define aos_free(mem_ptr)   k_free((void *)mem_ptr)

#define AOS_INTERRUPT_DECLARE   unsigned int level;
#define AOS_INTERRUPT_DISABLE   level = irq_lock();
#define AOS_INTERRUPT_RESTORE   irq_unlock(level);

/*
*********************************************************************
* Function   : AIC_ASSERT
* Arguments  : 入参n用作条件判断，当其为0时满足assert条件，会发生assert.
* Return     : None.
* Note(s)    : None.
* Description: AIC_ASSERT 为aic的通用assert接口，满足assert条件时，
*              需要出发系统assert。
*********************************************************************
*/
#define AIC_ASSERT      __ASSERT_NO_MSG

/*
*********************************************************************
* Function   : usleep
* Arguments  : nsec，定义睡眠的微妙时间.
* Return     : None.
* Note(s)    : None.
* Description: 当前task睡眠usec时间，到期后唤醒该task
*********************************************************************
*/
#if 0
#define usleep(usec)        k_busy_wait(usec)

#define sleep(sec)          k_msleep((sec)*1000)
#endif

/*
*********************************************************************
* Function   : alog_debug、alog_info、alog_warn、alog_error
* Arguments  : fmt ...
* Return     : None.
* Note(s)    : None.
* Description: 分别实现不同log level等级的log打印
*********************************************************************
*/

typedef enum
{
    /* Default: close all log */
    ALOG_LEVEL_NONE,
    /* Just open error log */
    ALOG_LEVEL_ERROR,
    /* Open error and warning log */
    ALOG_LEVEL_WARNING,
    /* Open info ,error and warning log */
    ALOG_LEVEL_INFO,
    /* Open all log */
    ALOG_LEVEL_DEBUG,

    ALOG_LEVEL_MAX,
    /* Ensure takeup 4 bytes. */
    ALOG_LEVEL_END = 0xFFFFFFFF
}alog_level_e;

int32_t alog_set_level(alog_level_e log_level);
alog_level_e alog_get_level(void);

/* alog default level */
#define ALOG_LEVEL_DEFAULT      ALOG_LEVEL_WARNING

/* alog_debug define */
#define alog_debug(fmt, ...) \
        if (ALOG_LEVEL_DEBUG <= alog_get_level()) { \
            do { printk(fmt, ##__VA_ARGS__); \
                printk("\n"); \
            } while (0); \
        }

/* alog_info define */
#define alog_info(fmt, ...) \
        if (ALOG_LEVEL_INFO <= alog_get_level()) { \
            do { printk(fmt, ##__VA_ARGS__); \
                printk("\n"); \
            } while (0); \
        }

/* alog_warn define */
#define alog_warn(fmt, ...) \
        if (ALOG_LEVEL_WARNING <= alog_get_level()) { \
            do { printk(fmt, ##__VA_ARGS__); \
                printk("\n"); \
            } while (0); \
        }

/* alog_error define */
#define alog_error(fmt, ...) \
        if (ALOG_LEVEL_ERROR <= alog_get_level()) { \
            do { printk(fmt, ##__VA_ARGS__); \
                printk("\n"); \
            } while (0); \
        }


#endif/* __AIC_PORTABLE_H__ */

