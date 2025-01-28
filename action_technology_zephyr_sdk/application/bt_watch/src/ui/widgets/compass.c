/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file compass.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "compass.h"
#include "img_label.h"
#include "simple_img.h"
#include "draw_util.h"
#include <lvgl/src/lvgl_private.h>

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &compass_class

/**********************
 *      TYPEDEFS
 **********************/
/** Data of anim image */
typedef struct {
	lv_obj_t obj;

	const lv_image_dsc_t *src_main;  /* main image */
	const lv_image_dsc_t *src_main_180;  /* main image with 180 degree rotation */
	const lv_image_dsc_t *src_decor; /* decor image */
	lv_obj_t *label;  /* the charactor image label */

	uint16_t hole_diameter; /* the circular hole diameter of the main image */
	uint16_t bearing;        /* clock-wise angle in degree */
} compass_t;

/**********************
 *  EXTERNAL PROTOTYPES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void compass_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void compass_event(const lv_obj_class_t * class_p, lv_event_t * e);

static void refresh_text(lv_obj_t * obj);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t compass_class = {
	.constructor_cb = compass_constructor,
	.event_cb = compass_event,
	.instance_size = sizeof(compass_t),
	.base_class = &lv_obj_class,
	.name = "compass",
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * compass_create(lv_obj_t * parent)
{
	LV_LOG_INFO("begin");
	lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
	lv_obj_class_init_obj(obj);
	return obj;
}

void compass_set_main_image(lv_obj_t *obj, const lv_image_dsc_t * src,
		const lv_image_dsc_t * src_rot180, uint16_t hole_diameter)
{
	compass_t * compass = (compass_t *)obj;

	if (src->header.w != src->header.h)
		return;

	compass->src_main = src;
	compass->src_main_180 = src_rot180;
	compass->hole_diameter = hole_diameter;

	lv_obj_set_width(obj, src->header.w);
	lv_obj_set_height(obj, src->header.h);
	lv_obj_invalidate(obj);

	refresh_text(obj);
}

void compass_set_decor_image(lv_obj_t *obj, const lv_image_dsc_t * src)
{
	compass_t * compass = (compass_t *)obj;

	compass->src_decor = src;
	lv_obj_invalidate(obj);
}

void compass_set_char_image(lv_obj_t *obj, const lv_image_dsc_t * srcs, uint16_t count)
{
	compass_t * compass = (compass_t *)obj;

	if (srcs == NULL || count != NUM_COMPASS_CHARS)
		return;

	img_label_set_src(compass->label, srcs, count);
}

void compass_set_bearing(lv_obj_t *obj, uint16_t bearing)
{
	compass_t * compass = (compass_t *)obj;

	if (bearing >= 360 || bearing == compass->bearing)
		return;

	compass->bearing = bearing;

	if (compass->src_main) {
		refresh_text(obj);
		lv_obj_invalidate(obj);
	}
}

uint16_t compass_get_bearing(lv_obj_t *obj)
{
	compass_t * compass = (compass_t *)obj;
	return compass->bearing;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void compass_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
	LV_UNUSED(class_p);
	LV_TRACE_OBJ_CREATE("begin");

	compass_t * compass = (compass_t *)obj;

	compass->label = img_label_create(obj);
	/* FIXME: should align label position to even pixels */
	//lv_obj_center(compass->label);

	lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
	LV_TRACE_OBJ_CREATE("finished");
}

static void compass_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * obj = lv_event_get_target(e);
	compass_t * compass = (compass_t *)obj;

	/*Ancestor events will be called during drawing*/
	if (code != LV_EVENT_COVER_CHECK && code != LV_EVENT_DRAW_MAIN && code != LV_EVENT_DRAW_POST) {
		/*Call the ancestor's event handler*/
		lv_result_t res = lv_obj_event_base(MY_CLASS, e);
		if(res != LV_RESULT_OK) return;
	}

	if (code == LV_EVENT_COVER_CHECK) {
		lv_cover_check_info_t * info = lv_event_get_param(e);
		if (info->res == LV_COVER_RES_MASKED) return;

		if (compass->src_main == NULL) {
			info->res = LV_COVER_RES_NOT_COVER;
			return;
		}

		if (compass->bearing != 0) {
			info->res = LV_COVER_RES_NOT_COVER;
			return;
		}

		info->res = lv_area_is_in(info->area, &obj->coords, 0) ?
				LV_COVER_RES_COVER : LV_COVER_RES_NOT_COVER;
	} else if (code == LV_EVENT_DRAW_MAIN) {
		lv_layer_t * layer = lv_event_get_layer(e);

		if (!lv_area_is_on(&layer->_clip_area, &obj->coords)) return;

		lv_draw_image_dsc_t draw_img_dsc;
		lv_draw_image_dsc_init(&draw_img_dsc);

		if (compass->src_main) {
			draw_img_dsc.rotation = compass->bearing ? (3600 - compass->bearing * 10) : 0;
			draw_img_dsc.pivot.x = compass->src_main->header.w / 2;
			draw_img_dsc.pivot.y = compass->src_main->header.h / 2;
			draw_img_dsc.src = compass->src_main;
			lv_draw_image(layer, &draw_img_dsc, &obj->coords);
		}

		if (compass->src_decor) {
			lv_area_t decor_area;

			/* make sure the position is even pixels aligned */
			decor_area.x1 = ((obj->coords.x1 + obj->coords.x2 + 1 - compass->src_decor->header.w) / 2) & ~0x1;
			decor_area.y1 = (obj->coords.y1 + obj->coords.y2 + 1 - compass->src_decor->header.h ) / 2;
			decor_area.x2 = decor_area.x1 + compass->src_decor->header.w - 1;
			decor_area.y2 = decor_area.y1 + compass->src_decor->header.h - 1;
			draw_img_dsc.rotation = 0;
			draw_img_dsc.src = compass->src_decor;
			lv_draw_image(layer, &draw_img_dsc, &decor_area);
		}
	}
}

static void refresh_text(lv_obj_t * obj)
{
	static const uint8_t text_indices[][2] = {
		{ IMG_LABEL_INVALID_IDX, COMPASS_CHAR_NORTH, }, /* due-north */
		{ COMPASS_CHAR_EAST, COMPASS_CHAR_NORTH, }, /* north-east */
		{ IMG_LABEL_INVALID_IDX, COMPASS_CHAR_EAST, }, /* due-east */
		{ COMPASS_CHAR_EAST, COMPASS_CHAR_SOUTH, }, /* south-east */
		{ IMG_LABEL_INVALID_IDX, COMPASS_CHAR_SOUTH, }, /* due-south */
		{ COMPASS_CHAR_WEST, COMPASS_CHAR_SOUTH, }, /* south-west */
		{ IMG_LABEL_INVALID_IDX, COMPASS_CHAR_WEST, }, /* due-west */
		{ COMPASS_CHAR_WEST, COMPASS_CHAR_NORTH, }, /* north-west */
	};
	compass_t * compass = (compass_t *)obj;
	uint16_t bearing;
	uint8_t indices[6];
	int count;

	bearing = compass->bearing * 10 + 225;
	if (bearing >= 3600)
		bearing -= 3600;

	count = split_decimal(compass->bearing, indices, sizeof(indices), 0);
	indices[count++] = COMPASS_CHAR_DEGREE;
	lv_memcpy(&indices[count], text_indices[bearing / 450], 2);
	count += 2;

	img_label_set_text(compass->label, indices, count);

	/* FIXME: should align label position to even pixels */
	lv_obj_align_to(compass->label, obj, LV_ALIGN_CENTER, 0, 0);
	if (lv_obj_get_style_x(compass->label, LV_PART_MAIN) & 0x1)
		lv_obj_set_x(compass->label, lv_obj_get_style_x(compass->label, LV_PART_MAIN) & ~0x1);
}
