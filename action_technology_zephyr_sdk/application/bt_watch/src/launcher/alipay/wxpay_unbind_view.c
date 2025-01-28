/*
 * Copyright (c) 2022 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <app_ui.h>
#include "alipay/alipay_ui.h"

enum {
	TXT_UNBIND = 0,
	NUM_TXTS,
};

const static uint32_t _txt_ids[] = {
	STR_UNBIND,
};

typedef struct wxpay_unbind_view_data_s {
	lv_obj_t *unbind_obj;
	lv_obj_t *unbind_id;
	lv_obj_t *unbind_name;
	lv_obj_t *unbind_btn;
	lv_obj_t *unbind_lbl;

	lv_font_t font;
	lv_style_t sty_btn[2];
	lv_style_t sty_lbl[2];

	lvgl_res_scene_t res_scene;
	lvgl_res_string_t res_txt[NUM_TXTS];

} wxpay_unbind_view_data_t;

static int32_t wxpay_unbind_preload_inited = 0;

static void _unbind_btn_evt_handler(lv_event_t * e);

static void _create_ui_unbind(view_data_t *view_data)
{
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	wxpay_unbind_view_data_t *data = view_data->user_data;
	uint32_t name_len = 128;
	char *name_buf;
	uint32_t id_len = 128;
	char *id_buf;

	// malloc string buffer
	name_buf = app_mem_malloc(name_len);
	if (!name_buf) {
		return;
	}
	id_buf = app_mem_malloc(id_len);
	if (!id_buf) {
		return;
	}

	// get userinfo
	presenter->get_userinfo(name_buf, &name_len, id_buf, &id_len);

	// black bg color
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);

	// init the style for label
	lv_style_reset(&data->sty_lbl[0]);
	lv_style_set_text_font(&data->sty_lbl[0], &data->font);
	lv_style_set_text_color(&data->sty_lbl[0], lv_color_white());

	// init the style for the default state
	lv_style_reset(&data->sty_btn[0]);
	lv_style_set_text_font(&data->sty_btn[0], &data->font);
	lv_style_set_radius(&data->sty_btn[0], 50);
	lv_style_set_bg_opa(&data->sty_btn[0], LV_OPA_100);
	lv_style_set_bg_color(&data->sty_btn[0], lv_palette_darken(LV_PALETTE_BLUE, 2));
	lv_style_set_text_color(&data->sty_btn[0], lv_color_white());

	// init the pressed style
	lv_style_reset(&data->sty_btn[1]);
	lv_style_set_bg_color(&data->sty_btn[1], lv_palette_main(LV_PALETTE_BLUE));

	// create list
	data->unbind_obj = lv_obj_create(scr);
	lv_obj_set_width(data->unbind_obj, DEF_UI_VIEW_WIDTH*2/3);
	lv_obj_set_height(data->unbind_obj, LV_SIZE_CONTENT);
	lv_obj_center(data->unbind_obj);
	lv_obj_align(data->unbind_obj, LV_ALIGN_CENTER, 0, 0);

	// create logon id
	data->unbind_id = lv_label_create(data->unbind_obj);
	lv_label_set_text(data->unbind_id, id_buf);
	lv_obj_add_style(data->unbind_id, &data->sty_lbl[0], LV_PART_MAIN);
	lv_obj_align(data->unbind_id, LV_ALIGN_TOP_MID, 0, 0);

	// create nick name
	data->unbind_name = lv_label_create(data->unbind_obj);
	lv_label_set_text(data->unbind_name, name_buf);
	lv_obj_center(data->unbind_name);
	lv_obj_add_style(data->unbind_name, &data->sty_lbl[0], LV_PART_MAIN);
	lv_obj_align_to(data->unbind_name, data->unbind_id, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);

	// create unbind btn
	data->unbind_btn = lv_button_create(data->unbind_obj);
	lv_obj_add_style(data->unbind_btn, &data->sty_btn[0], 0);
	lv_obj_add_style(data->unbind_btn, &data->sty_btn[1], LV_STATE_PRESSED);
	lv_obj_set_size(data->unbind_btn, 200, 60);
	lv_obj_align_to(data->unbind_btn, data->unbind_name, LV_ALIGN_OUT_BOTTOM_MID, 0, 80);
	lv_obj_add_event_cb(data->unbind_btn, _unbind_btn_evt_handler, LV_EVENT_SHORT_CLICKED, view_data);

	data->unbind_lbl = lv_label_create(data->unbind_btn);
	lv_label_set_text(data->unbind_lbl, data->res_txt[TXT_UNBIND].txt);
	lv_obj_center(data->unbind_lbl);

	// free string buffer
	app_mem_free(name_buf);
	app_mem_free(id_buf);
}

static void _delete_ui_unbind(view_data_t *view_data)
{
	wxpay_unbind_view_data_t *data = view_data->user_data;

	if (data->unbind_id) {
		lv_obj_delete(data->unbind_id);
		data->unbind_id = NULL;
	}
	if (data->unbind_name) {
		lv_obj_delete(data->unbind_name);
		data->unbind_name = NULL;
	}
	if (data->unbind_lbl) {
		lv_obj_delete(data->unbind_lbl);
		data->unbind_lbl = NULL;
	}
	if (data->unbind_btn) {
		lv_obj_delete(data->unbind_btn);
		data->unbind_btn = NULL;
	}
	if (data->unbind_obj) {
		lv_obj_delete(data->unbind_obj);
		data->unbind_obj = NULL;
	}
}

static void _rebuild_ui_unbind(view_data_t *view_data)
{
	// rebuild unbind ui
	_delete_ui_unbind(view_data);
	_create_ui_unbind(view_data);
}

static void _unbind_btn_evt_handler(lv_event_t * e)
{
	view_data_t *view_data = lv_event_get_user_data(e);
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);

	SYS_LOG_INF("click unbind");

	// unbind
	presenter->do_unbind();

	// jump to bind view
	wxpay_ui_update();
}

static int _wxpay_unbind_view_preload(view_data_t *view_data, bool update)
{
	if (wxpay_unbind_preload_inited == 0) {
		lvgl_res_preload_scene_compact_default_init(SCENE_WXPAY_UNBIND_VIEW, NULL, 0,
			NULL, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
		wxpay_unbind_preload_inited = 1;
	}

	return lvgl_res_preload_scene_compact_default(SCENE_WXPAY_UNBIND_VIEW, WXPAY_UNBIND_VIEW, update, 0);
}

static int _wxpay_unbind_view_layout_update(view_data_t *view_data, bool first_layout)
{
	wxpay_unbind_view_data_t *data = view_data->user_data;
	int idx, ret;

	if(data == NULL) {
		data = app_mem_malloc(sizeof(*data));
		if (!data) {
			return -ENOMEM;
		}
		memset(data, 0, sizeof(*data));
		view_data->user_data = data;
	}

	if(first_layout) {
		// load scene
		ret = lvgl_res_load_scene(SCENE_WXPAY_UNBIND_VIEW, &data->res_scene, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
		if (ret < 0) {
			SYS_LOG_ERR("SCENE_WXPAY_UNBIND_VIEW not found");
			return -ENOENT;
		}
		lvgl_res_load_strings_from_scene(&data->res_scene, _txt_ids, data->res_txt, NUM_TXTS);

		LVGL_FONT_OPEN_DEFAULT(&data->font, DEF_FONT_SIZE_NORMAL);

		for (idx = 0; idx < 2; idx++) {
			lv_style_init(&data->sty_btn[idx]);
			lv_style_init(&data->sty_lbl[idx]);
		}
	}

	// rebuild unbind view
	_rebuild_ui_unbind(view_data);

	return 0;
}

static int _wxpay_unbind_view_layout(view_data_t *view_data)
{
	int ret;

	ret = _wxpay_unbind_view_layout_update(view_data, true);
	if (ret < 0) {
		return ret;
	}

	lv_refr_now(view_data->display);
	return 0;
}

static int _wxpay_unbind_view_delete(view_data_t *view_data)
{
	wxpay_unbind_view_data_t *data = view_data->user_data;
	int idx;

	if (data) {
		for (idx = 0; idx < 2; idx++) {
			lv_style_reset(&data->sty_btn[idx]);
			lv_style_reset(&data->sty_lbl[idx]);
		}

		_delete_ui_unbind(view_data);
		lvgl_res_unload_strings(data->res_txt, NUM_TXTS);
		lvgl_res_unload_scene(&data->res_scene);

		LVGL_FONT_CLOSE(&data->font);
		app_mem_free(data);
		view_data->user_data = NULL;
	} else {
		lvgl_res_preload_cancel_scene(SCENE_WXPAY_UNBIND_VIEW);
	}
	
	lvgl_res_unload_scene_compact(SCENE_WXPAY_UNBIND_VIEW);

	return 0;
}

static int _wxpay_unbind_view_updated(view_data_t* view_data)
{
	return _wxpay_unbind_view_layout_update(view_data, false);
}

static int _wxpay_unbind_view_focus_changed(view_data_t *view_data, bool focused)
{
	wxpay_unbind_view_data_t *data = view_data->user_data;

	if (focused) {
		if(data && !lvgl_res_scene_is_loaded(SCENE_WXPAY_UNBIND_VIEW)) {
			_wxpay_unbind_view_preload(view_data, true);
		}
	} else {
		//if(data) {
			//_unload_pic_resource(data);
		//}

		lvgl_res_preload_cancel_scene(SCENE_WXPAY_UNBIND_VIEW);
		lvgl_res_unload_scene_compact(SCENE_WXPAY_UNBIND_VIEW);
	}

	return 0;
}

int _wxpay_unbind_view_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void *msg_data)
{
	assert(view_id == WXPAY_UNBIND_VIEW);

	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return _wxpay_unbind_view_preload(view_data, false);
	case MSG_VIEW_LAYOUT:
		return _wxpay_unbind_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _wxpay_unbind_view_delete(view_data);
	case MSG_VIEW_FOCUS:
		return _wxpay_unbind_view_focus_changed(view_data, true);
	case MSG_VIEW_DEFOCUS:
		return _wxpay_unbind_view_focus_changed(view_data, false);
	case MSG_VIEW_UPDATE:
		return _wxpay_unbind_view_updated(view_data);
	default:
		return 0;
	}
}

VIEW_DEFINE2(wxpay_unbind_view, _wxpay_unbind_view_handler, NULL,
		NULL, WXPAY_UNBIND_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);

