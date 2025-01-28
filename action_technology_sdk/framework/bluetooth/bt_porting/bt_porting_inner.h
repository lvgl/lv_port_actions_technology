/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt porting inner headfile(only use in bluetooth).
*/

#ifndef __BT_PORTING_INNER_H__
#define __BT_PORTING_INNER_H__

#ifdef CONFIG_SYS_WAKELOCK
#include <sys_wakelock.h>
#endif

void *bt_mem_malloc(int size);
void bt_mem_free(void *ptr);

int stack_print_hex(const char *prefix, const void *data, int size);
int stack_print_linkkey(const char *prefix, const void *data, int size);
int stack_print_char(const char *prefix, const char *data, int size);

#define bt_get_sys_cycle()		k_cycle_get_32()
void bt_rand32_set_seed(uint32_t seed);
uint32_t bt_rand32_get(void);

#ifdef CONFIG_SYS_WAKELOCK
#define bt_wake_lock()			sys_wake_lock(PARTIAL_WAKE_LOCK)
#define bt_wake_unlock() 		sys_wake_unlock(PARTIAL_WAKE_LOCK)
#else
#define bt_wake_lock()
#define bt_wake_unlock()
#endif

#endif
