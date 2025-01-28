#include <display/sw_draw.h>
#include <display/sw_rotate.h>
#include <assert.h>
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
static inline void sw_transform_compoute_x_range(
    int32_t* x_min, int32_t* x_max, int16_t img_w, int16_t img_h,
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
        }
        else {
            x_2 = (FIXEDPOINT16(0) - start_x) / dx_x;
            x_1 = (img_w_m1 - start_x + dx_x + 1) / dx_x;
        }

        *x_min = MAX(*x_min, x_1);
        *x_max = MIN(*x_max, x_2);
    }
    else if (start_x < FIXEDPOINT16(0) || start_x > img_w_m1) {
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
        }
        else {
            x_2 = (FIXEDPOINT16(0) - start_y) / dx_y;
            x_1 = (img_h_m1 - start_y + dx_y + 1) / dx_y;
        }

        *x_min = MAX(*x_min, x_1);
        *x_max = MIN(*x_max, x_2);
    }
    else if (start_y < FIXEDPOINT16(0) || start_y > img_h_m1) {
        *x_max = *x_min - 1;
        return;
    }
}

void sw_transform_config(int16_t img_x, int16_t img_y,
    int16_t pivot_x, int16_t pivot_y, uint16_t angle,
    uint16_t scale_x, uint16_t scale_y, uint16_t scale_bits,
    sw_matrix_t* matrix)
{
    const uint16_t scale_factor = (1 << scale_bits);
    uint16_t revert_scale_x;
    uint16_t revert_scale_y;
    uint16_t revert_angle;

    assert(angle <= 3600 && scale_x > 0 && scale_y > 0);

    revert_scale_x = (scale_x > 0) ? (scale_factor * scale_factor / scale_x) : 1;
    revert_scale_y = (scale_x == scale_y) ? revert_scale_x :
        ((scale_y > 0) ? (scale_factor * scale_factor / scale_y) : 1);

#ifdef CONFIG_GUI_API_BROM_LEOPARD
    if (revert_scale_x == revert_scale_y) {
        p_brom_libgui_api->p_sw_transform_config(img_x, img_y,
            pivot_x, pivot_y, angle, revert_scale_x, revert_scale_y, scale_bits, matrix);
        return;
    }
#endif

    revert_angle = 3600 - angle;

    /* coordinates in the destination coordinate system */
    matrix->tx = PX_FIXEDPOINT16(0);
    matrix->ty = PX_FIXEDPOINT16(0);
    matrix->sx = FIXEDPOINT16(1);
    matrix->shy = FIXEDPOINT16(0);
    matrix->shx = FIXEDPOINT16(0);
    matrix->sy = FIXEDPOINT16(1);

    /* transform back to the source coordinate system (rotate -> scaling) */
    sw_transform_point32_rot_first(&matrix->tx, &matrix->ty, matrix->tx, matrix->ty,
        PX_FIXEDPOINT16(img_x + pivot_x), PX_FIXEDPOINT16(img_y + pivot_y),
        revert_angle, revert_scale_x, revert_scale_y, scale_bits);
    sw_transform_point32_rot_first(&matrix->sx, &matrix->shy, matrix->sx, matrix->shy,
        FIXEDPOINT12(0), FIXEDPOINT12(0), revert_angle,
        revert_scale_x, revert_scale_y, scale_bits);
    sw_transform_point32_rot_first(&matrix->shx, &matrix->sy, matrix->shx, matrix->sy,
        FIXEDPOINT12(0), FIXEDPOINT12(0), revert_angle,
        revert_scale_x, revert_scale_y, scale_bits);

    /* map to the source pixel coordinate system */
    matrix->tx -= PX_FIXEDPOINT16(img_x);
    matrix->ty -= PX_FIXEDPOINT16(img_y);
}

void sw_transform_rgb565_over_rgb565(void* dst, const void* src,
    uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
    int16_t x, int16_t y, uint16_t w, uint16_t h,
    const sw_matrix_t* matrix)
{
#ifdef CONFIG_GUI_API_BROM_LARK
    if (src_pitch == src_w * 2) {
        p_brom_libgui_api->p_sw_rotate_rgb565_over_rgb565(
            dst, src, dst_pitch / 2, src_w, src_h, x, y, w, h, matrix);
        return;
    }
#endif

#ifdef CONFIG_GUI_API_BROM_LEOPARD
    p_brom_libgui_api->p_sw_transform_rgb565_over_rgb565(
        dst, src, dst_pitch, src_pitch, src_w, src_h, x, y, w, h, matrix);
#else
    uint8_t* dst8 = dst;
    uint16_t src_bytes_per_pixel = 2;
    int32_t src_coord_x = matrix->tx +
        y * matrix->shx + x * matrix->sx;
    int32_t src_coord_y = matrix->ty +
        y * matrix->sy + x * matrix->shy;

    for (int j = h; j > 0; j--) {
        int32_t p_x = src_coord_x;
        int32_t p_y = src_coord_y;
        uint16_t* tmp_dst = (uint16_t*)dst8;

        int x1 = 0, x2 = w - 1;

        sw_transform_compoute_x_range(&x1, &x2, src_w, src_h,
            p_x, p_y, matrix->sx, matrix->shy);
        if (x1 > x2) {
            goto next_line;
        }
        else if (x1 > 0) {
            p_x += matrix->sx * x1;
            p_y += matrix->shy * x1;
            tmp_dst += x1;
        }

        for (int i = x2 - x1; i >= 0; i--) {
            int x = FLOOR_FIXEDPOINT16(p_x);
            int y = FLOOR_FIXEDPOINT16(p_y);
            int x_frac = p_x - FIXEDPOINT16(x);
            int y_frac = p_y - FIXEDPOINT16(y);
            uint8_t* src1 = (uint8_t*)src + y * src_pitch + x * src_bytes_per_pixel;
            uint8_t* src2 = src1 + src_bytes_per_pixel;
            uint8_t* src3 = src1 + src_pitch;
            uint8_t* src4 = src2 + src_pitch;

            *tmp_dst = bilinear_rgb565_fast_m6(*(uint16_t*)src1,
                *(uint16_t*)src2, *(uint16_t*)src3, *(uint16_t*)src4,
                x_frac >> 10, y_frac >> 10, 6);

            tmp_dst += 1;
            p_x += matrix->sx;
            p_y += matrix->shy;
        }

    next_line:
        src_coord_x += matrix->shx;
        src_coord_y += matrix->sy;
        dst8 += dst_pitch;
    }
#endif /* CONFIG_GUI_API_BROM_LEOPARD */
}

void sw_transform_rgb565_over_argb8888(void* dst, const void* src,
    uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
    int16_t x, int16_t y, uint16_t w, uint16_t h,
    const sw_matrix_t* matrix)
{
    //#ifdef CONFIG_GUI_API_BROM_LEOPARD
    //	p_brom_libgui_api->p_sw_transform_rgb565_over_argb8888(
    //			dst, src, dst_pitch, src_pitch, src_w, src_h, x, y, w, h, matrix);
    //#else
    uint8_t* dst8 = dst;
    uint16_t src_bytes_per_pixel = 2;
    int32_t src_coord_x = matrix->tx +
        y * matrix->shx + x * matrix->sx;
    int32_t src_coord_y = matrix->ty +
        y * matrix->sy + x * matrix->shy;

    for (int j = h; j > 0; j--) {
        int32_t p_x = src_coord_x;
        int32_t p_y = src_coord_y;
        uint32_t* tmp_dst = (uint32_t*)dst8;

        int x1 = 0, x2 = w - 1;

        sw_transform_compoute_x_range(&x1, &x2, src_w, src_h,
            p_x, p_y, matrix->sx, matrix->shy);
        if (x1 > x2) {
            goto next_line;
        }
        else if (x1 > 0) {
            p_x += matrix->sx * x1;
            p_y += matrix->shy * x1;
            tmp_dst += x1;
        }

        for (int i = x2 - x1; i >= 0; i--) {
            int x = FLOOR_FIXEDPOINT16(p_x);
            int y = FLOOR_FIXEDPOINT16(p_y);
            int x_frac = p_x - FIXEDPOINT16(x);
            int y_frac = p_y - FIXEDPOINT16(y);
            uint8_t* src1 = (uint8_t*)src + y * src_pitch + x * src_bytes_per_pixel;
            uint8_t* src2 = src1 + src_bytes_per_pixel;
            uint8_t* src3 = src1 + src_pitch;
            uint8_t* src4 = src2 + src_pitch;
            uint16_t c16;

            c16 = bilinear_rgb565_fast_m6(*(uint16_t*)src1,
                *(uint16_t*)src2, *(uint16_t*)src3, *(uint16_t*)src4,
                x_frac >> 10, y_frac >> 10, 6);

            *tmp_dst = ((c16 & 0x1f) << 3) | (c16 & 0x07) |
                ((c16 & 0x07e0) << 5) | ((c16 & 0x60) << 3) |
                ((c16 & 0xf800) << 8) | ((c16 & 0x3800) << 5) | 0xFF000000;

            tmp_dst += 1;
            p_x += matrix->sx;
            p_y += matrix->shy;
        }

    next_line:
        src_coord_x += matrix->shx;
        src_coord_y += matrix->sy;
        dst8 += dst_pitch;
    }
    //#endif /* CONFIG_GUI_API_BROM_LEOPARD */
}

void sw_transform_argb8565_over_rgb565(void* dst, const void* src,
    uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
    int16_t x, int16_t y, uint16_t w, uint16_t h,
    const sw_matrix_t* matrix)
{
#ifdef CONFIG_GUI_API_BROM_LEOPARD
    p_brom_libgui_api->p_sw_transform_argb8565_over_rgb565(
        dst, src, dst_pitch, src_pitch, src_w, src_h, x, y, w, h, matrix);
#else
    uint8_t* dst8 = dst;
    uint16_t src_bytes_per_pixel = 3;
    int32_t src_coord_x = matrix->tx +
        y * matrix->shx + x * matrix->sx;
    int32_t src_coord_y = matrix->ty +
        y * matrix->sy + x * matrix->shy;

    for (int j = h; j > 0; j--) {
        int32_t p_x = src_coord_x;
        int32_t p_y = src_coord_y;
        uint16_t* tmp_dst = (uint16_t*)dst8;

        int x1 = 0, x2 = w - 1;

        sw_transform_compoute_x_range(&x1, &x2, src_w, src_h,
            p_x, p_y, matrix->sx, matrix->shy);
        if (x1 > x2) {
            goto next_line;
        }
        else if (x1 > 0) {
            p_x += matrix->sx * x1;
            p_y += matrix->shy * x1;
            tmp_dst += x1;
        }

        for (int i = x2 - x1; i >= 0; i--) {
            int x = FLOOR_FIXEDPOINT16(p_x);
            int y = FLOOR_FIXEDPOINT16(p_y);
            int x_frac = p_x - FIXEDPOINT16(x);
            int y_frac = p_y - FIXEDPOINT16(y);
            uint8_t* src1 = (uint8_t*)src + y * src_pitch + x * src_bytes_per_pixel;
            uint8_t* src2 = src1 + src_bytes_per_pixel;
            uint8_t* src3 = src1 + src_pitch;
            uint8_t* src4 = src2 + src_pitch;
            uint8_t result[3];

            bilinear_argb8565_fast_m6(result, src1, src2, src3, src4,
                x_frac >> 10, y_frac >> 10, 6);

            *tmp_dst = blend_argb8565_over_rgb565(*tmp_dst, result);

            tmp_dst += 1;
            p_x += matrix->sx;
            p_y += matrix->shy;
        }

    next_line:
        src_coord_x += matrix->shx;
        src_coord_y += matrix->sy;
        dst8 += dst_pitch;
    }
#endif /* CONFIG_GUI_API_BROM_LEOPARD */
}

void sw_transform_argb8565_over_argb8888(void* dst, const void* src,
    uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
    int16_t x, int16_t y, uint16_t w, uint16_t h,
    const sw_matrix_t* matrix)
{
#ifdef CONFIG_GUI_API_BROM_LEOPARD
    p_brom_libgui_api->p_sw_transform_argb8565_over_argb8888(
        dst, src, dst_pitch, src_pitch, src_w, src_h, x, y, w, h, matrix);
#else
    uint8_t* dst8 = dst;
    uint16_t src_bytes_per_pixel = 3;
    int32_t src_coord_x = matrix->tx +
        y * matrix->shx + x * matrix->sx;
    int32_t src_coord_y = matrix->ty +
        y * matrix->sy + x * matrix->shy;

    for (int j = h; j > 0; j--) {
        int32_t p_x = src_coord_x;
        int32_t p_y = src_coord_y;
        uint32_t* tmp_dst = (uint32_t*)dst8;

        int x1 = 0, x2 = w - 1;

        sw_transform_compoute_x_range(&x1, &x2, src_w, src_h,
            p_x, p_y, matrix->sx, matrix->shy);
        if (x1 > x2) {
            goto next_line;
        }
        else if (x1 > 0) {
            p_x += matrix->sx * x1;
            p_y += matrix->shy * x1;
            tmp_dst += x1;
        }

        for (int i = x2 - x1; i >= 0; i--) {
            int x = FLOOR_FIXEDPOINT16(p_x);
            int y = FLOOR_FIXEDPOINT16(p_y);
            int x_frac = p_x - FIXEDPOINT16(x);
            int y_frac = p_y - FIXEDPOINT16(y);
            uint8_t* src1 = (uint8_t*)src + y * src_pitch + x * src_bytes_per_pixel;
            uint8_t* src2 = src1 + src_bytes_per_pixel;
            uint8_t* src3 = src1 + src_pitch;
            uint8_t* src4 = src2 + src_pitch;
            uint8_t result[3];

            bilinear_argb8565_fast_m6(result, src1, src2, src3, src4,
                x_frac >> 10, y_frac >> 10, 6);

            *tmp_dst = blend_argb8565_over_argb8888(*tmp_dst, result);

            tmp_dst += 1;
            p_x += matrix->sx;
            p_y += matrix->shy;
        }

    next_line:
        src_coord_x += matrix->shx;
        src_coord_y += matrix->sy;
        dst8 += dst_pitch;
    }
#endif /* CONFIG_GUI_API_BROM_LEOPARD */
}

void sw_transform_argb6666_over_rgb565(void* dst, const void* src,
    uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
    int16_t x, int16_t y, uint16_t w, uint16_t h,
    const sw_matrix_t* matrix)
{
#ifdef CONFIG_GUI_API_BROM_LEOPARD
    p_brom_libgui_api->p_sw_transform_argb6666_over_rgb565(
        dst, src, dst_pitch, src_pitch, src_w, src_h, x, y, w, h, matrix);
#else
    uint8_t* dst8 = dst;
    uint16_t src_bytes_per_pixel = 3;
    int32_t src_coord_x = matrix->tx +
        y * matrix->shx + x * matrix->sx;
    int32_t src_coord_y = matrix->ty +
        y * matrix->sy + x * matrix->shy;

    for (int j = h; j > 0; j--) {
        int32_t p_x = src_coord_x;
        int32_t p_y = src_coord_y;
        uint16_t* tmp_dst = (uint16_t*)dst8;

        int x1 = 0, x2 = w - 1;

        sw_transform_compoute_x_range(&x1, &x2, src_w, src_h,
            p_x, p_y, matrix->sx, matrix->shy);
        if (x1 > x2) {
            goto next_line;
        }
        else if (x1 > 0) {
            p_x += matrix->sx * x1;
            p_y += matrix->shy * x1;
            tmp_dst += x1;
        }

        for (int i = x2 - x1; i >= 0; i--) {
            int x = FLOOR_FIXEDPOINT16(p_x);
            int y = FLOOR_FIXEDPOINT16(p_y);
            int x_frac = p_x - FIXEDPOINT16(x);
            int y_frac = p_y - FIXEDPOINT16(y);
            uint8_t* src1 = (uint8_t*)src + y * src_pitch + x * src_bytes_per_pixel;
            uint8_t* src2 = src1 + src_bytes_per_pixel;
            uint8_t* src3 = src1 + src_pitch;
            uint8_t* src4 = src2 + src_pitch;
            uint8_t result[3];

            bilinear_argb6666_fast_m6(result, src1, src2, src3, src4,
                x_frac >> 10, y_frac >> 10, 6);

            *tmp_dst = blend_argb6666_over_rgb565(*tmp_dst, result);

            tmp_dst += 1;
            p_x += matrix->sx;
            p_y += matrix->shy;
        }

    next_line:
        src_coord_x += matrix->shx;
        src_coord_y += matrix->sy;
        dst8 += dst_pitch;
    }
#endif /* CONFIG_GUI_API_BROM_LEOPARD */
}

void sw_transform_argb6666_over_argb8888(void* dst, const void* src,
    uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
    int16_t x, int16_t y, uint16_t w, uint16_t h,
    const sw_matrix_t* matrix)
{
#ifdef CONFIG_GUI_API_BROM_LEOPARD
    p_brom_libgui_api->p_sw_transform_argb6666_over_argb8888(
        dst, src, dst_pitch, src_pitch, src_w, src_h, x, y, w, h, matrix);
#else
    uint8_t* dst8 = dst;
    uint16_t src_bytes_per_pixel = 3;
    int32_t src_coord_x = matrix->tx +
        y * matrix->shx + x * matrix->sx;
    int32_t src_coord_y = matrix->ty +
        y * matrix->sy + x * matrix->shy;

    for (int j = h; j > 0; j--) {
        int32_t p_x = src_coord_x;
        int32_t p_y = src_coord_y;
        uint32_t* tmp_dst = (uint32_t*)dst8;

        int x1 = 0, x2 = w - 1;

        sw_transform_compoute_x_range(&x1, &x2, src_w, src_h,
            p_x, p_y, matrix->sx, matrix->shy);
        if (x1 > x2) {
            goto next_line;
        }
        else if (x1 > 0) {
            p_x += matrix->sx * x1;
            p_y += matrix->shy * x1;
            tmp_dst += x1;
        }

        for (int i = x2 - x1; i >= 0; i--) {
            int x = FLOOR_FIXEDPOINT16(p_x);
            int y = FLOOR_FIXEDPOINT16(p_y);
            int x_frac = p_x - FIXEDPOINT16(x);
            int y_frac = p_y - FIXEDPOINT16(y);
            uint8_t* src1 = (uint8_t*)src + y * src_pitch + x * src_bytes_per_pixel;
            uint8_t* src2 = src1 + src_bytes_per_pixel;
            uint8_t* src3 = src1 + src_pitch;
            uint8_t* src4 = src2 + src_pitch;
            uint8_t result[3];

            bilinear_argb6666_fast_m6(result, src1, src2, src3, src4,
                x_frac >> 10, y_frac >> 10, 6);

            *tmp_dst = blend_argb6666_over_argb8888(*tmp_dst, result);

            tmp_dst += 1;
            p_x += matrix->sx;
            p_y += matrix->shy;
        }

    next_line:
        src_coord_x += matrix->shx;
        src_coord_y += matrix->sy;
        dst8 += dst_pitch;
    }
#endif /* CONFIG_GUI_API_BROM_LEOPARD */
}

void sw_transform_argb8888_over_rgb565(void* dst, const void* src,
    uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
    int16_t x, int16_t y, uint16_t w, uint16_t h,
    const sw_matrix_t* matrix)
{
#ifdef CONFIG_GUI_API_BROM_LARK
    if (src_pitch == src_w * 4) {
        p_brom_libgui_api->p_sw_rotate_argb8888_over_rgb565(
            dst, src, dst_pitch / 2, src_w, src_h, x, y, w, h, matrix);
        return;
    }
#endif

#ifdef CONFIG_GUI_API_BROM_LEOPARD
    p_brom_libgui_api->p_sw_transform_argb8888_over_rgb565(
        dst, src, dst_pitch, src_pitch, src_w, src_h, x, y, w, h, matrix);
#else
    uint8_t* dst8 = dst;
    uint16_t src_bytes_per_pixel = 4;
    int32_t src_coord_x = matrix->tx +
        y * matrix->shx + x * matrix->sx;
    int32_t src_coord_y = matrix->ty +
        y * matrix->sy + x * matrix->shy;

    for (int j = h; j > 0; j--) {
        int32_t p_x = src_coord_x;
        int32_t p_y = src_coord_y;
        uint16_t* tmp_dst = (uint16_t*)dst8;

        int x1 = 0, x2 = w - 1;

        sw_transform_compoute_x_range(&x1, &x2, src_w, src_h,
            p_x, p_y, matrix->sx, matrix->shy);
        if (x1 > x2) {
            goto next_line;
        }
        else if (x1 > 0) {
            p_x += matrix->sx * x1;
            p_y += matrix->shy * x1;
            tmp_dst += x1;
        }

        for (int i = x2 - x1; i >= 0; i--) {
            int x = FLOOR_FIXEDPOINT16(p_x);
            int y = FLOOR_FIXEDPOINT16(p_y);
            int x_frac = p_x - FIXEDPOINT16(x);
            int y_frac = p_y - FIXEDPOINT16(y);
            uint8_t* src1 = (uint8_t*)src + y * src_pitch + x * src_bytes_per_pixel;
            uint8_t* src2 = src1 + src_bytes_per_pixel;
            uint8_t* src3 = src1 + src_pitch;
            uint8_t* src4 = src2 + src_pitch;

            uint32_t color = bilinear_argb8888_fast_m8(*(uint32_t*)src1,
                *(uint32_t*)src2, *(uint32_t*)src3, *(uint32_t*)src4,
                x_frac >> 8, y_frac >> 8, 8);

            *tmp_dst = blend_argb8888_over_rgb565(*tmp_dst, color);

            tmp_dst += 1;
            p_x += matrix->sx;
            p_y += matrix->shy;
        }

    next_line:
        src_coord_x += matrix->shx;
        src_coord_y += matrix->sy;
        dst8 += dst_pitch;
    }
#endif /* CONFIG_GUI_API_BROM_LEOPARD */
}

void sw_transform_argb8888_over_argb8888(void* dst, const void* src,
    uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
    int16_t x, int16_t y, uint16_t w, uint16_t h,
    const sw_matrix_t* matrix)
{
#ifdef CONFIG_GUI_API_BROM_LARK
    if (src_pitch == src_w * 4) {
        p_brom_libgui_api->p_sw_rotate_argb8888_over_argb8888(
            dst, src, dst_pitch / 4, src_w, src_h, x, y, w, h, matrix);
        return;
    }
#endif

#ifdef CONFIG_GUI_API_BROM_LEOPARD
    p_brom_libgui_api->p_sw_transform_argb8888_over_argb8888(
        dst, src, dst_pitch, src_pitch, src_w, src_h, x, y, w, h, matrix);
#else
    uint8_t* dst8 = dst;
    uint16_t src_bytes_per_pixel = 4;
    int32_t src_coord_x = matrix->tx +
        y * matrix->shx + x * matrix->sx;
    int32_t src_coord_y = matrix->ty +
        y * matrix->sy + x * matrix->shy;

    for (int j = h; j > 0; j--) {
        int32_t p_x = src_coord_x;
        int32_t p_y = src_coord_y;
        uint32_t* tmp_dst = (uint32_t*)dst8;
        int x1 = 0, x2 = w - 1;

        sw_transform_compoute_x_range(&x1, &x2, src_w, src_h,
            p_x, p_y, matrix->sx, matrix->shy);
        if (x1 > x2) {
            goto next_line;
        }
        else if (x1 > 0) {
            p_x += matrix->sx * x1;
            p_y += matrix->shy * x1;
            tmp_dst += x1;
        }

        for (int i = x2 - x1; i >= 0; i--) {
            int x = FLOOR_FIXEDPOINT16(p_x);
            int y = FLOOR_FIXEDPOINT16(p_y);
            int x_frac = p_x - FIXEDPOINT16(x);
            int y_frac = p_y - FIXEDPOINT16(y);
            uint8_t* src1 = (uint8_t*)src + y * src_pitch + x * src_bytes_per_pixel;
            uint8_t* src2 = src1 + src_bytes_per_pixel;
            uint8_t* src3 = src1 + src_pitch;
            uint8_t* src4 = src2 + src_pitch;

            uint32_t color = bilinear_argb8888_fast_m8(*(uint32_t*)src1,
                *(uint32_t*)src2, *(uint32_t*)src3, *(uint32_t*)src4,
                x_frac >> 8, y_frac >> 8, 8);

            *tmp_dst = blend_argb8888_over_argb8888(*tmp_dst, color);

            tmp_dst += 1;
            p_x += matrix->sx;
            p_y += matrix->shy;
        }

    next_line:
        src_coord_x += matrix->shx;
        src_coord_y += matrix->sy;
        dst8 += dst_pitch;
    }
#endif /* CONFIG_GUI_API_BROM_LEOPARD */
}
