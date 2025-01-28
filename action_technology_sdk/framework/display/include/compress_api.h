/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRAMEWORK_DISPLAY_INCLUDE_COMPRESS_API_H_
#define FRAMEWORK_DISPLAY_INCLUDE_COMPRESS_API_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TILE_MAX_H CONFIG_TILE_MAX_H
#define TILE_MAX_W CONFIG_TILE_MAX_W
#define TILE_HEAD_LEN 8

/* compressed picture magic codes */
#define PIC_MAGIC_CODE(a, b, c, d) \
	(((uint32_t)(a) << 24) | ((uint32_t)(b) << 16) | ((uint32_t)(c) << 8) | (uint32_t)(d))

#define LZ4_PIC_MAGIC PIC_MAGIC_CODE('L', 'Z', '4', 'C')
#define RLE_PIC_MAGIC PIC_MAGIC_CODE('R', 'L', 'E', 'C')
#define RAW_PIC_MAGIC PIC_MAGIC_CODE('R', 'A', 'W', 'C')
/**
 * @enum compressed_color_format
 * @brief compressed picture color format enumeration.
 */
enum compressed_color_format {
	COMPRESSED_PIC_CF_UNKNOWN = 0,
	COMPRESSED_PIC_CF_RGB_565,
	COMPRESSED_PIC_CF_ARGB_8565,
	COMPRESSED_PIC_CF_ARGB_6666,
	COMPRESSED_PIC_CF_ARGB_8888,
	COMPRESSED_PIC_CF_A8,
	COMPRESSED_PIC_CF_ARGB_1555,
};

/**
 * @enum compressed_pic_format
 * @brief compressed function used picture format enumeration.
 */
enum compressed_pic_forat {
	COMPRESSED_PIC_FORMAT_UNKNOWN = 0,
	COMPRESSED_PIC_FORMAT_RLE,
	COMPRESSED_PIC_FORMAT_LZ4,
	COMPRESSED_PIC_FORMAT_RAW,
};

/**
 * @struct tile_head
 * @brief Structure holding tile header
 */
typedef struct {
	uint32_t tile_addr;
	uint32_t tile_size;
} tile_head_t;

/**
 * @struct lz4_pic_head
 * @brief Structure holding lz4 compressed picture header
 */
typedef struct compress_pic_head {
	uint32_t magic;
	uint16_t width;
	uint16_t height;
	uint16_t tile_width;
	uint16_t tile_height;
	uint8_t format;
	uint8_t bytes_per_pixel;
	uint16_t tile_num;
} compress_pic_head_t;

int pic_compress(const char* picSrc, char* picDst, int srcWidth, int srcHight,
		int tileWidth, int tileHight, int maxOutputSize, uint8_t format, uint8_t compress_format);

int pic_decompress(const char* picSource, char* picDst, int compressedSize,
		int maxDecompressedSize, int out_stride, int x, int y, int w, int h);

int pic_compress_size(const char* picSource);

int pic_compress_format(const char* picSource);

#ifdef __cplusplus
}
#endif

#endif /* FRAMEWORK_DISPLAY_INCLUDE_COMPRESS_API_H_ */
