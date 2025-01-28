/**
 * @file lv_strace.h
 *
 */

#ifndef LV_STRACE_H
#define LV_STRACE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../lv_conf_internal.h"

#if LV_USE_STRACE
    #include LV_STRACE_INCLUDE
#endif /* LV_USE_STRACE */

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/

#ifndef LV_STRACE_ID_INDEV
    #define LV_STRACE_ID_INDEV 0
#endif

#ifndef LV_STRACE_ID_REFR
    #define LV_STRACE_ID_REFR  0
#endif

#ifndef LV_STRACE_ID_UPDATE_LAYOUT
    #define LV_STRACE_ID_UPDATE_LAYOUT  0
#endif

#ifndef LV_STRACE_ID_OBJ_DRAW
    #define LV_STRACE_ID_OBJ_DRAW  0
#endif

#ifndef LV_STRACE_ID_TASK_DRAW
    #define LV_STRACE_ID_TASK_DRAW  0
#endif

#ifndef LV_STRACE_ID_WAIT_FLUSH
    #define LV_STRACE_ID_WAIT_FLUSH  0
#endif

#ifndef LV_STRACE_ID_WAIT_DRAW
    #define LV_STRACE_ID_WAIT_DRAW  0
#endif

#ifndef LV_STRACE_VOID
    #define LV_STRACE_VOID(id)
#endif

#ifndef LV_STRACE_U32
    #define LV_STRACE_U32(id, p1)
#endif

#ifndef LV_STRACE_U32X2
    #define LV_STRACE_U32X2(id, p1, p2)
#endif

#ifndef LV_STRACE_U32X3
    #define LV_STRACE_U32X3(id, p1, p2, p3)
#endif

#ifndef LV_STRACE_U32X4
    #define LV_STRACE_U32X4(id, p1, p2, p3, p4)
#endif

#ifndef LV_STRACE_U32X5
    #define LV_STRACE_U32X5(id, p1, p2, p3, p4, p5)
#endif

#ifndef LV_STRACE_U32X6
    #define LV_STRACE_U32X6(id, p1, p2, p3, p4, p5, p6)
#endif

#ifndef LV_STRACE_U32X7
    #define LV_STRACE_U32X7(id, p1, p2, p3, p4, p5, p6, p7)
#endif

#ifndef LV_STRACE_U32X8
    #define LV_STRACE_U32X8(id, p1, p2, p3, p4, p5, p6, p7, p8)
#endif

#ifndef LV_STRACE_U32X9
    #define LV_STRACE_U32X9(id, p1, p2, p3, p4, p5, p6, p7, p8, p9)
#endif

#ifndef LV_STRACE_U32X10
    #define LV_STRACE_U32X10(id, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)
#endif

#ifndef LV_STRACE_STRING
    #define LV_STRACE_STRING(id, string)
#endif

#ifndef LV_STRACE_STRING_U32X5
    #define LV_STRACE_STRING_U32X5(id, string, p1, p2, p3, p4, p5)
#endif

#ifndef LV_STRACE_CALL
    #define LV_STRACE_CALL(id)
#endif

#ifndef LV_STRACE_CALL_U32
    #define LV_STRACE_CALL_U32(id, retv)
#endif

/*clang-format on*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_STRACE_H*/
