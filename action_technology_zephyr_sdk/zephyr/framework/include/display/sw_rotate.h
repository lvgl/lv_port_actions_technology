/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Utilities of sw rotation API
 */

#ifndef ZEPHYR_FRAMEWORK_INCLUDE_DISPLAY_SW_ROTATE_H_
#define ZEPHYR_FRAMEWORK_INCLUDE_DISPLAY_SW_ROTATE_H_

#include <stdbool.h>
#include "sw_math.h"

/**
 * @defgroup display-util Display Utilities
 * @ingroup display_libraries
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enumeration with possible predefined transformations
 */
enum {
	SW_FLIP_H  = 0x1, /* Flip source image horizontally */
	SW_FLIP_V  = 0x2, /* Flip source image verticallye */
	SW_ROT_90  = 0x4, /* Rotate source image 90 degrees clock-wise */
	SW_ROT_180 = 0x3, /* Rotate source image 180 degrees clock-wise */
	SW_ROT_270 = 0x7, /* Rotate source image 270 degrees clock-wise */
};

/**
 * @struct sw_matrix
 * @brief Structure holding 2x3 affine transform.
 *
 * [  sx shx  tx ]
 * [ shy  sy  ty ]
 * [ 0    0    1 ]
 *
 * Where:
 * sx and sy define scaling in the x and y directions, respectively;
 * shx and shy define shearing in the x and y directions, respectively;
 * tx and ty define translation in the x and y directions, respectively;
 *
 * An affine transformation maps a point (x, y) into the point (x', y') using
 * matrix multiplication:
 *
 * [  sx shx  tx ] [x]   [  x * sx  + y * shx + tx ]
 * [ shy  sy  ty ] [y] = [  x * shy + y * sy  + ty ]
 * [ 0    0    1 ] [1]   [              1          ]
 *
 * that is:
 * x' = sx * x + shx * y + tx
 * y' = shy * x + sy * y + ty
 */
typedef struct sw_matrix {
	int32_t tx;
	int32_t ty;
	int32_t sx;
	int32_t shy;
	int32_t shx;
	int32_t sy;
} sw_matrix_t;

/*
 * @brief generate a rotation configuration width scaling first
 *
 * The src is scaled then rotated to dest.
 *
 * @param img_x x coord of top-left corner of image src before rotation in the display coordinate
 * @param img_y y coord of top-left corner of image src before rotation in the display coordinate
 * @param pivot_x rotate/scale pivot X offset relative to top-left corner of the image
 * @param pivot_y rotate/scale pivot Y offset relative to top-left corner of the image
 * @param angle rotation angle in 0.1 degree [0, 3600)
 * @param scale_x scale factor X, equal to "(1 << scale_bits) * dest_w / src_w"
 * @param scale_y scale factor Y, equal to "(1 << scale_bits) * dest_h / src_h"
 * @param scale_bits scale factor bits of scale_x and scale_y
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_config(int16_t img_x, int16_t img_y,
		int16_t pivot_x, int16_t pivot_y, uint16_t angle,
		uint16_t scale_x, uint16_t scale_y, uint16_t scale_bits,
		sw_matrix_t *matrix);

/*
 * @brief generate a 90 (CW) degree rotation configuration
 *
 * Assume the top-left corner of image are (0, 0) in the display coordinate system
 * both before and after transformation.
 *
 * @param src_w transformed area width of source image
 * @param src_h transformed area height of source image
 * @param mode transformation mode
 *
 * @retval N/A
 */
void sw_transform_config_with_mode(uint16_t img_w, uint16_t img_h,
		uint8_t mode, sw_matrix_t *matrix);

/*
 * @brief rotate an rgb565 image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_rgb565_over_rgb565(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an rgb565 image over rg888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_rgb565_over_rgb888(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an rgb565 image over argb8888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_rgb565_over_argb8888(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an rgb565a8 image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_opa address of src opa image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_opa_pitch stride in bytes of src opa image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_rgb565a8_over_rgb565(void *dst, const void *src, const void *src_opa,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_opa_pitch,
		uint16_t src_w, uint16_t src_h, int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an rgb565a8 image over rg888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_opa address of src opa image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_opa_pitch stride in bytes of src opa image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_rgb565a8_over_rgb888(void *dst, const void *src, const void *src_opa,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_opa_pitch,
		uint16_t src_w, uint16_t src_h, int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an rgb565a8 image over argb8888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_opa address of src opa image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_opa_pitch stride in bytes of src opa image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_rgb565a8_over_argb8888(void *dst, const void *src, const void *src_opa,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_opa_pitch,
		uint16_t src_w, uint16_t src_h, int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an argb8565 image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_argb8565_over_rgb565(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an argb8565 image over rgb888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_argb8565_over_rgb888(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an argb8565 image over argb8888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_argb8565_over_argb8888(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an argb6666 image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_argb6666_over_rgb565(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an argb6666 image over rgb888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_argb6666_over_rgb888(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an argb6666 image over argb8888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_argb6666_over_argb8888(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an argb8888 image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_argb8888_over_rgb565(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an argb8888 image over rgb888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_argb8888_over_rgb888(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an argb8888 image over argb8888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_argb8888_over_argb8888(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an xrgb888 image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param color color of src image
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_xrgb8888_over_rgb565(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an xrgb888 image over rgb888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param color color of src image
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_xrgb8888_over_rgb888(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an xrgb888 image over argb8888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param color color of src image
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_xrgb8888_over_argb8888(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an rgb888 image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param color color of src image
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_rgb888_over_rgb565(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an rgb888 image over rgb888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param color color of src image
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_rgb888_over_rgb888(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an rgb888 image over argb8888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param color color of src image
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_rgb888_over_argb8888(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an a8 image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_color color of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_a8_over_rgb565(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an a8 image over rgb888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_color color of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_a8_over_rgb888(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an a8 image over argb8888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_color color of src image
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_a8_over_argb8888(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an index8 image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_clut address of src clut (ARGB8888 color lookup table)
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_index8_over_rgb565(void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an index8 image over rgb888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_clut address of src clut (ARGB8888 color lookup table)
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_index8_over_rgb888(void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an index8 image over argb8888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_clut address of src clut (ARGB8888 color lookup table)
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_index8_over_argb8888(void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an index4 (big endian) image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_clut address of src clut (ARGB8888 color lookup table)
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_index4_over_rgb565(void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an index4 (big endian) image over rgb888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_clut address of src clut (ARGB8888 color lookup table)
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_index4_over_rgb888(void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an index4 (big endian) image over argb8888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_clut address of src clut (ARGB8888 color lookup table)
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_index4_over_argb8888(void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an index2 (big endian) image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_clut address of src clut (ARGB8888 color lookup table)
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_index2_over_rgb565(void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an index2 (big endian) image over rgb888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_clut address of src clut (ARGB8888 color lookup table)
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_index2_over_rgb888(void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an index2 (big endian) image over argb8888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_clut address of src clut (ARGB8888 color lookup table)
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_index2_over_argb8888(void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an index1 (big endian) image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_clut address of src clut (ARGB8888 color lookup table)
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_index1_over_rgb565(void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an index1 (big endian) image over rgb888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_clut address of src clut (ARGB8888 color lookup table)
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_index1_over_rgb888(void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

/*
 * @brief rotate an index1 (big endian) image over argb8888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param src_clut address of src clut (ARGB8888 color lookup table)
 * @param dst_pitch stride in bytes of dst image
 * @param src_pitch stride in bytes of src image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x coordinate of the draw area in the display coordinate
 * @param y y coordinate of the draw area in the display coordinate
 * @param w width in pixels of the draw area
 * @param h height in pixels of the draw area
 * @param matrix matrix generated in func sw_transform_config()
 *
 * @retval N/A
 */
void sw_transform_index1_over_argb8888(void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_matrix_t *matrix);

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* ZEPHYR_FRAMEWORK_INCLUDE_DISPLAY_SW_ROTATE_H_ */
