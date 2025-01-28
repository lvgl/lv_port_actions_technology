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
#include "simple_img.h"
#include "draw_util.h"
#include <lvgl/src/lvgl_private.h>

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &simple_img_class

/**********************
 *      TYPEDEFS
 **********************/

/** Data of rotate image */
typedef struct {
	lv_obj_t obj;

	const void *src; /* image source */
	lv_image_header_t header; /* image header */
	lv_area_t trans_area;   /* image transformed area (relative to the top-left of the object) */
	lv_point_t pivot;  /* Rotation center of the image, relative to the top-left corner of the image */
	uint16_t rotation; /* Rotation angle in 0.1 degree [0, 3600) of the image */
	uint16_t scale;    /* 256 means no zoom, 512 double size, 128 half size */
} simple_img_t;

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
	.base_class = &lv_obj_class,
	.name = "simple-img",
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

	simple_img_t * img = (simple_img_t *)obj;
	lv_image_header_t header;

	if (src) {
		lv_image_src_t src_type = lv_image_src_get_type(src);
		if (src_type != LV_IMAGE_SRC_VARIABLE)
			return;

		if (lv_image_decoder_get_info(src, &header) != LV_RESULT_OK)
			return;
	}

	img->src = src;
	if (src) {
		lv_memcpy(&img->header, &header, sizeof(header));
		img->pivot.x = img->header.w / 2;
		img->pivot.y = img->header.h / 2;

		refresh_image_area(obj);
		lv_obj_refresh_self_size(obj);
	} else {
		lv_obj_invalidate(obj);
	}
}

void simple_img_set_pivot(lv_obj_t * obj, lv_coord_t pivot_x, lv_coord_t pivot_y)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	simple_img_t * img = (simple_img_t *)obj;

	if (img->pivot.x != pivot_x || img->pivot.y != pivot_y) {
		img->pivot.x = pivot_x;
		img->pivot.y = pivot_y;

		if (img->src && (img->rotation > 0 || img->scale != LV_SCALE_NONE))
			refresh_image_area(obj);
	}
}

void simple_img_set_rotation(lv_obj_t *obj, int16_t angle)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	simple_img_t * img = (simple_img_t *)obj;

	while (angle >= 3600) angle -= 3600;
	while (angle < 0) angle += 3600;

	if (angle != img->rotation) {
		img->rotation = angle;

		if (img->src)
			refresh_image_area(obj);
	}
}

void simple_img_set_scale(lv_obj_t * obj, uint16_t zoom)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	simple_img_t * img = (simple_img_t *)obj;

	if (zoom == 0) zoom = 1;

	if (zoom != img->scale) {
		img->scale = zoom;

		if (img->src)
			refresh_image_area(obj);
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

uint16_t simple_image_get_rotation(lv_obj_t * obj)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	simple_img_t * img = (simple_img_t *)obj;

	return img->rotation;
}

uint16_t simple_image_get_scale(lv_obj_t * obj)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	simple_img_t * img = (simple_img_t *)obj;

	return img->scale;
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

	if (img->rotation > 0 || img->scale != LV_SCALE_NONE) {
		lv_image_buf_get_transformed_area(&img->trans_area, img->header.w,
				img->header.h, img->rotation, img->scale, img->scale, &img->pivot);
	} else {
		img->trans_area.x1 = 0;
		img->trans_area.y1 = 0;
		img->trans_area.x2 = img->header.w - 1;
		img->trans_area.y2 = img->header.h - 1;
	}

	/* Disable invalidations because lv_obj_refresh_ext_draw_size would invalidate
	 * the whole ext draw area */
	lv_display_t * disp = lv_obj_get_display(obj);
	lv_display_enable_invalidation(disp, false);
	lv_obj_refresh_ext_draw_size(obj);
	lv_display_enable_invalidation(disp, true);

	invalidate_image_area(obj);
}

static void simple_img_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
	simple_img_t * img = (simple_img_t *)obj;

	LV_UNUSED(class_p);
	LV_TRACE_OBJ_CREATE("begin");

	lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_flag(obj, LV_OBJ_FLAG_ADV_HITTEST);

	img->scale = LV_SCALE_NONE;

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
		lv_result_t res = lv_obj_event_base(MY_CLASS, e);
		if (res != LV_RESULT_OK) return;
	}

	if (code == LV_EVENT_STYLE_CHANGED) {
		if (img->rotation > 0 || img->scale != LV_SCALE_NONE) {
			/*With transformation it might change*/
			lv_obj_refresh_ext_draw_size(obj);
		}
	} else if (code == LV_EVENT_REFR_EXT_DRAW_SIZE) {
		if (img->rotation > 0 || img->scale != LV_SCALE_NONE) {
			int32_t * s = lv_event_get_param(e);
			int32_t w = lv_obj_get_width(obj);
			int32_t h = lv_obj_get_height(obj);

			*s = LV_MAX(*s, -img->trans_area.x1);
			*s = LV_MAX(*s, -img->trans_area.y1);
			*s = LV_MAX(*s, img->trans_area.x2 - w + 1);
			*s = LV_MAX(*s, img->trans_area.y2 - h + 1);
		}
	} else if (code == LV_EVENT_HIT_TEST) {
		lv_hit_test_info_t * info = lv_event_get_param(e);

		if (img->rotation > 0 || img->scale != LV_SCALE_NONE) {
			lv_area_t coords;
			coords.x1 = img->trans_area.x1 + obj->coords.x1;
			coords.y1 = img->trans_area.y1 + obj->coords.y1;
			coords.x2 = img->trans_area.x2 + obj->coords.x1;
			coords.y2 = img->trans_area.y2 + obj->coords.y1;

			info->res = lv_area_is_point_on(&coords, info->point, 0);
		} else {
            lv_area_t a;
            lv_obj_get_click_area(obj, &a);
            info->res = lv_area_is_point_on(&a, info->point, 0);
		}
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

	if (code == LV_EVENT_COVER_CHECK) {
		lv_cover_check_info_t * info = lv_event_get_param(e);
		if (info->res == LV_COVER_RES_MASKED) return;

		if (img->rotation != 0 || img->src == NULL) {
			info->res = LV_COVER_RES_NOT_COVER;
			return;
		}

        /*Non true color format might have "holes"*/
        if (lv_color_format_has_alpha(img->header.cf)) {
            info->res = LV_COVER_RES_NOT_COVER;
            return;
        }

        /*With not LV_OPA_COVER images can't cover an area */
        if (lv_obj_get_style_image_opa(obj, LV_PART_MAIN) != LV_OPA_COVER) {
            info->res = LV_COVER_RES_NOT_COVER;
            return;
        }

		lv_area_set(&coords, img->trans_area.x1 + obj->coords.x1,
					img->trans_area.y1 + obj->coords.y1,
					img->trans_area.x2 + obj->coords.x1,
					img->trans_area.y2 + obj->coords.y1);
		if (lv_area_is_in(info->area, &coords, 0) == false) {
			info->res = LV_COVER_RES_NOT_COVER;
			return;
		}
	} else if (code == LV_EVENT_DRAW_MAIN) {
		if (img->src == NULL)
			return;

		lv_layer_t * layer = lv_event_get_layer(e);

		lv_draw_image_dsc_t draw_dsc;
		lv_draw_image_dsc_init(&draw_dsc);
		//lv_obj_init_draw_img_dsc(obj, LV_IMG_PART_MAIN, &draw_dsc);
		draw_dsc.recolor = lv_obj_get_style_image_recolor_filtered(obj, LV_PART_MAIN);
		draw_dsc.recolor_opa = lv_obj_get_style_image_recolor_opa(obj, LV_PART_MAIN);
		draw_dsc.opa = lv_obj_get_style_image_opa(obj, LV_PART_MAIN);
		draw_dsc.rotation = img->rotation;
		draw_dsc.scale_x = img->scale;
		draw_dsc.scale_y = img->scale;
		draw_dsc.pivot.x = img->pivot.x;
		draw_dsc.pivot.y = img->pivot.y;
		draw_dsc.src = img->src;

		/* FIXME: need tile as lv_img ? */
		lv_area_set(&coords, obj->coords.x1, obj->coords.y1,
					obj->coords.x1 + img->header.w - 1,
					obj->coords.y1 + img->header.h - 1);
		lv_draw_image(layer, &draw_dsc, &coords);
	}
}
