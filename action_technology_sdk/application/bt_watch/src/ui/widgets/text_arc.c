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
#include <stdio.h>
#include <math.h>
#include <ui_mem.h>
#include <memory/mem_cache.h>
#include <display/sw_draw.h>
#ifdef CONFIG_LVGL_USE_FREETYPE_FONT
#include <lvgl/lvgl_freetype_font.h>
#endif
#include <lvgl/src/lvgl_private.h>

#include "text_arc.h"

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
	char *text;
	fine_tuning_angle_cb_t fine_tuning_angle_cb;
	uint16_t radius;
	int16_t angle_st;
	int16_t interval_angle;
	uint8_t b_center : 1;
	uint8_t b_overturn : 1;
	lv_style_t style;
} text_arc_t;

typedef struct {
	uint32_t letter;
	int16_t st_angle;
	int16_t end_angle;
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

static void canvas_delete_event_cb(lv_event_t * e)
{
	lv_obj_t *canvas = lv_event_get_target(e);
	void *buffer = (void *)lv_canvas_get_buf(canvas);
	ui_mem_free(MEM_RES, buffer);
}

static void text_arc_letter_angle_calculate(text_arc_t *text_arc, lv_font_glyph_dsc_t *font_g, text_id_t *letter_data, uint32_t letter, int32_t radius)
{
	int32_t box_w = font_g->box_w / 2;
#if 0
	float c_angle = asin((float)(box_w - font_g.ofs_x) / radius);
	letter_data->st_angle = (int16_t)(c_angle * 180.0 / PI);
	c_angle = asin((float)(box_w) / radius);
	letter_data->end_angle = (int16_t)(c_angle * 180.0 / PI);
#else
	float c_angle = asin((float)(box_w) / radius);
	letter_data->st_angle = (int16_t)(c_angle * 180.0 / PI);
	letter_data->end_angle = letter_data->st_angle;
#endif
	/* Part unicode fine tuning angle*/
	if (text_arc->fine_tuning_angle_cb)
		text_arc->fine_tuning_angle_cb(letter, &letter_data->st_angle, &letter_data->end_angle);
}

static void text_arc_up(text_arc_t *text_arc)
{
	lv_obj_t *obj = &text_arc->obj;
	lv_obj_clean(obj);

	if (text_arc->text == NULL || text_arc->text[0] == '\0')
		return;

	const lv_font_t *font = lv_obj_get_style_text_font(obj, LV_PART_MAIN);;
	const char *text = text_arc->text;
	uint32_t ofs = 0;
	uint32_t letter = lv_text_encoded_next(text, &ofs);
	uint32_t letter_next = 0;
	const uint16_t letter_space = 0; /* add space for special characters */
	const uint16_t letter_h = lv_font_get_line_height(font) + letter_space * 2;
	int32_t angle = text_arc->angle_st;

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
		letter_data[letter_len].letter = letter;

		/* move to next position */
		text += ofs;
		ofs = 0;

		letter_next = lv_text_encoded_next(text, &ofs);

		lv_font_glyph_dsc_t font_g;
		lv_font_get_glyph_dsc(font, &font_g, letter, letter_next);
		text_arc_letter_angle_calculate(text_arc, &font_g, &letter_data[letter_len], letter, radius);
		if(all_angle > 360) {
			if(letter_len)
				letter_len--;
			letter = lv_text_encoded_next(".", NULL);
			lv_font_get_glyph_dsc(font, &font_g, letter, letter);
			text_id_t letter_ret = {0};
			text_arc_letter_angle_calculate(text_arc, &font_g, &letter_ret, letter, radius);
			int32_t angle_ret = letter_ret.st_angle + letter_ret.end_angle + text_arc->interval_angle;
			uint32_t letter_id = letter_len;
			for (uint8_t i = 0 ; i < 3; i++) {
				all_angle -= letter_data[letter_id].st_angle + letter_data[letter_id].end_angle + text_arc->interval_angle;
				letter_data[letter_id].letter = '.';
				letter_data[letter_id].st_angle = letter_ret.st_angle;
				letter_data[letter_id].end_angle = letter_ret.end_angle;
				all_angle += angle_ret;
				letter_id--;
				if(letter_id == 0)
					break;
			}
			letter_len++;
			break;
		} else {
			all_angle += letter_data[letter_len].st_angle + letter_data[letter_len].end_angle + text_arc->interval_angle;
		}
		letter_len++;
		if(letter_len >= mem_size) {
			mem_size += 10;
			letter_data = lv_realloc(letter_data, sizeof(text_id_t) * mem_size);
			if(letter_data == NULL) {
				letter_len = 0;
				LV_LOG_ERROR("letter_data realloc failed");
				break;
			}
		}
	}

	if(text_arc->b_center) {
		if(letter_len)
			all_angle -= letter_data[0].st_angle;
		angle -= all_angle / 2;
	}

	lv_obj_set_size(obj, text_arc->radius * 2, text_arc->radius * 2);
	lv_style_set_image_recolor(&text_arc->style, lv_obj_get_style_text_color_filtered(obj, LV_PART_MAIN));

#ifdef CONFIG_LVGL_USE_FREETYPE_FONT
	int force_bitmap = lvgl_freetype_force_bitmap((lv_font_t *)font, 1);
#endif

	for (uint32_t i = 0; i < letter_len; i++) {
		int16_t overturn_a = 0;
		text_id_t *at_letter = letter_data + i;
		if(text_arc->b_overturn) {
			at_letter = letter_data + letter_len - 1 - i;
			overturn_a = 180;
		}

		angle += at_letter->st_angle;

		lv_font_glyph_dsc_t font_g;
		lv_font_get_glyph_dsc(font, &font_g, at_letter->letter, 0);
		if (font_g.box_w == 0 || font_g.box_h == 0)
			continue;

		const lv_draw_buf_t * bmp_drawbuf = lv_font_get_glyph_bitmap(&font_g, NULL);
		if (bmp_drawbuf == NULL) {
			LV_LOG_ERROR("get_glyph_bitmap failed");
			break;
		}

		if (bmp_drawbuf->header.cf < LV_COLOR_FORMAT_A1 || bmp_drawbuf->header.cf > LV_COLOR_FORMAT_A8) {
			LV_LOG_ERROR("invalid cf %u", bmp_drawbuf->header.cf);
			lv_font_glyph_release_draw_data(&font_g);
			break;
		}

		uint32_t letter_buf_size = (uint32_t)font_g.box_w * font_g.box_h;
		void *buf = ui_mem_aligned_alloc(MEM_RES, 64, letter_buf_size, __func__);
		if (buf == NULL) {
			LV_LOG_ERROR("letter buffer alloc failed %d",letter_buf_size);
			lv_font_glyph_release_draw_data(&font_g);
			break;
		}

		if (bmp_drawbuf->header.cf == LV_COLOR_FORMAT_A8) {
			if (bmp_drawbuf->header.stride == bmp_drawbuf->header.w) {
				lv_memcpy(buf, bmp_drawbuf->data, letter_buf_size);
			} else {
				uint8_t *buf8 = buf;
				uint8_t *data8 = bmp_drawbuf->data;
				for (i = 0; i < bmp_drawbuf->header.h; i++) {
					lv_memcpy(buf8, data8, font_g.box_w);
					buf8 += font_g.box_w;
					data8 += bmp_drawbuf->header.stride;
				}
			}
		} else {
			sw_convert_a124_to_a8(buf, bmp_drawbuf->data, font_g.box_w,
				bmp_drawbuf->header.stride * 8, 0, lv_color_format_get_bpp(bmp_drawbuf->header.cf),
				font_g.box_w, font_g.box_h);
		}

		mem_dcache_clean(buf, letter_buf_size);
		mem_dcache_sync();

		lv_font_glyph_release_draw_data(&font_g);

		// compute the positon of letter
		lv_point_t pivot;
		pivot.x = radius + ((lv_trigo_sin(angle) * radius) >> LV_TRIGO_SHIFT);
		pivot.y = radius - ((lv_trigo_cos(angle) * radius) >> LV_TRIGO_SHIFT);

		// create canvas to hold letter
		lv_obj_t *canvas = lv_canvas_create(obj);
		if (canvas == NULL) {
			LV_LOG_ERROR("canvas obj create failed");
			ui_mem_free(MEM_RES, buf);
			break;
		}

		lv_canvas_set_buffer(canvas, buf, font_g.box_w, font_g.box_h, LV_COLOR_FORMAT_A8);
		lv_obj_set_pos(canvas, pivot.x - font_g.adv_w / 2 + letter_h,
				pivot.y + letter_h - font->base_line - font_g.box_h - font_g.ofs_y);
		lv_obj_add_style(canvas, &text_arc->style, LV_PART_MAIN);
		lv_obj_add_event_cb(canvas, canvas_delete_event_cb, LV_EVENT_DELETE, NULL);
		lv_image_set_pivot(canvas, font_g.box_w / 2, font->base_line + font_g.box_h + font_g.ofs_y);
		lv_image_set_rotation(canvas, (angle + overturn_a) * 10);

		angle += at_letter->end_angle;
		angle += text_arc->interval_angle;
	}

#ifdef CONFIG_LVGL_USE_FREETYPE_FONT
	lvgl_freetype_force_bitmap((lv_font_t *)font, force_bitmap);
#endif

	lv_free(letter_data);
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
		lv_free(text_arc->text);

	if(text) {
		uint32_t text_num = lv_strlen(text) + 1;
		text_arc->text = lv_malloc(text_num);
		if(text_arc->text == NULL)
		{
			LV_LOG_ERROR("text alloc error");
			return;
		}
		lv_memcpy(text_arc->text,text,text_num);
	}
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
}

static void text_arc_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
	LV_UNUSED(class_p);

	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_COVER_CHECK) {
		lv_cover_check_info_t * info = lv_event_get_param(e);
		info->res = LV_COVER_RES_NOT_COVER;
	}
}

static void text_arc_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
	LV_UNUSED(class_p);
	lv_obj_set_scroll_dir(obj, LV_DIR_NONE);
	lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);

	text_arc_t* text_arc = (text_arc_t*)obj;
	lv_style_init(&text_arc->style);
	lv_style_set_image_recolor_opa(&text_arc->style, LV_OPA_COVER);
}

static void text_arc_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
	LV_UNUSED(class_p);

	text_arc_t* text_arc = (text_arc_t*)obj;
	lv_style_reset(&text_arc->style);
	if (text_arc->text)
		lv_free(text_arc->text);
}
