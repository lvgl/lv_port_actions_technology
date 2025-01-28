/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Utilities of sw math API
 */

#ifndef ZEPHYR_FRAMEWORK_INCLUDE_DISPLAY_SW_MATH_H_
#define ZEPHYR_FRAMEWORK_INCLUDE_DISPLAY_SW_MATH_H_

#include <stddef.h>
#include <stdint.h>

/**
 * @defgroup display-util Display Utilities
 * @ingroup display_libraries
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/* fixedpoint with 16-bit fraction */
typedef int32_t fixedpoint16_t;

/* convert integer to fixedpoint-1 */
#define FIXEDPOINT1(x)        to_fixedpoint(x, 1)
#define FIXEDPOINT1_0_5       (FIXEDPOINT1(1) >> 1)
/* convert pixel coordinate to fixedpoint-1 coordinate */
#define PX_FIXEDPOINT1(x)     (FIXEDPOINT1(x) + (FIXEDPOINT1(1) >> 1))
/* convert fixedpoint-1 to integer */
#define FLOOR_FIXEDPOINT1(x)  floor_fixedpoint(x, 1)
#define ROUND_FIXEDPOINT1(x)  round_fixedpoint(x, 1)

/* convert integer to fixedpoint-12 */
#define FIXEDPOINT12(x)        to_fixedpoint(x, 12)
#define FIXEDPOINT12_0_5       (FIXEDPOINT12(1) >> 1)
/* convert pixel coordinate to fixedpoint-12 coordinate */
#define PX_FIXEDPOINT12(x)     (FIXEDPOINT12(x) + (FIXEDPOINT12(1) >> 1))
/* convert fixedpoint-12 to integer */
#define FLOOR_FIXEDPOINT12(x)  floor_fixedpoint(x, 12)
#define ROUND_FIXEDPOINT12(x)  round_fixedpoint(x, 12)

/* convert integer to fixedpoint-16 */
#define FIXEDPOINT16(x)        to_fixedpoint(x, 16)
#define FIXEDPOINT16_0_5       (FIXEDPOINT16(1) >> 1)
/* convert pixel coordinate to fixedpoint-16 coordinate */
#define PX_FIXEDPOINT16(x)     (FIXEDPOINT16(x) + (FIXEDPOINT16(1) >> 1))
/* convert fixedpoint-16 to integer */
#define FLOOR_FIXEDPOINT16(x)  floor_fixedpoint(x, 16)
#define ROUND_FIXEDPOINT16(x)  round_fixedpoint(x, 16)

/* Convert to fixedpoint */
static inline int32_t to_fixedpoint(int32_t x, int fraction_bits)
{
	return x << fraction_bits;
}

/* Round fixedpoint towards nearest integer (round to positive infinity). */
static inline int32_t round_fixedpoint(int32_t x, int fraction_bits)
{
	return (x + (1 << (fraction_bits - 1))) >> fraction_bits;
}

/* Round fixedpoint towards negative infinity. */
static inline int32_t floor_fixedpoint(int32_t x, int fraction_bits)
{
	return x >> fraction_bits;
}

/* Round fixedpoint towards positive infinity. */
static inline int32_t ceil_fixedpoint(int32_t x, int fraction_bits)
{
	return (x + ((1 << fraction_bits) - 1)) >> fraction_bits;
}

/* Round fixedpoint towards zero. */
static inline int32_t fix_fixedpoint(int32_t x, int fraction_bits)
{
	if (x >= 0)
		return floor_fixedpoint(x, fraction_bits);
	else
		return ceil_fixedpoint(x, fraction_bits);
}

/* Integer part of fixedpoint */
static inline int32_t fixedpoint_int(int32_t x, int fraction_bits)
{
	//return to_fixedpoint(floor_fixedpoint(x, fraction_bits));
	return x & ~((1 << fraction_bits) - 1);
}

/* Fraction part of fixedpoint */
static inline int32_t fixedpoint_frac(int32_t x, int fraction_bits)
{
	return x - fixedpoint_int(x, fraction_bits);
}

/*
 * @brief compute cos
 *
 * @param angle angle in 0.1 degree [0, 3600]
 *
 * @retval result in fixedpoint-30
 */
int32_t sw_cos30(uint16_t angle);

/*
 * @brief compute sin
 *
 * @param angle angle in 0.1 degree [0, 3600]
 *
 * @retval result in fixedpoint-30
 */
int32_t sw_sin30(uint16_t angle);

/*
 * @brief compute cos
 *
 * @param angle angle in 0.1 degree [0, 3600]
 *
 * @retval result in fixedpoint-14
 */
int16_t sw_cos14(uint16_t angle);

/*
 * @brief compute sin
 *
 * @param angle angle in 0.1 degree [0, 3600]
 *
 * @retval result in fixedpoint-14
 */
int16_t sw_sin14(uint16_t angle);

/*
 * @brief rotate a 32-bit point
 *
 * @param dest_x x coord of the point after rotation
 * @param dest_y y coord of the point after rotation
 * @param src_x x coord of the point before rotation
 * @param src_y y coord of the point image before rotation
 * @param pivot_x x coord of rotation pivot
 * @param pivot_y y coord of rotation pivot
 * @param angle rotation angle in 0.1 degree [0, 3600]
 *
 * @retval N/A
 */
void sw_rotate_point32(int32_t *dest_x, int32_t *dest_y,
		int32_t src_x, int32_t src_y, int32_t pivot_x, int32_t pivot_y, uint16_t angle);

/*
 * @brief rotate a 16-bit point
 *
 * @param dest_x x coord of the point after rotation
 * @param dest_y y coord of the point after rotation
 * @param src_x x coord of the point before rotation
 * @param src_y y coord of the point image before rotation
 * @param pivot_x x coord of rotation pivot
 * @param pivot_y y coord of rotation pivot
 * @param angle rotation angle in 0.1 degree [0, 3600]
 *
 * @retval N/A
 */
void sw_rotate_point16(int16_t *dest_x, int16_t *dest_y,
		int16_t src_x, int16_t src_y, int16_t pivot_x, int16_t pivot_y, uint16_t angle);

/*
 * @brief rotate a 16-bit area
 *
 * [x1, x2] x [y1, y2] define the pixel coordinates of a area
 *
 * @param dest_x1 x1 coord of rect after rotation
 * @param dest_y1 y1 coord of rect after rotation
 * @param dest_x2 x2 coord of rect after rotation
 * @param dest_y2 y2 coord of rect after rotation
 * @param src_x1 x1 coord of rect before rotation
 * @param src_y1 y1 coord of rect before rotation
 * @param src_x2 x2 coord of rect before rotation
 * @param src_y2 y2 coord of rect before rotation
 * @param pivot_x x coord of rotation pivot
 * @param pivot_y y coord of rotation pivot
 * @param angle rotation angle in 0.1 degree [0, 3600]
 *
 * @retval N/A
 */
void sw_rotate_area16(
		int16_t *dest_x1, int16_t *dest_y1, int16_t *dest_x2, int16_t *dest_y2,
		int16_t src_x1, int16_t src_y1, int16_t src_x2, int16_t src_y2,
		int16_t pivot_x, int16_t pivot_y, uint16_t angle);

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* ZEPHYR_FRAMEWORK_INCLUDE_DISPLAY_SW_MATH_H_ */
