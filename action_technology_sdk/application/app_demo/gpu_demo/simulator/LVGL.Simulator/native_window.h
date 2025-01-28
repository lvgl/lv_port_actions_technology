/**
 * @file native_window.h
 *
 */

#ifndef NATIVE_WINDOW_H_
#define NATIVE_WINDOW_H_

/*********************
 *      INCLUDES
 *********************/
#include <simulator_config.h>
#include <stdint.h>
#include <stdbool.h>
#include <input_manager.h>
#include <drivers/rtc.h>

/*********************
 *      DEFINES
 *********************/
/* callback type */
enum {
    NWIN_CB_VSYNC = 0, /* for display vsync */
    NWIN_CB_CLOCK, /* for local time update */

    NWIN_NUM_CBS,
};

/**********************
 *      TYPEDEFS
 **********************/
typedef void (*native_window_callback_t)(void* user_data);

/**********************
 * GLOBAL PROTOTYPES
 **********************/

#ifdef __cplusplus
extern "C" {
#endif

int native_window_init(void);

int native_window_handle_message_loop(void);

bool native_window_is_closed(void);

void * native_window_get_framebuffer(void);

void native_window_flush_framebuffer(
    int16_t x, int16_t y, int16_t w, int16_t h);

bool native_window_get_pointer_state(input_dev_data_t* data);

bool native_window_get_keypad_state(input_dev_data_t* data);

int native_window_register_callback(unsigned int type, native_window_callback_t cb, void *user_data);
int native_window_unregister_callback(unsigned int type);

void native_window_get_local_time(struct rtc_time* tm);

#ifdef __cplusplus
}
#endif

/**********************
 *      MACROS
 **********************/

#endif /*NATIVE_WINDOW_H_*/
