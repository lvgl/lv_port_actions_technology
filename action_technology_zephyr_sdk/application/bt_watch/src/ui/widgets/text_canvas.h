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
#define TEXT_CANVAS_EMOJI_MAX_COUNT  0
#define TEXT_CANVAS_EMOJI_INCR_COUNT 8

/**********************
 *      TYPEDEFS
 **********************/

/** Long mode behaviors */
typedef enum {
	TEXT_CANVAS_LONG_WRAP = LV_LABEL_LONG_WRAP,      /**< Keep the object width, wrap the too long lines and can manually scroll the text out of it*/
	TEXT_CANVAS_LONG_DOT = LV_LABEL_LONG_DOT,        /**< Keep the size and write dots at the end if the text is too long*/
	TEXT_CANVAS_LONG_SCROLL = LV_LABEL_LONG_SCROLL,  /**< Keep the size and roll the text back and forth*/
	TEXT_CANVAS_LONG_SCROLL_CIRCULAR = LV_LABEL_LONG_SCROLL_CIRCULAR,  /**< Keep the size and roll the text circularly*/
	TEXT_CANVAS_LONG_CLIP = LV_LABEL_LONG_CLIP, /**< Keep the size and clip the text out of it*/
	TEXT_CANVAS_LONG_LIST,                      /**< Shrink to one line and write dots at the end if the text is too long*/

	TEXT_CANVAS_NUM_LONG_MODES,
} text_canvas_long_mode_t;

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
static inline void text_canvas_set_max_height(lv_obj_t * obj, uint16_t h)
{
	lv_obj_set_style_max_height(obj, h, LV_PART_MAIN);
}

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
 * Set a new formatted text. Memory will be allocated to store the text by the label.
 * @param obj         pointer to a text canvas object
 * @param fmt           `printf`-like format
 * @example lv_label_set_text_fmt(label1, "%d user", user_num);
 */
void text_canvas_set_text_fmt(lv_obj_t * obj, const char * fmt, ...) LV_FORMAT_ATTRIBUTE(2, 3);

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
 * Set canvas image format to LV_IMG_CF_ALPHA_8BIT.
 *
 * @param obj         pointer to a text canvas object
 * @param en          set format to LV_IMG_CF_ALPHA_8BIT or not.
 */
void text_canvas_set_img_8bit(lv_obj_t * obj, bool en);

/*=====================
 * Getter functions
 *====================*/

/**
 * Get the image of the text canvas as a pointer to an `lv_image_dsc_t` variable.
 * @param canvas pointer to a canvas object
 * @return pointer to the image descriptor.
 */
lv_image_dsc_t * text_canvas_get_img(lv_obj_t * obj);

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
