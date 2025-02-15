/**
 * @file lv_draw_actsw.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_draw_actsw.h"

#if LV_USE_ACTIONS

#include <display/sw_draw.h>
#include <display/sw_rotate.h>
#include <display/ui_memsetcpy.h>
#include "../../lvgl.h"
#include "../../lvgl_private.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    lv_draw_unit_t base_unit;
    lv_draw_task_t * task_act;
} lv_draw_actsw_unit_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void _sw_transform(lv_draw_unit_t * draw_unit, const lv_draw_image_dsc_t * draw_dsc,
                          const lv_draw_buf_t * src_buf, const lv_area_t * src_coords);

static int32_t _sw_draw_dispatch(lv_draw_unit_t * draw_unit, lv_layer_t * layer);
static int32_t _sw_draw_dispatch_task(lv_draw_unit_t * draw_unit, lv_layer_t * layer, lv_draw_task_t * task);
static int32_t _sw_draw_evaluate(lv_draw_unit_t * draw_unit, lv_draw_task_t * task);
static int32_t _sw_draw_delete(lv_draw_unit_t * draw_unit);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_draw_unit_t * lv_draw_actsw_init(bool subunit)
{
    lv_draw_actsw_unit_t * draw_unit;
    if(subunit) {
        draw_unit = lv_malloc_zeroed(sizeof(*draw_unit));
        draw_unit->base_unit.dispatch_task_cb = _sw_draw_dispatch_task;
    }
    else {
        draw_unit = lv_draw_create_unit(sizeof(*draw_unit));
        draw_unit->base_unit.dispatch_cb = _sw_draw_dispatch;
    }

    draw_unit->base_unit.evaluate_cb = _sw_draw_evaluate;
    draw_unit->base_unit.delete_cb = _sw_draw_delete;

    return &draw_unit->base_unit;
}

void lv_draw_actsw_deinit(void)
{
}

void lv_draw_actsw_blit(lv_draw_unit_t * draw_unit, const lv_draw_buf_t * src_buf,
                        const lv_area_t * src_a, lv_color_t recolor, lv_opa_t opa)
{
    lv_draw_buf_t * dest_buf = draw_unit->target_layer->draw_buf;
    uint8_t *dest8 = (uint8_t *)lv_draw_buf_goto_xy(dest_buf,
                draw_unit->clip_area->x1 - draw_unit->target_layer->buf_area.x1,
                draw_unit->clip_area->y1 - draw_unit->target_layer->buf_area.y1);
    uint16_t dest_w = lv_area_get_width(draw_unit->clip_area);
    uint16_t dest_h = lv_area_get_height(draw_unit->clip_area);
    uint32_t color32 = 0;

    if(src_buf->header.cf >= LV_COLOR_FORMAT_A1 && src_buf->header.cf <= LV_COLOR_FORMAT_A8) {
        color32 = (lv_color_to_int(recolor) & 0xffffff) | ((uint32_t)opa << 24);
    }
    else if(opa < LV_OPA_MAX) {
        LV_LOG_ERROR("Unsupported opa %x", opa);
        return;
    }

    uint8_t src_bpp = lv_color_format_get_bpp(src_buf->header.cf);
    uint32_t src_bofs = src_a->x1 * src_bpp;
    const uint8_t *src_data = src_buf->data + src_a->y1 * src_buf->header.stride + (src_bofs >> 3);
    src_bofs &= 0x7;

    if(dest_buf->header.cf == LV_COLOR_FORMAT_RGB565) {
        switch(src_buf->header.cf) {
            case LV_COLOR_FORMAT_ARGB8565:
                sw_blend_argb8565_over_rgb565(dest8, src_data, dest_buf->header.stride,
                                              src_buf->header.stride, dest_w, dest_h);
                break;
            case LV_COLOR_FORMAT_I8:
                src_data += 256 * 4;
                sw_blend_index8_over_rgb565(dest8, src_data, (uint32_t *)src_buf->data, dest_buf->header.stride,
                                            src_buf->header.stride, dest_w, dest_h);
                break;
            case LV_COLOR_FORMAT_A8:
                sw_blend_a8_over_rgb565(dest8, src_data, color32, dest_buf->header.stride,
                                        src_buf->header.stride, dest_w, dest_h);
                break;
            case LV_COLOR_FORMAT_A4:
                sw_blend_a4_over_rgb565(dest8, src_data, color32, dest_buf->header.stride,
                                        src_buf->header.stride, src_bofs, dest_w, dest_h);
                break;
            case LV_COLOR_FORMAT_A2:
                sw_blend_a2_over_rgb565(dest8, src_data, color32, dest_buf->header.stride,
                                        src_buf->header.stride, src_bofs, dest_w, dest_h);
                break;
            default:
                LV_LOG_ERROR("Unsupported src cf %x", src_buf->header.cf);
                return;
        }
    }
    else if(dest_buf->header.cf == LV_COLOR_FORMAT_ARGB8565) {
        switch(src_buf->header.cf) {
            case LV_COLOR_FORMAT_ARGB8565:
                sw_blend_argb8565_over_argb8565(dest8, src_data, dest_buf->header.stride,
                                                src_buf->header.stride, dest_w, dest_h);
                break;
            case LV_COLOR_FORMAT_I8:
                src_data += 256 * 4;
                sw_blend_index8_over_argb8565(dest8, src_data, (uint32_t *)src_buf->data, dest_buf->header.stride,
                                              src_buf->header.stride, dest_w, dest_h);
                break;
            case LV_COLOR_FORMAT_A8:
                sw_blend_a8_over_argb8565(dest8, src_data, color32, dest_buf->header.stride,
                                          src_buf->header.stride, dest_w, dest_h);
                break;
            case LV_COLOR_FORMAT_A4:
                sw_blend_a4_over_argb8565(dest8, src_data, color32, dest_buf->header.stride,
                                          src_buf->header.stride, src_bofs, dest_w, dest_h);
                break;
            case LV_COLOR_FORMAT_A2:
                sw_blend_a2_over_argb8565(dest8, src_data, color32, dest_buf->header.stride,
                                          src_buf->header.stride, src_bofs, dest_w, dest_h);
                break;
            default:
                LV_LOG_ERROR("Unsupported src cf %x", src_buf->header.cf);
                return;
        }
    }
    else if(dest_buf->header.cf == LV_COLOR_FORMAT_RGB888) {
        switch(src_buf->header.cf) {
            case LV_COLOR_FORMAT_ARGB8565:
                sw_blend_argb8565_over_rgb888(dest8, src_data, dest_buf->header.stride,
                                              src_buf->header.stride, dest_w, dest_h);
                break;
            case LV_COLOR_FORMAT_I8:
                src_data += 256 * 4;
                sw_blend_index8_over_rgb888(dest8, src_data, (uint32_t *)src_buf->data, dest_buf->header.stride,
                                            src_buf->header.stride, dest_w, dest_h);
                break;
            case LV_COLOR_FORMAT_A8:
                sw_blend_a8_over_rgb888(dest8, src_data, color32, dest_buf->header.stride,
                                        src_buf->header.stride, dest_w, dest_h);
                break;
            case LV_COLOR_FORMAT_A4:
                sw_blend_a4_over_rgb888(dest8, src_data, color32, dest_buf->header.stride,
                                        src_buf->header.stride, src_bofs, dest_w, dest_h);
                break;
            case LV_COLOR_FORMAT_A2:
                sw_blend_a2_over_rgb888(dest8, src_data, color32, dest_buf->header.stride,
                                        src_buf->header.stride, src_bofs, dest_w, dest_h);
                break;
            default:
                LV_LOG_ERROR("Unsupported src cf %x", src_buf->header.cf);
                return;
        }
    }
    else if(dest_buf->header.cf == LV_COLOR_FORMAT_ARGB8888) {
        switch(src_buf->header.cf) {
            case LV_COLOR_FORMAT_ARGB8565:
                sw_blend_argb8565_over_argb8888(dest8, src_data, dest_buf->header.stride,
                                                src_buf->header.stride, dest_w, dest_h);
                break;
            case LV_COLOR_FORMAT_I8:
                src_data += 256 * 4;
                sw_blend_index8_over_argb8888(dest8, src_data, (uint32_t *)src_buf->data, dest_buf->header.stride,
                                              src_buf->header.stride, dest_w, dest_h);
                break;
            case LV_COLOR_FORMAT_A8:
                sw_blend_a8_over_argb8888(dest8, src_data, color32, dest_buf->header.stride,
                                          src_buf->header.stride, dest_w, dest_h);
                break;
            case LV_COLOR_FORMAT_A4:
                sw_blend_a4_over_argb8888(dest8, src_data, color32, dest_buf->header.stride,
                                          src_buf->header.stride, src_bofs, dest_w, dest_h);
                break;
            case LV_COLOR_FORMAT_A2:
                sw_blend_a2_over_argb8888(dest8, src_data, color32, dest_buf->header.stride,
                                          src_buf->header.stride, src_bofs, dest_w, dest_h);
                break;
            default:
                LV_LOG_ERROR("Unsupported src cf %x", src_buf->header.cf);
                return;
        }
    }
    else {
        LV_LOG_ERROR("Unsupported dest cf %x", dest_buf->header.cf);
        return;
    }
}

void lv_draw_buf_actsw_clear(lv_draw_buf_t * draw_buf, const lv_area_t * area)
{
    const lv_image_header_t * header = &draw_buf->header;
    uint8_t * buf;
    uint32_t height;
    uint32_t line_length;

    if(area == NULL) {
        buf = lv_draw_buf_goto_xy(draw_buf, 0, 0);
        height = header->h;
        line_length = header->stride;
    }
    else {
        buf = lv_draw_buf_goto_xy(draw_buf, area->x1, area->y1);
        height = lv_area_get_height(area);
        line_length = (lv_area_get_width(area) * lv_color_format_get_bpp(header->cf) + 7) >> 3;
    }

    ui_memset2d(buf, header->stride, 0, line_length, height);
    ui_memsetcpy_wait_finish(5000);
}

void lv_draw_buf_actsw_copy(lv_draw_buf_t * dest, const lv_area_t * dest_area,
                             const lv_draw_buf_t * src, const lv_area_t * src_area)
{
    uint8_t * dest_bufc;
    uint8_t * src_bufc;
    uint32_t width;
    uint32_t height;

    if(dest_area) {
        width = lv_area_get_width(dest_area);
        height = lv_area_get_height(dest_area);
    }
    else if(src_area) {
        width = lv_area_get_width(src_area);
        height = lv_area_get_height(src_area);
    }
    else {
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

    uint32_t line_bytes = (width * lv_color_format_get_bpp(dest->header.cf) + 7) >> 3;
    ui_memcpy2d(dest_bufc, dest->header.stride, src_bufc, src->header.stride,
                line_bytes, height);
    ui_memsetcpy_wait_finish(5000);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static bool _sw_dest_cf_supported(lv_color_format_t cf)
{
    switch(cf) {
        case LV_COLOR_FORMAT_RGB565:
        case LV_COLOR_FORMAT_ARGB8565:
        case LV_COLOR_FORMAT_RGB888:
        case LV_COLOR_FORMAT_ARGB8888:
            return true;
        case LV_COLOR_FORMAT_XRGB8888:
        case LV_COLOR_FORMAT_RGB565A8:
        default:
            return false;
    }
}

static bool _sw_src_cf_supported(lv_color_format_t cf)
{
    switch(cf) {
        case LV_COLOR_FORMAT_ARGB8565:
        case LV_COLOR_FORMAT_I8:
        case LV_COLOR_FORMAT_A1:
        case LV_COLOR_FORMAT_A2:
        case LV_COLOR_FORMAT_A4:
        case LV_COLOR_FORMAT_A8:
            return true;
        /* support LVGL sw unsupported CF is enough */
        case LV_COLOR_FORMAT_RGB565:
        case LV_COLOR_FORMAT_RGB565A8:
        case LV_COLOR_FORMAT_ARGB8888:
        case LV_COLOR_FORMAT_XRGB8888:
        case LV_COLOR_FORMAT_RGB888:
        case LV_COLOR_FORMAT_I1:
        case LV_COLOR_FORMAT_I2:
        case LV_COLOR_FORMAT_I4:
        default:
            return false;
    }
}

static void _sw_transform(lv_draw_unit_t * draw_unit, const lv_draw_image_dsc_t * draw_dsc,
                          const lv_draw_buf_t * src_buf, const lv_area_t * coords)
{
    void (* transform_fn)(void *, const void *, uint16_t, uint16_t, uint16_t, uint16_t,
            int16_t, int16_t, uint16_t, uint16_t, const sw_matrix_t *) = NULL;
    void (* transform_index_fn)(void *, const void *, const uint32_t *, uint16_t, uint16_t,
            uint16_t, uint16_t, int16_t, int16_t, uint16_t, uint16_t, const sw_matrix_t *) = NULL;
    void (* transform_alpha_fn)(void *dst, const void *src, uint32_t src_color,
		    uint16_t dst_pitch, uint16_t src_pitch, uint16_t src_w, uint16_t src_h,
		    int16_t x, int16_t y, uint16_t w, uint16_t h, const sw_matrix_t *matrix) = NULL;
    const uint32_t * src_palette = (uint32_t *)src_buf->data;
    const uint8_t * src_data = src_buf->data;
    lv_draw_buf_t * dest_buf = draw_unit->target_layer->draw_buf;

    if(dest_buf->header.cf == LV_COLOR_FORMAT_RGB565) {
        switch(src_buf->header.cf) {
            case LV_COLOR_FORMAT_ARGB8565:
                transform_fn = sw_transform_argb8565_over_rgb565;
                break;
            case LV_COLOR_FORMAT_I8:
                transform_index_fn = sw_transform_index8_over_rgb565;
                src_data += 1024;
                break;
            case LV_COLOR_FORMAT_A8:
                transform_alpha_fn = sw_transform_a8_over_rgb565;
                break;
            default:
                LV_LOG_ERROR("Unsupported src cf %x", src_buf->header.cf);
                return;
        }
    }
    else if(dest_buf->header.cf == LV_COLOR_FORMAT_ARGB8565) {
        switch(src_buf->header.cf) {
            case LV_COLOR_FORMAT_ARGB8565:
                transform_fn = sw_transform_argb8565_over_argb8565;
                break;
            case LV_COLOR_FORMAT_I8:
                transform_index_fn = sw_transform_index8_over_argb8565;
                src_data += 1024;
                break;
            case LV_COLOR_FORMAT_A8:
                transform_alpha_fn = sw_transform_a8_over_argb8565;
                break;
            default:
                LV_LOG_ERROR("Unsupported src cf %x", src_buf->header.cf);
                return;
        }
    }
    else if(dest_buf->header.cf == LV_COLOR_FORMAT_RGB888) {
        switch(src_buf->header.cf) {
            case LV_COLOR_FORMAT_ARGB8565:
                transform_fn = sw_transform_argb8565_over_rgb888;
                break;
            case LV_COLOR_FORMAT_I8:
                transform_index_fn = sw_transform_index8_over_rgb888;
                src_data += 1024;
                break;
            case LV_COLOR_FORMAT_A8:
                transform_alpha_fn = sw_transform_a8_over_rgb888;
                break;
            default:
                LV_LOG_ERROR("Unsupported src cf %x", src_buf->header.cf);
                return;
        }
    }
    else if(dest_buf->header.cf == LV_COLOR_FORMAT_ARGB8888) {
        switch(src_buf->header.cf) {
            case LV_COLOR_FORMAT_ARGB8565:
                transform_fn = sw_transform_argb8565_over_argb8888;
                break;
            case LV_COLOR_FORMAT_I8:
                transform_index_fn = sw_transform_index8_over_argb8888;
                src_data += 1024;
                break;
            case LV_COLOR_FORMAT_A8:
                transform_alpha_fn = sw_transform_a8_over_argb8888;
                break;
            default:
                LV_LOG_ERROR("Unsupported src cf %x", src_buf->header.cf);
                return;
        }
    }
    else {
        LV_LOG_ERROR("Unsupported dest cf %x", dest_buf->header.cf);
        return;
    }

    if(transform_fn || transform_index_fn || transform_alpha_fn) {
        sw_matrix_t matrix;
        uint8_t *dest8 = (uint8_t *)lv_draw_buf_goto_xy(dest_buf,
                    draw_unit->clip_area->x1 - draw_unit->target_layer->buf_area.x1,
                    draw_unit->clip_area->y1 - draw_unit->target_layer->buf_area.y1);
        uint16_t dest_w = lv_area_get_width(draw_unit->clip_area);
        uint16_t dest_h = lv_area_get_height(draw_unit->clip_area);
        int16_t src_ofs_x = draw_unit->clip_area->x1 - coords->x1;
        int16_t src_ofs_y = draw_unit->clip_area->y1 - coords->y1;

        sw_transform_config(0, 0, draw_dsc->pivot.x, draw_dsc->pivot.y,
                draw_dsc->rotation, draw_dsc->scale_x, draw_dsc->scale_y, 8, &matrix);

        if(transform_fn) {
            transform_fn(dest8, src_data, dest_buf->header.stride,
                    src_buf->header.stride, src_buf->header.w, src_buf->header.h,
                    src_ofs_x, src_ofs_y, dest_w, dest_h, &matrix);
        }
        else if(transform_index_fn) {
            transform_index_fn(dest8, src_data, src_palette,
                    dest_buf->header.stride, src_buf->header.stride,
                    src_buf->header.w, src_buf->header.h,
                    src_ofs_x, src_ofs_y, dest_w, dest_h, &matrix);
        }
        else {
            uint32_t color32 = (draw_dsc->opa << 24) +  (draw_dsc->recolor.red << 16) +
                    (draw_dsc->recolor.green << 8) + (draw_dsc->recolor.blue);
            transform_alpha_fn(dest8, src_data, color32,
                    dest_buf->header.stride, src_buf->header.stride,
                    src_buf->header.w, src_buf->header.h,
                    src_ofs_x, src_ofs_y, dest_w, dest_h, &matrix);
        }
    }
}

static lv_result_t _open_image_decoder(lv_image_decoder_dsc_t *decoder_dsc,
                                       const void *src, bool no_cache)
{
    lv_image_decoder_args_t args;
    lv_memzero(&args, sizeof(lv_image_decoder_args_t));
    args.stride_align = false;
    args.use_indexed = true;
    args.no_cache = no_cache;

    lv_result_t res = lv_image_decoder_open(decoder_dsc, src, &args);
    if(res != LV_RESULT_OK) {
        LV_LOG_ERROR("Failed to open image");
        return res;
    }

    const lv_draw_buf_t * decoded = decoder_dsc->decoded;
    if(decoded == NULL || decoded->data == NULL) {
        lv_image_decoder_close(decoder_dsc);
        LV_LOG_ERROR("image data is NULL");
        return LV_RESULT_INVALID;
    }

    return LV_RESULT_OK;
}

static void _sw_draw_image(lv_draw_unit_t * draw_unit, const lv_draw_image_dsc_t * draw_dsc,
                           const lv_area_t * coords, bool no_cache)
{
    lv_image_decoder_dsc_t decoder_dsc;

    if(draw_dsc->opa <= LV_OPA_MIN)
        return;

    LV_PROFILER_BEGIN;

    if(draw_dsc->tile) {
        int32_t img_w = draw_dsc->header.w;
        int32_t img_h = draw_dsc->header.h;

        lv_area_t tile_area;
        if(lv_area_get_width(&draw_dsc->image_area) >= 0) {
            tile_area = draw_dsc->image_area;
        }
        else {
            tile_area = *coords;
        }
        lv_area_set_width(&tile_area, img_w);
        lv_area_set_height(&tile_area, img_h);

        if(_open_image_decoder(&decoder_dsc, draw_dsc->src, no_cache) != LV_RESULT_OK) {
            LV_PROFILER_END;
            return;
        }

        const lv_area_t *clip_area_ori = draw_unit->clip_area;

        int32_t tile_x_start = tile_area.x1;
        int32_t tile_x_end = LV_MIN(clip_area_ori->x2, coords->x2);
        int32_t tile_y_end = LV_MIN(clip_area_ori->y2, coords->y2);

        while(tile_area.y1 <= tile_y_end) {
            while(tile_area.x1 <= tile_x_end) {
                lv_area_t clip_area;
                if(lv_area_intersect(&clip_area, &tile_area, clip_area_ori)) {
                    lv_area_t rel_img_area = clip_area;
                    lv_area_move(&rel_img_area, -tile_area.x1, -tile_area.y1);

                    draw_unit->clip_area = &clip_area;
                    lv_draw_actsw_blit(draw_unit, decoder_dsc.decoded,
                             &rel_img_area, draw_dsc->recolor, draw_dsc->opa);
                }

                tile_area.x1 += img_w;
                tile_area.x2 += img_w;
            }

            tile_area.y1 += img_h;
            tile_area.y2 += img_h;
            tile_area.x1 = tile_x_start;
            tile_area.x2 = tile_x_start + img_w - 1;
        }

        draw_unit->clip_area = clip_area_ori;
        lv_image_decoder_close(&decoder_dsc);
    }
    else {
        lv_area_t draw_area;

        if(draw_dsc->rotation || draw_dsc->scale_x != LV_SCALE_NONE || draw_dsc->scale_y != LV_SCALE_NONE) {
            int32_t w = lv_area_get_width(coords);
            int32_t h = lv_area_get_height(coords);

            lv_image_buf_get_transformed_area(&draw_area, w, h, draw_dsc->rotation,
                    draw_dsc->scale_x, draw_dsc->scale_y, &draw_dsc->pivot);
            lv_area_move(&draw_area, coords->x1, coords->y1);
        }
        else {
            lv_area_copy(&draw_area, coords);
        }

        if(!lv_area_intersect(&draw_area, &draw_area, draw_unit->clip_area)) {
            LV_PROFILER_END;
            return;
        }

        if(_open_image_decoder(&decoder_dsc, draw_dsc->src, no_cache) != LV_RESULT_OK) {
            LV_PROFILER_END;
            return;
        }

        const lv_area_t *clip_area_ori = draw_unit->clip_area;
        draw_unit->clip_area = &draw_area;

        if(draw_dsc->rotation || draw_dsc->scale_x != LV_SCALE_NONE || draw_dsc->scale_y != LV_SCALE_NONE) {
            _sw_transform(draw_unit, draw_dsc, decoder_dsc.decoded, coords);
        }
        else {
            lv_area_t rel_img_area;
            lv_area_copy(&rel_img_area, &draw_area);
            lv_area_move(&rel_img_area, -coords->x1, -coords->y1);

            lv_draw_actsw_blit(draw_unit, decoder_dsc.decoded, &rel_img_area,
                     draw_dsc->recolor, draw_dsc->opa);
        }

        draw_unit->clip_area = clip_area_ori;

        lv_image_decoder_close(&decoder_dsc);

        LV_PROFILER_END;
    }
}

static void _sw_draw_layer(lv_draw_unit_t * draw_unit,
              const lv_draw_image_dsc_t * draw_dsc, const lv_area_t * coords)
{
    lv_layer_t * layer_to_draw = (lv_layer_t *)draw_dsc->src;
    if(layer_to_draw->draw_buf == NULL)
        return;

    LV_PROFILER_BEGIN;

    lv_draw_image_dsc_t new_draw_dsc = *draw_dsc;
    new_draw_dsc.src = layer_to_draw->draw_buf;

    _sw_draw_image(draw_unit, &new_draw_dsc, coords, true);

    LV_PROFILER_END;
}

static void _sw_draw_glyph(lv_draw_unit_t * draw_unit, lv_draw_glyph_dsc_t * glyph_dsc)
{
    const lv_area_t * letter_coords = glyph_dsc->letter_coords;

    if(glyph_dsc->opa <= LV_OPA_MIN) return;

    lv_area_t blend_area;
    if(!lv_area_intersect(&blend_area, letter_coords, draw_unit->clip_area)) return;

    lv_area_t rel_coords = blend_area;
    lv_area_move(&rel_coords, -letter_coords->x1, -letter_coords->y1);

    const lv_area_t *clip_area_ori = draw_unit->clip_area;
    draw_unit->clip_area = &blend_area;

    lv_draw_actsw_blit(draw_unit, glyph_dsc->glyph_data, &rel_coords,
             glyph_dsc->color, glyph_dsc->opa);

    draw_unit->clip_area = clip_area_ori;
}

static void _sw_draw_letter_cb(lv_draw_unit_t * draw_unit,
                lv_draw_glyph_dsc_t * glyph_dsc, lv_draw_fill_dsc_t * fill_dsc,
                const lv_area_t * fill_area)
{
    if(glyph_dsc) {
        switch(glyph_dsc->format) {
            case LV_FONT_GLYPH_FORMAT_A1:
            case LV_FONT_GLYPH_FORMAT_A2:
            case LV_FONT_GLYPH_FORMAT_A4:
            case LV_FONT_GLYPH_FORMAT_A8: {
                _sw_draw_glyph(draw_unit, glyph_dsc);
                break;
            }

            case LV_FONT_GLYPH_FORMAT_IMAGE: {
#if LV_USE_IMGFONT
                lv_draw_image_dsc_t img_dsc;
                lv_draw_image_dsc_init(&img_dsc);
                img_dsc.opa = glyph_dsc->opa;
                img_dsc.src = glyph_dsc->glyph_data;
                _sw_draw_image(draw_unit, &img_dsc, glyph_dsc->letter_coords, true);
#endif
                break;
            }

            case LV_FONT_GLYPH_FORMAT_NONE: {
#if LV_USE_FONT_PLACEHOLDER
                /* Draw a placeholder rectangle*/
                lv_draw_border_dsc_t border_draw_dsc;
                lv_draw_border_dsc_init(&border_draw_dsc);
                border_draw_dsc.opa = glyph_dsc->opa;
                border_draw_dsc.color = glyph_dsc->color;
                border_draw_dsc.width = 1;
                lv_draw_sw_border(draw_unit, &border_draw_dsc, glyph_dsc->bg_coords);
#endif
                break;
            }

            default:
                LV_LOG_WARN("Invalid font glyph format %d", glyph_dsc->format);
                break;
        }
    }

    if(fill_dsc && fill_area) {
        lv_draw_sw_fill(draw_unit, fill_dsc, fill_area);
    }
}

static void _sw_draw_label(lv_draw_unit_t * draw_unit,
                const lv_draw_label_dsc_t * dsc, const lv_area_t * coords)
{
  if(dsc->opa <= LV_OPA_MIN) return;

  LV_PROFILER_BEGIN;
  lv_draw_label_iterate_characters(draw_unit, dsc, coords, _sw_draw_letter_cb);
  LV_PROFILER_END;
}

static void _sw_draw_execute(lv_draw_actsw_unit_t * u)
{
    lv_draw_unit_t * draw_unit = (lv_draw_unit_t *)u;
    lv_draw_task_t * task = u->task_act;

    switch(task->type) {
        case LV_DRAW_TASK_TYPE_IMAGE:
            _sw_draw_image(draw_unit, task->draw_dsc, &task->area, false);
            break;

        case LV_DRAW_TASK_TYPE_LAYER:
            _sw_draw_layer(draw_unit, task->draw_dsc, &task->area);
            break;

        case LV_DRAW_TASK_TYPE_LABEL:
            _sw_draw_label(draw_unit, task->draw_dsc, &task->area);
            break;

        default:
            break;
    }
}

static int32_t _sw_draw_dispatch(lv_draw_unit_t * draw_unit, lv_layer_t * layer)
{
    lv_draw_actsw_unit_t * u = (lv_draw_actsw_unit_t *)draw_unit;

    /* Return immediately if it's busy with draw task. */
    if(u->task_act) return 0;

    /* Try to get an ready to draw. */
    lv_draw_task_t * t = lv_draw_get_next_available_task(layer, NULL, ACTSW_DRAW_UNIT_ID);

    /* Return 0 is no selection, some tasks can be supported by other units. */
    if(t == NULL || t->preferred_draw_unit_id != ACTSW_DRAW_UNIT_ID) return LV_DRAW_UNIT_IDLE;

    void * buf = lv_draw_layer_alloc_buf(layer);
    if(buf == NULL) return LV_DRAW_UNIT_IDLE;

    t->state = LV_DRAW_TASK_STATE_IN_PROGRESS;
    u->base_unit.target_layer = layer;
    u->base_unit.clip_area = &t->clip_area;
    u->task_act = t;

    _sw_draw_execute(u);

    u->task_act->state = LV_DRAW_TASK_STATE_READY;
    u->task_act = NULL;

    /* Request a new dispatching as it can get a new task. */
    lv_draw_dispatch_request();

    return 1;
}

static int32_t _sw_draw_dispatch_task(lv_draw_unit_t * draw_unit, lv_layer_t * layer, lv_draw_task_t * task)
{
    lv_draw_actsw_unit_t * u = (lv_draw_actsw_unit_t *)draw_unit;

    /* Return immediately if it's busy with draw task. */
    if(u->task_act) return 0;

    void * buf = lv_draw_layer_alloc_buf(layer);
    if(buf == NULL) return LV_DRAW_UNIT_IDLE;

    task->state = LV_DRAW_TASK_STATE_IN_PROGRESS;
    u->base_unit.target_layer = layer;
    u->base_unit.clip_area = &task->clip_area;
    u->task_act = task;

    _sw_draw_execute(u);

    u->task_act->state = LV_DRAW_TASK_STATE_READY;
    u->task_act = NULL;

    return 1;
}

static int32_t _sw_draw_evaluate(lv_draw_unit_t * draw_unit, lv_draw_task_t * task)
{
    LV_UNUSED(draw_unit);

    lv_draw_dsc_base_t * draw_base_dsc = task->draw_dsc;
    if(!_sw_dest_cf_supported(draw_base_dsc->layer->color_format))
        return 0;

    switch(task->type) {
        case LV_DRAW_TASK_TYPE_IMAGE:
        case LV_DRAW_TASK_TYPE_LAYER: {
            const lv_draw_image_dsc_t * draw_dsc = task->draw_dsc;

            if(draw_dsc->blend_mode != LV_BLEND_MODE_NORMAL ||
                draw_dsc->bitmap_mask_src || draw_dsc->skew_x || draw_dsc->skew_y)
                return 0;

            if(task->type == LV_DRAW_TASK_TYPE_IMAGE) {
                if(!_sw_src_cf_supported(draw_dsc->header.cf) ||
                    (draw_dsc->header.flags & (LV_IMAGE_FLAGS_COMPRESSED | LV_IMAGE_FLAGS_PREMULTIPLIED)))
                    return 0;

                if((draw_dsc->recolor_opa != LV_OPA_TRANSP || draw_dsc->opa < LV_OPA_MAX) &&
                    (draw_dsc->header.cf < LV_COLOR_FORMAT_A1 || draw_dsc->header.cf > LV_COLOR_FORMAT_A8))
                    return 0;
            }
            else {
                lv_layer_t * layer_to_draw = (lv_layer_t *)draw_dsc->src;
                if(!_sw_src_cf_supported(layer_to_draw->color_format))
                    return 0;
            }

            break;
        }

        case LV_DRAW_TASK_TYPE_LABEL: {
            const lv_draw_label_dsc_t * draw_dsc = task->draw_dsc;
            if(draw_dsc->blend_mode != LV_BLEND_MODE_NORMAL)
                return 0;

            const lv_font_fmt_txt_dsc_t * font_dsc = draw_dsc->font->dsc;
            if(font_dsc &&
                (font_dsc->bpp < LV_FONT_GLYPH_FORMAT_A1 ||
                 font_dsc->bpp > LV_FONT_GLYPH_FORMAT_IMAGE))
                return 0;

            break;
        }

        default:
            return 0;
    }

    if(task->preference_score > ACTSW_DRAW_PREFERENCE_SCORE) {
        task->preference_score = ACTSW_DRAW_PREFERENCE_SCORE;
        task->preferred_draw_unit_id = ACTSW_DRAW_UNIT_ID;
    }

    return 1;
}

static int32_t _sw_draw_delete(lv_draw_unit_t * draw_unit)
{
    LV_UNUSED(draw_unit);
    return 1;
}

#endif /*LV_USE_ACTIONS*/
