/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file watch_list.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <sys/util.h>

#include "simple_img.h"
#include "text_canvas.h"
#include "watch_list.h"
#include <lvgl/src/lvgl_private.h>

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &watch_list_class

#define WATCH_LIST_DEF_PAD_COLUMN (10)

/**********************
 *      TYPEDEFS
 **********************/
/** Data of list */
typedef struct {
	lv_obj_t obj;

	lv_style_t sty_init_btn;
	lv_coord_t self_pad_top;
	lv_coord_t self_pad_bottom;

	int16_t sel_idx;  /* selected (clicked) item */

	watch_list_scroll_cb_t scroll_cb;
} watch_list_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void watch_list_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void watch_list_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void watch_list_event(const lv_obj_class_t * class_p, lv_event_t * e);
static void watch_list_draw(lv_event_t * e);
static void refr_position(lv_obj_t *obj);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t watch_list_class = {
	.constructor_cb = watch_list_constructor,
	.destructor_cb = watch_list_destructor,
	.event_cb = watch_list_event,
	.width_def = LV_PCT(100),
	.height_def = LV_PCT(100),
	.instance_size = sizeof(watch_list_t),
	.base_class = &lv_obj_class,
	.name = "watch-list",
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * watch_list_create(lv_obj_t * parent)
{
	LV_LOG_INFO("begin");
	lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
	lv_obj_class_init_obj(obj);

	lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_scroll_dir(obj, LV_DIR_VER);

	/* default no scrollbar */
	lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);

	return obj;
}

void watch_list_clean(lv_obj_t * obj)
{
	lv_obj_clean(obj);
}

/*=====================
 * Setter functions
 *====================*/

lv_obj_t * watch_list_add_btn(lv_obj_t *obj, const lv_image_dsc_t * icon, const char * text)
{
	watch_list_t *list = (watch_list_t *)obj;
	lv_obj_t *btn = lv_button_create(obj);

	lv_obj_add_flag(btn, LV_OBJ_FLAG_SCROLL_CHAIN);
	lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);
	lv_obj_add_style(btn, &list->sty_init_btn, LV_PART_MAIN);
	lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
	lv_obj_set_style_flex_cross_place(btn, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);

	if (icon) {
		lv_obj_t *obj_icon = simple_img_create(btn);
		simple_img_set_src(obj_icon, icon);
	}

	if (text) {
		lv_obj_t *obj_text = text_canvas_create(btn);
		lv_obj_remove_flag(obj_text, LV_OBJ_FLAG_CLICKABLE);
		text_canvas_set_text_static(obj_text, text);
	}

	return btn;
}

void watch_list_remove(lv_obj_t * obj, uint16_t index)
{
	lv_obj_t * child = lv_obj_get_child(obj, index);

	if (child) {
		lv_obj_delete(child);
	}
}

void watch_list_set_scroll_cb(lv_obj_t *obj, watch_list_scroll_cb_t scroll_cb)
{
	watch_list_t *list = (watch_list_t *)obj;

	list->scroll_cb = scroll_cb;

	/*Update the buttons position manually for first*/
	lv_obj_update_layout(obj);
	lv_obj_send_event(obj, LV_EVENT_SCROLL, NULL);
}

/*=====================
 * Getter functions
 *====================*/

int16_t watch_list_get_btn_index(const lv_obj_t * obj, const lv_obj_t * btn)
{
	int16_t index = 0;

	for (index = lv_obj_get_child_cnt(obj) - 1; index >= 0; index--) {
		if (btn == lv_obj_get_child(obj, index)) {
			break;
		}
	}

	return index;
}

lv_obj_t * watch_list_get_icon(const lv_obj_t * obj, uint16_t index)
{
	lv_obj_t *btn = lv_obj_get_child(obj, index);

	return btn ? lv_obj_get_child(btn, 0) : NULL;
}

lv_obj_t * watch_list_get_text(const lv_obj_t * obj, uint16_t index)
{
	lv_obj_t *btn = lv_obj_get_child(obj, index);

	return btn ? lv_obj_get_child(btn, 1) : NULL;
}

uint16_t watch_list_get_size(const lv_obj_t * obj)
{
	return lv_obj_get_child_cnt(obj);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void watch_list_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
	watch_list_t *list = (watch_list_t *)obj;

	lv_style_init(&list->sty_init_btn);
	lv_style_set_width(&list->sty_init_btn, LV_PCT(100));
	lv_style_set_pad_column(&list->sty_init_btn, WATCH_LIST_DEF_PAD_COLUMN);
}

static void watch_list_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
	watch_list_t *list = (watch_list_t *)obj;

	lv_style_reset(&list->sty_init_btn);
}

static void watch_list_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
	LV_UNUSED(class_p);

	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * obj = lv_event_get_target(e);

	/*Ancestor events will be called during drawing*/
	if (code != LV_EVENT_COVER_CHECK && code != LV_EVENT_DRAW_MAIN) {
		/* scrollbar is drawn in LV_EVENT_DRAW_POST */
		if (code != LV_EVENT_DRAW_POST || lv_obj_get_scrollbar_mode(obj) != LV_SCROLLBAR_MODE_OFF) {
			/*Call the ancestor's event handler*/
			lv_result_t res = lv_obj_event_base(MY_CLASS, e);
			if (res != LV_RESULT_OK) return;
		}

		if (code == LV_EVENT_CHILD_CHANGED || code == LV_EVENT_SCROLL) {
			refr_position(obj);
		}
	} else {
		watch_list_draw(e);
	}
}

static void watch_list_draw(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * obj = lv_event_get_target(e);

	if (code == LV_EVENT_COVER_CHECK) {
		lv_cover_check_info_t * info = lv_event_get_param(e);
		if (info->res == LV_COVER_RES_MASKED) return;

		if (lv_area_is_in(info->area, &obj->coords, 0) == false) {
			info->res = LV_COVER_RES_NOT_COVER;
			return;
		}

		if (lv_obj_get_style_bg_opa(obj, LV_PART_MAIN) < LV_OPA_MAX) {
			info->res = LV_COVER_RES_NOT_COVER;
			return;
		}

		info->res = LV_COVER_RES_COVER;
		return;
	}

	if (code == LV_EVENT_DRAW_MAIN) {
		lv_draw_rect_dsc_t draw_dsc;
		lv_draw_rect_dsc_init(&draw_dsc);
		//lv_obj_init_draw_rect_dsc(obj, LV_PART_MAIN, &draw_dsc);
		draw_dsc.bg_opa = lv_obj_get_style_bg_opa(obj, LV_PART_MAIN);
		draw_dsc.bg_color = lv_obj_get_style_bg_color_filtered(obj, LV_PART_MAIN);

		lv_layer_t * layer = lv_event_get_layer(e);
		lv_draw_rect(layer, &draw_dsc, &obj->coords);
	}
}

static void refr_position(lv_obj_t *obj)
{
	watch_list_t *list = (watch_list_t *)obj;
	int child_cnt = lv_obj_get_child_cnt(obj);
	int i;

	if (list->scroll_cb == NULL) {
		return;
	}

	for (i = 0; i < child_cnt; i++) {
		lv_obj_t * child = lv_obj_get_child(obj, i);

		lv_area_t child_a;
		lv_obj_get_coords(child, &child_a);

		if (child_a.y1 > obj->coords.y2 || child_a.y2 < obj->coords.y1)
			continue;

		lv_obj_t * icon = lv_obj_get_child(child, 0);
		lv_coord_t icon_w = lv_obj_get_width(icon);
		lv_point_t center = { icon_w / 2, child_a.y1 + lv_area_get_height(&child_a) / 2 };

		list->scroll_cb(obj, &center);

		/*Translate the item by the calculated X coordinate(also aligned to 2 pixels)*/
		lv_obj_set_style_translate_x(child, (center.x - icon_w / 2) & ~0x1, LV_PART_MAIN);
	}
}
