/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WIDGETS_DRAW_UTIL_H_
#define WIDGETS_DRAW_UTIL_H_

/*********************
 *      INCLUDES
 *********************/
#include <sys/util.h>
#include <display/sw_draw.h>
#include <display/sw_math.h>
#include <lvgl/lvgl.h>
#include <lvgl/src/lvgl_private.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/*
 * split a decimal integer into digits
 *
 * @param value decimal integer value
 * @param buf buffer to store the digits
 * @param len buffer len
 * @param min_w minimum split digits (may pad 0 if integer too small)
 *
 * @retval number of split digits
 */
int split_decimal(uint32_t value, uint8_t *buf, int len, int min_w);

/**
 * Get size of a text
 * @param size_res pointer to a 'point_t' variable to store the result
 * @param text pointer to a text
 * @param font pointer to font of the text
 * @param letter_space letter space of the text
 * @param line_space line space of the text
 * @param flags settings for the text from ::lv_text_flag_t
 * @param max_width max width of the text (break the lines to fit this size). Set COORD_MAX to avoid
 * @param max_height max height of the text (break the lines to fit this size). Set COORD_MAX to avoid
 * line breaks
 *
 * @retval true if exceed max_height, otherwise false
 */
bool lvgl_txt_get_size(lv_point_t * size_res, const char * text, const lv_font_t * font, lv_coord_t letter_space,
                     lv_coord_t line_space, lv_coord_t max_width, lv_coord_t max_height, lv_text_flag_t flag);

/**
 * Test if has enough encoded length
 * @param text pointer to a text
 * @param len encoded legthe to compared
 *
 * @retval true if has enough encoded length, otherwise false
 */
bool lvgl_txt_has_encoded_length(const char * text, uint32_t len);

#endif /* WIDGETS_DRAW_UTIL_H_ */
