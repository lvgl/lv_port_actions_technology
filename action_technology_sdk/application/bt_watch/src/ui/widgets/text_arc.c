/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file text_arc.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <assert.h>
#include <ui_mem.h>
#include "text_arc.h"
#include <widgets/text_canvas.h>
#include <stdio.h>
#include <math.h>
#include <ui_mem.h>
#include <memory/mem_cache.h>
#include <display/ui_memsetcpy.h>
#ifdef CONFIG_LVGL_USE_FREETYPE_FONT
#include <lvgl/lvgl_freetype_font.h>
#endif
#include <lvgl/src/lvgl_private.h>

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &text_arc_class
#define PI 3.141592654


/**********************
 *      TYPEDEFS
 **********************/
/** Data of text canvas */
typedef struct {
	lv_obj_t obj; /*Ext. of ancestor*/
	void *text_canvas;
	char *text;
	const lv_font_t *font;
	fine_tuning_angle_cb_t fine_tuning_angle_cb;
	uint16_t radius;
	int16_t angle_st;
	int16_t interval_angle;
	uint8_t b_center : 1;
	uint8_t b_overturn : 1;
} text_arc_t;

typedef struct {
	uint16_t letter_w;
	int16_t st_angle;
	int16_t end_angle;
	uint16_t ofs_text;
	uint8_t text_len;
} text_id_t;

/**********************
 *  EXTERNAL PROTOTYPES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void text_arc_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void text_arc_event(const lv_obj_class_t * class_p, lv_event_t * e);
static void text_arc_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t text_arc_class = {
	.destructor_cb = text_arc_destructor,
	.event_cb = text_arc_event,
	.constructor_cb = text_arc_constructor,
	.width_def = LV_SIZE_CONTENT,
	.height_def = LV_SIZE_CONTENT,
	.instance_size = sizeof(text_arc_t),
	.base_class = &lv_obj_class,
	.name = "text-arc",
};

static void text_arc_set_angle(lv_obj_t * obj, int16_t angle , lv_area_t *area)
{
    while(angle >= 3600) angle -= 3600;
    while(angle < 0) angle += 3600;

    lv_image_t * img = (lv_image_t *)obj;

    lv_obj_update_layout(obj);  /*Be sure the object's size is calculated*/
	if(angle == img->rotation)
	{
		lv_memcpy(area,&obj->coords,sizeof(*area));
		return;
	}
    lv_coord_t w = lv_obj_get_width(obj);
    lv_coord_t h = lv_obj_get_height(obj);
	img->pivot.x = w / 2;
	img->pivot.y = h;
    lv_area_t a;
    lv_image_buf_get_transformed_area(&a, w, h, img->rotation, img->scale_x, img->scale_y, &img->pivot);
    a.x1 += obj->coords.x1;
    a.y1 += obj->coords.y1;
    a.x2 += obj->coords.x1;
    a.y2 += obj->coords.y1;
    lv_obj_invalidate_area(obj, &a);

    img->rotation = angle;

    /* Disable invalidations because lv_obj_refresh_ext_draw_size would invalidate
     * the whole ext draw area */
    lv_disp_t * disp = lv_obj_get_disp(obj);
    lv_disp_enable_invalidation(disp, false);
    lv_obj_refresh_ext_draw_size(obj);
    lv_disp_enable_invalidation(disp, true);

    lv_image_buf_get_transformed_area(&a, w, h, img->rotation, img->scale_x, img->scale_y, &img->pivot);
    a.x1 += obj->coords.x1;
    a.y1 += obj->coords.y1;
    a.x2 += obj->coords.x1;
    a.y2 += obj->coords.y1;
    lv_obj_invalidate_area(obj, &a);
	lv_memcpy(area,&a,sizeof(lv_area_t));
}

static void canvas_delete_event_cb(lv_event_t * e)
{
	lv_obj_t *canvas = lv_event_get_target(e);
	lv_image_dsc_t *img_dsc = lv_canvas_get_image(canvas);
	ui_mem_free(MEM_RES,(void *)img_dsc->data);
}

static void text_arc_up(text_arc_t *text_arc)
{
	lv_obj_t *obj = &text_arc->obj;
	lv_obj_clean(obj);
	lv_draw_label_dsc_t label_draw_dsc;
	lv_draw_label_dsc_init(&label_draw_dsc);
	lv_obj_init_draw_label_dsc(obj, LV_PART_MAIN, &label_draw_dsc);
	const lv_font_t *font = label_draw_dsc.font;
	char *text = text_arc->text;
	if(font == NULL || text == NULL)
		return;
	uint32_t ofs = 0;
	uint32_t letter = lv_text_encoded_next(text, &ofs);
	uint32_t letter_next = 0;
	uint16_t letter_w;
	const uint16_t letter_space = 0; /* add space for special characters */
	const uint16_t letter_h = lv_font_get_line_height(font) + letter_space * 2;
	const lv_color_format_t cf = LV_COLOR_FORMAT_ARGB8888;
	const uint8_t px_size = 4;
	int32_t angle = text_arc->angle_st;

	lv_draw_label_dsc_t draw_dsc;
	lv_draw_label_dsc_init(&draw_dsc);
	draw_dsc.font = font;
	draw_dsc.color = label_draw_dsc.color;

	uint32_t letter_len = 0;
	uint32_t mem_size = 10;
	text_id_t *letter_data = lv_malloc(sizeof(text_id_t) * mem_size);
	if(letter_data == NULL) {
		LV_LOG_ERROR("letter_data alloc failed");
		return;
	}
	int32_t all_angle = 0;
	int32_t radius = text_arc->radius - letter_h;
	for (; letter != 0; letter = letter_next) {
		/* copy the text */
		char tmp_str[8] = {0};
		if (ofs >= sizeof(tmp_str)) {
			LV_LOG_ERROR("increase buffer to hold text frag");
			break;
		}
		lv_memcpy(tmp_str, text, ofs);
		tmp_str[ofs] = '\0';
		letter_data[letter_len].ofs_text = text - text_arc->text;
		letter_data[letter_len].text_len = ofs;

		/* move to next position */
		text += ofs;
		ofs = 0;

		letter_next = lv_text_encoded_next(text, &ofs);

		LV_ASSERT_NULL(font);
		lv_font_glyph_dsc_t font_g;
		lv_font_get_glyph_dsc(font, &font_g, letter, letter_next);
		letter_data[letter_len].letter_w = font_g.adv_w;

		int32_t box_w = font_g.box_w / 2;
#if 1
		float c_angle = asin((float)(box_w - font_g.ofs_x) / radius);
		letter_data[letter_len].st_angle = (int16_t)(c_angle * 180.0 / PI);
		c_angle = asin((float)(box_w) / radius);
		letter_data[letter_len].end_angle = (int16_t)(c_angle * 180.0 / PI);
#else
		float c_angle = asin((float)(box_w) / radius);
		letter_data[letter_len].st_angle = (int16_t)(c_angle * 180.0 / PI);
		letter_data[letter_len].end_angle = letter_data[letter_len].st_angle;
#endif
		/* Part unicode fine tuning angle*/
		if (text_arc->fine_tuning_angle_cb)
			text_arc->fine_tuning_angle_cb(letter, &letter_data[letter_len].st_angle, &letter_data[letter_len].end_angle);
		all_angle += letter_data[letter_len].st_angle + letter_data[letter_len].end_angle + text_arc->interval_angle;

		letter_len++;
		if(letter_len >= mem_size) {
			mem_size += 10;
			letter_data = lv_realloc(letter_data, sizeof(text_id_t) * mem_size);
			if(letter_data == NULL) {
				letter_len = 0;
				LV_LOG_ERROR("letter_data realloc failed");
				return;
			}
		}
	}
	if(text_arc->b_center) {
		if(letter_len)
			all_angle -= letter_data[0].st_angle;
		angle -= all_angle / 2;
	}

	for (uint32_t i = 0; i < letter_len; i++) {
		int16_t overturn_a = 0;
		text_id_t *at_letter = letter_data + i;
		if(text_arc->b_overturn) {
			at_letter = letter_data + letter_len - 1 - i;
			overturn_a = 180;
		}
		letter_w = at_letter->letter_w;
		angle += at_letter->st_angle;

		// add space for letter if exceed font height
		letter_w += 2 * letter_space;

		// allocate the buffer for letter
		uint32_t letter_buf_size = (uint32_t)letter_w * letter_h * px_size;
		void *buf = ui_mem_alloc(MEM_RES,letter_buf_size,__func__);
		if (buf == NULL) {
			LV_LOG_ERROR("letter buffer alloc failed %d",letter_buf_size);
			break;
		}

		mem_dcache_flush(buf, letter_buf_size);
		mem_dcache_sync();
		ui_memset(buf, 0, letter_buf_size);

		// compute the positon of letter
		lv_point_t pivot;
		pivot.x = radius + ((lv_trigo_sin(angle) * radius) >> LV_TRIGO_SHIFT);
		pivot.y = radius - ((lv_trigo_cos(angle) * radius) >> LV_TRIGO_SHIFT);

		// create canvas to hold letter
		lv_obj_t *canvas = lv_canvas_create(obj);
		if (canvas == NULL) {
			LV_LOG_ERROR("canvas obj create failed");
			break;
		}
		ui_memsetcpy_wait_finish(500);
		lv_canvas_set_buffer(canvas, buf, letter_w, letter_h, cf);

		lv_layer_t layer;
		lv_canvas_init_layer(canvas, &layer);

		lv_area_t coords;
		coords.x1 = letter_space;
		coords.y1 = letter_space;
		coords.x2 = letter_w - 1;
		coords.y2 = letter_h - 1;

		char tmp_str[8] = {0};
		lv_memcpy(tmp_str, text_arc->text + at_letter->ofs_text, at_letter->text_len);

		draw_dsc.text = tmp_str;
		lv_draw_label(&layer, &draw_dsc, &coords);

		lv_canvas_finish_layer(canvas, &layer);

		lv_obj_set_pos(canvas, pivot.x - letter_w / 2 + letter_h, pivot.y - letter_h + letter_h);
		lv_obj_add_event_cb(canvas, canvas_delete_event_cb, LV_EVENT_DELETE, NULL);
		lv_area_t area = {0};
		text_arc_set_angle(canvas, (angle + overturn_a) * 10 , &area);
		mem_dcache_clean(buf, letter_buf_size);
		mem_dcache_sync();
		angle += at_letter->end_angle;
		angle += text_arc->interval_angle;
	}
	lv_free(letter_data);
	lv_obj_set_size(obj, text_arc->radius * 2, text_arc->radius * 2);
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t *text_arc_create(lv_obj_t * parent)
{
	LV_LOG_INFO("begin");
	lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
	lv_obj_class_init_obj(obj);
	return obj;
}

void text_arc_set_text(lv_obj_t *class_p , char *text)
{
	LV_UNUSED(class_p);
	text_arc_t *text_arc = (text_arc_t *)class_p;
	if(text_arc->text)
	{
		if(lv_strcmp(text_arc->text,text) == 0)
			return;
		lv_free(text_arc->text);
	}
	uint32_t text_num = lv_strlen(text) + 1;
	text_arc->text = lv_malloc(text_num);
	if(text_arc->text == NULL)
	{
		LV_LOG_ERROR("text alloc error");
		return;
	}
	lv_memcpy(text_arc->text,text,text_num);
}

void text_arc_set_radian(lv_obj_t *obj, uint16_t radius, int16_t angle_st, int16_t interval_angle)
{
	LV_UNUSED(obj);
	text_arc_t *text_arc = (text_arc_t *)obj;
	if(text_arc->angle_st == angle_st && text_arc->interval_angle == interval_angle && text_arc->radius == radius)
		return;
	text_arc->angle_st = angle_st;
	text_arc->interval_angle = interval_angle;
	text_arc->radius = radius;
}

void text_arc_set_center(lv_obj_t *obj, bool b_center)
{
	LV_UNUSED(obj);
	text_arc_t *text_arc = (text_arc_t *)obj;
	text_arc->b_center = b_center;
	if(text_arc->b_center == b_center)
		return;
}

void text_arc_set_overturn(lv_obj_t *obj, bool b_overturn)
{
	LV_UNUSED(obj);
	text_arc_t *text_arc = (text_arc_t *)obj;
	if(text_arc->b_overturn == b_overturn)
		return;
	text_arc->b_overturn = b_overturn;
}

void text_arc_set_tuning_cb(lv_obj_t *obj, fine_tuning_angle_cb_t cb)
{
	LV_UNUSED(obj);
	text_arc_t *text_arc = (text_arc_t *)obj;
	text_arc->fine_tuning_angle_cb = cb;
}

void text_arc_refresh(lv_obj_t *obj)
{
	LV_UNUSED(obj);
	text_arc_t* text_arc = (text_arc_t*)obj;
	text_arc_up(text_arc);
	lv_obj_invalidate(obj);
}

static void text_arc_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
	LV_UNUSED(class_p);

    lv_result_t res = lv_obj_event_base(MY_CLASS, e);
    if(res != LV_RESULT_OK) return;
}

static void text_arc_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
	lv_obj_set_scroll_dir(obj,LV_DIR_NONE);
}

static void text_arc_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
	LV_UNUSED(class_p);

	text_arc_t* text_arc = (text_arc_t*)obj;
	if (text_arc->text)
		lv_free(text_arc->text);
}
