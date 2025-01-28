/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file simple_img.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <sys/byteorder.h>
#include <lvgl/src/misc/lv_style.h>
#include "simple_img.h"
#include "draw_util.h"

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &simple_img_class

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void simple_img_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void simple_img_event(const lv_obj_class_t * class_p, lv_event_t * e);
static void draw_img(lv_event_t * e);

static void invalidate_image_area(lv_obj_t * img);
static void refresh_image_area(lv_obj_t * img);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t simple_img_class = {
	.constructor_cb = simple_img_constructor,
	.event_cb = simple_img_event,
	.width_def = LV_SIZE_CONTENT,
	.height_def = LV_SIZE_CONTENT,
	.instance_size = sizeof(simple_img_t),
	.base_class = &lv_obj_class
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * simple_img_create(lv_obj_t * parent)
{
	LV_LOG_INFO("begin");
	lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
	lv_obj_class_init_obj(obj);
	return obj;
}

/*=====================
 * Setter functions
 *====================*/

void simple_img_set_src(lv_obj_t *obj, const void *src)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	if (src) {
		lv_img_src_t src_type = lv_img_src_get_type(src);
		if (src_type != LV_IMG_SRC_VARIABLE)
			return;
	}

	simple_img_t * img = (simple_img_t *)obj;
	lv_obj_invalidate(obj);

	img->src = src;
	if (img->src) {
		lv_img_decoder_get_info(src, &img->header);

		img->pivot.x = img->header.w / 2;
		img->pivot.y = img->header.h / 2;

		refresh_image_area(obj);
		lv_obj_refresh_self_size(obj);
	}
}

void simple_img_set_pivot(lv_obj_t * obj, lv_coord_t pivot_x, lv_coord_t pivot_y)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	simple_img_t * img = (simple_img_t *)obj;

	if (img->rounded)
		return;

	if (img->pivot.x != pivot_x || img->pivot.y != pivot_y) {
		img->pivot.x = pivot_x;
		img->pivot.y = pivot_y;

		if (img->src && (img->angle > 0 || img->zoom != LV_IMG_ZOOM_NONE))
			refresh_image_area(obj);
	}
}

void simple_img_set_angle(lv_obj_t *obj, int16_t angle)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	simple_img_t * img = (simple_img_t *)obj;

	while (angle >= 3600) angle -= 3600;
	while (angle < 0) angle += 3600;

	if (angle != img->angle) {
		img->angle = angle;

		if (img->src)
			refresh_image_area(obj);
	}
}

void simple_img_set_zoom(lv_obj_t * obj, uint16_t zoom)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	simple_img_t * img = (simple_img_t *)obj;

	if (zoom == 0) zoom = 1;

	if (zoom != img->zoom) {
		img->zoom = zoom;

		if (img->src)
			refresh_image_area(obj);
	}
}

void simple_img_set_rounded(lv_obj_t * obj, bool rounded)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	simple_img_t * img = (simple_img_t *)obj;

	if (img->rounded != rounded) {
		img->rounded = rounded;

		if (img->rounded && img->src) {
			img->pivot.x = img->header.w / 2;
			img->pivot.y = img->header.h / 2;

			if (img->angle > 0 && img->zoom != LV_IMG_ZOOM_NONE)
				refresh_image_area(obj);
		}
	}
}

const void * simple_img_get_src(lv_obj_t * obj)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	simple_img_t * img = (simple_img_t *)obj;

	return img->src;
}

void simple_img_get_pivot(lv_obj_t * obj, lv_point_t * pivot)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	simple_img_t * img = (simple_img_t *)obj;

	*pivot = img->pivot;
}

uint16_t simple_img_get_angle(lv_obj_t * obj)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	simple_img_t * img = (simple_img_t *)obj;

	return img->angle;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void invalidate_image_area(lv_obj_t * obj)
{
	simple_img_t * img = (simple_img_t *)obj;

	lv_area_t abs_area;
	abs_area.x1 = img->trans_area.x1 + obj->coords.x1;
	abs_area.y1 = img->trans_area.y1 + obj->coords.y1;
	abs_area.x2 = img->trans_area.x2 + obj->coords.x1;
	abs_area.y2 = img->trans_area.y2 + obj->coords.y1;

	lv_obj_invalidate_area(obj, &abs_area);
}

static void refresh_image_area(lv_obj_t * obj)
{
	simple_img_t * img = (simple_img_t *)obj;

	invalidate_image_area(obj);

	if (img->angle > 0 || img->zoom != LV_IMG_ZOOM_NONE) {
		_lv_img_buf_get_transformed_area(&img->trans_area, img->header.w,
				img->header.h, img->angle, img->zoom, &img->pivot);

		if (img->angle > 0 && img->rounded) {
			lv_area_t bbox_area;
			_lv_img_buf_get_transformed_area(&bbox_area, img->header.w,
					img->header.h, 0, img->zoom, &img->pivot);
			_lv_area_intersect(&img->trans_area, &img->trans_area, &bbox_area);
		}

		lv_obj_refresh_ext_draw_size(obj);
	} else {
		img->trans_area.x1 = 0;
		img->trans_area.y1 = 0;
		img->trans_area.x2 = img->header.w - 1;
		img->trans_area.y2 = img->header.h - 1;
	}

	invalidate_image_area(obj);
}

static void simple_img_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
	simple_img_t * img = (simple_img_t *)obj;

	LV_UNUSED(class_p);
	LV_TRACE_OBJ_CREATE("begin");

	lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_flag(obj, LV_OBJ_FLAG_ADV_HITTEST);

	img->zoom = LV_IMG_ZOOM_NONE;

	LV_TRACE_OBJ_CREATE("finished");
}

static void simple_img_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
	LV_UNUSED(class_p);

	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * obj = lv_event_get_target(e);
	simple_img_t * img = (simple_img_t *)obj;

	/*Ancestor events will be called during drawing*/
	if (code != LV_EVENT_COVER_CHECK && code != LV_EVENT_DRAW_MAIN && code != LV_EVENT_DRAW_POST) {
		/*Call the ancestor's event handler*/
		lv_res_t res = lv_obj_event_base(MY_CLASS, e);
		if (res != LV_RES_OK) return;
	}

	if (code == LV_EVENT_STYLE_CHANGED) {
		if (img->angle > 0 || img->zoom != LV_IMG_ZOOM_NONE) {
			/*With transformation it might change*/
			lv_obj_refresh_ext_draw_size(obj);
		}
	} else if (code == LV_EVENT_REFR_EXT_DRAW_SIZE) {
		if (img->angle > 0 || img->zoom != LV_IMG_ZOOM_NONE) {
			lv_coord_t * s = lv_event_get_param(e);
			lv_coord_t pad_ori = *s;
			lv_coord_t w = lv_obj_get_width(obj);
			lv_coord_t h = lv_obj_get_height(obj);

			*s = LV_MAX(*s, pad_ori - img->trans_area.x1);
			*s = LV_MAX(*s, pad_ori - img->trans_area.y1);
			*s = LV_MAX(*s, pad_ori + img->trans_area.x2 - w + 1);
			*s = LV_MAX(*s, pad_ori + img->trans_area.y2 - h + 1);
		}
	} else if (code == LV_EVENT_HIT_TEST) {
		lv_hit_test_info_t * info = lv_event_get_param(e);
		lv_area_t coords;
		coords.x1 = img->trans_area.x1 + obj->coords.x1;
		coords.y1 = img->trans_area.y1 + obj->coords.y1;
		coords.x2 = img->trans_area.x2 + obj->coords.x1;
		coords.y2 = img->trans_area.y2 + obj->coords.y1;

		info->res = _lv_area_is_point_on(&coords, info->point, 0);
	} else if (code == LV_EVENT_GET_SELF_SIZE) {
		lv_point_t * p = lv_event_get_param(e);
		if (img->src) {
			p->x = img->header.w;
			p->y = img->header.h;
		}
	} else if (code == LV_EVENT_DRAW_MAIN || code == LV_EVENT_DRAW_POST || code == LV_EVENT_COVER_CHECK) {
		draw_img(e);
	}
}

static void draw_img(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * obj = lv_event_get_target(e);
	simple_img_t * img = (simple_img_t *)obj;
	lv_area_t coords;

	lv_area_set(&coords,
		img->trans_area.x1 + obj->coords.x1, img->trans_area.y1 + obj->coords.y1,
		img->trans_area.x2 + obj->coords.x1, img->trans_area.y2 + obj->coords.y1);

	if (code == LV_EVENT_COVER_CHECK) {
		lv_cover_check_info_t * info = lv_event_get_param(e);
		if (info->res == LV_COVER_RES_MASKED) return;

		if (img->angle != 0 || img->src == NULL) {
			info->res = LV_COVER_RES_NOT_COVER;
			return;
		}

		if (img->header.cf != LV_IMG_CF_TRUE_COLOR && img->header.cf != LV_IMG_CF_RAW) {
			info->res = LV_COVER_RES_NOT_COVER;
			return;
		}

		/*With not LV_OPA_COVER images can't cover an area */
		if (lv_obj_get_style_img_opa(obj, LV_PART_MAIN) != LV_OPA_COVER) {
			info->res = LV_COVER_RES_NOT_COVER;
			return;
		}

		if (_lv_area_is_in(info->area, &coords, 0) == false) {
			info->res = LV_COVER_RES_NOT_COVER;
			return;
		}

		info->res = LV_COVER_RES_COVER;
	} else if (code == LV_EVENT_DRAW_MAIN) {
		lv_draw_ctx_t * draw_ctx = lv_event_get_draw_ctx(e);

		if (img->src == NULL)
			return;

		lv_draw_img_dsc_t draw_dsc;
		lv_draw_img_dsc_init(&draw_dsc);
		//lv_obj_init_draw_img_dsc(obj, LV_IMG_PART_MAIN, &draw_dsc);
		draw_dsc.recolor = lv_obj_get_style_img_recolor_filtered(obj, LV_PART_MAIN);
		draw_dsc.recolor_opa = lv_obj_get_style_img_recolor_opa(obj, LV_PART_MAIN);
		draw_dsc.opa = lv_obj_get_style_img_opa(obj, LV_PART_MAIN);
		draw_dsc.angle = img->angle;
		draw_dsc.zoom = img->zoom;
		draw_dsc.pivot.x = img->pivot.x;
		draw_dsc.pivot.y = img->pivot.y;

		lv_draw_img(draw_ctx, &draw_dsc, &obj->coords, img->src);
	}
}
