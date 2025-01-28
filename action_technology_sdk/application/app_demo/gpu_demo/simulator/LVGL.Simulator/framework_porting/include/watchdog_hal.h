/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief watchdog hal interface
 */

#ifndef __WATCHDOG_HAL_H__
#define __WATCHDOG_HAL_H__

#ifdef CONFIG_WATCHDOG
extern void watchdog_start(int timeout_ms);
extern void watchdog_stop(void);
extern void watchdog_clear(void);
#else
#define watchdog_start(timeout_ms)	do { } while (0)
#define watchdog_stop()			do { } while (0)
#define watchdog_clear()		do { } while (0)
#endif

#endif   //__WATCHDOG_HAL_H__
