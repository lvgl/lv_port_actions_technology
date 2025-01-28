/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <lvgl/lvgl_view.h>
#include <lvgl/lvgl_ext.h>
#include <lvgl/lvgl_virtual_display.h>
#include <ui_surface.h>
#include <ui_service.h>
#include <ui_manager.h>
#include <display/display_hal.h>
#include "lvgl_input_dev.h"
#ifdef CONFIG_GLYPHIX
#include "gx_lvgx.h"
#endif

static void * _lvgl_view_init(surface_t * surface)
{
	lv_display_t *disp = NULL;

	disp = lvgl_virtual_display_create(surface);
	if (disp == NULL) {
		SYS_LOG_ERR("Failed to create lvgl display");
	} else {
		lvgl_virtual_display_delete_refr_timer(disp);
	}

	return disp;
}

static void _lvgl_view_deinit(void * display)
{
	lvgl_virtual_display_delete(display);
}

static int _lvgl_view_resume(void * display, bool full_invalidate)
{
	lvgl_virtual_display_enable_refr(display, true);

	if (full_invalidate) {
		lv_obj_t * scr = lv_display_get_screen_active(display);
		if (scr)
			lv_obj_invalidate(scr);
	}

	return 0;
}

static int _lvgl_view_pause(void * display)
{
	lvgl_virtual_display_enable_refr(display, false);
	return 0;
}

static int _lvgl_view_set_default(void * display)
{
	return lvgl_virtual_display_set_default(display);
}

static int _lvgl_view_focus(void * display, bool focus)
{
	if (focus) {
		return lvgl_virtual_display_set_focus(display, false);
	}

	return 0;
}

static int _lvgl_view_refresh(void * display, bool full_refresh)
{
	lvgl_virtual_display_refr_now(display);
	return 0;
}

static int _lvgl_view_update_orientation(void * display, uint16_t rotation)
{
	return lvgl_virtual_display_update(display, rotation);
}

static void _lvgl_view_task_handler(void)
{
	lv_indev_t *indev = lv_indev_get_next(NULL);
	while (indev) {
		lv_indev_read_timer_cb(lv_indev_get_read_timer(indev));
		indev = lv_indev_get_next(indev);
	}

	lv_anim_refr_now();
	lv_timer_handler();
	lvgl_virtual_display_refr_now(NULL);

#ifdef CONFIG_GLYPHIX
	lvgx_process_events();
#endif
}

static const ui_view_gui_callback_t view_callback = {
	.init = _lvgl_view_init,
	.deinit = _lvgl_view_deinit,
	.resume = _lvgl_view_resume,
	.pause = _lvgl_view_pause,
	.set_default = _lvgl_view_set_default,
	.focus = _lvgl_view_focus,
	.refresh = _lvgl_view_refresh,
	.update_orientation = _lvgl_view_update_orientation,
	.task_handler = _lvgl_view_task_handler,
};

static void _msg_callback(struct app_msg *msg, int res, void *data)
{
	uint32_t opa_format;
	uint32_t trans_format;

	if (LV_COLOR_DEPTH == 16) {
		opa_format = HAL_PIXEL_FORMAT_RGB_565;
		trans_format = HAL_PIXEL_FORMAT_ARGB_8565;
	} else {
		opa_format = (surface_get_max_possible_buffer_count() > 0) ?
					HAL_PIXEL_FORMAT_BGR_888 : HAL_PIXEL_FORMAT_XRGB_8888;
		trans_format = HAL_PIXEL_FORMAT_ARGB_8888;
	}

	lvx_port_init();

	lv_timer_pause(lv_anim_get_timer());

	/* register view callback */
	ui_service_register_gui_view_callback(UI_VIEW_LVGL, opa_format, trans_format, &view_callback);

	/* register input callback */
	lvgl_input_pointer_init();
}

int lvgl_view_system_init(void)
{
	if (is_in_ui_thread()) {
		_msg_callback(NULL, 0, NULL);
		return 0;
	}

	return ui_message_send_sync2(VIEW_INVALID_ID, MSG_VIEW_NULL, 0, _msg_callback);
}

#ifdef CONFIG_GLYPHIX
static void _lvgx_init_callback(struct app_msg *msg, int res, void *data)
{
	lvgx_init();
}

int lvgx_view_system_init(void)
{
	if (is_in_ui_thread()) {
		_lvgx_init_callback(NULL, 0, NULL);
		return 0;
	}
	return ui_message_send_sync2(VIEW_INVALID_ID, MSG_VIEW_NULL, 0, _lvgx_init_callback);
}

static void _lvgx_deinit_callback(struct app_msg *msg, int res, void *data)
{
	lvgx_deinit();
}

int lvgx_view_system_deinit(void)
{
	if (is_in_ui_thread()) {
		_lvgx_deinit_callback(NULL, 0, NULL);
		return 0;
	}
	return ui_message_send_sync2(VIEW_INVALID_ID, MSG_VIEW_NULL, 0, _lvgx_deinit_callback);
}
#endif
