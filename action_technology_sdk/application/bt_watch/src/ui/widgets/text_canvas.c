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
#include <lvgl/src/lvgl_private.h>
#include <lvgl/lvgl_bitmap_font.h>
#include <lvgl/src/misc/lv_text_ap.h>
#ifdef CONFIG_LVGL_USE_FREETYPE_FONT
#include <lvgl/lvgl_freetype_font.h>
#endif
#include "text_canvas.h"

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &text_canvas_class

#define TEXT_CANVAS_DEF_SCROLL_SPEED   (lv_anim_speed_clamped(40, 300, 10000))
#define TEXT_CANVAS_SCROLL_DELAY       300

#define OPT_LV_TXT_GET_SIZE       1
#define OPT_LONG_SCROLL_HW_ALIGN  1

/* lv_image_dsc_t only support image height less than 2048. */
#define IMG_MAX_H                 UINT16_MAX

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
	lv_font_glyph_dsc_t g;
	lv_point_t pos;
} lvgl_emoji_dsc_t;

typedef struct {
	lvgl_emoji_dsc_t * emoji;
	const lv_font_t * font;

	uint16_t count;
	uint16_t max_count;
} lvgl_draw_emoji_dsc_t;

/** Data of text canvas */
typedef struct {
	lv_obj_t obj; /*Ext. of ancestor*/

	/*New data for this type */
	char * text;
	lv_image_dsc_t dsc;
	uint32_t dsc_w;     /* effective width in pixels, may be less than dsc.header.w */
	uint32_t dsc_h_max; /* max height of dsc */
	uint8_t dsc_w_mask;   /* dsc_w align mask (align to 1 byte) */
	uint8_t dsc_x_mask;  /* dsc x position align mask*/
	uint8_t ext_draw_size; /* extra draw area on the bottom */

	char * cvt_text;   /*pre-processed text. text replaced with end dots or replaced by ARABIC_PERSIAN_CHARS */
	lv_point_t offset; /*Text draw position offset*/
	lv_point_t offset_circ; /*Text draw position circular offset for TEXT_CANVAS_LONG_SCROLL_CIRCULAR */
	text_canvas_long_mode_t long_mode : 3; /* Determinate what to do with the long texts */
	uint8_t static_txt : 1; /*Flag to indicate the text is static*/
	uint8_t expand : 1;  /*Ignore real width*/
	uint8_t expand_scroll : 1;  /*Ignore real width and scroll the text out of the object*/
	uint8_t force_a8 : 1;

	uint8_t emoji_en : 1;
	lvgl_draw_emoji_dsc_t emoji_dsc;
} text_canvas_t;

typedef struct {
	lv_draw_unit_t base;
	text_canvas_t *canvas;
} text_canvas_draw_unit_t;

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
static void refr_text_to_img(lv_obj_t *obj, lv_draw_label_dsc_t *draw_dsc);

static void free_cvt_text(lv_obj_t *obj);
static void replace_text_dot_end(lv_obj_t *obj, lv_draw_label_dsc_t *draw_dsc);
static uint32_t get_letter_on(lv_obj_t * obj, lv_draw_label_dsc_t *draw_dsc, lv_point_t * pos_in);

static lv_color_format_t get_text_font_color_format(lv_obj_t * obj, const lv_font_t *font);
static void draw_letter_cb(lv_draw_unit_t * draw_unit, lv_draw_glyph_dsc_t * dsc,
		lv_draw_fill_dsc_t * fill_dsc, const lv_area_t * fill_area);

static void append_emoji_dsc(text_canvas_t * canvas, const lv_font_glyph_dsc_t *g, const lv_point_t * pos);
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
	.name = "text-canvas",
};

static int32_t g_line_count;
static int32_t g_line_pos_y;

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

/*=====================
 * Setter functions
 *====================*/

void text_canvas_set_emoji_enable(lv_obj_t* obj, bool en)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);
	text_canvas_t * canvas = (text_canvas_t *)obj;
	canvas->emoji_en = en;

	if (!en) {
		free_emoji_dsc(&canvas->emoji_dsc);
	}
}

void text_canvas_set_text(lv_obj_t * obj, const char * text)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	text_canvas_t * canvas = (text_canvas_t *)obj;

	if (text == NULL) {
		text_canvas_set_text_static(obj, text);
		return;
	}

	if (canvas->text && !canvas->static_txt) {
		lv_free(canvas->text);
	}

	canvas->text = NULL;
	canvas->static_txt = 0;

#if LV_USE_ARABIC_PERSIAN_CHARS
	size_t len = lv_text_ap_calc_bytes_count(text);

	canvas->text = lv_malloc(len);
	if(canvas->text == NULL) return;

	lv_text_ap_proc(text, canvas->text);
#else
	size_t len = lv_strlen(text) + 1;

	canvas->text = lv_malloc(len);
	if (canvas->text == NULL) return;

	lv_strncpy(canvas->text, text, len);
#endif /* LV_USE_ARABIC_PERSIAN_CHARS */

	text_canvas_refr_text(obj);
}

void text_canvas_set_text_fmt(lv_obj_t * obj, const char * fmt, ...)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	text_canvas_t * canvas = (text_canvas_t *)obj;

	if (fmt == NULL) {
		text_canvas_set_text_static(obj, NULL);
		return;
	}

	if (canvas->text && !canvas->static_txt) {
		lv_free(canvas->text);
	}

	canvas->text = NULL;
	canvas->static_txt = 0;

	va_list args;
	va_start(args, fmt);
	canvas->text = lv_text_set_text_vfmt(fmt, args);
	va_end(args);

	if (canvas->text == NULL) return;

	text_canvas_refr_text(obj);
}

void text_canvas_set_text_static(lv_obj_t * obj, const char * text)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	text_canvas_t * canvas = (text_canvas_t *)obj;

	if (canvas->text && !canvas->static_txt) {
		lv_free(canvas->text);
	}

	canvas->text = (char *)text;
	canvas->static_txt = 1;

	text_canvas_refr_text(obj);
}

void text_canvas_set_long_mode(lv_obj_t * obj, text_canvas_long_mode_t long_mode)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

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

void text_canvas_set_img_8bit(lv_obj_t * obj, bool en)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	text_canvas_t * canvas = (text_canvas_t *)obj;

	canvas->force_a8 = en ? 1 : 0;
	if (canvas->text)
		text_canvas_refr_text(obj);
}

/*=====================
 * Getter functions
 *====================*/

lv_image_dsc_t * text_canvas_get_img(lv_obj_t * obj)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	text_canvas_t * canvas = (text_canvas_t *)obj;

	return &canvas->dsc;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void text_canvas_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	text_canvas_t * canvas = (text_canvas_t *)obj;

	text_canvas_set_text_static(obj, NULL);

	ui_mem_free(MEM_RES, (void *)canvas->dsc.data);
	canvas->dsc.data_size = 0;

	free_emoji_dsc(&canvas->emoji_dsc);
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
		lv_result_t res = lv_obj_event_base(MY_CLASS, e);
		if (res != LV_RESULT_OK) return;
	}

	if (code == LV_EVENT_STYLE_CHANGED) {
		if (canvas->text) {
			LV_LOG_INFO("style changed after text may reduce speed");
			text_canvas_refr_text(obj);
		}
	} else if (code == LV_EVENT_SIZE_CHANGED) {
		if (canvas->text)
			text_canvas_refr_text(obj);
	} else if (code == LV_EVENT_REFR_EXT_DRAW_SIZE) {
		lv_event_set_ext_draw_size(e, canvas->ext_draw_size);
	} else if (code == LV_EVENT_GET_SELF_SIZE) {
		lv_point_t * p = lv_event_get_param(e);
		p->x = canvas->dsc.header.w;
		p->y = canvas->dsc.header.h - canvas->ext_draw_size * 2;
	} else if (code == LV_EVENT_COVER_CHECK) {
		lv_event_set_cover_res(e, LV_COVER_RES_NOT_COVER);
	} else if (code == LV_EVENT_DRAW_MAIN) {
		text_canvas_draw(e);
	}
}

static void text_canvas_draw(lv_event_t * e)
{
	lv_layer_t * layer = lv_event_get_layer(e);
	lv_obj_t * obj = lv_event_get_target(e);
	text_canvas_t * canvas = (text_canvas_t *)obj;

	lv_area_t txt_area, draw_area;
	lv_point_t offsets[2];
	int i, parts = 1;

	lv_area_t dsc_coords;
	lv_obj_get_content_coords(obj, &dsc_coords);
	dsc_coords.y1 -= canvas->ext_draw_size;

	offsets[0] = canvas->offset;
	if (canvas->offset_circ.x || canvas->offset_circ.y) {
		offsets[1].x = offsets[0].x + canvas->offset_circ.x;
		offsets[1].y = offsets[0].y + canvas->offset_circ.y;
		parts = 2;
	}

	lv_draw_image_dsc_t draw_dsc;
	lv_draw_image_dsc_init(&draw_dsc);
	//lv_obj_init_draw_img_dsc(obj, LV_IMG_PART_MAIN, &draw_dsc);
	draw_dsc.recolor = lv_obj_get_style_text_color(obj, LV_PART_MAIN);
	draw_dsc.opa = lv_obj_get_style_text_opa(obj, LV_PART_MAIN);
	draw_dsc.src = &canvas->dsc;

	for (i = 0; i < parts; i++) {
		txt_area.x1 = dsc_coords.x1 + offsets[i].x;
		txt_area.y1 = dsc_coords.y1 + offsets[i].y;
		txt_area.x2 = txt_area.x1 + canvas->dsc.header.w - 1;
		txt_area.y2 = txt_area.y1 + canvas->dsc.header.h - 1;

		if (lv_area_intersect(&draw_area, &txt_area, &layer->_clip_area) == false)
			continue;

		//assert(((txt_area.x1 - draw_area.x1) & canvas->dsc_x_mask) == 0);

		lv_draw_image(layer, &draw_dsc, &txt_area);

		if (canvas->emoji_dsc.count > 0) {
			for (int j = canvas->emoji_dsc.count - 1; j >= 0; j--) {
				lvgl_emoji_dsc_t *emoji = &canvas->emoji_dsc.emoji[j];

				draw_dsc.src = lv_font_get_glyph_bitmap(&emoji->g, NULL);
				if (draw_dsc.src) {
					lv_image_header_t header;
					lv_image_decoder_get_info(draw_dsc.src, &header);

					lv_area_t emoji_area;
					emoji_area.x1 = emoji->pos.x + txt_area.x1;
					emoji_area.y1 = emoji->pos.y + txt_area.y1;
					emoji_area.x2 = emoji_area.x1 + header.w - 1;
					emoji_area.y2 = emoji_area.y1 + header.h - 1;
					lv_draw_image(layer, &draw_dsc, &emoji_area);

					lv_font_glyph_release_draw_data(&emoji->g);
				}
			}
		}
	}
}

static void text_canvas_refr_text(lv_obj_t *obj)
{
	text_canvas_t * canvas = (text_canvas_t *)obj;
	lv_coord_t sty_max_h = lv_obj_get_style_max_height(obj, LV_PART_MAIN);
	lv_coord_t dsc_h_max = LV_MIN(sty_max_h, IMG_MAX_H);
	lv_coord_t txt_w = lv_obj_get_content_width(obj);
	lv_coord_t txt_h = lv_obj_get_content_height(obj);
	const char *canvas_text = canvas->text;
	uint16_t line_height, line_height_font;
	bool skip_txt_height = false;

	/*Delete the old scroll animation (if exists)*/
	lv_anim_delete(obj, set_ofs_x_anim);
	lv_anim_delete(obj, set_ofs_y_anim);
	lv_point_set(&canvas->offset, 0, 0);
	lv_point_set(&canvas->offset_circ, 0, 0);

	canvas->emoji_dsc.count = 0;

	if (canvas_text == NULL) {
		canvas->dsc.header.h = 0;
		lv_obj_refresh_self_size(obj);
		goto out_exit;
	}

	bool is_content_w = (lv_obj_get_style_width(obj, LV_PART_MAIN) == LV_SIZE_CONTENT && !obj->w_layout);
	bool is_content_h = (lv_obj_get_style_height(obj, LV_PART_MAIN) == LV_SIZE_CONTENT);

	if (!is_content_h && dsc_h_max > txt_h)
		dsc_h_max = txt_h;

	canvas->dsc_w = txt_w;
	if (canvas->dsc_w <= 0 && !is_content_w) {
		canvas->dsc.header.h = 0;
		goto out_exit;
	}

	lv_draw_label_dsc_t draw_dsc;
	lv_draw_label_dsc_init(&draw_dsc);
	lv_obj_init_draw_label_dsc(obj, LV_PART_MAIN, &draw_dsc);
	lv_bidi_calculate_align(&draw_dsc.align, &draw_dsc.bidi_dir, canvas_text);

	draw_dsc.text = canvas_text;
	draw_dsc.opa = LV_OPA_COVER;
	draw_dsc.flag = LV_TEXT_FLAG_NONE;
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

	canvas->dsc.header.cf = get_text_font_color_format(obj, draw_dsc.font);
	if (canvas->dsc.header.cf == LV_COLOR_FORMAT_UNKNOWN) {
		canvas->dsc.header.h = 0;
		goto out_exit;
	}

	uint8_t px_size = lv_color_format_get_bpp(canvas->dsc.header.cf);
	canvas->dsc_x_mask = (8 / px_size) - 1; /* 1-byte aligned */

#if OPT_LV_TXT_GET_SIZE
	/* invalid sty_max_h if greater than IMG_MAX_H */
	if (is_content_w || (is_content_h && sty_max_h > IMG_MAX_H) || canvas->long_mode != TEXT_CANVAS_LONG_WRAP)
#endif /* OPT_LV_TXT_GET_SIZE */
	{
		lv_point_t size;
		lv_text_flag_t flag = draw_dsc.flag;
		if (is_content_w) flag |= LV_TEXT_FLAG_FIT;

		bool exceeded = lvgl_txt_get_size(&size, draw_dsc.text, draw_dsc.font, draw_dsc.letter_space,
				draw_dsc.line_space, canvas->dsc_w, dsc_h_max, flag);

		if (is_content_w || canvas->expand_scroll)
			canvas->dsc_w = size.x;

		if (is_content_w) txt_w = canvas->dsc_w;

		canvas->dsc.header.h = LV_MIN(size.y, dsc_h_max);
		if (is_content_h) {
			txt_h = canvas->dsc.header.h;
		} else if (canvas->long_mode == TEXT_CANVAS_LONG_CLIP) {
			canvas->dsc.header.h = LV_MIN(canvas->dsc.header.h, (uint32_t)txt_h);
		}

		if (canvas->long_mode == TEXT_CANVAS_LONG_DOT ||
			canvas->long_mode == TEXT_CANVAS_LONG_LIST) {
			if (exceeded && lvgl_txt_has_encoded_length(draw_dsc.text, TEXT_CANVAS_DOT_NUM)) {
				if (canvas->long_mode == TEXT_CANVAS_LONG_LIST) {
					canvas->dsc.header.h = LV_MIN(line_height_font, dsc_h_max);
					if (is_content_h) txt_h = canvas->dsc.header.h;
				}

				replace_text_dot_end(obj, &draw_dsc);
				draw_dsc.text = canvas->cvt_text ? canvas->cvt_text : canvas->text;
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
				uint32_t anim_time = lv_obj_get_style_anim_duration(obj, LV_PART_MAIN);
				if(anim_time == 0) anim_time = TEXT_CANVAS_DEF_SCROLL_SPEED;

				lv_anim_t anim;
				lv_anim_init(&anim);
				lv_anim_set_var(&anim, obj);
				lv_anim_set_exec_cb(&anim, exec_cb);
				lv_anim_set_values(&anim, start, end);
				lv_anim_set_duration(&anim, anim_time);
				lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
				if (is_circular == false) {
					lv_anim_set_playback_time(&anim, anim_time);
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

	canvas->dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
	canvas->dsc.header.w = canvas->dsc_w;
	canvas->dsc.header.h = LV_MIN(canvas->dsc.header.h + canvas->ext_draw_size * 2,
			LV_MIN(dsc_h_max + canvas->ext_draw_size, IMG_MAX_H));
	canvas->dsc.header.stride = (canvas->dsc_w * px_size + 7) / 8;

	uint32_t buf_size = (uint32_t)canvas->dsc.header.stride * canvas->dsc.header.h;
	if (canvas->dsc.data_size < buf_size) {
		ui_mem_free(MEM_RES, (void *)canvas->dsc.data);
#if LV_USE_DRAW_VG_LITE
		if (canvas->force_a8) {
			canvas->dsc.data = ui_mem_aligned_alloc(MEM_RES, 64, buf_size, __func__);
		} else
#endif
		{
			canvas->dsc.data = ui_mem_alloc(MEM_RES, buf_size, __func__);
		}

		canvas->dsc.data_size = canvas->dsc.data ? buf_size : 0;
	}

	LV_LOG_INFO("w %d, h %d, data %p, size 0x%x\n", canvas->dsc.header.w,
			canvas->dsc.header.h, canvas->dsc.data, canvas->dsc.data_size);

	if (canvas->dsc.data == NULL) {
		LV_LOG_ERROR("text_canvas: fail to allocate dsc data %u bytes", buf_size);
		canvas->dsc.header.h = 0;
		goto out_exit;
	}

	/*Initialize emoji descriptor*/
	canvas->emoji_dsc.count = 0;
	canvas->emoji_dsc.font = draw_dsc.font;
	/*Initialize line counter*/
	g_line_count = 0;
	g_line_pos_y = -1;

	refr_text_to_img(obj, &draw_dsc);

#if OPT_LV_TXT_GET_SIZE
	if (skip_txt_height) {
		/* set the real height */
		g_line_count = LV_MAX(1, g_line_count);

		uint16_t dsc_h = g_line_count * line_height_font +
				(g_line_count - 1) * (draw_dsc.line_space) + canvas->ext_draw_size * 2;
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
		new_line_start += lv_text_get_next_line(&txt[line_start],
				draw_dsc->font, draw_dsc->letter_space, max_w, NULL, draw_dsc->flag);

		if (pos.y <= y + letter_height) {
			/*The line is found (stored in 'line_start')*/
			/*Include the NULL terminator in the last line*/
			uint32_t tmp = new_line_start;
			uint32_t letter;
			letter = lv_text_encoded_prev(txt, &tmp);
			if (letter != '\n' && txt[new_line_start] == '\0') new_line_start++;
			break;
		}
		y += letter_height + draw_dsc->line_space;

		line_start = new_line_start;
	}

#if LV_USE_BIDI
	bidi_txt = lv_malloc(new_line_start - line_start + 1);
	uint32_t txt_len = new_line_start - line_start;
	if (new_line_start > 0 && txt[new_line_start - 1] == '\0' && txt_len > 0) txt_len--;
	lv_bidi_process_paragraph(txt + line_start, bidi_txt, txt_len, draw_dsc->bidi_dir, NULL, 0);
#else
	bidi_txt = (char *)txt + line_start;
#endif

	/*Calculate the x coordinate*/
	int32_t x = 0;
	if (align == LV_TEXT_ALIGN_CENTER) {
		int32_t line_w;
		line_w = lv_text_get_width(bidi_txt, new_line_start - line_start, draw_dsc->font, draw_dsc->letter_space);
		x += canvas->dsc_w / 2 - line_w / 2;
	} else if(align == LV_TEXT_ALIGN_RIGHT) {
		int32_t line_w;
		line_w = lv_text_get_width(bidi_txt, new_line_start - line_start, draw_dsc->font, draw_dsc->letter_space);
		x += canvas->dsc_w - line_w;
	}

	uint32_t i = 0;
	uint32_t i_act = i;

	if (new_line_start > 0) {
		while(i + line_start < new_line_start) {
			/*Get the current letter and the next letter for kerning*/
			/*Be careful 'i' already points to the next character*/
			uint32_t letter;
			uint32_t letter_next;
			lv_text_encoded_letter_next_2(bidi_txt, &letter, &letter_next, &i);

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
	uint32_t cid = lv_text_encoded_get_char_id(bidi_txt, i);
	if (txt[line_start + i] == '\0') {
		logical_pos = i;
	} else {
		bool is_rtl;
		logical_pos = lv_bidi_get_logical_pos(&txt[line_start], NULL,
											  txt_len, draw_dsc->bidi_dir, cid, &is_rtl);
		if (is_rtl) logical_pos++;
	}
	lv_free(bidi_txt);
#else
	logical_pos = lv_text_encoded_get_char_id(bidi_txt, i);
#endif

	return logical_pos + lv_text_encoded_get_char_id(txt, line_start);
}

static void free_cvt_text(lv_obj_t *obj)
{
	text_canvas_t * canvas = (text_canvas_t *)obj;

	if (canvas->cvt_text) {
		lv_free(canvas->cvt_text);
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
	uint32_t byte_id = lv_text_encoded_get_byte_id(canvas_text, letter_id);
	bool is_cvt_text_nil = (canvas->cvt_text == NULL);

	canvas->cvt_text = lv_realloc(canvas->cvt_text, byte_id + TEXT_CANVAS_DOT_NUM + 1);
	if (canvas->cvt_text) {
		if (is_cvt_text_nil)
			lv_memcpy(canvas->cvt_text, canvas->text, byte_id);

		for (int i = 0; i < TEXT_CANVAS_DOT_NUM; i++)
			canvas->cvt_text[byte_id + i] = '.';

		canvas->cvt_text[byte_id + TEXT_CANVAS_DOT_NUM] = '\0';
	} else {
		LV_LOG_ERROR("malloc failed for long dots");
	}
}

static void refr_text_to_img(lv_obj_t *obj, lv_draw_label_dsc_t *draw_dsc)
{
	text_canvas_t * canvas = (text_canvas_t *)obj;
	lv_image_dsc_t * img_dsc = &canvas->dsc;
	uint32_t buf_size = img_dsc->header.stride * img_dsc->header.h;

#ifdef CONFIG_LVGL_USE_FREETYPE_FONT
	lvgl_freetype_force_bitmap((lv_font_t *)draw_dsc->font, true);
#endif

	lv_memzero((void *)img_dsc->data, buf_size);

	text_canvas_draw_unit_t draw_unit;
	lv_memzero(&draw_unit, sizeof(draw_unit));
	draw_unit.canvas = canvas;

	lv_area_t canvas_area = { 0, canvas->ext_draw_size, img_dsc->header.w - 1, img_dsc->header.h - 1 - canvas->ext_draw_size };
	lv_area_t clip_area = {0, 0, img_dsc->header.w - 1, img_dsc->header.h - 1 };
	draw_unit.base.clip_area = &clip_area;

	lv_draw_label_iterate_characters(&draw_unit.base, draw_dsc, &canvas_area, draw_letter_cb);

	mem_dcache_clean(img_dsc->data, buf_size);

#ifdef CONFIG_LVGL_USE_FREETYPE_FONT
	lvgl_freetype_force_bitmap((lv_font_t *)draw_dsc->font, false);
#endif
}

static lv_color_format_t get_text_font_color_format(lv_obj_t *obj, const lv_font_t *font)
{
	text_canvas_t * canvas = (text_canvas_t *)obj;
	uint8_t bpp = 0;

	if (canvas->force_a8)
		return LV_COLOR_FORMAT_A8;

	const lv_font_fmt_txt_dsc_t * font_dsc = font->dsc;
	if (font_dsc && font_dsc->bpp >= LV_FONT_GLYPH_FORMAT_A1 && font_dsc->bpp <= LV_FONT_GLYPH_FORMAT_A8) {
		bpp = font_dsc->bpp;
	} else {
#ifdef CONFIG_FREETYPE_FONT_BITMAP_BPP
		bpp = CONFIG_FREETYPE_FONT_BITMAP_BPP;
#endif
	}

#ifdef CONFIG_LVGL_USE_FREETYPE_FONT
	lvgl_freetype_force_bitmap((lv_font_t*)font, 0);
#endif

	LV_LOG_INFO("font bpp %d\n", dsc_out.bpp);

	if (bpp == 8) {
		return LV_COLOR_FORMAT_A8;
	} else if (bpp == 4) {
		return LV_COLOR_FORMAT_A4;
	} else if (bpp == 2) {
		return LV_COLOR_FORMAT_A2;
	} else if (bpp == 1) {
		return LV_COLOR_FORMAT_A1;
	} else {
		LV_LOG_WARN("Unrecognized bpp %u", bpp);
		return LV_COLOR_FORMAT_UNKNOWN;
	}
}

static void draw_letter_cb(lv_draw_unit_t * draw_unit, lv_draw_glyph_dsc_t * dsc,
		lv_draw_fill_dsc_t * fill_dsc, const lv_area_t * fill_area)
{
	text_canvas_draw_unit_t *canvas_unit = (text_canvas_draw_unit_t *)draw_unit;
	text_canvas_t * canvas = canvas_unit->canvas;
	lv_image_dsc_t * dst_buf = &canvas->dsc;
	lv_draw_buf_t * map_buf = dsc->glyph_data;
	uint8_t dst_bpp = lv_color_format_get_bpp(canvas->dsc.header.cf);
	uint8_t bpp;

	if (dsc == NULL) return;

	lv_area_t area;
	if (false == lv_area_intersect(&area, draw_unit->clip_area, dsc->letter_coords))
		return;

	if (dsc->bg_coords->y1 != g_line_pos_y) {
		g_line_pos_y = dsc->bg_coords->y1;
		if (g_line_pos_y + lv_font_get_line_height(dsc->g->resolved_font) - 1 <= draw_unit->clip_area->y2)
			g_line_count++;
	}

	switch (dsc->format) {
	case LV_FONT_GLYPH_FORMAT_A1:
	case LV_FONT_GLYPH_FORMAT_A2:
	case LV_FONT_GLYPH_FORMAT_A4:
	case LV_FONT_GLYPH_FORMAT_A8:
		bpp = dsc->format;
		break;

	case LV_FONT_GLYPH_FORMAT_IMAGE:
		if (canvas->emoji_en) {
			lv_point_t pos;
			lv_point_set(&pos, dsc->letter_coords->x1, dsc->letter_coords->y1);
			append_emoji_dsc(canvas, dsc->g, &pos);
		}
		return;

	case LV_FONT_GLYPH_FORMAT_NONE:
	default:
		LV_LOG_ERROR("unknown glyph format %d", dsc->format);
		return;
	}

	if (dst_bpp != 8 && dst_bpp != bpp) {
		LV_LOG_WARN("bpp %u - %u unmatched", bpp, dst_bpp);
		return;
	}

	uint32_t bofs = (area.x1 - dsc->letter_coords->x1) * bpp;
	uint8_t *map_p = map_buf->data + (bofs >> 3) +
				(area.y1 - dsc->letter_coords->y1) * map_buf->header.stride;
	uint8_t col_bit_max = 8 - bpp;
	uint8_t map_bofs_init = col_bit_max - (bofs & 0x7); /* "& 0x7" equals to "% 8" just faster */
	uint8_t bitmask = (1u << bpp) - 1;

	/* Calculate the draw buf*/
	bofs = (area.x1 - draw_unit->clip_area->x1) * dst_bpp;
	uint8_t dst_bofs_init = bofs & 0x7;
	uint8_t *dst_p = (uint8_t *)dst_buf->data + (bofs >> 3) +
				(area.y1 - draw_unit->clip_area->y1) * dst_buf->header.stride;

	int16_t num_cols  = lv_area_get_width(&area);
	int16_t num_rows  = lv_area_get_height(&area);
	int16_t col, row;

	if (dst_bpp == bpp) {
		for (row = num_rows; row > 0; row--) {
			uint8_t *dst_p_tmp = dst_p;
			uint8_t *map_p_tmp = map_p;
			uint16_t dst_bofs = dst_bofs_init;
			uint8_t map_bofs = map_bofs_init;
			uint8_t map_px = *map_p;

			for (col = num_cols; col > 0; col--) {
				/*Load the pixel's opacity into the mask*/
				uint8_t letter_px = (map_px >> map_bofs) & bitmask;
				if (letter_px) {
					dst_p_tmp += (dst_bofs >> 3);
					dst_bofs &= 0x7;

					/* already do memset 0 */
					*dst_p_tmp |= letter_px << (col_bit_max - dst_bofs);
				}

				/*Go to the next column*/
				if (map_bofs > 0) {
					map_bofs -= bpp;
				} else {
					map_bofs = col_bit_max;
					map_px = *(++map_p_tmp);
				}

				dst_bofs += bpp;
			}

			map_p += map_buf->header.stride;
			dst_p += dst_buf->header.stride;
		}
	} else {
		static const lv_opa_t alpha1t8_opa_table[] = { 0, 255 };
		static const lv_opa_t alpha2t8_opa_table[] = { 0, 85, 170, 255 };
		static const lv_opa_t alpha4t8_opa_table[] = {
			0,  17, 34,  51, 68, 85, 102, 119,
			136, 153, 170, 187, 204, 221, 238, 255
        };
		const lv_opa_t *opa_table = (bpp == 1) ? alpha1t8_opa_table :
					(bpp == 2 ? alpha2t8_opa_table : alpha4t8_opa_table);

		for (row = num_rows; row > 0; row--) {
			uint8_t *dst_p_tmp = dst_p;
			uint8_t *map_p_tmp = map_p;
			uint8_t map_bofs = map_bofs_init;
			uint8_t map_px = *map_p;

			for (col = num_cols; col > 0; col--) {
				/*Load the pixel's opacity into the mask*/
				uint8_t letter_px = (map_px >> map_bofs) & bitmask;
				if (letter_px) {
					*dst_p_tmp = opa_table[letter_px];
				}

				/*Go to the next column*/
				if (map_bofs > 0) {
					map_bofs -= bpp;
				} else {
					map_bofs = col_bit_max;
					map_px = *(++map_p_tmp);
				}

				dst_p_tmp++;
			}

			map_p += map_buf->header.stride;
			dst_p += dst_buf->header.stride;
		}
	}
}

static void append_emoji_dsc(text_canvas_t * canvas, const lv_font_glyph_dsc_t *g, const lv_point_t * pos)
{
	lvgl_draw_emoji_dsc_t *emoji_dsc = &canvas->emoji_dsc;

	if (emoji_dsc->count >= emoji_dsc->max_count) {
#if TEXT_CANVAS_EMOJI_MAX_COUNT > 0
		if (emoji_dsc->max_count >= TEXT_CANVAS_EMOJI_MAX_COUNT) {
			LV_LOG_WARN("increase TEXT_CANVAS_EMOJI_MAX_COUNT to support more emoji");
			return;
		}
#endif

		emoji_dsc->max_count += TEXT_CANVAS_EMOJI_INCR_COUNT;
		emoji_dsc->emoji = lv_realloc(emoji_dsc->emoji, emoji_dsc->max_count * sizeof(lvgl_emoji_dsc_t));
		if (emoji_dsc->emoji == NULL) {
			LV_LOG_WARN("emoji dsc alloc failed");
			emoji_dsc->count = 0;
			emoji_dsc->max_count = 0;
			return;
		}
	}

	lv_memcpy(&emoji_dsc->emoji[emoji_dsc->count].g, g, sizeof(*g));
	emoji_dsc->emoji[emoji_dsc->count].pos = *pos;
	emoji_dsc->count++;
}

static void free_emoji_dsc(lvgl_draw_emoji_dsc_t *emoji_dsc)
{
	if (emoji_dsc->emoji) {
		lv_free(emoji_dsc->emoji);
		emoji_dsc->emoji = NULL;
	}

	emoji_dsc->max_count = 0;
	emoji_dsc->count = 0;
}

static void set_ofs_x_anim(void * obj, int32_t v)
{
	text_canvas_t * canvas = (text_canvas_t *)obj;

	//v &= ~canvas->dsc_x_mask;
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
