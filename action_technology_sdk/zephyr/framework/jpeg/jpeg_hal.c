/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <string.h>
#include <assert.h>
#include <os_common_api.h>
#include <jpeg_hal.h>
#include <mem_manager.h>
#include <logging/log.h>
#ifdef CONFIG_DISPLAY
#include <ui_region.h>
#endif
#include <memory/mem_cache.h>
#include <spicache.h>

#define JPEG_MIN_SIZE 16
#define JPEG_HW_TIMEOUT   (50) //ms
/****************************************************************************
 * Private Data
 ****************************************************************************/
static OS_MUTEX_DEFINE(g_parser_info_mutex);
static OS_MUTEX_DEFINE(g_decode_mutex);

static jpeg_parser_info_t g_parser_info;

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static void _hal_jpeg_device_handler(int err_code, void *user_data)
{
	LOG_INF("_hal_jpeg_device_handler err_code (%d)\n",err_code);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int hal_jpeg_decode_open(hal_jpeg_handle_t *hjpeg)
{
	return jpeg_open(hjpeg->device);
}

static int _hal_jpeg_decode_sub_image(hal_jpeg_handle_t *hjpeg,
					void *jpeg_src, int jpeg_size,
					void *bmp_buffer, int output_format, int output_stride,
					int win_x, int win_y, int win_w, int win_h)
{
	int ret = 0;
	struct jpeg_info_t *jpeg_info = NULL;

	os_mutex_lock(&g_parser_info_mutex, OS_FOREVER);

	jpeg_parser_info_t *parser_info = &g_parser_info;

	memset(parser_info, 0 , sizeof(jpeg_parser_info_t));
	parser_info->jpeg_base = jpeg_src;
	parser_info->jpeg_size = jpeg_size;

	ret = jpeg_parser_process(parser_info, 1);
	if (ret) {
		LOG_ERR("jpeg_parser_process failed (%d)\n",ret);
		ret = -HAL_JPEG_PARSER_ERROR;
		goto err_exit;
	}

	jpeg_info = &parser_info->jpeg_info;
	jpeg_info->window_x = win_x;
	jpeg_info->window_y = win_y;
	jpeg_info->window_w = win_w;
	jpeg_info->window_h = win_h;
	jpeg_info->output_stride = output_stride;
	jpeg_info->output_bmp = bmp_buffer;
	jpeg_info->output_format = output_format;
	jpeg_info->stream_offset = 0;
	jpeg_info->stream_size = (jpeg_info->stream_size + 511) / 512 * 512;

	ret = jpeg_config(hjpeg->device, jpeg_info);
	if (ret) {
		LOG_ERR("jpeg_config failed (%d)\n",ret);
		ret = -HAL_JPEG_CONFIG_DECODER_ERROR;
		goto err_exit;
	}

	ret = jpeg_decode(hjpeg->device);
	if (ret) {
		LOG_ERR("jpeg_decode failed (%d)\n",ret);
		ret = -HAL_JPEG_DECODER_ERROR;
		goto err_exit;
	}

	ret = HAL_JPEG_ERROR_NONE;

err_exit:
	os_mutex_unlock(&g_parser_info_mutex);
	return ret;
}

int hal_jpeg_decode(hal_jpeg_handle_t *hjpeg, void *jpeg_src, int jpeg_size,
					void *bmp_buffer, int output_format, int output_stride,
					int win_x, int win_y, int win_w, int win_h)
{
	int ret = 0;
	struct jpeg_head_info *head_info = (struct jpeg_head_info *)jpeg_src;

	os_strace_u32x4(SYS_TRACE_ID_IMG_DECODE, 0, (uint32_t)jpeg_src, win_w, win_h);

#ifndef CONFIG_SOC_NO_PSRAM
	if (buf_is_psram(jpeg_src)) {
		mem_dcache_flush(jpeg_src, jpeg_size);
		mem_dcache_sync();
	}

	if (buf_is_psram(bmp_buffer)) {
		if(output_format == HAL_JPEG_OUT_RGB888) {
			mem_dcache_flush(bmp_buffer, win_h * output_stride * 3);
		} else {
			mem_dcache_flush(bmp_buffer, win_h * output_stride * 2);
		}
		mem_dcache_sync();
	}
#endif

	if (head_info->flag != JPEG_FLAG) {
		ret = _hal_jpeg_decode_sub_image(hjpeg, jpeg_src, jpeg_size,
				bmp_buffer, output_format, output_stride,
				win_x, win_y, win_w, win_h);
		return ret;
	}

	if (head_info->split_num <= 1) {
		ret = _hal_jpeg_decode_sub_image(hjpeg, (uint8_t *)jpeg_src + sizeof(*head_info),
				jpeg_size - sizeof(*head_info), bmp_buffer, output_format, output_stride,
				win_x, win_y, win_w, win_h);
		return ret;
	}

#ifdef CONFIG_DISPLAY
	uint8_t h_block_num = (head_info->pic_height + head_info->split_height - 1)
								 / head_info->split_height ;
    uint8_t w_block_num = (head_info->pic_width + head_info->split_width - 1)
                                 / head_info->split_width;
	uint8_t bytes_per_pixel = output_format ? 2 : 3;
	jpeg_sub_info_t sub_info[MAX_BLOCK_NUM];

    memcpy(sub_info, (void*)&head_info->offset, h_block_num * w_block_num * 4);

    for (int j = 0; j < h_block_num; j++) {
        for (int i = 0; i < w_block_num; i++) {
            int sub_jpeg_size = sub_info[j * w_block_num + i].size;
            int dec_w, dec_h;
            ui_region_t pic_region = {
                .x1 = i * head_info->split_width,
                .y1 = j * head_info->split_height,
                .x2 = (i + 1) * head_info->split_width - 1,
                .y2 = (j + 1) * head_info->split_height - 1,
            };

            if (pic_region.y2 > head_info->pic_height - 1) {
                pic_region.y2 = head_info->pic_height - 1;
            }

            if (pic_region.x2 > head_info->pic_width - 1) {
                pic_region.x2 = head_info->pic_width - 1;
            }

            ui_region_t output_region = {
                .x1 = win_x,
                .y1 = win_y,
                .x2 = win_x + win_w - 1,
                .y2 = win_y + win_h - 1,
            };

            ui_region_t dec_region;
            if (ui_region_intersect(&dec_region, &output_region, &pic_region) == false) {
                continue;
            }

            void* sub_jpeg_src = (void*)((uint32_t)jpeg_src + sub_info[j * w_block_num + i].offset);
            void* sub_out_dest = (void*)((uint32_t)bmp_buffer
                + (dec_region.x1 - win_x) * bytes_per_pixel
                + (dec_region.y1 - win_y) * output_stride * bytes_per_pixel);

			dec_w =  dec_region.x2 - dec_region.x1 + 1;
			dec_h =  dec_region.y2 - dec_region.y1 + 1;
			ret = _hal_jpeg_decode_sub_image(hjpeg, sub_jpeg_src, sub_jpeg_size, sub_out_dest,
						 output_format, output_stride,
						 dec_region.x1 - pic_region.x1 , dec_region.y1 - pic_region.y1,
						 dec_w,
						 dec_h);

			if (ret != 0) {
				LOG_ERR("jpeg_decode failed (%d)\n",ret);
				ret = -HAL_JPEG_DECODER_ERROR;
				goto err_exit;
			}

			if (dec_w * dec_h < JPEG_MIN_SIZE * JPEG_MIN_SIZE) {
				jpeg_decode_wait_finished(hjpeg->device, 1);
			} else {
				jpeg_decode_wait_finished(hjpeg->device, JPEG_HW_TIMEOUT);
			}
		}
	}

err_exit:
	os_strace_end_call(SYS_TRACE_ID_IMG_DECODE);
	return ret;
#else
	os_strace_end_call(SYS_TRACE_ID_IMG_DECODE);
	return 0;
#endif
}

int hal_jpeg_get_picture_info(void *jpeg_src , uint32_t jpeg_size,
								uint32_t *picture_w, uint32_t *picture_h)
{
	struct jpeg_head_info *head_info = (struct jpeg_head_info *)jpeg_src;
	jpeg_parser_info_t *parser_info = &g_parser_info;

	if (head_info->flag == JPEG_FLAG) {
		*picture_w = head_info->pic_width;
		*picture_h = head_info->pic_height;
		return 0;
	}

	os_mutex_lock(&g_parser_info_mutex, OS_FOREVER);

	memset(parser_info, 0, sizeof(jpeg_parser_info_t));
	parser_info->jpeg_base = jpeg_src;
	parser_info->jpeg_size = jpeg_size;

	if (jpeg_parser_process(parser_info, 1)) {
		os_mutex_unlock(&g_parser_info_mutex);
		return HAL_JPEG_PARSER_ERROR;
	}

	*picture_w = parser_info->jpeg_info.image_w;
	*picture_h = parser_info->jpeg_info.image_h;

	os_mutex_unlock(&g_parser_info_mutex);
	return 0;
}

int hal_jpeg_decode_wait_finised(hal_jpeg_handle_t *hjpeg, int timeout)
{
	return jpeg_decode_wait_finished(hjpeg->device, timeout);
}

int hal_jpeg_decode_close(hal_jpeg_handle_t *hjpeg)
{
	return jpeg_close(hjpeg->device);
}

int hal_jpeg_init(hal_jpeg_handle_t *hjpeg, uint32_t preferred_modes)
{
	/* Check the jpeg peripheral existence */
	hjpeg->device = device_get_binding(CONFIG_JPEG_HW_DEV_NAME);
	if (hjpeg->device == NULL) {
		return -ENODEV;
	}

	/* Register the jpeg device instance callback */
	jpeg_register_callback(hjpeg->device, _hal_jpeg_device_handler, hjpeg);
	return 0;
}

int hal_jpeg_deinit(hal_jpeg_handle_t *hjpeg)
{
	/* Assign invald value */
	hjpeg->device = NULL;
	return 0;
}

int jpg_decode(void* jpeg_src, int jpeg_size,
    void* bmp_buffer, int output_format, int output_stride,
    int win_x, int win_y, int win_w, int win_h)
{
    static hal_jpeg_handle_t jpg_decoder;
    static bool jpg_inited = false;
    int res;
	int bytes_per_pixel = output_format ? 2 : 3;

	res = os_mutex_lock(&g_decode_mutex, OS_FOREVER);
	if (res) {
		return res;
	}

    if (jpg_inited == false) {
        res = hal_jpeg_init(&jpg_decoder, 0);
        if (res)
            return res;

        jpg_inited = true;
    }

    res = hal_jpeg_decode_open(&jpg_decoder);
    if (res) {
		LOG_ERR("hal_jpeg_decode_open fail\n");
		os_mutex_unlock(&g_decode_mutex);
        return res;
    }

    res = hal_jpeg_decode(&jpg_decoder, (void*)jpeg_src, (int)jpeg_size,
        bmp_buffer, output_format, output_stride,
        win_x, win_y, win_w, win_h);

	hal_jpeg_decode_wait_finised(&jpg_decoder, JPEG_HW_TIMEOUT);

    hal_jpeg_decode_close(&jpg_decoder);
	os_mutex_unlock(&g_decode_mutex);
    return res ? res : (win_w * win_h * bytes_per_pixel);
}

