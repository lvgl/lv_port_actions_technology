/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file ui_math.c
 */

#ifdef CONFIG_LVGL
#  include <lvgl/lvgl.h> /* reuse the math function of LVGL */
#endif
#include <ui_math.h>

float ui_map_f(float x, float min_in, float max_in, float min_out, float max_out)
{
    if(max_in >= min_in && x >= max_in) return max_out;
    if(max_in >= min_in && x <= min_in) return min_out;

    if(max_in <= min_in && x <= max_in) return max_out;
    if(max_in <= min_in && x >= min_in) return min_out;

    /**
     * The equation should be:
     *   ((x - min_in) * delta_out) / delta in) + min_out
     * To avoid rounding error reorder the operations:
     *   (x - min_in) * (delta_out / delta_min) + min_out
     */

    float delta_in = max_in - min_in;
    float delta_out = max_out - min_out;

    return ((x - min_in) * delta_out) / delta_in + min_out;
}

int32_t ui_map(int32_t x, int32_t min_in, int32_t max_in, int32_t min_out, int32_t max_out)
{
#ifdef CONFIG_LVGL
	return lv_map(x, min_in, max_in, min_out, max_out);
#else
	if (x >= max_in) return max_out;
	if (x <= min_in) return min_out;

	/**
	 * The equation should be:
	 *   ((x - min_in) * delta_out) / delta in) + min_out
	 * To avoid rounding error reorder the operations:
	 *   (x - min_in) * (delta_out / delta_min) + min_out
	 */
	int32_t delta_in = max_in - min_in;
	int32_t delta_out = max_out - min_out;

	return ((x - min_in) * delta_out) / delta_in + min_out;
#endif
}

uint32_t ui_bezier3(uint32_t t, uint32_t u0, uint32_t u1, uint32_t u2, uint32_t u3)
{
#ifdef CONFIG_LVGL
	/* must make sure UI_BEZIER_VAL_MAX is equal to LV_BEZIER_VAL_MAX */
	return lv_bezier3(t, u0, u1, u2, u3);
#else
	uint32_t t_rem  = 1024 - t;
	uint32_t t_rem2 = (t_rem * t_rem) >> 10;
	uint32_t t_rem3 = (t_rem2 * t_rem) >> 10;
	uint32_t t2     = (t * t) >> 10;
	uint32_t t3     = (t2 * t) >> 10;

	uint32_t v1 = (t_rem3 * u0) >> 10;
	uint32_t v2 = (3 * t_rem2 * t * u1) >> 20;
	uint32_t v3 = (3 * t_rem * t2 * u2) >> 20;
	uint32_t v4 = (t3 * u3) >> 10;

	return v1 + v2 + v3 + v4;
#endif
}
