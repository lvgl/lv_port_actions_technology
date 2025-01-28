/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*********************
 *      INCLUDES
 *********************/
#include "draw_util.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *  GLOBAL VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
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
int split_decimal(uint32_t value, uint8_t *buf, int len, int min_w)
{
	uint32_t tmp_value = value;
	int n_digits = 0;

	/* compute digit count of value */
	do {
		n_digits++;
		tmp_value /= 10;
	} while (tmp_value > 0);

	/* return digit count if buf not available */
	if (buf == NULL || len <= 0) {
		return n_digits;
	}

	/* compute filled digit count */
	if (n_digits >= len) {
		n_digits = len;
	}

	if (min_w >= len) {
		min_w = len;
	} else {
		len = MAX(n_digits, min_w);
	}

	/* fill digits of value */
	buf += len - 1;
	min_w -= n_digits;

	for (; n_digits > 0; n_digits--) {
		*buf-- = value % 10;
		value /= 10;
	}

	/* pad zero */
	for (; min_w > 0; min_w--) {
		*buf-- = 0;
	}

	return len;
}

bool lvgl_txt_get_size(lv_point_t * size_res, const char * text, const lv_font_t * font, lv_coord_t letter_space,
					 lv_coord_t line_space, lv_coord_t max_width, lv_coord_t max_height, lv_text_flag_t flag)
{
	size_res->x = 0;
	size_res->y = 0;

	if(text == NULL) return false;
	if(font == NULL) return false;

	if(flag & LV_TEXT_FLAG_EXPAND) max_width = LV_COORD_MAX;

	uint32_t line_start     = 0;
	uint32_t new_line_start = 0;
	uint16_t letter_height = lv_font_get_line_height(font);

	/*Calc. the height and longest line*/
	while(text[line_start] != '\0') {
		new_line_start += lv_text_get_next_line(&text[line_start], font, letter_space, max_width, NULL, flag);

		if((unsigned long)size_res->y + (unsigned long)letter_height + (unsigned long)line_space > LV_MAX_OF(lv_coord_t)) {
			LV_LOG_WARN("integer overflow while calculating text height");
			return true;
		}
		else {
			size_res->y += letter_height;
			size_res->y += line_space;
		}

		/*Calculate the longest line*/
		lv_coord_t act_line_length = lv_text_get_width(&text[line_start], new_line_start - line_start, font, letter_space);

		size_res->x = LV_MAX(act_line_length, size_res->x);
		line_start  = new_line_start;

		/* FIXME: should we use "size_res->y >= max_height" to keep compatibility, or just keep complete lines ? */
		if(size_res->y + letter_height > max_height) {
			break;
		}
	}

	/*Make the text one line taller if the last character is '\n' or '\r'*/
	if((line_start != 0) && (text[line_start - 1] == '\n' || text[line_start - 1] == '\r')) {
		size_res->y += letter_height + line_space;
	}

	/*Correction with the last line space or set the height manually if the text is empty*/
	if(size_res->y == 0) {
		size_res->y = letter_height;
	}
	else {
		size_res->y -= line_space;
	}

	return (size_res->y > max_height || text[line_start] != '\0') ? true : false;
}

bool lvgl_txt_has_encoded_length(const char * text, uint32_t len)
{
	uint32_t i  = 0;

	if(len == 0) return true;

	while(text[i] != '\0') {
		lv_text_encoded_next(text, &i);
		if (--len == 0) return true;
	}

	return false;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
