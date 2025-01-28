/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRAMEWORK_DISPLAY_LIBDISPLAY_LVGL_LVGL_VIRTUAL_DISPLAY_H
#define FRAMEWORK_DISPLAY_LIBDISPLAY_LVGL_LVGL_VIRTUAL_DISPLAY_H

#include <ui_surface.h>
#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a lvgl virtual display
 *
 * The requested pixel format must have the same color depth as LV_COLOR_DEPTH,
 * so no color transform is required between lvgl and the display surface.
 *
 * @param surface pointer to the display surface structure
 *
 * @return pointer to the created display; NULL if failed.
 */
lv_display_t * lvgl_virtual_display_create(surface_t * surface);

/**
 * @brief Destroy a lvgl virtual display
 *
 * @param disp pointer to a lvgl display
 *
 * @return N/A
 */
void lvgl_virtual_display_delete(lv_display_t * disp);

/**
 * @brief Set a lvgl virtual display as default display
 *
 * Set display as default does not mean it is focuesd and has input events.
 * It is just for convenience of LVGL user, like calling lv_scr_act().
 *
 * @param disp pointer to a lvgl display
 *
 * @retval 0 on success else negative errno code.
 */
int lvgl_virtual_display_set_default(lv_display_t * disp);

/**
 * @brief Focus on a lvgl virtual display
 *
 * @param disp pointer to a lvgl display
 * @param reset_indev should reset the indev or not
 *
 * @retval 0 on success else negative errno code.
 */
int lvgl_virtual_display_set_focus(lv_display_t * disp, bool reset_indev);

/**
 * @brief Update display rotation
 *
 * @param disp pointer to a lvgl display
 * @param rotation rotation angle (CW) in degrees
 *
 * @retval 0 on success else negative errno code.
 */
int lvgl_virtual_display_update(lv_display_t * disp, uint16_t rotation);

/**
 * @brief Refresh display immediately
 *
 * @param disp pointer to a lvgl display. set NULL to refresh all displays.
 *
 * @retval N/A
 */
void lvgl_virtual_display_refr_now(lv_display_t * disp);

/**
 * @brief Enable display refreshing or not
 *
 * To get full control of the refreshing, the refr_timer can be deleted. Or
 * lv_refr_now() can still trigger the refresh.
 *
 * @param disp pointer to a lvgl display
 * @param enable true to enable, false to disable
 *
 * @retval 0 on success else negative errno code.
 */
int lvgl_virtual_display_enable_refr(lv_display_t * disp, bool enable);

/**
 * @brief Get a pointer to the screen refresher timer
 *
 * @param disp pointer to a lvgl display
 *
 * @retval pointer to the display refresher timer. (NULL on error)
 */
static inline lv_timer_t * lvgl_virtual_display_get_refr_timer(lv_display_t * disp)
{
	return lv_display_get_refr_timer(disp);
}

/**
 * @brief Delete screen refresher timer
 *
 * @param disp pointer to a lvgl display
 *
 * @retval N/A
 */
static inline void lvgl_virtual_display_delete_refr_timer(lv_display_t * disp)
{
	lv_display_delete_refr_timer(disp);
}

#ifdef __cplusplus
}
#endif

#endif /* FRAMEWORK_DISPLAY_LIBDISPLAY_LVGL_LVGL_VIRTUAL_DISPLAY_H */
