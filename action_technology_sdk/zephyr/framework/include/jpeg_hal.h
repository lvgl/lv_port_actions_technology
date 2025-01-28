/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief JPEG_HW Public API
 */

#ifndef ZEPHYR_FRAMEWORK_INCLUDE_JPEG_HAL_H_
#define ZEPHYR_FRAMEWORK_INCLUDE_JPEG_HAL_H_

#include <string.h>
#include <jpeg_parser.h>
#include <video/jpeg_hw/jpeg_hw.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_BLOCK_NUM   12

#define JPEG_MAGIC_CODE(a, b, c, d) \
	(((uint32_t)(d) << 24) | ((uint32_t)(c) << 16) | ((uint32_t)(b) << 8) | (uint32_t)(a))

#define JPEG_FLAG JPEG_MAGIC_CODE('J', 'P', 'E', 'G')

typedef struct jpeg_sub_info
{
    uint16_t     offset;
    uint16_t     size;
}jpeg_sub_info_t;

typedef struct jpeg_head_info
{
#ifdef CONFIG_LVGL_USE_IMG_DECODER_ACTS
	uint16_t     tile_width;
	uint16_t     tile_height;
#endif
	uint32_t     flag;
	uint16_t     pic_width;
	uint16_t     pic_height;
    uint16_t     split_width;
	uint16_t     split_height;
    uint16_t     split_num;
	uint16_t     offset[1];
	uint16_t     size[1];
}jpeg_head_info_t;

/* JPEG decode state */
typedef enum { 
	HAL_JPEG_ERROR_NONE = 0,
	HAL_JPEG_PARSER_ERROR = 1,
	HAL_JPEG_CONFIG_DECODER_ERROR = 2,
	HAL_JPEG_DECODER_ERROR = 3,
} jpeg_decode_state_e;

/* JPEG decode output format */
typedef enum { 
	HAL_JPEG_OUT_RGB888 = 0,
	HAL_JPEG_OUT_RGB565 = 1,
} jpeg_decode_output_format_e;

/**
 * @struct hal_jpeg_handle_t
 * @brief Structure holding jpeg handle
 *
 */
typedef struct _hal_jpeg_handle {
	const void *device;            /*!< jpeg Device Handle */
} hal_jpeg_handle_t;


int hal_jpeg_decode_open(hal_jpeg_handle_t *hjpeg);

/**
 * @brief  decode jpeg pitcutre, 
 *
 * @param hjpeg pointer to a hal_jpeg_handle_t structure that contains
 *                 the configuration information for the jpeg.
 *
 * @param  jpeg_src source of jpeg picture addr
 *
 * @param  jpeg_src size of jpeg picture
 *
 * @param  bmp_buffer pointer to out put bmp buffer
 *
 * @param  output_format output format , only support RGB565/RGB888
 *
 * @param  output_stride output stride of output buffer
 *
 * @param  win_x x of output widnow
 *
 * @param  win_y y of output widnow
 *
 * @param  win_w w of output widnow
 *
 * @param  win_h h of output widnow
 *
 * @return the name string
 */
int hal_jpeg_decode(hal_jpeg_handle_t *hjpeg, void *jpeg_src, int jpeg_size,
					void *bmp_buffer, int output_format, int output_stride,
					int win_x, int win_y, int win_w, int win_h);

/**
 * @brief get jpeg picture width and height inforamtion , not realy decode ,only parser jpeg
 *
 * @param  jpeg_src pointer to picture of jpeg picture
 *
 * @param  jpeg_size size of picture 
 *
 * @param  picture_w return the picture width
 *
 * @param  picture_h return the picture height
 *
 * @retval 0 on success else negative errno code.
 */
int hal_jpeg_get_picture_info(void *jpeg_src ,uint32_t jpeg_size,
								uint32_t *picture_w, uint32_t *picture_h);

/**
 * @brief close the jpeg decoder
 *
 * @param  hjpeg pointer to a hal_jpeg_handle_t structure that contains
 *                 the configuration information for the jpeg.
 *
 * @retval 0 on success else negative errno code.
 */
int hal_jpeg_decode_close(hal_jpeg_handle_t *hjpeg);

/**
 * @brief  Initialize the jpeg peripheral and create the associated handle.
 *
 * @param  hjpeg pointer to a hal_jpeg_handle_t structure that contains
 *                 the configuration information for the jpeg.
 *
 * @param preferred_modes "bitwise or" of output modes that maybe used.
 *
 * @retval 0 on success else negative errno code.
 */

int hal_jpeg_init(hal_jpeg_handle_t *hjpeg, uint32_t preferred_modes);

/**
 * @brief deinitialize jpeg decorder
 *
 * @param jpeg handle of jpeg decoder
 *
 * @retval 0 on success else negative errno code.
 */

int hal_jpeg_deinit(hal_jpeg_handle_t *hjpeg);

/**
 * @brief wait jpeg decode finished
 *
 * @param hjpeg handle of jpeg decoder
 *
 * @retval 0 on success else negative errno code.
 */

int hal_jpeg_decode_wait_finised(hal_jpeg_handle_t *hjpeg, int timeout);


int jpg_decode(void* jpeg_src, int jpeg_size,
    void* bmp_buffer, int output_format, int output_stride,
    int win_x, int win_y, int win_w, int win_h);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_FRAMEWORK_INCLUDE_JPEG_HAL_H_ */
