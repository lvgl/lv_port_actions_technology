/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file text_canvas.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <assert.h>
#include <ui_mem.h>
#include <memory/mem_cache.h>
#include <lvgl/lvgl_bitmap_font.h>
#include <lvgl/src/misc/lv_txt_ap.h>
#include <lvgl/src/draw/sw/lv_draw_sw.h>
#ifdef CONFIG_LVGL_USE_FREETYPE_FONT
#include <lvgl/lvgl_freetype_font.h>
#endif
#include "text_canvas.h"

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &text_canvas_class

#define TEXT_CANVAS_DEF_SCROLL_SPEED   (lv_disp_get_dpi(lv_obj_get_disp(obj)) / 3)
#define TEXT_CANVAS_SCROLL_DELAY       300

#define OPT_LV_TXT_GET_SIZE       1
#define OPT_STYLE_CHANGED_REDRAW  1
#define OPT_LONG_SCROLL_HW_ALIGN  1

/* lv_img_dsc_t only support image height less than 2048. */
#define IMG_MAX_H                 2047

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  EXTERNAL PROTOTYPES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void text_canvas_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void text_canvas_event(const lv_obj_class_t * class_p, lv_event_t * e);
static void text_canvas_draw(lv_event_t * e);
static void text_canvas_refr_text(lv_obj_t *obj);
static void refr_text_to_img(lv_obj_t *obj, lv_draw_label_dsc_t *draw_dsc,
		lv_img_dsc_t *img_dsc, lvgl_draw_label_result_t *feedback);

static void free_cvt_text(lv_obj_t *obj);
static void replace_text_dot_end(lv_obj_t *obj, lv_draw_label_dsc_t *draw_dsc);
static uint32_t get_letter_on(lv_obj_t * obj, lv_draw_label_dsc_t *draw_dsc, lv_point_t * pos_in);

static lv_img_cf_t get_text_font_img_cf(const lv_font_t *font);
#if 0
static void set_px_cb_alpha(lv_disp_drv_t * disp_drv, uint8_t * buf, lv_coord_t buf_w, lv_coord_t x, lv_coord_t y,
							lv_color_t color, lv_opa_t opa);
#endif
static void draw_letter_cb(lv_draw_ctx_t * draw_ctx, const lv_draw_label_dsc_t * dsc,
		const lv_point_t * pos_p, uint32_t letter);

static void alloc_emoji_dsc(lv_obj_t *obj, const char *txt);
static void free_emoji_dsc(lvgl_draw_emoji_dsc_t *emoji_dsc);

static void set_ofs_x_anim(void * obj, int32_t v);
static void set_ofs_y_anim(void * obj, int32_t v);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t text_canvas_class = {
	.destructor_cb = text_canvas_destructor,
	.event_cb = text_canvas_event,
	.width_def = LV_SIZE_CONTENT,
	.height_def = LV_SIZE_CONTENT,
	.instance_size = sizeof(text_canvas_t),
	.base_class = &lv_obj_class,
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * text_canvas_create(lv_obj_t * parent)
{
	LV_LOG_INFO("begin");
	lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
	lv_obj_class_init_obj(obj);
	return obj;
}

void text_canvas_clean(lv_obj_t * obj)
{
	text_canvas_t * canvas = (text_canvas_t *)obj;

	text_canvas_set_text_static(obj, NULL);

	lv_mem_free((void *)canvas->dsc.data);
	canvas->dsc.data_size = 0;

	free_emoji_dsc(&canvas->emoji_dsc);
}

/*=====================
 * Setter functions
 *====================*/

void text_canvas_set_max_height(lv_obj_t *obj, uint16_t h)
{
	text_canvas_t * canvas = (text_canvas_t *)obj;

	/* lv_img_dsc_t only support image height less than 2048. */
	h = LV_MIN(h, IMG_MAX_H);

	if (h != canvas->dsc_h_max) {
		canvas->dsc_h_max = h;
	}
}

void text_canvas_set_emoji_enable(lv_obj_t* obj, bool en)
{
	text_canvas_t * canvas = (text_canvas_t *)obj;
	canvas->emoji_en = en;

	if (!en) {
		free_emoji_dsc(&canvas->emoji_dsc);
	}
}

void text_canvas_set_text(lv_obj_t * obj, const char * text)
{
	text_canvas_t * canvas = (text_canvas_t *)obj;

	if (text == NULL) {
		text_canvas_set_text_static(obj, text);
		return;
	}

	if (canvas->text && !canvas->static_txt) {
		lv_mem_free(canvas->text);
	}

	canvas->text = NULL;
	canvas->static_txt = 0;
	lv_obj_refr_size(obj);

#if LV_USE_ARABIC_PERSIAN_CHARS
	size_t len = _lv_txt_ap_calc_bytes_cnt(text);

	canvas->text = lv_mem_alloc(len);
	if(canvas->text == NULL) return;

	_lv_txt_ap_proc(text, canvas->text);
#else
	size_t len = strlen(text) + 1;

	canvas->text = lv_mem_alloc(len);
	if (canvas->text == NULL) return;

	strncpy(canvas->text, text, len);
#endif /* LV_USE_ARABIC_PERSIAN_CHARS */

	text_canvas_refr_text(obj);
}

void text_canvas_set_text_static(lv_obj_t * obj, const char * text)
{
	text_canvas_t * canvas = (text_canvas_t *)obj;

	if (canvas->text && !canvas->static_txt) {
		lv_mem_free(canvas->text);
	}

	canvas->text = NULL;
	canvas->static_txt = 1;

	if (text != NULL) {
		lv_obj_refr_size(obj);
		canvas->text = (char *)text;
	}

	text_canvas_refr_text(obj);
}

void text_canvas_set_long_mode(lv_obj_t * obj, text_canvas_long_mode_t long_mode)
{
	text_canvas_t * canvas = (text_canvas_t *)obj;

	if (long_mode >= TEXT_CANVAS_NUM_LONG_MODES || long_mode == canvas->long_mode)
		return;

	if (long_mode == TEXT_CANVAS_LONG_CLIP ||
		long_mode == TEXT_CANVAS_LONG_SCROLL ||
		long_mode == TEXT_CANVAS_LONG_SCROLL_CIRCULAR) {
		canvas->expand = 1;
		canvas->expand_scroll = (long_mode != TEXT_CANVAS_LONG_CLIP);
	} else {
		canvas->expand = 0;
		canvas->expand_scroll = 0;
	}

	canvas->long_mode = long_mode;
	if (canvas->text)
		text_canvas_refr_text(obj);
}

void text_canvas_set_recolor(lv_obj_t * obj, bool en)
{
	text_canvas_t * canvas = (text_canvas_t *)obj;

	if (en == canvas->recolor) return;

	canvas->recolor = en;
	if (canvas->text)
		text_canvas_refr_text(obj);
}

/*=====================
 * Getter functions
 *====================*/

lv_img_dsc_t * text_canvas_get_img(lv_obj_t * obj)
{
	text_canvas_t * canvas = (text_canvas_t *)obj;

	return &canvas->dsc;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void text_canvas_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
	text_canvas_clean(obj);
}

static void text_canvas_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
	LV_UNUSED(class_p);

	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * obj = lv_event_get_target(e);
	text_canvas_t * canvas = (text_canvas_t *)obj;

	/*Ancestor events will be called during drawing*/
	if (code != LV_EVENT_COVER_CHECK && code != LV_EVENT_DRAW_MAIN && code != LV_EVENT_DRAW_POST) {
		/*Call the ancestor's event handler*/
		lv_res_t res = lv_obj_event_base(MY_CLASS, e);
		if (res != LV_RES_OK) return;
	}

	if (code == LV_EVENT_STYLE_CHANGED) {
#if OPT_STYLE_CHANGED_REDRAW == 0
		/* FIXME: should we respond this event ? */
		if (canvas->text)
			text_canvas_refr_text(obj);
#endif
	} else if (code == LV_EVENT_SIZE_CHANGED) {
		if (canvas->text)
			text_canvas_refr_text(obj);
	} else if (code == LV_EVENT_REFR_EXT_DRAW_SIZE) {
		lv_event_set_ext_draw_size(e, canvas->ext_draw_size);
	} else if (code == LV_EVENT_GET_SELF_SIZE) {
		lv_point_t * p = lv_event_get_param(e);
		p->x = canvas->dsc_w;
		p->y = canvas->dsc.header.h - canvas->ext_draw_size * 2;
	} else if (code == LV_EVENT_COVER_CHECK) {
		lv_event_set_cover_res(e, LV_COVER_RES_NOT_COVER);
	} else if (code == LV_EVENT_DRAW_MAIN) {
		text_canvas_draw(e);
	}
}

static void text_canvas_draw(lv_event_t * e)
{
	lv_obj_t * obj = lv_event_get_target(e);
	text_canvas_t * canvas = (text_canvas_t *)obj;

	lv_draw_ctx_t * draw_ctx = lv_event_get_draw_ctx(e);
	lv_area_t txt_area, draw_area;
	lv_point_t offsets[2];
	int i, parts = 1;

	lv_area_t dsc_coords;
	lv_obj_get_content_coords(obj, &dsc_coords);
	dsc_coords.y1 -= canvas->ext_draw_size + lv_obj_get_scroll_top(obj);

	offsets[0] = canvas->offset;
	if (canvas->offset_circ.x || canvas->offset_circ.y) {
		offsets[1].x = offsets[0].x + canvas->offset_circ.x;
		offsets[1].y = offsets[0].y + canvas->offset_circ.y;
		parts = 2;
	}

	lv_draw_img_dsc_t draw_dsc;
	lv_draw_img_dsc_init(&draw_dsc);
	//lv_obj_init_draw_img_dsc(obj, LV_IMG_PART_MAIN, &draw_dsc);
	draw_dsc.recolor = lv_obj_get_style_text_color(obj, LV_PART_MAIN);
	draw_dsc.opa = lv_obj_get_style_text_opa(obj, LV_PART_MAIN);

	for (i = 0; i < parts; i++) {
		txt_area.x1 = dsc_coords.x1 + offsets[i].x;
		txt_area.y1 = dsc_coords.y1 + offsets[i].y;
		txt_area.x2 = txt_area.x1 + canvas->dsc.header.w - 1;
		txt_area.y2 = txt_area.y1 + canvas->dsc.header.h - 1;

		if (_lv_area_intersect(&draw_area, &txt_area, draw_ctx->clip_area) == false)
			continue;

		//assert(((txt_area.x1 - draw_area.x1) & canvas->dsc_w_mask) == 0);

		lv_draw_img(draw_ctx, &draw_dsc, &txt_area, &canvas->dsc);

		if (canvas->emoji_dsc.count > 0) {
			for (int i = canvas->emoji_dsc.count - 1; i >= 0; i--) {
				lvgl_emoji_dsc_t *emoji = &canvas->emoji_dsc.emoji[i];

				lv_area_t emoji_area;
				emoji_area.x1 = emoji->pos.x + txt_area.x1;
				emoji_area.y1 = emoji->pos.y + txt_area.y1;
				emoji_area.x2 = emoji_area.x1 + emoji->img.header.w - 1;
				emoji_area.y2 = emoji_area.y1 + emoji->img.header.h - 1;

				if (_lv_area_is_on(&emoji_area, &draw_area) == false)
					continue;

				int res = lvgl_bitmap_font_get_emoji_dsc(canvas->emoji_dsc.font,
						emoji->code, &emoji->img, NULL, false);
				if (res) {
					if (draw_ctx->wait_for_finish) {
						draw_ctx->wait_for_finish(draw_ctx);
					}

					res = lvgl_bitmap_font_get_emoji_dsc(canvas->emoji_dsc.font,
							emoji->code, &emoji->img, NULL, true);
					if (res) {
						LV_LOG_ERROR("cannot find image for emoji unicode 0x%x", emoji->code);
						continue;
					}
				}

				lv_draw_img(draw_ctx, &draw_dsc, &emoji_area, &emoji->img);
			}
		}
	}
}

static void text_canvas_refr_text(lv_obj_t *obj)
{
	text_canvas_t * canvas = (text_canvas_t *)obj;
	lv_coord_t dsc_h_max = (canvas->dsc_h_max > 0) ? canvas->dsc_h_max : IMG_MAX_H;
	lv_coord_t txt_w = lv_obj_get_content_width(obj);
	lv_coord_t txt_h = lv_obj_get_content_height(obj);
	const char *canvas_text = canvas->text;
	uint16_t line_height, line_height_font;
	bool skip_txt_height = false;

	/*Delete the old scroll animation (if exists)*/
	lv_anim_del(obj, set_ofs_x_anim);
	lv_anim_del(obj, set_ofs_y_anim);

	canvas->offset.x = 0;
	canvas->offset.y = 0;
	canvas->offset_circ.x = 0;
	canvas->offset_circ.y = 0;
	canvas->emoji_dsc.count = 0;

	if (canvas_text == NULL) {
		canvas->dsc.header.h = 0;
		goto out_exit;
	}

#if LV_USE_ARABIC_PERSIAN_CHARS
	if (canvas->static_txt) {
		size_t len = _lv_txt_ap_calc_bytes_cnt(canvas->text);

		canvas->cvt_text = lv_mem_alloc(len);
		if (canvas->cvt_text == NULL) goto out_exit;

		_lv_txt_ap_proc(canvas->text, canvas->cvt_text);
		canvas_text = canvas->cvt_text;
	}
#endif /* LV_USE_ARABIC_PERSIAN_CHARS */

	bool is_content_w = (lv_obj_get_style_width(obj, LV_PART_MAIN) == LV_SIZE_CONTENT && !obj->w_layout);
	bool is_content_h = (lv_obj_get_style_height(obj, LV_PART_MAIN) == LV_SIZE_CONTENT);

	canvas->dsc_w = txt_w;
	if (canvas->dsc_w <= 0 && !is_content_w) {
		canvas->dsc.header.h = 0;
		goto out_exit;
	}

	lv_draw_label_dsc_t draw_dsc;
	lv_draw_label_dsc_init(&draw_dsc);
	lv_obj_init_draw_label_dsc(obj, LV_PART_MAIN, &draw_dsc);
	lv_bidi_calculate_align(&draw_dsc.align, &draw_dsc.bidi_dir, canvas_text);

	draw_dsc.opa = LV_OPA_COVER;
	draw_dsc.flag = LV_TEXT_FLAG_NONE;
	if (canvas->recolor) draw_dsc.flag |= LV_TEXT_FLAG_RECOLOR;
	if (canvas->expand) draw_dsc.flag |= LV_TEXT_FLAG_EXPAND;

	line_height_font = lv_font_get_line_height(draw_dsc.font);
	line_height = line_height_font + draw_dsc.line_space;

#if LV_USE_ARABIC_PERSIAN_CHARS
	/* Italic or other non-typical letters can be drawn of out of the object.
	  * It happens if box_w + ofs_x > adw_w in the glyph.
	  * To avoid this add some extra draw area.
	  * font_h / 4 is an empirical value. */
	canvas->ext_draw_size = line_height_font / 4;
#endif

	canvas->dsc.header.cf = get_text_font_img_cf(draw_dsc.font);
	if (canvas->dsc.header.cf == LV_IMG_CF_UNKNOWN) {
		canvas->dsc.header.h = 0;
		goto out_exit;
	}

	uint8_t px_size = lv_img_cf_get_px_size(canvas->dsc.header.cf);
	canvas->dsc_w_mask = (8 / px_size) - 1; /* 1-byte aligned */
	uint8_t dsc_stride_mask = (canvas->dsc_w_mask + 1) * 4 - 1; /* 4-byte aligned */

#if OPT_LONG_SCROLL_HW_ALIGN
#  ifdef CONFIG_DISPLAY_ENGINE_LARK
	canvas->dsc_x_mask = (canvas->dsc_w_mask + 1) * 4 - 1; /* 4-byte aligned */
#  else
	canvas->dsc_x_mask = canvas->dsc_w_mask; /* 1-byte aligned */
#  endif /* CONFIG_DISPLAY_ENGINE_LARK */
#else
	canvas->dsc_x_mask = canvas->dsc_w_mask; /* 1-byte aligned */
#endif /* OPT_LONG_SCROLL_HW_ALIGN */

	/* should not greater than the content width */
	if (!is_content_w) canvas->dsc_w &= ~canvas->dsc_w_mask;

#if OPT_LV_TXT_GET_SIZE
	if (is_content_w || is_content_h || canvas->dsc_h_max <= 0 || canvas->long_mode != TEXT_CANVAS_LONG_WRAP)
#endif /* OPT_LV_TXT_GET_SIZE */
	{
		lv_point_t size;
		lv_text_flag_t flag = draw_dsc.flag;
		if (is_content_w) flag |= LV_TEXT_FLAG_FIT;

		lv_txt_get_size(&size, canvas_text, draw_dsc.font, draw_dsc.letter_space,
				draw_dsc.line_space, canvas->dsc_w, flag);

		if (is_content_w || canvas->expand_scroll)
			canvas->dsc_w = (size.x + canvas->dsc_w_mask) & ~canvas->dsc_w_mask;

		if (is_content_w) txt_w = canvas->dsc_w;

		canvas->dsc.header.h = LV_MIN(size.y, dsc_h_max);
		if (is_content_h) {
			txt_h = canvas->dsc.header.h;
		} else if (canvas->long_mode == TEXT_CANVAS_LONG_CLIP) {
			canvas->dsc.header.h = LV_MIN(canvas->dsc.header.h, (uint32_t)txt_h);
		}

		if (canvas->long_mode == TEXT_CANVAS_LONG_DOT ||
			canvas->long_mode == TEXT_CANVAS_LONG_LIST) {
			if (size.y > dsc_h_max &&
				_lv_txt_get_encoded_length(canvas_text) > TEXT_CANVAS_DOT_NUM) {

				if (canvas->long_mode == TEXT_CANVAS_LONG_LIST) {
					canvas->dsc.header.h = LV_MIN(line_height_font, dsc_h_max);
					if (is_content_h) txt_h = canvas->dsc.header.h;
				}

				replace_text_dot_end(obj, &draw_dsc);
				canvas_text = canvas->cvt_text ? canvas->cvt_text : canvas->text;
			}
		} else if (canvas->expand_scroll) {
			bool is_circular = (canvas->long_mode == TEXT_CANVAS_LONG_SCROLL_CIRCULAR);
			int32_t start = 0, end = 0;
			lv_anim_exec_xcb_t exec_cb = NULL;

			size.x = canvas->dsc_w;
			if (size.x > txt_w) {
				if (is_circular) {
					canvas->offset_circ.x = size.x + lv_font_get_glyph_width(draw_dsc.font, ' ', ' ')  * TEXT_CANVAS_WAIT_CHAR_COUNT;
					canvas->offset_circ.x = (canvas->offset_circ.x + canvas->dsc_x_mask) & ~canvas->dsc_x_mask;
					end = -canvas->offset_circ.x;
				} else {
					end = txt_w - size.x;
				}

		#if LV_USE_BIDI
				if (draw_dsc.bidi_dir == LV_BASE_DIR_RTL) {
					start = end;
					end = 0;
				}
		#endif

				exec_cb = set_ofs_x_anim;

				/* In SROLL and SROLL_CIRC mode the CENTER and RIGHT are pointless so remove them.
				* (In addition they will result misalignment is this case)*/
				draw_dsc.align = LV_TEXT_ALIGN_LEFT;
			}

			size.y = canvas->dsc.header.h;
			if (exec_cb == NULL && size.y > txt_h) {
				if (is_circular) {
					canvas->offset_circ.y = size.y + lv_font_get_line_height(draw_dsc.font);
					end = -canvas->offset_circ.y;
				} else {
					end = txt_h - size.y - lv_font_get_line_height(draw_dsc.font);
				}

				exec_cb = set_ofs_y_anim;
			}

			if (exec_cb) {
				uint16_t anim_speed = lv_obj_get_style_anim_speed(obj, LV_PART_MAIN);
				if (anim_speed == 0) anim_speed = TEXT_CANVAS_DEF_SCROLL_SPEED;

				lv_anim_t anim;
				lv_anim_init(&anim);
				lv_anim_set_var(&anim, obj);
				lv_anim_set_exec_cb(&anim, exec_cb);
				lv_anim_set_values(&anim, start, end);
				lv_anim_set_time(&anim, lv_anim_speed_to_time(anim_speed, start, end));
				lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
				if (is_circular == false) {
					lv_anim_set_playback_time(&anim, anim.time);
					lv_anim_set_playback_delay(&anim, TEXT_CANVAS_SCROLL_DELAY);
					lv_anim_set_repeat_delay(&anim, TEXT_CANVAS_SCROLL_DELAY);
				}

				lv_anim_start(&anim);
			}
		}
	}
#if OPT_LV_TXT_GET_SIZE
	else {
		/* initialize to the maximum height */
		canvas->dsc.header.h = dsc_h_max;
		skip_txt_height = true;
	}
#endif

	canvas->dsc.header.w = ((canvas->dsc_w + dsc_stride_mask) & ~dsc_stride_mask);
	canvas->dsc.header.h = LV_MIN(canvas->dsc.header.h + canvas->ext_draw_size * 2,
			LV_MIN(dsc_h_max + canvas->ext_draw_size, IMG_MAX_H));

	uint32_t buf_size = (uint32_t)canvas->dsc.header.w * canvas->dsc.header.h * px_size / 8;
	if (canvas->dsc.data_size < buf_size) {
		lv_mem_free((void *)canvas->dsc.data);
		canvas->dsc.data = lv_mem_alloc(buf_size);
		canvas->dsc.data_size = canvas->dsc.data ? buf_size : 0;
	}

	LV_LOG_INFO("w %d, h %d, data %p, size 0x%x\n", canvas->dsc.header.w,
			canvas->dsc.header.h, canvas->dsc.data, canvas->dsc.data_size);

	if (canvas->dsc.data == NULL) {
		LV_LOG_ERROR("text_canvas: fail to allocate dsc data %u bytes", buf_size);
		canvas->dsc.header.h = 0;
		goto out_exit;
	}

	if (canvas->emoji_en)
		alloc_emoji_dsc(obj, canvas_text);

	lvgl_draw_label_result_t feedback = { .remain_txt = canvas_text, };
	refr_text_to_img(obj, &draw_dsc, &canvas->dsc, &feedback);

#if OPT_LV_TXT_GET_SIZE
	if (skip_txt_height) {
		/* set the real height */
		feedback.line_count = LV_MAX(1, feedback.line_count);

		uint16_t dsc_h = feedback.line_count * line_height_font +
				(feedback.line_count - 1) * (draw_dsc.line_space) + canvas->ext_draw_size * 2;

		canvas->dsc.header.h = LV_MIN(canvas->dsc.header.h, dsc_h);
	}
#endif

out_exit:
	if (canvas->dsc.header.h <= 0) canvas->ext_draw_size = 0;
	free_cvt_text(obj);
	lv_obj_scroll_to_y(obj, 0, LV_ANIM_OFF);
	lv_obj_refresh_self_size(obj);
	lv_obj_refresh_ext_draw_size(obj);
	lv_obj_invalidate(obj);
}

static uint32_t get_letter_on(lv_obj_t * obj, lv_draw_label_dsc_t *draw_dsc, lv_point_t * pos_in)
{
	text_canvas_t * canvas = (text_canvas_t *)obj;
	const char *canvas_text = canvas->cvt_text ? canvas->cvt_text : canvas->text;

	lv_point_t pos;
	pos.x = pos_in->x;
	pos.y = pos_in->y;

	const char * txt         = canvas_text;
	uint32_t line_start      = 0;
	uint32_t new_line_start  = 0;
	lv_coord_t max_w         = canvas->dsc_w;
	lv_coord_t letter_height = lv_font_get_line_height(draw_dsc->font);
	lv_coord_t y             = 0;
	uint32_t logical_pos;
	char * bidi_txt;

	lv_text_align_t align = draw_dsc->align;

	/*Search the line of the index letter*/;
	while (txt[line_start] != '\0') {
		new_line_start += _lv_txt_get_next_line(&txt[line_start],
				draw_dsc->font, draw_dsc->letter_space, max_w, NULL, draw_dsc->flag);

		if (pos.y <= y + letter_height) {
			/*The line is found (stored in 'line_start')*/
			/*Include the NULL terminator in the last line*/
			uint32_t tmp = new_line_start;
			uint32_t letter;
			letter = _lv_txt_encoded_prev(txt, &tmp);
			if (letter != '\n' && txt[new_line_start] == '\0') new_line_start++;
			break;
		}
		y += letter_height + draw_dsc->line_space;

		line_start = new_line_start;
	}

#if LV_USE_BIDI
	bidi_txt = lv_mem_buf_get(new_line_start - line_start + 1);
	uint32_t txt_len = new_line_start - line_start;
	if (new_line_start > 0 && txt[new_line_start - 1] == '\0' && txt_len > 0) txt_len--;
	_lv_bidi_process_paragraph(txt + line_start, bidi_txt, txt_len, draw_dsc->bidi_dir, NULL, 0);
#else
	bidi_txt = (char *)txt + line_start;
#endif

	/*Calculate the x coordinate*/
	lv_coord_t x = 0;
	if (align == LV_TEXT_ALIGN_CENTER) {
		lv_coord_t line_w;
		line_w = lv_txt_get_width(bidi_txt, new_line_start - line_start, draw_dsc->font, draw_dsc->letter_space, draw_dsc->flag);
		x += canvas->dsc_w / 2 - line_w / 2;
	} else if(align == LV_TEXT_ALIGN_RIGHT) {
		lv_coord_t line_w;
		line_w = lv_txt_get_width(bidi_txt, new_line_start - line_start, draw_dsc->font, draw_dsc->letter_space, draw_dsc->flag);
		x += canvas->dsc_w - line_w;
	}

	lv_text_cmd_state_t cmd_state = LV_TEXT_CMD_STATE_WAIT;

	uint32_t i = 0;
	uint32_t i_act = i;

	if (new_line_start > 0) {
		while(i + line_start < new_line_start) {
			/*Get the current letter and the next letter for kerning*/
			/*Be careful 'i' already points to the next character*/
			uint32_t letter;
			uint32_t letter_next;
			_lv_txt_encoded_letter_next_2(bidi_txt, &letter, &letter_next, &i);

			/*Handle the recolor command*/
			if ((draw_dsc->flag & LV_TEXT_FLAG_RECOLOR) != 0) {
				if (_lv_txt_is_cmd(&cmd_state, bidi_txt[i]) != false) {
					continue; /*Skip the letter if it is part of a command*/
				}
			}

			lv_coord_t gw = lv_font_get_glyph_width(draw_dsc->font, letter, letter_next);

			/*Finish if the x position or the last char of the next line is reached*/
			if (pos.x < x + gw || i + line_start == new_line_start ||  txt[i_act + line_start] == '\0') {
				i = i_act;
				break;
			}
			x += gw;
			x += draw_dsc->letter_space;
			i_act = i;
		}
	}

#if LV_USE_BIDI
	/*Handle Bidi*/
	uint32_t cid = _lv_txt_encoded_get_char_id(bidi_txt, i);
	if (txt[line_start + i] == '\0') {
		logical_pos = i;
	} else {
		bool is_rtl;
		logical_pos = _lv_bidi_get_logical_pos(&txt[line_start], NULL,
											   txt_len, draw_dsc->bidi_dir, cid, &is_rtl);
		if (is_rtl) logical_pos++;
	}
	lv_mem_buf_release(bidi_txt);
#else
	logical_pos = _lv_txt_encoded_get_char_id(bidi_txt, i);
#endif

	return logical_pos + _lv_txt_encoded_get_char_id(txt, line_start);
}

static void free_cvt_text(lv_obj_t *obj)
{
	text_canvas_t * canvas = (text_canvas_t *)obj;

	if (canvas->cvt_text) {
		lv_mem_free(canvas->cvt_text);
		canvas->cvt_text = NULL;
	}
}

static void replace_text_dot_end(lv_obj_t *obj, lv_draw_label_dsc_t *draw_dsc)
{
	text_canvas_t * canvas = (text_canvas_t *)obj;
	const char *canvas_text = canvas->cvt_text ? canvas->cvt_text : canvas->text;
	lv_coord_t y_overed, dot_overed;
	lv_point_t p;

	dot_overed = (lv_font_get_glyph_width(draw_dsc->font, '.', '.') + draw_dsc->letter_space) *
				TEXT_CANVAS_DOT_NUM; /*Shrink with dots*/
	if (draw_dsc->bidi_dir == LV_BASE_DIR_RTL) {
		p.x = dot_overed; /*Shrink with dots*/
	} else {
		p.x = canvas->dsc_w - dot_overed; /*Shrink with dots*/
	}

	p.y = canvas->dsc.header.h;
	y_overed = p.y %
				(lv_font_get_line_height(draw_dsc->font) + draw_dsc->line_space); /*Round down to the last line*/
	if (y_overed >= lv_font_get_line_height(draw_dsc->font)) {
		p.y -= y_overed;
		p.y += lv_font_get_line_height(draw_dsc->font);
	} else {
		p.y -= y_overed;
		p.y -= draw_dsc->line_space;
	}

	uint32_t letter_id = get_letter_on(obj, draw_dsc, &p);
	uint32_t byte_id = _lv_txt_encoded_get_byte_id(canvas_text, letter_id);
	bool is_cvt_text_nil = (canvas->cvt_text == NULL);

	canvas->cvt_text = lv_mem_realloc(canvas->cvt_text, byte_id + TEXT_CANVAS_DOT_NUM + 1);
	if (canvas->cvt_text) {
		if (is_cvt_text_nil)
			memcpy(canvas->cvt_text, canvas->text, byte_id);

		for (int i = 0; i < TEXT_CANVAS_DOT_NUM; i++)
			canvas->cvt_text[byte_id + i] = '.';

		canvas->cvt_text[byte_id + TEXT_CANVAS_DOT_NUM] = '\0';
	} else {
		LV_LOG_ERROR("malloc failed for long dots");
	}
}

static void refr_text_to_img(lv_obj_t *obj, lv_draw_label_dsc_t *draw_dsc,
		lv_img_dsc_t *img_dsc, lvgl_draw_label_result_t *feedback)
{
	text_canvas_t * canvas = (text_canvas_t *)obj;
	uint32_t buf_size = img_dsc->header.w * img_dsc->header.h * lv_img_cf_get_px_size(img_dsc->header.cf) / 8;

	memset((void *)img_dsc->data, 0, buf_size);

	/*Create a dummy display to fool the lv_draw function.
	 *It will think it draws to real screen.*/
	lv_disp_t fake_disp;
	lv_disp_drv_t driver;
	lv_draw_ctx_t * draw_ctx;
	lv_area_t coords, buf_area;

	lv_area_set(&buf_area, 0, 0, img_dsc->header.w - 1, img_dsc->header.h - 1);
	lv_area_set(&coords, 0, canvas->ext_draw_size, canvas->dsc_w - 1, img_dsc->header.h - 1);

	draw_ctx = lv_mem_alloc(sizeof(lv_draw_sw_ctx_t));
	LV_ASSERT_MALLOC(draw_ctx);
	if(draw_ctx == NULL) return;
	lv_draw_sw_init_ctx(&driver, draw_ctx);
	draw_ctx->clip_area = &buf_area;
	draw_ctx->buf_area = &buf_area;
	draw_ctx->buf = (void *)img_dsc->data;
	draw_ctx->draw_letter = draw_letter_cb;
	draw_ctx->user_data = img_dsc;

	memset(&fake_disp, 0, sizeof(lv_disp_t));
	fake_disp.driver = &driver;
	fake_disp.driver->hor_res = img_dsc->header.w;
	fake_disp.driver->ver_res = img_dsc->header.h;
	fake_disp.driver->draw_ctx = draw_ctx;
	//fake_disp.driver->set_px_cb = set_px_cb_alpha;
	fake_disp.driver->user_data = img_dsc;

	lv_disp_t * disp_ori = _lv_refr_get_disp_refreshing();
	_lv_refr_set_disp_refreshing(&fake_disp);

#ifdef CONFIG_LVGL_USE_FREETYPE_FONT
	lvgl_freetype_force_bitmap((lv_font_t *)draw_dsc->font, 1);
#endif
	if (canvas->emoji_dsc.max_count > 0) {
		lvgl_draw_label_with_emoji(draw_ctx, draw_dsc, &coords,
				feedback->remain_txt, NULL, feedback, &canvas->emoji_dsc);

		if (canvas->emoji_dsc.count > 0) {
			canvas->emoji_dsc.font = draw_dsc->font;

			for (int i = canvas->emoji_dsc.count - 1; i >= 0; i--) {
				lvgl_emoji_dsc_t *emoji = &canvas->emoji_dsc.emoji[i];

				/* load to cache, get the img size, and adjust the y offset */
				lvgl_bitmap_font_get_emoji_dsc(canvas->emoji_dsc.font,
						emoji->code, &emoji->img, &emoji->pos, true);
			}
		}
	} else {
		lvgl_draw_label_with_emoji(draw_ctx, draw_dsc, &coords,
				feedback->remain_txt, NULL, feedback, NULL);
	}
#ifdef CONFIG_LVGL_USE_FREETYPE_FONT
	lvgl_freetype_force_bitmap((lv_font_t*)draw_dsc->font, 0);
#endif

	_lv_refr_set_disp_refreshing(disp_ori);
	lv_draw_sw_deinit_ctx(fake_disp.driver, fake_disp.driver->draw_ctx);
	lv_mem_free(fake_disp.driver->draw_ctx);

	mem_dcache_clean(img_dsc->data, buf_size);
}

static lv_img_cf_t get_text_font_img_cf(const lv_font_t *font)
{
	lv_font_glyph_dsc_t dsc_out;

	if (lv_font_get_glyph_dsc(font, &dsc_out, ' ', ' ') == false) {
		LV_LOG_WARN("fail to get font glyph bpp, so fall back to 4\n");
		dsc_out.bpp = 4; /* default to 4 */
	}

	LV_LOG_INFO("font bpp %d\n", dsc_out.bpp);

#ifdef CONFIG_SOC_SERIES_LARK
	if (dsc_out.bpp == 8) {
		return LV_IMG_CF_ALPHA_8BIT;
	} else if (dsc_out.bpp == 4) {
		return LV_IMG_CF_ALPHA_4BIT_LE;
	} else if (dsc_out.bpp == 2) {
		//return LV_IMG_CF_ALPHA_2BIT;
		return LV_IMG_CF_ALPHA_4BIT_LE;
	} else if (dsc_out.bpp == 1) {
		return LV_IMG_CF_ALPHA_1BIT_LE;
	} else {
		LV_LOG_ERROR("unsupported font bpp %d\n", dsc_out.bpp);
		return LV_IMG_CF_UNKNOWN;
	}
#else
	if (dsc_out.bpp == 8) {
		return LV_IMG_CF_ALPHA_8BIT;
	} else if (dsc_out.bpp == 4) {
		return LV_IMG_CF_ALPHA_4BIT;
	} else if (dsc_out.bpp == 2) {
		return LV_IMG_CF_ALPHA_2BIT;
	} else if (dsc_out.bpp == 1) {
		return LV_IMG_CF_ALPHA_1BIT;
	} else {
		LV_LOG_ERROR("unsupported font bpp %d\n", dsc_out.bpp);
		return LV_IMG_CF_UNKNOWN;
	}
#endif /* CONFIG_DISPLAY_ENGINE_LARK */
}

#if 0
static void set_px_cb_alpha(lv_disp_drv_t * disp_drv, uint8_t * buf, lv_coord_t buf_w, lv_coord_t x, lv_coord_t y,
							 lv_color_t color, lv_opa_t opa)
{
	lv_img_cf_t cf = ((lv_img_dsc_t *)disp_drv->user_data)->header.cf;

	if (opa <= LV_OPA_MIN) return;

	if (cf == LV_IMG_CF_ALPHA_4BIT_LE) {
		uint8_t bit = (x & 0x1) * 4;

		buf += (buf_w >> 1) * y + (x >> 1);
		buf[0] &= ~(0xF << bit);
		buf[0] |= ((opa >> 4) << bit); /* opa -> [0,15] */
	} else if (cf == LV_IMG_CF_ALPHA_1BIT_LE) {
		uint8_t bit = x & 0x7;

		buf += (buf_w >> 3) * y + (x >> 3);
		buf[0] &= ~(1 << bit);
		buf[0] |= ((opa >> 7) << bit); /* opa -> [0,1] */
	} else if (cf == LV_IMG_CF_ALPHA_4BIT) {
		uint8_t bit = 4 - ((x & 0x1) * 4);

		buf += (buf_w >> 1) * y + (x >> 1);
		buf[0] &= ~(0xF << bit);
		buf[0] |= ((opa >> 4) << bit); /* opa -> [0,15] */
	} else if(cf == LV_IMG_CF_ALPHA_2BIT) {
		uint8_t bit = 6 - ((x & 0x3) * 2);

		buf += (buf_w >> 2) * y + (x >> 2);
		buf[0] &= ~(3 << bit);
		buf[0] |= ((opa >> 6) << bit); /* opa -> [0,3] */
	} else if (cf == LV_IMG_CF_ALPHA_1BIT) {
		uint8_t bit = 7 - (x & 0x7);

		buf += (buf_w >> 3) * y + (x >> 3);
		buf[0] &= ~(1 << bit);
		buf[0] |= ((opa >> 7) << bit); /* opa -> [0,1] */
	} else if (cf == LV_IMG_CF_ALPHA_8BIT) {
		buf += buf_w * y + x;
		buf[0] = opa;
	}
}
#endif

static void draw_letter_cb(lv_draw_ctx_t * draw_ctx, const lv_draw_label_dsc_t * dsc,
		const lv_point_t * pos_p, uint32_t letter)
{
	lv_font_glyph_dsc_t g;
	if (lv_font_get_glyph_dsc(dsc->font, &g, letter, '\0') == false) {
		return;
	}

	/*Don't draw anything if the character is empty. E.g. space*/
	if ((g.box_h == 0) || (g.box_w == 0)) return;

	lv_point_t gpos;
	gpos.x = pos_p->x + g.ofs_x;
	gpos.y = pos_p->y + (dsc->font->line_height - dsc->font->base_line) - g.box_h - g.ofs_y;

	/*If the letter is completely out of mask don't draw it*/
	if (gpos.x + g.box_w < draw_ctx->clip_area->x1 ||
		gpos.x > draw_ctx->clip_area->x2 ||
		gpos.y + g.box_h < draw_ctx->clip_area->y1 ||
		gpos.y > draw_ctx->clip_area->y2)  {
		return;
	}

	const uint8_t * map_p = lv_font_get_glyph_bitmap(g.resolved_font, letter);
	if (map_p == NULL) {
		LV_LOG_WARN("lv_draw_letter: character's bitmap not found");
		return;
	}

	lv_img_cf_t cf = ((lv_img_dsc_t *)draw_ctx->user_data)->header.cf;
	uint16_t draw_bpp = lv_img_cf_get_px_size(cf);

	uint16_t bitmask = (1u << g.bpp) - 1;
	uint16_t bpp = g.bpp;
	int16_t col, row;

	/* Calculate the col/row start/end on the map*/
	int16_t col_start = gpos.x >= draw_ctx->clip_area->x1 ? 0 : draw_ctx->clip_area->x1 - gpos.x;
	int16_t col_end   = gpos.x + g.box_w <= draw_ctx->clip_area->x2 ? g.box_w : draw_ctx->clip_area->x2 - gpos.x + 1;
	int16_t row_start = gpos.y >= draw_ctx->clip_area->y1 ? 0 : draw_ctx->clip_area->y1 - gpos.y;
	int16_t row_end   = gpos.y + g.box_h <= draw_ctx->clip_area->y2 ? g.box_h : draw_ctx->clip_area->y2 - gpos.y + 1;
	int16_t num_cols  = col_end - col_start;
	int16_t num_rows  = row_end - row_start;

	/*Move on the map too*/
	uint32_t bit_ofs = (row_start * g.box_w + col_start) * bpp;
	map_p += bit_ofs >> 3;

	uint16_t col_bit = bit_ofs & 0x7; /* "& 0x7" equals to "% 8" just faster */
	uint16_t col_bit_max = 8 - bpp;
	uint16_t col_bit_row_ofs = (g.box_w - num_cols) * bpp;

	/* Calculate the draw buf*/
	uint16_t draw_row_ofs = lv_area_get_width(draw_ctx->buf_area) * draw_bpp / 8;
	uint16_t draw_bit_col_ofs = (col_start + gpos.x - draw_ctx->buf_area->x1) * draw_bpp;
	uint16_t draw_col_bit_init = draw_bit_col_ofs & 0x7;
	uint8_t *draw_p = (uint8_t *)draw_ctx->buf +
		(row_start + gpos.y - draw_ctx->buf_area->y1) * draw_row_ofs + (draw_bit_col_ofs >> 3);

	if (cf == LV_IMG_CF_ALPHA_4BIT_LE && bpp == 2) {
		const lv_opa_t alpha2t4_opa_table[] = { 0, 5, 10, 15 };

		for (row = num_rows; row > 0; row--) {
			uint8_t *draw_p_tmp = draw_p;
			uint16_t draw_col_bit = draw_col_bit_init;
			uint16_t col_bit_be = col_bit_max - col_bit;
			uint8_t map_px = *map_p;

			for (col = num_cols; col > 0; col--) {
				/*Load the pixel's opacity into the mask*/
				uint8_t letter_px = (map_px >> col_bit_be) & bitmask;

				if (letter_px) {
					draw_p_tmp += (draw_col_bit >> 3);
					draw_col_bit &= 0x7;

					/* already do memset 0 */
					*draw_p_tmp |= alpha2t4_opa_table[letter_px] << draw_col_bit;
				}

				/*Go to the next column*/
				if (col_bit_be > 0) {
					col_bit_be -= bpp;
				} else {
					col_bit_be = col_bit_max;
					map_px = *(++map_p);
				}

				draw_col_bit += draw_bpp;
			}

			col_bit = col_bit_max - col_bit_be;
			col_bit += col_bit_row_ofs;
			map_p += (col_bit >> 3);
			col_bit = col_bit & 0x7;

			draw_p += draw_row_ofs;
		}
	} else if (cf == LV_IMG_CF_ALPHA_4BIT_LE || cf == LV_IMG_CF_ALPHA_1BIT_LE) {
		for (row = num_rows; row > 0; row--) {
			uint8_t *draw_p_tmp = draw_p;
			uint16_t draw_col_bit = draw_col_bit_init;
			uint16_t col_bit_be = col_bit_max - col_bit;
			uint8_t map_px = *map_p;

			for (col = num_cols; col > 0; col--) {
				/*Load the pixel's opacity into the mask*/
				uint8_t letter_px = (map_px >> col_bit_be) & bitmask;

				if (letter_px) {
					draw_p_tmp += (draw_col_bit >> 3);
					draw_col_bit &= 0x7;

					/* already do memset 0 */
					*draw_p_tmp |= letter_px << draw_col_bit;
				}

				/*Go to the next column*/
				if (col_bit_be > 0) {
					col_bit_be -= bpp;
				} else {
					col_bit_be = col_bit_max;
					map_px = *(++map_p);
				}

				draw_col_bit += bpp;
			}

			col_bit = col_bit_max - col_bit_be;
			col_bit += col_bit_row_ofs;
			map_p += (col_bit >> 3);
			col_bit = col_bit & 0x7;

			draw_p += draw_row_ofs;
		}
	} else {
		for (row = num_rows; row > 0; row--) {
			uint8_t *draw_p_tmp = draw_p;
			uint16_t draw_col_bit = draw_col_bit_init;
			uint16_t col_bit_be = col_bit_max - col_bit;
			uint8_t map_px = *map_p;

			for (col = num_cols; col > 0; col--) {
				/*Load the pixel's opacity into the mask*/
				uint8_t letter_px = (map_px >> col_bit_be) & bitmask;

				if (letter_px) {
					draw_p_tmp += (draw_col_bit >> 3);
					draw_col_bit &= 0x7;

					/* already do memset 0 */
					*draw_p_tmp |= letter_px << (col_bit_max - draw_col_bit);
				}

				/*Go to the next column*/
				if (col_bit_be > 0) {
					col_bit_be -= bpp;
				} else {
					col_bit_be = col_bit_max;
					map_px = *(++map_p);
				}

				draw_col_bit += bpp;
			}

			col_bit = col_bit_max - col_bit_be;
			col_bit += col_bit_row_ofs;
			map_p += (col_bit >> 3);
			col_bit = col_bit & 0x7;

			draw_p += draw_row_ofs;
		}
	}
}

static void alloc_emoji_dsc(lv_obj_t *obj, const char *txt)
{
	text_canvas_t * canvas = (text_canvas_t *)obj;
	uint32_t unicode, unicode_next, offset = 0;
	uint16_t emoji_count = 0;

	while (txt[offset] != 0) {
		_lv_txt_encoded_letter_next_2(txt, &unicode, &unicode_next, &offset);
		if (unicode >= EMOJI_UNICODE_START) {
			emoji_count++;
		}
	}

	if (TEXT_CANVAS_EMOJI_NUM > 0 && emoji_count > TEXT_CANVAS_EMOJI_NUM) {
		emoji_count = TEXT_CANVAS_EMOJI_NUM;
	}

	if (emoji_count > canvas->emoji_dsc.max_count) {
		//pre malloc here, real count will be calculated after,
		if (canvas->emoji_dsc.emoji)
			lv_mem_free(canvas->emoji_dsc.emoji);

		canvas->emoji_dsc.emoji = lv_mem_alloc(emoji_count * sizeof(lvgl_emoji_dsc_t));
		if (canvas->emoji_dsc.emoji == NULL) {
			LV_LOG_ERROR("malloc failed, too many emoji %u", emoji_count);
			canvas->emoji_dsc.max_count = 0;
		} else {
			canvas->emoji_dsc.max_count = emoji_count;
		}
	}

	canvas->emoji_dsc.count = 0;
}

static void free_emoji_dsc(lvgl_draw_emoji_dsc_t *emoji_dsc)
{
	if (emoji_dsc->emoji) {
		lv_mem_free(emoji_dsc->emoji);
		emoji_dsc->emoji = NULL;
	}

	emoji_dsc->max_count = 0;
	emoji_dsc->count = 0;
}

static void set_ofs_x_anim(void * obj, int32_t v)
{
	text_canvas_t * canvas = (text_canvas_t *)obj;

	v &= ~canvas->dsc_x_mask;
	if (v != canvas->offset.x) {
		canvas->offset.x = v;
		lv_obj_invalidate(obj);
	}
}

static void set_ofs_y_anim(void * obj, int32_t v)
{
	text_canvas_t * canvas = (text_canvas_t *)obj;

	if (v != canvas->offset.y) {
		canvas->offset.y = v;
		lv_obj_invalidate(obj);
	}
}
