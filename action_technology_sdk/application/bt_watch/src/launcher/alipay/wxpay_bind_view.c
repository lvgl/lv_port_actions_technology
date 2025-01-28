/*
 * Copyright (c) 2022 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <app_ui.h>
#include "alipay/alipay_ui.h"

#define TIMER_PERIOD		(1000)
#define TIMEOUT_BINDING		(60*1000)
#define TIMEOUT_FAILED		(8*1000)
#define TIMEOUT_OK			(3*1000)

enum {
	TXT_BIND_TITLE = 0,
	TXT_BIND_START,
	TXT_BIND_FAILED,
	TXT_BIND_OK,
	NUM_TXTS,
};

const static uint32_t _txt_ids[] = {
	STR_BIND_TITLE,
	STR_BIND_START,
	STR_BIND_FAILED,
	STR_BIND_OK,
};

typedef struct wxpay_bind_view_data_s {
	lv_obj_t* bind_title;
	lv_obj_t* bind_qrcode;
	lv_obj_t* bind_info;

	lv_font_t font;
	lv_style_t sty_lbl;
	lv_timer_t *timer;

	lvgl_res_scene_t res_scene;
	lvgl_res_string_t res_txt[NUM_TXTS];

	/* user data*/
	int bind_status;
	int status_elaps;
} wxpay_bind_view_data_t;

static int32_t wxpay_bind_preload_inited = 0;

static void _status_timer_cb(lv_timer_t *timer);
static void _bind_result_timer_cb(lv_timer_t *timer);

static void _create_ui_bind(view_data_t *view_data)
{
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	wxpay_bind_view_data_t *data = view_data->user_data;
	uint32_t buf_len = 128;
	char *str_buf;

	// malloc string buffer
	str_buf = app_mem_malloc(buf_len);
	if (!str_buf) {
		return;
	}

	// get binding string
	presenter->get_binding_string(str_buf, &buf_len);

	// set bg color
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);

	// init the style for label
	lv_style_reset(&data->sty_lbl);
	lv_style_set_text_font(&data->sty_lbl, &data->font);
	lv_style_set_text_color(&data->sty_lbl, lv_color_white());

	// create qrcode
	data->bind_qrcode = lv_qrcode_create(scr);
	lv_qrcode_set_size(data->bind_qrcode, 220);
	lv_qrcode_set_dark_color(data->bind_qrcode, lv_color_black());
	lv_qrcode_set_light_color(data->bind_qrcode, lv_color_white());
	lv_qrcode_update(data->bind_qrcode, str_buf, buf_len, qrcodegen_Ecc_QUARTILE);
	lv_obj_center(data->bind_qrcode);
	lv_obj_set_style_border_color(data->bind_qrcode, lv_color_white(), 0);
	lv_obj_set_style_border_width(data->bind_qrcode, 8, 0);

	// create title
	data->bind_title = lv_label_create(scr);
	lv_label_set_text(data->bind_title, data->res_txt[TXT_BIND_TITLE].txt);
	lv_obj_add_style(data->bind_title, &data->sty_lbl, LV_PART_MAIN);
	lv_obj_align_to(data->bind_title, data->bind_qrcode, LV_ALIGN_OUT_TOP_MID, 0, -5);

	// free string buffer
	app_mem_free(str_buf);
}

static void _delete_ui_bind(view_data_t *view_data)
{
	wxpay_bind_view_data_t *data = view_data->user_data;

	if (data->bind_qrcode) {
		lv_obj_delete(data->bind_qrcode);
		data->bind_qrcode = NULL;
	}
	if (data->bind_title) {
		lv_obj_delete(data->bind_title);
		data->bind_title = NULL;
	}
	if (data->bind_info) {
		lv_obj_delete(data->bind_info);
		data->bind_info = NULL;
	}
}

static void _show_bind_result(view_data_t *view_data, char *info, int timeout)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	wxpay_bind_view_data_t *data = view_data->user_data;

	// hide qrcode
	if (data->bind_qrcode) {
		lv_obj_add_flag(data->bind_qrcode, LV_OBJ_FLAG_HIDDEN);
	}
	if (data->bind_title) {
		lv_obj_add_flag(data->bind_title, LV_OBJ_FLAG_HIDDEN);
	}

	if (data->bind_info == NULL) {
		// black bg color
		lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
		lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);

		// init the style for label
		lv_style_reset(&data->sty_lbl);
		lv_style_set_text_font(&data->sty_lbl, &data->font);
		lv_style_set_text_color(&data->sty_lbl, lv_color_white());

		data->bind_info = lv_label_create(scr);
		lv_obj_add_style(data->bind_info, &data->sty_lbl, LV_PART_MAIN);
		lv_obj_center(data->bind_info);
	}
	lv_label_set_text(data->bind_info, info);

	// set timeout timer
	if (timeout > 0) {
		lv_timer_set_period(data->timer, timeout);
		lv_timer_set_cb(data->timer, _bind_result_timer_cb);
		lv_timer_reset(data->timer);
	}
}

static void _rebuild_ui_bind(view_data_t *view_data)
{
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);
	wxpay_bind_view_data_t *data = view_data->user_data;

	// binding env init
	presenter->bind_init();

	// rebuild bind ui
	_delete_ui_bind(view_data);
	_create_ui_bind(view_data);
	
	// update status
	data->bind_status = presenter->get_binding_status();
	data->status_elaps = 0;
}

static void _status_timer_cb(lv_timer_t *timer)
{
	view_data_t *view_data = timer->user_data;
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);
	wxpay_bind_view_data_t *data = view_data->user_data;
	int bind_status = presenter->get_binding_status();

	// update status counter
	data->status_elaps += TIMER_PERIOD;
	if (data->bind_status != bind_status) {
		data->status_elaps = 0;
	}

	switch(bind_status) {
		case STATE_UNBINDED:
		case STATE_WAIT_SCAN:
			break;
		case STATE_BINDING:
			if (data->status_elaps == 0) {
				_show_bind_result(view_data, data->res_txt[TXT_BIND_START].txt, 0);
			} else if (data->status_elaps >= TIMEOUT_BINDING) {
				// binding env deinit
				presenter->bind_deinit();

				// rebuild bind view
				_rebuild_ui_bind(view_data);
			}
			break;
		case STATE_BINDING_FAIL:
			_show_bind_result(view_data, data->res_txt[TXT_BIND_FAILED].txt, TIMEOUT_FAILED);
			break;
		case STATE_BINDING_OK:
			_show_bind_result(view_data, data->res_txt[TXT_BIND_OK].txt, TIMEOUT_OK);
			break;
	}

	data->bind_status = bind_status;
}

static void _bind_result_timer_cb(lv_timer_t *timer)
{
	view_data_t *view_data = timer->user_data;
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);
	wxpay_bind_view_data_t *data = view_data->user_data;

	// binding env deinit
	presenter->bind_deinit();

	// rebuild ui
	if (data->bind_status == STATE_BINDING_OK) {
		// jump to pay view
		wxpay_ui_update();
	} else {
		// rebuild bind view
		_rebuild_ui_bind(view_data);
	}

	// set status timer
	lv_timer_set_period(data->timer, TIMER_PERIOD);
	lv_timer_set_cb(data->timer, _status_timer_cb);
	lv_timer_reset(data->timer);
	data->status_elaps = 0;
}

static int _wxpay_bind_view_preload(view_data_t *view_data, bool update)
{
	if (wxpay_bind_preload_inited == 0) {
		lvgl_res_preload_scene_compact_default_init(SCENE_WXPAY_BIND_VIEW, NULL, 0,
			NULL, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
		wxpay_bind_preload_inited = 1;
	}

	return lvgl_res_preload_scene_compact_default(SCENE_WXPAY_BIND_VIEW, WXPAY_BIND_VIEW, update, 0);
}

static int _wxpay_bind_view_layout_update(view_data_t *view_data, bool first_layout)
{
	wxpay_bind_view_data_t *data = view_data->user_data;
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
		ret = lvgl_res_load_scene(SCENE_WXPAY_BIND_VIEW, &data->res_scene, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
		if (ret < 0) {
			SYS_LOG_ERR("SCENE_WXPAY_BIND_VIEW not found");
			return -ENOENT;
		}
		lvgl_res_load_strings_from_scene(&data->res_scene, _txt_ids, data->res_txt, NUM_TXTS);

		LVGL_FONT_OPEN_DEFAULT(&data->font, DEF_FONT_SIZE_NORMAL);

		lv_style_init(&data->sty_lbl);

		data->timer = lv_timer_create(_status_timer_cb, TIMER_PERIOD, view_data);
	}

	// rebuild bind view
	_rebuild_ui_bind(view_data);

	return 0;
}

static int _wxpay_bind_view_layout(view_data_t *view_data)
{
	int ret;

	ret = _wxpay_bind_view_layout_update(view_data, true);
	if (ret < 0) {
		return ret;
	}

	lv_refr_now(view_data->display);
	return 0;
}

static int _wxpay_bind_view_delete(view_data_t *view_data)
{
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);
	wxpay_bind_view_data_t *data = view_data->user_data;

	// stop adv
	presenter->stop_adv();

	// unlock screen
	presenter->unlock_screen();

	if (data) {
		lv_style_reset(&data->sty_lbl);

		if (data->timer) {
			lv_timer_delete(data->timer);
			data->timer = NULL;
		}

		_delete_ui_bind(view_data);
		lvgl_res_unload_strings(data->res_txt, NUM_TXTS);
		lvgl_res_unload_scene(&data->res_scene);

		LVGL_FONT_CLOSE(&data->font);

		app_mem_free(data);
		view_data->user_data = NULL;
	} else {
		lvgl_res_preload_cancel_scene(SCENE_WXPAY_BIND_VIEW);
	}

	lvgl_res_unload_scene_compact(SCENE_WXPAY_BIND_VIEW);

	return 0;
}

static int _wxpay_bind_view_updated(view_data_t* view_data)
{
	return _wxpay_bind_view_layout_update(view_data, false);
}

static int _wxpay_bind_view_focus_changed(view_data_t *view_data, bool focused)
{
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);
	wxpay_bind_view_data_t *data = view_data->user_data;

	if (focused) {
		if(data && !lvgl_res_scene_is_loaded(SCENE_WXPAY_BIND_VIEW)) {
			_wxpay_bind_view_preload(view_data, true);
		}
	} else {
		//if(data) {
			//_unload_pic_resource(data);
		//}

		lvgl_res_preload_cancel_scene(SCENE_WXPAY_BIND_VIEW);
		lvgl_res_unload_scene_compact(SCENE_WXPAY_BIND_VIEW);
	}

	if (focused) {
		// lock screen
		presenter->lock_screen();

		// start adv
		presenter->start_adv();

		if (data) {
			lv_timer_resume(data->timer);
		}
	} else {
		// stop adv
		presenter->stop_adv();

		// unlock screen
		presenter->unlock_screen();

		if (data) {
			lv_timer_pause(data->timer);
		}
	}

	return 0;
}

int _wxpay_bind_view_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void *msg_data)
{
	assert(view_id == WXPAY_BIND_VIEW);

	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return _wxpay_bind_view_preload(view_data, false);
	case MSG_VIEW_LAYOUT:
		return _wxpay_bind_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _wxpay_bind_view_delete(view_data);
	case MSG_VIEW_FOCUS:
		return _wxpay_bind_view_focus_changed(view_data, true);
	case MSG_VIEW_DEFOCUS:
		return _wxpay_bind_view_focus_changed(view_data, false);
	case MSG_VIEW_UPDATE:
		return _wxpay_bind_view_updated(view_data);
	default:
		return 0;
	}
}

VIEW_DEFINE2(wxpay_bind_view, _wxpay_bind_view_handler, NULL,
		NULL, WXPAY_BIND_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);

