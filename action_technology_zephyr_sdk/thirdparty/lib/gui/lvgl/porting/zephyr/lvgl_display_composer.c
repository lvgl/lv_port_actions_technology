/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(CONFIG_UI_SERVICE) && defined(CONFIG_DISPLAY_COMPOSER)

/*********************
 *      INCLUDES
 *********************/
#include <memory/mem_cache.h>
#include <ui_mem.h>
#include <ui_surface.h>
#ifdef CONFIG_DMA2D_HAL
    #include <dma2d_hal.h>
#endif
#include <display/display_hal.h>
#include <display/display_composer.h>
#include <lvgl/lvgl_virtual_display.h>
#include "lvgl_inner.h"

/*********************
 *      DEFINES
 *********************/
#if LV_COLOR_DEPTH == 16
#  define SURFACE_PIXEL_FORMAT HAL_PIXEL_FORMAT_RGB_565
#elif LV_COLOR_DEPTH == 24
#  define SURFACE_PIXEL_FORMAT HAL_PIXEL_FORMAT_RGB_888
#else
#  define SURFACE_PIXEL_FORMAT HAL_PIXEL_FORMAT_XRGB_8888
#endif

/**********************
 *      TYPEDEFS
 **********************/

typedef struct lvgl_disp_user_data {
    bool initialized;
    bool in_overlay;

    uint8_t pm_state;

    surface_t *surface;
    const lvx_display_observer_t *observer;

    struct k_sem ready_sem;
} lvgl_disp_user_data_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void _display_vsync_cb(const struct display_callback *callback, uint32_t timestamp);
static void _display_pm_notify_cb(const struct display_callback *callback, uint32_t pm_action);

static void _surface_post_handler(uint32_t event, void *data, void *user_data);

static int _init_overlay_layer(ui_layer_t *layer, graphic_buffer_t *gbuf,
                               const lv_draw_buf_t * buf, const lv_point_t * pos,
                               lv_opa_t opa);

/**********************
 *  STATIC VARIABLES
 **********************/
static const struct display_callback s_disp_callback = {
    .vsync = _display_vsync_cb,
    .pm_notify = _display_pm_notify_cb,
};

static lvgl_disp_user_data_t s_disp_data;

static OS_SEM_DEFINE(s_disp_sem, 0, 1);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
int lv_port_display_init(void)
{
    uint16_t hor_res = 0;
    uint16_t ver_res = 0;
    uint8_t flags = 0;

    if (display_composer_init()) {
        LV_LOG_ERROR("composer init failed");
        return LV_RESULT_INVALID;
    }

    display_composer_register_callback(&s_disp_callback);
    display_composer_get_geometry(&hor_res, &ver_res, NULL, NULL);

    switch (display_composer_get_orientation()) {
    case 90:
        flags = SURFACE_ROTATED_90;
        break;
    case 180:
        flags = SURFACE_ROTATED_180;
        break;
    case 270:
        flags = SURFACE_ROTATED_270;
        break;
    default:
        break;
    }

    k_sem_init(&s_disp_data.ready_sem, 0, 1);
    s_disp_data.pm_state = LVX_DISPLAY_STATE_ON;

    surface_t *surface = surface_create(hor_res, ver_res, SURFACE_PIXEL_FORMAT, 2, flags);
    if (surface == NULL) {
        LV_LOG_ERROR("surface_create failed");
        return LV_RESULT_INVALID;
    }

    surface_register_callback(surface, SURFACE_CB_POST, _surface_post_handler, surface);

    lv_display_t *disp = lvgl_virtual_display_create(surface);
    if (disp == NULL) {
        LV_LOG_ERROR("display_create failed");
        surface_destroy(surface);
        return LV_RESULT_INVALID;
    }

    lvgl_virtual_display_set_focus(disp, true);

    s_disp_data.surface = surface;

    /* wait display ready */
    os_sem_take(&s_disp_data.ready_sem, OS_FOREVER);
    return 0;
}

lv_result_t lvx_display_add_observer(const lvx_display_observer_t * observer)
{
    s_disp_data.observer = observer;
    return LV_RESULT_OK;
}

lv_result_t lvx_display_flush_wait(void)
{
    if (s_disp_data.surface == NULL)
        return LV_RESULT_INVALID;

    surface_wait_for_refresh(s_disp_data.surface, -1);

    return (display_composer_flush(100) < 0) ? LV_RESULT_INVALID : LV_RESULT_OK;
}

lv_result_t lvx_display_read_to_draw_buf(lv_draw_buf_t * draw_buf)
{
    graphic_buffer_t *gbuf;
    uint32_t required_size;
    int res;

    res = lvx_display_flush_wait();
    if (res < 0)
        return LV_RESULT_INVALID;

    gbuf = surface_get_post_buffer(s_disp_data.surface);
    required_size = graphic_buffer_get_bytes_per_line(gbuf) * gbuf->height;
    if (draw_buf == NULL || draw_buf->data_size < required_size)
        return LV_RESULT_INVALID;

    draw_buf->header.cf = LV_COLOR_FORMAT_NATIVE;
    draw_buf->header.w = graphic_buffer_get_width(gbuf);
    draw_buf->header.h = graphic_buffer_get_height(gbuf);
    draw_buf->header.stride = graphic_buffer_get_bytes_per_line(gbuf);

    memcpy(draw_buf->data, gbuf->data, required_size);
    return LV_RESULT_OK;
}

void * lvx_display_overlay_prepare(void)
{
    graphic_buffer_t *gbuf = NULL;

    if (lvx_display_flush_wait())
        return NULL;

    surface_set_max_buffer_count(s_disp_data.surface, 1);
    gbuf = surface_get_post_buffer(s_disp_data.surface);

#ifdef CONFIG_DMA2D_HAL
    hal_dma2d_set_global_enabled(false);
#endif

    s_disp_data.in_overlay = true;

    return (void *)gbuf->data;
}

lv_result_t lvx_display_overlay_unprepare(void)
{
    display_composer_flush(-1);
    surface_set_min_buffer_count(s_disp_data.surface, 2);

#ifdef CONFIG_DMA2D_HAL
    hal_dma2d_set_global_enabled(true);
#endif

    s_disp_data.in_overlay = false;
    return LV_RESULT_OK;
}

lv_result_t lvx_display_overlay_flush(const lv_draw_buf_t * bg_buf, const lv_point_t * bg_pos,
                                      const lv_draw_buf_t * fg_buf, const lv_point_t * fg_pos,
                                      lv_opa_t fg_opa)
{
    ui_layer_t layers[2];
    graphic_buffer_t gbufs[2];
    int num = 1;

    if (s_disp_data.in_overlay == false)
        return LV_RESULT_INVALID;

    if (_init_overlay_layer(&layers[0], &gbufs[0], bg_buf, bg_pos, LV_OPA_COVER))
        return LV_RESULT_INVALID;

    if (!_init_overlay_layer(&layers[1], &gbufs[1], fg_buf, fg_pos, fg_opa))
        num = 2;

    display_composer_post(layers, num, POST_FULL_FRAME);
    return LV_RESULT_OK;
}

/**********************
 *  STATIC FUNCTIONS
 **********************/
static void _display_vsync_cb(const struct display_callback *callback, uint32_t timestamp)
{
    if (!s_disp_data.initialized) {
        s_disp_data.initialized = true;
        k_sem_give(&s_disp_data.ready_sem);
    }

    lv_port_indev_pointer_scan();

    if (s_disp_data.observer && s_disp_data.observer->vsync_cb) {
        s_disp_data.observer->vsync_cb(s_disp_data.observer, timestamp);
    }
}

static void _display_pm_notify_cb(const struct display_callback *callback, uint32_t pm_action)
{
    switch (pm_action) {
    case PM_DEVICE_ACTION_LATE_RESUME:
        s_disp_data.pm_state = LVX_DISPLAY_STATE_ON;
        break;
    case PM_DEVICE_ACTION_LOW_POWER:
        s_disp_data.pm_state = LVX_DISPLAY_STATE_IDLE;
        break;
    case PM_DEVICE_ACTION_EARLY_SUSPEND:
    default:
        s_disp_data.pm_state = LVX_DISPLAY_STATE_OFF;
        break;
    }

    if (s_disp_data.observer && s_disp_data.observer->state_cb) {
        s_disp_data.observer->state_cb(s_disp_data.observer, s_disp_data.pm_state);
    }
}

static void _view_surface_post_cleanup(void *surface)
{
    surface_complete_one_post(surface);
}

static void _surface_post_handler(uint32_t event, void *data, void *user_data)
{
    const surface_post_data_t *post_data = data;
    surface_t *surface = user_data;
    ui_layer_t layer;

    if (s_disp_data.in_overlay) {
        surface_complete_one_post(surface);
        return;
    }

    layer.buffer = surface_get_post_buffer(surface);
    layer.cleanup_cb = _view_surface_post_cleanup;
    layer.cleanup_data = surface;
    layer.blending = DISPLAY_BLENDING_NONE;

    memcpy(&layer.crop, post_data->area, sizeof(layer.crop));
    display_composer_round(&layer.crop);
    memcpy(&layer.frame, &layer.crop, sizeof(layer.crop));

    display_composer_post(&layer, 1, POST_FULL_FRAME);
}

static int _init_overlay_layer(ui_layer_t *layer, graphic_buffer_t *gbuf,
                               const lv_draw_buf_t * buf, const lv_point_t * pos,
                               lv_opa_t opa)
{
    lv_display_t *disp = lv_disp_get_default();
    lv_area_t disp_area = {
        0, 0, lv_display_get_horizontal_resolution(disp) - 1,
        lv_display_get_vertical_resolution(disp) - 1,
    };
    lv_area_t clip_area = {
        pos->x, pos->y, pos->x + buf->header.w - 1, pos->y + buf->header.h - 1,
    };

    if (buf == NULL || pos == NULL)
        return -EINVAL;

    if (lv_area_intersect(&clip_area, &clip_area, &disp_area) == false)
        return -EINVAL;

    if (graphic_buffer_init(gbuf, buf->header.w, buf->header.h,
                            SURFACE_PIXEL_FORMAT, 0, buf->header.stride / (LV_COLOR_DEPTH / 8),
                            (void *)buf->data))
        return -EINVAL;

    layer->buffer = gbuf;
    layer->cleanup_cb = NULL;
    layer->plane_alpha = opa;
    layer->blending = (opa >= LV_OPA_MAX) ? HAL_BLENDING_NONE : HAL_BLENDING_COVERAGE;
    layer->frame.x1 = clip_area.x1;
    layer->frame.y1 = clip_area.y1;
    layer->frame.x2 = clip_area.x2;
    layer->frame.y2 = clip_area.y2;
    layer->crop.x1 = clip_area.x1 - pos->x;
    layer->crop.y1 = clip_area.y1 - pos->y;
    layer->crop.x2 = clip_area.x2 - pos->x;
    layer->crop.y2 = clip_area.y2 - pos->y;

    return 0;
}

#endif /* !defined(CONFIG_UI_SERVICE) && defined(CONFIG_DISPLAY_COMPOSER) */
