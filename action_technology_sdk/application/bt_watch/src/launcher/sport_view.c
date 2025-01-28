/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file sport view
 */
#include <assert.h>
#include <app_ui.h>
#include "m_effect.h"
#include "ui_coder/ui_coder.h"

#define ENG_STR_FILE			CONFIG_APP_UI_DISK"/bt_watch.enG"

enum {
	BMP_BG = 0,
	BMP_BTN,
	BMP_BTN_H,
	NUM_BMPS,
};

enum {
	TXT_TEST = 0,
	NUM_TXTS,
};

const static uint32_t _bmp_ids[] = {
	PIC_BG, PIC_BTN, PIC_BTN_H
};

const static uint32_t _txt_ids[] = {
	STR_TEXT,
};

static int32_t sport_preload_inited = 0;
static const char *str_file = DEF_STR_FILE;

typedef struct sport_view_data {
	/* lvgl object */
	lv_obj_t *btn;
	lv_obj_t *img;
	lv_obj_t *lbl;

	/* ui-editor resource */
	resource_info_t *res_info;
	lvgl_res_scene_t res_scene;
	lv_image_dsc_t img_dsc_bmp[NUM_BMPS];
	lvgl_res_string_t res_txt[NUM_TXTS];

	/* lvgl resource */
	lv_point_t pt_bmp[NUM_BMPS];
	lv_point_t pt_txt[NUM_TXTS];
	lv_style_t style_txt[NUM_TXTS];
	lv_font_t font;
} sport_view_data_t;

static int _sport_view_reload(view_data_t *view_data);

static void _btn_evt_handler(lv_event_t * e)
{
	lv_event_code_t event = lv_event_get_code(e);
	view_data_t *view_data = lv_event_get_user_data(e);

	if (event == LV_EVENT_CLICKED) {
		SYS_LOG_INF("Clicked! Change language\n");
		if (!strcmp(str_file,DEF_STR_FILE)) {
			str_file = ENG_STR_FILE;
		} else {
			str_file = DEF_STR_FILE;
		}
		_sport_view_reload(view_data);
	} else if (event == LV_EVENT_VALUE_CHANGED) {
		SYS_LOG_INF("Toggled\n");
	}
}

static void _cvt_txt_array(lv_point_t *pt, lv_style_t *sty, lv_font_t *font, lvgl_res_string_t *txt, uint32_t num)
{
	int i;

	for (i = 0; i < num; i++) {
		pt[i].x = txt[i].x;
		pt[i].y = txt[i].y;

		lv_style_init(&sty[i]);
		lv_style_set_text_font(&sty[i], font);
		lv_style_set_text_color(&sty[i], txt[i].color);
	}
}

static int _load_resource(sport_view_data_t *data, bool first_layout)
{
	int32_t ret;

	if(first_layout)
	{
		/* load scene */
		ret = lvgl_res_load_scene(SCENE_SPORT_VIEW, &data->res_scene, DEF_STY_FILE, DEF_RES_FILE, str_file);
		if (ret < 0) {
			SYS_LOG_ERR("SCENE_SPORT_VIEW not found");
			return -ENOENT;
		}
	}

	/* load resource */
	lvgl_res_load_pictures_from_scene(&data->res_scene, _bmp_ids, data->img_dsc_bmp, data->pt_bmp, NUM_BMPS);

	/* load string */
	lvgl_res_load_strings_from_scene(&data->res_scene, _txt_ids, data->res_txt, NUM_TXTS);

	if (first_layout)
	{
		/* open font */
		if (LVGL_FONT_OPEN_DEFAULT(&data->font, DEF_FONT_SIZE_NORMAL) < 0) {
			SYS_LOG_ERR("font not found");
			return -ENOENT;
		}

		_cvt_txt_array(data->pt_txt, data->style_txt, &data->font, data->res_txt, NUM_TXTS);
	}

	SYS_LOG_INF("load resource succeed");

	return 0;
}

static void _unload_pic_str_resource(sport_view_data_t *data)
{
	lvgl_res_unload_pictures(data->img_dsc_bmp, NUM_BMPS);
	lvgl_res_unload_strings(data->res_txt, NUM_TXTS);
}

static void _unload_resource(sport_view_data_t *data)
{
	lvgl_res_unload_pictures(data->img_dsc_bmp, NUM_BMPS);
	lvgl_res_unload_strings(data->res_txt, NUM_TXTS);
	lvgl_res_unload_scene(&data->res_scene);

	LVGL_FONT_CLOSE(&data->font);
}

static int _sport_view_preload(view_data_t *view_data, bool update)
{
	if (sport_preload_inited == 0) {
		lvgl_res_preload_scene_compact_default_init(SCENE_SPORT_VIEW, NULL, 0,
			NULL, DEF_STY_FILE, DEF_RES_FILE, str_file);
		sport_preload_inited = 1;
	}

	return lvgl_res_preload_scene_compact_default(SCENE_SPORT_VIEW, SPORT_VIEW, update, 0);
}

static void _sport_event_handler(lv_event_t * e)
{
	if(lvgl_click_decision())
	{
		go_to_m_effect();
		lv_obj_remove_event_cb(lv_event_get_user_data(e),_sport_event_handler);
	}
}

static int _sport_view_layout_update(view_data_t *view_data, bool first_layout)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	sport_view_data_t *data = view_data->user_data;

	if(first_layout)
	{
		data = app_mem_malloc(sizeof(*data));
		if (!data) {
			return -ENOMEM;
		}
		memset(data, 0, sizeof(*data));

		view_data->user_data = data;
	}

	if (_load_resource(data, first_layout)) {
		app_mem_free(data);
		view_data->user_data = NULL;
		return -ENOENT;
	}

	// create image
	if (first_layout) {
		coder_simulation_tp_register(scr, SPORT_VIEW, LV_DIR_HOR, 50, NULL);
		data->img = lv_image_create(scr);
		lv_obj_set_pos(data->img, data->pt_bmp[BMP_BG].x, data->pt_bmp[BMP_BG].y);
		lv_image_set_src(data->img, &data->img_dsc_bmp[BMP_BG]);
		lv_obj_add_flag(data->img, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_event_cb(data->img, _sport_event_handler, LV_EVENT_SHORT_CLICKED, data->img);

		data->btn = lv_imagebutton_create(scr);
		lv_obj_set_pos(data->btn, data->pt_bmp[BMP_BTN].x, data->pt_bmp[BMP_BTN].y);
		lv_obj_set_size(data->btn, data->img_dsc_bmp[BMP_BTN].header.w, data->img_dsc_bmp[BMP_BTN].header.h);
		lv_obj_add_flag(data->btn, LV_OBJ_FLAG_CHECKABLE);
		lv_obj_add_event_cb(data->btn, _btn_evt_handler, LV_EVENT_ALL, view_data);

		lv_imagebutton_set_src(data->btn, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &data->img_dsc_bmp[BMP_BTN], NULL);
		lv_imagebutton_set_src(data->btn, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &data->img_dsc_bmp[BMP_BTN_H], NULL);
		lv_imagebutton_set_src(data->btn, LV_IMAGEBUTTON_STATE_CHECKED_RELEASED, NULL, &data->img_dsc_bmp[BMP_BTN_H], NULL);
		lv_imagebutton_set_src(data->btn, LV_IMAGEBUTTON_STATE_CHECKED_PRESSED, NULL, &data->img_dsc_bmp[BMP_BTN], NULL);

		data->lbl = lv_label_create(scr);
		lv_obj_set_pos(data->lbl, data->pt_txt[TXT_TEST].x, data->pt_txt[TXT_TEST].y);
		lv_obj_add_style(data->lbl, &data->style_txt[TXT_TEST], LV_PART_MAIN);
	} else {
		lv_image_cache_drop(NULL);
	}

	SYS_LOG_INF("update text: %s", data->res_txt[TXT_TEST].txt);
	lv_label_set_text(data->lbl, data->res_txt[TXT_TEST].txt);

	return 0;
}

static int _sport_view_layout(view_data_t *view_data)
{
	int ret;

	ret = _sport_view_layout_update(view_data, true);
	if(ret < 0)
	{
		return ret;
	}

	lv_refr_now(view_data->display);
	SYS_LOG_INF("_sport_view_layout");

	return 0;
}

static int _sport_view_paint(view_data_t *view_data)
{
	sport_view_data_t *data = view_data->user_data;

	if (data) {
		SYS_LOG_INF("_sport_view_paint");

		if (lv_obj_has_state(data->btn, LV_STATE_CHECKED)) {
			lv_obj_remove_state(data->btn, LV_STATE_CHECKED);
		} else {
			lv_obj_add_state(data->btn, LV_STATE_CHECKED);
		}

		lv_obj_invalidate(data->btn);
	}

	return 0;
}

static int _sport_view_delete(view_data_t *view_data)
{
	sport_view_data_t *data = view_data->user_data;
	int i;

	if (data) {
		lv_obj_delete(data->btn);
		lv_obj_delete(data->img);
		lv_obj_delete(data->lbl);

		for(i=0;i<NUM_TXTS;i++)
		{
			lv_style_reset(&data->style_txt[i]);
		}
		_unload_resource(data);
		app_mem_free(data);
		view_data->user_data = NULL;
	} else {
		lvgl_res_preload_cancel_scene(SCENE_SPORT_VIEW);
	}

	lvgl_res_unload_scene_compact(SCENE_SPORT_VIEW);
	return 0;
}

static int _sport_view_updated(view_data_t* view_data)
{
	return _sport_view_layout_update(view_data, false);
}

static int _sport_view_focus_changed(view_data_t *view_data, bool focused)
{
	sport_view_data_t *data = view_data->user_data;

	if (focused)
	{
		if(!data)
		{
			return 0;
		}

		if(!lvgl_res_scene_is_loaded(SCENE_SPORT_VIEW))
		{
			_sport_view_preload(view_data, true);
		}
	}
	else
	{
		if(data)
		{
			_unload_pic_str_resource(data);
		}

		lvgl_res_preload_cancel_scene(SCENE_SPORT_VIEW);
		lvgl_res_unload_scene_compact(SCENE_SPORT_VIEW);
	}

	return 0;
}

static int _sport_view_reload(view_data_t *view_data)
{
	sport_view_data_t *data = view_data->user_data;

	if(!data)
	{
		return 0;
	}

	// unload scene
	lvgl_res_unload_strings(data->res_txt, NUM_TXTS);
	lvgl_res_unload_scene(&data->res_scene);

	// load scene
	SYS_LOG_INF("reload str file: %s", str_file);
	lvgl_res_load_scene(SCENE_SPORT_VIEW, &data->res_scene, DEF_STY_FILE, DEF_RES_FILE, str_file);
	lvgl_res_load_strings_from_scene(&data->res_scene, _txt_ids, data->res_txt, NUM_TXTS);

	SYS_LOG_INF("update text: %s", data->res_txt[TXT_TEST].txt);
	lv_label_set_text(data->lbl, data->res_txt[TXT_TEST].txt);

	return 0;
}

int _sport_view_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void * msg_data)
{
	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return _sport_view_preload(view_data, false);
	case MSG_VIEW_LAYOUT:
		return _sport_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _sport_view_delete(view_data);
	case MSG_VIEW_FOCUS:
		return _sport_view_focus_changed(view_data, true);
	case MSG_VIEW_DEFOCUS:
		return _sport_view_focus_changed(view_data, false);
	case MSG_VIEW_UPDATE:
		return _sport_view_updated(view_data);
	case MSG_VIEW_PAINT:
		return _sport_view_paint(view_data);
	default:
		return 0;
	}
	return 0;
}

VIEW_DEFINE2(sport_view, _sport_view_handler, NULL, \
		NULL, SPORT_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);
