/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file display_hal.h
 */

#ifndef ZEPHYR_FRAMEWORK_INCLUDE_DISPLAY_DISPLAY_HAL_H_
#define ZEPHYR_FRAMEWORK_INCLUDE_DISPLAY_DISPLAY_HAL_H_

#include <stdint.h>
#include <drivers/display/display_graphics.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief HAL pixel format definitions
 *
 * HAL pixel format enumeration.
 *
 */
enum hal_pixel_format {
	/*
	 * "linear" color pixel formats:
	 */
	/* order: [31..24] A, [23..16] R, [15..8] G, [7..0] B */
	HAL_PIXEL_FORMAT_ARGB_8888          = PIXEL_FORMAT_ARGB_8888,

	/* order: [31..24] X, [23..16] R, [15..8] G, [7..0] B */
	HAL_PIXEL_FORMAT_XRGB_8888          = PIXEL_FORMAT_XRGB_8888,

	/* order: [23..18] A, [17..12] R, [11..6] G, [5..0] B */
	HAL_PIXEL_FORMAT_ARGB_6666          = PIXEL_FORMAT_BGRA_6666,

	/* order: [23..18] A, [17..12] B, [11..6] G, [5..0] R */
	HAL_PIXEL_FORMAT_ABGR_6666          = PIXEL_FORMAT_RGBA_6666,

	/* order: [23..16] A, [15..11] R, [10..5] G, [4..0] B */
	HAL_PIXEL_FORMAT_ARGB_8565          = PIXEL_FORMAT_BGRA_5658,

	/* order: [23..16] R, [15..8] G, [7..0] B */
	//HAL_PIXEL_FORMAT_BGR_888            = PIXEL_FORMAT_BGR_888,

	/* order: [23..16] B, [15..8] G, [7..0] R */
	HAL_PIXEL_FORMAT_RGB_888            = PIXEL_FORMAT_RGB_888,

	/* order: [15..11] R, [10..5] G, [4..0] B */
	HAL_PIXEL_FORMAT_RGB_565            = PIXEL_FORMAT_BGR_565,

	/* order: [15..13] G[2..0], [12..8] B, [7..3] R, [2..0] G[5..3] */
	HAL_PIXEL_FORMAT_RGB_565_BE         = PIXEL_FORMAT_RGB_565,

	/* order: [15..15] A, [14..10] R, [9..5] G, [4..0] B */
	HAL_PIXEL_FORMAT_ARGB_1555          = PIXEL_FORMAT_BGRA_5551,

	/* order: [7..0] I */
	HAL_PIXEL_FORMAT_I8                 = PIXEL_FORMAT_I8,

	/* order: [7..4] I0, [3..0] I1 */
	HAL_PIXEL_FORMAT_I4                 = PIXEL_FORMAT_I4,

	/* order: [7..6] I0, [5..4] I1, [3..2] I2, [1..0] I3 */
	HAL_PIXEL_FORMAT_I2                 = PIXEL_FORMAT_I2,

	/* order: [7] I0, [6] I1, [5] I2, [4] I3, [3] I4, [2] I5, [1] I6, [0] I7 */
	HAL_PIXEL_FORMAT_I1                 = PIXEL_FORMAT_I1,

	/* order: [7..0] A */
	HAL_PIXEL_FORMAT_A8                 = PIXEL_FORMAT_A8,

	/* order: [7..4] A0, [3..0] A1 */
	HAL_PIXEL_FORMAT_A4                 = PIXEL_FORMAT_A4,

	/* order: [7..6] A0, [5..4] A1, [3..2] A2, [1..0] A3 */
	HAL_PIXEL_FORMAT_A2                 = PIXEL_FORMAT_A2,

	/* order: [7] A0, [6] A1, [5] A2, [4] A3, [3] A4, [2] A5, [1] A6, [0] A7 */
	HAL_PIXEL_FORMAT_A1                 = PIXEL_FORMAT_A1,

	/* order: [7..4] A1, [3..0] A0 */
	HAL_PIXEL_FORMAT_A4_LE              = PIXEL_FORMAT_A4_LE,

	/* order: [7..6] A3, [5..4] A2, [3..2] A1, [1..0] A0 */
	//HAL_PIXEL_FORMAT_A2_LE              = PIXEL_FORMAT_A2_LE,

	/* order: [7] A7, [6] A6, [5] A5, [4] A4, [3] A3, [2] A2, [1] A1, [0] A0 */
	HAL_PIXEL_FORMAT_A1_LE              = PIXEL_FORMAT_A1_LE,
};

/**
 * @brief HAL blending type definitions
 *
 * HAL blending type enumeration.
 *
 */
enum hal_blending_type {
	/* no blending */
	HAL_BLENDING_NONE = DISPLAY_BLENDING_NONE,

	/* ONE / ONE_MINUS_SRC_ALPHA */
	HAL_BLENDING_PREMULT = DISPLAY_BLENDING_PREMULT,

	/* SRC_ALPHA / ONE_MINUS_SRC_ALPHA */
	HAL_BLENDING_COVERAGE = DISPLAY_BLENDING_COVERAGE,
};

/**
 * @brief HAL transform type definitions
 *
 * HAL transform type enumeration.
 *
 */
enum hal_transform_type {
	/* flip source image horizontally (around the vertical axis) */
	HAL_TRANSFORM_FLIP_H = DISPLAY_TRANSFORM_FLIP_H,
	/* flip source image vertically (around the horizontal axis)*/
	HAL_TRANSFORM_FLIP_V = DISPLAY_TRANSFORM_FLIP_V,
	/* rotate source image 90 degrees clockwise */
	HAL_TRANSFORM_ROT_90 = DISPLAY_TRANSFORM_ROT_90,
	/* rotate source image 180 degrees */
	HAL_TRANSFORM_ROT_180 = DISPLAY_TRANSFORM_ROT_180,
	/* rotate source image 270 degrees clockwise */
	HAL_TRANSFORM_ROT_270 = DISPLAY_TRANSFORM_ROT_270,
	/* don't use. see system/window.h */
	HAL_TRANSFORM_RESERVED = DISPLAY_TRANSFORM_RESERVED,
};

/**
 * @struct hal_color
 * @brief Structure holding color
 *
 */
typedef struct hal_color {
	union {
		 /* access as per-channel name for ARGB-8888 */
		struct {
			uint8_t b; /* blue component */
			uint8_t g; /* green component */
			uint8_t r; /* red component */
			uint8_t a; /* alpha component */
		};

		uint8_t  c8[4];  /* access as 8-bit byte array */
		uint16_t c16[2]; /* access as 16-bit half word array */
		uint32_t full;   /* access as 32-bit full value */
	};
} hal_color_t;

/**
 * @brief Construct color
 *
 * @param r r component
 * @param g g component
 * @param b b component
 * @param a a component
 *
 * @return color structure
 */
static inline hal_color_t hal_color_make(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	hal_color_t color = { .r = r, .g = g, .b = b, .a = a, };

	return color;
}

/**
 * @brief Construct color from 32-bit hex value
 *
 * @param c 32-bit color value argb-8888
 *
 * @return color structure
 */
static inline hal_color_t hal_color_hex(uint32_t c)
{
	hal_color_t color = { .full = c, };

	return color;
}

/**
 * @brief Construct color from 16-bit hex value
 *
 * @param c 16-bit color value rgb-565
 *
 * @return color structure
 */
static inline hal_color_t hal_color_hex16(uint16_t c)
{
	hal_color_t color = {
		.a = 0xff,
		.r = (c & 0xf800) >> 8,
		.g = (c & 0x07f0) >> 3,
		.b = (c & 0x001f) << 3,
	};

	color.r = color.r | ((color.r >> 5) & 0x7);
	color.g = color.g | ((color.g >> 6) & 0x3);
	color.b = color.b | ((color.b >> 5) & 0x7);

	return color;
}

/**
 * @brief Query display format is opaque
 *
 * @param pixel_format hal pixel format, see@enum hal_pixel_format
 *
 * @return the query result
 */
static inline bool hal_pixel_format_is_opaque(uint32_t pixel_format)
{
	return display_format_is_opaque(pixel_format);
}

/**
 * @brief Query pixel format bits per pixel
 *
 * @param pixel_format hal pixel format, see@enum hal_pixel_format
 *
 * @return the query result
 */
static inline uint8_t hal_pixel_format_get_bits_per_pixel(uint32_t pixel_format)
{
	return display_format_get_bits_per_pixel(pixel_format);
}

/**
 * @brief Get display format name string
 *
 * @param pixel_format hal pixel format, see@enum hal_pixel_format
 *
 * @return the name string
 */
static inline const char * hal_pixel_format_get_name(uint32_t pixel_format)
{
	return display_format_get_name(pixel_format);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_FRAMEWORK_INCLUDE_DISPLAY_DISPLAY_HAL_H_ */
