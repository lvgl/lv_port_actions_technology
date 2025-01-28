/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**********************
 *      INCLUDES
 **********************/
#include <os_common_api.h>
#include <memory/mem_cache.h>
#include <display/display_hal.h>
#include <ui_mem.h>
#include <lvgl/lvgl.h>
#include <lvgl/src/lvgl_private.h>
#include <lvgl/lvgl_ext.h>
#include <lvgl/lvgl_virtual_display.h>

#if defined(CONFIG_TRACING) && defined(CONFIG_UI_SERVICE)
#  include <tracing/tracing.h>
#  include <view_manager.h>
#endif

/**********************
 *      DEFINES
 **********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef struct lvgl_disp_drv_data {
	surface_t *surface;
	os_sem wait_sem;

	ui_region_t flush_area;
	uint8_t flush_idx;
	uint8_t flush_flag;
	uint8_t refr_en;

#if defined(CONFIG_TRACING) && defined(CONFIG_UI_SERVICE)
	uint32_t frame_cnt;
#endif
} lvgl_disp_drv_data_t;

typedef struct lvgl_async_flush_ctx {
	bool initialized;
	bool enabled;
	bool flushing;
	lv_color_t * flush_buf;
	os_work flush_work;
	os_sem flush_sem;

	lvgl_disp_drv_data_t *disp_data;
} lvgl_async_flush_ctx_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
#if CONFIG_LV_VDB_NUM > 0
static void _lvgl_draw_buf_init_shared_vdb(lv_display_t * disp);
#endif

static int _lvgl_draw_buf_alloc(lv_display_t * disp, surface_t * surface);

static void _lvgl_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);
static void _lvgl_flush_wait_cb(lv_display_t * disp);
static void _lvgl_render_start_cb(lv_event_t * e);
static void _lvgl_render_area_start_cb(lv_event_t * e);
static void _lvgl_refresh_now(lv_display_t * disp);

static void _lvgl_invalidate_area_cb(lv_event_t * e);
static void _lvgl_surface_draw_cb(uint32_t event, void * data, void * user_data);

static uint8_t _lvgl_rotate_flag_from_surface(uint16_t rotation);

#ifdef CONFIG_SURFACE_TRANSFORM_UPDATE
static uint8_t _lvgl_rotate_flag_to_surface(lv_display_t * disp);
static void _lvgl_rotate_area_to_surface(lv_display_t * disp,
				ui_region_t *region, const lv_area_t *area);
#endif

/**********************
 *  STATIC VARIABLES
 **********************/

#if CONFIG_LV_VDB_NUM > 0

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

#ifndef CONFIG_UI_MEM_VDB_SHARE_SURFACE_BUFFER
#if CONFIG_LV_VDB_SIZE <= 0
#  error CONFIG_LV_VDB_SIZE must greater than  0
#endif

#define BUFFER_SIZE ((CONFIG_LV_VDB_SIZE + (BUFFER_ALIGN - 1)) & ~(BUFFER_ALIGN - 1))

static uint8_t vdb_buf_0[BUFFER_SIZE] __aligned(BUFFER_ALIGN) __in_section_unique(lvgl.noinit.vdb.0);
#if CONFIG_LV_VDB_NUM > 1
static uint8_t vdb_buf_1[BUFFER_SIZE] __aligned(BUFFER_ALIGN) __in_section_unique(lvgl.noinit.vdb.1);
#endif
#endif /* CONFIG_UI_MEM_VDB_SHARE_SURFACE_BUFFER */
#endif /* CONFIG_LV_VDB_NUM > 0*/

/* active display with indev attached to */
static lv_display_t * g_act_disp = NULL;
static lv_display_t * g_refr_disp = NULL;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
lv_display_t * lvgl_virtual_display_create(surface_t * surface)
{
	lv_display_t *disp = NULL;
	lvgl_disp_drv_data_t *drv_data = NULL;

	drv_data = lv_malloc_zeroed(sizeof(*drv_data));
	if (drv_data == NULL) {
		LV_LOG_ERROR("Failed to allocate dirver data");
		return NULL;
	}

	os_sem_init(&drv_data->wait_sem, 0, 1);
	drv_data->surface = surface;
	drv_data->refr_en = 1;

	disp = lv_display_create(surface_get_width(surface), surface_get_height(surface));
	if (disp == NULL) {
		LV_LOG_ERROR("Failed to create display");
		lv_free(drv_data);
		return NULL;
	}

	lv_display_set_color_format(disp, lvx_color_format_from_display(surface->pixel_format));
	lv_display_set_rotation(disp, _lvgl_rotate_flag_from_surface(surface_get_orientation(surface)));
	lv_display_set_driver_data(disp, drv_data);

	if (_lvgl_draw_buf_alloc(disp, surface)) {
		goto fail_del_display;
	}

	lv_display_set_flush_cb(disp, _lvgl_flush_cb);
	lv_display_set_flush_wait_cb(disp, _lvgl_flush_wait_cb);
	lv_display_add_event_cb(disp, _lvgl_render_start_cb, LV_EVENT_RENDER_START, drv_data);
	lv_display_add_event_cb(disp, _lvgl_render_area_start_cb, LV_EVENT_RENDER_AREA_START, drv_data);

	if (surface_get_max_possible_buffer_count() == 0 ||
		hal_pixel_format_get_bits_per_pixel(surface->pixel_format) == 16) {
		lv_display_add_event_cb(disp, _lvgl_invalidate_area_cb, LV_EVENT_INVALIDATE_AREA, drv_data);
	}

	lv_obj_t *bottom = lv_display_get_layer_bottom(disp);
	lv_obj_set_style_bg_color(bottom, lv_color_black(), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(bottom, LV_OPA_COVER, LV_PART_MAIN);

	/* Skip drawing the empty frame */
	lv_timer_pause(lv_display_get_refr_timer(disp));

	surface_register_callback(surface, SURFACE_CB_DRAW, _lvgl_surface_draw_cb, disp);
	surface_set_continuous_draw_count(surface, CONFIG_LV_VDB_NUM);

	LV_LOG_INFO("disp %p created\n", disp);
	return disp;

fail_del_display:
	lv_display_delete(disp);
	lv_free(drv_data);
	return NULL;
}

void lvgl_virtual_display_delete(lv_display_t * disp)
{
	lvgl_disp_drv_data_t *drv_data = lv_display_get_driver_data(disp);

	/* wait flush finished */
	_lvgl_flush_wait_cb(disp);

	/* unregister surface callback */
	surface_register_callback(drv_data->surface, SURFACE_CB_DRAW, NULL, NULL);

	if (g_act_disp == disp) {
		lvgl_virtual_display_set_focus(NULL, true);
	}

	if (g_refr_disp == disp) {
		g_refr_disp = NULL;
	}

	lv_display_delete(disp);
	lv_free(drv_data);

	LV_LOG_INFO("disp %p destroyed\n", disp);
}

int lvgl_virtual_display_set_default(lv_display_t * disp)
{
	/* Set default display */
	lv_display_set_default(disp);
	return 0;
}

int lvgl_virtual_display_set_focus(lv_display_t * disp, bool reset_indev)
{
	if (disp == g_act_disp) {
		return 0;
	}

	if (disp == NULL) {
		LV_LOG_INFO("no active display\n");
	}

	/* Retach the input devices */
	lv_indev_t *indev = lv_indev_get_next(NULL);
	while (indev) {
		 /* Reset the indev when focus changed */
		if (reset_indev) {
			lv_indev_reset(indev, NULL);
		} else {
			lv_indev_wait_release(indev);
		}

		lv_indev_set_display(indev, disp);
		indev = lv_indev_get_next(indev);

		LV_LOG_INFO("indev %p attached to disp %p\n", indev, disp);
	}

	/* Set default display */
	lv_display_set_default(disp);

	g_act_disp = disp;
	return 0;
}

int lvgl_virtual_display_update(lv_display_t * disp, uint16_t rotation)
{
	lv_display_set_rotation(disp, _lvgl_rotate_flag_from_surface(rotation));
	return 0;
}

void lvgl_virtual_display_refr_now(lv_display_t * disp)
{
	if (disp) {
		_lvgl_refresh_now(disp);
	} else {
        disp = lv_display_get_next(NULL);
        while (disp) {
			_lvgl_refresh_now(disp);
            disp = lv_display_get_next(disp);
        }
	}
}

int lvgl_virtual_display_enable_refr(lv_display_t * disp, bool enable)
{
	lvgl_disp_drv_data_t *drv_data = lv_display_get_driver_data(disp);
	if (drv_data->refr_en == enable)
		return 0;

	drv_data->refr_en = enable;

	lv_timer_t *refr_timer = lv_display_get_refr_timer(disp);
	if (refr_timer) {
		if (enable) {
			lv_timer_set_period(refr_timer, 1); /* run at max performance */
		} else {
			lv_timer_set_period(refr_timer, UINT32_MAX); /* run max period */
			lv_timer_reset(refr_timer);
		}
	}

	return 0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#if CONFIG_LV_VDB_NUM > 0
static void _lvgl_draw_buf_init_shared_vdb(lv_display_t * disp)
{
#ifdef CONFIG_UI_MEM_VDB_SHARE_SURFACE_BUFFER
	uint8_t *bufs[CONFIG_LV_VDB_NUM];
	unsigned int total_size = ui_mem_get_share_surface_buffer_size();
	unsigned int buf_size = ((total_size / CONFIG_LV_VDB_NUM) & ~(BUFFER_ALIGN - 1));
	int i;

	if (CONFIG_LV_VDB_SIZE > 0 && CONFIG_LV_VDB_SIZE < buf_size) {
		buf_size = (CONFIG_LV_VDB_SIZE + BUFFER_ALIGN - 1) & ~(BUFFER_ALIGN - 1);
	}

	bufs[0] = ui_mem_get_share_surface_buffer();
	for (i = 1; i < CONFIG_LV_VDB_NUM; i++) {
		bufs[i] = bufs[i - 1] + buf_size;
	}

#else /* CONFIG_UI_MEM_VDB_SHARE_SURFACE_BUFFER */
	uint8_t *bufs[CONFIG_LV_VDB_NUM] = {
		vdb_buf_0,
#if CONFIG_LV_VDB_NUM > 1
		vdb_buf_1,
#endif
	};

	unsigned int buf_size = BUFFER_SIZE;
#endif /* CONFIG_UI_MEM_VDB_SHARE_SURFACE_BUFFER */

	LV_LOG_INFO("LVGL VDB: size %u, num %u\n", buf_size, CONFIG_LV_VDB_NUM);

	lv_display_set_buffers(disp, bufs[0], bufs[1], buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
}
#endif /* CONFIG_LV_VDB_NUM > 0 */

static int _lvgl_draw_buf_alloc(lv_display_t * disp, surface_t * surface)
{
#if CONFIG_LV_VDB_NUM > 0
#ifndef CONFIG_SURFACE_TRANSFORM_UPDATE
	if (lv_display_get_rotation(disp) != LV_DISPLAY_ROTATION_0) {
		LV_LOG_ERROR("Enable CONFIG_SURFACE_TRANSFORM_UPDATE to support rotation");
		return -ENOTSUP;
	}
#endif

	_lvgl_draw_buf_init_shared_vdb(disp);
#else
	lvgl_disp_drv_data_t *drv_data = lv_display_get_driver_data(disp);

	if (surface_get_max_possible_buffer_count() == 0) {
		LV_LOG_ERROR("no vdb, must increase CONFIG_SURFACE_MAX_BUFFER_COUNT to use direct mode");
		return -EINVAL;
	}

	if (lv_display_get_rotation(disp) != LV_DISPLAY_ROTATION_0) {
		LV_LOG_ERROR("rotation not supported in direct mode");
		return -ENOTSUP;
	}

	LV_LOG_INFO("no vdb, use direct mode");

	graphic_buffer_t *buf1 = surface_get_draw_buffer(surface);
	graphic_buffer_t *buf2 = surface_get_post_buffer(surface);
	LV_ASSERT(buf1 != NULL);

	lv_display_set_buffers(disp, buf1->data, (buf2 == NULL || buf2 == buf1) ? NULL : buf2->data,
			graphic_buffer_get_bytes_per_line(buf1) * graphic_buffer_get_height(buf1),
			LV_DISPLAY_RENDER_MODE_DIRECT);
#endif /* CONFIG_LV_VDB_NUM > 0*/

	return 0;
}

static void _lvgl_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
	lvgl_disp_drv_data_t *drv_data = lv_display_get_driver_data(disp);
	int res = 0;

#ifdef CONFIG_SURFACE_TRANSFORM_UPDATE
	drv_data->flush_flag = _lvgl_rotate_flag_to_surface(disp);
	if (drv_data->flush_idx == 0)
		drv_data->flush_flag |= SURFACE_FIRST_DRAW;
#else
	drv_data->flush_flag = (drv_data->flush_idx == 0) ? SURFACE_FIRST_DRAW : 0;
#endif

	if (lv_display_flush_is_last(disp)) {
		drv_data->flush_flag |= SURFACE_LAST_DRAW;
		drv_data->flush_idx = 0;
#if defined(CONFIG_TRACING) && defined(CONFIG_UI_SERVICE)
		drv_data->frame_cnt++;
#endif
	} else {
		drv_data->flush_idx++;
	}

	LV_LOG_TRACE("lvgl flush: flag %x, buf %p, area (%d %d %d %d)\n",
			drv_data->flush_flag, color_p, area->x1, area->y1, area->x2, area->y2);

	if (disp->render_mode == LV_DISPLAY_RENDER_MODE_DIRECT) {
		if (drv_data->flush_flag & SURFACE_LAST_DRAW) {
			res = surface_end_draw(drv_data->surface, &drv_data->flush_area);
			surface_end_frame(drv_data->surface);
			if (res) {
				LV_LOG_ERROR("surface end failed");
				lv_display_flush_ready(disp);
			}
		} else {
			lv_display_flush_ready(disp);
		}
	} else {
#ifdef CONFIG_SURFACE_TRANSFORM_UPDATE
		_lvgl_rotate_area_to_surface(disp, &drv_data->flush_area, area);
#else
		ui_region_set(&drv_data->flush_area, area->x1, area->y1, area->x2, area->y2);
#endif

		res = surface_update(drv_data->surface, drv_data->flush_flag,
					&drv_data->flush_area, px_map, lv_area_get_width(area),
					surface_get_pixel_format(drv_data->surface));
		if (drv_data->flush_flag & SURFACE_LAST_DRAW) {
			surface_end_frame(drv_data->surface);
		}

		if (res) {
			LV_LOG_ERROR("surface update failed");
			lv_display_flush_ready(disp);
		}
	}
}

static void _lvgl_flush_wait_cb(lv_display_t * disp)
{
	lvgl_disp_drv_data_t *drv_data = lv_display_get_driver_data(disp);

	LV_LOG_TRACE("lvgl wait\n");

	while (disp->flushing) {
		os_sem_take(&drv_data->wait_sem, OS_FOREVER);
	}
}

static void _lvgl_render_start_cb(lv_event_t * e)
{
	lv_display_t *disp = lv_event_get_target(e);
	lvgl_disp_drv_data_t *drv_data = lv_display_get_driver_data(disp);

	surface_begin_frame(drv_data->surface);

	if (disp->render_mode == LV_DISPLAY_RENDER_MODE_DIRECT) {
		graphic_buffer_t *drawbuf = NULL;
		surface_begin_draw(drv_data->surface, SURFACE_FIRST_DRAW | SURFACE_LAST_DRAW, &drawbuf);
	}

	if (g_refr_disp != disp) {
		if (g_refr_disp) {
			drv_data = lv_display_get_driver_data(g_refr_disp);
			surface_wait_for_update(drv_data->surface, -1);
		}

		g_refr_disp = disp;
	}
}

static void _lvgl_render_area_start_cb(lv_event_t * e)
{
	lv_display_t *disp = lv_event_get_target(e);
	lvgl_disp_drv_data_t *drv_data = lv_display_get_driver_data(disp);
	const lv_area_t *area = lv_event_get_param(e);

#if defined(CONFIG_TRACING) && defined(CONFIG_UI_SERVICE)
	ui_view_context_t *view = drv_data->surface->user_data[SURFACE_CB_POST];

	os_strace_u32x7(SYS_TRACE_ID_VIEW_DRAW, view->entry->id, drv_data->frame_cnt,
			drv_data->flush_idx, area->x1, area->y1, area->x2, area->y2);
#endif /* CONFIG_TRACING */

	if (disp->render_mode == LV_DISPLAY_RENDER_MODE_DIRECT) {
		if (drv_data->flush_idx == 0) {
			drv_data->flush_area.x1 = area->x1;
			drv_data->flush_area.y1 = area->y1;
			drv_data->flush_area.x2 = area->x2;
			drv_data->flush_area.y2 = area->y2;
		} else {
			drv_data->flush_area.x1 = LV_MIN(drv_data->flush_area.x1, area->x1);
			drv_data->flush_area.y1 = LV_MIN(drv_data->flush_area.y1, area->y1);
			drv_data->flush_area.x2 = LV_MAX(drv_data->flush_area.x2, area->x2);
			drv_data->flush_area.y2 = LV_MAX(drv_data->flush_area.y2, area->y2);
		}
	}
}

static void _lvgl_refresh_now(lv_display_t * disp)
{
	lvgl_disp_drv_data_t *drv_data = lv_display_get_driver_data(disp);
	if (!drv_data->refr_en) return;

	lv_timer_t *refr_timer = lv_display_get_refr_timer(disp);
	if (refr_timer) {
		lv_display_refr_timer(disp->refr_timer);
	} else {
		lv_display_t *disp_def = lv_display_get_default();
        lv_display_set_default(disp);
        lv_display_refr_timer(NULL);
        lv_display_set_default(disp_def);
	}
}

static void _lvgl_invalidate_area_cb(lv_event_t * e)
{
	lv_display_t *disp = lv_event_get_target(e);
	lv_area_t *area = lv_event_get_param(e);

	/*
	 * (1) Some LCD DDIC require even pixel alignment, so set even area if possible
	 * for framebuffer-less refresh mode.
	 * (2) Haraware may run faster on aligned pixels, so set horizontal even if possible
	 */
	if (!(disp->hor_res & 0x1)) {
		area->x1 &= ~0x1;
		area->x2 |= 0x1;
	}

	/* Framebuffer-less refresh mode must meet LCD DDIC pixel algnment */
	if (surface_get_max_possible_buffer_count() == 0 && !(disp->ver_res & 0x1)) {
		area->y1 &= ~0x1;
		area->y2 |= 0x1;
	}
}

static uint8_t _lvgl_rotate_flag_from_surface(uint16_t rotation)
{
	LV_ASSERT(rotation <= 270);

	switch (rotation) {
	case 90:
		return LV_DISPLAY_ROTATION_270;
	case 180:
		return LV_DISPLAY_ROTATION_180;
	case 270:
		return LV_DISPLAY_ROTATION_90;
	default:
		return LV_DISPLAY_ROTATION_0;
	}
}

#ifdef CONFIG_SURFACE_TRANSFORM_UPDATE
static uint8_t _lvgl_rotate_flag_to_surface(lv_display_t * disp)
{
	static const uint8_t flags[] = {
		0, SURFACE_ROTATED_270, SURFACE_ROTATED_180, SURFACE_ROTATED_90,
	};

	return flags[disp->rotation];
}

static void _lvgl_rotate_area_to_surface(lv_display_t * disp,
				ui_region_t * region, const lv_area_t * area)
{
	switch (disp->rotation) {
	case LV_DISPLAY_ROTATION_90: /* 270 degree clockwise rotation */
		region->x1 = area->y1;
		region->y1 = disp->hor_res - area->x2 - 1;
		region->x2 = area->y2;
		region->y2 = disp->hor_res - area->x1 - 1;
		break;
	case LV_DISPLAY_ROTATION_180: /* 180 degree clockwise rotation */
		region->x1 = disp->hor_res - area->x2 - 1;
		region->y1 = disp->ver_res - area->y2 - 1;
		region->x2 = disp->hor_res - area->x1 - 1;
		region->y2 = disp->ver_res - area->y1 - 1;
		break;
	case LV_DISPLAY_ROTATION_270: /* 90 degree clockwise rotation */
		region->x1 = disp->ver_res - area->y2 - 1;
		region->y1 = area->x1;
		region->x2 = disp->ver_res - area->y1 - 1;
		region->y2 = area->x2;
		break;
	case LV_DISPLAY_ROTATION_0:
	default:
		ui_region_set(region, area->x1, area->y1, area->x2, area->y2);
		break;
	}
}
#endif /* CONFIG_SURFACE_TRANSFORM_UPDATE */

static void _lvgl_surface_draw_cb(uint32_t event, void * data, void * user_data)
{
	lv_display_t *disp = user_data;
	lvgl_disp_drv_data_t *drv_data = lv_display_get_driver_data(disp);

	if (event == SURFACE_EVT_DRAW_READY) {
		lv_display_flush_ready(disp);
		os_sem_give(&drv_data->wait_sem);
	} else if (event == SURFACE_EVT_DRAW_COVER_CHECK) {
		surface_cover_check_data_t *cover_check = data;

		/* direct mode: buffer copy will be done in lv_refr.c */
		if (disp->render_mode == LV_DISPLAY_RENDER_MODE_DIRECT || (
			disp->inv_areas[0].x1 <= cover_check->area->x1 &&
			disp->inv_areas[0].y1 <= cover_check->area->y1 &&
			disp->inv_areas[0].x2 >= cover_check->area->x2 &&
			disp->inv_areas[0].y2 >= cover_check->area->y2)) {
			cover_check->covered = true;
		} else {
			cover_check->covered = false;
		}
	}
}
