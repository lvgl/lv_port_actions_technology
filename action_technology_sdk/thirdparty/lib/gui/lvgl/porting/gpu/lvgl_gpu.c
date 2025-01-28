/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*********************
 *      INCLUDES
 *********************/

#include "lvgl_gpu.h"
#include "lvgl_decoder.h"
#include <memory/mem_cache.h>

#if defined(CONFIG_UI_MEMORY_MANAGER) && CONFIG_UI_RES_MEM_POOL_SIZE > 0
    #include <ui_mem.h>
#endif

#if LV_USE_DRAW_VG_LITE
    #include <vg_lite/vg_lite.h>
    #include "../../src/draw/vg_lite/lv_draw_vg_lite.h"
#endif

/*********************
 *      DEFINES
 *********************/
#define default_handlers LV_GLOBAL_DEFAULT()->draw_buf_handlers
#define font_draw_buf_handlers LV_GLOBAL_DEFAULT()->font_draw_buf_handlers
#define image_cache_draw_buf_handlers LV_GLOBAL_DEFAULT()->image_cache_draw_buf_handlers

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

#ifndef _WIN32
static void draw_buf_flush_cache_cb(const lv_draw_buf_t * draw_buf, const lv_area_t * area);
static void draw_buf_clear_cb(lv_draw_buf_t * draw_buf, const lv_area_t * area);
static void draw_buf_copy_cb(lv_draw_buf_t * dest, const lv_area_t * dest_area,
                      const lv_draw_buf_t * src, const lv_area_t * src_area);
#endif

#if defined(CONFIG_UI_MEMORY_MANAGER) && CONFIG_UI_RES_MEM_POOL_SIZE > 0
static void * draw_buf_malloc_cb(size_t size, lv_color_format_t color_format);
static void draw_buf_free_cb(void * buf);
#endif

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

#if LV_VG_LITE_USE_GPU_INIT
void gpu_init(void)
{
    vg_lite_set_command_buffer_size(CONFIG_LV_VG_LITE_COMMAND_BUFFER_SIZE << 10);
    /* FIXME:
     * Suggest max resolution is 480x480
     */
    vg_lite_init(480, 480);
}
#endif

#if LV_USE_DRAW_CUSTOM_GPU_INIT
void lv_draw_custom_gpu_init(void)
{
    lv_gpu_draw_buf_init_handlers();

    /* FIXME: Make sure vglite dispatch() invoked first due to its implementation */
    lv_gpu_draw_swrender_init();

#if LV_USE_DRAW_ACTS_DMA2D
    lv_gpu_draw_dma2d_init();
#endif

    lvgl_decoder_init();
}
#endif

void lv_gpu_draw_buf_init_handlers(void)
{
    lv_draw_buf_handlers_t * handlers[] = {
        &font_draw_buf_handlers,
        &default_handlers,
        &image_cache_draw_buf_handlers,
    };

    for (int i = 0; i < sizeof(handlers) / sizeof(handlers[0]); i++) {
        handlers[i]->width_to_stride_cb = lv_gpu_draw_buf_width_to_stride;

#ifndef _WIN32
        handlers[i]->flush_cache_cb = draw_buf_flush_cache_cb;
        handlers[i]->invalidate_cache_cb = draw_buf_flush_cache_cb;

        if (i > 0) {
            handlers[i]->clear_cb = draw_buf_clear_cb;
            handlers[i]->copy_cb = draw_buf_copy_cb;
        }
#endif

#if defined(CONFIG_UI_MEMORY_MANAGER) && CONFIG_UI_RES_MEM_POOL_SIZE > 0
        handlers[i]->buf_malloc_cb = draw_buf_malloc_cb;
        handlers[i]->buf_free_cb = draw_buf_free_cb;
#endif
    }
}

uint32_t lv_gpu_draw_buf_width_to_stride(uint32_t w, lv_color_format_t color_format)
{
    uint32_t width_byte;

    if (color_format == LV_COLOR_FORMAT_ETC2_EAC) {
        width_byte = (w + 3) & ~3;
    }
    else {
        width_byte = (w * lv_color_format_get_bpp(color_format) + 7) >> 3; /*Round up*/
    }

    return width_byte;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#ifndef _WIN32
static void draw_buf_flush_cache_cb(const lv_draw_buf_t * draw_buf, const lv_area_t * area)
{
    if (mem_is_cacheable(draw_buf->data)) {
        uint8_t bpp = lv_color_format_get_bpp(draw_buf->header.cf);
        const uint8_t *data = draw_buf->data + draw_buf->header.stride * area->y1 + area->x1 * bpp / 8;
        uint32_t size = lv_area_get_height(area) * draw_buf->header.stride + 1; /* add 1 more byte for bpp < 8 */

        mem_dcache_flush(data, size);
        /* mem_dcache_sync(); */
    }
}

static void draw_buf_clear_cb(lv_draw_buf_t * draw_buf, const lv_area_t * area)
{
#if LV_USE_DRAW_ACTS_DMA2D
    if (lv_gpu_draw_buf_dma2d_clear(draw_buf, area) == LV_RESULT_OK) return;
#endif

    lv_gpu_draw_buf_sw_clear(draw_buf, area);
}

static void draw_buf_copy_cb(lv_draw_buf_t * dest, const lv_area_t * dest_area,
                      const lv_draw_buf_t * src, const lv_area_t * src_area)
{
#if LV_USE_DRAW_ACTS_DMA2D
    if (lv_gpu_draw_buf_dma2d_copy(dest, dest_area, src, src_area) == LV_RESULT_OK) return;
#endif

    lv_gpu_draw_buf_sw_copy(dest, dest_area, src, src_area);
}
#endif /* _WIN32 */

#if defined(CONFIG_UI_MEMORY_MANAGER) && CONFIG_UI_RES_MEM_POOL_SIZE > 0
static void * draw_buf_malloc_cb(size_t size, lv_color_format_t color_format)
{
    return ui_mem_aligned_alloc(MEM_RES, LV_DRAW_BUF_ALIGN, size, __func__);
}

static void draw_buf_free_cb(void * buf)
{
    ui_mem_free(MEM_RES, buf);
}
#endif
