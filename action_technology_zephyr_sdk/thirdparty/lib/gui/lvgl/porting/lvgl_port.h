/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file lvgl_port.h
 *
 */

#ifndef PORTING_LVGL_PORT_H
#define PORTING_LVGL_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../lvgl.h"

/**********************
 *      TYPEDEFS
 **********************/

typedef enum {
    LVX_DISPLAY_STATE_ON = 0, /* display screen on */
    LVX_DISPLAY_STATE_IDLE,   /* display screen AOD on */
    LVX_DISPLAY_STATE_OFF,    /* display screen off */
} lvx_display_state_t;

/* Display state observer */
typedef struct lvx_display_observer {
    /* Vsync/TE callback: timestamp: measured in sys clock cycles */
    void (*vsync_cb)(const struct lvx_display_observer * observer, uint32_t timestamp);
    /* (*state_cb) is called when state changed */
    void (*state_cb)(const struct lvx_display_observer * observer, uint32_t state);
    /* user specific data */
    void * user_data;
} lvx_display_observer_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * @brief Initialize LVGL
 *
 * Only required called once.
 *
 * @return LV_RESULT_OK on success, LV_RESULT_INVALID on error.
 */
lv_result_t lvx_port_init(void);

/**
 * @brief Add LVGL Display callback from physical screen
 *
 * Only required for physical and virtual displays which are registerd in lvx_port_init().
 *
 * @param pointer to structure lvx_display_observer_t
 *
 * @return LV_RESULT_OK on success, LV_RESULT_INVALID on error.
 */
lv_result_t lvx_display_add_observer(const lvx_display_observer_t * observer);

/**
 * @brief Wait flush finish
 *
 * @return LV_RESULT_OK on success, LV_RESULT_INVALID on error.
 */
lv_result_t lvx_display_flush_wait(void);

/**
 * @brief Wait flush finish
 *
 * @param draw_buf the draw buffer to store the image result. It's reshaped automatically.
 *
 * @return LV_RESULT_OK on success, LV_RESULT_INVALID on error.
 */
lv_result_t lvx_display_read_to_draw_buf(lv_draw_buf_t * draw_buf);

/**
 * @brief Begin overlay flushing
 *
 * @return LV_RESULT_OK on success, LV_RESULT_INVALID on error.
 */
void * lvx_display_overlay_prepare(void);

/**
 * @brief End overlay flushing
 *
 * @return LV_RESULT_OK on success, LV_RESULT_INVALID on error.
 */
lv_result_t lvx_display_overlay_unprepare(void);

/**
 * @brief Flush using overlay
 *
 * @param bg_buf background draw buffer
 * @param bg_pos background area
 * @param fg_buf foreground draw buffer
 * @param fg_pos foreground area
 * @param fg_opa foreground opacity
 *
 * @return LV_RESULT_OK on success, LV_RESULT_INVALID on error.
 */
lv_result_t lvx_display_overlay_flush(const lv_draw_buf_t * bg_buf, const lv_point_t * bg_pos,
                                      const lv_draw_buf_t * fg_buf, const lv_point_t * fg_pos,
                                      lv_opa_t fg_opa);

/**
 * @brief set refresh display/indev manually, not by timer
 *
 * @retval N/A
 */
void lvx_refr_set_manual(lv_display_t * disp);

/**
 * @brief refresh display/indev immediately
 *
 * @param disp pointer to display which refresh manually. Set NULL to indicate
 *             all the displays.
 *
 * @retval N/A
 */
void lvx_refr_all(void);

/**
 * @brief refresh display immediately
 *
 * @param disp pointer to display which refresh manually. Set NULL to indicate
 *             all the displays.
 *
 * @retval N/A
 */
void lvx_refr_display(lv_display_t * disp);

/**
 * @brief refresh indev immediately
 *
 * @param indev pointer to indev which refresh manually. Set NULL to indicate
 *             all the indevs.
 *
 * @retval N/A
 */
void lvx_refr_indev(lv_indev_t * indev);

/**
 * @brief Wait for a layer's draw operation to complete
 * @param disp   pointer to a display on which the dispatching was requested
 * @param layer  pointer to a layer
 * @retval       N/A
 */
void lvx_refr_finish_layer(lv_display_t * disp, lv_layer_t * layer);

/**
 * @brief Make display/indev refresh timer ready, will not wait their periods
 *
 * @retval N/A
 */
void lvx_refr_ready(void);

/**
 * @brief make display/indev refresh timer pause, will not wait their periods
 *
 * @retval N/A
 */
void lvx_refr_pause(void);

/**
 * @brief make display/indev refresh timer resume, will not wait their periods
 *
 * @retval N/A
 */
void lvx_refr_resume(void);

/**
 * @brief Update display/indev refresh period
 *
 * @param period new period in milliseconds.
 *
 * @retval N/A
 */
void lvx_refr_set_period(uint32_t period);

#ifdef __cplusplus
}
#endif

#endif /* PORTING_LVGL_PORT_H */
