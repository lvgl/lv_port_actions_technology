/*
 * Copyright (c) 2022 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <app_ui.h>
#include "alipay/alipay_ui.h"
#include <view_stack.h>

enum {
	TXT_PAY_CODE = 0,
	TXT_TRANSIT_CODE,
	NUM_TXTS,
};

enum {
	BTN_PAY_CODE = 0,
	BTN_TRANSIT_CODE,
	NUM_BTNS,
};

typedef struct alipay_menu_view_data_s {
	lv_obj_t* pay_btn[NUM_BTNS];

	lv_font_t font;
	lv_style_t sty_lbl;

	lvgl_res_scene_t res_scene;
	lvgl_res_group_t res_group[NUM_BTNS];
	lv_image_dsc_t icon[NUM_BTNS];
	lv_point_t pic_pos[NUM_BTNS];
	lvgl_res_string_t text[NUM_BTNS];
} alipay_menu_view_data_t;

static int32_t alipay_pay_preload_inited = 0;

static void _menu_btn_evt_handler(lv_event_t * e);

static const uint32_t pic_grp_ids[] = {
	RES_PAYCODE_ENTRY, RES_TRANSITCODE_ENTRY,
};

static void _alipay_menu_unload_resource(alipay_menu_view_data_t *data)
{
	LVGL_FONT_CLOSE(&data->font);
	lvgl_res_unload_pictures(data->icon, ARRAY_SIZE(data->icon));
	lvgl_res_unload_strings(data->text, ARRAY_SIZE(data->text));
	for(int i = 0; i < ARRAY_SIZE(pic_grp_ids); i++) {
		lvgl_res_unload_group(&data->res_group[i]);
	}
	lvgl_res_unload_scene(&data->res_scene);
}


static int _alipay_menu_load_resource(alipay_menu_view_data_t *data)
{
	int ret;

	/* scene */
	ret = lvgl_res_load_scene(SCENE_ALIPAY_MENU_VIEW, &data->res_scene, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (ret < 0) {
		SYS_LOG_ERR("scene not found");
		return -ENOENT;
	}

	for (int i = 0; i < ARRAY_SIZE(pic_grp_ids); i++) {
		uint32_t res_id;

		ret = lvgl_res_load_group_from_scene(&data->res_scene, pic_grp_ids[i], &data->res_group[i]);
		if (ret < 0) {
			goto fail_exit;
		}
		SYS_LOG_ERR("res_grp: %d %d %d %d", data->res_group[i].x, data->res_group[i].y, 
		data->res_group[i].height, data->res_group[i].width);

		res_id = PIC_ICON;
		ret = lvgl_res_load_pictures_from_group(&data->res_group[i], &res_id, &data->icon[i], &data->pic_pos[i], 1);
		if (ret < 0) {
			lvgl_res_unload_group(&data->res_group[i]);
			goto fail_exit;
		}

		res_id = STR_TEXT;
		ret = lvgl_res_load_strings_from_group(&data->res_group[i], &res_id, &data->text[i], 1);
		if (ret < 0) {
			lvgl_res_unload_group(&data->res_group[i]);
			goto fail_exit;
		}

	}

	/* open font */
	if (LVGL_FONT_OPEN_DEFAULT(&data->font, DEF_FONT_SIZE_NORMAL) < 0) {
		goto fail_exit;
	}

	lvgl_res_unload_scene(&data->res_scene);
	return 0;
fail_exit:
	_alipay_menu_unload_resource(data);
	lvgl_res_unload_scene(&data->res_scene);
	return -ENOENT;
}

static void _create_ui_menu(view_data_t *view_data)
{
	//const alipay_view_presenter_t *presenter = view_get_presenter(view_data);
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	alipay_menu_view_data_t *data = view_data->user_data;
	uint32_t buf_len = 128;
	char *str_buf;

	// malloc string buffer
	str_buf = app_mem_malloc(buf_len);
	if (!str_buf) {
		return;
	}

	// set bg color
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);
	// init the style for label
	lv_style_reset(&data->sty_lbl);
	lv_style_set_text_font(&data->sty_lbl, &data->font);
	lv_style_set_text_color(&data->sty_lbl, lv_color_white());
	// create btn
	int i;
	for(i=0; i<NUM_BTNS; i++) {
		data->pay_btn[i] = lv_button_create(scr);
		lv_obj_set_style_bg_color(data->pay_btn[i], lv_color_make(0x3B, 0x3B, 0x3B), LV_PART_MAIN);
		lv_obj_set_style_radius(data->pay_btn[i], 20, LV_PART_MAIN);
		lv_obj_set_size(data->pay_btn[i], data->res_group[i].width, data->res_group[i].height);
		lv_obj_set_style_bg_opa(data->pay_btn[i], LV_OPA_COVER, LV_PART_MAIN);
		lv_obj_set_pos(data->pay_btn[i], data->res_group[i].x, data->res_group[i].y);
		
		lv_obj_add_event_cb(data->pay_btn[i], _menu_btn_evt_handler, LV_EVENT_SHORT_CLICKED, view_data);

		lv_obj_t *obj_icon = lv_image_create(data->pay_btn[i]);
		lv_image_set_src(obj_icon, &data->icon[i]);
		lv_obj_set_pos(obj_icon, data->pic_pos[i].x, data->pic_pos[i].y);

		lv_obj_t *label = lv_label_create(data->pay_btn[i]);
		lv_obj_align(label, LV_ALIGN_LEFT_MID, 107, 0);
		lv_obj_remove_flag(label, LV_OBJ_FLAG_CLICKABLE);
		lv_label_set_text(label, data->text[i].txt);
		lv_obj_add_style(label, &data->sty_lbl, LV_PART_MAIN);
	}
	// free string buffer
	app_mem_free(str_buf);
}

static void _delete_ui_menu(view_data_t *view_data)
{
	alipay_menu_view_data_t *data = view_data->user_data;

	int i;
	for (i=0; i<NUM_BTNS; i++) {
		if (data->pay_btn[i]) {
			lv_obj_delete(data->pay_btn[i]);
			data->pay_btn[i] = NULL;
		}
	}
}

static void _menu_btn_evt_handler(lv_event_t * e)
{
	view_data_t *view_data = lv_event_get_user_data(e);
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);
	alipay_menu_view_data_t *data = view_data->user_data;
	lv_obj_t *obj = lv_event_get_target(e);
	int ret;
	
	SYS_LOG_INF("obj:%p", obj);
	if (obj == data->pay_btn[BTN_PAY_CODE]) {
		SYS_LOG_INF("BTN_PAY_CODE");
		view_stack_push_view(ALIPAY_PAY_VIEW, &alipay_view_presenter);
	} else if (obj == data->pay_btn[BTN_TRANSIT_CODE]) {
		SYS_LOG_INF("BTN_TRANSIT_CODE");
		ret = presenter->load_card(-1);
		if (!ret) {
			view_stack_push_view(ALIPAY_TRANSITCODE_VIEW, &alipay_view_presenter);
		} else {
			view_stack_push_view(ALIPAY_CARDLIST_VIEW, &alipay_view_presenter);
		}
	}
}

static int _alipay_menu_view_preload(view_data_t *view_data, bool update)
{
	if (alipay_pay_preload_inited == 0) {
		lvgl_res_preload_scene_compact_default_init(SCENE_ALIPAY_MENU_VIEW, NULL, 0,
			NULL, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
		alipay_pay_preload_inited = 1;
	}

	return lvgl_res_preload_scene_compact_default(SCENE_ALIPAY_MENU_VIEW, ALIPAY_MENU_VIEW, update, 0);
}

static int _alipay_menu_view_layout_update(view_data_t *view_data, bool first_layout)
{
	alipay_menu_view_data_t *data = view_data->user_data;
	int ret;

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
		ret = lvgl_res_load_scene(SCENE_ALIPAY_MENU_VIEW, &data->res_scene, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
		if (ret < 0) {
			SYS_LOG_ERR("SCENE_ALIPAY_MENU_VIEW not found");
			return -ENOENT;
		}
		_alipay_menu_load_resource(data);
		lv_style_init(&data->sty_lbl);
	}

	// rebuild pay view
	if (first_layout) {
		_create_ui_menu(view_data);
	}

	return 0;
}

static int _alipay_menu_view_layout(view_data_t *view_data)
{
	int ret;

	ret = _alipay_menu_view_layout_update(view_data, true);
	if (ret < 0) {
		return ret;
	}

	lv_refr_now(view_data->display);
	return 0;
}

static int _alipay_menu_view_delete(view_data_t *view_data)
{
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);
	alipay_menu_view_data_t *data = view_data->user_data;

	// stop adv
	presenter->stop_adv();

	// unlock screen
	presenter->unlock_screen();

	if (data) {
		lv_style_reset(&data->sty_lbl);
		_delete_ui_menu(view_data);
		_alipay_menu_unload_resource(data);
		app_mem_free(data);
		view_data->user_data = NULL;
	} else {
		lvgl_res_preload_cancel_scene(SCENE_ALIPAY_MENU_VIEW);
	}
	
	lvgl_res_unload_scene_compact(SCENE_ALIPAY_MENU_VIEW);

	return 0;
}

static int _alipay_menu_view_updated(view_data_t* view_data)
{
	return _alipay_menu_view_layout_update(view_data, false);
}

static int _alipay_menu_view_focus_changed(view_data_t *view_data, bool focused)
{
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);
	alipay_menu_view_data_t *data = view_data->user_data;

	if (focused) {
		if(data && !lvgl_res_scene_is_loaded(SCENE_ALIPAY_MENU_VIEW)) {
			_alipay_menu_view_preload(view_data, true);
		}
	} else {
		lvgl_res_preload_cancel_scene(SCENE_ALIPAY_MENU_VIEW);
		lvgl_res_unload_scene_compact(SCENE_ALIPAY_MENU_VIEW);
	}

	if (focused) {
		// lock screen
		presenter->lock_screen();

		// start adv
		presenter->start_adv();
	} else {
		// stop adv
		presenter->stop_adv();

		// unlock screen
		presenter->unlock_screen();
	}

	return 0;
}

int _alipay_menu_view_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void *msg_data)
{
	assert(view_id == ALIPAY_MENU_VIEW);
	
	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return _alipay_menu_view_preload(view_data, false);
	case MSG_VIEW_LAYOUT:
		return _alipay_menu_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _alipay_menu_view_delete(view_data);
	case MSG_VIEW_FOCUS:
		return _alipay_menu_view_focus_changed(view_data, true);
	case MSG_VIEW_DEFOCUS:
		return _alipay_menu_view_focus_changed(view_data, false);
	case MSG_VIEW_UPDATE:
		return _alipay_menu_view_updated(view_data);
	default:
		return 0;
	}
}

VIEW_DEFINE2(alipay_menu_view, _alipay_menu_view_handler, NULL,
		NULL, ALIPAY_MENU_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);

