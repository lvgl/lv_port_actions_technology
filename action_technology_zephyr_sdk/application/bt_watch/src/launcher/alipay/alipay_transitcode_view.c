/*
 * Copyright (c) 2022 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <app_ui.h>
#include "alipay/alipay_ui.h"
#include <view_stack.h>

#define TIMER_PERIOD 100

typedef struct alipay_transitcode_view_data_s {
	lv_obj_t* cont;
	lv_obj_t* transit_title;
	lv_obj_t* transit_qrcode;
	lv_obj_t* transit_btn;
	lv_font_t font;
	lv_style_t sty_lbl;
	lv_style_t sty_blackfont;
	lv_timer_t *timer;
} alipay_transitcode_view_data_t;

static int cur_card_view_status;

static void _create_ui_card(view_data_t *view_data, int card_status);
static void _delete_ui_card(view_data_t* view_data);

static void _card_status_timer_cb(lv_timer_t *timer) {
	view_data_t *view_data = timer->user_data;
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);
	
	int card_status = presenter->get_card_status();

	if(card_status != cur_card_view_status) {
		_delete_ui_card(view_data);
		_create_ui_card(view_data, card_status);
	}
}

static void _transit_btn_evt_handler(lv_event_t * e)
{
	view_stack_jump_view(ALIPAY_CARDLIST_VIEW, &alipay_view_presenter);
}

static void retry_btn_evt_handler(lv_event_t * e)
{
	view_data_t *view_data = lv_event_get_user_data(e);
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);
	
	presenter->load_card_retry();
}

static void _create_ui_card(view_data_t *view_data, int card_status)
{
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	alipay_transitcode_view_data_t *data = view_data->user_data;
	char *title_buf = NULL;
	uint8_t *code_buf = NULL;
	uint32_t code_len;
	int ret;
	lv_obj_t* tip_label;
	lv_obj_t* retry_btn;
	lv_obj_t* retry_btn_label;
	
	cur_card_view_status = card_status;

	// init the style for label
	lv_style_reset(&data->sty_lbl);
	lv_style_set_text_font(&data->sty_lbl, &data->font);
	lv_style_set_text_color(&data->sty_lbl, lv_color_white());

	// init the style for label
	lv_style_reset(&data->sty_blackfont);
	lv_style_set_text_font(&data->sty_blackfont, &data->font);
	lv_style_set_text_color(&data->sty_blackfont, lv_color_black());

	data->cont = lv_obj_create(scr);

	switch(card_status) {
	case STATE_LOADING:
		// set bg color
		lv_obj_set_size(data->cont, LV_PCT(100), LV_PCT(100));
		lv_obj_center(data->cont);
		lv_obj_set_style_bg_opa(data->cont, LV_OPA_COVER, LV_PART_MAIN);
		lv_obj_set_style_bg_color(data->cont, lv_color_black(), LV_PART_MAIN);
		tip_label = lv_label_create(data->cont);
		lv_obj_set_style_bg_opa(tip_label, LV_OPA_COVER, LV_PART_MAIN);
		lv_obj_set_style_bg_color(tip_label, lv_color_make(0x0, 0x0, 0x0), LV_PART_MAIN);
		lv_obj_remove_flag(tip_label, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_center(tip_label);
		lv_label_set_text(tip_label, "乘车码加载中");
		lv_obj_add_style(tip_label, &data->sty_lbl, LV_PART_MAIN);
		break;
	case STATE_LOAD_OK:
		// malloc string buffer
		title_buf = app_mem_malloc(40);
		if (!title_buf) {
			return;
		}

		code_buf = app_mem_malloc(512);
		if (!code_buf) {
			return;
		}

		ret = presenter->get_transitcode(title_buf, code_buf, &code_len);
		if(ret) {
			return;
		}

		// set bg color
		lv_obj_set_size(data->cont, LV_PCT(100), LV_PCT(100));
		lv_obj_center(data->cont);
		lv_obj_set_style_bg_opa(data->cont, LV_OPA_COVER, LV_PART_MAIN);
		lv_obj_set_style_bg_color(data->cont, lv_color_make(0x16, 0x77, 0xFF), LV_PART_MAIN);

		// create qrcode
		data->transit_qrcode = lv_qrcode_create(data->cont);
		lv_qrcode_set_size(data->transit_qrcode, 220);
		lv_qrcode_set_dark_color(data->transit_qrcode, lv_color_black());
		lv_qrcode_set_light_color(data->transit_qrcode, lv_color_white());
		lv_qrcode_update(data->transit_qrcode, code_buf, code_len, qrcodegen_Ecc_LOW);
		lv_obj_center(data->transit_qrcode);
		lv_obj_set_style_border_color(data->transit_qrcode, lv_color_white(), 0);
		lv_obj_set_style_border_width(data->transit_qrcode, 8, 0);

		// create title
		data->transit_title = lv_label_create(data->cont);
		lv_label_set_text(data->transit_title, title_buf);
		lv_obj_add_style(data->transit_title, &data->sty_lbl, LV_PART_MAIN);
		lv_obj_align_to(data->transit_title, data->transit_qrcode, LV_ALIGN_OUT_TOP_MID, 0, -5);


		// create btn
		data->transit_btn = lv_button_create(data->cont);
		lv_obj_set_size(data->transit_btn, 200, 60);
		lv_obj_align_to(data->transit_btn, data->transit_qrcode, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
		lv_obj_add_flag(data->transit_btn, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_event_cb(data->transit_btn, _transit_btn_evt_handler, LV_EVENT_SHORT_CLICKED, view_data);
		tip_label = lv_label_create(data->transit_btn);
		lv_obj_remove_flag(tip_label, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_center(tip_label);
		lv_label_set_text(tip_label, "切换交通卡");
		lv_obj_add_style(tip_label, &data->sty_lbl, LV_PART_MAIN);
		break;
	case STATE_LOAD_FAIL:
		lv_obj_set_size(data->cont, LV_PCT(100), LV_PCT(100));
		lv_obj_center(data->cont);
		lv_obj_set_style_bg_opa(data->cont, LV_OPA_COVER, LV_PART_MAIN);
		lv_obj_set_style_bg_color(data->cont, lv_color_black(), LV_PART_MAIN);
		tip_label = lv_label_create(data->cont);
		lv_obj_set_style_bg_opa(tip_label, LV_OPA_COVER, LV_PART_MAIN);
		lv_obj_set_style_bg_color(tip_label, lv_color_make(0x0, 0x0, 0x0), LV_PART_MAIN);
		lv_obj_remove_flag(tip_label, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_set_size(tip_label, 350, 150);
		lv_obj_set_pos(tip_label, 58, 168);
		lv_label_set_text(tip_label, presenter->get_transit_error_string());
		lv_obj_add_style(tip_label, &data->sty_lbl, LV_PART_MAIN);
		// create btn
		retry_btn = lv_button_create(data->cont);
		lv_obj_set_style_bg_opa(retry_btn, LV_OPA_COVER, LV_PART_MAIN);
		lv_obj_set_style_bg_color(retry_btn, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN);
		lv_obj_align_to(retry_btn, tip_label, LV_ALIGN_OUT_BOTTOM_MID, -75, 5);
		lv_obj_set_style_radius(retry_btn, 30, LV_PART_MAIN);
		lv_obj_set_size(retry_btn, 150, 60);
		lv_obj_add_event_cb(retry_btn, retry_btn_evt_handler, LV_EVENT_SHORT_CLICKED, view_data);

		retry_btn_label = lv_label_create(retry_btn);
		lv_obj_remove_flag(retry_btn_label, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_center(retry_btn_label);
		lv_label_set_text(retry_btn_label, "重试");
		lv_obj_add_style(retry_btn_label, &data->sty_blackfont, LV_PART_MAIN);
		break;
	default:
		break;
	}

	// free string buffer
	if(title_buf)
		app_mem_free(title_buf);
	
	if(code_buf)
		app_mem_free(code_buf);
}

static void _delete_ui_card(view_data_t *view_data)
{
	alipay_transitcode_view_data_t *data = view_data->user_data;

	if(data->cont) {
		lv_obj_delete(data->cont);
		data->cont = NULL;
	}
}

static int _alipay_transitcode_view_preload(view_data_t *view_data, bool update)
{
	ui_view_update(ALIPAY_TRANSITCODE_VIEW);
	return 0;
}

static int _alipay_transitcode_view_layout_update(view_data_t *view_data, bool first_layout)
{
	alipay_transitcode_view_data_t *data = view_data->user_data;
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);
	
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
		lv_style_init(&data->sty_blackfont);
	}

	if (first_layout) {
		_create_ui_card(view_data, presenter->get_card_status());
		data->timer = lv_timer_create(_card_status_timer_cb, TIMER_PERIOD, view_data);
	}

	return 0;
}

static int _alipay_transitcode_view_layout(view_data_t *view_data)
{
	int ret;

	ret = _alipay_transitcode_view_layout_update(view_data, true);
	if (ret < 0) {
		return ret;
	}

	lv_refr_now(view_data->display);
	return 0;
}

static int _alipay_transitcode_view_delete(view_data_t *view_data)
{
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);
	alipay_transitcode_view_data_t *data = view_data->user_data;

	// stop adv
	presenter->stop_adv();

	// unlock screen
	presenter->unlock_screen();

	if (data) {
		if(data->timer) {
			lv_timer_delete(data->timer);
			data->timer = NULL;
		}
		lv_style_reset(&data->sty_lbl);
		lv_style_reset(&data->sty_blackfont);
		_delete_ui_card(view_data);
		LVGL_FONT_CLOSE(&data->font);
		app_mem_free(data);
		view_data->user_data = NULL;
	}

	return 0;
}

static int _alipay_transitcode_view_updated(view_data_t* view_data)
{
	return _alipay_transitcode_view_layout_update(view_data, false);
}

static int _alipay_transitcode_view_focus_changed(view_data_t *view_data, bool focused)
{
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);

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

int _alipay_transitcode_view_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void *msg_data)
{
	assert(view_id == ALIPAY_TRANSITCODE_VIEW);

	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return _alipay_transitcode_view_preload(view_data, false);
	case MSG_VIEW_LAYOUT:
		return _alipay_transitcode_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _alipay_transitcode_view_delete(view_data);
	case MSG_VIEW_FOCUS:
		return _alipay_transitcode_view_focus_changed(view_data, true);
	case MSG_VIEW_DEFOCUS:
		return _alipay_transitcode_view_focus_changed(view_data, false);
	case MSG_VIEW_UPDATE:
		return _alipay_transitcode_view_updated(view_data);
	default:
		return 0;
	}
}

VIEW_DEFINE2(alipay_transitcode_view, _alipay_transitcode_view_handler, NULL,
		NULL, ALIPAY_TRANSITCODE_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);

