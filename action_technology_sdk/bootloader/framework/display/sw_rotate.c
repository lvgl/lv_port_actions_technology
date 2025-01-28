#include <display/sw_draw.h>
#include <display/sw_rotate.h>
#ifdef CONFIG_GUI_API_BROM
#include <brom_interface.h>
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

/*
 * compute x range in pixels inside the src image
 *
 * The routine will update the range value [*x_min, *x_max].
 *
 * @param x_min address of minimum range value in pixels
 * @param x_max address of maximum range value in pixels
 * @param img_w source image width in pixels
 * @param img_h source image height in pixels
 * @param start_x X coord of start point in fixedpoint-16, corresponding to the original *x_min value
 * @param start_y Y coord of start point in fixedpoint-16, corresponding to the original *x_min value
 * @param dx_x X coord of point delta in x direction in fixedpoint-16
 * @param dx_y Y coord of point delta in x direction in fixedpoint-16
 *
 * @return N/A
 */
static inline void sw_rotate_compoute_x_range(
		int32_t *x_min, int32_t *x_max, int16_t img_w, int16_t img_h,
		int32_t start_x, int32_t start_y, int32_t dx_x, int32_t dx_y)
{
	const int32_t img_w_m1 = FIXEDPOINT16(img_w - 1);
	const int32_t img_h_m1 = FIXEDPOINT16(img_h - 1);
	int x_1, x_2;

	/* FIXME: the compiler seems to divide towards to zero. */
	if (dx_x != 0) {
		/*
			* floor(Δx * dx_x + start_x) >= FIXEDPOINT16(0)
			* ceil(Δx * dx_x + start_x) <= img_w_m1
			*/
		if (dx_x > 0) {
			x_1 = (FIXEDPOINT16(0) - start_x + dx_x - 1) / dx_x;
			x_2 = (img_w_m1 - start_x) / dx_x;
		} else {
			x_2 = (FIXEDPOINT16(0) - start_x) / dx_x;
			x_1 = (img_w_m1 - start_x + dx_x + 1) / dx_x;
		}

		*x_min = MAX(*x_min, x_1);
		*x_max = MIN(*x_max, x_2);
	} else if (start_x < FIXEDPOINT16(0) || start_x > img_w_m1) {
		*x_max = *x_min - 1;
		return;
	}

	if (dx_y != 0) {
		/*
			* floor(Δy * dx_y + start_y) >= FIXEDPOINT16(0)
			* ceil(Δy * dx_y + start_y) <= img_h_m1
			*/
		if (dx_y > 0) {
			x_1 = (FIXEDPOINT16(0) - start_y + dx_y - 1) / dx_y;
			x_2 = (img_h_m1 - start_y) / dx_y;
		} else {
			x_2 = (FIXEDPOINT16(0) - start_y) / dx_y;
			x_1 = (img_h_m1 - start_y + dx_y + 1) / dx_y;
		}

		*x_min = MAX(*x_min, x_1);
		*x_max = MIN(*x_max, x_2);
	} else if (start_y < FIXEDPOINT16(0) || start_y > img_h_m1) {
		*x_max = *x_min - 1;
		return;
	}
}

void sw_rotate_configure(int16_t draw_x, int16_t draw_y, int16_t img_x, int16_t img_y,
		int16_t pivot_x, int16_t pivot_y, uint16_t angle, sw_rotate_config_t *cfg)
{
#ifdef CONFIG_GUI_API_BROM
	p_brom_libgui_api->p_sw_rotate_configure(
			draw_x, draw_y, img_x, img_y, pivot_x, pivot_y, angle, cfg);
#else
	uint16_t reverse_angle = 3600 - angle;

	/* destination coordinate system */
	cfg->src_coord_x0 = PX_FIXEDPOINT16(draw_x);
	cfg->src_coord_y0 = PX_FIXEDPOINT16(draw_y);
	cfg->src_coord_dx_ax = FIXEDPOINT16(1);
	cfg->src_coord_dy_ax = FIXEDPOINT16(0);
	cfg->src_coord_dx_ay = FIXEDPOINT16(0);
	cfg->src_coord_dy_ay = FIXEDPOINT16(1);

	/* map to the source coordinate system */
	sw_rotate_point32(&cfg->src_coord_x0, &cfg->src_coord_y0,
			cfg->src_coord_x0, cfg->src_coord_y0,
			FIXEDPOINT16(pivot_x), FIXEDPOINT16(pivot_y), reverse_angle);
	sw_rotate_point32(&cfg->src_coord_dx_ax, &cfg->src_coord_dy_ax,
			cfg->src_coord_dx_ax, cfg->src_coord_dy_ax, 0, 0, reverse_angle);
	sw_rotate_point32(&cfg->src_coord_dx_ay, &cfg->src_coord_dy_ay,
			cfg->src_coord_dx_ay, cfg->src_coord_dy_ay, 0, 0, reverse_angle);

	/* map to the source pixel coordinate system */
	cfg->src_coord_x0 -= PX_FIXEDPOINT16(img_x);
	cfg->src_coord_y0 -= PX_FIXEDPOINT16(img_y);
#endif /* CONFIG_GUI_API_BROM */
}

void sw_rotate_rgb565_over_rgb565(void *dst, const void *src,
		uint16_t dst_stride, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_rotate_config_t *cfg)
{
#ifdef CONFIG_GUI_API_BROM
	p_brom_libgui_api->p_sw_rotate_rgb565_over_rgb565(
			dst, src, dst_stride, src_w, src_h, x, y, w, h, cfg);
#else
	uint16_t * dst16 = dst;
	uint16_t src_pitch = src_w * 2;
	uint16_t src_bytes_per_pixel = 2;
	int32_t src_coord_x = cfg->src_coord_x0 +
			y * cfg->src_coord_dx_ay + x * cfg->src_coord_dx_ax;
	int32_t src_coord_y = cfg->src_coord_y0 +
			y * cfg->src_coord_dy_ay + x * cfg->src_coord_dy_ax;

	for (int j = h; j > 0; j--) {
		int32_t p_x = src_coord_x;
		int32_t p_y = src_coord_y;
		uint16_t *tmp_dst = dst16;

		int x1 = 0, x2 = w - 1;

		sw_rotate_compoute_x_range(&x1, &x2, src_w, src_h,
				p_x, p_y, cfg->src_coord_dx_ax, cfg->src_coord_dy_ax);
		if (x1 > x2) {
			goto next_line;
		} else if (x1 > 0) {
			p_x += cfg->src_coord_dx_ax * x1;
			p_y += cfg->src_coord_dy_ax * x1;
			tmp_dst += x1;
		}

		for (int i = x2 - x1; i >= 0; i--) {
			int x = FLOOR_FIXEDPOINT16(p_x);
			int y = FLOOR_FIXEDPOINT16(p_y);
			int x_frac = p_x - FIXEDPOINT16(x);
			int y_frac = p_y - FIXEDPOINT16(y);
			uint8_t *src1 = (uint8_t *)src + y * src_pitch + x * src_bytes_per_pixel;
			uint8_t *src2 = src1 + src_bytes_per_pixel;
			uint8_t *src3 = src1 + src_pitch;
			uint8_t *src4 = src2 + src_pitch;

			*tmp_dst = bilinear_rgb565_fast_m6(*(uint16_t*)src1,
					*(uint16_t*)src2, *(uint16_t*)src3, *(uint16_t*)src4,
					x_frac >> 10, y_frac >> 10, 6);

			p_x += cfg->src_coord_dx_ax;
			p_y += cfg->src_coord_dy_ax;
			tmp_dst += 1;
		}

next_line:
		src_coord_x += cfg->src_coord_dx_ay;
		src_coord_y += cfg->src_coord_dy_ay;
		dst16 += dst_stride;
	}
#endif /* CONFIG_GUI_API_BROM */
}

void sw_rotate_argb8565_over_rgb565(void *dst, const void *src,
		uint16_t dst_stride, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_rotate_config_t *cfg)
{
	uint16_t * dst16 = dst;
	uint16_t src_pitch = src_w * 3;
	uint16_t src_bytes_per_pixel = 3;
	int32_t src_coord_x = cfg->src_coord_x0 +
			y * cfg->src_coord_dx_ay + x * cfg->src_coord_dx_ax;
	int32_t src_coord_y = cfg->src_coord_y0 +
			y * cfg->src_coord_dy_ay + x * cfg->src_coord_dy_ax;

	for (int j = h; j > 0; j--) {
		int32_t p_x = src_coord_x;
		int32_t p_y = src_coord_y;
		uint16_t *tmp_dst = dst16;

		int x1 = 0, x2 = w - 1;

		sw_rotate_compoute_x_range(&x1, &x2, src_w, src_h,
				p_x, p_y, cfg->src_coord_dx_ax, cfg->src_coord_dy_ax);
		if (x1 > x2) {
			goto next_line;
		} else if (x1 > 0) {
			p_x += cfg->src_coord_dx_ax * x1;
			p_y += cfg->src_coord_dy_ax * x1;
			tmp_dst += x1;
		}

		for (int i = x2 - x1; i >= 0; i--) {
			int x = FLOOR_FIXEDPOINT16(p_x);
			int y = FLOOR_FIXEDPOINT16(p_y);
			int x_frac = p_x - FIXEDPOINT16(x);
			int y_frac = p_y - FIXEDPOINT16(y);
			uint8_t *src1 = (uint8_t *)src + y * src_pitch + x * src_bytes_per_pixel;
			uint8_t *src2 = src1 + src_bytes_per_pixel;
			uint8_t *src3 = src1 + src_pitch;
			uint8_t *src4 = src2 + src_pitch;

			uint16_t color = bilinear_rgb565_fast_m6(
					((uint16_t)src1[1] << 8) | src1[0],
					((uint16_t)src2[1] << 8) | src2[0],
					((uint16_t)src3[1] << 8) | src3[0],
					((uint16_t)src4[1] << 8) | src4[0],
					x_frac >> 10, y_frac >> 10, 6);

			*tmp_dst = blend_rgb565_over_rgb565(*tmp_dst, color, src1[2]);

			p_x += cfg->src_coord_dx_ax;
			p_y += cfg->src_coord_dy_ax;
			tmp_dst += 1;
		}

next_line:
		src_coord_x += cfg->src_coord_dx_ay;
		src_coord_y += cfg->src_coord_dy_ay;
		dst16 += dst_stride;
	}
}

void sw_rotate_argb8888_over_rgb565(void *dst, const void *src,
		uint16_t dst_stride, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_rotate_config_t *cfg)
{
#ifdef CONFIG_GUI_API_BROM
	p_brom_libgui_api->p_sw_rotate_argb8888_over_rgb565(
			dst, src, dst_stride, src_w, src_h, x, y, w, h, cfg);
#else
	uint16_t * dst16 = dst;
	uint16_t src_pitch = src_w * 4;
	uint16_t src_bytes_per_pixel = 4;
	int32_t src_coord_x = cfg->src_coord_x0 +
			y * cfg->src_coord_dx_ay + x * cfg->src_coord_dx_ax;
	int32_t src_coord_y = cfg->src_coord_y0 +
			y * cfg->src_coord_dy_ay + x * cfg->src_coord_dy_ax;

	for (int j = h; j > 0; j--) {
		int32_t p_x = src_coord_x;
		int32_t p_y = src_coord_y;
		uint16_t *tmp_dst = dst16;

		int x1 = 0, x2 = w - 1;

		sw_rotate_compoute_x_range(&x1, &x2, src_w, src_h,
				p_x, p_y, cfg->src_coord_dx_ax, cfg->src_coord_dy_ax);
		if (x1 > x2) {
			goto next_line;
		} else if (x1 > 0) {
			p_x += cfg->src_coord_dx_ax * x1;
			p_y += cfg->src_coord_dy_ax * x1;
			tmp_dst += x1;
		}

		for (int i = x2 - x1; i >= 0; i--) {
			int x = FLOOR_FIXEDPOINT16(p_x);
			int y = FLOOR_FIXEDPOINT16(p_y);
			int x_frac = p_x - FIXEDPOINT16(x);
			int y_frac = p_y - FIXEDPOINT16(y);
			uint8_t *src1 = (uint8_t *)src + y * src_pitch + x * src_bytes_per_pixel;
			uint8_t *src2 = src1 + src_bytes_per_pixel;
			uint8_t *src3 = src1 + src_pitch;
			uint8_t *src4 = src2 + src_pitch;

			uint32_t color = bilinear_argb8888_fast_m8(*(uint32_t*)src1,
					*(uint32_t*)src2, *(uint32_t*)src3, *(uint32_t*)src4,
					x_frac >> 8, y_frac >> 8, 8);

			*tmp_dst = blend_argb8888_over_rgb565(*tmp_dst, color);

			p_x += cfg->src_coord_dx_ax;
			p_y += cfg->src_coord_dy_ax;
			tmp_dst += 1;
		}

next_line:
		src_coord_x += cfg->src_coord_dx_ay;
		src_coord_y += cfg->src_coord_dy_ay;
		dst16 += dst_stride;
	}
#endif /* CONFIG_GUI_API_BROM */
}

void sw_rotate_argb8888_over_argb8888(void *dst, const void *src,
		uint16_t dst_stride, uint16_t src_w, uint16_t src_h,
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const sw_rotate_config_t *cfg)
{
#ifdef CONFIG_GUI_API_BROM
	p_brom_libgui_api->p_sw_rotate_argb8888_over_argb8888(
			dst, src, dst_stride, src_w, src_h, x, y, w, h, cfg);
#else
	uint32_t * dst32 = dst;
	uint16_t src_pitch = src_w * 4;
	uint16_t src_bytes_per_pixel = 4;
	int32_t src_coord_x = cfg->src_coord_x0 +
			y * cfg->src_coord_dx_ay + x * cfg->src_coord_dx_ax;
	int32_t src_coord_y = cfg->src_coord_y0 +
			y * cfg->src_coord_dy_ay + x * cfg->src_coord_dy_ax;

	for (int j = h; j > 0; j--) {
		int32_t p_x = src_coord_x;
		int32_t p_y = src_coord_y;
		uint32_t *tmp_dst = dst32;
		int x1 = 0, x2 = w - 1;

		sw_rotate_compoute_x_range(&x1, &x2, src_w, src_h,
				p_x, p_y, cfg->src_coord_dx_ax, cfg->src_coord_dy_ax);
		if (x1 > x2) {
			goto next_line;
		} else if (x1 > 0) {
			p_x += cfg->src_coord_dx_ax * x1;
			p_y += cfg->src_coord_dy_ax * x1;
			tmp_dst += x1;
		}

		for (int i = x2 - x1; i >= 0; i--) {
			int x = FLOOR_FIXEDPOINT16(p_x);
			int y = FLOOR_FIXEDPOINT16(p_y);
			int x_frac = p_x - FIXEDPOINT16(x);
			int y_frac = p_y - FIXEDPOINT16(y);
			uint8_t *src1 = (uint8_t *)src + y * src_pitch + x * src_bytes_per_pixel;
			uint8_t *src2 = src1 + src_bytes_per_pixel;
			uint8_t *src3 = src1 + src_pitch;
			uint8_t *src4 = src2 + src_pitch;

			uint32_t color = bilinear_argb8888_fast_m8(*(uint32_t*)src1,
					*(uint32_t*)src2, *(uint32_t*)src3, *(uint32_t*)src4,
					x_frac >> 8, y_frac >> 8, 8);

			*tmp_dst = blend_argb8888_over_argb8888(*tmp_dst, color);

			p_x += cfg->src_coord_dx_ax;
			p_y += cfg->src_coord_dy_ax;
			tmp_dst += 1;
		}

next_line:
		src_coord_x += cfg->src_coord_dx_ay;
		src_coord_y += cfg->src_coord_dy_ay;
		dst32 += dst_stride;
	}
#endif /* CONFIG_GUI_API_BROM */
}
