/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <display/sw_draw.h>
#include <display/display_hal.h>

static inline void cvt_buf_argb8888_to_bgr888(void * dest, const void * src, uint32_t len)
{
	uint8_t *dest8 = dest;
	const uint8_t *src8 = src;

	for (; len > 0; len--) {
		*dest8++ = src8[2];
		*dest8++ = src8[1];
		*dest8++ = src8[0];
		src8 += 4;
	}
}

int sw_convert_color_buffer(void * dest_buf, uint32_t dest_cf, const void * src_buf, uint32_t src_cf, uint32_t len)
{
	if (dest_cf == src_cf) {
		uint8_t px_size = hal_pixel_format_get_bits_per_pixel(dest_cf);
		memcpy(dest_buf, src_buf, px_size * len);
		return 0;
	}

	if (dest_cf == HAL_PIXEL_FORMAT_BGR_888) {
		if (src_cf == HAL_PIXEL_FORMAT_ARGB_8888 || src_cf == HAL_PIXEL_FORMAT_XRGB_8888) {
			cvt_buf_argb8888_to_bgr888(dest_buf, src_buf, len);
			return 0;
		}
	}

	return -ENOSYS;
}
