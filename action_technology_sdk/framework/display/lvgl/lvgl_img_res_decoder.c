/**
 * @file lvgl_img_res_decoder.c
 *
 */

/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*********************
 *      INCLUDES
 *********************/
#include <os_common_api.h>
#include <res_manager_api.h>
#include <lvgl/src/draw/lv_image_decoder.h>
#include <lvgl/src/draw/lv_image_decoder_private.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/


/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_result_t _img_decoder_acts_res_info(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * src, lv_image_header_t * header);
static lv_result_t _img_decoder_acts_res_open(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc);
static void _img_decoder_acts_res_close(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lvgl_img_decoder_acts_res_init(void)
{
	lv_image_decoder_t *res_decoder = lv_image_decoder_create();
	LV_ASSERT_MALLOC(res_decoder);
	if(res_decoder == NULL) {
		LV_LOG_WARN("lvgl_img_decoder_acts_init: out of memory");
		return;
	}

	lv_image_decoder_set_info_cb(res_decoder, _img_decoder_acts_res_info);
	lv_image_decoder_set_open_cb(res_decoder, _img_decoder_acts_res_open);
	lv_image_decoder_set_close_cb(res_decoder, _img_decoder_acts_res_close);
	res_decoder->name = "ACTS_RES";
}

static lv_color_format_t _get_res_img_cf(const style_bitmap_t *sty)
{
	lv_color_format_t cf;

	switch (sty->format) {
	case RESOURCE_BITMAP_FORMAT_ARGB8888:
		cf = LV_COLOR_FORMAT_ARGB8888;
		break;
	case RESOURCE_BITMAP_FORMAT_RGB565:
		cf = LV_COLOR_FORMAT_RGB565;
		break;
	case RESOURCE_BITMAP_FORMAT_ARGB8565:
		cf = LV_COLOR_FORMAT_ARGB8565;
		break;
	case RESOURCE_BITMAP_FORMAT_RGB888:
		cf = LV_COLOR_FORMAT_RGB888;
		break;
	case RESOURCE_BITMAP_FORMAT_JPEG:
		cf = (LV_COLOR_DEPTH <= 16) ? LV_COLOR_FORMAT_RGB565 : LV_COLOR_FORMAT_RGB888;
		break;
	case RESOURCE_BITMAP_FORMAT_LVGL_INDEX8:
		cf = LV_COLOR_FORMAT_I8;
		break;
	case RESOURCE_BITMAP_FORMAT_INDEX4:
		cf = LV_COLOR_FORMAT_I4;
		break;
	case RESOURCE_BITMAP_FORMAT_A8:
		cf = LV_COLOR_FORMAT_A8;
		break;
	case RESOURCE_BITMAP_FORMAT_ETC2_EAC:
		cf = LV_COLOR_FORMAT_ETC2_EAC;
		break;
	default:
		cf = LV_COLOR_FORMAT_UNKNOWN;
		break;
	}

	return cf;
}

static lv_result_t _img_decoder_acts_res_info(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc, lv_image_header_t * header)
{
	LV_UNUSED(decoder);

	if (dsc->src_type == LV_IMAGE_SRC_VARIABLE) {
		const lv_image_dsc_t * image = dsc->src;

		if (image->header.cf != LV_COLOR_FORMAT_RAW && image->header.cf != LV_COLOR_FORMAT_RAW_ALPHA) {
			return LV_RESULT_INVALID;
		}

		if (image->data_size != sizeof(style_bitmap_t)) {
			return LV_RESULT_INVALID;
		}

		const style_bitmap_t * sty = (style_bitmap_t *)image->data;
 		header->magic = LV_IMAGE_HEADER_MAGIC;
 		header->flags = 0;
		header->w = sty->width;
		header->h = sty->height;
		header->cf = _get_res_img_cf(sty);
		header->stride = lv_draw_buf_width_to_stride(header->w, header->cf);
		return LV_RESULT_OK;
	}

	return LV_RESULT_INVALID;
}

static lv_result_t _img_decoder_acts_res_open(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc)
{
	LV_UNUSED(decoder);

	const lv_image_dsc_t * image = dsc->src;
	if (image->data_size != sizeof(style_bitmap_t))
		return LV_RESULT_INVALID;

	style_bitmap_t * sty = (style_bitmap_t *)image->data;
	if (res_manager_load_bitmap_for_decoder(sty) < 0)
		return LV_RESULT_INVALID;

	lv_draw_buf_t * decoded = lv_malloc(sizeof(lv_draw_buf_t));
	if (decoded == NULL)
		return LV_RESULT_INVALID;

	lv_draw_buf_init(decoded, dsc->header.w, dsc->header.h, dsc->header.cf, dsc->header.stride,
                     sty->buffer, dsc->header.stride * dsc->header.h);

	dsc->decoded = decoded;
	return LV_RESULT_OK;
}

static void _img_decoder_acts_res_close(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc)
{
	LV_UNUSED(decoder);

	if (dsc->decoded) {
		res_manager_free_bitmap_for_decoder((void *)dsc->decoded->data);
		lv_free((lv_draw_buf_t *)dsc->decoded);
	}
}
