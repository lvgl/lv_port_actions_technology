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

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* ZEPHYR_FRAMEWORK_INCLUDE_DISPLAY_SW_ROTATE_H_ */
