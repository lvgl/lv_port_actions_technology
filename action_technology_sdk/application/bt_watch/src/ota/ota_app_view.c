/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ota view
 */
#include "ota_app.h"
#include <string.h>
#include <stdio.h>
#include <app_ui.h>
#include <app_defines.h>
#include <widgets/img_number.h>

#ifdef CONFIG_RTC_ACTS
#  include <drivers/rtc.h>
#endif
#ifdef CONFIG_PROPERTY
#  include <property_manager.h>
#endif

#include "main_view.h"

#define OTA_GROGRESS_UPDATE_PERIOD (100)

typedef struct ota_view_data {
	lv_obj_t *obj_ota_bg;
	lv_obj_t *obj_prg_bar;
	/*slider style*/
	lv_style_t style_slider_bg;
	lv_style_t style_slider_indic;
	lv_style_t style_slider_knob;
	/* user data*/
	lv_timer_t *timer;
	uint8_t progress;
} ota_view_data_t;

static ota_view_data_t *p_ota_view_data = NULL;
extern const main_view_presenter_t main_view_presenter;

void ota_view_init(void)
{
#ifdef CONFIG_UI_SWITCH_EFFECT
	ui_switch_effect_set_type(UI_SWITCH_EFFECT_NONE);
#endif

#ifdef CONFIG_UI_MANAGER
	ui_view_create(OTA_VIEW, NULL, UI_CREATE_FLAG_SHOW | UI_CREATE_FLAG_NO_PRELOAD);
#endif

	SYS_LOG_INF("ok\n");
}

void ota_view_deinit(void)
{
#ifdef CONFIG_UI_MANAGER
	ui_view_delete(OTA_VIEW);
#endif

#ifdef CONFIG_PROPERTY
	property_flush(NULL); 
#endif

#ifdef CONFIG_UI_SWITCH_EFFECT
	ui_switch_effect_set_type(main_view_presenter.get_switch_mode());
#endif
	SYS_LOG_INF("ok\n");
}

void ota_view_show_upgrade_result(uint8_t *string, bool is_faill)
{
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_display_string(SLED_NUMBER1, string, true);
	seg_led_manager_set_timeout_event_start();
	seg_led_manager_set_timeout_event(1000, NULL);
#endif
#ifdef CONFIG_LED_MANAGER
	if (is_faill) {
		led_manager_set_display(0, LED_OFF, OS_FOREVER, NULL);
		led_manager_set_display(1, LED_OFF, OS_FOREVER, NULL);
	} else {
		led_manager_set_display(0, LED_ON, OS_FOREVER, NULL);
		led_manager_set_display(1, LED_ON, OS_FOREVER, NULL);
	}
#endif
}

void ota_view_show_upgrade_progress(uint8_t progress)
{
	if (p_ota_view_data) {
		p_ota_view_data->progress = progress;
		if (p_ota_view_data->timer) {
			lv_timer_ready(p_ota_view_data->timer);
		}
	}
}

static int _create_progress_bar(ota_view_data_t *data, lv_obj_t *scr)
{
	if (data == NULL)
		return -EINVAL;

	lv_style_init(&data->style_slider_bg);
	lv_style_set_bg_color(&data->style_slider_bg, lv_color_make(0xC0, 0xC0, 0xC0));
	lv_style_set_bg_opa(&data->style_slider_bg, LV_OPA_COVER);
	lv_style_set_radius(&data->style_slider_bg, LV_RADIUS_CIRCLE);

	lv_style_init(&data->style_slider_indic);
	lv_style_set_bg_color(&data->style_slider_indic, lv_color_make(0, 0, 255));
	lv_style_set_bg_opa(&data->style_slider_indic, LV_OPA_COVER);

	lv_style_init(&data->style_slider_knob);
	lv_style_set_bg_opa(&data->style_slider_knob, LV_OPA_COVER);
	lv_style_set_bg_color(&data->style_slider_knob, lv_color_make(0, 0, 255));
	lv_style_set_radius(&data->style_slider_knob, LV_RADIUS_CIRCLE);
	lv_style_set_pad_all(&data->style_slider_knob, 10);

	data->obj_prg_bar = lv_slider_create(scr);
	if (data->obj_prg_bar == NULL)
		return -ENOMEM;
	lv_obj_set_size(data->obj_prg_bar, DEF_UI_WIDTH - 100, 10);
	lv_obj_add_style(data->obj_prg_bar, &data->style_slider_bg, LV_PART_MAIN);
	lv_obj_add_style(data->obj_prg_bar, &data->style_slider_indic, LV_PART_INDICATOR);
	lv_obj_add_style(data->obj_prg_bar, &data->style_slider_knob, LV_PART_KNOB);
	lv_obj_remove_flag(data->obj_prg_bar, LV_OBJ_FLAG_CLICKABLE);/*remove click event*/

	lv_obj_center(data->obj_prg_bar);
	lv_slider_set_range(data->obj_prg_bar, 0, 100);

	lv_slider_set_value(data->obj_prg_bar, 0, LV_ANIM_OFF);
	return 0;
}

static int _ota_view_paint(ota_view_data_t *data)
{
	if (data && data->obj_prg_bar) {
		lv_slider_set_value(data->obj_prg_bar, data->progress, LV_ANIM_OFF);

	}
	return 0;
}

static void _ota_update_process_task_cb(lv_timer_t *timer)
{
	if (p_ota_view_data)
		_ota_view_paint(p_ota_view_data);
}

static int _ota_view_layout(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	ota_view_data_t *data = NULL;
	int ret = 0;

	SYS_LOG_INF("enter data size %d\n", sizeof(*data));

	data = app_mem_malloc(sizeof(*data));
	if (!data) {
		return -ENOMEM;
	}
	memset(data, 0, sizeof(*data));

	view_data->user_data = data;

	p_ota_view_data = data;

	/* create ota bg image */
	data->obj_ota_bg = lv_image_create(scr);
	if (data->obj_ota_bg == NULL) {
		SYS_LOG_ERR("obj_ota_bg null\n");
		goto fail_exit;
	}
	lv_obj_set_pos(data->obj_ota_bg, 0, 0);
	lv_obj_set_size(data->obj_ota_bg, DEF_UI_WIDTH, DEF_UI_HEIGHT);
	lv_obj_set_style_bg_color(data->obj_ota_bg, lv_color_make(0x3b, 0x3b, 0x3b), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(data->obj_ota_bg, LV_OPA_COVER, LV_PART_MAIN);

	/*set current background clickable to cut out click event send to front bg*/
	lv_obj_add_flag(data->obj_ota_bg, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_remove_flag(data->obj_ota_bg, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_GESTURE_BUBBLE);
	/* hidden left and right view to receive callback function when BMP_VOL_BG was draw */
	ui_gesture_lock_scroll();

	ret = _create_progress_bar(data, data->obj_ota_bg);
	if (ret)
		goto fail_exit;
	data->timer = lv_timer_create(_ota_update_process_task_cb, OTA_GROGRESS_UPDATE_PERIOD, data);
	if (data->timer)
		lv_timer_ready(data->timer);

	return ret;
fail_exit:
	if (data->obj_ota_bg)
		lv_obj_delete(data->obj_ota_bg);
	app_mem_free(data);
	view_data->user_data = NULL;
	p_ota_view_data = NULL;

	return ret;
}

static int _ota_view_delete(view_data_t *view_data)
{
	ota_view_data_t *data = view_data->user_data;

	if (data) {
		if (data->obj_prg_bar) {
			lv_obj_delete(data->obj_prg_bar);
			data->obj_prg_bar = NULL;
			lv_style_reset(&data->style_slider_bg);
			lv_style_reset(&data->style_slider_indic);
			lv_style_reset(&data->style_slider_knob);
		}
		if (data->timer) {
			lv_timer_delete(data->timer);
			data->timer = NULL;
		}

		if (data->obj_ota_bg) {
			lv_obj_delete(data->obj_ota_bg);
			data->obj_ota_bg = NULL;
		}

		app_mem_free(data);
		view_data->user_data = NULL;
		SYS_LOG_INF("ok\n");
	}

	ui_gesture_wait_release();
	ui_gesture_unlock_scroll();
	lv_indev_wait_release(lv_indev_get_act());

	p_ota_view_data = NULL;
	//lvgl_res_unload_scene_compact(SCENE_OTA_VIEW);
	//lvgl_res_cache_clear(0);

	return 0;
}

static int _ota_view_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void * msg_data)
{
	SYS_LOG_INF("msg_id %d\n", msg_id);
	switch (msg_id) {
	case MSG_VIEW_LAYOUT:
		return _ota_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _ota_view_delete(view_data);
	case MSG_VIEW_PAINT:
		return _ota_view_paint(view_data->user_data);
	default:
		return 0;
	}
	return 0;
}

VIEW_DEFINE2(ota_view, _ota_view_handler, NULL, \
		NULL, OTA_VIEW, HIGHEST_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);
