/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Utilities of sw draw API
 */

#ifndef ZEPHYR_FRAMEWORK_INCLUDE_DISPLAY_SW_DRAW_H_
#define ZEPHYR_FRAMEWORK_INCLUDE_DISPLAY_SW_DRAW_H_

#include <stddef.h>
#include <stdint.h>

/**
 * @defgroup display-util Display Utilities
 * @ingroup display_libraries
 * @{
 */

#ifdef _WIN32
#define ALWAYS_INLINE inline
#elif !defined(ALWAYS_INLINE)
#define ALWAYS_INLINE inline __attribute__((always_inline))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 *      public types
 ******************************************************************************/

typedef union {
	struct {
		uint16_t b : 5;
		uint16_t g : 6;
		uint16_t r : 5;
	};

	uint16_t full;
} sw_color16_t;

typedef struct {
	union {
		struct {
			uint16_t b : 5;
			uint16_t g : 6;
			uint16_t r : 5;
		};

		uint16_t rgb;
	};

	uint8_t a;
} sw_color16a8_t;

typedef union {
	struct {
		uint8_t b;
		uint8_t g;
		uint8_t r;
		uint8_t a;
	};

	uint32_t full;
} sw_color32_t;

/*******************************************************************************
 *      pixel format convert
 ******************************************************************************/
/*
 * @brief convert rgb565 to argb8888
 *
 * @param src_color value
 * @param dest_color value
 *
 * @retval N/A
 */
static ALWAYS_INLINE void cvt_rgb565_to_argb8888(uint8_t dest_color[4], const uint8_t src_color[2])
{
	dest_color[0] = (src_color[0] << 3) | (src_color[0] & 0x7);
	dest_color[1] = (src_color[1] << 5) | ((src_color[0] >> 3) & 0x1c) | ((src_color[0] >> 5) & 0x3);
	dest_color[2] = (src_color[1] & 0xf8) | ((src_color[1] >> 3) & 0x7);
	dest_color[3] = 0xFF;
}

/*
 * @brief convert argb8565 to argb8888
 *
 * @param src_color value
 * @param dest_color value
 *
 * @retval N/A
 */
static ALWAYS_INLINE void cvt_argb8565_to_argb8888(uint8_t dest_color[4], const uint8_t src_color[3])
{
	dest_color[0] = (src_color[0] << 3) | (src_color[0] & 0x7);
	dest_color[1] = (src_color[1] << 5) | ((src_color[0] >> 3) & 0x1c) | ((src_color[0] >> 5) & 0x3);
	dest_color[2] = (src_color[1] & 0xf8) | ((src_color[1] >> 3) & 0x7);
	dest_color[3] = src_color[2];
}

/*
 * @brief convert argb6666 to argb8888
 *
 * @param src_color value
 * @param dest_color value
 *
 * @retval N/A
 */
static ALWAYS_INLINE void cvt_argb6666_to_argb8888(uint8_t dest_color[4], const uint8_t src_color[3])
{
	dest_color[0] = (src_color[0] << 2) | (src_color[0] & 0x3);
	dest_color[1] = (src_color[1] << 4) | ((src_color[0] >> 4) & 0x0c) | (src_color[0] >> 6);
	dest_color[2] = (src_color[2] << 6) | ((src_color[1] >> 2) & 0x3c) | ((src_color[1] >> 4) & 0x03);
	dest_color[3] = (src_color[2] & 0xfc) | ((src_color[2] >> 2) & 0x03);
}

/*
 * @brief convert argb1555 to argb8888
 *
 * @param src_color value
 * @param dest_color value
 *
 * @retval N/A
 */
static ALWAYS_INLINE void cvt_argb1555_to_argb8888(uint8_t dest_color[4], const uint8_t src_color[2])
{
	dest_color[0] = (src_color[0] << 3) | (src_color[0] & 0x7);
	dest_color[1] = (src_color[1] << 6) | ((src_color[0] >> 2) & 0x38) | (src_color[0] >> 5);
	dest_color[2] = (src_color[1] & 0xf8) | ((src_color[1] >> 3) & 0x7);
	dest_color[3] = (src_color[1] & 0x8) ? 0xFF : 0;
}

/*
 * @brief convert argb8888 to rgb565
 *
 * @param src_color value
 * @param dest_color value
 *
 * @retval N/A
 */
static ALWAYS_INLINE void cvt_argb8888_to_rgb565(uint8_t dest_color[2], const uint8_t src_color[4])
{
	dest_color[0] = ((src_color[1] & 0x1c) << 3) | (src_color[0] >> 3);
	dest_color[1] = (src_color[2] & 0xF8) | (src_color[1]>> 5);
}

/*
 * @brief convert argb8888 to argb8565
 *
 * @param src_color value
 * @param dest_color value
 *
 * @retval N/A
 */
static ALWAYS_INLINE void cvt_argb8888_to_argb8565(uint8_t dest_color[3], const uint8_t src_color[4])
{
	dest_color[0] = ((src_color[1] & 0x1c) << 3) | (src_color[0] >> 3);
	dest_color[1] = (src_color[2] & 0xF8) | (src_color[1]>> 5);
	dest_color[2] = src_color[3];
}

/*
 * @brief convert argb8888 to argb6666
 *
 * @param src_color value
 * @param dest_color value
 *
 * @retval N/A
 */
static ALWAYS_INLINE void cvt_argb8888_to_argb6666(uint8_t dest_color[3], const uint8_t src_color[4])
{
	dest_color[0] = ((src_color[1] & 0x0c) << 4) | ((src_color[0] & 0xfc) >> 2);
	dest_color[1] = ((src_color[2] & 0x3c) << 2) | ((src_color[1] & 0xf0) >> 4);
	dest_color[2] = (src_color[3] & 0xfc) | ((src_color[2] & 0xc0) >> 6);
}

/*
 * @brief convert argb8888 to argb1555
 *
 * @param src_color value
 * @param dest_color value
 *
 * @retval N/A
 */
static ALWAYS_INLINE void cvt_argb8888_to_argb1555(uint8_t dest_color[2], const uint8_t src_color[4])
{
	dest_color[0] = ((src_color[1] & 0x38) << 2) | (src_color[0] >> 3);
	dest_color[1] = ((src_color[2] & 0xF8) >> 1) | (src_color[3] & 0x80);
}

/*******************************************************************************
 *      pixel format conversion
 ******************************************************************************/
/*
 * @brief convert buffer pixel format
 *
 * @param dest_buf dest buffer address
 * @param dest_cf dest buffer pixel format
 * @param src_buf source buffer address
 * @param src_cf source buffer pixel format
 * @param len number of pixels to convert
 *
 * @retval argb8888 color value
 */
int sw_convert_color_buffer(void * dest_buf, uint32_t dest_cf, const void * src_buf, uint32_t src_cf, uint32_t len);

/*******************************************************************************
 *      pixel blending
 ******************************************************************************/
/*
 * @brief blend argb8888 over argb8888
 *
 * @param dest_color dest argb8888 value
 * @param src_color src argb8888 value
 *
 * @retval argb8888 color value
 */
static ALWAYS_INLINE uint32_t mix_argb8888_over_argb8888(uint32_t dest_color, uint32_t src_color)
{
	uint32_t a = src_color >> 24;
	uint32_t ra = 255 - a;
	uint32_t result_1 = (src_color & 0x00FF00FF) * a + (dest_color & 0x00FF00FF) * ra;
	uint32_t result_2 = ((src_color & 0xFF00FF00) >> 8) * a + ((dest_color & 0xFF00FF00) >> 8) * ra;

	return ((result_1 & 0xFF00FF00) >> 8) | (result_2 & 0xFF00FF00);
}

/*
 * @brief blend argb8888 over rgb565
 *
 * @param dest_color dest rgb565 value
 * @param src_color src argb8888 value
 *
 * @retval rgb565 color value
 */
static ALWAYS_INLINE uint16_t mix_argb8888_over_rgb565(uint16_t dest_color, uint32_t src_color)
{
	uint32_t a = src_color >> 24;
	uint32_t ra = 255 - a;

#if 0
	uint32_t dest32_rb = ((dest_color & 0xf800) << 5) | (dest_color & 0x1f);
	dest32_rb = (dest32_rb << 3) | (dest32_rb & 0x00070007);
	dest32_rb = (src_color & 0x00FF00FF) * a + dest32_rb * ra;

	uint32_t dest32_g = ((dest_color & 0x07e0) << 5) | ((dest_color & 0x0060) << 3);
	dest32_g = (src_color & 0x0000FF00) * a + dest32_g * ra;

	return ((dest32_rb >> 16) & 0xf800) | ((dest32_rb >> 11) & 0x1f) |
			((dest32_g >> 13) & 0x07e0);
#else
	uint32_t dest32_rb = ((dest_color & 0xf800) << 8) | ((dest_color & 0x1f) << 3);
	dest32_rb = (src_color & 0x00FF00FF) * a + dest32_rb * ra;

	uint32_t dest32_g = (dest_color & 0x07e0) << 5;
	dest32_g = (src_color & 0x0000FF00) * a + dest32_g * ra;

	return ((dest32_rb >> 16) & 0xf800) | ((dest32_rb >> 11) & 0x1f) |
			((dest32_g >> 13) & 0x07e0);
#endif
}

/*
 * @brief blend rgb565 over rgb565
 *
 * @param dest_color dest rgb565 value
 * @param src_color src rgb565 value
 *
 * @retval rgb565 color value
 */
static ALWAYS_INLINE uint16_t mix_rgb565_over_rgb565(uint16_t dest_color, uint16_t src_color, uint8_t src_a)
{
	uint16_t dest_r = (dest_color >> 11) * (255 - src_a) + (src_color >> 11) * src_a;
	uint16_t dest_b = (dest_color & 0x001f) * (255 - src_a) + (src_color & 0x001f) * src_a;
	uint32_t dest_g = (dest_color & 0x07e0) * (255 - src_a) + (src_color & 0x07e0) * (uint32_t)src_a;

	return ((dest_r >> 8) << 11) | ((dest_g >> 8) & 0x07e0) | (dest_b >> 8);
}

/*
 * @brief blend rgb565 over rgb565
 *
 * @param dest_color dest rgb565 value
 * @param src_color src rgb565 value
 *
 * @retval rgb565 color value
 */
static ALWAYS_INLINE uint16_t blend_rgb565_over_rgb565(uint16_t dest_color, uint16_t src_color, uint8_t src_a)
{
	if (src_a <= 0) {
		return dest_color;
	} else if (src_a >= 255) {
		return src_color;
	} else {
		return mix_rgb565_over_rgb565(dest_color, src_color, src_a);
	}
}

/*
 * @brief blend rgb565 over argb8888
 *
 * @param dest_color dest argb8888 value
 * @param src_color src rgb565 value
 *
 * @retval argb8888 color value
 */
static ALWAYS_INLINE uint32_t blend_rgb565_over_argb8888(uint32_t dest_color, uint16_t src_color, uint8_t src_a)
{
	if (src_a <= 0) {
		return dest_color;
	} else {
		uint32_t src32_ag = ((src_color & 0x07e0) >> 3) | ((src_color & 0x0060) >> 5) | ((uint32_t)src_a << 16);
		uint32_t src32_rb = ((src_color & 0xf800) << 5) | (src_color & 0x1f);
		src32_rb = (src32_rb << 3) | (src32_rb & 0x00070007);

		if (src_a >= 255) {
			return (src32_ag << 8) | src32_rb;
		} else {
			uint32_t dest32_rb = src32_rb * src_a + (dest_color & 0x00FF00FF) * (255 - src_a);
			uint32_t dest32_ag = src32_ag * src_a + ((dest_color & 0xFF00FF00) >> 8) * (255 - src_a);

			return ((dest32_rb & 0xFF00FF00) >> 8) | (dest32_ag & 0xFF00FF00);
		}
	}
}

/*
 * @brief blend argb8565 over rgb565
 *
 * @param dest_color dest rgb565 value
 * @param src_color src argb8565 value
 *
 * @retval rgb565 color value
 */
static ALWAYS_INLINE uint16_t blend_argb8565_over_rgb565(uint16_t dest_color, const uint8_t src_color[3])
{
	return blend_rgb565_over_rgb565(dest_color, ((uint16_t)src_color[1] << 8) | src_color[0], src_color[2]);
}

/*
 * @brief blend argb8565 over argb8888
 *
 * @param dest_color dest argb8888 value
 * @param src_color src argb8565 value
 *
 * @retval argb8888 color value
 */
static ALWAYS_INLINE uint32_t blend_argb8565_over_argb8888(uint32_t dest_color, const uint8_t src_color[3])
{
	return blend_rgb565_over_argb8888(dest_color, ((uint16_t)src_color[1] << 8) | src_color[0], src_color[2]);
}

/*
 * @brief blend rgb565 over rgb565
 *
 * @param dest_color dest rgb565 value
 * @param src_color src argb6666 value
 *
 * @retval rgb565 color value
 */
static ALWAYS_INLINE uint16_t blend_argb6666_over_rgb565(uint16_t dest_color, const uint8_t src_color[3])
{
	uint8_t src_a = src_color[2] >> 2;

	if (src_a <= 0) {
		return dest_color;
	} else {
		uint8_t src_b = (src_color[0] & 0x3f);
		uint8_t src_g = ((src_color[1] & 0x0f) << 2) | (src_color[0] >> 6);
		uint8_t src_r = ((src_color[2] & 0x03) << 4) | (src_color[1] >> 4);

		if (src_a >= 63) {
			return ((src_r & 0x3e) << 10) | ((uint16_t)src_g << 5) | (src_b >> 1);
		} else {
			uint16_t dest_b = ((dest_color & 0x001f) << 1) | (dest_color & 0x1);
			dest_b = dest_b * (63 - src_a) + (uint16_t)src_b * src_a;

			uint16_t dest_g = (dest_color & 0x07e0) >> 5;
			dest_g = dest_g * (63 - src_a) + (uint16_t)src_g * src_a;

			uint16_t dest_r = ((dest_color & 0xf800) >> 10) | ((dest_color & 0x0800) >> 11);
			dest_r = dest_r * (63 - src_a) + (uint16_t)src_r * src_a;

			return ((dest_r & 0x0f80) << 4) | ((dest_g & 0x0fc0) >> 1) | (dest_b >> 7);
		}
	}
}

/*
 * @brief blend argb6666 over argb8888
 *
 * @param dest_color dest rgb565 value
 * @param src_color src argb6666 value
 *
 * @retval argb8888 color value
 */
static ALWAYS_INLINE uint32_t blend_argb6666_over_argb8888(uint32_t dest_color, const uint8_t src_color[3])
{
	uint8_t src_a = src_color[2] | ((src_color[2] &0x0c) >> 2);

	if (src_a <= 0) {
		return dest_color;
	} else {
		uint8_t src_b = ((src_color[0] & 0x3f) << 2) | (src_color[0] & 0x03);
		uint32_t src32_r = ((src_color[2] & 0x03) << 22) | ((src_color[1] & 0xf0) << 14) | ((src_color[1] & 0x30) << 12);
		uint32_t src32_rb = src32_r | src_b;
		uint32_t src32_g = ((src_color[1] & 0x0f) << 4) | ((src_color[0] & 0xc0) >> 2) | ((src_color[0] & 0xc0) >> 6);
		uint32_t src32_ag = src32_g | ((uint32_t)src_a << 16) ;

		if (src_a >= 255) {
			return (src32_ag << 8) | src32_rb;
		} else {
			uint32_t dest32_rb = src32_rb * src_a + (dest_color & 0x00FF00FF) * (255 - src_a);
			uint32_t dest32_ag = src32_ag * src_a + ((dest_color & 0xFF00FF00) >> 8) * (255 - src_a);

			return ((dest32_rb & 0xFF00FF00) >> 8) | (dest32_ag & 0xFF00FF00);
		}
	}
}

/*
 * @brief blend argb1555 over rgb565
 *
 * @param dest_color dest rgb565 value
 * @param src_color src argb1555 value
 *
 * @retval rgb565 color value
 */
static ALWAYS_INLINE uint16_t blend_argb1555_over_rgb565(uint16_t dest_color, uint16_t src_color)
{
	if (src_color & 0x8000) {
		return (src_color & 0x3f) | ((src_color & 0xffe0) << 1);
	} else {
		return dest_color;
	}
}

/*
 * @brief blend argb1555 over argb8888
 *
 * @param dest_color dest argb8888 value
 * @param src_color src argb1555 value
 *
 * @retval argb8888 color value
 */
static ALWAYS_INLINE uint32_t blend_argb1555_over_argb8888(uint32_t dest_color, uint16_t src_color)
{
	if (src_color & 0x8000) {
		return ((src_color & 0x1f) << 3) | (src_color & 0x7) | /* b */
				((src_color & 0x3e0) << 6) | ((src_color & 0xe0) << 3) | /* g */
				((src_color & 0x7c00) << 9) | ((src_color & 0x1c00) << 6) | /* r */
				0xff000000;
	} else {
		return dest_color;
	}
}

/*
 * @brief blend argb8888 over rgb565
 *
 * @param dest_color dest rgb565 value
 * @param src_color src argb8888 value
 *
 * @retval rgb565 color value
 */
static ALWAYS_INLINE uint16_t blend_argb8888_over_rgb565(uint16_t dest_color, uint32_t src_color)
{
	uint8_t src_a = src_color >> 24;

	if (src_a <= 0) {
		return dest_color;
	} else if (src_a >= 255) {
		return ((src_color & 0xf80000) >> 8) | ((src_color & 0x00fc00) >> 5) |
				((src_color & 0x0000f8) >> 3);
	} else {
		return mix_argb8888_over_rgb565(dest_color, src_color);
	}
}

/*
 * @brief blend argb8888 over argb8888
 *
 * @param dest_color dest argb8888 value
 * @param src_color src argb8888 value
 *
 * @retval argb8888 color value
 */
static ALWAYS_INLINE uint32_t blend_argb8888_over_argb8888(uint32_t dest_color, uint32_t src_color)
{
	uint8_t src_a = src_color >> 24;

	if (src_a <= 0) {
		return dest_color;
	} else if (src_a >= 255) {
		return src_color;
	} else {
		return mix_argb8888_over_argb8888(dest_color, src_color);
	}
}

/*******************************************************************************
 *      pixel filtering
 ******************************************************************************/
/*
 * @brief rgb565 bilinear filtering with lossy precition distance grid
 *
 * c00 | c10
 * ----|----
 * c01 | c11
 *
 * @param c00 color value at (0, 0)
 * @param c10 color value at (1, 0)
 * @param c01 color value at (0, 1)
 * @param c11 color value at (1, 1)
 * @param x_tap filter tap coefficient in x direction
 * @param y_tap filter tap coefficient in y direction
 * @param tap_bits bits of filter tap coefficient, must not greater than 6
 *
 * @retval rgb565 color value
 */
static ALWAYS_INLINE uint16_t bilinear_rgb565_fast_m6(
		uint16_t c00, uint16_t c10, uint16_t c01, uint16_t c11,
		int16_t x_tap, int16_t y_tap, int16_t tap_bits)
{
	uint32_t pm11 = (x_tap * y_tap) >> tap_bits;
	uint32_t pm10 = x_tap - pm11;
	uint32_t pm01 = y_tap - pm11;
	uint32_t pm00 = (1 << tap_bits) - pm01 - pm10 - pm11;

	uint32_t rb = (c00 & 0xf81f) * pm00;
	uint32_t g = (c00 & 0x07e0) * pm00;

	rb += (c10 & 0xf81f) * pm10;
	g += (c10 & 0x07e0) * pm10;

	rb += (c01 & 0xf81f) * pm01;
	g += (c01 & 0x07e0) * pm01;

	rb += (c11 & 0xf81f) * pm11;
	g += (c11 & 0x07e0) * pm11;

	return ((rb >> tap_bits) & 0xf81f) | ((g >> tap_bits) & 0x07e0);
}

/*
 * @brief argb8565 bilinear filtering with lossy precition distance grid
 *
 * c00 | c10
 * ----|----
 * c01 | c11
 *
 * @param result store the result
 * @param c00 color value at (0, 0)
 * @param c10 color value at (1, 0)
 * @param c01 color value at (0, 1)
 * @param c11 color value at (1, 1)
 * @param x_tap filter tap coefficient in x direction
 * @param y_tap filter tap coefficient in y direction
 * @param tap_bits bits of filter tap coefficient, must not greater than 6
 *
 * @retval N/A
 */
static ALWAYS_INLINE void bilinear_argb8565_fast_m6(
		uint8_t result[3],
		const uint8_t c00[3], const uint8_t c10[3],
		const uint8_t c01[3], const uint8_t c11[3],
		int16_t x_tap, int16_t y_tap, int16_t tap_bits)
{
	uint32_t pm11 = (x_tap * y_tap) >> tap_bits;
	uint32_t pm10 = x_tap - pm11;
	uint32_t pm01 = y_tap - pm11;
	uint32_t pm00 = (1 << tap_bits) - pm01 - pm10 - pm11;

	uint32_t c16 = ((uint16_t)c00[1] << 8) | c00[0];
	uint32_t rb = (c16 & 0xf81f) * pm00;
	uint32_t g = (c16 & 0x07e0) * pm00;
	uint32_t a = c00[2] * pm00;

	c16 = ((uint16_t)c10[1] << 8) | c10[0];
	rb += (c16 & 0xf81f) * pm10;
	g += (c16 & 0x07e0) * pm10;
	a += c10[2] * pm10;

	c16 = ((uint16_t)c01[1] << 8) | c01[0];
	rb += (c16 & 0xf81f) * pm01;
	g += (c16 & 0x07e0) * pm01;
	a += c01[2] * pm01;

	c16 = ((uint16_t)c11[1] << 8) | c11[0];
	rb += (c16 & 0xf81f) * pm11;
	g += (c16 & 0x07e0) * pm11;
	a += c11[2] * pm11;

	c16 = ((rb >> tap_bits) & 0xf81f) | ((g >> tap_bits) & 0x07e0);

	result[0] = c16 & 0xff;
	result[1] = c16 >> 8;
	result[2] = a >> tap_bits;
}

/*
 * @brief argb6666 bilinear filtering with lossy precition distance grid
 *
 * c00 | c10
 * ----|----
 * c01 | c11
 *
 * @param result store the result
 * @param c00 color value at (0, 0)
 * @param c10 color value at (1, 0)
 * @param c01 color value at (0, 1)
 * @param c11 color value at (1, 1)
 * @param x_tap filter tap coefficient in x direction
 * @param y_tap filter tap coefficient in y direction
 * @param tap_bits bits of filter tap coefficient, must not greater than 6
 *
 * @retval N/A
 */
static ALWAYS_INLINE void bilinear_argb6666_fast_m6(
		uint8_t result[3],
		const uint8_t c00[3], const uint8_t c10[3],
		const uint8_t c01[3], const uint8_t c11[3],
		int16_t x_tap, int16_t y_tap, int16_t tap_bits)
{
	uint32_t pm11 = (x_tap * y_tap) >> tap_bits;
	uint32_t pm10 = x_tap - pm11;
	uint32_t pm01 = y_tap - pm11;
	uint32_t pm00 = (1 << tap_bits) - pm01 - pm10 - pm11;

	uint32_t c32 = ((uint32_t)c00[2] << 16) | ((uint32_t)c00[1] << 8) | c00[0];
	uint32_t rb = (c32 & 0x03f03f) * pm00;
	uint32_t ag = (c32 & 0xfc0fc0) * pm00;

	c32 = ((uint32_t)c10[2] << 16) | ((uint32_t)c10[1] << 8) | c10[0];
	rb += (c32 & 0x03f03f) * pm10;
	ag += (c32 & 0xfc0fc0) * pm10;

	c32 = ((uint32_t)c01[2] << 16) | ((uint32_t)c01[1] << 8) | c01[0];
	rb += (c32 & 0x03f03f) * pm01;
	ag += (c32 & 0xfc0fc0) * pm01;

	c32 = ((uint32_t)c11[2] << 16) | ((uint32_t)c11[1] << 8) | c11[0];
	rb += (c32 & 0x03f03f) * pm11;
	ag += (c32 & 0xfc0fc0) * pm11;

	c32 = ((rb >> tap_bits) & 0x03f03f) | ((ag >> tap_bits) & 0xfc0fc0);

	result[0] = c32 & 0xff;
	result[1] = (c32 >> 8) & 0xff;
	result[2] = (c32 >> 16) & 0xff;
}

/*
 * @brief argb8888 bilinear filtering with lossy precition distance grid
 *
 * c00 | c10
 * ----|----
 * c01 | c11
 *
 * @param c00 color value at (0, 0)
 * @param c10 color value at (1, 0)
 * @param c01 color value at (0, 1)
 * @param c11 color value at (1, 1)
 * @param x_tap filter tap coefficient in x direction
 * @param y_tap filter tap coefficient in y direction
 * @param tap_bits bits of filter tap coefficient, must not greater than 8
 *
 * @retval argb8888 color value
 */
static ALWAYS_INLINE uint32_t bilinear_argb8888_fast_m8(
		uint32_t c00, uint32_t c10, uint32_t c01, uint32_t c11,
		int16_t x_tap, int16_t y_tap, int16_t tap_bits)
{
	uint32_t pm11 = ((uint32_t)x_tap * y_tap) >> tap_bits;
	uint32_t pm10 = x_tap - pm11;
	uint32_t pm01 = y_tap - pm11;
	uint32_t pm00 = (1 << tap_bits) - pm01 - pm10 - pm11;

	uint32_t rb = (c00 & 0x00FF00FF) * pm00;
	uint32_t ag = ((c00 & 0xFF00FF00) >> tap_bits) * pm00;

	rb += (c10 & 0x00FF00FF) * pm10;
	ag += ((c10 & 0xFF00FF00) >> tap_bits) * pm10;

	rb += (c01 & 0x00FF00FF) * pm01;
	ag += ((c01 & 0xFF00FF00) >> tap_bits) * pm01;

	rb += (c11 & 0x00FF00FF) * pm11;
	ag += ((c11 & 0xFF00FF00) >> tap_bits) * pm11;

	return ((rb >> tap_bits) & 0x00FF00FF) | (ag & 0xFF00FF00);
}

/*
 * @brief rgb888 (r[23:16], g[15:8], b[7:0]) bilinear filtering with lossy precition distance grid
 *
 * c00 | c10
 * ----|----
 * c01 | c11
 *
 * @param c00 color value at (0, 0)
 * @param c10 color value at (1, 0)
 * @param c01 color value at (0, 1)
 * @param c11 color value at (1, 1)
 * @param x_tap filter tap coefficient in x direction
 * @param y_tap filter tap coefficient in y direction
 * @param tap_bits bits of filter tap coefficient, must not greater than 8
 *
 * @retval rgb888 color value (lower 24bits)
 */
static ALWAYS_INLINE uint32_t bilinear_rgb888_fast_m8(
		const uint8_t *c00, const uint8_t *c10, const uint8_t *c01, const uint8_t *c11,
		int16_t x_tap, int16_t y_tap, int16_t tap_bits)
{
	uint32_t pm11 = ((uint32_t)x_tap * y_tap) >> tap_bits;
	uint32_t pm10 = x_tap - pm11;
	uint32_t pm01 = y_tap - pm11;
	uint32_t pm00 = (1 << tap_bits) - pm01 - pm10 - pm11;

	uint32_t rb = (c00[0] | (c00[2] << 16)) * pm00;
	uint16_t ag = c00[1] * pm00;

	rb += (c10[0] | (c10[2] << 16)) * pm10;
	ag += c10[1] * pm10;

	rb += (c01[0] | (c01[2] << 16)) * pm01;
	ag += c01[1] * pm01;

	rb += (c11[0] | (c11[2] << 16)) * pm11;
	ag += c11[1] * pm11;

	return ((rb >> tap_bits) & 0x00FF00FF) | ((ag >> tap_bits) << 8);
}

/*
 * @brief A8 bilinear filtering with lossy precition distance grid
 *
 * c00 | c10
 * ----|----
 * c01 | c11
 *
 * @param c00 color value at (0, 0)
 * @param c10 color value at (1, 0)
 * @param c01 color value at (0, 1)
 * @param c11 color value at (1, 1)
 * @param x_tap filter tap coefficient in x direction
 * @param y_tap filter tap coefficient in y direction
 * @param tap_bits bits of filter tap coefficient, must not greater than 8
 *
 * @retval A8 color value
 */
static ALWAYS_INLINE uint8_t bilinear_a8_fast_m8(
		uint8_t c00, uint8_t c10, uint8_t c01, uint8_t c11,
		int16_t x_tap, int16_t y_tap, int16_t tap_bits)
{
	uint32_t pm11 = ((uint32_t)x_tap * y_tap) >> tap_bits;
	uint32_t pm10 = x_tap - pm11;
	uint32_t pm01 = y_tap - pm11;
	uint32_t pm00 = (1 << tap_bits) - pm01 - pm10 - pm11;
	uint32_t alpha = c00 * pm00;

	alpha += c10 * pm10;
	alpha += c01 * pm01;
	alpha += c11 * pm11;

	return (alpha >> tap_bits);
}

/*
 * @brief blend a const color over rgb565 image
 *
 * @param dst address of dst image
 * @param src_color constant color (ARGB8888) of src
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 * @param opa global opa of src
 *
 * @retval N/A
 */
void sw_blend_color_over_rgb565(void *dst, uint32_t src_color,
		uint16_t dst_pitch, uint16_t w, uint16_t h);

/*
 * @brief blend a const color over rgb888 image
 *
 * @param dst address of dst image
 * @param src_color constant color (ARGB8888) of src
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 * @param opa global opa of src
 *
 * @retval N/A
 */
void sw_blend_color_over_rgb888(void *dst, uint32_t src_color,
		uint16_t dst_pitch, uint16_t w, uint16_t h);

/*
 * @brief blend a const color over argb8888 image
 *
 * @param dst address of dst image
 * @param src_color constant color (ARGB8888) of src
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 * @param opa global opa of src
 *
 * @retval N/A
 */
void sw_blend_color_over_argb8888(void *dst, uint32_t src_color,
		uint16_t dst_pitch, uint16_t w, uint16_t h);

/*
 * @brief blend a A8 image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_color constant color (ARGB8888) of src
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 * @param opa global opa of src
 *
 * @retval N/A
 */
void sw_blend_a8_over_rgb565(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h);

/*
 * @brief blend a A8 image over rgb888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_color constant color (ARGB8888) of src
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 * @param opa global opa of src
 *
 * @retval N/A
 */
void sw_blend_a8_over_rgb888(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h);

/*
 * @brief blend a A8 image over argb8888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_color constant color (ARGB8888) of src
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 * @param opa global opa of src
 *
 * @retval N/A
 */
void sw_blend_a8_over_argb8888(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h);

/*
 * @brief blend a A4 (big endian) image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_color constant color (ARGB8888) of src
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_bofs src address bit offset
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 * @param opa global opa of src
 *
 * @retval N/A
 */
void sw_blend_a4_over_rgb565(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h);

/*
 * @brief blend a A4 (big endian) image over rgb888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_color constant color (ARGB8888) of src
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_bofs src address bit offset
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 * @param opa global opa of src
 *
 * @retval N/A
 */
void sw_blend_a4_over_rgb888(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h);

/*
 * @brief blend a A4 (big endian) image over argb8888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_color constant color (ARGB8888) of src
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_bofs src address bit offset
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 * @param opa global opa of src
 *
 * @retval N/A
 */
void sw_blend_a4_over_argb8888(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h);

/*
 * @brief blend a A2 (big endian) image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_color constant color (ARGB8888) of src
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_bofs src address bit offset
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 * @param opa global opa of src
 *
 * @retval N/A
 */
void sw_blend_a2_over_rgb565(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h);

/*
 * @brief blend a A2 (big endian) image over rgb888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_color constant color (ARGB8888) of src
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_bofs src address bit offset
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 * @param opa global opa of src
 *
 * @retval N/A
 */
void sw_blend_a2_over_rgb888(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h);

/*
 * @brief blend a A2 (big endian) image over argb8888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_color constant color (ARGB8888) of src
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_bofs src address bit offset
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 * @param opa global opa of src
 *
 * @retval N/A
 */
void sw_blend_a2_over_argb8888(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h);

/*
 * @brief blend a A1 (big endian) image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_color constant color (ARGB8888) of src
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_bofs src address bit offset
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 * @param opa global opa of src
 *
 * @retval N/A
 */
void sw_blend_a1_over_rgb565(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h);

/*
 * @brief blend a A1 (big endian) image over rgb888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_color constant color (ARGB8888) of src
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_bofs src address bit offset
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 * @param opa global opa of src
 *
 * @retval N/A
 */
void sw_blend_a1_over_rgb888(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h);

/*
 * @brief blend a A1 (big endian) image over argb8888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_color constant color (ARGB8888) of src
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_bofs src address bit offset
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 * @param opa global opa of src
 *
 * @retval N/A
 */
void sw_blend_a1_over_argb8888(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h);

/*
 * @brief blend an index8 image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image'
 * @param src_clut address of src clut (ARGB8888 color lookup table)
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_index8_over_rgb565(void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h);

/*
 * @brief blend an index8 image over rgb888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_clut address of src clut (ARGB8888 color lookup table)
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_index8_over_rgb888(void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h);

/*
 * @brief blend an index8 image over argb8888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_clut address of src clut (ARGB8888 color lookup table)
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_index8_over_argb8888(void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h);

/*
 * @brief blend an index4 (big endian) image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_clut address of src clut (ARGB8888 color lookup table)
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_bofs src address bit offset
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_index4_over_rgb565(void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs, uint16_t w, uint16_t h);

/*
 * @brief blend an index4 (big endian) image over rgb888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_clut address of src clut (ARGB8888 color lookup table)
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_bofs src address bit offset
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_index4_over_rgb888(void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs, uint16_t w, uint16_t h);

/*
 * @brief blend an index4 (big endian) image over argb8888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_clut address of src clut (ARGB8888 color lookup table)
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_bofs src address bit offset
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_index4_over_argb8888(void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs, uint16_t w, uint16_t h);

/*
 * @brief blend an index2 (big endian) image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_clut address of src clut (ARGB8888 color lookup table)
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_bofs src address bit offset
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_index2_over_rgb565(void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs, uint16_t w, uint16_t h);

/*
 * @brief blend an index2 (big endian) image over rgb888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_clut address of src clut (ARGB8888 color lookup table)
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_bofs src address bit offset
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_index2_over_rgb888(void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs, uint16_t w, uint16_t h);

/*
 * @brief blend an index2 (big endian) image over argb8888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_clut address of src clut (ARGB8888 color lookup table)
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_bofs src address bit offset
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_index2_over_argb8888(void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs, uint16_t w, uint16_t h);

/*
 * @brief blend an index1 (big endian) image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_clut address of src clut (ARGB8888 color lookup table)
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_bofs src address bit offset
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_index1_over_rgb565(void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs, uint16_t w, uint16_t h);

/*
 * @brief blend an index1 (big endian) image over rgb888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_clut address of src clut (ARGB8888 color lookup table)
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_bofs src address bit offset
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_index1_over_rgb888(void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs, uint16_t w, uint16_t h);

/*
 * @brief blend an index1 (big endian) image over argb8888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_clut address of src clut (ARGB8888 color lookup table)
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_bofs src address bit offset
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_index1_over_argb8888(void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs, uint16_t w, uint16_t h);

/*
 * @brief blend an rgb565a8 image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_opa address of src opa image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_opa_pitch stride in bytes of src opa image
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_rgb565a8_over_rgb565(void *dst, const void *src, const void *src_opa,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_opa_pitch, uint16_t w, uint16_t h);

/*
 * @brief blend an rgb565a8 image over rgb888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_opa address of src opa image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_opa_pitch stride in bytes of src opa image
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_rgb565a8_over_rgb888(void *dst, const void *src, const void *src_opa,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_opa_pitch, uint16_t w, uint16_t h);

/*
 * @brief blend an rgb565a8 image over argb8888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_opa address of src opa image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_opa_pitch stride in bytes of src opa image
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_rgb565a8_over_argb8888(void *dst, const void *src, const void *src_opa,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_opa_pitch, uint16_t w, uint16_t h);

/*
 * @brief blend an argb8565 image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_argb8565_over_rgb565(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h);

/*
 * @brief blend an argb8565 image over rgb888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_argb8565_over_rgb888(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h);

/*
 * @brief blend an argb8565 image over argb8888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_argb8565_over_argb8888(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h);

/*
 * @brief blend an argb6666 image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_argb6666_over_rgb565(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h);

/*
 * @brief blend an argb6666 image over rgb888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_argb6666_over_rgb888(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h);

/*
 * @brief blend an argb6666 image over argb8888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_argb6666_over_argb8888(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h);

/*
 * @brief blend an argb1555 image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_argb1555_over_rgb565(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h);

/*
 * @brief blend an argb1555 image over rgb888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_argb1555_over_rgb888(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h);

/*
 * @brief blend an argb1555 image over argb8888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_argb1555_over_argb8888(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h);

/*
 * @brief blend an argb8888 image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_argb8888_over_rgb565(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h);

/*
 * @brief blend an argb8888 image over rgb888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_argb8888_over_rgb888(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h);

/*
 * @brief blend an argb8888 image over argb8888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_argb8888_over_argb8888(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h);

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* ZEPHYR_FRAMEWORK_INCLUDE_DISPLAY_SW_DRAW_H_ */
