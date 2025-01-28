/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 /**
 * @file Trace dump interface
 *
 * NOTE: All Trace dump functions cannot be called in interrupt context.
 */

#ifndef _TRACEDUMP__H_
#define _TRACEDUMP__H_

#ifdef __cplusplus
extern "C" {
#endif

#include <toolchain.h>
#include <arch/cpu.h>

/* trace event */
enum {
	TRACE_NULL = 0,               /* NULL=0 */
	TRACE_HEAP = (1 << 0),        /* HEAP=1 */
	TRACE_HEAP_LEAK = (1 << 1),   /* HEAP_LEAK=2 */
	TRACE_WAKELOCK = (1 << 2),    /* WAKELOCK=4 */
};

#ifdef CONFIG_DEBUG_TRACEDUMP

/**
 * @brief Set enable event.
 *
 * This routine enables event.
 *
 * @param event trace event id
 * @param enable 0/1
 *
 * @return 0 if success, otherwise return -1
 */
extern int tracedump_set_enable(int event, int enable);

/**
 * @brief Get enable event.
 *
 * This routine get enable event.
 *
 * @param event trace event id
 *
 * @return 1 if enable, otherwise return 0
 */
extern int tracedump_get_enable(int event);

/**
 * @brief Save tracedump.
 *
 * This routine saves tracedump to flash.
 *
 * @param event trace event id
 * @param data1 user data1
 * @param data2 user data2
 *
 * @return 0 if success, otherwise return -1
 */
extern int tracedump_save(int event, uint32_t data1, uint32_t data2);

/**
 * @brief Remove tracedump.
 *
 * This routine remove tracedump to flash.
 *
 * @param event trace event id
 * @param data1 user data1
 *
 * @return 0 if success, otherwise return -1
 */
extern int tracedump_remove(int event, uint32_t data1);

/**
 * @brief Dump tracedump.
 *
 * This routine dump tracedump region from flash.
 *
 * @param N/A
 *
 * @return true if success, otherwise return false
 */
extern int tracedump_dump(void);

/**
 * @brief calling traverse_cb to transfer tracedump.
 *
 * This routine transfer tracedump region from flash.
 *
 * @param N/A
 *
 * @return transfer length.
 */
extern int tracedump_transfer(int (*traverse_cb)(uint8_t *data, uint32_t max_len));

/**
 * @brief get flash offset of tracedump.
 *
 * This routine get tracedump offset from flash.
 *
 * @param N/A
 *
 * @return tracedump offset.
 */
extern uint32_t tracedump_get_offset(void);

/**
 * @brief get flash size of tracedump.
 *
 * This routine get tracedump size from flash.
 *
 * @param N/A
 *
 * @return tracedump size.
 */
extern uint32_t tracedump_get_size(void);

/**
 * @brief Reset tracedump.
 *
 * This routine resets tracedump to flash.
 *
 * @param N/A
 *
 * @return true if success, otherwise return false
 */
extern int tracedump_reset(void);

/**
 * @brief Lock tracedump.
 *
 * This routine locks tracedump.
 *
 * @param N/A
 *
 * @return true if success, otherwise return false
 */
extern int tracedump_lock(void);

/**
 * @brief Unlock tracedump.
 *
 * This routine unlocks tracedump.
 *
 * @param N/A
 *
 * @return true if success, otherwise return false
 */
extern int tracedump_unlock(void);

/**
 * @brief Set filter for tracedump.
 *
 * This routine set filter for tracedump.
 *
 * @param data_off data offset for filter
 * @param data_sz data size for filter
 *
 * @return 0 if success, otherwise return -1
 */
extern int tracedump_set_filter(uint32_t data_off, uint32_t data_sz);

#else

#define tracedump_set_enable(x,y)	do{}while(0)
#define tracedump_get_enable(x)		(0)
#define tracedump_save(x,y,z)		do{}while(0)
#define tracedump_remove(x,y)		do{}while(0)
#define tracedump_dump()			do{}while(0)
#define tracedump_transfer(x)		do{}while(0)
#define tracedump_get_offset(x)		(0)
#define tracedump_get_size(x)		(0)
#define tracedump_reset()			do{}while(0)
#define tracedump_lock()			do{}while(0)
#define tracedump_unlock()			do{}while(0)
#define tracedump_set_filter(x,y)	do{}while(0)

#endif

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_DEBUG_TRACEDUMP */
