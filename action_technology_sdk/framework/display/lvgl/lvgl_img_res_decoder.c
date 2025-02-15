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
//#include <lvgl/lvgl_img_decoder.h>
#include <lvgl/src/draw/lv_image_decoder.h>
//#include <lvgl/src/draw/lv_img_cache.h>
#include <lvgl/src/draw/lv_draw_image.h>
//#include <lvgl/porting/decoder/lv_img_decoder_acts.h>
#include <res_manager_api.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/


/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_res_t _img_decoder_acts_res_info(lv_image_decoder_t * decoder, const void * src,
									   lv_image_header_t * header);
static lv_res_t _img_decoder_acts_res_open(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc);
static lv_res_t _img_decoder_acts_res_read_line(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc,
											lv_coord_t x, lv_coord_t y, lv_coord_t len, uint8_t * buf);
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
	lv_image_decoder_set_get_area_cb(res_decoder, _img_decoder_acts_res_get_area);
	lv_image_decoder_set_close_cb(res_decoder, _img_decoder_acts_res_close);

}

void lvgl_img_decoder_acts_res_release_cache(void)
{
	
}

static lv_color_format_t _get_res_img_cf(const resource_bitmap_t *sty)
{
	lv_color_format_t cf; 

	switch (sty->sty_data->format) {
	case RESOURCE_BITMAP_FORMAT_ARGB8888:
		cf = LV_COLOR_FORMAT_ARGB8888;
		break;
	case RESOURCE_BITMAP_FORMAT_RGB565:
		cf = LV_COLOR_FORMAT_RGB565;
		break;
	case RESOURCE_BITMAP_FORMAT_ARGB8565:
		cf = LV_COLOR_FORMAT_ARGB8565;
		break;
	case RESOURCE_BITMAP_FORMAT_A8:
		cf = LV_COLOR_FORMAT_A8;
		break;
	case RESOURCE_BITMAP_FORMAT_LVGL_INDEX8:
		cf = LV_COLOR_FORMAT_I8;
		break;
	case RESOURCE_BITMAP_FORMAT_INDEX4:
		cf = LV_COLOR_FORMAT_I4;
		break;
	case RESOURCE_BITMAP_FORMAT_RAW:
		cf = LV_COLOR_FORMAT_RAW;
		break;
	case RESOURCE_BITMAP_FORMAT_JPEG:
#if LV_COLOR_DEPTH >= 24
		cf = LV_COLOR_FORMAT_RGB888;
#else
		cf = LV_COLOR_FORMAT_RGB565;
#endif	
		break;
	case RESOURCE_BITMAP_FORMAT_ETC2_EAC:
		cf = LV_COLOR_FORMAT_ETC2_EAC;
		break;		
	case RESOURCE_BITMAP_FORMAT_RGB888:
	case RESOURCE_BITMAP_FORMAT_ARGB6666:
	case RESOURCE_BITMAP_FORMAT_ARGB1555:
	case RESOURCE_BITMAP_FORMAT_INDEX8:
	default:
		cf = LV_COLOR_FORMAT_UNKNOWN;
	}

	return cf;
}

static lv_res_t _img_decoder_acts_res_info(lv_image_decoder_t * decoder, const void * src, lv_image_header_t * header)
{
	LV_UNUSED(decoder);
	lv_image_src_t src_type = lv_image_src_get_type(src);

	// SYS_LOG_INF("in _img_decoder_acts_res_info %d\n", src_type);
	if (src_type == LV_IMAGE_SRC_VARIABLE) 
	{
		const lv_image_dsc_t * img_dsc = src;
		if (img_dsc->header.cf != LV_COLOR_FORMAT_RAW && img_dsc->header.cf != LV_COLOR_FORMAT_RAW_ALPHA)
		{
			// SYS_LOG_INF("not raw\n");
			return LV_RESULT_INVALID;
		}
			
		if (img_dsc->data_size != sizeof(resource_bitmap_t))
		{
			SYS_LOG_INF("%d not style bitmap %d\n", img_dsc->data_size, sizeof(resource_bitmap_t));
			return LV_RESULT_INVALID;
		}

		const resource_bitmap_t * bitmap = (resource_bitmap_t *)img_dsc->data;
		header->w  = bitmap->sty_data->width;
		header->h  = bitmap->sty_data->height;
		header->cf = _get_res_img_cf(bitmap);

		return LV_RESULT_OK;
	}

	return LV_RESULT_INVALID;
}


static lv_res_t _img_decoder_acts_res_open(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc)
{
	LV_UNUSED(decoder);
	int ret;
	lv_image_dsc_t* src = (lv_image_dsc_t*)dsc->src;

	if(src->data_size != sizeof(resource_bitmap_t))
	{
		return LV_RESULT_INVALID;
	}

	resource_bitmap_t* sty = (resource_bitmap_t*)src->data;
	ret = res_manager_load_bitmap_for_decoder(sty, 0);
	if(ret < 0)
	{
		if(ret == -2)
		{
			dsc->error_msg = (void*)2;
		}
		return LV_RESULT_INVALID;
	}
	dsc->decoded = sty->buffer;

	return LV_RESULT_OK;
}

static lv_res_t _img_decoder_acts_res_get_area(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc,
											const lv_area_t * full_area, lv_area_t * decoded_area)
{
	return LV_RESULT_OK;
}

static void _img_decoder_acts_res_close(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc)
{
	LV_UNUSED(decoder);
	lv_image_dsc_t* src = (lv_image_dsc_t*)dsc->src;

	if(src->data_size != sizeof(resource_bitmap_t))
	{
		return;
	}


	if(dsc->decoded)
	{
		res_manager_free_bitmap_for_decoder((void*)dsc->decoded);
	}	

	dsc->decoded = NULL;
	dsc->error_msg = NULL;
	dsc->user_data = NULL;
	dsc->time_to_open = 0;	

	return;
}

