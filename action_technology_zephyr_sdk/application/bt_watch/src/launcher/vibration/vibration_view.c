/*
 * Copyright (c) 2022 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file gps view
 */

#include <assert.h>
#include <view_stack.h>
#ifdef CONFIG_SYS_WAKELOCK
#include <sys_wakelock.h>
#endif
#include "app_ui.h"
#include "app_defines.h"
#include "system_app.h"
#include "vibration_view.h"
#include "vibration_ui.h"
#include <widgets/text_roller.h>

enum {
	TXT_VIB_TITLE = 0,
	TXT_LOW,
	TXT_MID,
	TXT_HIGH,	
	NUM_TXTS,
};

const static uint32_t _txt_ids[] = {
	STR_VIB_TITLE,
	STR_LOW,
	STR_MID,
	STR_HIGH,
};

static int32_t vib_preload_inited = 0;

typedef struct vib_view_data {
	/* lvgl object */
	lv_obj_t *cont;
	lv_obj_t *roller;
	lv_timer_t *roller_timer;
	/* ui-editor resource */
	lvgl_res_scene_t res_scene;
	lvgl_res_string_t res_txt[NUM_TXTS];
	lv_font_t normal_font;
	lv_font_t small_font;
	uint8_t cur_level;
} vib_view_data_t;


static int _load_resource(vib_view_data_t *data)
{
	int32_t ret;

	/* load scene */
	ret = lvgl_res_load_scene(SCENE_VIB_VIEW, &data->res_scene, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (ret < 0) {
		SYS_LOG_ERR("SCENE_VIB_VIEW not found");
		return -ENOENT;
	}

	/* open font */
	if (LVGL_FONT_OPEN_DEFAULT(&data->normal_font, DEF_FONT_SIZE_NORMAL) < 0) {
		SYS_LOG_ERR("font not found");
		return -ENOENT;
	}

	/* open small font */
	if (LVGL_FONT_OPEN_DEFAULT(&data->small_font, DEF_FONT_SIZE_SMALL) < 0) {
		SYS_LOG_ERR("font not found");
		return -ENOENT;
	}

	/* load string */
	lvgl_res_load_strings_from_scene(&data->res_scene, _txt_ids, data->res_txt, NUM_TXTS);

	SYS_LOG_INF("load resource succeed");

	return 0;
}

static void _unload_resource(vib_view_data_t *data)
{
	lvgl_res_unload_strings(data->res_txt, NUM_TXTS);
	lvgl_res_unload_scene(&data->res_scene);

	LVGL_FONT_CLOSE(&data->normal_font);
	LVGL_FONT_CLOSE(&data->small_font);
}

static int _vib_view_preload(view_data_t *view_data)
{
	if (vib_preload_inited == 0) {
		lvgl_res_preload_scene_compact_default_init(SCENE_VIB_VIEW, NULL, 0,
			NULL, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
		vib_preload_inited = 1;
	}
	
	return lvgl_res_preload_scene_compact_default(SCENE_VIB_VIEW, VIB_VIEW, 0, 0);
}

static void roller_shake_timer_cb(lv_timer_t *timer)
{
	view_data_t *view_data = (view_data_t *)timer->user_data;
	vib_view_data_t *data = (vib_view_data_t *)view_data->user_data;
	const vibration_view_presenter_t *presenter = view_get_presenter(view_data);
	if(presenter)
		presenter->stop();
	if(data)
	{
		lv_timer_delete(data->roller_timer);
		data->roller_timer = NULL;
	}
}

static void roller_update_cb(lv_obj_t *roller)
{
	view_data_t *view_data = (view_data_t *)lv_obj_get_user_data(roller);
	vib_view_data_t *data = (vib_view_data_t *)view_data->user_data;
	if(data)
	{
		const vibration_view_presenter_t *presenter = view_get_presenter(view_data);
		if(presenter)
			presenter->start();
		if(data->roller_timer)
			lv_timer_resume(data->roller_timer);
		else
			data->roller_timer = lv_timer_create(roller_shake_timer_cb, 10, (void *)view_data);
	}
}

static void shake_btn_evt_handler(lv_event_t * e)
{
	view_data_t *view_data = lv_event_get_user_data(e);
	vib_view_data_t *data = (vib_view_data_t *)view_data->user_data;
	if(data)
	{
		const vibration_view_presenter_t *presenter = view_get_presenter(view_data);
		if(presenter)
		{
			data->cur_level = text_roller_get_texe_id(data->roller);
			presenter->set_level(data->cur_level);
			SYS_LOG_INF("set shake level %d",data->cur_level);
		}
	}
}

static int _vib_view_updated(view_data_t* view_data)
{
	vib_view_data_t *data = view_data->user_data;

	if(data)
	{
		const vibration_view_presenter_t *presenter = view_get_presenter(view_data);
		if (presenter) {
			data->cur_level = presenter->get_level();
			if(data->roller)
				text_roller_set_text_id(data->roller ,data->cur_level);
		}
	}
	return 0;
}

static int _vib_view_layout_creation(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	vib_view_data_t *data = view_data->user_data;

	data = app_mem_malloc(sizeof(*data));
	if (!data) {
		return -ENOMEM;
	}
	memset(data, 0, sizeof(*data));

	view_data->user_data = data;

	if (_load_resource(data)) {
		app_mem_free(data);
		view_data->user_data = NULL;
		return -ENOENT;
	}

	// create label
	data->cont = lv_obj_create(scr);
	lv_obj_set_style_pad_all(data->cont,0,LV_PART_MAIN);
	lv_obj_set_style_border_width(data->cont,0,LV_PART_MAIN);
	lv_obj_set_size(data->cont, DEF_UI_WIDTH, DEF_UI_HEIGHT);

	/* show title */
	lv_obj_t *title = lv_label_create(data->cont);
	lv_obj_set_pos(title, data->res_txt[TXT_VIB_TITLE].x, data->res_txt[TXT_VIB_TITLE].y);
	lv_label_set_text(title, data->res_txt[TXT_VIB_TITLE].txt);
	lv_obj_set_style_text_font(title, &data->normal_font, LV_PART_MAIN);
	lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

	/* text roller */
	data->roller = text_roller_create(data->cont);
	lv_obj_set_size(data->roller, 350, 400);
	lv_obj_align(data->roller,LV_ALIGN_CENTER,0,-20);
	text_roller_set_visible_row_count(data->roller,3);
	char *str[] = {
		(char *)data->res_txt[TXT_LOW].txt,
		(char *)data->res_txt[TXT_MID].txt,
		(char *)data->res_txt[TXT_HIGH].txt
	};
	text_roller_set_str(data->roller, (char **)str, 0, sizeof(str)/sizeof(str[0]));
	lv_obj_set_style_text_font(data->roller, &data->small_font, LV_PART_MAIN);
	lv_obj_set_style_text_color(data->roller, lv_color_hex(0x333333), LV_PART_MAIN);
	lv_obj_set_style_text_font(data->roller, &data->normal_font, LV_PART_SELECTED);
	lv_obj_set_style_text_color(data->roller, lv_color_hex(0xFFFFFF), LV_PART_SELECTED);
	lv_obj_set_style_bg_color(data->roller, lv_color_hex(0x1F1F1F), LV_PART_SELECTED);
	lv_obj_set_style_bg_opa(data->roller, LV_OPA_100, LV_PART_SELECTED);
	lv_obj_set_style_radius(data->roller, 40, LV_PART_SELECTED);
	lv_obj_set_style_text_line_space(data->roller, 40,LV_PART_MAIN);
	lv_obj_set_style_text_align(data->roller,LV_TEXT_ALIGN_CENTER,LV_PART_MAIN);
	lv_obj_set_user_data(data->roller, view_data);
	text_roller_set_update_cb(data->roller,roller_update_cb);

	lv_obj_t *btn = lv_button_create(data->cont);
	lv_obj_set_size(btn, 180 , 80);
	lv_obj_align(btn,LV_ALIGN_BOTTOM_MID,0,-40);
	lv_obj_set_style_radius(btn, 40, LV_PART_MAIN);
	lv_obj_set_style_bg_color(btn, lv_color_hex(0x00FF00), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(btn, LV_OPA_100, LV_PART_MAIN);
	lv_obj_add_event_cb(btn, shake_btn_evt_handler, LV_EVENT_SHORT_CLICKED, view_data);

	lv_obj_t *title_ok = lv_label_create(btn);
	lv_obj_align(title_ok,LV_ALIGN_CENTER,0,0);
	lv_label_set_text(title_ok, "OK");
	lv_obj_set_style_text_font(title_ok, &data->normal_font, LV_PART_MAIN);
	lv_obj_set_style_text_color(title_ok, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
	lv_obj_set_style_text_align(title_ok,LV_TEXT_ALIGN_CENTER,LV_PART_MAIN);

	_vib_view_updated(view_data);
	return 0;
}

static int _vib_view_layout(view_data_t *view_data)
{
	int ret;

	ret = _vib_view_layout_creation(view_data);
	if(ret < 0)
	{
		return ret;
	}

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_lock(FULL_WAKE_LOCK);
#endif
	SYS_LOG_INF("_vib_view_layout");

	return 0;
}

static int _vib_view_delete(view_data_t *view_data)
{
	vib_view_data_t *data = view_data->user_data;

	if (data) {
		lv_obj_t *scr = lv_display_get_screen_active(view_data->display);

		if(data->roller_timer)
		{
			lv_timer_delete(data->roller_timer);
			data->roller_timer = NULL;
		}
		lv_obj_clean(scr);

		_unload_resource(data);
		app_mem_free(data);
		view_data->user_data = NULL;

#ifdef CONFIG_SYS_WAKELOCK
		sys_wake_unlock(FULL_WAKE_LOCK);
#endif
	} else {
		lvgl_res_preload_cancel();
	}

	lvgl_res_unload_scene_compact(SCENE_VIB_VIEW);
	return 0;
}

int _vib_view_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void * msg_data)
{
	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return _vib_view_preload(view_data);
	case MSG_VIEW_LAYOUT:
		return _vib_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _vib_view_delete(view_data);
	case MSG_VIEW_UPDATE:
		return _vib_view_updated(view_data);		
	default:
		return 0;
	}
	return 0;
}

VIEW_DEFINE2(vib_view, _vib_view_handler, NULL, \
		NULL, VIB_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_WIDTH, DEF_UI_HEIGHT);

