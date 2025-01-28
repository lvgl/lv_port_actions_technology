/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file watch_tileview.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "watch_tileview.h"

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &watch_tileview_class

#define CROSS_SCROLL_OPA 128
#define MAIN_SCROLL_ZOOM_MIN LV_IMG_ZOOM_NONE
//#define MAIN_SCROLL_ZOOM_MIN (LV_IMG_ZOOM_NONE / 2)

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void watch_tileview_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void watch_tileview_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void watch_tileview_event_cb(const lv_obj_class_t * class_p, lv_event_t * e);

static void watch_tileview_tile_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);

static lv_obj_t * watch_tileview_add_tile(lv_obj_t * obj, uint8_t col_id, uint8_t row_id, lv_dir_t dir);
static void watch_tileview_set_cross_opa(lv_obj_t * obj, lv_opa_t opa);
static void watch_tileview_set_main_zoom(lv_obj_t * obj, bool reset_nozoom);
static void watch_tileview_adjust_main_rotate(lv_obj_t * obj, lv_anim_enable_t anim_en);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t watch_tileview_class = {
	.constructor_cb = watch_tileview_constructor,
	.destructor_cb = watch_tileview_destructor,
	.event_cb = watch_tileview_event_cb,
	.width_def = LV_PCT(100),
	.height_def = LV_PCT(100),
	.base_class = &lv_obj_class,
	.instance_size = sizeof(watch_tileview_t),
};

const lv_obj_class_t watch_tileview_tile_class ={
	.constructor_cb = watch_tileview_tile_constructor,
	.width_def = LV_PCT(100),
	.height_def = LV_PCT(100),
	.base_class = &lv_obj_class,
	.instance_size = sizeof(watch_tileview_tile_t),
};

static bool bypass_scroll_event;
static lv_dir_t create_dir;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * watch_tileview_create(lv_obj_t * parent)
{
	lv_obj_t* obj = lv_obj_class_create_obj(MY_CLASS, parent);
	lv_obj_class_init_obj(obj);
	return obj;
}

/*=====================
 * Setter functions
 *====================*/

void watch_tileview_add_tiles(lv_obj_t * obj, lv_obj_t * tiles[], const uint8_t tile_cnts[4],
							  lv_dir_t main_dir, bool main_rotated, bool cross_overlapped)
{
	watch_tileview_t* tv = (watch_tileview_t*)obj;
	lv_dir_t cross_dir = (main_dir & LV_DIR_HOR) ? LV_DIR_VER : LV_DIR_HOR;
	uint8_t col_id, row_id;
	int i, j = 0;

	tv->cross_dir = (main_dir & LV_DIR_HOR) ? LV_DIR_VER : LV_DIR_HOR;
	tv->cross_overlapped = cross_overlapped && (tile_cnts[2] + tile_cnts[3] > 0);
	tv->main_cnt = tile_cnts[0] + tile_cnts[1] + 1;
	tv->main_rotated = main_rotated && (tv->main_cnt >= 3); /* require 3 main tiles*/

	lv_obj_update_layout(obj);

	for (i = 0; i < tile_cnts[0] + tile_cnts[1] + 1; i++) {
		lv_dir_t dir = (i == tile_cnts[0] && (tile_cnts[2] + tile_cnts[3] > 0)) ? LV_DIR_ALL : main_dir;
		col_id = (main_dir & LV_DIR_HOR) ? i : tile_cnts[2];
		row_id = (main_dir & LV_DIR_HOR) ? tile_cnts[2] : i;
		tiles[j++] = watch_tileview_add_tile(obj, col_id, row_id, dir);
	}

	for (i = 0; i < tile_cnts[2]; i++) {
		col_id = (main_dir & LV_DIR_HOR) ? tile_cnts[0] : i;
		row_id = (main_dir & LV_DIR_HOR) ? i : tile_cnts[0];
		tiles[j++] = watch_tileview_add_tile(obj, col_id, row_id, cross_dir);
	}

	for (i = 0; i < tile_cnts[3]; i++) {
		col_id = (main_dir & LV_DIR_HOR) ? tile_cnts[0] : (tile_cnts[2] + 1 + i);
		row_id = (main_dir & LV_DIR_HOR) ? (tile_cnts[2] + 1 + i) : tile_cnts[0];
		tiles[j++] = watch_tileview_add_tile(obj, col_id, row_id, cross_dir);
	}
}

lv_res_t watch_tileview_set_tile(lv_obj_t * obj, lv_obj_t * tile_obj, lv_anim_enable_t anim_en)
{
	watch_tileview_t *tv = (watch_tileview_t *)obj;
	watch_tileview_tile_t *tile = (watch_tileview_tile_t *)tile_obj;
	watch_tileview_tile_t *tile_act;

	/*
	 * FIXME:
	 * donot change active tile when scrolling by pointer. If wanted, must
	 * reset the scroll_obj immediately, since scroll_obj may be destroyed
	 * after jumping.
	 */
	if (lv_obj_is_scrolling(obj))
		return LV_RES_INV;

	lv_obj_update_snap(obj, LV_ANIM_OFF);
	lv_indev_wait_release(lv_indev_get_next(NULL));

	tile_act = (watch_tileview_tile_t *)tv->tile_act;
	if (tv->tile_act == tile_obj)
		return LV_RES_OK;

	bypass_scroll_event = true;

	/* 1) scroll to center first if not on one line */
	if (tv->cross_overlapped && ((tile_obj == tv->tile_center) || !(tile_act->dir & tile->dir))) {
		tv->tile_act = tv->tile_center;
		lv_obj_set_scroll_dir(obj, LV_DIR_ALL);
		lv_obj_scroll_to(obj, tv->scroll_center.x, tv->scroll_center.y, LV_ANIM_OFF);
	}

	/* 2) handle center tile floating */
	if (tv->cross_overlapped && tile_obj != tv->tile_center) {
		if (tile->dir & tv->cross_dir) {
			if (!lv_obj_has_flag(tv->tile_center, LV_OBJ_FLAG_FLOATING)) {
				lv_obj_add_flag(tv->tile_center, LV_OBJ_FLAG_FLOATING);
				lv_obj_set_pos(tv->tile_center, 0, 0);
			}
		}
		else if (lv_obj_has_flag(tv->tile_center, LV_OBJ_FLAG_FLOATING)) {
			lv_obj_clear_flag(tv->tile_center, LV_OBJ_FLAG_FLOATING);
			lv_obj_set_pos(tv->tile_center, tv->scroll_center.x, tv->scroll_center.y);
		}
	}

	/* 3) scroll to destination */
	if (tile_obj != tv->tile_act) {
		lv_coord_t tx = lv_obj_get_x(tile_obj);
		lv_coord_t ty = lv_obj_get_y(tile_obj);

		tv->tile_act = tile_obj;
		lv_obj_set_scroll_dir(obj, tile->dir);
		lv_obj_scroll_to(obj, tx, ty, LV_ANIM_OFF);
	}

	/* reset zoom */
	if (MAIN_SCROLL_ZOOM_MIN != LV_IMG_ZOOM_NONE)
		lv_obj_set_style_transform_zoom(tv->tile_act, LV_IMG_ZOOM_NONE, LV_PART_MAIN);

	lv_event_send(obj, LV_EVENT_VALUE_CHANGED, NULL);

	bypass_scroll_event = false;

	/* 4) adjust for main rotate */
	watch_tileview_adjust_main_rotate(obj, LV_ANIM_OFF);

	return LV_RES_OK;
}

lv_obj_t * watch_tileview_get_tile_act(lv_obj_t * obj)
{
	watch_tileview_t *tv = (watch_tileview_t *)obj;
	return tv->tile_act;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void watch_tileview_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
	watch_tileview_t *tv = (watch_tileview_t *)obj;

	tv->cross_dir = LV_DIR_VER;

	lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
	lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_ONE);
	lv_obj_set_scroll_snap_x(obj, LV_SCROLL_SNAP_CENTER);
	lv_obj_set_scroll_snap_y(obj, LV_SCROLL_SNAP_CENTER);

	lv_style_init(&tv->style);
	lv_style_set_bg_color(&tv->style, lv_color_black());
	lv_style_set_bg_opa(&tv->style, LV_OPA_COVER);
}

static void watch_tileview_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
	watch_tileview_t *tv = (watch_tileview_t *)obj;
	lv_style_reset(&tv->style);
}

static void watch_tileview_tile_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
	watch_tileview_tile_t *tile = (watch_tileview_tile_t *)obj;

	lv_obj_update_layout(obj);
	tile->dir = create_dir;
}

static lv_obj_t * watch_tileview_add_tile(lv_obj_t * obj, uint8_t col_id, uint8_t row_id, lv_dir_t dir)
{
	watch_tileview_t *tv = (watch_tileview_t *)obj;

	create_dir = dir;
	if (col_id == 0 && row_id == 0)
		lv_obj_set_scroll_dir(obj, create_dir);

	lv_obj_t *tile = lv_obj_class_create_obj(&watch_tileview_tile_class, obj);
	lv_obj_class_init_obj(tile);

	lv_obj_set_pos(tile, col_id * lv_obj_get_content_width(obj),
			row_id * lv_obj_get_content_height(obj));
	lv_obj_add_style(tile, &tv->style, LV_PART_MAIN | LV_STATE_DEFAULT);

	if (MAIN_SCROLL_ZOOM_MIN != LV_IMG_ZOOM_NONE) {
		lv_obj_set_style_transform_pivot_x(tile, lv_obj_get_content_width(obj) / 2, LV_PART_MAIN);
		lv_obj_set_style_transform_pivot_y(tile, lv_obj_get_content_height(obj) / 2, LV_PART_MAIN);
	}

	if (dir == LV_DIR_ALL) {
		tv->tile_center = tile;
		tv->scroll_center.x = lv_obj_get_style_x(tile, LV_PART_MAIN);
		tv->scroll_center.y = lv_obj_get_style_y(tile, LV_PART_MAIN);
	}
	else if (dir & tv->cross_dir) {
		lv_obj_move_foreground(tile);
	}

	return tile;
}

static void watch_tileview_set_cross_opa(lv_obj_t * obj, lv_opa_t opa)
{
	watch_tileview_t *tv = (watch_tileview_t *)obj;
	int i;

	for (i = lv_obj_get_child_cnt(obj) - 1; i >= tv->main_cnt; i--) {
		lv_obj_t *tile_obj = lv_obj_get_child(obj, i);
		lv_obj_set_style_opa(tile_obj, opa, LV_PART_MAIN);
	}
}

static void watch_tileview_set_main_zoom(lv_obj_t * obj, bool reset_nozoom)
{
	watch_tileview_t *tv = (watch_tileview_t *)obj;
	int i;

	lv_area_t tv_area;
	lv_obj_get_content_coords(obj, &tv_area);
	uint32_t tv_size = lv_area_get_size(&tv_area);

	for (i = tv->main_cnt - 1; i >= 0; i--) {
		lv_obj_t *tile_obj = lv_obj_get_child(obj, i);
		lv_area_t area;

		if (false == _lv_area_intersect(&area, &tile_obj->coords, &tv_area))
			continue;

		uint16_t zoom = LV_IMG_ZOOM_NONE;
		if (reset_nozoom == false) {
			zoom = MAIN_SCROLL_ZOOM_MIN + (LV_IMG_ZOOM_NONE - MAIN_SCROLL_ZOOM_MIN) *
					lv_area_get_size(&area) / tv_size;
		}

		lv_obj_set_style_transform_zoom(tile_obj, zoom, LV_PART_MAIN);
	}
}

static void watch_tileview_adjust_main_rotate(lv_obj_t * obj, lv_anim_enable_t anim_en)
{
	watch_tileview_t *tv = (watch_tileview_t *)obj;
	lv_dir_t main_dir = (tv->cross_dir & LV_DIR_HOR) ? LV_DIR_VER : LV_DIR_HOR;

	/* cannot use tile_act->dir, since maybe still anim scrolling */
	if (!tv->main_rotated || !(lv_obj_get_scroll_dir(obj) & main_dir))
		return;

	lv_coord_t content_w = lv_obj_get_content_width(obj);
	lv_coord_t content_h = lv_obj_get_content_height(obj);
	lv_coord_t scroll_x_max = content_w * (tv->main_cnt - 1);
	lv_coord_t scroll_y_max = content_h * (tv->main_cnt - 1);
	lv_obj_t *adjusted_main = NULL;
	int i;

	lv_point_t scroll_end;
	lv_obj_get_scroll_end(obj, &scroll_end);

	if ((main_dir == LV_DIR_HOR && scroll_end.x > 0 && scroll_end.x < scroll_x_max) ||
		(main_dir != LV_DIR_HOR && scroll_end.y > 0 && scroll_end.y < scroll_y_max))
		return;

	bypass_scroll_event = true;

	/* prepare to set positon */
	if (tv->tile_center && lv_obj_has_flag(tv->tile_center, LV_OBJ_FLAG_FLOATING)) {
		lv_obj_clear_flag(tv->tile_center, LV_OBJ_FLAG_FLOATING);
		lv_obj_set_pos(tv->tile_center, tv->scroll_center.x, tv->scroll_center.y);
	}

	if (main_dir == LV_DIR_HOR) {
		if (scroll_end.x == 0) {
			for (i = 0; i < tv->main_cnt - 1; i++) {
				lv_obj_t *tile_obj = lv_obj_get_child(obj, i);
				lv_obj_set_x(tile_obj, (i + 1) * content_w);
			}

			adjusted_main = lv_obj_get_child(obj, tv->main_cnt - 1);
			lv_obj_set_x(adjusted_main, 0);
			lv_obj_move_to_index(adjusted_main, 0);
			lv_obj_scroll_to_x(obj, lv_obj_get_scroll_x(obj) + content_w, LV_ANIM_OFF);
			scroll_end.x += content_w;
		}
		else if (scroll_end.x == scroll_x_max) {
			for (i = 1; i < tv->main_cnt; i++) {
				lv_obj_t *tile_obj = lv_obj_get_child(obj, i);
				lv_obj_set_x(tile_obj, (i - 1) * content_w);
			}

			adjusted_main = lv_obj_get_child(obj, 0);
			lv_obj_set_x(adjusted_main, scroll_x_max);
			lv_obj_move_to_index(adjusted_main, tv->main_cnt - 1);
			lv_obj_scroll_to_x(obj, lv_obj_get_scroll_x(obj) - content_w, LV_ANIM_OFF);
			scroll_end.x -= content_w;
		}

		tv->scroll_center.x = lv_obj_get_style_x(tv->tile_center, 0);

		for (i = tv->main_cnt; i < lv_obj_get_child_cnt(obj); i++) {
			lv_obj_t *tile_obj = lv_obj_get_child(obj, i);
			lv_obj_set_x(tile_obj, tv->scroll_center.x);
		}
	}
	else {
		if (scroll_end.y == 0) {
			for (i = 0; i < tv->main_cnt - 1; i++) {
				lv_obj_t *tile_obj = lv_obj_get_child(obj, i);
				lv_obj_set_y(tile_obj, (i + 1) * content_h);
			}

			adjusted_main = lv_obj_get_child(obj, tv->main_cnt - 1);
			lv_obj_set_y(adjusted_main, 0);
			lv_obj_move_to_index(adjusted_main, 0);
			lv_obj_scroll_to_y(obj, lv_obj_get_scroll_y(obj) + content_h, LV_ANIM_OFF);
			scroll_end.y += content_h;
		}
		else if (scroll_end.y == scroll_y_max) {
			for (i = 1; i < tv->main_cnt; i++) {
				lv_obj_t *tile_obj = lv_obj_get_child(obj, i);
				lv_obj_set_y(tile_obj, (i - 1) * content_h);
			}

			adjusted_main = lv_obj_get_child(obj, 0);
			lv_obj_set_y(adjusted_main, scroll_y_max);
			lv_obj_move_to_index(adjusted_main, tv->main_cnt - 1);
			lv_obj_scroll_to_y(obj, lv_obj_get_scroll_y(obj) - content_h, LV_ANIM_OFF);
			scroll_end.y -= content_h;
		}

		tv->scroll_center.y = lv_obj_get_style_y(tv->tile_center, 0);

		for (i = tv->main_cnt; i < lv_obj_get_child_cnt(obj); i++) {
			lv_obj_t *tile_obj = lv_obj_get_child(obj, i);
			lv_obj_set_y(tile_obj, tv->scroll_center.y);
		}
	}

	bypass_scroll_event = false;

	/* continue the anim scrolling */
	lv_obj_scroll_to(obj, scroll_end.x, scroll_end.y, anim_en);
}

static void watch_tileview_event_cb(const lv_obj_class_t * class_p, lv_event_t * e)
{
	LV_UNUSED(class_p);

	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *obj = lv_event_get_target(e);
	watch_tileview_t *tv = (watch_tileview_t *)obj;

	/*Call the ancestor's event handler*/
	lv_res_t res = lv_obj_event_base(MY_CLASS, e);
	if (res != LV_RES_OK) return;

	if (bypass_scroll_event) return;

	/*Ancestor events will be called during drawing*/
	if (code == LV_EVENT_SCROLL_BEGIN) {
		/* FIXME:
		 * if event param exist, this is invoked by scroll anim
		 */
		if (lv_event_get_param(e) == NULL) {
			lv_indev_t *indev = lv_indev_get_act();
			lv_dir_t dir = lv_indev_get_scroll_dir(indev);

			if (tv->cross_overlapped) {
				if (dir & tv->cross_dir) {
					if (CROSS_SCROLL_OPA < LV_OPA_COVER)
						watch_tileview_set_cross_opa(obj, CROSS_SCROLL_OPA);

					if (!lv_obj_has_flag(tv->tile_center, LV_OBJ_FLAG_FLOATING)) {
						lv_obj_add_flag(tv->tile_center, LV_OBJ_FLAG_FLOATING);
						lv_obj_set_pos(tv->tile_center, 0, 0);
					}
				}
				else if (lv_obj_has_flag(tv->tile_center, LV_OBJ_FLAG_FLOATING)) {
					lv_obj_clear_flag(tv->tile_center, LV_OBJ_FLAG_FLOATING);
					lv_obj_set_pos(tv->tile_center, tv->scroll_center.x, tv->scroll_center.y);
				}
			}

			/*
			 * Keep the scroll dir when scrolling by pressing just begin.
			 *
			 * If the scroll dir is LV_DIR_ALL during anim scrolling (only
			 * changed after the whole scrolling ends), the cross scrolling
			 * (relative to the current anim scrolling dir) by pressing will
			 * result in cross scrolling while the anim scroling still go on.
			 */
			lv_obj_set_scroll_dir(obj, dir);
		}
	}
	else if (code == LV_EVENT_SCROLL_END) {
		lv_indev_t* indev = lv_event_get_param(e);
		if (indev && indev->proc.state == LV_INDEV_STATE_PRESSED) {
			return;
		}

		lv_coord_t w = lv_obj_get_content_width(obj);
		lv_coord_t h = lv_obj_get_content_height(obj);

		lv_point_t scroll_end;
		lv_obj_get_scroll_end(obj, &scroll_end);

		lv_coord_t tx = ((scroll_end.x + (w / 2)) / w) * w;
		lv_coord_t ty = ((scroll_end.y + (h / 2)) / h) * h;

		for (int i = lv_obj_get_child_cnt(obj) - 1; i >= 0; i--) {
			lv_obj_t *tile_obj = lv_obj_get_child(obj, i);
			lv_coord_t x = lv_obj_get_x(tile_obj);
			lv_coord_t y = lv_obj_get_y(tile_obj);

			/* if has LV_FLAG_FLOATING, it must be tile_center at (0, 0) which has lower child index */
			if (lv_obj_has_flag(tile_obj, LV_OBJ_FLAG_FLOATING) || (x == tx && y == ty)) {
				if (tile_obj != tv->tile_act) {
					tv->tile_act = tile_obj;
					lv_event_send(obj, LV_EVENT_VALUE_CHANGED, NULL);
				}
				break;
			}
		}

		/* handle rotate mode */
		watch_tileview_adjust_main_rotate(obj, LV_ANIM_ON);

		/*
		 * Maybe still scrolling by pressing when previous anim scrolling end,
		 * since anim scrolling will not stopped before pressing scrolling.
		 */
		if (!indev && lv_obj_is_scrolling(obj))
			return;

		/* scroll_end may changed by watch_tileview_adjust_main_rotate() */
		lv_obj_get_scroll_end(obj, &scroll_end);

		if (scroll_end.x == lv_obj_get_scroll_x(obj) &&
			scroll_end.y == lv_obj_get_scroll_y(obj) &&
			scroll_end.x % w == 0 && scroll_end.y % h == 0) {
			watch_tileview_tile_t *tile = (watch_tileview_tile_t *)tv->tile_act;

			if (CROSS_SCROLL_OPA < LV_OPA_COVER && tv->cross_overlapped)
				watch_tileview_set_cross_opa(obj, LV_OPA_COVER);

			lv_obj_set_scroll_dir(obj, tile->dir);
		}
	}
	else if (code == LV_EVENT_SCROLL) {
		lv_dir_t dir = lv_obj_get_scroll_dir(obj);
		if (MAIN_SCROLL_ZOOM_MIN != LV_IMG_ZOOM_NONE && !(dir & tv->cross_dir)) {
			watch_tileview_set_main_zoom(obj, false);
		}
	}
}
