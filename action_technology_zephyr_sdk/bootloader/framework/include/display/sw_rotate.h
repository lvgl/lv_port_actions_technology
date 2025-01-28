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

#include "sw_math.h"

/**
 * @defgroup display-util Display Utilities
 * @ingroup display_libraries
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sw_rotate_config {
	/* assistant variables */
	int32_t src_coord_x0;    /* src X coord in .16 fixedpoint mapping to dest coord (0, 0) */
	int32_t src_coord_y0;    /* src Y coord in .16 fixedpoint mapping to dest coord (0, 0) */
	int32_t src_coord_dx_ax; /* src X coord diff in .16 fixedpoint along the dest X-axis */
	int32_t src_coord_dy_ax; /* src Y coord diff in .16 fixedpoint along the dest X-axis */
	int32_t src_coord_dx_ay; /* src X coord diff in .16 fixedpoint along the dest Y-axis */
	int32_t src_coord_dy_ay; /* src Y coord diff in .16 fixedpoint along the dest Y-axis */
} sw_rotate_config_t;

/*
 * @brief generate a rotation configuration
 *
 * The pivot keep the same before and after rotation.
 *
 * @param draw_x x coord of top-left corner of draw area after rotation
 * @param draw_y y coord of top-left corner of draw area after rotation
 * @param img_x x coord of top-left corner of image area before rotation
 * @param img_y y coord of top-left corner of image area before rotation
 * @param pivot_x x coord of rotation pivot (the same pivot during rotation)
 * @param pivot_y y coord of rotation pivot (the same pivot during rotation)
 * @param angle rotation angle in 0.1 degree [0, 3600)
 * @param cfg rotation config generated in func sw_rotate_configure()
 *
 * @retval N/A
 */
void sw_rotate_configure(int16_t draw_x, int16_t draw_y, int16_t img_x, int16_t img_y,
		int16_t pivot_x, int16_t pivot_y, uint16_t angle, sw_rotate_config_t *cfg);

/*
 * @brief rotate an rgb565 image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_stride stride in pixels of dst image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x offset from the top-left corner of original draw area in sw_rotate_configure()
 * @param y y offset from the top-left corner of original draw area in sw_rotate_configure()
 * @param w width in pixels of current draw area
 * @param h height in pixels of current draw  area
 * @param cfg rotation config generated in func sw_rotate_configure()
 *
 * @retval N/A
 */
void sw_rotate_rgb565_over_rgb565(void *dst, const void *src,
		uint16_t dst_stride, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_rotate_config_t *cfg);

/*
 * @brief rotate an argb8565 image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_stride stride in pixels of dst image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x offset from the top-left corner of original draw area in sw_rotate_configure()
 * @param y y offset from the top-left corner of original draw area in sw_rotate_configure()
 * @param w width in pixels of current draw area
 * @param h height in pixels of current draw  area
 * @param cfg rotation config generated in func sw_rotate_configure()
 *
 * @retval N/A
 */
void sw_rotate_argb8565_over_rgb565(void *dst, const void *src,
		uint16_t dst_stride, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_rotate_config_t *cfg);

/*
 * @brief rotate an argb8888 image over rgb565 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_stride stride in pixels of dst image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x offset from the top-left corner of original draw area in sw_rotate_configure()
 * @param y y offset from the top-left corner of original draw area in sw_rotate_configure()
 * @param w width in pixels of current draw area
 * @param h height in pixels of current draw  area
 * @param cfg rotation config generated in func sw_rotate_configure()
 *
 * @retval N/A
 */
void sw_rotate_argb8888_over_rgb565(void *dst, const void *src,
		uint16_t dst_stride, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_rotate_config_t *cfg);

/*
 * @brief rotate an argb8888 image over argb8888 image
 *
 * @param dst address of dst image
 * @param src address of src image
 * @param dst_stride stride in pixels of dst image
 * @param src_w width in pixels of src image
 * @param src_h height in pixels of src image
 * @param x x offset from the top-left corner of original draw area in sw_rotate_configure()
 * @param y y offset from the top-left corner of original draw area in sw_rotate_configure()
 * @param w width in pixels of current draw area
 * @param h height in pixels of current draw  area
 * @param cfg rotation config generated in func sw_rotate_configure()
 *
 * @retval N/A
 */
void sw_rotate_argb8888_over_argb8888(void *dst, const void *src,
		uint16_t dst_stride, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_rotate_config_t *cfg);

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* ZEPHYR_FRAMEWORK_INCLUDE_DISPLAY_SW_ROTATE_H_ */
