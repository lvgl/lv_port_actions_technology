/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file view manager interface
 */

#ifndef FRAMEWORK_DISPLAY_INCLUDE_UI_MATH_H_
#define FRAMEWORK_DISPLAY_INCLUDE_UI_MATH_H_

/**
 * @defgroup ui_manager_apis app ui Manager APIs
 * @ingroup system_apis
 * @{
 */

#include <stdint.h>
#include <display/sw_math.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Value of @p x aligned down to the previous multiple of @p align,
 *        which must be a power of 2.
 */
#define UI_ALIGN(x, align) (((x) + (align) - 1) & ~((align) - 1))

/**
 * @brief Value of @p x rounded up to the next multiple of @p align,
 *        which must be a power of 2.
 */
#define UI_ROUND_UP(x, align)                                   \
	(((unsigned long)(x) + ((unsigned long)(align) - 1)) & \
	 ~((unsigned long)(align) - 1))

/**
 * @brief Value of @p x rounded down to the previous multiple of @p align,
 *        which must be a power of 2.
 */
#define UI_ROUND_DOWN(x, align)                                 \
	((unsigned long)(x) & ~((unsigned long)(align) - 1))

/** @brief Unsigned integer with bit position @p n set */
#define UI_BIT(n)  (1UL << (n))

/**
 * @brief Bit mask with bits 0 through <tt>n-1</tt> (inclusive) set,
 * or 0 if @p n is 0.
 */
#define UI_BIT_MASK(n) (UI_BIT(n) - 1)

/** @brief The smaller value between @p a and @p b. */
#define UI_MIN(a, b) ((a) < (b) ? (a) : (b))

/** @brief The larger value between @p a and @p b. */
#define UI_MAX(a, b) ((a) > (b) ? (a) : (b))

/** @brief Absolute value of @p x. */
#define UI_ABS(x) ((x) >= 0 ? (x) : (-(x)))

/** @brief Clamp a value to a given range. */
#define UI_CLAMP(val, low, high) (((val) <= (low)) ? (low) : UI_MIN(val, high))

#define UI_BEZIER_VAL_MAX 1024 /**< Max time in Bezier functions (not [0..1] to use integers)*/
#define UI_BEZIER_VAL_SHIFT 10 /**< log2(UI_BEZIER_VAL_MAX): used to normalize up scaled values*/

/**
 * Get the linear mapped of a number given an input and output range
 *
 * @param x integer which mapped value should be calculated
 * @param min_in min input range
 * @param max_in max input range
 * @param min_out max output range
 * @param max_out max output range
 *
 * @return the mapped number
 */
int32_t ui_map(int32_t x, int32_t min_in, int32_t max_in, int32_t min_out, int32_t max_out);
float ui_map_f(float x, float min_in, float max_in, float min_out, float max_out);

/**
 * Calculate a value of a Cubic Bezier function.
 * @param t time in range of [0..UI_BEZIER_VAL_MAX]
 * @param u0 start values in range of [0..UI_BEZIER_VAL_MAX]
 * @param u1 control value 1 values in range of [0..UI_BEZIER_VAL_MAX]
 * @param u2 control value 2 in range of [0..UI_BEZIER_VAL_MAX]
 * @param u3 end values in range of [0..UI_BEZIER_VAL_MAX]
 *
 * @return the value calculated from the given parameters in range of [0..UI_BEZIER_VAL_MAX]
 */
uint32_t ui_bezier3(uint32_t t, uint32_t u0, uint32_t u1, uint32_t u2, uint32_t u3);

#ifdef __cplusplus
}
#endif

/**
 * @} end defgroup system_apis
 */
#endif /* FRAMEWORK_DISPLAY_INCLUDE_UI_MATH_H_ */
