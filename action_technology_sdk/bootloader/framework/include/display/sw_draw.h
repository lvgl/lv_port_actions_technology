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

#ifndef ALWAYS_INLINE
#define ALWAYS_INLINE inline __attribute__((always_inline))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 *      pixel blending
 ******************************************************************************/
/*
 * @brief blend argb8888 over argb8888
 *
 * @param dest_color dest argb8888 value
 * @param src_color src argb8888 value
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
 * @brief blend rgb565 over rgb565
 *
 * @param dest_color dest rgb565 value
 * @param src_color src rgb565 value
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
 * @brief blend argb8888 over rgb565
 *
 * @param dest_color dest rgb565 value
 * @param src_color src argb8888 value
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
 * @param c00 color value at (0, 0)
 * @param c10 color value at (1, 0)
 * @param c01 color value at (0, 1)
 * @param c11 color value at (1, 1)
 * @param x_tap filter tap coefficient in x direction
 * @param y_tap filter tap coefficient in y direction
 * @param tap_bits bits of filter tap coefficient, must not greater than 6
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
 * @brief argb8888 bilinear filtering with lossy precition distance grid
 *
 * @param c00 color value at (0, 0)
 * @param c10 color value at (1, 0)
 * @param c01 color value at (0, 1)
 * @param c11 color value at (1, 1)
 * @param x_tap filter tap coefficient in x direction
 * @param y_tap filter tap coefficient in y direction
 * @param tap_bits bits of filter tap coefficient, must not greater than 8
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
 * @brief blend an argb8565 image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_stride stride in pixels of dst image
 * @param src_stride stride in pixels of src image
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_argb8565_over_rgb565(void *dst, const void *src,
		uint16_t dst_stride, uint16_t src_stride, uint16_t w, uint16_t h);

/*
 * @brief blend an argb6666 image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_stride stride in pixels of dst image
 * @param src_stride stride in pixels of src image
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_argb6666_over_rgb565(void *dst, const void *src,
		uint16_t dst_stride, uint16_t src_stride, uint16_t w, uint16_t h);

/*
 * @brief blend an argb8888 image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_stride stride in pixels of dst image
 * @param src_stride stride in pixels of src image
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_argb8888_over_rgb565(void *dst, const void *src,
		uint16_t dst_stride, uint16_t src_stride, uint16_t w, uint16_t h);

/*
 * @brief blend an argb8888 image over argb8888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_stride stride in pixels of dst image
 * @param src_stride stride in pixels of src image
 * @param w width in pixels of blend area
 * @param h height in pixels of blend area
 *
 * @retval N/A
 */
void sw_blend_argb8888_over_argb8888(void *dst, const void *src,
		uint16_t dst_stride, uint16_t src_stride, uint16_t w, uint16_t h);

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* ZEPHYR_FRAMEWORK_INCLUDE_DISPLAY_SW_DRAW_H_ */
