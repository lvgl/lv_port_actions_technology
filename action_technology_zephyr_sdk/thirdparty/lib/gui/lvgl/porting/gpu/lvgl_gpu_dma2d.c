/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*********************
 *      INCLUDES
 *********************/

#include "lvgl_gpu.h"

#if LV_USE_DRAW_ACTS_DMA2D

#include <dma2d_hal.h>
#include <errno.h>

/*********************
 *      DEFINES
 *********************/

/* Minimum area (in pixels) for DMA2D blit/fill processing. */
#define LV_DRAW_DMA2D_SIZE_LIMIT 32

#define INDEX_PIXEL_FORMATS \
	    (HAL_PIXEL_FORMAT_I8 | HAL_PIXEL_FORMAT_I4 | HAL_PIXEL_FORMAT_I2 | HAL_PIXEL_FORMAT_I1)

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    lv_draw_unit_t base_unit;
    lv_draw_task_t * task_act;
    hal_dma2d_handle_t * hdma2d;
} lv_draw_dma2d_unit_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static uint32_t _lv_cf_to_dma2d_for_copy(lv_color_format_t cf);

static int _dma2d_fill(lv_draw_unit_t * draw_unit, const lv_area_t * clip_a, lv_color_t color, lv_opa_t opa);
static int _dma2d_rotate(lv_draw_unit_t * draw_unit, const lv_area_t * clip_a,
                         const lv_draw_image_dsc_t * draw_dsc,
                         const lv_draw_buf_t * src_buf, const lv_area_t * src_coords);
static int _dma2d_blit(lv_draw_unit_t * draw_unit, const lv_area_t * clip_a,
                       const lv_draw_buf_t * src_buf, const lv_area_t * src_a,
                       lv_color_t recolor, lv_opa_t opa);

static int32_t _dma2d_draw_dispatch(lv_draw_unit_t * draw_unit, lv_layer_t * layer);
static int32_t _dma2d_draw_evaluate(lv_draw_unit_t * draw_unit, lv_draw_task_t * task);
static int32_t _dma2d_draw_delete(lv_draw_unit_t * draw_unit);
static int32_t _dma2d_wait_for_finish(lv_draw_unit_t * draw_unit);

/**********************
 *  STATIC VARIABLES
 **********************/

static hal_dma2d_handle_t g_hdma2d __in_section_unique(lvgl.noinit.gpu);

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_gpu_draw_dma2d_init(void)
{
    if (hal_dma2d_init(&g_hdma2d, HAL_DMA2D_FULL_MODES)) {
        LV_LOG_ERROR("DMA2D init failed");
        return;
    }

    lv_draw_dma2d_unit_t * draw_unit = lv_draw_create_unit(sizeof(*draw_unit));
    if (draw_unit) {
        draw_unit->hdma2d = &g_hdma2d;
        draw_unit->base_unit.dispatch_cb = _dma2d_draw_dispatch;
        draw_unit->base_unit.evaluate_cb = _dma2d_draw_evaluate;
        draw_unit->base_unit.delete_cb = _dma2d_draw_delete;
        draw_unit->base_unit.wait_for_finish_cb = _dma2d_wait_for_finish;
    }
}

lv_result_t lv_gpu_draw_buf_dma2d_clear(lv_draw_buf_t * draw_buf, const lv_area_t * area)
{
    hal_dma2d_handle_t * hdma2d = &g_hdma2d;
    const lv_image_header_t * header = &draw_buf->header;
    uint8_t * buf;
    uint32_t width;
    uint32_t height;

    hdma2d->output_cfg.color_format = _lv_cf_to_dma2d_for_copy(header->cf);
    if (hdma2d->output_cfg.color_format == 0)
        return LV_RESULT_INVALID;

    if (area == NULL) {
        buf = lv_draw_buf_goto_xy(draw_buf, 0, 0);
        width = header->w;
        height = header->h;
    }
    else {
        buf = lv_draw_buf_goto_xy(draw_buf, area->x1, area->y1);
        width = lv_area_get_width(area);
        height = lv_area_get_height(area);
    }

    hdma2d->output_cfg.mode = HAL_DMA2D_R2M;
    hdma2d->output_cfg.output_pitch = header->stride;
    hal_dma2d_config_output(hdma2d);

    int res = hal_dma2d_start(hdma2d, 0, (uint32_t)buf, width, height);
    if (res >= 0) {
        hal_dma2d_poll_transfer(hdma2d, -1);
        return LV_RESULT_OK;
    }

    return LV_RESULT_INVALID;
}

lv_result_t lv_gpu_draw_buf_dma2d_copy(lv_draw_buf_t * dest, const lv_area_t * dest_area,
                                       const lv_draw_buf_t * src, const lv_area_t * src_area)
{
    hal_dma2d_handle_t * hdma2d = &g_hdma2d;
    uint8_t * dest_bufc;
    uint8_t * src_bufc;
    uint32_t width;
    uint32_t height;

    hdma2d->output_cfg.color_format = _lv_cf_to_dma2d_for_copy(dest->header.cf);
    if (hdma2d->output_cfg.color_format == 0)
        return LV_RESULT_INVALID;

    if (dest_area) {
        width = lv_area_get_width(dest_area);
        height = lv_area_get_height(dest_area);
    } else if (src_area) {
        width = lv_area_get_width(src_area);
        height = lv_area_get_height(src_area);
    } else {
        width = dest->header.w;
        height = dest->header.h;
    }

    /* For indexed image, copy the palette if we are copying full image area*/
    if(dest_area == NULL || src_area == NULL) {
        if(LV_COLOR_FORMAT_IS_INDEXED(dest->header.cf)) {
            lv_memcpy(dest->data, src->data, LV_COLOR_INDEXED_PALETTE_SIZE(dest->header.cf) * sizeof(lv_color32_t));
        }
    }

    if(src_area) src_bufc = lv_draw_buf_goto_xy(src, src_area->x1, src_area->y1);
    else src_bufc = lv_draw_buf_goto_xy(src, 0, 0);

    if(dest_area) dest_bufc = lv_draw_buf_goto_xy(dest, dest_area->x1, dest_area->y1);
    else dest_bufc = lv_draw_buf_goto_xy(dest, 0, 0);

    hdma2d->output_cfg.mode = HAL_DMA2D_M2M;
    hdma2d->output_cfg.output_pitch = dest->header.stride;
    hal_dma2d_config_output(hdma2d);

    hdma2d->layer_cfg[1].color_format = hdma2d->output_cfg.color_format;
    hdma2d->layer_cfg[1].input_pitch = src->header.stride;
    hdma2d->layer_cfg[1].alpha_mode = HAL_DMA2D_NO_MODIF_ALPHA;
    hdma2d->layer_cfg[1].input_alpha = 0xffffffff;
    hal_dma2d_config_layer(hdma2d, HAL_DMA2D_FOREGROUND_LAYER);

    int res = hal_dma2d_start(hdma2d, (uint32_t)src_bufc, (uint32_t)dest_bufc, width, height);
    if (res >= 0) {
        hal_dma2d_poll_transfer(hdma2d, -1);
        return LV_RESULT_OK;
    }

    return LV_RESULT_INVALID;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static inline void _lv_color_swap_rb(lv_color_t *color)
{
    uint8_t red = color->red;
    color->red = color->blue;
    color->blue = red;
}

static uint32_t _lv_cf_to_dma2d(lv_color_format_t cf, bool rbswap)
{
    switch (cf) {
        case LV_COLOR_FORMAT_RGB565:
            return rbswap ? HAL_PIXEL_FORMAT_RGB_565_BE : HAL_PIXEL_FORMAT_RGB_565;
        case LV_COLOR_FORMAT_ARGB8888:
            return rbswap ? HAL_PIXEL_FORMAT_ABGR_8888 : HAL_PIXEL_FORMAT_ARGB_8888;
        case LV_COLOR_FORMAT_XRGB8888:
            return rbswap ? HAL_PIXEL_FORMAT_XBGR_8888 : HAL_PIXEL_FORMAT_XRGB_8888;
        case LV_COLOR_FORMAT_RGB888:
            return rbswap ? HAL_PIXEL_FORMAT_BGR_888 : HAL_PIXEL_FORMAT_RGB_888;
        case LV_COLOR_FORMAT_ARGB8565:
            return rbswap ? HAL_PIXEL_FORMAT_ABGR_8565 : HAL_PIXEL_FORMAT_ARGB_8565;

        case LV_COLOR_FORMAT_I8:
            return HAL_PIXEL_FORMAT_I8;
        case LV_COLOR_FORMAT_I4:
            return HAL_PIXEL_FORMAT_I4;
        case LV_COLOR_FORMAT_I2:
            return HAL_PIXEL_FORMAT_I2;
        case LV_COLOR_FORMAT_I1:
            return HAL_PIXEL_FORMAT_I1;
        case LV_COLOR_FORMAT_A8:
            return HAL_PIXEL_FORMAT_A8;
        case LV_COLOR_FORMAT_A4:
            return HAL_PIXEL_FORMAT_A4;
        case LV_COLOR_FORMAT_A2:
            return HAL_PIXEL_FORMAT_A2;
        case LV_COLOR_FORMAT_A1:
            return HAL_PIXEL_FORMAT_A1;
        default:
            return 0;
    }
}

static uint32_t _lv_cf_to_dma2d_for_copy(lv_color_format_t cf)
{
    uint8_t px_size = lv_color_format_get_size(cf);

    switch (px_size) {
        case 4:
            return HAL_PIXEL_FORMAT_ARGB_8888;
        case 3:
            return HAL_PIXEL_FORMAT_BGR_888;
        case 2:
            return HAL_PIXEL_FORMAT_RGB_565;
        case 1:
            /*return HAL_PIXEL_FORMAT_A8;*/
        default:
            return 0;
    }
}

static bool _dma2d_dest_cf_supported(lv_color_format_t cf)
{
    switch (cf) {
        case LV_COLOR_FORMAT_RGB565:
        case LV_COLOR_FORMAT_ARGB8565:
        case LV_COLOR_FORMAT_ARGB8888:
        case LV_COLOR_FORMAT_XRGB8888:
        case LV_COLOR_FORMAT_RGB888: /* swap source RB channel to support */
            return true;
        /* case LV_COLOR_FORMAT_RGB888: */
        case LV_COLOR_FORMAT_RGB565A8:
        default:
            return false;
    }
}

static bool _dma2d_src_cf_supported(lv_color_format_t cf)
{
    switch (cf) {
        case LV_COLOR_FORMAT_RGB565:
        case LV_COLOR_FORMAT_ARGB8565:
        case LV_COLOR_FORMAT_ARGB8888:
        case LV_COLOR_FORMAT_XRGB8888:
        case LV_COLOR_FORMAT_RGB888:
        case LV_COLOR_FORMAT_I1:
        case LV_COLOR_FORMAT_I2:
        case LV_COLOR_FORMAT_I4:
        case LV_COLOR_FORMAT_I8:
        case LV_COLOR_FORMAT_A1:
        case LV_COLOR_FORMAT_A2:
        case LV_COLOR_FORMAT_A4:
        case LV_COLOR_FORMAT_A8:
            return true;
        case LV_COLOR_FORMAT_RGB565A8:
        default:
            return false;
    }
}

static bool _dma2d_src_cf_rotation_supported(lv_color_format_t cf)
{
    switch (cf) {
        case LV_COLOR_FORMAT_RGB565:
        case LV_COLOR_FORMAT_ARGB8888:
        case LV_COLOR_FORMAT_XRGB8888:
            return true;
        default:
            return false;
    }
}

static int _dma2d_fill(lv_draw_unit_t * draw_unit, const lv_area_t * clip_a, lv_color_t color, lv_opa_t opa)
{
    hal_dma2d_handle_t * hdma2d = ((lv_draw_dma2d_unit_t *)draw_unit)->hdma2d;
    lv_draw_buf_t * dest_buf = draw_unit->target_layer->draw_buf;
    lv_area_t dest_area = {
        clip_a->x1 - draw_unit->target_layer->buf_area.x1, clip_a->y1 - draw_unit->target_layer->buf_area.y1,
        clip_a->x2 - draw_unit->target_layer->buf_area.x1, clip_a->y2 - draw_unit->target_layer->buf_area.y1,
    };
    uint32_t dest_addr = (uint32_t)lv_draw_buf_goto_xy(dest_buf, dest_area.x1, dest_area.y1);
    uint16_t dest_w = lv_area_get_width(&dest_area);
    uint16_t dest_h = lv_area_get_height(&dest_area);
    int res;

    bool rbswap = (dest_buf->header.cf == LV_COLOR_FORMAT_RGB888);
    if (rbswap) _lv_color_swap_rb(&color);

    hdma2d->output_cfg.mode = (opa < LV_OPA_MAX) ? HAL_DMA2D_M2M_BLEND_FG : HAL_DMA2D_R2M;
    hdma2d->output_cfg.color_format = _lv_cf_to_dma2d(dest_buf->header.cf, rbswap);
    hdma2d->output_cfg.output_pitch = dest_buf->header.stride;
    hal_dma2d_config_output(hdma2d);

    lv_draw_buf_invalidate_cache(dest_buf, &dest_area);

    if (opa < LV_OPA_MAX) {
        hdma2d->layer_cfg[0].color_format = hdma2d->output_cfg.color_format;
        hdma2d->layer_cfg[0].input_width = dest_w;
        hdma2d->layer_cfg[0].input_height = dest_h;
        hdma2d->layer_cfg[0].input_pitch = hdma2d->output_cfg.output_pitch;
        hal_dma2d_config_layer(hdma2d, HAL_DMA2D_BACKGROUND_LAYER);

        uint32_t color32 = (opa << 24) + (color.red << 16) + (color.green << 8) + (color.blue);
        res = hal_dma2d_blending_start(hdma2d, color32, dest_addr, dest_addr, dest_w, dest_h);
    }
    else {
        res = hal_dma2d_start(hdma2d, lv_color_to_u32(color), dest_addr, dest_w, dest_h);
    }

    if (res < 0) {
        LV_LOG_WARN("error: dest 0x%x, w %u, h %u, stride %u, fmt 0x%x",
            dest_addr, dest_w, dest_h, dest_buf->header.stride, dest_buf->header.cf);
    }
    else {
#if LV_USE_DRAW_VG_LITE
        hal_dma2d_poll_transfer(hdma2d, -1);
#endif
    }

    return res;
}

static int _dma2d_rotate(lv_draw_unit_t * draw_unit, const lv_area_t * clip_a,
                         const lv_draw_image_dsc_t * draw_dsc,
                         const lv_draw_buf_t * src_buf, const lv_area_t * src_coords)
{
    hal_dma2d_handle_t * hdma2d = ((lv_draw_dma2d_unit_t *)draw_unit)->hdma2d;
    lv_draw_buf_t * dest_buf = draw_unit->target_layer->draw_buf;
    lv_area_t dest_area = {
        clip_a->x1 - draw_unit->target_layer->buf_area.x1, clip_a->y1 - draw_unit->target_layer->buf_area.y1,
        clip_a->x2 - draw_unit->target_layer->buf_area.x1, clip_a->y2 - draw_unit->target_layer->buf_area.y1,
    };

    uint32_t dest_addr = (uint32_t)lv_draw_buf_goto_xy(dest_buf, dest_area.x1, dest_area.y1);
    uint16_t dest_w = lv_area_get_width(&dest_area);
    uint16_t dest_h = lv_area_get_height(&dest_area);
    int res;

    hdma2d->output_cfg.color_format = _lv_cf_to_dma2d(dest_buf->header.cf, false);
    hdma2d->output_cfg.mode = (draw_dsc->opa < LV_OPA_MAX || lv_color_format_has_alpha(src_buf->header.cf)) ?
                              HAL_DMA2D_M2M_TRANSFORM_BLEND : HAL_DMA2D_M2M_TRANSFORM;
    hdma2d->output_cfg.output_pitch = dest_buf->header.stride;
    hal_dma2d_config_output(hdma2d);

    /* configure the foreground */
    hdma2d->layer_cfg[1].color_format = _lv_cf_to_dma2d(src_buf->header.cf, false);
    hdma2d->layer_cfg[1].input_pitch = src_buf->header.stride;
    hdma2d->layer_cfg[1].input_width = src_buf->header.w;
    hdma2d->layer_cfg[1].input_height = src_buf->header.h;
    hdma2d->layer_cfg[1].alpha_mode = HAL_DMA2D_COMBINE_ALPHA;
    hdma2d->layer_cfg[1].input_alpha = (uint32_t)draw_dsc->opa << 24;
    hal_dma2d_config_layer(hdma2d, HAL_DMA2D_FOREGROUND_LAYER);

    hdma2d->trans_cfg.angle = draw_dsc->rotation;
    hdma2d->trans_cfg.image_x0 = src_coords->x1;
    hdma2d->trans_cfg.image_y0 = src_coords->y1;
    hdma2d->trans_cfg.rect.pivot_x = draw_dsc->pivot.x;
    hdma2d->trans_cfg.rect.pivot_y = draw_dsc->pivot.y;
    hdma2d->trans_cfg.rect.scale_x = LV_SCALE_NONE;
    hdma2d->trans_cfg.rect.scale_y = LV_SCALE_NONE;
    res = hal_dma2d_config_transform(hdma2d);
    if (res < 0) return -1;

    lv_draw_buf_invalidate_cache(src_buf, NULL);
    lv_draw_buf_invalidate_cache(dest_buf, &dest_area);

    res = hal_dma2d_transform_start(hdma2d, (uint32_t)src_buf->data, dest_addr,
            clip_a->x1, clip_a->y1, dest_w, dest_h);
    if (res < 0) {
        LV_LOG_WARN("error: dest 0x%x, w %u, h %u, stride %u, fmt 0x%x",
            dest_addr, dest_w, dest_h, dest_buf->header.stride, dest_buf->header.cf);
    }
    else {
#if LV_USE_DRAW_VG_LITE
        hal_dma2d_poll_transfer(hdma2d, -1);
#endif
    }

    return res;
}

static int _dma2d_blit(lv_draw_unit_t * draw_unit, const lv_area_t * clip_a,
                       const lv_draw_buf_t * src_buf, const lv_area_t * src_a,
                       lv_color_t recolor, lv_opa_t opa)
{
    hal_dma2d_handle_t * hdma2d = ((lv_draw_dma2d_unit_t *)draw_unit)->hdma2d;
    lv_draw_buf_t * dest_buf = draw_unit->target_layer->draw_buf;
    lv_area_t dest_area = {
        clip_a->x1 - draw_unit->target_layer->buf_area.x1, clip_a->y1 - draw_unit->target_layer->buf_area.y1,
        clip_a->x2 - draw_unit->target_layer->buf_area.x1, clip_a->y2 - draw_unit->target_layer->buf_area.y1,
    };

    uint32_t dest_addr = (uint32_t)lv_draw_buf_goto_xy(dest_buf, dest_area.x1, dest_area.y1);
    uint16_t dest_w = lv_area_get_width(&dest_area);
    uint16_t dest_h = lv_area_get_height(&dest_area);
    int res;

    bool rbswap = (dest_buf->header.cf == LV_COLOR_FORMAT_RGB888);
    if (rbswap) _lv_color_swap_rb(&recolor);

    hdma2d->output_cfg.color_format = _lv_cf_to_dma2d(dest_buf->header.cf, rbswap);
    hdma2d->output_cfg.mode = (opa < LV_OPA_MAX || lv_color_format_has_alpha(src_buf->header.cf)) ?
                              HAL_DMA2D_M2M_BLEND : HAL_DMA2D_M2M;
    hdma2d->output_cfg.output_pitch = dest_buf->header.stride;
    hal_dma2d_config_output(hdma2d);

    /* configure foreground layer */
    uint32_t src_addr = (uint32_t)src_buf->data;
    uint8_t src_bpp = lv_color_format_get_bpp(src_buf->header.cf);
    if (src_a) {
        uint32_t bit_xofs = src_bpp * src_a->x1;

        src_addr += src_buf->header.stride * src_a->y1 + bit_xofs / 8;
        hdma2d->layer_cfg[1].input_xofs = (src_bpp < 8) ? ((bit_xofs & 0x7) >> (src_bpp >> 1)) : 0;
        hdma2d->layer_cfg[1].input_width = lv_area_get_width(src_a);
        hdma2d->layer_cfg[1].input_height = lv_area_get_height(src_a);
    }
    else {
        hdma2d->layer_cfg[1].input_xofs = 0;
        hdma2d->layer_cfg[1].input_width = src_buf->header.w;
        hdma2d->layer_cfg[1].input_height = src_buf->header.h;
    }

    hdma2d->layer_cfg[1].color_format = _lv_cf_to_dma2d(src_buf->header.cf, rbswap);
    hdma2d->layer_cfg[1].input_pitch = src_buf->header.stride;
    hdma2d->layer_cfg[1].alpha_mode = HAL_DMA2D_COMBINE_ALPHA;
    hdma2d->layer_cfg[1].input_alpha = (opa << 24) + (recolor.red << 16) + (recolor.green << 8) + (recolor.blue);
    hal_dma2d_config_layer(hdma2d, HAL_DMA2D_FOREGROUND_LAYER);

    if (hdma2d->layer_cfg[1].color_format & INDEX_PIXEL_FORMATS) {
        uint32_t *palette = (uint32_t *)src_buf->data;
        uint32_t palette_size = (1 << src_bpp);

        src_addr += palette_size * sizeof(uint32_t);
        hal_dma2d_clut_load_start(hdma2d, 1, palette_size, palette);
    }

    /* FIXME: always invalidate the src buffer cache ? */
    lv_draw_buf_invalidate_cache(src_buf, src_a);
    lv_draw_buf_invalidate_cache(dest_buf, &dest_area);

    /* configure background layer */
    if (hdma2d->output_cfg.mode == HAL_DMA2D_M2M_BLEND) {
        hdma2d->layer_cfg[0].color_format = hdma2d->output_cfg.color_format;
        hdma2d->layer_cfg[0].input_pitch = hdma2d->output_cfg.output_pitch;
        hdma2d->layer_cfg[0].input_width = dest_w;
        hdma2d->layer_cfg[0].input_height = dest_h;
        hal_dma2d_config_layer(hdma2d, HAL_DMA2D_BACKGROUND_LAYER);
        res = hal_dma2d_blending_start(hdma2d, src_addr,
                        dest_addr, dest_addr, dest_w, dest_h);
    }
    else {
        res = hal_dma2d_start(hdma2d, src_addr, dest_addr, dest_w, dest_h);
    }

    if (res < 0) {
        LV_LOG_WARN("error: dest 0x%x, w %u, h %u, stride %u, fmt 0x%x; src 0x%x, w %u, h %u, stride %u, fmt 0x%x",
                dest_addr, dest_w, dest_h, dest_buf->header.stride, dest_buf->header.cf,
                src_addr, src_buf->header.w, src_buf->header.h, src_buf->header.stride, src_buf->header.cf);
    }
    else {
#if LV_USE_DRAW_VG_LITE
        hal_dma2d_poll_transfer(hdma2d, -1);
#endif
    }

    return res;
}

static void _dma2d_draw_wait_finish(lv_draw_unit_t * draw_unit)
{
    lv_draw_dma2d_unit_t * unit = (lv_draw_dma2d_unit_t *)draw_unit;
    hal_dma2d_poll_transfer(unit->hdma2d, -1);
}

static void _dma2d_draw_fill(lv_draw_unit_t * draw_unit,
              lv_draw_fill_dsc_t * dsc, const lv_area_t * coords)
{
    if (dsc->opa <= LV_OPA_MIN) return;

    lv_area_t draw_area;
    if (!lv_area_intersect(&draw_area, coords, draw_unit->clip_area)) return;

    LV_PROFILER_BEGIN;

    if (_dma2d_fill(draw_unit, &draw_area, dsc->color, dsc->opa) < 0)
        lv_draw_sw_fill(draw_unit, dsc, coords);

    LV_PROFILER_END;
}

static lv_result_t _open_image_decoder(lv_image_decoder_dsc_t *decoder_dsc,
                                       const void *src, bool no_cache)
{
    lv_image_decoder_args_t args;
    lv_memzero(&args, sizeof(lv_image_decoder_args_t));
    args.stride_align = true;
    args.use_indexed = true;
    args.no_cache = no_cache;
    args.flush_cache = true;

    lv_result_t res = lv_image_decoder_open(decoder_dsc, src, &args);
    if (res != LV_RESULT_OK) {
        LV_LOG_ERROR("Failed to open image");
        return res;
    }

    const lv_draw_buf_t * decoded = decoder_dsc->decoded;
    if (decoded == NULL || decoded->data == NULL) {
        lv_image_decoder_close(decoder_dsc);
        LV_LOG_ERROR("image data is NULL");
        return LV_RESULT_INVALID;
    }

    return LV_RESULT_OK;
}

static void _dma2d_draw_image(lv_draw_unit_t * draw_unit, const lv_draw_image_dsc_t * draw_dsc,
                              const lv_area_t * coords, bool no_cache)
{
    lv_image_decoder_dsc_t decoder_dsc;

    if (draw_dsc->opa <= LV_OPA_MIN)
        return;

    LV_PROFILER_BEGIN;

    if (draw_dsc->tile) {
        int32_t img_w = draw_dsc->header.w;
        int32_t img_h = draw_dsc->header.h;

        lv_area_t tile_area = *coords;
        lv_area_set_width(&tile_area, img_w);
        lv_area_set_height(&tile_area, img_h);

        int32_t tile_x_start = tile_area.x1;

        if (_open_image_decoder(&decoder_dsc, draw_dsc->src, no_cache) != LV_RESULT_OK) {
            LV_PROFILER_END;
            return;
        }

        while (tile_area.y1 <= draw_unit->clip_area->y2) {
            while (tile_area.x1 <= draw_unit->clip_area->x2) {
                lv_area_t clip_area;
                if (lv_area_intersect(&clip_area, &tile_area, draw_unit->clip_area)) {
                    lv_area_t rel_img_area = clip_area;
                    lv_area_move(&rel_img_area, -tile_area.x1, -tile_area.y1);
                    _dma2d_blit(draw_unit, &clip_area,
                                decoder_dsc.decoded, &rel_img_area,
                                draw_dsc->recolor, draw_dsc->opa);
                }

                tile_area.x1 += img_w;
                tile_area.x2 += img_w;
            }

            tile_area.y1 += img_h;
            tile_area.y2 += img_h;
            tile_area.x1 = tile_x_start;
            tile_area.x2 = tile_x_start + img_w - 1;
        }

        lv_image_decoder_close(&decoder_dsc);
    }
    else {
        lv_area_t draw_area;

        if (draw_dsc->rotation || draw_dsc->scale_x != LV_SCALE_NONE || draw_dsc->scale_y != LV_SCALE_NONE) {
            int32_t w = lv_area_get_width(coords);
            int32_t h = lv_area_get_height(coords);

            lv_image_buf_get_transformed_area(&draw_area, w, h, draw_dsc->rotation,
                    draw_dsc->scale_x, draw_dsc->scale_y, &draw_dsc->pivot);
            lv_area_move(&draw_area, coords->x1, coords->y1);
        }
        else {
            lv_area_copy(&draw_area, coords);
        }

        if (!lv_area_intersect(&draw_area, &draw_area, draw_unit->clip_area)) {
            LV_PROFILER_END;
            return;
        }

        if (_open_image_decoder(&decoder_dsc, draw_dsc->src, no_cache) != LV_RESULT_OK) {
            LV_PROFILER_END;
            return;
        }

        lv_area_t rel_img_area;

        if (draw_dsc->rotation || draw_dsc->scale_x != LV_SCALE_NONE || draw_dsc->scale_y != LV_SCALE_NONE) {
            int32_t w = lv_area_get_width(&draw_area);
            int32_t h = lv_area_get_height(&draw_area);

            lv_point_t pivot;
            pivot.x = draw_dsc->pivot.x + coords->x1 - draw_area.x1;
            pivot.y = draw_dsc->pivot.y + coords->y1 - draw_area.y1;

            lv_image_buf_get_transformed_area(&rel_img_area, w, h,
                    3600 - draw_dsc->rotation,
                    LV_SCALE_NONE * LV_SCALE_NONE / draw_dsc->scale_x,
                    LV_SCALE_NONE * LV_SCALE_NONE / draw_dsc->scale_y,
                    &pivot);
            lv_area_move(&rel_img_area, draw_area.x1, draw_area.y1);

            if (!lv_area_intersect(&rel_img_area, &rel_img_area, coords)) {
                LV_PROFILER_END;
                return;
            }
        }
        else {
            lv_area_copy(&rel_img_area, &draw_area);
        }

        if (draw_dsc->rotation) {
            _dma2d_rotate(draw_unit, &draw_area, draw_dsc, (void *)decoder_dsc.decoded, coords);
        }
        else {
            lv_area_move(&rel_img_area, -coords->x1, -coords->y1);
            _dma2d_blit(draw_unit, &draw_area, (void *)decoder_dsc.decoded, &rel_img_area,
                        draw_dsc->recolor, draw_dsc->opa);
        }

        lv_image_decoder_close(&decoder_dsc);

        LV_PROFILER_END;
    }
}

static void _dma2d_draw_layer(lv_draw_unit_t * draw_unit,
              const lv_draw_image_dsc_t * draw_dsc, const lv_area_t * coords)
{
    lv_layer_t * layer_to_draw = (lv_layer_t *)draw_dsc->src;

    /**
     * It can happen that nothing was draw on a layer and therefore
     * its buffer is not allocated. In this case just return.
     */
    if (layer_to_draw->draw_buf == NULL)
        return;

    LV_PROFILER_BEGIN;

    lv_draw_image_dsc_t new_draw_dsc = *draw_dsc;
    new_draw_dsc.src = layer_to_draw->draw_buf;

    _dma2d_draw_image(draw_unit, &new_draw_dsc, coords, true);

    LV_PROFILER_END;
}

static void _dma2d_draw_glyph(lv_draw_unit_t * draw_unit, lv_draw_glyph_dsc_t * glyph_dsc)
{
    const lv_area_t * letter_coords = glyph_dsc->letter_coords;

    if (glyph_dsc->opa <= LV_OPA_MIN) return;

    lv_area_t blend_area;
    if (!lv_area_intersect(&blend_area, letter_coords, draw_unit->clip_area)) return;

    lv_draw_buf_t * draw_buf = glyph_dsc->glyph_data;
    int res = -1;

    if (lv_area_get_width(&blend_area) >= 2 &&
        lv_area_get_size(&blend_area) >= LV_DRAW_DMA2D_SIZE_LIMIT) {
        lv_area_t rel_coords = blend_area;
        lv_area_move(&rel_coords, -letter_coords->x1, -letter_coords->y1);
        res = _dma2d_blit(draw_unit, &blend_area, draw_buf, &rel_coords,
                          glyph_dsc->color, glyph_dsc->opa);
    }

#if LV_USE_DRAW_VG_LITE == 0
    /* FIXME: temp glyph data ? */
    _dma2d_draw_wait_finish(draw_unit);
#endif

    if (res < 0) {
        lv_area_t mask_area = *letter_coords;
        mask_area.x2 = mask_area.x1 + lv_draw_buf_width_to_stride(
                lv_area_get_width(&mask_area), draw_buf->header.cf) - 1;

        lv_draw_sw_blend_dsc_t blend_dsc;
        lv_memzero(&blend_dsc, sizeof(blend_dsc));
        blend_dsc.color = glyph_dsc->color;
        blend_dsc.opa = glyph_dsc->opa;
        blend_dsc.mask_buf = draw_buf->data;
        blend_dsc.mask_area = &mask_area;
        blend_dsc.mask_stride = draw_buf->header.stride;
        blend_dsc.blend_area = letter_coords;
        blend_dsc.mask_res = LV_DRAW_SW_MASK_RES_CHANGED;

        lv_draw_sw_blend(draw_unit, &blend_dsc);
    }
}

static void _dma2d_draw_letter_cb(lv_draw_unit_t * draw_unit,
                lv_draw_glyph_dsc_t * glyph_dsc, lv_draw_fill_dsc_t * fill_dsc,
                const lv_area_t * fill_area)
{
    if (glyph_dsc) {
        switch (glyph_dsc->format) {
            case LV_FONT_GLYPH_FORMAT_A1:
            case LV_FONT_GLYPH_FORMAT_A2:
            case LV_FONT_GLYPH_FORMAT_A4:
            case LV_FONT_GLYPH_FORMAT_A8: {
                _dma2d_draw_glyph(draw_unit, glyph_dsc);
                break;
            }

            case LV_FONT_GLYPH_FORMAT_IMAGE: {
#if LV_USE_IMGFONT
                lv_draw_image_dsc_t img_dsc;
                lv_draw_image_dsc_init(&img_dsc);
                img_dsc.opa = glyph_dsc->opa;
                img_dsc.src = glyph_dsc->glyph_data;
                _dma2d_draw_image(draw_unit, &img_dsc, glyph_dsc->letter_coords, true);
#endif
                break;
            }

            case LV_FONT_GLYPH_FORMAT_NONE:
            default:
                LV_LOG_WARN("Invalid font glyph format %d", glyph_dsc->format);
                break;
        }
    }

    if (fill_dsc && fill_area) {
        _dma2d_draw_fill(draw_unit, fill_dsc, fill_area);
    }
}

static void _dma2d_draw_label(lv_draw_unit_t * draw_unit,
                const lv_draw_label_dsc_t * dsc, const lv_area_t * coords)
{
    if (dsc->opa <= LV_OPA_MIN) return;

    LV_PROFILER_BEGIN;
    lv_draw_label_iterate_characters(draw_unit, dsc, coords, _dma2d_draw_letter_cb);
    LV_PROFILER_END;
}

static void _dma2d_draw_execute(lv_draw_dma2d_unit_t * u)
{
    lv_draw_unit_t * draw_unit = (lv_draw_unit_t *)u;
    lv_draw_task_t * task = u->task_act;

    switch (task->type) {
        case LV_DRAW_TASK_TYPE_FILL:
            _dma2d_draw_fill(draw_unit, task->draw_dsc, &task->area);
            break;

        case LV_DRAW_TASK_TYPE_IMAGE:
            _dma2d_draw_image(draw_unit, task->draw_dsc, &task->area, false);
            break;

        case LV_DRAW_TASK_TYPE_LAYER:
            _dma2d_draw_layer(draw_unit, task->draw_dsc, &task->area);
            break;

        case LV_DRAW_TASK_TYPE_LABEL:
            _dma2d_draw_label(draw_unit, task->draw_dsc, &task->area);
            break;

        default:
            break;
    }
}

static int32_t _dma2d_draw_dispatch(lv_draw_unit_t * draw_unit, lv_layer_t * layer)
{
    lv_draw_dma2d_unit_t * u = (lv_draw_dma2d_unit_t *)draw_unit;

    /* Return immediately if it's busy with draw task. */
    if (u->task_act) return 0;

    /* Try to get an ready to draw. */
    lv_draw_task_t * t = lv_draw_get_next_available_task(layer, NULL, DMA2D_DRAW_UNIT_ID);

    /* Return 0 is no selection, some tasks can be supported by other units. */
    if (t == NULL || t->preferred_draw_unit_id != DMA2D_DRAW_UNIT_ID) {
        _dma2d_draw_wait_finish(draw_unit);
        return LV_DRAW_UNIT_IDLE;
    }

    void * buf = lv_draw_layer_alloc_buf(layer);
    if (buf == NULL) return LV_DRAW_UNIT_IDLE;

    t->state = LV_DRAW_TASK_STATE_IN_PROGRESS;
    u->base_unit.target_layer = layer;
    u->base_unit.clip_area = &t->clip_area;
    u->task_act = t;

    _dma2d_draw_execute(u);

    u->task_act->state = LV_DRAW_TASK_STATE_READY;
    u->task_act = NULL;

    /* Request a new dispatching as it can get a new task. */
    lv_draw_dispatch_request();

    return 1;
}

static int _dma2d_evaluate_task_layer(lv_draw_task_t * task)
{
    lv_draw_dsc_base_t * base_dsc = task->draw_dsc;
    lv_layer_t * layer = base_dsc->layer;
    int32_t area_w = lv_area_get_width(&task->area);
    int32_t area_h = lv_area_get_height(&task->area);

    if (area_w < 2 || (area_w * area_h) < LV_DRAW_DMA2D_SIZE_LIMIT)
        return -EINVAL;

    if (!_dma2d_dest_cf_supported(layer->color_format))
        return -EINVAL;

    return 0;
}

static int32_t _dma2d_draw_evaluate(lv_draw_unit_t * draw_unit, lv_draw_task_t * task)
{
    LV_UNUSED(draw_unit);

    if (_dma2d_evaluate_task_layer(task))
        return 0;

    switch (task->type) {
        case LV_DRAW_TASK_TYPE_FILL: {
            const lv_draw_fill_dsc_t * draw_dsc = task->draw_dsc;
            if (draw_dsc->radius > 0 || draw_dsc->grad.dir != LV_GRAD_DIR_NONE)
                return 0;

            break;
        }

        case LV_DRAW_TASK_TYPE_IMAGE:
        case LV_DRAW_TASK_TYPE_LAYER: {
            lv_layer_t * layer = ((lv_draw_dsc_base_t *)task->draw_dsc)->layer;
            const lv_draw_image_dsc_t * draw_dsc = task->draw_dsc;

            if (draw_dsc->blend_mode != LV_BLEND_MODE_NORMAL || draw_dsc->bitmap_mask_src)
                return 0;
            if (draw_dsc->skew_x || draw_dsc->skew_y)
                return 0;

#if LV_USE_DRAW_VG_LITE
            /* DMA2D's scaling may draw differently, so dispatch to vglite. */
            if (draw_dsc->rotation || draw_dsc->scale_x != LV_SCALE_NONE || draw_dsc->scale_y != LV_SCALE_NONE)
                return 0;
#else
            if (draw_dsc->rotation && (draw_dsc->scale_x != LV_SCALE_NONE || draw_dsc->scale_y != LV_SCALE_NONE))
                return 0;
#endif

            if (task->type == LV_DRAW_TASK_TYPE_IMAGE) {
                if ((draw_dsc->header.flags & (LV_IMAGE_FLAGS_COMPRESSED | LV_IMAGE_FLAGS_PREMULTIPLIED)) ||
                    _dma2d_src_cf_supported(draw_dsc->header.cf) == false) {
                    return 0;
                }

                if (draw_dsc->rotation && (layer->color_format == LV_COLOR_FORMAT_RGB888 ||
                         _dma2d_src_cf_rotation_supported(draw_dsc->header.cf) == false)) {
                    return 0;
                }

                if (draw_dsc->recolor_opa != LV_OPA_TRANSP &&
                    (draw_dsc->header.cf < LV_COLOR_FORMAT_A1 || draw_dsc->header.cf > LV_COLOR_FORMAT_A8)) {
                    return 0;
                }

                /* Cannot swap RB */
                if (layer->color_format == LV_COLOR_FORMAT_RGB888 &&
                    draw_dsc->header.cf >= LV_COLOR_FORMAT_I1 && draw_dsc->header.cf <= LV_COLOR_FORMAT_I8) {
                    return 0;
                }
            }
            else {
                lv_layer_t * layer_to_draw = (lv_layer_t *)draw_dsc->src;
                if (_dma2d_src_cf_supported(layer_to_draw->color_format) == false) {
                    return 0;
                }

                if (draw_dsc->rotation && (layer->color_format == LV_COLOR_FORMAT_RGB888 ||
                         _dma2d_src_cf_rotation_supported(layer_to_draw->color_format) == false)) {
                    return 0;
                }
            }

            break;
        }

        case LV_DRAW_TASK_TYPE_LABEL: {
            const lv_draw_label_dsc_t * draw_dsc = task->draw_dsc;
            if (draw_dsc->blend_mode != LV_BLEND_MODE_NORMAL)
                return 0;

            const lv_font_fmt_txt_dsc_t * font_dsc = draw_dsc->font->dsc;
            if (font_dsc) {
                if (font_dsc->bpp < LV_FONT_GLYPH_FORMAT_A1 ||
                    font_dsc->bpp > LV_FONT_GLYPH_FORMAT_IMAGE)
                    return 0;

                if (font_dsc->bpp <= LV_FONT_GLYPH_FORMAT_A8 &&
                    task->preference_score > DMA2D_DRAW_LABEL_PREFERENCE_SCORE) {
                    task->preference_score = DMA2D_DRAW_LABEL_PREFERENCE_SCORE;
                    task->preferred_draw_unit_id = DMA2D_DRAW_UNIT_ID;
                    return 1;
                }
            }
            break;
        }

        default:
            return 0;
    }

    if (task->preference_score > DMA2D_DRAW_PREFERENCE_SCORE) {
        task->preference_score = DMA2D_DRAW_PREFERENCE_SCORE;
        task->preferred_draw_unit_id = DMA2D_DRAW_UNIT_ID;
    }

    return 1;
}

static int32_t _dma2d_draw_delete(lv_draw_unit_t * draw_unit)
{
    lv_draw_dma2d_unit_t * u = (lv_draw_dma2d_unit_t *)draw_unit;
    hal_dma2d_deinit(u->hdma2d);
    return 1;
}

static int32_t _dma2d_wait_for_finish(lv_draw_unit_t * draw_unit)
{
    lv_draw_dma2d_unit_t * u = (lv_draw_dma2d_unit_t *)draw_unit;
    hal_dma2d_poll_transfer(u->hdma2d, -1);
    return 0;
}

#endif /* LV_USE_DRAW_ACTS_DMA2D */
