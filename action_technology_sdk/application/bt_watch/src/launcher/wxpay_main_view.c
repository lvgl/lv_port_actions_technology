/*
 * Copyright (c) 2022 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <app_ui.h>
#include "alipay/alipay_ui.h"
#include "m_effect.h"
#include "ui_coder/ui_coder.h"

#define TIMER_PERIOD		(1000)
#define TIMEOUT_BINDING		(60*1000)
#define TIMEOUT_PAY			(60*1000)
#define TIMEOUT_FAILED		(8*1000)
#define TIMEOUT_OK			(3*1000)

enum {
	TXT_BIND_TITLE = 0,
	TXT_BIND_START,
	TXT_BIND_FAILED,
	TXT_BIND_OK,
	TXT_PAY_TITLE,
	TXT_PAY_SW_QR,
	TXT_PAY_SW_BAR,
	NUM_TXTS,
};

const static uint32_t _txt_ids[] = {
	STR_BIND_TITLE,
	STR_BIND_START,
	STR_BIND_FAILED,
	STR_BIND_OK,
	STR_PAY_TITLE,
	STR_PAY_SWITCH_QR,
	STR_PAY_SWITCH_BAR,
};

typedef struct wxpay_main_view_data_s {
	lv_obj_t* bind_title;
	lv_obj_t* bind_qrcode;
	lv_obj_t* bind_info;

	lv_obj_t* pay_title;
	lv_obj_t* pay_qrcode;
	lv_obj_t* pay_barcode;
	lv_obj_t* pay_btn;
	lv_obj_t* pay_code;

	lv_font_t font;
	lv_style_t sty_lbl;
	lv_style_t sty_lbl_pr;
	lv_timer_t *timer;

	lvgl_res_scene_t res_scene;
	lvgl_res_string_t res_txt[NUM_TXTS];

	/* user data*/
	int bind_status;
	int status_elaps;
} wxpay_main_view_data_t;

static int32_t wxpay_main_preload_inited = 0;

static void _status_timer_cb(lv_timer_t *timer);
static void _bind_result_timer_cb(lv_timer_t *timer);
static void _pay_btn_evt_handler(lv_event_t * e);

static int _load_txt_resource(wxpay_main_view_data_t *data)
{
	// load scene
	int ret = lvgl_res_load_scene(SCENE_WXPAY_MAIN_VIEW, &data->res_scene, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (ret < 0) {
		SYS_LOG_ERR("SCENE_WXPAY_MAIN_VIEW not found");
		return -ENOENT;
	}	
	lvgl_res_load_strings_from_scene(&data->res_scene, _txt_ids, data->res_txt, NUM_TXTS);
	return 0;
}

static void _unload_txt_resource(wxpay_main_view_data_t *data)
{
	lvgl_res_unload_strings(data->res_txt, NUM_TXTS);
	lvgl_res_unload_scene(&data->res_scene);

}

static void _create_ui_bind(view_data_t *view_data)
{
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	wxpay_main_view_data_t *data = view_data->user_data;
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
	wxpay_main_view_data_t *data = view_data->user_data;

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
	wxpay_main_view_data_t *data = view_data->user_data;

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

static void _create_ui_pay(view_data_t *view_data)
{
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	wxpay_main_view_data_t *data = view_data->user_data;
	bool is_barcode = presenter->is_barcode_on();
	uint32_t buf_len = 128;
	char *str_buf;

	// malloc string buffer
	str_buf = app_mem_malloc(buf_len);
	if (!str_buf) {
		return;
	}

	// get paycode string
	presenter->get_paycode_string(str_buf, &buf_len);

	// set bg color
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);

	// init the style for label
	lv_style_reset(&data->sty_lbl);
	lv_style_set_text_font(&data->sty_lbl, &data->font);
	lv_style_set_text_color(&data->sty_lbl, lv_color_white());

	// init the style for btn
	lv_style_reset(&data->sty_lbl_pr);
	lv_style_set_text_font(&data->sty_lbl_pr, &data->font);
	lv_style_set_text_color(&data->sty_lbl_pr, lv_palette_main(LV_PALETTE_BLUE));

	if (is_barcode) {
		// create barcode
		data->pay_barcode = lv_barcode_create(scr);
		if(data->pay_barcode) {
			lv_obj_set_height(data->pay_barcode, 100);
			lv_barcode_set_dark_color(data->pay_barcode, lv_color_black());
			lv_barcode_set_light_color(data->pay_barcode, lv_color_white());
			lv_barcode_update(data->pay_barcode, str_buf);
			lv_obj_center(data->pay_barcode);
			lv_obj_set_style_outline_color(data->pay_barcode, lv_color_white(), 0);
			lv_obj_set_style_outline_width(data->pay_barcode, 8, 0);
		}
	} else {
		// create qrcode
		data->pay_qrcode = lv_qrcode_create(scr);
		if(data->pay_qrcode) {
			lv_qrcode_set_size(data->pay_qrcode, 220);
			lv_qrcode_set_dark_color(data->pay_qrcode, lv_color_black());
			lv_qrcode_set_light_color(data->pay_qrcode, lv_color_white());
			lv_qrcode_update(data->pay_qrcode, str_buf, buf_len, qrcodegen_Ecc_QUARTILE);
			lv_obj_center(data->pay_qrcode);
			lv_obj_set_style_border_color(data->pay_qrcode, lv_color_white(), 0);
			lv_obj_set_style_border_width(data->pay_qrcode, 8, 0);
		}
	}

	// create title
	data->pay_title = lv_label_create(scr);
	lv_label_set_text(data->pay_title, data->res_txt[TXT_PAY_TITLE].txt);
	lv_obj_add_style(data->pay_title, &data->sty_lbl, LV_PART_MAIN);
	if (is_barcode) {
		lv_obj_align_to(data->pay_title, data->pay_barcode, LV_ALIGN_OUT_TOP_MID, 0, -5);
	} else {
		lv_obj_align_to(data->pay_title, data->pay_qrcode, LV_ALIGN_OUT_TOP_MID, 0, -5);
	}

	// create paycode
	if (is_barcode) {
		data->pay_code = lv_label_create(scr);
		lv_label_set_text(data->pay_code, str_buf);
		lv_obj_add_style(data->pay_code, &data->sty_lbl, LV_PART_MAIN);
		lv_obj_align_to(data->pay_code, data->pay_barcode, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
	}

	// create btn
	data->pay_btn = lv_label_create(scr);
	lv_obj_add_style(data->pay_btn, &data->sty_lbl_pr, LV_PART_MAIN);
	if (is_barcode) {
		lv_label_set_text(data->pay_btn, data->res_txt[TXT_PAY_SW_QR].txt);
		lv_obj_align_to(data->pay_btn, data->pay_code, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
	} else {
		lv_label_set_text(data->pay_btn, data->res_txt[TXT_PAY_SW_BAR].txt);
		lv_obj_align_to(data->pay_btn, data->pay_qrcode, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
	}
	lv_obj_add_flag(data->pay_btn, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_event_cb(data->pay_btn, _pay_btn_evt_handler, LV_EVENT_SHORT_CLICKED, view_data);

	// free string buffer
	app_mem_free(str_buf);
}

static void _delete_ui_pay(view_data_t *view_data)
{
	wxpay_main_view_data_t *data = view_data->user_data;

	if (data->pay_title) {
		lv_obj_delete(data->pay_title);
		data->pay_title = NULL;
	}
	if (data->pay_qrcode) {
		lv_obj_delete(data->pay_qrcode);
		data->pay_qrcode = NULL;
	}
	if (data->pay_barcode) {
		lv_obj_delete(data->pay_barcode);
		data->pay_barcode = NULL;
	}
	if (data->pay_code) {
		lv_obj_delete(data->pay_code);
		data->pay_code = NULL;
	}
	if (data->pay_btn) {
		lv_obj_delete(data->pay_btn);
		data->pay_btn = NULL;
	}
}

static void _rebuild_ui_main(view_data_t *view_data)
{
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);
	wxpay_main_view_data_t *data = view_data->user_data;
	int bind_status = presenter->get_binding_status();

	// destroy old ui
	_delete_ui_bind(view_data);
	_delete_ui_pay(view_data);

	//reload string resource, in case language changed in sport
	_load_txt_resource(data);

	if (bind_status == STATE_BINDING_OK) {
		// create pay ui
		_create_ui_pay(view_data);
	} else {
		// binding env init
		presenter->bind_init();

		// create bind ui
		_create_ui_bind(view_data);
	}

	// update status
	data->bind_status = bind_status;
	data->status_elaps = 0;
}

static void _pay_btn_evt_handler(lv_event_t * e)
{
	view_data_t *view_data = lv_event_get_user_data(e);
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);

	// switch paycode
	presenter->toggle_barcode();

	// rebuild pay view
	_rebuild_ui_main(view_data);
}

static void _status_timer_cb(lv_timer_t *timer)
{
	view_data_t *view_data = timer->user_data;
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);
	wxpay_main_view_data_t *data = view_data->user_data;
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
				_rebuild_ui_main(view_data);
			}
			break;
		case STATE_BINDING_FAIL:
			_show_bind_result(view_data, data->res_txt[TXT_BIND_FAILED].txt, TIMEOUT_FAILED);
			break;
		case STATE_BINDING_OK:
			if (data->status_elaps == 0) {
				_show_bind_result(view_data, data->res_txt[TXT_BIND_OK].txt, TIMEOUT_OK);
			} else if (data->status_elaps >= TIMEOUT_PAY) {
				// rebuild pay view every 60 secs
				_rebuild_ui_main(view_data);
				data->status_elaps = 0;
			}
			break;
	}

	data->bind_status = bind_status;
}

static void _bind_result_timer_cb(lv_timer_t *timer)
{
	view_data_t *view_data = timer->user_data;
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);
	wxpay_main_view_data_t *data = view_data->user_data;

	// binding env deinit
	presenter->bind_deinit();

	// rebuild ui
	_rebuild_ui_main(view_data);

	// set status timer
	lv_timer_set_period(data->timer, TIMER_PERIOD);
	lv_timer_set_cb(data->timer, _status_timer_cb);
	lv_timer_reset(data->timer);
	data->status_elaps = 0;
}

static int _wxpay_main_view_preload(view_data_t *view_data, bool update)
{
	if (wxpay_main_preload_inited == 0) {
		lvgl_res_preload_scene_compact_default_init(SCENE_WXPAY_MAIN_VIEW, NULL, 0,
			NULL, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
		wxpay_main_preload_inited = 1;
	}

	return lvgl_res_preload_scene_compact_default(SCENE_WXPAY_MAIN_VIEW, WXPAY_MAIN_VIEW, update, 0);
}

static void _wxpay_event_handler(lv_event_t * e)
{
	if(lvgl_click_decision())
	{
		go_to_m_effect();
		lv_obj_remove_event_cb(lv_event_get_user_data(e),_wxpay_event_handler);
	}
}

static int _wxpay_main_view_layout_update(view_data_t *view_data, bool first_layout)
{
	wxpay_main_view_data_t *data = view_data->user_data;

	if(data == NULL) {
		data = app_mem_malloc(sizeof(*data));
		if (!data) {
			return -ENOMEM;
		}
		memset(data, 0, sizeof(*data));
		view_data->user_data = data;
	}

	if(first_layout) {
		LVGL_FONT_OPEN_DEFAULT(&data->font, DEF_FONT_SIZE_NORMAL);

		lv_style_init(&data->sty_lbl);
		lv_style_init(&data->sty_lbl_pr);

		data->timer = lv_timer_create(_status_timer_cb, TIMER_PERIOD, view_data);

		lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
		lv_obj_remove_event_cb(scr,_wxpay_event_handler);
		lv_obj_add_flag(scr,LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_event_cb(scr, _wxpay_event_handler, LV_EVENT_SHORT_CLICKED, scr);
		coder_simulation_tp_register(scr, WXPAY_MAIN_VIEW, LV_DIR_HOR, 50, NULL);
	}

	// rebuild main view
	_rebuild_ui_main(view_data);

	return 0;
}

static int _wxpay_main_view_layout(view_data_t *view_data)
{
	int ret;

	ret = _wxpay_main_view_layout_update(view_data, true);
	if (ret < 0) {
		return ret;
	}

	lv_refr_now(view_data->display);
	return 0;
}

static int _wxpay_main_view_delete(view_data_t *view_data)
{
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);
	wxpay_main_view_data_t *data = view_data->user_data;

	// stop adv
	presenter->stop_adv();

	// unlock screen
	presenter->unlock_screen();

	if (data) {
		lv_style_reset(&data->sty_lbl);
		lv_style_reset(&data->sty_lbl_pr);

		if (data->timer) {
			lv_timer_delete(data->timer);
			data->timer = NULL;
		}

		_delete_ui_bind(view_data);
		_delete_ui_pay(view_data);

		_unload_txt_resource(data);
	
		LVGL_FONT_CLOSE(&data->font);

		app_mem_free(data);
		view_data->user_data = NULL;
	} else {
		lvgl_res_preload_cancel_scene(SCENE_WXPAY_MAIN_VIEW);
	}

	lvgl_res_unload_scene_compact(SCENE_WXPAY_MAIN_VIEW);

	return 0;
}

static int _wxpay_main_view_updated(view_data_t* view_data)
{
	return _wxpay_main_view_layout_update(view_data, false);
}

static int _wxpay_main_view_focus_changed(view_data_t *view_data, bool focused)
{
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);
	wxpay_main_view_data_t *data = view_data->user_data;

	if (focused) {
		if(data && !lvgl_res_scene_is_loaded(SCENE_WXPAY_MAIN_VIEW)) {
			_wxpay_main_view_preload(view_data, true);
		}
	} else {
		if(data) {
			_unload_txt_resource(data);
		}

		lvgl_res_preload_cancel_scene(SCENE_WXPAY_MAIN_VIEW);
		lvgl_res_unload_scene_compact(SCENE_WXPAY_MAIN_VIEW);
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

int _wxpay_main_view_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void *msg_data)
{
	assert(view_id == WXPAY_MAIN_VIEW);

	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return _wxpay_main_view_preload(view_data, false);
	case MSG_VIEW_LAYOUT:
		return _wxpay_main_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _wxpay_main_view_delete(view_data);
	case MSG_VIEW_FOCUS:
		return _wxpay_main_view_focus_changed(view_data, true);
	case MSG_VIEW_DEFOCUS:
		return _wxpay_main_view_focus_changed(view_data, false);
	case MSG_VIEW_UPDATE:
		return _wxpay_main_view_updated(view_data);
	default:
		return 0;
	}
}

VIEW_DEFINE2(wxpay_main_view, _wxpay_main_view_handler, NULL,
		NULL, WXPAY_MAIN_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);

