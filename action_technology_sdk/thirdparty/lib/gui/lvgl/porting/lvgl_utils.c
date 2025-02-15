/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*********************
 *      INCLUDES
 *********************/

#include <display/display_hal.h>
#ifdef CONFIG_VG_LITE
    #include <vg_lite/vglite_util.h>
#endif

#include "lvgl_utils.h"
#include "../src/draw/actions/lv_draw_actions.h"

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

uint32_t lvx_color_format_to_display(lv_color_format_t cf, uint8_t * bits_per_pixel)
{
    uint32_t format = 0;
    uint8_t bpp = 0;

    switch(cf) {
        case LV_COLOR_FORMAT_RGB565:
            format = HAL_PIXEL_FORMAT_RGB_565;
            bpp = 16;
            break;
        case LV_COLOR_FORMAT_ARGB8565:
            format = HAL_PIXEL_FORMAT_ARGB_8565;
            bpp = 24;
            break;
        case LV_COLOR_FORMAT_ARGB8888:
            format = HAL_PIXEL_FORMAT_ARGB_8888;
            bpp = 32;
            break;
        case LV_COLOR_FORMAT_XRGB8888:
            format = HAL_PIXEL_FORMAT_XRGB_8888;
            bpp = 32;
            break;
        case LV_COLOR_FORMAT_RGB888:
            format = HAL_PIXEL_FORMAT_RGB_888;
            bpp = 24;
            break;

        case LV_COLOR_FORMAT_A8:
            format = HAL_PIXEL_FORMAT_A8;
            bpp = 8;
            break;
        case LV_COLOR_FORMAT_A4:
            format = HAL_PIXEL_FORMAT_A4;
            bpp = 4;
            break;
        case LV_COLOR_FORMAT_A2:
            format = HAL_PIXEL_FORMAT_A2;
            bpp = 2;
            break;
        case LV_COLOR_FORMAT_A1:
            format = HAL_PIXEL_FORMAT_A1;
            bpp = 1;
            break;

        case LV_COLOR_FORMAT_I8:
            format = HAL_PIXEL_FORMAT_I8;
            bpp = 8;
            break;
        case LV_COLOR_FORMAT_I4:
            format = HAL_PIXEL_FORMAT_I4;
            bpp = 4;
            break;
        case LV_COLOR_FORMAT_I2:
            format = HAL_PIXEL_FORMAT_I2;
            bpp = 2;
            break;
        case LV_COLOR_FORMAT_I1:
            format = HAL_PIXEL_FORMAT_I1;
            bpp = 1;
            break;

        default:
            break;
    }

    if(bits_per_pixel) {
        *bits_per_pixel = bpp;
    }

    return format;
}

lv_color_format_t lvx_color_format_from_display(uint32_t format)
{
    lv_color_format_t cf;

    switch(format) {
        case HAL_PIXEL_FORMAT_RGB_565:
            cf = LV_COLOR_FORMAT_RGB565;
            break;
        case HAL_PIXEL_FORMAT_ARGB_8565:
            cf = LV_COLOR_FORMAT_ARGB8565;
            break;
        case HAL_PIXEL_FORMAT_ARGB_8888:
            cf = LV_COLOR_FORMAT_ARGB8888;
            break;
        case HAL_PIXEL_FORMAT_XRGB_8888:
            cf = LV_COLOR_FORMAT_XRGB8888;
            break;
        case HAL_PIXEL_FORMAT_RGB_888:
            cf = LV_COLOR_FORMAT_RGB888;
            break;

        case HAL_PIXEL_FORMAT_A8:
            cf = LV_COLOR_FORMAT_A8;
            break;
        case HAL_PIXEL_FORMAT_A4:
            cf = LV_COLOR_FORMAT_A4;
            break;
        case HAL_PIXEL_FORMAT_A2:
            cf = LV_COLOR_FORMAT_A2;
            break;
        case HAL_PIXEL_FORMAT_A1:
            cf = LV_COLOR_FORMAT_A1;
            break;

        case HAL_PIXEL_FORMAT_I8:
            cf = LV_COLOR_FORMAT_I8;
            break;
        case HAL_PIXEL_FORMAT_I4:
            cf = LV_COLOR_FORMAT_I4;
            break;
        case HAL_PIXEL_FORMAT_I2:
            cf = LV_COLOR_FORMAT_I2;
            break;
        case HAL_PIXEL_FORMAT_I1:
            cf = LV_COLOR_FORMAT_I1;
            break;

        default:
            cf = LV_COLOR_FORMAT_UNKNOWN;
            break;
    }

    return cf;
}


#ifdef CONFIG_VG_LITE

int lvx_color_format_to_vglite(lv_color_format_t cf, uint8_t * bits_per_pixel)
{
    int format = -1;
    uint8_t bpp = 0;

    switch(cf) {
        case LV_COLOR_FORMAT_RGB565:
            format = VG_LITE_BGR565;
            bpp = 16;
            break;

        case LV_COLOR_FORMAT_ARGB8565:
            format = VG_LITE_BGRA5658;
            bpp = 24;
            break;

        case LV_COLOR_FORMAT_ARGB8888:
            format = VG_LITE_BGRA8888;
            bpp = 32;
            break;

        case LV_COLOR_FORMAT_XRGB8888:
            format = VG_LITE_BGRX8888;
            bpp = 32;
            break;

        case LV_COLOR_FORMAT_RGB888:
            format = VG_LITE_BGR888;
            bpp = 24;
            break;

        case LV_COLOR_FORMAT_A8:
            format = VG_LITE_A8;
            bpp = 8;
            break;

        case LV_COLOR_FORMAT_I8:
            format = VG_LITE_INDEX_8;
            bpp = 8;
            break;

        case LV_COLOR_FORMAT_ETC2_EAC:
            format = VG_LITE_RGBA8888_ETC2_EAC;
            bpp = 8;
            break;

        default:
            break;
    }

    if(bits_per_pixel) {
        *bits_per_pixel = bpp;
    }

    return format;
}

int lvx_vglite_map_draw_buf(vg_lite_buffer_t * vgbuf, const lv_draw_buf_t * draw_buf)
{
    if(vgbuf == NULL || draw_buf == NULL)
        return -EINVAL;

    int format = lvx_color_format_to_vglite(draw_buf->header.cf, NULL);
    if(format < 0)
        return -EINVAL;

    uint8_t * data = draw_buf->data;
    if(format == VG_LITE_INDEX_8)
        data += 256 * sizeof(uint32_t);

    uint16_t stride = draw_buf->header.stride;
    if(stride == 0)
        stride = lv_acts_draw_buf_width_to_stride(draw_buf->header.w, draw_buf->header.cf);

    return vglite_buf_map(vgbuf, data, draw_buf->header.w, draw_buf->header.h, stride, format);
}

int lvx_vglite_unmap(vg_lite_buffer_t * vgbuf)
{
    return vglite_buf_unmap(vgbuf);
}

int lvx_vglite_set_image_palette(const lv_image_dsc_t * img_dsc)
{
    vg_lite_uint32_t *palette = (vg_lite_uint32_t *)img_dsc->data;

    switch(img_dsc->header.cf) {
    case LV_COLOR_FORMAT_I8:
        return vg_lite_set_CLUT(256, palette);
    default:
        return -EINVAL;
    }
}

#endif /* CONFIG_VG_LITE */
