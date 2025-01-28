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
#include <ui_region.h>
#include <lv_sjpg.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

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
    return 0;
}

int hal_jpeg_decode(hal_jpeg_handle_t *hjpeg, void *jpeg_src, int jpeg_size,
					void *bmp_buffer, int output_format, int output_stride,
					int win_x, int win_y, int win_w, int win_h)
{
	int ret = 0;

	struct jpeg_head_info *head_info = (struct jpeg_head_info *)jpeg_src;

	uint8_t h_block_num = (head_info->pic_height + head_info->split_height - 1)
								 / head_info->split_height ;
    uint8_t w_block_num = (head_info->pic_width + head_info->split_width - 1)
                                 / head_info->split_width;

    jpeg_sub_info_t sub_info[MAX_BLOCK_NUM];

	if (head_info->flag != JPEG_FLAG) {
		ret = decoder_jpeg(jpeg_src, jpeg_size,
				bmp_buffer, output_stride, win_x, win_y, win_w, win_h);
		return ret;
	}

	if (head_info->split_num <= 1) {
		ret = decoder_jpeg((uint8_t *)jpeg_src + sizeof(*head_info),
				jpeg_size - sizeof(*head_info), bmp_buffer, output_stride,
				win_x, win_y, win_w, win_h);
		return ret;
	}

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
                + (dec_region.x1 - win_x) * 2
                + (dec_region.y1 - win_y) * output_stride * 2);

            dec_w = dec_region.x2 - dec_region.x1 + 1;
            dec_h = dec_region.y2 - dec_region.y1 + 1;

            ret = decoder_jpeg(sub_jpeg_src, sub_jpeg_size, sub_out_dest, output_stride, dec_region.x1 - pic_region.x1,
                dec_region.y1 - pic_region.y1, dec_w, dec_h);
        }
    }

	return ret;

#ifdef CONFIG_DISPLAY
err_exit:
	return ret;
#endif
}

int hal_jpeg_get_picture_info(void *jpeg_src , uint32_t jpeg_size,
								uint32_t *picture_w, uint32_t *picture_h)
{
	struct jpeg_head_info *head_info = (struct jpeg_head_info *)jpeg_src;

	*picture_w = head_info->pic_width;
	*picture_h = head_info->pic_height;
	return 0;
}

int hal_jpeg_decode_wait_finised(hal_jpeg_handle_t *hjpeg, int timeout)
{
    return 0;
}

int hal_jpeg_decode_close(hal_jpeg_handle_t *hjpeg)
{
	return 0;
}

int hal_jpeg_init(hal_jpeg_handle_t *hjpeg, uint32_t preferred_modes)
{
	/* Check the jpeg peripheral existence */
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

    if (jpg_inited == false) {
        res = hal_jpeg_init(&jpg_decoder, 0);
        if (res)
            return res;

        jpg_inited = true;
    }

    res = hal_jpeg_decode_open(&jpg_decoder);
    if (res)
        return res;

    res = hal_jpeg_decode(&jpg_decoder, (void*)jpeg_src, (int)jpeg_size,
        bmp_buffer, output_format, output_stride,
        win_x, win_y, win_w, win_h);

    hal_jpeg_decode_close(&jpg_decoder);

    return res ? res : (win_w * win_h * 2);
}


