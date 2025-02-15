/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <display/sw_draw.h>
#include <display/display_hal.h>

extern const uint8_t g_alpha1t8_opa_table[2];
extern const uint8_t g_alpha2t8_opa_table[4];
extern const uint8_t g_alpha4t8_opa_table[16];

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

void sw_convert_a124_to_a8(void * dst, const void *src,
            uint16_t dst_pitch, uint16_t src_pitch_bits,
            uint16_t src_bofs, uint8_t src_bpp, uint16_t w, uint16_t h)
{
	uint8_t * dst8 = dst;
	const uint8_t * src8 = src;
	uint16_t src_bit_max = 8 - src_bpp;
	uint8_t src_bit_mask = (1u << src_bpp) - 1;
	const uint8_t *opa_table = (src_bpp == 4) ? g_alpha4t8_opa_table :
				(src_bpp == 2 ? g_alpha2t8_opa_table : g_alpha1t8_opa_table);

	for (uint16_t i = h; i > 0; i--) {
		uint8_t *tmp_dst8 = dst8;
		const uint8_t *tmp_src8 = src8;
		uint16_t src_bit_be = src_bit_max - src_bofs;
		uint8_t px = *tmp_src8;

		for (uint16_t j = w; j > 0; j--) {
			/*Load the pixel's opacity into the mask*/
			uint8_t letter_px = (px >> src_bit_be) & src_bit_mask;
			*tmp_dst8++ = opa_table[letter_px];

			/*Go to the next column*/
			if (src_bit_be > 0) {
				src_bit_be -= src_bpp;
			} else {
				src_bit_be = src_bit_max;
				px = *(++tmp_src8);
			}
		}

		dst8 += dst_pitch;
		src_bofs += src_pitch_bits;
		src8 += (src_bofs >> 3);
		src_bofs &= 0x7;
	}
}
