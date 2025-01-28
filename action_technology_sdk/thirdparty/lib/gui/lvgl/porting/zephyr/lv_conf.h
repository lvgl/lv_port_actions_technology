/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LV_CONF_H
#define LV_CONF_H

/*=========================
   STDLIB WRAPPER SETTINGS
 *=========================*/

/** Possible values
 * - LV_STDLIB_BUILTIN:     LVGL's built in implementation
 * - LV_STDLIB_CLIB:        Standard C functions, like malloc, strlen, etc
 * - LV_STDLIB_MICROPYTHON: MicroPython implementation
 * - LV_STDLIB_RTTHREAD:    RT-Thread implementation
 * - LV_STDLIB_CUSTOM:      Implement the functions externally
 */
#define LV_USE_STDLIB_MALLOC    LV_STDLIB_CUSTOM

/** Possible values
 * - LV_STDLIB_BUILTIN:     LVGL's built in implementation
 * - LV_STDLIB_CLIB:        Standard C functions, like malloc, strlen, etc
 * - LV_STDLIB_MICROPYTHON: MicroPython implementation
 * - LV_STDLIB_RTTHREAD:    RT-Thread implementation
 * - LV_STDLIB_CUSTOM:      Implement the functions externally
 */
#define LV_USE_STDLIB_STRING    LV_STDLIB_CLIB

/** Possible values
 * - LV_STDLIB_BUILTIN:     LVGL's built in implementation
 * - LV_STDLIB_CLIB:        Standard C functions, like malloc, strlen, etc
 * - LV_STDLIB_MICROPYTHON: MicroPython implementation
 * - LV_STDLIB_RTTHREAD:    RT-Thread implementation
 * - LV_STDLIB_CUSTOM:      Implement the functions externally
 */
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_CLIB

/*========================
 * RENDERING CONFIGURATION
 *========================*/

#define LV_USE_DRAW_CUSTOM_GPU_INIT 1
#define LV_VG_LITE_USE_GPU_INIT     1

#if defined(CONFIG_LV_USE_DRAW_VG_LITE)
    #define LV_DRAW_BUF_ALIGN 64
#else
    #define LV_DRAW_BUF_ALIGN 1
#endif

#ifndef LV_USE_DRAW_ACTS_DMA2D
    #ifdef CONFIG_LV_USE_DRAW_ACTS_DMA2D
        #define LV_USE_DRAW_ACTS_DMA2D 1
    #else
        #define LV_USE_DRAW_ACTS_DMA2D 0
    #endif
#endif

/*-------------
 * Asserts
 *-----------*/

#define LV_ASSERT_HANDLER_INCLUDE <sys/__assert.h>
#define LV_ASSERT_HANDLER __ASSERT(0, "LVGL fail");   /*Halt by default*/

/*=======================
 * FEATURE CONFIGURATION
 *=======================*/

/*=====================
 *  COMPILER SETTINGS
 *====================*/

/** Will be added where memory needs to be aligned (with -Os data might not be aligned to boundary by default).
 *  E.g. __attribute__((aligned(4)))*/
#if defined(CONFIG_LV_USE_DRAW_VG_LITE)
    #define LV_ATTRIBUTE_MEM_ALIGN __attribute__((aligned(64)))
    #define LV_ATTRIBUTE_MEM_ALIGN_SIZE 64
#else
    #define LV_ATTRIBUTE_MEM_ALIGN __attribute__((aligned(4)))
#endif

/** Attribute to mark large constant arrays, for example for font bitmaps */
#define LV_ATTRIBUTE_LARGE_CONST

/*=================
 *  TEXT SETTINGS
 *=================*/

/**
 * Select a character encoding for strings.
 * Your IDE or editor should have the same character encoding.
 * - LV_TXT_ENC_UTF8
 * - LV_TXT_ENC_ASCII
 */
#define LV_TXT_ENC LV_TXT_ENC_UTF8

/*==================
 * THEMES
 *==================*/

#define LV_USE_THEME_DEFAULT 0
#define LV_USE_THEME_SIMPLE  0
#define LV_USE_THEME_MONO    0

/*==================
 * OTHERS
 *==================*/

/*1: Enable the runtime performance profiler*/
#if defined(CONFIG_LV_USE_PROFILER)
    /*1: Enable the built-in profiler*/
    #define LV_USE_PROFILER_BUILTIN 0

    /*Header to include for the profiler*/
    #define LV_PROFILER_INCLUDE "tracing/tracing.h"

    #define LV_PROFILER_TRACE_ID SYS_TRACE_ID_LVGL_PROFILE

    /*Profiler start point function*/
    #define LV_PROFILER_BEGIN    LV_PROFILER_BEGIN_TAG(__func__)

    /*Profiler end point function*/
    #define LV_PROFILER_END      LV_PROFILER_END_TAG(__func__)

    /*Profiler start point function with custom tag*/
    #define LV_PROFILER_BEGIN_TAG(tag) sys_trace_string(LV_PROFILER_TRACE_ID, tag)

    /*Profiler end point function with custom tag*/
    #define LV_PROFILER_END_TAG(tag)   sys_trace_end_call(LV_PROFILER_TRACE_ID)
#endif /* CONFIG_LV_USE_PROFILER */

#if defined(CONFIG_LV_USE_STRACE)
    #define LV_STRACE_INCLUDE           "tracing/tracing.h"

    #define LV_STRACE_ID_INDEV          SYS_TRACE_ID_GUI_INDEV_TASK
    #define LV_STRACE_ID_REFR           SYS_TRACE_ID_GUI_REFR_TASK
    #define LV_STRACE_ID_UPDATE_LAYOUT  SYS_TRACE_ID_GUI_UPDATE_LAYOUT
    #define LV_STRACE_ID_OBJ_DRAW       SYS_TRACE_ID_GUI_WIDGET_DRAW
    #define LV_STRACE_ID_TASK_DRAW      SYS_TRACE_ID_GUI_TASK_DRAW
    #define LV_STRACE_ID_WAIT_FLUSH     SYS_TRACE_ID_GUI_WAIT
    #define LV_STRACE_ID_WAIT_DRAW      SYS_TRACE_ID_GUI_WAIT_DRAW
    #define LV_STRACE_ID_ANIM_CB        SYS_TRACE_ID_GUI_ANIM_CB
    #define LV_STRACE_ID_TIMER_CB       SYS_TRACE_ID_GUI_TIMER_CB

    #define LV_STRACE_VOID(id)  sys_trace_void(id)
    #define LV_STRACE_U32(id, p1)  sys_trace_u32(id, p1)
    #define LV_STRACE_U32X2(id, p1, p2)  sys_trace_u32x2(id, p1, p2)
    #define LV_STRACE_U32X3(id, p1, p2, p3)  sys_trace_u32x3(id, p1, p2, p3)
    #define LV_STRACE_U32X4(id, p1, p2, p3, p4)  sys_trace_u32x4(id, p1, p2, p3, p4)
    #define LV_STRACE_U32X5(id, p1, p2, p3, p4, p5)  sys_trace_u32x5(id, p1, p2, p3, p4, p5)
    #define LV_STRACE_U32X6(id, p1, p2, p3, p4, p5, p6)  sys_trace_u32x6(id, p1, p2, p3, p4, p5, p6)
    #define LV_STRACE_U32X7(id, p1, p2, p3, p4, p5, p6, p7)  sys_trace_u32x7(id, p1, p2, p3, p4, p5, p6, p7)
    #define LV_STRACE_U32X8(id, p1, p2, p3, p4, p5, p6, p7, p8)  sys_trace_u32x8(id, p1, p2, p3, p4, p5, p6, p7, p8)
    #define LV_STRACE_U32X9(id, p1, p2, p3, p4, p5, p6, p7, p8, p9)  sys_trace_u32x9(id, p1, p2, p3, p4, p5, p6, p7, p8, p9)
    #define LV_STRACE_U32X10(id, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)  sys_trace_u32x10(id, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)

    #define LV_STRACE_STRING(id, string)  sys_trace_string(id, string)
    #define LV_STRACE_STRING_U32X5(id, string, p1, p2, p3, p4, p5)  sys_trace_string_u32x5(id, string, p1, p2, p3, p4, p5)

    #define LV_STRACE_CALL(id)  sys_trace_end_call(id)
    #define LV_STRACE_CALL_U32(id, retv)  sys_trace_end_call_u32(id, retv)
#endif /* CONFIG_LV_USE_STRACE */

/*==================
* EXAMPLES
*==================*/

#define LV_BUILD_EXAMPLES 0

#endif /* LV_CONF_H */
