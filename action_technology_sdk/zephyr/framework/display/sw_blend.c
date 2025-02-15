/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <display/sw_draw.h>
#ifdef CONFIG_GUI_API_BROM
#  include <brom_interface.h>
#endif

#ifndef ARRAY_SIZE
#  define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#endif

/*Opacity mapping with bpp = 1*/
const uint8_t g_alpha1t8_opa_table[2] = {
	0, 255
};

/*Opacity mapping with bpp = 2*/
const uint8_t g_alpha2t8_opa_table[4] = {
	0, 85, 170, 255
};

/*Opacity mapping with bpp = 4*/
const uint8_t g_alpha4t8_opa_table[16] = {
	0,  17, 34, 51, 68, 85, 102, 119,
	136, 153, 170, 187, 204, 221, 238, 255
};

void sw_blend_color_over_rgb565(void *dst, uint32_t src_color,
		uint16_t dst_pitch, uint16_t w, uint16_t h)
{
	uint8_t *dst8 = dst;

	for (int j = h; j > 0; j--) {
		uint16_t *tmp_dst = (uint16_t *)dst8;

		for (int i = w; i > 0; i--) {
			*tmp_dst = blend_argb8888_over_rgb565(*tmp_dst, src_color);
			tmp_dst++;
		}

		dst8 += dst_pitch;
	}
}

void sw_blend_color_over_argb8565(void *dst, uint32_t src_color,
		uint16_t dst_pitch, uint16_t w, uint16_t h)
{
	uint8_t *dst8 = dst;

	for (int j = h; j > 0; j--) {
		uint8_t *tmp_dst = (uint8_t *)dst8;

		for (int i = w; i > 0; i--) {
			blend_argb8888_over_argb8565(tmp_dst, src_color);
			tmp_dst += 3;
		}

		dst8 += dst_pitch;
	}
}

void sw_blend_color_over_rgb888(void *dst, uint32_t src_color,
		uint16_t dst_pitch, uint16_t w, uint16_t h)
{
	uint8_t *dst8 = dst;

	for (int j = h; j > 0; j--) {
		uint8_t *tmp_dst = (uint8_t *)dst8;

		for (int i = w; i > 0; i--) {
			sw_color32_t col32 = {
				.a = 255,
				.r = tmp_dst[2],
				.g = tmp_dst[1],
				.b = tmp_dst[0],
			};

			col32.full = blend_argb8888_over_argb8888(col32.full, src_color);
			*tmp_dst++ = col32.b;
			*tmp_dst++ = col32.g;
			*tmp_dst++ = col32.r;
		}

		dst8 += dst_pitch;
	}
}

void sw_blend_color_over_argb8888(void *dst, uint32_t src_color,
		uint16_t dst_pitch, uint16_t w, uint16_t h)
{
	uint8_t *dst8 = dst;

	for (int j = h; j > 0; j--) {
		uint32_t *tmp_dst = (uint32_t *)dst8;

		for (int i = w; i > 0; i--) {
			*tmp_dst = blend_argb8888_over_argb8888(*tmp_dst, src_color);
			tmp_dst++;
		}

		dst8 += dst_pitch;
	}
}

void sw_blend_a8_over_rgb565(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
	uint8_t src_opa = (src_color >> 24);

#ifdef CONFIG_GUI_API_BROM_LEOPARD
	if (src_opa == 255) {
		p_brom_libgui_api->p_sw_blend_a8_over_rgb565(
				dst, src, src_color, dst_pitch, src_pitch, w, h);
		return;
	}
#endif /* CONFIG_GUI_API_BROM_LEOPARD */

	const uint8_t *src8 = src;
	uint8_t *dst8 = dst;

	src_color &= ~0xFF000000;

	if (src_opa == 255) {
		for (int j = h; j > 0; j--) {
			const uint8_t *tmp_src = src8;
			uint16_t *tmp_dst = (uint16_t *)dst8;

			for (int i = w; i > 0; i--) {
				uint32_t color32 = src_color | ((uint32_t)*tmp_src << 24);

				*tmp_dst = blend_argb8888_over_rgb565(*tmp_dst, color32);
				tmp_dst++;
				tmp_src++;
			}

			dst8 += dst_pitch;
			src8 += src_pitch;
		}
	} else {
		for (int j = h; j > 0; j--) {
			const uint8_t *tmp_src = src8;
			uint16_t *tmp_dst = (uint16_t *)dst8;

			for (int i = w; i > 0; i--) {
				uint8_t opa = (*tmp_src * src_opa) >> 8;
				uint32_t color32 = src_color | ((uint32_t)opa << 24);

				*tmp_dst = blend_argb8888_over_rgb565(*tmp_dst, color32);
				tmp_dst++;
				tmp_src++;
			}

			dst8 += dst_pitch;
			src8 += src_pitch;
		}
	}
}

void sw_blend_a8_over_argb8565(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
	uint8_t src_opa = (src_color >> 24);
	const uint8_t *src8 = src;
	uint8_t *dst8 = dst;

	src_color &= ~0xFF000000;

	if (src_opa == 255) {
		for (int j = h; j > 0; j--) {
			const uint8_t *tmp_src = src8;
			uint8_t *tmp_dst = (uint8_t *)dst8;

			for (int i = w; i > 0; i--) {
				uint32_t color32 = src_color | ((uint32_t)*tmp_src << 24);

				blend_argb8888_over_argb8565(tmp_dst, color32);
				tmp_dst += 3;
				tmp_src++;
			}

			dst8 += dst_pitch;
			src8 += src_pitch;
		}
	} else {
		for (int j = h; j > 0; j--) {
			const uint8_t *tmp_src = src8;
			uint8_t *tmp_dst = (uint8_t *)dst8;

			for (int i = w; i > 0; i--) {
				uint8_t opa = (*tmp_src * src_opa) >> 8;
				uint32_t color32 = src_color | ((uint32_t)opa << 24);

				blend_argb8888_over_argb8565(tmp_dst, color32);
				tmp_dst += 3;
				tmp_src++;
			}

			dst8 += dst_pitch;
			src8 += src_pitch;
		}
	}
}

void sw_blend_a8_over_rgb888(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
	uint8_t src_opa = (src_color >> 24);
	const uint8_t *src8 = src;
	uint8_t *dst8 = dst;

	src_color &= ~0xFF000000;

	if (src_opa == 255) {
		for (int j = h; j > 0; j--) {
			const uint8_t *tmp_src = src8;
			uint8_t *tmp_dst = (uint8_t *)dst8;

			for (int i = w; i > 0; i--) {
				uint32_t color32 = src_color | ((uint32_t)*tmp_src << 24);
				sw_color32_t col32 = {
					.a = 255,
					.r = tmp_dst[2],
					.g = tmp_dst[1],
					.b = tmp_dst[0],
				};

				col32.full = blend_argb8888_over_argb8888(col32.full, color32);
				*tmp_dst++ = col32.b;
				*tmp_dst++ = col32.g;
				*tmp_dst++ = col32.r;

				tmp_src++;
			}

			dst8 += dst_pitch;
			src8 += src_pitch;
		}
	} else {
		for (int j = h; j > 0; j--) {
			const uint8_t *tmp_src = src8;
			uint8_t *tmp_dst = (uint8_t *)dst8;

			for (int i = w; i > 0; i--) {
				uint8_t opa = (*tmp_src * src_opa) >> 8;
				uint32_t color32 = src_color | ((uint32_t)opa << 24);
				sw_color32_t col32 = {
					.a = 255,
					.r = tmp_dst[2],
					.g = tmp_dst[1],
					.b = tmp_dst[0],
				};

				col32.full = blend_argb8888_over_argb8888(col32.full, color32);
				*tmp_dst++ = col32.b;
				*tmp_dst++ = col32.g;
				*tmp_dst++ = col32.r;

				tmp_src++;
			}

			dst8 += dst_pitch;
			src8 += src_pitch;
		}
	}
}

void sw_blend_a8_over_argb8888(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
	uint8_t src_opa = (src_color >> 24);

#ifdef CONFIG_GUI_API_BROM_LEOPARD
	if (src_opa == 255) {
		p_brom_libgui_api->p_sw_blend_a8_over_argb8888(
				dst, src, src_color, dst_pitch, src_pitch, w, h);
		return;
	}
#endif /* CONFIG_GUI_API_BROM_LEOPARD */

	const uint8_t *src8 = src;
	uint8_t *dst8 = dst;

	src_color &= ~0xFF000000;

	if (src_opa == 255) {
		for (int j = h; j > 0; j--) {
			const uint8_t *tmp_src = src8;
			uint32_t *tmp_dst = (uint32_t *)dst8;

			for (int i = w; i > 0; i--) {
				uint32_t color32 = src_color | ((uint32_t)*tmp_src << 24);

				*tmp_dst = blend_argb8888_over_argb8888(*tmp_dst, color32);
				tmp_dst++;
				tmp_src++;
			}

			dst8 += dst_pitch;
			src8 += src_pitch;
		}
	} else {
		for (int j = h; j > 0; j--) {
			const uint8_t *tmp_src = src8;
			uint32_t *tmp_dst = (uint32_t *)dst8;

			for (int i = w; i > 0; i--) {
				uint8_t opa = (*tmp_src * src_opa) >> 8;
				uint32_t color32 = src_color | ((uint32_t)opa << 24);

				*tmp_dst = blend_argb8888_over_argb8888(*tmp_dst, color32);
				tmp_dst++;
				tmp_src++;
			}

			dst8 += dst_pitch;
			src8 += src_pitch;
		}
	}
}

void sw_blend_a4_over_rgb565(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h)
{
	uint8_t src_opa = (src_color >> 24);
	const uint8_t *opa_table = g_alpha4t8_opa_table;
	uint8_t tmp_opa_table[ARRAY_SIZE(g_alpha4t8_opa_table)];

	if (src_opa < 255) {
		for (int i = 0; i < ARRAY_SIZE(g_alpha4t8_opa_table); i++)
			tmp_opa_table[i] = (g_alpha4t8_opa_table[i] * src_opa) >> 8;

		opa_table = tmp_opa_table;
	}

	const uint8_t *src8 = src;
	const uint8_t src_bpp = 4;
	const uint8_t src_bmask = (1 << src_bpp) - 1;
	const uint8_t src_bofs_max = 8 - src_bpp;
	uint8_t *dst8 = dst;

	src_color &= ~0xFF000000;

	for (int j = h; j > 0; j--) {
		const uint8_t *tmp_src = src8;
		uint16_t *tmp_dst = (uint16_t *)dst8;
		uint8_t bpos = src_bofs_max - src_bofs;

		for (int i = w; i > 0; i--) {
			uint8_t opa = opa_table[(*tmp_src >> bpos) & src_bmask];
			uint32_t color32 = src_color | ((uint32_t)opa << 24);

			*tmp_dst = blend_argb8888_over_rgb565(*tmp_dst, color32);
			tmp_dst++;

			if (bpos == 0) {
				bpos = src_bofs_max;
				tmp_src++;
			} else {
				bpos -= src_bpp;
			}
		}

		dst8 += dst_pitch;
		src8 += src_pitch;
	}
}

void sw_blend_a4_over_argb8565(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h)
{
	uint8_t src_opa = (src_color >> 24);
	const uint8_t *opa_table = g_alpha4t8_opa_table;
	uint8_t tmp_opa_table[ARRAY_SIZE(g_alpha4t8_opa_table)];

	if (src_opa < 255) {
		for (int i = 0; i < ARRAY_SIZE(g_alpha4t8_opa_table); i++)
			tmp_opa_table[i] = (g_alpha4t8_opa_table[i] * src_opa) >> 8;

		opa_table = tmp_opa_table;
	}

	const uint8_t *src8 = src;
	const uint8_t src_bpp = 4;
	const uint8_t src_bmask = (1 << src_bpp) - 1;
	const uint8_t src_bofs_max = 8 - src_bpp;
	uint8_t *dst8 = dst;

	src_color &= ~0xFF000000;

	for (int j = h; j > 0; j--) {
		const uint8_t *tmp_src = src8;
		uint8_t *tmp_dst = (uint8_t *)dst8;
		uint8_t bpos = src_bofs_max - src_bofs;

		for (int i = w; i > 0; i--) {
			uint8_t opa = opa_table[(*tmp_src >> bpos) & src_bmask];
			uint32_t color32 = src_color | ((uint32_t)opa << 24);

			blend_argb8888_over_argb8565(tmp_dst, color32);
			tmp_dst += 3;

			if (bpos == 0) {
				bpos = src_bofs_max;
				tmp_src++;
			} else {
				bpos -= src_bpp;
			}
		}

		dst8 += dst_pitch;
		src8 += src_pitch;
	}
}

void sw_blend_a4_over_rgb888(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h)
{
	uint8_t src_opa = (src_color >> 24);
	const uint8_t *opa_table = g_alpha4t8_opa_table;
	uint8_t tmp_opa_table[ARRAY_SIZE(g_alpha4t8_opa_table)];

	if (src_opa < 255) {
		for (int i = 0; i < ARRAY_SIZE(g_alpha4t8_opa_table); i++)
			tmp_opa_table[i] = (g_alpha4t8_opa_table[i] * src_opa) >> 8;

		opa_table = tmp_opa_table;
	}

	const uint8_t *src8 = src;
	const uint8_t src_bpp = 4;
	const uint8_t src_bmask = (1 << src_bpp) - 1;
	const uint8_t src_bofs_max = 8 - src_bpp;
	uint8_t *dst8 = dst;

	src_color &= ~0xFF000000;

	for (int j = h; j > 0; j--) {
		const uint8_t *tmp_src = src8;
		uint8_t *tmp_dst = (uint8_t *)dst8;
		uint8_t bpos = src_bofs_max - src_bofs;

		for (int i = w; i > 0; i--) {
			uint8_t opa = opa_table[(*tmp_src >> bpos) & src_bmask];
			uint32_t color32 = src_color | ((uint32_t)opa << 24);
			sw_color32_t col32 = {
				.a = 255,
				.r = tmp_dst[2],
				.g = tmp_dst[1],
				.b = tmp_dst[0],
			};

			col32.full = blend_argb8888_over_argb8888(col32.full, color32);
			*tmp_dst++ = col32.b;
			*tmp_dst++ = col32.g;
			*tmp_dst++ = col32.r;

			if (bpos == 0) {
				bpos = src_bofs_max;
				tmp_src++;
			} else {
				bpos -= src_bpp;
			}
		}

		dst8 += dst_pitch;
		src8 += src_pitch;
	}
}

void sw_blend_a4_over_argb8888(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h)
{
	uint8_t src_opa = (src_color >> 24);
	const uint8_t *opa_table = g_alpha4t8_opa_table;
	uint8_t tmp_opa_table[ARRAY_SIZE(g_alpha4t8_opa_table)];

	if (src_opa < 255) {
		for (int i = 0; i < ARRAY_SIZE(g_alpha4t8_opa_table); i++)
			tmp_opa_table[i] = (g_alpha4t8_opa_table[i] * src_opa) >> 8;

		opa_table = tmp_opa_table;
	}

	const uint8_t *src8 = src;
	const uint8_t src_bpp = 4;
	const uint8_t src_bmask = (1 << src_bpp) - 1;
	const uint8_t src_bofs_max = 8 - src_bpp;
	uint8_t *dst8 = dst;

	src_color &= ~0xFF000000;

	for (int j = h; j > 0; j--) {
		const uint8_t *tmp_src = src8;
		uint32_t *tmp_dst = (uint32_t *)dst8;
		uint8_t bpos = src_bofs_max - src_bofs;

		for (int i = w; i > 0; i--) {
			uint8_t opa = opa_table[(*tmp_src >> bpos) & src_bmask];
			uint32_t color32 = src_color | ((uint32_t)opa << 24);

			*tmp_dst = blend_argb8888_over_argb8888(*tmp_dst, color32);
			tmp_dst++;

			if (bpos == 0) {
				bpos = src_bofs_max;
				tmp_src++;
			} else {
				bpos -= src_bpp;
			}
		}

		dst8 += dst_pitch;
		src8 += src_pitch;
	}
}

void sw_blend_a2_over_rgb565(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h)
{
	uint8_t src_opa = (src_color >> 24);
	const uint8_t *opa_table = g_alpha2t8_opa_table;
	uint8_t tmp_opa_table[ARRAY_SIZE(g_alpha2t8_opa_table)];

	if (src_opa < 255) {
		for (int i = 0; i < ARRAY_SIZE(g_alpha2t8_opa_table); i++)
			tmp_opa_table[i] = (g_alpha2t8_opa_table[i] * src_opa) >> 8;

		opa_table = tmp_opa_table;
	}

	const uint8_t *src8 = src;
	const uint8_t src_bpp = 2;
	const uint8_t src_bmask = (1 << src_bpp) - 1;
	const uint8_t src_bofs_max = 8 - src_bpp;
	uint8_t *dst8 = dst;

	src_color &= ~0xFF000000;

	for (int j = h; j > 0; j--) {
		const uint8_t *tmp_src = src8;
		uint16_t *tmp_dst = (uint16_t *)dst8;
		uint8_t bpos = src_bofs_max - src_bofs;

		for (int i = w; i > 0; i--) {
			uint8_t opa = opa_table[(*tmp_src >> bpos) & src_bmask];
			uint32_t color32 = src_color | ((uint32_t)opa << 24);

			*tmp_dst = blend_argb8888_over_rgb565(*tmp_dst, color32);
			tmp_dst++;

			if (bpos == 0) {
				bpos = src_bofs_max;
				tmp_src++;
			} else {
				bpos -= src_bpp;
			}
		}

		dst8 += dst_pitch;
		src8 += src_pitch;
	}
}

void sw_blend_a2_over_argb8565(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h)
{
	uint8_t src_opa = (src_color >> 24);
	const uint8_t *opa_table = g_alpha2t8_opa_table;
	uint8_t tmp_opa_table[ARRAY_SIZE(g_alpha2t8_opa_table)];

	if (src_opa < 255) {
		for (int i = 0; i < ARRAY_SIZE(g_alpha2t8_opa_table); i++)
			tmp_opa_table[i] = (g_alpha2t8_opa_table[i] * src_opa) >> 8;

		opa_table = tmp_opa_table;
	}

	const uint8_t *src8 = src;
	const uint8_t src_bpp = 2;
	const uint8_t src_bmask = (1 << src_bpp) - 1;
	const uint8_t src_bofs_max = 8 - src_bpp;
	uint8_t *dst8 = dst;

	src_color &= ~0xFF000000;

	for (int j = h; j > 0; j--) {
		const uint8_t *tmp_src = src8;
		uint8_t *tmp_dst = (uint8_t *)dst8;
		uint8_t bpos = src_bofs_max - src_bofs;

		for (int i = w; i > 0; i--) {
			uint8_t opa = opa_table[(*tmp_src >> bpos) & src_bmask];
			uint32_t color32 = src_color | ((uint32_t)opa << 24);

			blend_argb8888_over_argb8565(tmp_dst, color32);
			tmp_dst += 3;

			if (bpos == 0) {
				bpos = src_bofs_max;
				tmp_src++;
			} else {
				bpos -= src_bpp;
			}
		}

		dst8 += dst_pitch;
		src8 += src_pitch;
	}
}

void sw_blend_a2_over_rgb888(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h)
{
	uint8_t src_opa = (src_color >> 24);
	const uint8_t *opa_table = g_alpha2t8_opa_table;
	uint8_t tmp_opa_table[ARRAY_SIZE(g_alpha2t8_opa_table)];

	if (src_opa < 255) {
		for (int i = 0; i < ARRAY_SIZE(g_alpha2t8_opa_table); i++)
			tmp_opa_table[i] = (g_alpha2t8_opa_table[i] * src_opa) >> 8;

		opa_table = tmp_opa_table;
	}

	const uint8_t *src8 = src;
	const uint8_t src_bpp = 2;
	const uint8_t src_bmask = (1 << src_bpp) - 1;
	const uint8_t src_bofs_max = 8 - src_bpp;
	uint8_t *dst8 = dst;

	src_color &= ~0xFF000000;

	for (int j = h; j > 0; j--) {
		const uint8_t *tmp_src = src8;
		uint8_t *tmp_dst = (uint8_t *)dst8;
		uint8_t bpos = src_bofs_max - src_bofs;

		for (int i = w; i > 0; i--) {
			uint8_t opa = opa_table[(*tmp_src >> bpos) & src_bmask];
			uint32_t color32 = src_color | ((uint32_t)opa << 24);
			sw_color32_t col32 = {
				.a = 255,
				.r = tmp_dst[2],
				.g = tmp_dst[1],
				.b = tmp_dst[0],
			};

			col32.full = blend_argb8888_over_argb8888(col32.full, color32);
			*tmp_dst++ = col32.b;
			*tmp_dst++ = col32.g;
			*tmp_dst++ = col32.r;

			if (bpos == 0) {
				bpos = src_bofs_max;
				tmp_src++;
			} else {
				bpos -= src_bpp;
			}
		}

		dst8 += dst_pitch;
		src8 += src_pitch;
	}
}

void sw_blend_a2_over_argb8888(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h)
{
	uint8_t src_opa = (src_color >> 24);
	const uint8_t *opa_table = g_alpha2t8_opa_table;
	uint8_t tmp_opa_table[ARRAY_SIZE(g_alpha2t8_opa_table)];

	if (src_opa < 255) {
		for (int i = 0; i < ARRAY_SIZE(g_alpha2t8_opa_table); i++)
			tmp_opa_table[i] = (g_alpha2t8_opa_table[i] * src_opa) >> 8;

		opa_table = tmp_opa_table;
	}

	const uint8_t *src8 = src;
	const uint8_t src_bpp = 2;
	const uint8_t src_bmask = (1 << src_bpp) - 1;
	const uint8_t src_bofs_max = 8 - src_bpp;
	uint8_t *dst8 = dst;

	src_color &= ~0xFF000000;

	for (int j = h; j > 0; j--) {
		const uint8_t *tmp_src = src8;
		uint32_t *tmp_dst = (uint32_t *)dst8;
		uint8_t bpos = src_bofs_max - src_bofs;

		for (int i = w; i > 0; i--) {
			uint8_t opa = opa_table[(*tmp_src >> bpos) & src_bmask];
			uint32_t color32 = src_color | ((uint32_t)opa << 24);

			*tmp_dst = blend_argb8888_over_argb8888(*tmp_dst, color32);
			tmp_dst++;

			if (bpos == 0) {
				bpos = src_bofs_max;
				tmp_src++;
			} else {
				bpos -= src_bpp;
			}
		}

		dst8 += dst_pitch;
		src8 += src_pitch;
	}
}

void sw_blend_a1_over_rgb565(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h)
{
	const uint8_t *src8 = src;
	uint8_t *dst8 = dst;

	for (int j = h; j > 0; j--) {
		const uint8_t *tmp_src = src8;
		uint16_t *tmp_dst = (uint16_t *)dst8;
		uint8_t bmask = 0x80 >> src_bofs;

		for (int i = w; i > 0; i--) {
			uint8_t opa = *tmp_src & bmask;
			if (opa > 0) {
				uint32_t color32 = src_color;
				*tmp_dst = blend_argb8888_over_rgb565(*tmp_dst, color32);
			}

			tmp_dst++;

			bmask >>= 1;
			if (bmask == 0) {
				bmask = 0x80;
				tmp_src++;
			}
		}

		dst8 += dst_pitch;
		src8 += src_pitch;
	}
}

void sw_blend_a1_over_argb8565(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h)
{
	const uint8_t *src8 = src;
	uint8_t *dst8 = dst;

	for (int j = h; j > 0; j--) {
		const uint8_t *tmp_src = src8;
		uint8_t *tmp_dst = (uint8_t *)dst8;
		uint8_t bmask = 0x80 >> src_bofs;

		for (int i = w; i > 0; i--) {
			uint8_t opa = *tmp_src & bmask;
			if (opa > 0) {
				uint32_t color32 = src_color;
				blend_argb8888_over_argb8565(tmp_dst, color32);
			}

			tmp_dst += 3;

			bmask >>= 1;
			if (bmask == 0) {
				bmask = 0x80;
				tmp_src++;
			}
		}

		dst8 += dst_pitch;
		src8 += src_pitch;
	}
}

void sw_blend_a1_over_rgb888(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h)
{
	const uint8_t *src8 = src;
	uint8_t *dst8 = dst;

	for (int j = h; j > 0; j--) {
		const uint8_t *tmp_src = src8;
		uint8_t *tmp_dst = (uint8_t *)dst8;
		uint8_t bmask = 0x80 >> src_bofs;

		for (int i = w; i > 0; i--) {
			uint8_t opa = *tmp_src & bmask;
			if (opa > 0) {
				uint32_t color32 = src_color;
				sw_color32_t col32 = {
					.a = 255,
					.r = tmp_dst[2],
					.g = tmp_dst[1],
					.b = tmp_dst[0],
				};
				col32.full = blend_argb8888_over_argb8888(col32.full, color32);
				tmp_dst[0] = col32.b;
				tmp_dst[1] = col32.g;
				tmp_dst[2] = col32.r;
			}

			tmp_dst += 3;

			bmask >>= 1;
			if (bmask == 0) {
				bmask = 0x80;
				tmp_src++;
			}
		}

		dst8 += dst_pitch;
		src8 += src_pitch;
	}
}

void sw_blend_a1_over_argb8888(void *dst, const void *src, uint32_t src_color,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h)
{
	const uint8_t *src8 = src;
	uint8_t *dst8 = dst;

	for (int j = h; j > 0; j--) {
		const uint8_t *tmp_src = src8;
		uint32_t *tmp_dst = (uint32_t *)dst8;
		uint8_t bmask = 0x80 >> src_bofs;

		for (int i = w; i > 0; i--) {
			uint8_t opa = *tmp_src & bmask;
			if (opa > 0) {
				uint32_t color32 = src_color;
				*tmp_dst = blend_argb8888_over_argb8888(*tmp_dst, color32);
			}

			tmp_dst++;

			bmask >>= 1;
			if (bmask == 0) {
				bmask = 0x80;
				tmp_src++;
			}
		}

		dst8 += dst_pitch;
		src8 += src_pitch;
	}
}

void sw_blend_index8_over_rgb565(void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
	const uint8_t *src8 = (uint8_t *)src;
	uint8_t *dst8 = dst;

	for (int j = h; j > 0; j--) {
		const uint8_t *tmp_src = (uint8_t *)src8;
		uint16_t *tmp_dst = (uint16_t *)dst8;

		for (int i = w; i > 0; i--) {
			*tmp_dst = blend_argb8888_over_rgb565(*tmp_dst, src_clut[*tmp_src]);
			tmp_dst++;
			tmp_src++;
		}

		src8 += src_pitch;
		dst8 += dst_pitch;
	}
}

void sw_blend_index8_over_argb8565(void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
	const uint8_t *src8 = (uint8_t *)src;
	uint8_t *dst8 = dst;

	for (int j = h; j > 0; j--) {
		const uint8_t *tmp_src = (uint8_t *)src8;
		uint8_t *tmp_dst = (uint8_t *)dst8;

		for (int i = w; i > 0; i--) {
			blend_argb8888_over_argb8565(tmp_dst, src_clut[*tmp_src]);
			tmp_dst += 3;
			tmp_src++;
		}

		src8 += src_pitch;
		dst8 += dst_pitch;
	}
}

void sw_blend_index8_over_rgb888(void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
	const uint8_t *src8 = (uint8_t *)src;
	uint8_t *dst8 = dst;

	for (int j = h; j > 0; j--) {
		const uint8_t *tmp_src = (uint8_t *)src8;
		uint8_t *tmp_dst = (uint8_t *)dst8;

		for (int i = w; i > 0; i--) {
			sw_color32_t col32 = {
				.a = 255,
				.r = tmp_dst[2],
				.g = tmp_dst[1],
				.b = tmp_dst[0],
			};
			col32.full = blend_argb8888_over_argb8888(col32.full, src_clut[*tmp_src]);
			*tmp_dst++ = col32.b;
			*tmp_dst++ = col32.g;
			*tmp_dst++ = col32.r;

			tmp_src++;
		}

		src8 += src_pitch;
		dst8 += dst_pitch;
	}
}

void sw_blend_index8_over_argb8888(void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
	const uint8_t *src8 = (uint8_t *)src;
	uint8_t *dst8 = dst;

	for (int j = h; j > 0; j--) {
		const uint8_t *tmp_src = (uint8_t *)src8;
		uint32_t *tmp_dst = (uint32_t *)dst8;

		for (int i = w; i > 0; i--) {
			*tmp_dst = blend_argb8888_over_argb8888(*tmp_dst, src_clut[*tmp_src]);
			tmp_dst++;
			tmp_src++;
		}

		src8 += src_pitch;
		dst8 += dst_pitch;
	}
}

static void sw_blend_index124_over_rgb565(
		void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint8_t src_bpp, uint16_t w, uint16_t h)
{
	const uint8_t *src8 = (uint8_t *)src;
	const uint8_t src_bmask = (1 << src_bpp) - 1;
	const uint8_t src_bofs_max = 8 - src_bpp;
	uint8_t *dst8 = dst;

	for (int j = h; j > 0; j--) {
		const uint8_t *tmp_src = (uint8_t *)src8;
		uint16_t *tmp_dst = (uint16_t *)dst8;
		uint8_t src_bpos = src_bofs_max - src_bofs;

		for (int i = w; i > 0; i--) {
			uint8_t idx = (*tmp_src >> src_bpos) & src_bmask;
			*tmp_dst = blend_argb8888_over_rgb565(*tmp_dst, src_clut[idx]);
			tmp_dst++;

			if (src_bpos == 0) {
				src_bpos = src_bofs_max;
				tmp_src++;
			} else {
				src_bpos -= src_bpp;
			}
		}

		src8 += src_pitch;
		dst8 += dst_pitch;
	}
}

static void sw_blend_index124_over_argb8565(
		void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint8_t src_bpp, uint16_t w, uint16_t h)
{
	const uint8_t *src8 = (uint8_t *)src;
	const uint8_t src_bmask = (1 << src_bpp) - 1;
	const uint8_t src_bofs_max = 8 - src_bpp;
	uint8_t *dst8 = dst;

	for (int j = h; j > 0; j--) {
		const uint8_t *tmp_src = (uint8_t *)src8;
		uint8_t *tmp_dst = (uint8_t *)dst8;
		uint8_t src_bpos = src_bofs_max - src_bofs;

		for (int i = w; i > 0; i--) {
			uint8_t idx = (*tmp_src >> src_bpos) & src_bmask;
			blend_argb8888_over_argb8565(tmp_dst, src_clut[idx]);
			tmp_dst += 3;

			if (src_bpos == 0) {
				src_bpos = src_bofs_max;
				tmp_src++;
			} else {
				src_bpos -= src_bpp;
			}
		}

		src8 += src_pitch;
		dst8 += dst_pitch;
	}
}

static void sw_blend_index124_over_rgb888(
		void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint8_t src_bpp, uint16_t w, uint16_t h)
{
	const uint8_t *src8 = (uint8_t *)src;
	const uint8_t src_bmask = (1 << src_bpp) - 1;
	const uint8_t src_bofs_max = 8 - src_bpp;
	uint8_t *dst8 = dst;

	for (int j = h; j > 0; j--) {
		const uint8_t *tmp_src = (uint8_t *)src8;
		uint8_t *tmp_dst = (uint8_t *)dst8;
		uint8_t src_bpos = src_bofs_max - src_bofs;

		for (int i = w; i > 0; i--) {
			uint8_t idx = (*tmp_src >> src_bpos) & src_bmask;
			sw_color32_t col32 = {
				.a = 255,
				.r = tmp_dst[2],
				.g = tmp_dst[1],
				.b = tmp_dst[0],
			};
			col32.full = blend_argb8888_over_argb8888(col32.full, src_clut[idx]);
			*tmp_dst++ = col32.b;
			*tmp_dst++ = col32.g;
			*tmp_dst++ = col32.r;

			if (src_bpos == 0) {
				src_bpos = src_bofs_max;
				tmp_src++;
			} else {
				src_bpos -= src_bpp;
			}
		}

		src8 += src_pitch;
		dst8 += dst_pitch;
	}
}

static void sw_blend_index124_over_argb8888(
		void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint8_t src_bpp, uint16_t w, uint16_t h)
{
	const uint8_t *src8 = (uint8_t *)src;
	const uint8_t src_bmask = (1 << src_bpp) - 1;
	const uint8_t src_bofs_max = 8 - src_bpp;
	uint8_t *dst8 = dst;

	for (int j = h; j > 0; j--) {
		const uint8_t *tmp_src = (uint8_t *)src8;
		uint32_t *tmp_dst = (uint32_t *)dst8;
		uint8_t src_bpos = src_bofs_max - src_bofs;

		for (int i = w; i > 0; i--) {
			uint8_t idx = (*tmp_src >> src_bpos) & src_bmask;
			*tmp_dst = blend_argb8888_over_argb8888(*tmp_dst, src_clut[idx]);
			tmp_dst++;

			if (src_bpos == 0) {
				src_bpos = src_bofs_max;
				tmp_src++;
			} else {
				src_bpos -= src_bpp;
			}
		}

		src8 += src_pitch;
		dst8 += dst_pitch;
	}
}

void sw_blend_index4_over_rgb565(
		void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h)
{
	sw_blend_index124_over_rgb565(dst, src, src_clut, dst_pitch, src_pitch,
			src_bofs, 4, w, h);
}

void sw_blend_index4_over_argb8565(
		void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h)
{
	sw_blend_index124_over_argb8565(dst, src, src_clut, dst_pitch, src_pitch,
			src_bofs, 4, w, h);
}

void sw_blend_index4_over_rgb888(
		void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h)
{
	sw_blend_index124_over_rgb888(dst, src, src_clut, dst_pitch, src_pitch,
			src_bofs, 4, w, h);
}

void sw_blend_index4_over_argb8888(
		void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h)
{
	sw_blend_index124_over_argb8888(dst, src, src_clut, dst_pitch, src_pitch,
			src_bofs, 4, w, h);
}

void sw_blend_index2_over_rgb565(
		void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h)
{
	sw_blend_index124_over_rgb565(dst, src, src_clut, dst_pitch, src_pitch,
			src_bofs, 2, w, h);
}

void sw_blend_index2_over_argb8565(
		void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h)
{
	sw_blend_index124_over_argb8565(dst, src, src_clut, dst_pitch, src_pitch,
			src_bofs, 2, w, h);
}

void sw_blend_index2_over_rgb888(
		void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h)
{
	sw_blend_index124_over_rgb888(dst, src, src_clut, dst_pitch, src_pitch,
			src_bofs, 2, w, h);
}

void sw_blend_index2_over_argb8888(
		void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h)
{
	sw_blend_index124_over_argb8888(dst, src, src_clut, dst_pitch, src_pitch,
			src_bofs, 2, w, h);
}

void sw_blend_index1_over_rgb565(
		void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h)
{
	sw_blend_index124_over_rgb565(dst, src, src_clut, dst_pitch, src_pitch,
			src_bofs, 1, w, h);
}

void sw_blend_index1_over_argb8565(
		void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h)
{
	sw_blend_index124_over_argb8565(dst, src, src_clut, dst_pitch, src_pitch,
			src_bofs, 1, w, h);
}

void sw_blend_index1_over_rgb888(
		void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h)
{
	sw_blend_index124_over_rgb888(dst, src, src_clut, dst_pitch, src_pitch,
			src_bofs, 1, w, h);
}

void sw_blend_index1_over_argb8888(
		void *dst, const void *src, const uint32_t *src_clut,
		uint16_t dst_pitch, uint16_t src_pitch, uint8_t src_bofs,
		uint16_t w, uint16_t h)
{
	sw_blend_index124_over_argb8888(dst, src, src_clut, dst_pitch, src_pitch,
			src_bofs, 1, w, h);
}

void sw_blend_rgb565a8_over_rgb565(void *dst, const void *src, const void *src_opa,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_opa_pitch, uint16_t w, uint16_t h)
{
	const uint8_t *src8 = src;
	const uint8_t *src_opa8 = src_opa;
	uint8_t *dst8 = dst;

	for (int j = h; j > 0; j--) {
		const uint16_t *tmp_src = (uint16_t *)src8;
		const uint8_t *tmp_src_opa = src_opa8;
		uint16_t *tmp_dst = (uint16_t *)dst8;

		for (int i = w; i > 0; i--) {
			sw_color16a8_t col24 = {
				.rgb = *tmp_src,
				.a = *tmp_src_opa,
			};
			*tmp_dst = blend_argb8565_over_rgb565(*tmp_dst, (uint8_t *)&col24);
			tmp_dst++;
			tmp_src++;
			tmp_src_opa++;
		}

		dst8 += dst_pitch;
		src8 += src_pitch;
		src_opa8 += src_opa_pitch;
	}
}

void sw_blend_rgb565a8_over_argb8565(void *dst, const void *src, const void *src_opa,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_opa_pitch, uint16_t w, uint16_t h)
{
	const uint8_t *src8 = src;
	const uint8_t *src_opa8 = src_opa;
	uint8_t *dst8 = dst;

	for (int j = h; j > 0; j--) {
		const uint16_t *tmp_src = (uint16_t *)src8;
		const uint8_t *tmp_src_opa = src_opa8;
		uint8_t *tmp_dst = (uint8_t *)dst8;

		for (int i = w; i > 0; i--) {
			sw_color16a8_t col24 = {
				.rgb = *tmp_src,
				.a = *tmp_src_opa,
			};
			blend_argb8565_over_argb8565(tmp_dst, (uint8_t *)&col24);
			tmp_dst += 3;
			tmp_src++;
			tmp_src_opa++;
		}

		dst8 += dst_pitch;
		src8 += src_pitch;
		src_opa8 += src_opa_pitch;
	}
}

void sw_blend_rgb565a8_over_rgb888(void *dst, const void *src, const void *src_opa,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_opa_pitch, uint16_t w, uint16_t h)
{
	const uint8_t *src8 = src;
	const uint8_t *src_opa8 = src_opa;
	uint8_t *dst8 = dst;

	for (int j = h; j > 0; j--) {
		const uint16_t *tmp_src = (uint16_t *)src8;
		const uint8_t *tmp_src_opa = src_opa8;
		uint8_t *tmp_dst = (uint8_t *)dst8;

		for (int i = w; i > 0; i--) {
			sw_color16a8_t col24 = {
				.rgb = *tmp_src,
				.a = *tmp_src_opa,
			};
			sw_color32_t col32 = {
				.a = 255,
				.r = tmp_dst[2],
				.g = tmp_dst[1],
				.b = tmp_dst[0],
			};

			col32.full = blend_argb8565_over_argb8888(col32.full, (uint8_t *)&col24);
			*tmp_dst++ = col32.b;
			*tmp_dst++ = col32.g;
			*tmp_dst++ = col32.r;

			tmp_src++;
			tmp_src_opa++;
		}

		dst8 += dst_pitch;
		src8 += src_pitch;
		src_opa8 += src_opa_pitch;
	}
}

void sw_blend_rgb565a8_over_argb8888(void *dst, const void *src, const void *src_opa,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_opa_pitch, uint16_t w, uint16_t h)
{
	const uint8_t *src8 = src;
	const uint8_t *src_opa8 = src_opa;
	uint8_t *dst8 = dst;

	for (int j = h; j > 0; j--) {
		const uint16_t *tmp_src = (uint16_t *)src8;
		const uint8_t *tmp_src_opa = src_opa8;
		uint32_t *tmp_dst = (uint32_t *)dst8;

		for (int i = w; i > 0; i--) {
			sw_color16a8_t col24 = {
				.rgb = *tmp_src,
				.a = *tmp_src_opa,
			};
			*tmp_dst = blend_argb8565_over_argb8888(*tmp_dst, (uint8_t *)&col24);
			tmp_dst++;
			tmp_src++;
			tmp_src_opa++;
		}

		dst8 += dst_pitch;
		src8 += src_pitch;
		src_opa8 += src_opa_pitch;
	}
}

void sw_blend_argb8565_over_rgb565(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
#ifdef CONFIG_GUI_API_BROM_LARK
	if (src_pitch % 3 == 0) {
		p_brom_libgui_api->p_sw_blend_argb8565_over_rgb565(
				dst, src, dst_pitch / 2, src_pitch / 3, w, h);
		return;
	}
#endif

#ifdef CONFIG_GUI_API_BROM_LEOPARD
	p_brom_libgui_api->p_sw_blend_argb8565_over_rgb565(
			dst, src, dst_pitch, src_pitch, w, h);
#else
	const uint8_t *src8 = src;
	uint8_t *dst8 = dst;
	uint8_t src_x_step = 3;

	for (int j = h; j > 0; j--) {
		const uint8_t *tmp_src = src8;
		uint16_t *tmp_dst = (uint16_t *)dst8;

		for (int i = w; i > 0; i--) {
			*tmp_dst = blend_argb8565_over_rgb565(*tmp_dst, tmp_src);
			tmp_dst++;
			tmp_src += src_x_step;
		}

		dst8 += dst_pitch;
		src8 += src_pitch;
	}
#endif /* CONFIG_GUI_API_BROM_LEOPARD */
}

void sw_blend_argb8565_over_argb8565(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
	const uint8_t *src8 = src;
	uint8_t *dst8 = dst;
	uint8_t src_x_step = 3;

	for (int j = h; j > 0; j--) {
		const uint8_t *tmp_src = src8;
		uint8_t *tmp_dst = (uint8_t *)dst8;

		for (int i = w; i > 0; i--) {
			blend_argb8565_over_argb8565(tmp_dst, tmp_src);
			tmp_dst += 3;
			tmp_src += src_x_step;
		}

		dst8 += dst_pitch;
		src8 += src_pitch;
	}
}

void sw_blend_argb8565_over_rgb888(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
	const uint8_t *src8 = src;
	uint8_t *dst8 = dst;
	uint8_t src_x_step = 3;

	for (int j = h; j > 0; j--) {
		const uint8_t *tmp_src = src8;
		uint8_t *tmp_dst = (uint8_t *)dst8;

		for (int i = w; i > 0; i--) {
			sw_color32_t col32 = {
				.a = 255,
				.r = tmp_dst[2],
				.g = tmp_dst[1],
				.b = tmp_dst[0],
			};

			col32.full = blend_argb8565_over_argb8888(col32.full, tmp_src);
			*tmp_dst++ = col32.b;
			*tmp_dst++ = col32.g;
			*tmp_dst++ = col32.r;

			tmp_src += src_x_step;
		}

		dst8 += dst_pitch;
		src8 += src_pitch;
	}
}

void sw_blend_argb8565_over_argb8888(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
#ifdef CONFIG_GUI_API_BROM_LEOPARD
	p_brom_libgui_api->p_sw_blend_argb8565_over_argb8888(
			dst, src, dst_pitch, src_pitch, w, h);
#else
	const uint8_t *src8 = src;
	uint8_t *dst8 = dst;
	uint8_t src_x_step = 3;

	for (int j = h; j > 0; j--) {
		const uint8_t *tmp_src = src8;
		uint32_t *tmp_dst = (uint32_t *)dst8;

		for (int i = w; i > 0; i--) {
			*tmp_dst = blend_argb8565_over_argb8888(*tmp_dst, tmp_src);
			tmp_dst++;
			tmp_src += src_x_step;
		}

		dst8 += dst_pitch;
		src8 += src_pitch;
	}
#endif /* CONFIG_GUI_API_BROM_LEOPARD */
}

void sw_blend_argb6666_over_rgb565(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
#ifdef CONFIG_GUI_API_BROM_LEOPARD
	p_brom_libgui_api->p_sw_blend_argb6666_over_rgb565(
			dst, src, dst_pitch, src_pitch, w, h);
#else
	const uint8_t *src8 = src;
	uint8_t *dst8 = dst;
	uint8_t src_x_step = 3;

	for (int j = h; j > 0; j--) {
		const uint8_t *tmp_src = src8;
		uint16_t *tmp_dst = (uint16_t *)dst8;

		for (int i = w; i > 0; i--) {
			*tmp_dst = blend_argb6666_over_rgb565(*tmp_dst, tmp_src);
			tmp_dst++;
			tmp_src += src_x_step;
		}

		dst8 += dst_pitch;
		src8 += src_pitch;
	}
#endif /* CONFIG_GUI_API_BROM_LEOPARD */
}

void sw_blend_argb6666_over_argb8565(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
	const uint8_t *src8 = src;
	uint8_t *dst8 = dst;
	uint8_t src_x_step = 3;

	for (int j = h; j > 0; j--) {
		const uint8_t *tmp_src = src8;
		uint8_t *tmp_dst = (uint8_t *)dst8;

		for (int i = w; i > 0; i--) {
			blend_argb6666_over_argb8565(tmp_dst, tmp_src);
			tmp_dst += 3;
			tmp_src += src_x_step;
		}

		dst8 += dst_pitch;
		src8 += src_pitch;
	}
}

void sw_blend_argb6666_over_rgb888(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
	const uint8_t *src8 = src;
	uint8_t *dst8 = dst;
	uint8_t src_x_step = 3;

	for (int j = h; j > 0; j--) {
		const uint8_t *tmp_src = src8;
		uint8_t *tmp_dst = (uint8_t *)dst8;

		for (int i = w; i > 0; i--) {
			sw_color32_t col32 = {
				.a = 255,
				.r = tmp_dst[2],
				.g = tmp_dst[1],
				.b = tmp_dst[0],
			};
			col32.full = blend_argb6666_over_argb8888(col32.full, tmp_src);
			*tmp_dst++ = col32.b;
			*tmp_dst++ = col32.g;
			*tmp_dst++ = col32.r;

			tmp_src += src_x_step;
		}

		dst8 += dst_pitch;
		src8 += src_pitch;
	}
}

void sw_blend_argb6666_over_argb8888(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
#ifdef CONFIG_GUI_API_BROM_LEOPARD
	p_brom_libgui_api->p_sw_blend_argb6666_over_argb8888(
			dst, src, dst_pitch, src_pitch, w, h);
#else
	const uint8_t *src8 = src;
	uint8_t *dst8 = dst;
	uint8_t src_x_step = 3;

	for (int j = h; j > 0; j--) {
		const uint8_t *tmp_src = src8;
		uint32_t *tmp_dst = (uint32_t *)dst8;

		for (int i = w; i > 0; i--) {
			*tmp_dst = blend_argb6666_over_argb8888(*tmp_dst, tmp_src);
			tmp_dst++;
			tmp_src += src_x_step;
		}

		dst8 += dst_pitch;
		src8 += src_pitch;
	}
#endif /* CONFIG_GUI_API_BROM_LEOPARD */
}

void sw_blend_argb1555_over_rgb565(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
	const uint8_t *src8 = src;
	uint8_t *dst8 = dst;

	for (int j = h; j > 0; j--) {
		const uint16_t *tmp_src = (uint16_t *)src8;
		uint16_t *tmp_dst = (uint16_t *)dst8;

		for (int i = w; i > 0; i--) {
			*tmp_dst = blend_argb1555_over_rgb565(*tmp_dst, *tmp_src);
			tmp_dst++;
			tmp_src++;
		}

		dst8 += dst_pitch;
		src8 += src_pitch;
	}
}

void sw_blend_argb1555_over_argb8565(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
	const uint8_t *src8 = src;
	uint8_t *dst8 = dst;

	for (int j = h; j > 0; j--) {
		const uint16_t *tmp_src = (uint16_t *)src8;
		uint8_t *tmp_dst = (uint8_t *)dst8;

		for (int i = w; i > 0; i--) {
			blend_argb1555_over_argb8565(tmp_dst, *tmp_src);
			tmp_dst += 3;
			tmp_src++;
		}

		dst8 += dst_pitch;
		src8 += src_pitch;
	}
}

void sw_blend_argb1555_over_rgb888(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
	const uint8_t *src8 = src;
	uint8_t *dst8 = dst;

	for (int j = h; j > 0; j--) {
		const uint16_t *tmp_src = (uint16_t *)src8;
		uint8_t *tmp_dst = (uint8_t *)dst8;

		for (int i = w; i > 0; i--) {
			sw_color32_t col32 = {
				.a = 255,
				.r = tmp_dst[2],
				.g = tmp_dst[1],
				.b = tmp_dst[0],
			};
			col32.full = blend_argb1555_over_argb8888(col32.full, *tmp_src);
			*tmp_dst++ = col32.b;
			*tmp_dst++ = col32.g;
			*tmp_dst++ = col32.r;

			tmp_src++;
		}

		dst8 += dst_pitch;
		src8 += src_pitch;
	}
}

void sw_blend_argb1555_over_argb8888(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
	const uint8_t *src8 = src;
	uint8_t *dst8 = dst;

	for (int j = h; j > 0; j--) {
		const uint16_t *tmp_src = (uint16_t *)src8;
		uint32_t *tmp_dst = (uint32_t *)dst8;

		for (int i = w; i > 0; i--) {
			*tmp_dst = blend_argb1555_over_argb8888(*tmp_dst, *tmp_src);
			tmp_dst++;
			tmp_src++;
		}

		dst8 += dst_pitch;
		src8 += src_pitch;
	}
}

void sw_blend_argb8888_over_rgb565(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
#ifdef CONFIG_GUI_API_BROM_LARK
	p_brom_libgui_api->p_sw_blend_argb8888_over_rgb565(
			dst, src, dst_pitch / 2, src_pitch / 4, w, h);
#elif defined(CONFIG_GUI_API_BROM_LEOPARD)
	p_brom_libgui_api->p_sw_blend_argb8888_over_rgb565(
			dst, src, dst_pitch, src_pitch, w, h);
#else
	const uint8_t *src8 = src;
	uint8_t *dst8 = dst;

	for (int j = h; j > 0; j--) {
		const uint32_t *tmp_src = (uint32_t *)src8;
		uint16_t *tmp_dst = (uint16_t *)dst8;

		for (int i = w; i > 0; i--) {
			*tmp_dst = blend_argb8888_over_rgb565(*tmp_dst, *tmp_src);
			tmp_dst++;
			tmp_src++;
		}

		src8 += src_pitch;
		dst8 += dst_pitch;
	}
#endif /* CONFIG_GUI_API_BROM_LEOPARD */
}

void sw_blend_argb8888_over_argb8565(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
	const uint8_t *src8 = src;
	uint8_t *dst8 = dst;

	for (int j = h; j > 0; j--) {
		const uint32_t *tmp_src = (uint32_t *)src8;
		uint8_t *tmp_dst = (uint8_t *)dst8;

		for (int i = w; i > 0; i--) {
			blend_argb8888_over_argb8565(tmp_dst, *tmp_src);
			tmp_dst += 3;
			tmp_src++;
		}

		src8 += src_pitch;
		dst8 += dst_pitch;
	}
}

void sw_blend_argb8888_over_rgb888(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
	const uint8_t *src8 = src;
	uint8_t *dst8 = dst;

	for (int j = h; j > 0; j--) {
		const uint32_t *tmp_src = (uint32_t *)src8;
		uint8_t *tmp_dst = (uint8_t *)dst8;

		for (int i = w; i > 0; i--) {
			sw_color32_t col32 = {
				.a = 255,
				.r = tmp_dst[2],
				.g = tmp_dst[1],
				.b = tmp_dst[0],
			};
			col32.full = blend_argb8888_over_argb8888(col32.full, *tmp_src);
			*tmp_dst++ = col32.b;
			*tmp_dst++ = col32.g;
			*tmp_dst++ = col32.r;

			tmp_src++;
		}

		src8 += src_pitch;
		dst8 += dst_pitch;
	}
}

void sw_blend_argb8888_over_argb8888(void *dst, const void *src,
		uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
#ifdef CONFIG_GUI_API_BROM_LARK
	p_brom_libgui_api->p_sw_blend_argb8888_over_argb8888(
			dst, src, dst_pitch / 4, src_pitch / 4, w, h);
#elif defined(CONFIG_GUI_API_BROM_LEOPARD)
	p_brom_libgui_api->p_sw_blend_argb8888_over_argb8888(
			dst, src, dst_pitch, src_pitch, w, h);
#else
	const uint8_t *src8 = src;
	uint8_t *dst8 = dst;

	for (int j = h; j > 0; j--) {
		const uint32_t *tmp_src = (uint32_t *)src8;
		uint32_t *tmp_dst = (uint32_t *)dst8;

		for (int i = w; i > 0; i--) {
			*tmp_dst = blend_argb8888_over_argb8888(*tmp_dst, *tmp_src);
			tmp_dst++;
			tmp_src++;
		}

		src8 += src_pitch;
		dst8 += dst_pitch;
	}
#endif /* CONFIG_GUI_API_BROM_LEOPARD */
}
