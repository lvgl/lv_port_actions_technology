/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(CONFIG_UI_SERVICE) && !defined(CONFIG_DISPLAY_COMPOSER)

/*********************
 *      INCLUDES
 *********************/

#include <zephyr.h>
#include <board.h>
#include <sys/atomic.h>
#include <drivers/display.h>
#include <drivers/display/display_graphics.h>
#include <memory/mem_cache.h>
#include "lvgl_inner.h"

/**********************
 *      DEFINES
 **********************/

#if CONFIG_LV_VDB_NUM <= 0
#  error CONFIG_LV_VDB_NUM must greater than  0
#endif

#if CONFIG_LV_VDB_SIZE <= 0
#  error CONFIG_LV_VDB_SIZE must greater than  0
#endif

#define DEBUG_PERF_FPS 0

/**********************
 *      TYPEDEFS
 **********************/

typedef struct lvgl_disp_flush_data {
    uint8_t idx;
    lv_area_t area;
    const uint8_t * buf;
} lvgl_disp_flush_data_t;

typedef struct lvgl_disp_user_data {
    lv_display_t *disp;
    const struct device *disp_dev;    /* display device */
    struct display_callback disp_cb; /* display callback to register */
    struct display_capabilities disp_cap;  /* display capabilities */
    struct k_sem wait_sem;        /* display complete wait sem */
    struct k_sem ready_sem;

    uint8_t pm_state;
    bool initialized;

    bool flushing;
    uint8_t flush_idx;
    atomic_t flush_cnt;

    uint8_t flush_data_wr;
    uint8_t flush_data_rd;
    lvgl_disp_flush_data_t flush_data[CONFIG_LV_VDB_NUM];

    const lvx_display_observer_t *observer;

#if DEBUG_PERF_FPS
    uint32_t pref_last_time;
    uint8_t perf_frame_cnt;
#endif
} lvgl_disp_user_data_t;

/**********************
 *  STATIC VARIABLES
 **********************/

/* NOTE:
 * (1) depending on chosen color depth buffer may be accessed using uint8_t *,
 * uint16_t * or uint32_t *, therefore buffer needs to be aligned accordingly to
 * prevent unaligned memory accesses.
 * (2) must align each buffer address and size to psram cache line size (32 bytes)
 * if allocated in psram.
 * (3) Verisilicon vg_lite buffer memory requires 64 bytes aligned
 */
#ifdef CONFIG_VG_LITE
#  define BUFFER_ALIGN 64
#else
#  define BUFFER_ALIGN 32
#endif

#define BUFFER_SIZE ((CONFIG_LV_VDB_SIZE + (BUFFER_ALIGN - 1)) & ~(BUFFER_ALIGN - 1))

static uint8_t vdb_buf_0[BUFFER_SIZE] __aligned(BUFFER_ALIGN) __in_section_unique(lvgl.noinit.vdb.0);
#if CONFIG_LV_VDB_NUM > 1
static uint8_t vdb_buf_1[BUFFER_SIZE] __aligned(BUFFER_ALIGN) __in_section_unique(lvgl.noinit.vdb.1);
#endif

static lvgl_disp_user_data_t disp_user_data;

static K_SEM_DEFINE(s_disp_sem, 0, 1);

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void _display_vsync_cb(const struct display_callback *callback, uint32_t timestamp);
static void _display_complete_cb(const struct display_callback *callback);
static void _display_pm_notify_cb(const struct display_callback *callback, uint32_t pm_action);

static int lvgl_display_init(lvgl_disp_user_data_t * drv_data);
static void lvgl_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);
static void lvgl_flush_wait_cb(lv_display_t * disp);
static void lvgl_invalidate_area_cb(lv_event_t * e);
static void lvgl_round_draw_area(lv_display_t * disp, lv_area_t * area);
static void lvgl_round_flush_area(lv_display_t * disp, lv_area_t * area);
//static void lvgl_round_invalidate_area(lv_display_t * disp, lv_area_t * area);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

int lv_port_display_init(void)
{
    lvgl_disp_user_data_t *drv_data = &disp_user_data;

    drv_data->disp_dev = device_get_binding(CONFIG_LCD_DISPLAY_DEV_NAME);
    if (drv_data->disp_dev == NULL) {
        LV_LOG_ERROR("Display device not found.");
        return -ENODEV;
    }

    drv_data->pm_state = LVX_DISPLAY_STATE_ON;
    drv_data->flush_idx = 0;
    drv_data->flushing = false;
    drv_data->flush_data_wr = 0;
    drv_data->flush_data_rd = 0;
    atomic_set(&drv_data->flush_cnt, 0);

    k_sem_init(&drv_data->wait_sem, 0, 1);
    k_sem_init(&drv_data->ready_sem, 0, 1);

    drv_data->disp_cb.vsync = _display_vsync_cb;
    drv_data->disp_cb.complete = _display_complete_cb;
    drv_data->disp_cb.pm_notify = _display_pm_notify_cb,
    display_register_callback(drv_data->disp_dev, &drv_data->disp_cb);
    //display_blanking_off(drv_data->disp_dev);

    if (lvgl_display_init(drv_data)) {
        LV_LOG_ERROR("display init failed");
        return -EINVAL;
    }

    /* wait display ready */
    k_sem_take(&drv_data->ready_sem, K_FOREVER);
    return 0;
}

lv_result_t lvx_display_add_observer(const lvx_display_observer_t * observer)
{
    lv_display_t * disp = lv_disp_get_default();
    lvgl_disp_user_data_t *drv_data = lv_display_get_driver_data(disp);

    if (drv_data) {
        drv_data->observer = observer;
    }

    return LV_RESULT_OK;
}

lv_result_t lvx_display_flush_wait(void)
{
    lvgl_flush_wait_cb(lv_disp_get_default());
    return LV_RESULT_OK;
}

lv_result_t lvx_display_read_to_draw_buf(lv_draw_buf_t * draw_buf)
{
    return LV_RESULT_INVALID;
}

void * lvx_display_overlay_prepare(void)
{
    return NULL;
}

lv_result_t lvx_display_overlay_unprepare(void)
{
    return LV_RESULT_INVALID;
}

lv_result_t lvx_display_overlay_flush(const lv_draw_buf_t * bg_buf, const lv_point_t * bg_pos,
                                      const lv_draw_buf_t * fg_buf, const lv_point_t * fg_pos,
                                      lv_opa_t fg_opa)
{
    return LV_RESULT_INVALID;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void _display_flush(lvgl_disp_user_data_t *drv_data)
{
    lvgl_disp_flush_data_t *flush_data = &drv_data->flush_data[drv_data->flush_data_rd];
    struct display_buffer_descriptor desc;
    const uint8_t *flush_buf = flush_data->buf;

#if LV_COLOR_DEPTH == 16
    desc.pixel_format = PIXEL_FORMAT_BGR_565;
#elif LV_COLOR_DEPTH == 24
    desc.pixel_format = PIXEL_FORMAT_BGR_888;
#elif LV_COLOR_DEPTH == 32
    desc.pixel_format = PIXEL_FORMAT_XRGB_8888;
#else
#  error invalid LV_COLOR_DEPTH
#endif

    desc.width = lv_area_get_width(&flush_data->area);
    desc.height = lv_area_get_height(&flush_data->area);

    if (drv_data->disp->render_mode == LV_DISPLAY_RENDER_MODE_DIRECT) {
        desc.pitch = lv_display_get_horizontal_resolution(drv_data->disp) * (LV_COLOR_DEPTH / 8);
        flush_buf += flush_data->area.y1 * desc.pitch + flush_data->area.x1 * (LV_COLOR_DEPTH / 8);
    }
    else {
        desc.pitch = desc.width * (LV_COLOR_DEPTH / 8);
    }

    desc.buf_size = desc.pitch * desc.height;

    drv_data->flushing = true;
    if (++drv_data->flush_data_rd >= ARRAY_SIZE(drv_data->flush_data)) {
        drv_data->flush_data_rd = 0;
    }

    display_write(drv_data->disp_dev, flush_data->area.x1, flush_data->area.y1, &desc, flush_buf);
}

static void _display_drop_pending_flush(lvgl_disp_user_data_t *drv_data)
{
    unsigned int key = irq_lock();

    for (int cnt = atomic_get(&drv_data->flush_cnt); cnt > 0; cnt--) {
        lv_display_flush_ready(drv_data->disp);
    }

    atomic_set(&drv_data->flush_cnt, 0);
    drv_data->flush_data_rd = drv_data->flush_data_wr;
    drv_data->flushing = false;
    k_sem_give(&drv_data->wait_sem);

    irq_unlock(key);
}

static void _display_vsync_cb(const struct display_callback *callback, uint32_t timestamp)
{
    lvgl_disp_user_data_t *drv_data =
            CONTAINER_OF(callback, lvgl_disp_user_data_t, disp_cb);
    lvgl_disp_flush_data_t *flush_data = &drv_data->flush_data[drv_data->flush_data_rd];

    if (!drv_data->initialized) {
        drv_data->initialized = true;
        k_sem_give(&drv_data->ready_sem);
    }

    if (drv_data->flushing == false && atomic_get(&drv_data->flush_cnt) > 0 && flush_data->idx == 0) {
        _display_flush(drv_data);
    }

    lv_port_indev_pointer_scan();

    if (drv_data->observer && drv_data->observer->vsync_cb) {
        drv_data->observer->vsync_cb(drv_data->observer, timestamp);
    }
}

static void _display_complete_cb(const struct display_callback *callback)
{
    lvgl_disp_user_data_t *drv_data =
            CONTAINER_OF(callback, lvgl_disp_user_data_t, disp_cb);
    lvgl_disp_flush_data_t *flush_data = &drv_data->flush_data[drv_data->flush_data_rd];

    drv_data->flushing = false;

    if (atomic_dec(&drv_data->flush_cnt) > 1 && flush_data->idx > 0) {
        _display_flush(drv_data);
    }

    lv_display_flush_ready(drv_data->disp);
    k_sem_give(&drv_data->wait_sem);
}

static void _display_pm_notify_cb(const struct display_callback *callback, uint32_t pm_action)
{
    lvgl_disp_user_data_t *drv_data =
            CONTAINER_OF(callback, lvgl_disp_user_data_t, disp_cb);

    switch (pm_action) {
    case PM_DEVICE_ACTION_LATE_RESUME:
#if DEBUG_PERF_FPS
        drv_data->perf_frame_cnt = 0;
        drv_data->pref_last_time = lv_tick_get();
#endif
        drv_data->pm_state = LVX_DISPLAY_STATE_ON;
        break;
    case PM_DEVICE_ACTION_LOW_POWER:
         drv_data->pm_state = LVX_DISPLAY_STATE_IDLE;
         break;
    case PM_DEVICE_ACTION_EARLY_SUSPEND:
    default:
        drv_data->pm_state = LVX_DISPLAY_STATE_OFF;
        for (int try_cnt = 500; try_cnt > 0; --try_cnt) {
            if (atomic_get(&drv_data->flush_cnt) <= 0)
                break;

            k_msleep(1);
        }

        if (atomic_get(&drv_data->flush_cnt) > 0) {
            LV_LOG_WARN("drop flush cnt %d", atomic_get(&drv_data->flush_cnt));
            _display_drop_pending_flush(drv_data);
        }

        break;
    }

    if (drv_data->observer && drv_data->observer->state_cb) {
        drv_data->observer->state_cb(drv_data->observer, drv_data->pm_state);
    }
}

static int lvgl_display_init(lvgl_disp_user_data_t * drv_data)
{
    const struct device *display_dev = drv_data->disp_dev;

    display_get_capabilities(display_dev, &drv_data->disp_cap);

    lv_display_t * disp = lv_display_create(drv_data->disp_cap.x_resolution, drv_data->disp_cap.y_resolution);
    if (disp == NULL) {
        LV_LOG_ERROR("create display failed");
        return -ENOMEM;
    }

    lv_display_rotation_t rotation;
    switch (drv_data->disp_cap.current_orientation) {
    case DISPLAY_ORIENTATION_ROTATED_90:
        rotation = LV_DISPLAY_ROTATION_270;
        break;
    case DISPLAY_ORIENTATION_ROTATED_180:
        rotation = LV_DISPLAY_ROTATION_180;
        break;
    case DISPLAY_ORIENTATION_ROTATED_270:
        rotation = LV_DISPLAY_ROTATION_90;
        break;
    case DISPLAY_ORIENTATION_NORMAL:
    default:
        rotation = LV_DISPLAY_ROTATION_0;
        break;
    }

    lv_display_set_rotation(disp, rotation);

    lv_display_render_mode_t render_mode;
    uint32_t screen_buf_size = lv_display_get_horizontal_resolution(disp) *
            lv_display_get_vertical_resolution(disp) *
            lv_color_format_get_size(disp->color_format);
    if (rotation ==LV_DISPLAY_ROTATION_0 && screen_buf_size <= BUFFER_SIZE) {
        render_mode = LV_DISPLAY_RENDER_MODE_DIRECT;
    }
    else {
        render_mode = LV_DISPLAY_RENDER_MODE_PARTIAL;
    }

#if CONFIG_LV_VDB_NUM > 1
    lv_display_set_buffers(disp, vdb_buf_0, vdb_buf_1, BUFFER_SIZE, render_mode);
#else
    lv_display_set_buffers(disp, vdb_buf_0, NULL, BUFFER_SIZE, render_mode);
#endif

    lv_display_set_flush_cb(disp, lvgl_flush_cb);
    lv_display_set_flush_wait_cb(disp, lvgl_flush_wait_cb);
    lv_display_add_event_cb(disp, lvgl_invalidate_area_cb, LV_EVENT_INVALIDATE_AREA, NULL);

    lv_display_set_driver_data(disp, drv_data);

    lv_obj_t *bottom = lv_display_get_layer_bottom(disp);
    lv_obj_set_style_bg_color(bottom, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bottom, LV_OPA_COVER, LV_PART_MAIN);

    drv_data->disp = disp;
    return 0;
}

static void lvgl_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    lvgl_disp_user_data_t *drv_data = lv_display_get_driver_data(disp);
    lvgl_disp_flush_data_t *flush_data = &drv_data->flush_data[drv_data->flush_data_wr];

    if (lv_display_get_rotation(disp) != LV_DISPLAY_ROTATION_0) {
        LV_LOG_WARN("rotation not implemented yet");
        lv_display_flush_ready(disp);
        return;
    }

    if (disp->render_mode == LV_DISPLAY_RENDER_MODE_DIRECT) {
        if (drv_data->flush_idx == 0) {
            lv_memcpy(&flush_data->area, area, sizeof(*area));
        }
        else {
            _lv_area_join(&flush_data->area, &flush_data->area, area);
        }

         if (!lv_display_flush_is_last(disp)) {
            lv_display_flush_ready(disp);
            goto out_exit;
        }
    }

    /*Screen off, skip*/
    if (drv_data->pm_state == LVX_DISPLAY_STATE_OFF) {
        unsigned int key = irq_lock();
        lv_display_flush_ready(disp);
        irq_unlock(key);
        goto out_exit;
    }

    /*Update the flush buffer*/
    flush_data->buf = px_map;
    if (disp->render_mode == LV_DISPLAY_RENDER_MODE_DIRECT) {
        lvgl_round_flush_area(disp, &flush_data->area);
    }
    else {
        flush_data->idx = drv_data->flush_idx;
        lv_memcpy(&flush_data->area, area, sizeof(*area));
    }

    if (++drv_data->flush_data_wr >= ARRAY_SIZE(drv_data->flush_data)) {
        drv_data->flush_data_wr = 0;
    }

    if (atomic_inc(&drv_data->flush_cnt) == 0 && flush_data->idx > 0) {
        _display_flush(drv_data);
    }

#if DEBUG_PERF_FPS
    if (lv_display_flush_is_last(disp)) {
        drv_data->perf_frame_cnt++;

        uint32_t now = lv_tick_get();
        uint32_t elaps = now - drv_data->pref_last_time;
        if (elaps >= 1000) {
            uint32_t fps = drv_data->perf_frame_cnt * 1000 / elaps;
            LV_LOG("fps %u\n", fps);

            drv_data->pref_last_time = now;
            drv_data->perf_frame_cnt = 0;
        }
    }
#endif /* DEBUG_PERF_FPS */

out_exit:
    drv_data->flush_idx = lv_display_flush_is_last(disp) ? 0 : (drv_data->flush_idx + 1);
}

static void lvgl_flush_wait_cb(lv_display_t * disp)
{
    lvgl_disp_user_data_t *drv_data = lv_display_get_driver_data(disp);

    while (disp->flushing) {
        k_sem_take(&drv_data->wait_sem, K_FOREVER);
    }
}

static void lvgl_invalidate_area_cb(lv_event_t * e)
{
    lv_display_t *disp = lv_event_get_target(e);
    lv_area_t *area = lv_event_get_param(e);

    lvgl_round_draw_area(disp, area);
}

static void lvgl_round_draw_area(lv_display_t * disp, lv_area_t * area)
{
    lvgl_disp_user_data_t *drv_data = lv_display_get_driver_data(disp);
    struct display_capabilities *cap = &drv_data->disp_cap;

    /* For LCD constrain and DMA2D performance */
	if (!(cap->x_resolution & 0x1)) {
		area->x1 &= ~0x1;
		area->x2 |= 0x1;
	}

	if (disp->render_mode != LV_DISPLAY_RENDER_MODE_DIRECT && !(cap->y_resolution & 0x1)) {
		area->y1 &= ~0x1;
		area->y2 |= 0x1;
	}
}

static void lvgl_round_flush_area(lv_display_t * disp, lv_area_t * area)
{
    lvgl_disp_user_data_t *drv_data = lv_display_get_driver_data(disp);
    struct display_capabilities *cap = &drv_data->disp_cap;

    if (cap->screen_info & SCREEN_INFO_X_ALIGNMENT_WIDTH) {
        area->x1 = 0;
        area->x2 = cap->x_resolution - 1;
    }
    else if (!(cap->x_resolution & 0x1)) {
        area->x1 &= ~0x1;
        area->x2 |= 0x1;
    }

    if (cap->screen_info & SCREEN_INFO_Y_ALIGNMENT_HEIGHT) {
        area->y1 = 0;
        area->y2 = cap->y_resolution - 1;
    }
    else if (!(cap->y_resolution & 0x1)) {
        area->y1 &= ~0x1;
        area->y2 |= 0x1;
    }
}

#endif /* !defined(CONFIG_UI_SERVICE) && !defined(CONFIG_DISPLAY_COMPOSER) */
