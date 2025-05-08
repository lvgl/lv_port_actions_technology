/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _VG_LITE_PLATFORM_H
#define _VG_LITE_PLATFORM_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/printk.h>

#define VG_LITE_ATTRIBUTE_FAST_BSS  __attribute__((section(".vglite.bss")))
#define VG_LITE_ATTRIBUTE_FAST_FUNC __attribute__((long_call, section(".vglite.func")))
#define VG_LITE_ATTRIBUTE_NOINLINE  __attribute__((noinline))

#define VG_LITE_DEBUG_DUMP_BEGIN(...) printk_dma_switch(0)
#define VG_LITE_DEBUG_DUMP_END(...)   printk_dma_switch(1)

#endif /* _VG_LITE_PLATFORM_H */
