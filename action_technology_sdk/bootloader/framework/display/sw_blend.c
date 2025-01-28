#include <display/sw_draw.h>
#ifdef CONFIG_GUI_API_BROM
#include <brom_interface.h>
#endif

void sw_blend_argb8565_over_rgb565(void *dst, const void *src,
		uint16_t dst_stride, uint16_t src_stride, uint16_t w, uint16_t h)
{
#ifdef CONFIG_GUI_API_BROM
	p_brom_libgui_api->p_sw_blend_argb8565_over_rgb565(
			dst, src, dst_stride, src_stride, w, h);
#else
	const uint8_t *src8 = src;
	uint16_t *dst16 = dst;
	uint16_t src_y_step = src_stride * 3;
	uint16_t src_x_step = 3;

	for (int j = h; j > 0; j--) {
		const uint8_t *tmp_src8 = src8;
		uint16_t *tmp_dst = dst16;

		for (int i = w; i > 0; i--) {
			*tmp_dst = blend_rgb565_over_rgb565(*tmp_dst,
					((uint16_t)tmp_src8[1] << 8) | tmp_src8[0], tmp_src8[2]);
			tmp_dst++;
			tmp_src8 += src_x_step;
		}

		dst16 += dst_stride;
		src8 += src_y_step;
	}
#endif /* CONFIG_GUI_API_BROM */
}

void sw_blend_argb6666_over_rgb565(void *dst, const void *src,
		uint16_t dst_stride, uint16_t src_stride, uint16_t w, uint16_t h)
{
	const uint8_t *src8 = src;
	uint16_t *dst16 = dst;
	uint16_t src_y_step = src_stride * 3;
	uint16_t src_x_step = 3;

	for (int j = h; j > 0; j--) {
		const uint8_t *tmp_src8 = src8;
		uint16_t *tmp_dst = dst16;

		for (int i = w; i > 0; i--) {
			*tmp_dst = blend_argb6666_over_rgb565(*tmp_dst, tmp_src8);
			tmp_dst++;
			tmp_src8 += src_x_step;
		}

		dst16 += dst_stride;
		src8 += src_y_step;
	}
}

void sw_blend_argb8888_over_rgb565(void *dst, const void *src,
		uint16_t dst_stride, uint16_t src_stride, uint16_t w, uint16_t h)
{
#ifdef CONFIG_GUI_API_BROM
	p_brom_libgui_api->p_sw_blend_argb8888_over_rgb565(
			dst, src, dst_stride, src_stride, w, h);
#else
	const uint32_t *src32 = src;
	uint16_t *dst16 = dst;

	for (int j = h; j > 0; j--) {
		const uint32_t *tmp_src = src32;
		uint16_t *tmp_dst = dst16;

		for (int i = w; i > 0; i--) {
			*tmp_dst = blend_argb8888_over_rgb565(*tmp_dst, *tmp_src);
			tmp_dst++;
			tmp_src++;
		}

		src32 += src_stride;
		dst16 += dst_stride;
	}
#endif /* CONFIG_GUI_API_BROM */
}

void sw_blend_argb8888_over_argb8888(void *dst, const void *src,
		uint16_t dst_stride, uint16_t src_stride, uint16_t w, uint16_t h)
{
#ifdef CONFIG_GUI_API_BROM
	p_brom_libgui_api->p_sw_blend_argb8888_over_argb8888(
			dst, src, dst_stride, src_stride, w, h);
#else
	const uint32_t *src32 = src;
	uint32_t *dst32 = dst;

	for (int j = h; j > 0; j--) {
		const uint32_t *tmp_src = src32;
		uint32_t *tmp_dst = dst32;

		for (int i = w; i > 0; i--) {
			*tmp_dst = blend_argb8888_over_argb8888(*tmp_dst, *tmp_src);
			tmp_dst++;
			tmp_src++;
		}

		src32 += src_stride;
		dst32 += dst_stride;
	}
#endif /* CONFIG_GUI_API_BROM */
}
