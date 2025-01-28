/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os_common_api.h>
#include <widgets/simple_img.h>
#include <widgets/text_canvas.h>
#include "applist_view_inner.h"

/**********************
 *   DEFINES
 **********************/
#define USE_ZOOM_TEXT 0

/* Background parameters */
#define CONT_PAD_HOR    10
#define CONT_PAD_VER    30
/* Item parameters */
#define BTN_COLOR     0xFFA0A0A0
#define BTN_OPA_MAX   300
#define BTN_PAD_SPACE 6
#define BTN_COL_SPACE 6
#define BTN_ZOOM_MIN  (LV_SCALE_NONE / 2)

/**********************
 *   STATIC PROTOTYPES
 **********************/
static int _waterwheel_view_create(lv_obj_t * scr);
static void _waterwheel_view_delete(lv_obj_t * scr);
static void _waterwheel_view_update_icon(applist_ui_data_t * data, uint8_t idx, const lv_image_dsc_t * src);
static void _waterwheel_view_update_text(applist_ui_data_t * data, uint8_t idx, const char * text);

/**********************
 *  GLOBAL VARIABLES
 **********************/
const applist_view_cb_t g_applist_waterwheel_view_cb = {
	.create = _waterwheel_view_create,
	.delete = _waterwheel_view_delete,
	.update_icon = _waterwheel_view_update_icon,
	.update_text = _waterwheel_view_update_text,
};

/**********************
 *  STATIC VARIABLES
 **********************/

static void _waterwheel_compute_btn_space(applist_ui_data_t *data)
{
	applist_waterwheel_view_t* waterwheel_view = &data->waterwheel_view;
	lv_obj_t* cont_bg = lv_obj_get_child(data->cont, 0);
	if(!cont_bg)
		return;
	
	lv_obj_t* btn = lv_obj_get_child(cont_bg, 0);
	if(!btn)
		return;
	
	lv_obj_t* btn_icon = lv_obj_get_child(btn, 0);
	lv_obj_t* btn_text = lv_obj_get_child(btn, 1);

	lv_obj_update_layout(data->cont);

	waterwheel_view->btn_h = BTN_PAD_SPACE * 2 +
			LV_MAX(lv_obj_get_height(btn_icon), lv_obj_get_height(btn_text));
	waterwheel_view->icon_space = (waterwheel_view->btn_h - lv_obj_get_height(btn_icon)) / 2;

	if (data->item_height < waterwheel_view->btn_h)
		data->item_height = waterwheel_view->btn_h;
}

static void _waterwheel_scroll_event_cb(lv_event_t * e)
{
	applist_ui_data_t *data = lv_event_get_user_data(e);
	applist_waterwheel_view_t* waterwheel_view = &data->waterwheel_view;
	lv_obj_t *cont_bg = lv_obj_get_child(data->cont, 0);
	lv_coord_t scrl_y = lv_obj_get_scroll_y(data->cont) - CONT_PAD_VER;
	lv_coord_t cont_w = lv_obj_get_content_width(data->cont);
	lv_coord_t cont_h = lv_obj_get_height(data->cont);
	lv_coord_t half_y = cont_h / 2;

	if(!cont_bg)
		return;

	for (int i = lv_obj_get_child_cnt(cont_bg) - 1; i >= 0; i--) {
		lv_obj_t * btn = lv_obj_get_child(cont_bg, i);
		lv_coord_t btn_y1 = i * data->item_height - scrl_y;
		lv_coord_t btn_pivot_y = btn_y1 + data->item_height / 2;

		if (btn_pivot_y >= cont_h || btn_pivot_y <= 0) {
			lvgl_obj_set_hidden(btn, true);
			continue;
		}

		lv_coord_t offset_y = btn_pivot_y - half_y;
		uint16_t zoom = LV_SCALE_NONE - (LV_SCALE_NONE - BTN_ZOOM_MIN) * LV_ABS(offset_y) / half_y;
		lv_coord_t btn_w = cont_w * zoom / LV_SCALE_NONE;
		lv_coord_t btn_h = waterwheel_view->btn_h * zoom / LV_SCALE_NONE;
		lv_coord_t mid_ofs = (data->item_height - btn_h) / 2;

		lvgl_obj_set_hidden(btn, false);
		lv_obj_set_size(btn, btn_w, btn_h);
		lv_obj_set_style_bg_opa(btn, BTN_OPA_MAX - zoom, LV_PART_MAIN);
		lv_obj_set_pos(btn, (cont_w - btn_w) / 2, i * data->item_height +
				mid_ofs - mid_ofs * offset_y / half_y);

		lv_coord_t icon_w = data->icon[0].header.w * zoom / LV_SCALE_NONE;
		lv_coord_t icon_space = waterwheel_view->icon_space * zoom / LV_SCALE_NONE;
		lv_coord_t icon_ofs = icon_space + (icon_w - (lv_coord_t)data->icon[0].header.w) / 2;
		lv_coord_t text_space = BTN_COL_SPACE * zoom / LV_SCALE_NONE;
		lv_coord_t text_ofs = icon_w + text_space * 2;

		lv_obj_t *obj_icon = lv_obj_get_child(btn, 0);
		if(obj_icon) {
			lv_obj_set_x(obj_icon, icon_ofs);
			simple_img_set_scale(obj_icon, zoom);
		}

#if USE_ZOOM_TEXT
		lv_obj_t *obj_text = lv_obj_get_child(btn, 2);
		if (obj_text) {
			const lv_image_dsc_t *text_src = simple_img_get_src(obj_text);
			lv_coord_t text_w = text_src->header.w * zoom / LV_SCALE_NONE;

			lv_obj_set_x(obj_text, text_ofs + (text_w - (lv_coord_t)text_src->header.w) / 2);
			simple_img_set_scale(obj_text, zoom);
		}
#else
		lv_obj_t *obj_text = lv_obj_get_child(btn, 1);
		if (obj_text)
			lv_obj_set_x(obj_text, text_ofs);
#endif
	}
}

static int _waterwheel_view_create(lv_obj_t * scr)
{
	applist_ui_data_t *data = lv_obj_get_user_data(scr);
	applist_waterwheel_view_t *waterwheel_view = &data->waterwheel_view;

	data->cont = lv_obj_create(scr);
	lv_obj_set_pos(data->cont, data->res_scene.x, data->res_scene.y);
	lv_obj_set_size(data->cont, data->res_scene.width, data->res_scene.height);
	lv_obj_set_style_pad_hor(data->cont, CONT_PAD_HOR, LV_PART_MAIN);
	lv_obj_set_style_pad_ver(data->cont, CONT_PAD_VER, LV_PART_MAIN);
	lv_obj_set_style_text_font(data->cont, &data->font, LV_PART_MAIN); /* inherit text style */
	lv_obj_set_style_text_color(data->cont, data->text[0].color, LV_PART_MAIN);
	lv_obj_set_scroll_dir(data->cont, LV_DIR_VER);
	lv_obj_set_scrollbar_mode(data->cont, LV_SCROLLBAR_MODE_OFF); /* default no scrollbar */
	lv_obj_add_event_cb(data->cont, _waterwheel_scroll_event_cb, LV_EVENT_SCROLL, data);
	coder_simulation_tp_register(data->cont, APPLIST_VIEW, LV_DIR_VER, 50, NULL);

	lv_style_init(&waterwheel_view->sty_btn);
	lv_style_set_bg_color(&waterwheel_view->sty_btn, lv_color_hex(BTN_COLOR));
	lv_style_set_radius(&waterwheel_view->sty_btn, LV_RADIUS_CIRCLE);

	lv_obj_t * cont_bg = lv_obj_create(data->cont);
	lv_obj_set_size(cont_bg, LV_PCT(100), data->item_height * NUM_ITEMS);

	/* Add buttons to the list */
	for (int i = 0; i < NUM_ITEMS; i++) {
		lv_obj_t *btn = lv_button_create(cont_bg);
		lv_obj_set_pos(btn, 0, i * data->item_height);
		lv_obj_set_size(btn, LV_PCT(100), data->item_height);
		lv_obj_add_style(btn, &waterwheel_view->sty_btn, LV_PART_MAIN);
		lv_obj_add_event_cb(btn, applist_btn_event_def_handler, LV_EVENT_SHORT_CLICKED, data);
		lv_obj_set_user_data(btn, (void *)i);

		lv_obj_t *obj_icon = simple_img_create(btn);
		simple_img_set_src(obj_icon, applist_get_icon(data, i));
		lv_obj_align(obj_icon, LV_ALIGN_LEFT_MID, 0, 0);

		lv_obj_t *obj_text = text_canvas_create(btn);
		text_canvas_set_text_static(obj_text, applist_get_text(data, i));
		lv_obj_remove_flag(obj_text, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_align(obj_text, LV_ALIGN_LEFT_MID, data->icon[0].header.w + data->item_space, 0);

#if USE_ZOOM_TEXT
		lv_obj_add_flag(obj_text, LV_OBJ_FLAG_HIDDEN);

		lv_obj_t *obj_text_img = simple_img_create(btn);
		simple_img_set_src(obj_text_img, text_canvas_get_img(obj_text));
#endif
	}

	_waterwheel_compute_btn_space(data);

	int16_t scrl_y = 0;
	data->presenter->load_scroll_value(NULL, &scrl_y);

	/* update the buttons position manually for first*/
	lv_obj_send_event(data->cont, LV_EVENT_SCROLL, NULL);
	/* scroll to the last position */
	lv_obj_scroll_to_y(data->cont, scrl_y, LV_ANIM_OFF);

	return 0;
}

static void _waterwheel_view_delete(lv_obj_t * scr)
{
	applist_ui_data_t *data = lv_obj_get_user_data(scr);
	applist_waterwheel_view_t *waterwheel_view = &data->waterwheel_view;

	lv_style_reset(&waterwheel_view->sty_btn);

	data->presenter->save_scroll_value(0, lv_obj_get_scroll_y(data->cont));
}

static void _waterwheel_view_update_icon(applist_ui_data_t * data, uint8_t idx, const lv_image_dsc_t * src)
{
	lv_obj_t *cont_bg =lv_obj_get_child(data->cont, 0);
	if(!cont_bg) {
		return;
	}
	
	lv_obj_t *btn = lv_obj_get_child(cont_bg, idx);
	if (!btn) {
		return;
	}

	lv_obj_t *obj_icon = lv_obj_get_child(btn, 0);
	if (!obj_icon) {
		return;
	}

	simple_img_set_src(obj_icon, src);
}

static void _waterwheel_view_update_text(applist_ui_data_t * data, uint8_t idx, const char * text)
{
	lv_obj_t *cont_bg =lv_obj_get_child(data->cont, 0);
	if(!cont_bg) {
		return;
	}
	
	lv_obj_t *btn = lv_obj_get_child(cont_bg, idx);
	if (!btn) {
		return;
	}
	
	lv_obj_t *obj_text = lv_obj_get_child(btn, 1);
	if (obj_text)
		text_canvas_set_text_static(obj_text, text);

#if USE_ZOOM_TEXT
	lv_obj_t *obj_text_img = lv_obj_get_child(btn, 2);
	if (obj_text_img)
		simple_img_set_src(obj_text_img, text_canvas_get_img(obj_text));
#endif
}
