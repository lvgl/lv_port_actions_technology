#include <display/sw_draw.h>
#ifdef CONFIG_GUI_API_BROM
#include <brom_interface.h>
#endif

void sw_blend_color_over_rgb565(void* dst, uint32_t src_color,
    uint16_t dst_pitch, uint16_t w, uint16_t h)
{
    uint8_t* dst8 = dst;

    for (int j = h; j > 0; j--) {
        uint16_t* tmp_dst = (uint16_t*)dst8;

        for (int i = w; i > 0; i--) {
            *tmp_dst = blend_argb8888_over_rgb565(*tmp_dst, src_color);
            tmp_dst++;
        }

        dst8 += dst_pitch;
    }
}

void sw_blend_color_over_argb8888(void* dst, uint32_t src_color,
    uint16_t dst_pitch, uint16_t w, uint16_t h)
{
    uint8_t* dst8 = dst;

    for (int j = h; j > 0; j--) {
        uint32_t* tmp_dst = (uint32_t*)dst8;

        for (int i = w; i > 0; i--) {
            *tmp_dst = blend_argb8888_over_argb8888(*tmp_dst, src_color);
            tmp_dst++;
        }

        dst8 += dst_pitch;
    }
}

void sw_blend_a8_over_rgb565(void* dst, const void* src, uint32_t src_color,
    uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
#ifdef CONFIG_GUI_API_BROM_LEOPARD
    p_brom_libgui_api->p_sw_blend_a8_over_rgb565(
        dst, src, src_color, dst_pitch, src_pitch, w, h);
#else
    const uint8_t* src8 = src;
    uint8_t* dst8 = dst;
    uint8_t src_x_step = 1;

    src_color &= ~0xFF000000;

    for (int j = h; j > 0; j--) {
        const uint8_t* tmp_src = src8;
        uint16_t* tmp_dst = (uint16_t*)dst8;

        for (int i = w; i > 0; i--) {
            uint32_t color32 = src_color | ((uint32_t)*tmp_src << 24);

            *tmp_dst = blend_argb8888_over_rgb565(*tmp_dst, color32);
            tmp_dst++;
            tmp_src += src_x_step;
        }

        dst8 += dst_pitch;
        src8 += src_pitch;
    }
#endif /* CONFIG_GUI_API_BROM_LEOPARD */
}

void sw_blend_a8_over_argb8888(void* dst, const void* src, uint32_t src_color,
    uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
#ifdef CONFIG_GUI_API_BROM_LEOPARD
    p_brom_libgui_api->p_sw_blend_a8_over_argb8888(
        dst, src, src_color, dst_pitch, src_pitch, w, h);
#else
    const uint8_t* src8 = src;
    uint8_t* dst8 = dst;
    uint8_t src_x_step = 1;

    src_color &= ~0xFF000000;

    for (int j = h; j > 0; j--) {
        const uint8_t* tmp_src = src8;
        uint32_t* tmp_dst = (uint32_t*)dst8;

        for (int i = w; i > 0; i--) {
            uint32_t color32 = src_color | ((uint32_t)*tmp_src << 24);

            *tmp_dst = blend_argb8888_over_argb8888(*tmp_dst, color32);
            tmp_dst++;
            tmp_src += src_x_step;
        }

        dst8 += dst_pitch;
        src8 += src_pitch;
    }
#endif /* CONFIG_GUI_API_BROM_LEOPARD */
}

void sw_blend_argb8565_over_rgb565(void* dst, const void* src,
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
    const uint8_t* src8 = src;
    uint8_t* dst8 = dst;
    uint8_t src_x_step = 3;

    for (int j = h; j > 0; j--) {
        const uint8_t* tmp_src = src8;
        uint16_t* tmp_dst = (uint16_t*)dst8;

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

void sw_blend_argb8565_over_argb8888(void* dst, const void* src,
    uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
#ifdef CONFIG_GUI_API_BROM_LEOPARD
    p_brom_libgui_api->p_sw_blend_argb8565_over_argb8888(
        dst, src, dst_pitch, src_pitch, w, h);
#else
    const uint8_t* src8 = src;
    uint8_t* dst8 = dst;
    uint8_t src_x_step = 3;

    for (int j = h; j > 0; j--) {
        const uint8_t* tmp_src = src8;
        uint32_t* tmp_dst = (uint32_t*)dst8;

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

void sw_blend_argb6666_over_rgb565(void* dst, const void* src,
    uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
#ifdef CONFIG_GUI_API_BROM_LEOPARD
    p_brom_libgui_api->p_sw_blend_argb6666_over_rgb565(
        dst, src, dst_pitch, src_pitch, w, h);
#else
    const uint8_t* src8 = src;
    uint8_t* dst8 = dst;
    uint8_t src_x_step = 3;

    for (int j = h; j > 0; j--) {
        const uint8_t* tmp_src = src8;
        uint16_t* tmp_dst = (uint16_t*)dst8;

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

void sw_blend_argb6666_over_argb8888(void* dst, const void* src,
    uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
#ifdef CONFIG_GUI_API_BROM_LEOPARD
    p_brom_libgui_api->p_sw_blend_argb6666_over_argb8888(
        dst, src, dst_pitch, src_pitch, w, h);
#else
    const uint8_t* src8 = src;
    uint8_t* dst8 = dst;
    uint8_t src_x_step = 3;

    for (int j = h; j > 0; j--) {
        const uint8_t* tmp_src = src8;
        uint32_t* tmp_dst = (uint32_t*)dst8;

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

void sw_blend_argb1555_over_rgb565(void* dst, const void* src,
    uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
    const uint8_t* src8 = src;
    uint8_t* dst8 = dst;

    for (int j = h; j > 0; j--) {
        const uint16_t* tmp_src = (uint16_t*)src8;
        uint16_t* tmp_dst = (uint16_t*)dst8;

        for (int i = w; i > 0; i--) {
            *tmp_dst = blend_argb1555_over_rgb565(*tmp_dst, *tmp_src);
            tmp_dst++;
            tmp_src++;
        }

        dst8 += dst_pitch;
        src8 += src_pitch;
    }
}

void sw_blend_argb1555_over_argb8888(void* dst, const void* src,
    uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
    const uint8_t* src8 = src;
    uint8_t* dst8 = dst;

    for (int j = h; j > 0; j--) {
        const uint16_t* tmp_src = (uint16_t*)src8;
        uint32_t* tmp_dst = (uint32_t*)dst8;

        for (int i = w; i > 0; i--) {
            *tmp_dst = blend_argb1555_over_argb8888(*tmp_dst, *tmp_src);
            tmp_dst++;
            tmp_src++;
        }

        dst8 += dst_pitch;
        src8 += src_pitch;
    }
}

void sw_blend_argb8888_over_rgb565(void* dst, const void* src,
    uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
#ifdef CONFIG_GUI_API_BROM_LARK
    p_brom_libgui_api->p_sw_blend_argb8888_over_rgb565(
        dst, src, dst_pitch / 2, src_pitch / 4, w, h);
#elif defined(CONFIG_GUI_API_BROM_LEOPARD)
    p_brom_libgui_api->p_sw_blend_argb8888_over_rgb565(
        dst, src, dst_pitch, src_pitch, w, h);
#else
    const uint8_t* src8 = src;
    uint8_t* dst8 = dst;

    for (int j = h; j > 0; j--) {
        const uint32_t* tmp_src = (uint32_t*)src8;
        uint16_t* tmp_dst = (uint16_t*)dst8;

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

void sw_blend_argb8888_over_argb8888(void* dst, const void* src,
    uint16_t dst_pitch, uint16_t src_pitch, uint16_t w, uint16_t h)
{
#ifdef CONFIG_GUI_API_BROM_LARK
    p_brom_libgui_api->p_sw_blend_argb8888_over_argb8888(
        dst, src, dst_pitch / 4, src_pitch / 4, w, h);
#elif defined(CONFIG_GUI_API_BROM_LEOPARD)
    p_brom_libgui_api->p_sw_blend_argb8888_over_argb8888(
        dst, src, dst_pitch, src_pitch, w, h);
#else
    const uint8_t* src8 = src;
    uint8_t* dst8 = dst;

    for (int j = h; j > 0; j--) {
        const uint32_t* tmp_src = (uint32_t*)src8;
        uint32_t* tmp_dst = (uint32_t*)dst8;

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
