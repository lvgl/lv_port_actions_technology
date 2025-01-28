/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file text_canvas.h
 *
 */

#ifndef WIDGETS_TEXT_CANVAS_H_
#define WIDGETS_TEXT_CANVAS_H_

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <lvgl/lvgl.h>
#include "draw_util.h"

/*********************
 *      DEFINES
 *********************/
#define TEXT_CANVAS_WAIT_CHAR_COUNT  3
#define TEXT_CANVAS_DOT_NUM          3
#define TEXT_CANVAS_EMOJI_NUM        0


/**********************
 *      TYPEDEFS
 **********************/

/** Long mode behaviors */
enum {
	TEXT_CANVAS_LONG_WRAP = 0,         /**< Keep the object width, wrap the too long lines and can manually scroll the text out of it*/
	TEXT_CANVAS_LONG_CLIP,             /**< Keep the size and clip the text out of it*/
	TEXT_CANVAS_LONG_SCROLL,           /**< Keep the size and roll the text back and forth*/
	TEXT_CANVAS_LONG_SCROLL_CIRCULAR,  /**< Keep the size and roll the text circularly*/
	TEXT_CANVAS_LONG_DOT,              /**< Keep the size and write dots at the end if the text is too long*/
	TEXT_CANVAS_LONG_LIST,             /**< Shrink to one line and write dots at the end if the text is too long*/

	TEXT_CANVAS_NUM_LONG_MODES,
};
typedef uint8_t text_canvas_long_mode_t;

/** Data of text canvas */
typedef struct {
	lv_obj_t obj; /*Ext. of ancestor*/

	/*New data for this type */
	char * text;
	lv_img_dsc_t dsc;
	lv_coord_t dsc_w;     /* effective width in pixels, may be less than dsc.header.w */
	lv_coord_t dsc_h_max; /* max height of dsc */
	lv_coord_t dsc_w_mask;   /* dsc_w align mask (align to 1 byte) */
	lv_coord_t dsc_x_mask;  /* dsc x position align mask*/
	lv_coord_t ext_draw_size; /* extra draw area on the bottom */

	char * cvt_text;   /*pre-processed text. text replaced with end dots or replaced by ARABIC_PERSIAN_CHARS */
	lv_point_t offset; /*Text draw position offset*/
	lv_point_t offset_circ; /*Text draw position circular offset for TEXT_CANVAS_LONG_SCROLL_CIRCULAR */
	text_canvas_long_mode_t long_mode : 3; /* Determinate what to do with the long texts */
	uint8_t static_txt : 1; /*Flag to indicate the text is static*/
	uint8_t recolor : 1; /*Enable in-line letter re-coloring*/
	uint8_t expand : 1;  /*Ignore real width*/
	uint8_t expand_scroll : 1;  /*Ignore real width and scroll the text out of the object*/

	uint8_t emoji_en : 1;
	lvgl_draw_emoji_dsc_t emoji_dsc;
} text_canvas_t;

extern const lv_obj_class_t text_canvas_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create an text canvas object
 * @param parent pointer to an object, it will be the parent of the new object
 * @return pointer to the created text canvas object
 */
lv_obj_t * text_canvas_create(lv_obj_t * parent);

/**
 * Clean the resource
 * @param obj pointer to a text canvas object
 */
void text_canvas_clean(lv_obj_t * obj);

/*=====================
 * Setter functions
 *====================*/

/**
 * Set the maximum height of the whole text.
 *
 * It will applied on next text_canvas_set_text[_static](). Set 0, it means the
 * maximum possible height of the whole text.
 *
 * @param obj pointer to a text canvas object
 * @param h the maximum height, it take effect on next set_text()
 */
void text_canvas_set_max_height(lv_obj_t * obj, uint16_t h);

/**
 * enable/disable text emoji support
 *
 * It will enable/disable emoji support on text canvas, only emoji enabled
 * text canvas can be used for text display with emoji chars
 *
 * @param obj pointer to a text canvas object
 * @param en enable/disable flag of emoji support.
 */
void text_canvas_set_emoji_enable(lv_obj_t * obj, bool en);

/**
 * Set a new text.
 *
 *  Memory will be allocated to store the text by the object.
 *
 * @param obj         pointer to a text canvas object
 * @param text        '\0' terminated character string. NULL to refresh with the current text.
 */
void text_canvas_set_text(lv_obj_t * obj, const char * text);

/**
 * Set a static text.
 *
 * It will not be saved by the label so the 'text' variable
 * has to be 'alive' while the label exists.
 *
 * @param obj         pointer to a text canvas object
 * @param text        pointer to a text. NULL to refresh with the current text.
 */
void text_canvas_set_text_static(lv_obj_t * obj, const char * text);

/**
 * Set the behavior with longer text than the object size
 * @param obj pointer to a text canvas object
 * @param long_mode the new mode from 'text_canvas_long_mode' enum.
 */
void text_canvas_set_long_mode(lv_obj_t * obj, text_canvas_long_mode_t long_mode);

/**
 * Enable the recoloring by in-line commands
 * @param obj pointer to a text canvas object
 * @param en  true: enable recoloring, false: disable
 * @example "This is a #ff0000 red# word"
 */
void text_canvas_set_recolor(lv_obj_t * obj, bool en);

/*=====================
 * Getter functions
 *====================*/

/**
 * Get the image of the text canvas as a pointer to an `lv_img_dsc_t` variable.
 * @param canvas pointer to a canvas object
 * @return pointer to the image descriptor.
 */
lv_img_dsc_t * text_canvas_get_img(lv_obj_t * obj);

/*=====================
 * Other functions
 *====================*/

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* WIDGETS_TEXT_CANVAS_H_ */
