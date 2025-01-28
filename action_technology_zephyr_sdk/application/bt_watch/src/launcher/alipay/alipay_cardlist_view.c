/*
 * Copyright (c) 2022 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <app_ui.h>
#include "alipay/alipay_ui.h"
#include <view_stack.h>

#define BTN_MAX_NUM 10
#define TIMER_PERIOD		(100)

typedef struct alipay_tansit_CardTitle
{
    char  title[40];                //卡标题
}alipay_tansit_CardTitle_t;

typedef struct alipay_cardlist_view_data_s {
	lv_obj_t* cont;
	lv_font_t font;
	lv_style_t sty_lbl;
	lv_style_t sty_blackfont;
	lv_obj_t* exit_btn;
	lv_obj_t* finished_btn;
	lv_timer_t *timer;
	lv_timer_t *auto_refresh_cardlist_timer;
	uint32_t auto_refresh_cnt;
} alipay_cardlist_view_data_t;

static const alipay_view_presenter_t *presenter;
static int cur_cardlist_view_status;
static void _create_ui_cardlist(view_data_t *view_data, int cardlist_status);
static void _delete_ui_cardlist(view_data_t *view_data);

static void auto_refresh_cardlist_timer_cb(lv_timer_t *timer) {
	view_data_t *view_data = timer->user_data;
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);
	alipay_cardlist_view_data_t *data = view_data->user_data;
	int card_num = presenter->get_card_num();

	data->auto_refresh_cnt += TIMER_PERIOD;

	if(cur_cardlist_view_status == STATE_LOAD_OK) {
		if(card_num == 0) {
			if(data->auto_refresh_cnt >= 10000) {
				SYS_LOG_INF("auto refresh cardlist");
				presenter->load_cardlist(1);
				data->auto_refresh_cnt = 0;
			}
		} else {
			SYS_LOG_INF("auto refresh cardlist OK");
			if(data->auto_refresh_cardlist_timer) {
				lv_timer_delete(data->auto_refresh_cardlist_timer);
				data->auto_refresh_cardlist_timer = NULL;
			}
			_delete_ui_cardlist(view_data);
			_create_ui_cardlist(view_data, STATE_LOAD_OK);
			if(data->timer)
				lv_timer_resume(data->timer);
		}
	}
}

static void _cardlist_status_timer_cb(lv_timer_t *timer) {
	view_data_t *view_data = timer->user_data;
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);
	int cardlist_status = presenter->get_cardlist_status();

	if(cardlist_status != cur_cardlist_view_status) {
		_delete_ui_cardlist(view_data);
		_create_ui_cardlist(view_data, cardlist_status);
	}
}

static void refresh_btn_evt_handler(lv_event_t* e)
{
	SYS_LOG_INF("btn refresh cardlist");
	presenter->load_cardlist(1);
}

static void cardlist_btn_evt_handler(lv_event_t* e)
{
	int i = (int)lv_event_get_user_data(e);
	SYS_LOG_INF("select card %d", i);
	presenter->load_card(i);
	view_stack_jump_view(ALIPAY_TRANSITCODE_VIEW, &alipay_view_presenter);
}

static void tip_btn_evt_handler(lv_event_t* e)
{
	view_data_t *view_data = lv_event_get_user_data(e);
	alipay_cardlist_view_data_t *data = view_data->user_data;
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);
	lv_obj_t *obj = lv_event_get_target(e);
	
	if(obj == data->exit_btn) {
		view_stack_pop();
	} else if(obj == data->finished_btn) {
		presenter->load_cardlist(1);
	}
}

static void retry_btn_evt_handler(lv_event_t * e)
{
	view_data_t *view_data = lv_event_get_user_data(e);
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);
	int ret;
	
	ret = presenter->load_cardlist(0);
	if(ret) {
		ret = presenter->load_cardlist(1);
	}
}

static void _create_ui_cardlist(view_data_t *view_data, int cardlist_status)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	alipay_cardlist_view_data_t *data = view_data->user_data;
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);
	lv_obj_t *tip_label;
	lv_obj_t *exit_label;
	lv_obj_t *finished_label;
	lv_obj_t *cardlist_area;
	lv_obj_t* retry_btn;
	lv_obj_t* retry_btn_label;
	uint8_t card_num;
	int i;
	char card_title_buf[40];

	cur_cardlist_view_status = cardlist_status;
	data->cont = lv_obj_create(scr);
	// set bg color
	lv_obj_set_size(data->cont, LV_PCT(100), LV_PCT(100));
	lv_obj_center(data->cont);
	lv_obj_set_style_bg_opa(data->cont, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_bg_color(data->cont, lv_color_black(), LV_PART_MAIN);

	// init the style for label
	lv_style_reset(&data->sty_lbl);
	lv_style_set_text_font(&data->sty_lbl, &data->font);
	lv_style_set_text_color(&data->sty_lbl, lv_color_white());

	lv_style_reset(&data->sty_blackfont);
	lv_style_set_text_font(&data->sty_blackfont, &data->font);
	lv_style_set_text_color(&data->sty_blackfont, lv_color_black());
	
	switch(cardlist_status) {
	case STATE_LOAD_IDLE:
		tip_label = lv_label_create(data->cont);
		lv_obj_set_style_bg_opa(tip_label, LV_OPA_COVER, LV_PART_MAIN);
		lv_obj_set_style_bg_color(tip_label, lv_color_make(0x0, 0x0, 0x0), LV_PART_MAIN);
		lv_obj_remove_flag(tip_label, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_set_pos(tip_label, 50, 90);
		lv_obj_set_size(tip_label, 366, 250);
		lv_label_set_text(tip_label, "初次使用，完成以下步骤\n1.手机打开蓝牙功能；"
			"\n2.使用[乘车测试应用]搜索并连接手表；\n3.点击START开启服务。");
		lv_obj_add_style(tip_label, &data->sty_lbl, LV_PART_MAIN);
		
		data->exit_btn = lv_button_create(data->cont);
		lv_obj_set_style_bg_opa(data->exit_btn, LV_OPA_COVER, LV_PART_MAIN);
		lv_obj_set_style_bg_color(data->exit_btn, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN);
		lv_obj_align_to(data->exit_btn, tip_label, LV_ALIGN_OUT_BOTTOM_MID, -155, 5);
		lv_obj_set_style_radius(data->exit_btn, 30, LV_PART_MAIN);
		lv_obj_set_size(data->exit_btn, 150, 60);
		lv_obj_add_event_cb(data->exit_btn, tip_btn_evt_handler, LV_EVENT_SHORT_CLICKED, view_data);

		exit_label = lv_label_create(data->exit_btn);
		lv_obj_remove_flag(exit_label, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_center(exit_label);
		lv_label_set_text(exit_label, "退出");
		lv_obj_add_style(exit_label, &data->sty_blackfont, LV_PART_MAIN);

		data->finished_btn = lv_button_create(data->cont);
		lv_obj_set_style_bg_opa(data->finished_btn, LV_OPA_COVER, LV_PART_MAIN);
		lv_obj_set_style_bg_color(data->finished_btn, lv_color_make(0x16, 0x77, 0xFF), LV_PART_MAIN);
		lv_obj_align_to(data->finished_btn, tip_label, LV_ALIGN_OUT_BOTTOM_MID, 5, 5);
		lv_obj_set_style_radius(data->finished_btn, 30, LV_PART_MAIN);
		lv_obj_set_size(data->finished_btn, 150, 60);
		lv_obj_add_event_cb(data->finished_btn, tip_btn_evt_handler, LV_EVENT_SHORT_CLICKED, view_data);

		finished_label = lv_label_create(data->finished_btn);
		lv_obj_remove_flag(finished_label, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_center(finished_label);
		lv_label_set_text(finished_label, "已完成");
		lv_obj_add_style(finished_label, &data->sty_lbl, LV_PART_MAIN);
		break;
	case STATE_LOADING:
		tip_label = lv_label_create(data->cont);
		lv_obj_set_style_bg_opa(tip_label, LV_OPA_COVER, LV_PART_MAIN);
		lv_obj_set_style_bg_color(tip_label, lv_color_make(0x0, 0x0, 0x0), LV_PART_MAIN);
		lv_obj_remove_flag(tip_label, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_center(tip_label);
		lv_label_set_text(tip_label, "卡列表加载中");
		lv_obj_add_style(tip_label, &data->sty_lbl, LV_PART_MAIN);
		break;
	case STATE_LOAD_OK:
		card_num = presenter->get_card_num();
		if(card_num == 0) {
			lv_obj_set_style_bg_color(data->cont, lv_color_make(0x16, 0x77, 0xFF), LV_PART_MAIN);
			uint8_t *code_buf = app_mem_malloc(128);
			uint32_t code_len;
			presenter->get_transit_guide_string(code_buf, &code_len);
			lv_obj_t *qrcode = lv_qrcode_create(data->cont);
			lv_qrcode_set_size(qrcode, 220);
			lv_qrcode_set_dark_color(qrcode, lv_color_black());
			lv_qrcode_set_light_color(qrcode, lv_color_white());
			lv_qrcode_update(qrcode, code_buf, code_len, qrcodegen_Ecc_LOW);
			lv_obj_center(qrcode);
			lv_obj_set_style_border_color(qrcode, lv_color_white(), 0);
			lv_obj_set_style_border_width(qrcode, 8, 0);

			lv_obj_t *label_top = lv_label_create(data->cont);
			lv_label_set_text(label_top, "乘车码未开通");
			lv_obj_add_style(label_top, &data->sty_lbl, LV_PART_MAIN);
			lv_obj_align_to(label_top, qrcode, LV_ALIGN_OUT_TOP_MID, 0, -5);

			lv_obj_t *label_bottom = lv_label_create(data->cont);
			lv_label_set_text(label_bottom, "请用支付宝扫码开通");
			lv_obj_add_style(label_bottom, &data->sty_lbl, LV_PART_MAIN);
			lv_obj_align_to(label_bottom, qrcode, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
			if(!data->auto_refresh_cardlist_timer) {
				if(data->timer)
					lv_timer_pause(data->timer);
				data->auto_refresh_cnt = 0;
				data->auto_refresh_cardlist_timer = lv_timer_create(auto_refresh_cardlist_timer_cb, TIMER_PERIOD, view_data);
			}
		} else {
			cardlist_area = lv_obj_create(data->cont);
			lv_obj_set_size(cardlist_area, LV_PCT(100), LV_PCT(80));
			lv_obj_set_flex_flow(data->cont, LV_FLEX_FLOW_COLUMN);
			lv_obj_set_style_clip_corner(data->cont, true, 0);
			lv_obj_set_scroll_dir(data->cont, LV_DIR_VER);
			lv_obj_set_scrollbar_mode(data->cont, LV_SCROLLBAR_MODE_OFF);
			
			lv_obj_t *btn = lv_button_create(scr);
			lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
			lv_obj_set_style_bg_color(btn, lv_color_make(0x16, 0x77, 0xFF), LV_PART_MAIN);
			lv_obj_set_size(btn, 150, 60);
			lv_obj_set_style_radius(btn, 30, LV_PART_MAIN);
			lv_obj_align_to(btn, cardlist_area, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
			lv_obj_add_event_cb(btn, refresh_btn_evt_handler, LV_EVENT_SHORT_CLICKED, NULL);

			lv_obj_t *label = lv_label_create(btn);
			lv_obj_remove_flag(label, LV_OBJ_FLAG_CLICKABLE);
			lv_obj_center(label);
			lv_label_set_text(label, "刷新");
			lv_obj_add_style(label, &data->sty_lbl, LV_PART_MAIN);
			
			// create btn		
			for(i=0; i<card_num+1; i++) {
				lv_obj_t* btn = lv_button_create(cardlist_area);
				if(i < card_num) {
					presenter->get_card_title(i, card_title_buf, sizeof(card_title_buf));
					lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
					lv_obj_set_style_bg_color(btn, lv_color_make(0x3B, 0x3B, 0x3B), LV_PART_MAIN);
					lv_obj_add_event_cb(btn, cardlist_btn_evt_handler, LV_EVENT_SHORT_CLICKED, (void *)i);	
				} else {
					lv_obj_set_style_bg_opa(btn, LV_OPA_0, LV_PART_MAIN);
				}
				lv_obj_set_size(btn, 350, 85);
				lv_obj_set_pos(btn, 58, i*92+80);
				lv_obj_set_style_radius(btn, 20, LV_PART_MAIN);

				if(i < card_num) {
					lv_obj_t *label = lv_label_create(btn);
					lv_obj_remove_flag(label, LV_OBJ_FLAG_CLICKABLE);
					lv_obj_align(label, LV_ALIGN_LEFT_MID, 10, 0);
					lv_label_set_text(label, card_title_buf);
					lv_obj_add_style(label, &data->sty_lbl, LV_PART_MAIN);
				}
			}
		}
		
		break;
	case STATE_LOAD_FAIL:
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
}

static void _delete_ui_cardlist(view_data_t *view_data)
{
	alipay_cardlist_view_data_t *data = view_data->user_data;
	
	if(data->cont) {
		lv_obj_delete(data->cont);
		data->cont = NULL;
	}
}


static int _alipay_cardlist_view_preload(view_data_t *view_data, bool update)
{
	ui_view_update(ALIPAY_CARDLIST_VIEW);
	return 0;
}

static int _alipay_cardlist_view_layout_update(view_data_t *view_data, bool first_layout)
{
	alipay_cardlist_view_data_t *data = view_data->user_data;

	if(data == NULL) {
		data = app_mem_malloc(sizeof(*data));
		if (!data) {
			return -ENOMEM;
		}
		memset(data, 0, sizeof(*data));
		view_data->user_data = data;
	}

	if(first_layout) {
		presenter = view_get_presenter(view_data);
		LVGL_FONT_OPEN_DEFAULT(&data->font, DEF_FONT_SIZE_NORMAL);
		lv_style_init(&data->sty_lbl);
		lv_style_init(&data->sty_blackfont);
	}

	// build view
	if (first_layout) {
		presenter->load_cardlist(0);
		_create_ui_cardlist(view_data, presenter->get_cardlist_status());
		data->timer = lv_timer_create(_cardlist_status_timer_cb, TIMER_PERIOD, view_data);
	}

	return 0;
}

static int _alipay_cardlist_view_layout(view_data_t *view_data)
{
	int ret;

	ret = _alipay_cardlist_view_layout_update(view_data, true);
	if (ret < 0) {
		return ret;
	}

	lv_refr_now(view_data->display);
	return 0;
}

static int _alipay_cardlist_view_delete(view_data_t *view_data)
{
	const alipay_view_presenter_t *presenter = view_get_presenter(view_data);
	alipay_cardlist_view_data_t *data = view_data->user_data;

	// stop adv
	presenter->stop_adv();

	// unlock screen
	presenter->unlock_screen();

	if (data) {
		if(data->timer) {
			lv_timer_delete(data->timer);
			data->timer = NULL;
		}
		if(data->auto_refresh_cardlist_timer) {
			lv_timer_delete(data->auto_refresh_cardlist_timer);
			data->auto_refresh_cardlist_timer = NULL;
		}
		lv_style_reset(&data->sty_lbl);
		lv_style_reset(&data->sty_blackfont);
		_delete_ui_cardlist(view_data);
		LVGL_FONT_CLOSE(&data->font);
		app_mem_free(data);
		view_data->user_data = NULL;
	}

	return 0;
}

static int _alipay_cardlist_view_updated(view_data_t* view_data)
{
	return _alipay_cardlist_view_layout_update(view_data, false);
}

static int _alipay_cardlist_view_focus_changed(view_data_t *view_data, bool focused)
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

int _alipay_cardlist_view_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void *msg_data)
{
	assert(view_id == ALIPAY_CARDLIST_VIEW);

	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return _alipay_cardlist_view_preload(view_data, false);
	case MSG_VIEW_LAYOUT:
		return _alipay_cardlist_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _alipay_cardlist_view_delete(view_data);
	case MSG_VIEW_FOCUS:
		return _alipay_cardlist_view_focus_changed(view_data, true);
	case MSG_VIEW_DEFOCUS:
		return _alipay_cardlist_view_focus_changed(view_data, false);
	case MSG_VIEW_UPDATE:
		return _alipay_cardlist_view_updated(view_data);
	default:
		return 0;
	}
}

VIEW_DEFINE2(alipay_cardlist_view, _alipay_cardlist_view_handler, NULL,
		NULL, ALIPAY_CARDLIST_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);

