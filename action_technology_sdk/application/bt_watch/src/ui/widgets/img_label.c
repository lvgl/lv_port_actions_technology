/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file img_label.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <sys/util.h>
#include "draw_util.h"
#include "img_label.h"
#include <lvgl/src/lvgl_private.h>

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &img_label_class

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void img_label_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void img_label_event(const lv_obj_class_t * class_p, lv_event_t * e);
static void draw_label(lv_event_t * e);

static void refresh_area(lv_obj_t * obj);
static void refresh_text(lv_obj_t * obj);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t img_label_class = {
	.constructor_cb = img_label_constructor,
	.event_cb = img_label_event,
	.width_def = LV_SIZE_CONTENT,
	.height_def = LV_SIZE_CONTENT,
	.instance_size = sizeof(img_label_t),
	.base_class = &lv_obj_class,
	.name = "img-label",
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * img_label_create(lv_obj_t * parent)
{
	LV_LOG_INFO("begin");
	lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
	lv_obj_class_init_obj(obj);
	return obj;
}

void img_label_set_align(lv_obj_t *obj, lv_align_t align)
{
	img_label_t * label = (img_label_t *)obj;

	if (align > LV_ALIGN_RIGHT_MID)
		return;

	if (label->align != align) {
		label->align = align;
		refresh_text(obj);
	}
}

void img_label_set_src(lv_obj_t *obj, const lv_image_dsc_t * chars, uint8_t cnt)
{
	img_label_t * label = (img_label_t *)obj;

	label->src_chars = chars;
	label->num_chars = cnt;
	refresh_text(obj);
}

void img_label_set_text(lv_obj_t * obj, const uint8_t * indices, uint8_t cnt)
{
	img_label_t * label = (img_label_t *)obj;

	label->count = LV_MIN(cnt, IMG_LABEL_MAX_COUNT);
	lv_memcpy(label->indices, indices, sizeof(*indices) * label->count);
	refresh_text(obj);
}

uint32_t img_label_get_size(lv_obj_t *obj)
{
	img_label_t * label = (img_label_t *)obj;

	return label->count;
}

void img_label_split_src(lv_obj_t *obj, lv_image_dsc_t * chars,
		const lv_image_dsc_t * src, uint8_t cnt, bool vertical)
{
	uint32_t data_size = src->data_size / cnt;
	uint16_t w = vertical ? src->header.w : (src->header.w / cnt);
	uint16_t h = vertical ? (src->header.h / cnt) : src->header.h;
	uint16_t stride = vertical ? src->header.stride : (src->header.stride / cnt);
	const uint8_t *data = src->data;

	for (int i = 0; i < cnt; i++) {
		chars[i].header.magic = LV_IMAGE_HEADER_MAGIC;
		chars[i].header.cf = src->header.cf;
		chars[i].header.w = w;
		chars[i].header.h = h;
		chars[i].header.stride = stride;
		chars[i].data_size = data_size;
		chars[i].data = data;

		data += data_size;
	}
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void img_label_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
	LV_UNUSED(class_p);
	LV_TRACE_OBJ_CREATE("begin");

	img_label_t *label = (img_label_t *)obj;

	lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
	label->align = LV_ALIGN_CENTER;

	LV_TRACE_OBJ_CREATE("finished");
}

static void img_label_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
	LV_UNUSED(class_p);

	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * obj = lv_event_get_target(e);
	img_label_t *label = (img_label_t *)obj;

	/*Ancestor events will be called during drawing*/
	if (code != LV_EVENT_COVER_CHECK && code != LV_EVENT_DRAW_MAIN && code != LV_EVENT_DRAW_POST) {
		/*Call the ancestor's event handler*/
		lv_result_t res = lv_obj_event_base(MY_CLASS, e);
		if(res != LV_RESULT_OK) return;
	}

	if (code == LV_EVENT_SIZE_CHANGED) {
		lv_area_t *ori = lv_event_get_param(e);

		if (lv_area_get_width(ori) != lv_obj_get_width(obj) ||
			lv_area_get_height(ori) != lv_obj_get_height(obj)) {
			refresh_area(obj);
		}
	} else if (code == LV_EVENT_GET_SELF_SIZE) {
		lv_point_t * p = lv_event_get_param(e);
		p->x = lv_area_get_width(&label->area);
		p->y = lv_area_get_height(&label->area);
	} else if (code == LV_EVENT_DRAW_MAIN || code == LV_EVENT_DRAW_POST || code == LV_EVENT_COVER_CHECK) {
		draw_label(e);
	}
}

static void draw_label(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * obj = lv_event_get_target(e);
	img_label_t *label = (img_label_t *)obj;
	lv_area_t abs_area;

	lv_area_set(&abs_area,
		label->area.x1 + obj->coords.x1, label->area.y1 + obj->coords.y1,
		label->area.x2 + obj->coords.x1, label->area.y2 + obj->coords.y1);

	if (code == LV_EVENT_COVER_CHECK) {
		lv_cover_check_info_t * info = lv_event_get_param(e);
		if (info->res == LV_COVER_RES_MASKED) return;

		if (label->src_chars == NULL || label->same_height == 0) {
			info->res = LV_COVER_RES_NOT_COVER;
			return;
		}

        /*Non true color format might have "holes"*/
        if (lv_color_format_has_alpha(label->src_chars[0].header.cf)) {
            info->res = LV_COVER_RES_NOT_COVER;
            return;
        }

        /*With not LV_OPA_COVER images can't cover an area */
        if (lv_obj_get_style_image_opa(obj, LV_PART_MAIN) != LV_OPA_COVER) {
            info->res = LV_COVER_RES_NOT_COVER;
            return;
        }

		if (lv_area_is_in(info->area, &abs_area, 0) == false) {
			info->res = LV_COVER_RES_NOT_COVER;
			return;
		}

		info->res = LV_COVER_RES_COVER;
	} else if (code == LV_EVENT_DRAW_MAIN) {
		if (label->src_chars == NULL)
			return;

		lv_layer_t * layer = lv_event_get_layer(e);

		if (lv_area_is_on(&abs_area, &layer->_clip_area) == false)
			return;

		lv_draw_image_dsc_t img_dsc;
		lv_draw_image_dsc_init(&img_dsc);
		img_dsc.opa = lv_obj_get_style_image_opa(obj, LV_PART_MAIN);
		img_dsc.recolor = lv_obj_get_style_image_recolor(obj, LV_PART_MAIN);
		img_dsc.recolor_opa = lv_obj_get_style_image_recolor_opa(obj, LV_PART_MAIN);

		lv_area_t area;
		area.x1 = abs_area.x1;

		for (int i = 0; i < label->count; i++) {
			if (label->indices[i] >= label->num_chars)
				continue;

			const lv_image_dsc_t *src = &label->src_chars[label->indices[i]];
			area.x2 = area.x1 + src->header.w - 1;
			/* align to the middle of area */
			area.y1 = (abs_area.y1 + abs_area.y2 + 1 - src->header.h) / 2;
			area.y2 = area.y1 + src->header.h - 1;

			if (lv_area_is_on(&area, &layer->_clip_area)) {
				img_dsc.src = src;
				lv_draw_image(layer, &img_dsc, &area);
			}

			area.x1 += src->header.w;
		}
	}
}

static void refresh_area(lv_obj_t * obj)
{
	img_label_t *label = (img_label_t *)obj;

	lv_area_align(&obj->coords, &label->area, label->align, 0, 0);
	lv_area_move(&label->area, -obj->coords.x1, -obj->coords.y1);
}

static void refresh_text(lv_obj_t * obj)
{
	img_label_t *label = (img_label_t *)obj;
	lv_coord_t w = 0, h = 0;
	lv_area_t abs_area;

	if (label->num_chars == 0 || label->count == 0) {
		return;
	}

	label->same_height = 1;

	for (int i = 0; i < label->count; i++) {
		if (label->indices[i] >= label->num_chars)
			continue;

		const lv_image_dsc_t *src = &label->src_chars[label->indices[i]];

		if (h > 0 && h != src->header.h)
			label->same_height = 0;

		w += src->header.w;
		h = LV_MAX(h, src->header.h);
	}

	lv_area_set(&abs_area,
		label->area.x1 + obj->coords.x1, label->area.y1 + obj->coords.y1,
		label->area.x2 + obj->coords.x1, label->area.y2 + obj->coords.y1);
	lv_obj_invalidate_area(obj, &abs_area);

	lv_area_set(&label->area, 0, 0, w - 1, h - 1);

	lv_area_align(&obj->coords, &label->area, label->align, 0, 0);
	lv_area_move(&label->area, -obj->coords.x1, -obj->coords.y1);

	lv_area_set(&abs_area,
		label->area.x1 + obj->coords.x1, label->area.y1 + obj->coords.y1,
		label->area.x2 + obj->coords.x1, label->area.y2 + obj->coords.y1);

	lv_obj_refresh_self_size(obj);
	lv_obj_invalidate_area(obj, &abs_area);
}
