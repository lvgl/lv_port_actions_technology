/**
 * @file win32drv.h
 *
 */

#ifndef LV_WIN32DRV_H
#define LV_WIN32DRV_H

/*********************
 *      INCLUDES
 *********************/

#define USE_WIN32DRV 1

/* Scale window by this factor (useful when simulating small screens) */
#define WIN32DRV_MONITOR_ZOOM        1

#if USE_WIN32DRV

#include <Windows.h>

#if _MSC_VER >= 1200
 // Disable compilation warnings.
#pragma warning(push)
// nonstandard extension used : bit field types other than int
#pragma warning(disable:4214)
// 'conversion' conversion from 'type1' to 'type2', possible loss of data
#pragma warning(disable:4244)
#endif

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#if _MSC_VER >= 1200
// Restore compilation warnings.
#pragma warning(pop)
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

EXTERN_C bool lv_win32_quit_signal;

EXTERN_C lv_indev_t* lv_win32_pointer_device_object;
EXTERN_C lv_indev_t* lv_win32_keypad_device_object;
EXTERN_C lv_indev_t* lv_win32_encoder_device_object;

EXTERN_C void lv_win32_add_all_input_devices_to_group(
    lv_group_t* group);

EXTERN_C bool lv_win32_init(
    HINSTANCE instance_handle,
    int show_window_mode,
    int16_t hor_res,
    int16_t ver_res,
    HICON icon_handle);

EXTERN_C int lv_win32_handle_message_loop(void);

EXTERN_C void * lv_win32_get_frame_buffer(void);

EXTERN_C void lv_win32_flush_frame_buffer(
    int16_t x, int16_t y, int16_t w, int16_t h);

EXTERN_C void lv_win32_get_pointer_state(
    bool * pressed, int16_t * x, int16_t * y);

EXTERN_C void lv_win32_get_keypad_state(
    bool * pressed, uint32_t * key_val);

/**********************
 *      MACROS
 **********************/

#endif /*USE_WIN32DRV*/

#endif /*LV_WIN32DRV_H*/
