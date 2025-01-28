/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_CUSTOMER

#ifndef CONFIG_UI_SERVICE
#  undef CONFIG_TRACING
#endif

#include <os_common_api.h>
#include <assert.h>
#include <string.h>
#include <mem_manager.h>
#include <memory/mem_cache.h>
#include <display/sw_draw.h>
#include <display/ui_memsetcpy.h>
#include <ui_mem.h>
#include <ui_surface.h>
#ifdef CONFIG_DMA2D_HAL
#  include <dma2d_hal.h>
#endif
#if defined(CONFIG_TRACING) && defined(CONFIG_UI_SERVICE)
#  include <tracing/tracing.h>
#  include <view_manager.h>
#endif

LOG_MODULE_REGISTER(surface, LOG_LEVEL_INF);

/**********************
 *      DEFINES
 **********************/
#ifdef CONFIG_DMA2D_HAL
#  if defined(CONFIG_LV_COLOR_DEPTH_32) || defined(CONFIG_SURFACE_TRANSFORM_UPDATE)
#    define DMA2D_OPEN_MODE  HAL_DMA2D_FULL_MODES /* may require color conversion */
#  else
#    define DMA2D_OPEN_MODE  HAL_DMA2D_M2M
#endif
#endif /* CONFIG_DMA2D_HAL */

/**********************
 *      TYPEDEFS
 **********************/
#ifdef CONFIG_DMA2D_HAL
typedef struct {
	surface_t * surface;
	ui_region_t area;
	os_work work;
	os_sem sem;
	os_mutex mutex;

	uint16_t draw_seq;
	uint16_t cplt_seq;
} surface_delayed_update_t;
#endif /* CONFIG_DMA2D_HAL */

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void _surface_buffer_destroy_cb(struct graphic_buffer *buffer);

#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 0
static int _surface_buffer_copy(graphic_buffer_t *dstbuf, const ui_region_t *dst_region,
		const uint8_t *src_buf, uint32_t src_pixel_format, uint16_t src_stride, uint8_t flags);
#endif

#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 1
static void _surface_swapbuf(surface_t *surface);
#elif CONFIG_SURFACE_MAX_BUFFER_COUNT > 0
static inline void _surface_swapbuf(surface_t *surface) { }
#endif

#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 0 && defined(CONFIG_DMA2D_HAL)
static void _surface_dma2d_init(void);
static void _surface_dma2d_poll(void);
static void _surface_draw_wait_finish(surface_t *surface);
#else
static inline void _surface_draw_wait_finish(surface_t *surface) { }
#endif

#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 1 && defined(CONFIG_DMA2D_HAL)
static void _surface_swapbuf_wait_finish(surface_t *surface);
#else
static inline void _surface_swapbuf_wait_finish(surface_t *surface) { }
#endif

static void _surface_frame_wait_end(surface_t *surface);

static int _surface_begin_draw_internal(surface_t *surface, uint8_t flags, graphic_buffer_t **drawbuf);
static int _surface_end_draw_internal(surface_t *surface, const ui_region_t *area,
		const void *buf, uint16_t stride, uint32_t pixel_format);

static void _surface_invoke_draw_ready(surface_t *surface);
static void _surface_invoke_post_start(surface_t *surface, const ui_region_t *area, uint8_t flags);

/**********************
 *  STATIC VARIABLES
 **********************/
#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 0 && defined(CONFIG_DMA2D_HAL)
static bool dma2d_inited = false;
static hal_dma2d_handle_t hdma2d __in_section_unique(ram.noinit.surface);
static surface_delayed_update_t delayed_update;
#endif /* CONFIG_DMA2D_HAL */

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
graphic_buffer_t *surface_buffer_create(uint16_t w, uint16_t h,
		uint32_t pixel_format, uint32_t usage)
{
	graphic_buffer_t *buffer = NULL;
	uint32_t stride, data_size;
	uint8_t bits_per_pixel;
	uint8_t multiple;
	void *data;

	bits_per_pixel = hal_pixel_format_get_bits_per_pixel(pixel_format);
	if (bits_per_pixel == 0 || bits_per_pixel > 32) {
		return NULL;
	}

#if 0
	/* require bytes per row is 4 bytes align */
	multiple = 32;
	uint8_t tmp_bpp = bits_per_pixel;
	while ((tmp_bpp & 0x1) == 0) {
		tmp_bpp >>= 1;
		multiple >>= 1;
	}

#else
	/* 1 byte align is enough */
	multiple = (bits_per_pixel >= 8) ? 1 : (8 >> (bits_per_pixel >> 1));
#endif

	stride = UI_ROUND_UP(w, multiple);
	data_size = stride * h * bits_per_pixel / 8;

	/* FIXME: if size too small, allocate it from GUI to save memory */
	if (data_size < CONFIG_UI_MEM_BLOCK_SIZE / 16) {
		data = ui_mem_alloc(MEM_RES, data_size, __func__);
		if (data == NULL) {
			data = ui_mem_alloc(MEM_GUI, data_size, __func__);
		}
	} else {
		data = ui_mem_alloc(MEM_FB, data_size, __func__);
	}

	if (data == NULL) {
		SYS_LOG_ERR("surface mem alloc failed");
		return NULL;
	}

	buffer = mem_malloc(sizeof(*buffer));
	if (buffer == NULL) {
		ui_mem_free2(data);
		return NULL;
	}

	if (graphic_buffer_init(buffer, w, h, pixel_format, usage, stride, data)) {
		ui_mem_free2(data);
		mem_free(buffer);
		return NULL;
	}

	graphic_buffer_set_destroy_cb(buffer, _surface_buffer_destroy_cb);
	return buffer;
}

surface_t *surface_create(uint16_t w, uint16_t h,
		uint32_t pixel_format, uint8_t buf_count, uint8_t flags)
{
	surface_t *surface = mem_malloc(sizeof(*surface));
	if (surface == NULL) {
		return NULL;
	}

	memset(surface, 0, sizeof(*surface));
	surface->width = w;
	surface->height = h;
	surface->pixel_format = pixel_format;
	surface->create_flags = flags;

	atomic_set(&surface->refcount, 1);
	os_sem_init(&surface->post_sem, 0, 1);
	os_sem_init(&surface->frame_sem, 0, 1);

#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 0
	if (surface_set_min_buffer_count(surface, buf_count)) {
		surface_destroy(surface);
		return NULL;
	}

#ifdef CONFIG_DMA2D_HAL
	_surface_dma2d_init();
#endif

#else /* CONFIG_SURFACE_MAX_BUFFER_COUNT > 0 */
	if (flags & SURFACE_POST_IN_SYNC_MODE) {
		mem_free(surface);
		return NULL;
	}

	surface_set_continuous_draw_count(surface, 2);
	if (surface->buffers == NULL) {
		mem_free(surface);
		return NULL;
	}
#endif /* CONFIG_SURFACE_MAX_BUFFER_COUNT > 0 */

	return surface;
}

void surface_destroy(surface_t *surface)
{
	if (surface == NULL) {
		return;
	}

#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 0
	surface_set_max_buffer_count(surface, 0);
#else
	_surface_frame_wait_end(surface);

	while (atomic_get(&surface->post_cnt) > 0) {
		SYS_LOG_DBG("%p wait post", surface);
		os_sem_take(&surface->post_sem, OS_FOREVER);
	}

	mem_free(surface->buffers);
#endif /* CONFIG_SURFACE_MAX_BUFFER_COUNT > 0 */

	if (atomic_dec(&surface->refcount) == 1) {
		mem_free(surface);
	}
}

void surface_register_callback(surface_t *surface,
		int callback_id, surface_callback_t callback_fn, void *user_data)
{
	unsigned int key = os_irq_lock();

	surface->callback[callback_id] = callback_fn;
	surface->user_data[callback_id] = user_data;

	os_irq_unlock(key);
}

void surface_set_continuous_draw_count(surface_t *surface, uint8_t count)
{
#if CONFIG_SURFACE_MAX_BUFFER_COUNT == 0
	if (surface->buf_count < count) {
		graphic_buffer_t * buffers = mem_malloc(sizeof(graphic_buffer_t) * count);
		if (buffers) {
			mem_free(surface->buffers);
			surface->buffers = buffers;
			surface->buf_count = count;
			surface->draw_idx = 0;
		}
	}
#endif
}

int surface_set_min_buffer_count(surface_t *surface, uint8_t min_count)
{
#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 0
	uint8_t buf_count;
	int res = 0;

	if (min_count > CONFIG_SURFACE_MAX_BUFFER_COUNT)
		return -EDOM;

	if (min_count <= surface->buf_count)
		return 0;

	_surface_frame_wait_end(surface);
	_surface_draw_wait_finish(surface);
	_surface_swapbuf_wait_finish(surface);

	buf_count = surface->buf_count;

	for (int i = buf_count; i < min_count; i++) {
		surface->buffers[i] = surface_buffer_create(surface->width,
				surface->height, surface->pixel_format,
				GRAPHIC_BUFFER_SW_MASK | GRAPHIC_BUFFER_HW_MASK);
		if (surface->buffers[i] == NULL) {
			SYS_LOG_ERR("alloc failed %d ",i);
			res = -ENOMEM;
			break;
		}

		buf_count++;
	}

	if (buf_count != surface->buf_count) {
		/* make sure the front buffer index keep the same */
		os_sched_lock();
		surface->buf_count = buf_count;
		surface->draw_idx = buf_count - 1;
		os_sched_unlock();

		/* invalidate the new dirty area */
		ui_region_set(&surface->dirty_area, 0, 0, surface->width - 1, surface->height - 1);

		SYS_LOG_DBG("buf count %d", surface->buf_count);
	}

	return res;
#else
	return (min_count > 0) ? -EDOM : 0;
#endif /* CONFIG_SURFACE_MAX_BUFFER_COUNT > 0 */
}

int surface_set_max_buffer_count(surface_t *surface, uint8_t max_count)
{
#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 0
	uint8_t buf_count = surface->buf_count;
	uint8_t max_post_cnt = (surface->create_flags & SURFACE_POST_IN_SYNC_MODE) ? 1 : max_count;

	if (max_count >= buf_count)
		return 0;

	_surface_frame_wait_end(surface);
	_surface_draw_wait_finish(surface);
	_surface_swapbuf_wait_finish(surface);

	/* synchronize */
	while (atomic_get(&surface->post_cnt) > max_post_cnt) {
		SYS_LOG_DBG("%p wait post", surface);
		os_sem_take(&surface->post_sem, OS_FOREVER);
	}

	/* make sure the front buffer index keep the same */
	os_sched_lock();

#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 1
	if (buf_count > 1) {
		/**
		 * Keep the post buffer which has the latest content, and the result of
		 * surface_get_post_buffer() unaffected which is used by view manager.
		 */
		if (surface->draw_idx == 0) {
			graphic_buffer_t *tmp = surface->buffers[1];
			surface->buffers[1] = surface->buffers[0];
			surface->buffers[0] = tmp;
		}
	}
#endif

	surface->draw_idx = 0;
	surface->post_idx = 0;
	surface->buf_count = max_count;

	os_sched_unlock();

	for (int i = max_count; i < buf_count; i++) {
		graphic_buffer_destroy(surface->buffers[i]);
		surface->buffers[i] = NULL;
	}
#endif /* CONFIG_SURFACE_MAX_BUFFER_COUNT > 0 */

	return 0;
}

int surface_set_buffer_count(surface_t *surface, uint8_t buf_count)
{
#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 0
	if (buf_count > surface->buf_count) {
		return surface_set_min_buffer_count(surface, buf_count);
	} else {
		return surface_set_max_buffer_count(surface, buf_count);
	}
#else
	return (buf_count == 0) ? 0 : -EINVAL;
#endif /* CONFIG_SURFACE_MAX_BUFFER_COUNT > 0 */
}

int surface_begin_frame(surface_t *surface)
{
#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 0
	bool covered = true;

	if (surface->buf_count == 0) {
		return -ENOBUFS;
	}

#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 1
	if (surface->buf_count == 2) {
		surface_cover_check_data_t cover_check_data = {
			.area = &surface->dirty_area,
			.covered = ui_region_is_empty(&surface->dirty_area),
		};

		if (!cover_check_data.covered && surface->callback[SURFACE_CB_DRAW]) {
			surface->callback[SURFACE_CB_DRAW](SURFACE_EVT_DRAW_COVER_CHECK,
					&cover_check_data, surface->user_data[SURFACE_CB_DRAW]);
		}

		covered = cover_check_data.covered;

		SYS_LOG_DBG("dirty (%d %d %d %d), covered %d",
			surface->dirty_area.x1, surface->dirty_area.y1,
			surface->dirty_area.x2, surface->dirty_area.y2, covered);
	}
#endif /* CONFIG_SURFACE_MAX_BUFFER_COUNT > 1 */

	/* surface_update() may called in another thread, so synchronization is required */
	_surface_frame_wait_end(surface);
	_surface_draw_wait_finish(surface);

	/* surface swap buffer ? */
	if (!covered) {
		/* wait backbuf available */
		while (atomic_get(&surface->post_cnt) >= surface->buf_count) {
			SYS_LOG_DBG("%p wait post", surface);
			os_sem_take(&surface->post_sem, OS_FOREVER);
		}

		_surface_swapbuf(surface);
	}

	ui_region_set(&surface->dirty_area, surface->width, surface->height, 0, 0);

#else
	/* surface_update() may called in another thread, so synchronization is required */
	_surface_frame_wait_end(surface);
#endif /* CONFIG_SURFACE_MAX_BUFFER_COUNT > 0 */

	/* indicate the shared buffer was painted */
	ui_mem_set_share_surface_buffer_painted(true);

	surface->in_frame = 1;
	return 0;
}

int surface_end_frame(surface_t *surface)
{
	surface->in_frame = 0;
	os_sem_give(&surface->frame_sem);
	return 0;
}

int surface_begin_draw(surface_t *surface, uint8_t flags, graphic_buffer_t **drawbuf)
{
#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 0
	return _surface_begin_draw_internal(surface, flags, drawbuf);
#else
	return -ENOBUFS;
#endif
}

int surface_end_draw(surface_t *surface, const ui_region_t *area)
{
#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 0
	return _surface_end_draw_internal(surface, area, NULL, 0, 0);
#else
	return -ENOBUFS;
#endif
}

int surface_update(surface_t *surface, uint8_t flags,
				   const ui_region_t *area, const void *buf,
				   uint16_t stride, uint32_t pixel_format)
{
	graphic_buffer_t *drawbuf = NULL;
	int res;

	_surface_draw_wait_finish(surface);

	res = _surface_begin_draw_internal(surface, flags, &drawbuf);
	if (res) {
		return res;
	}

#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 0
	res = _surface_buffer_copy(drawbuf, area, buf, pixel_format, stride, flags);

#ifdef CONFIG_DMA2D_HAL
	if (res >= 0) {
		bool completed;
		unsigned int key = os_irq_lock();

		completed = (res == delayed_update.cplt_seq) ? true : false;
		if (completed == false) {
			delayed_update.draw_seq = (uint16_t)res;
			delayed_update.surface = surface;
			memcpy(&delayed_update.area, area, sizeof(*area));
		}

		os_irq_unlock(key);

		if (completed) {
			res = _surface_end_draw_internal(surface, area, NULL, 0, 0);
		} else {
			res = 0;
		}
	} else {
		res = _surface_end_draw_internal(surface, area, NULL, 0, 0);
	}
#else
	res = _surface_end_draw_internal(surface, area, NULL, 0, 0);
#endif /* CONFIG_DMA2D_HAL */

#else /* CONFIG_SURFACE_MAX_BUFFER_COUNT > 0 */
	res = _surface_end_draw_internal(surface, area, buf, stride, pixel_format);
#endif /* CONFIG_SURFACE_MAX_BUFFER_COUNT > 0 */

	return res;
}

int surface_wait_for_update(surface_t *surface, int timeout)
{
	_surface_frame_wait_end(surface);
	_surface_draw_wait_finish(surface);
	return 0;
}

int surface_wait_for_refresh(surface_t *surface, int timeout)
{
	uint8_t max_post_cnt = (surface->create_flags & SURFACE_POST_IN_SYNC_MODE) ? 1 : 0;

	_surface_frame_wait_end(surface);
	_surface_draw_wait_finish(surface);
	_surface_swapbuf_wait_finish(surface);

	/* synchronize */
	while (atomic_get(&surface->post_cnt) > max_post_cnt) {
		SYS_LOG_DBG("%p wait post", surface);
		os_sem_take(&surface->post_sem, OS_FOREVER);
	}

	return 0;
}

void surface_complete_one_post(surface_t *surface)
{
	atomic_dec(&surface->post_cnt);
	SYS_LOG_DBG("%p post cplt %d", surface, atomic_get(&surface->post_cnt));
	os_sem_give(&surface->post_sem);

#if CONFIG_SURFACE_MAX_BUFFER_COUNT == 0
	_surface_invoke_draw_ready(surface);
#endif

	if (atomic_dec(&surface->refcount) == 1) {
		mem_free(surface);
	}
}

graphic_buffer_t *surface_get_draw_buffer(surface_t *surface)
{
#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 0
	return (surface->buf_count > 0) ? surface->buffers[surface->draw_idx] : NULL;
#else
	return NULL;
#endif
}

graphic_buffer_t *surface_get_post_buffer(surface_t *surface)
{
#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 0
	return (surface->buf_count > 0) ? surface->buffers[surface->post_idx] : NULL;
#else
	return &surface->buffers[surface->draw_idx];
#endif
}

uint8_t surface_get_buffer_count(surface_t *surface)
{
#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 0
	return surface->buf_count;
#else
	return 0;
#endif
}

uint8_t surface_get_max_possible_buffer_count(void)
{
	return CONFIG_SURFACE_MAX_BUFFER_COUNT;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void _surface_buffer_destroy_cb(struct graphic_buffer *buffer)
{
	assert(buffer != NULL && buffer->data != NULL);

	ui_mem_free2(buffer->data);
	mem_free(buffer);
}

static int _surface_begin_draw_internal(surface_t *surface, uint8_t flags, graphic_buffer_t **drawbuf)
{
#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 0
	if (surface->buf_count == 0) {
		SYS_LOG_WRN("surface %p no buffer", surface);
		return -ENOBUFS;
	}

	if (flags & SURFACE_FIRST_DRAW) {
		/* wait backbuf available */
		while (atomic_get(&surface->post_cnt) >= surface->buf_count) {
			SYS_LOG_DBG("%p wait post", surface);
			os_sem_take(&surface->post_sem, OS_FOREVER);
		}

		_surface_swapbuf_wait_finish(surface);
	}

	*drawbuf = surface->buffers[surface->draw_idx];
#endif /* CONFIG_SURFACE_MAX_BUFFER_COUNT > 0 */

	surface->draw_flags = flags;
	return 0;
}

static int _surface_end_draw_internal(surface_t *surface, const ui_region_t *area,
		const void *buf, uint16_t stride, uint32_t pixel_format)
{
#if defined(CONFIG_TRACING) && defined(CONFIG_UI_SERVICE)
	ui_view_context_t *view = surface->user_data[SURFACE_CB_POST];
	os_strace_end_call_u32(SYS_TRACE_ID_VIEW_DRAW, view->entry->id);
#endif /* CONFIG_TRACING */

#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 0
	assert(buf == NULL);

	/* post based on frame */
	ui_region_merge(&surface->dirty_area, &surface->dirty_area, area);

	_surface_invoke_draw_ready(surface);

	if (surface->draw_flags & SURFACE_LAST_DRAW) {
		/* make sure the swap buffer not interrupted by posting */
		os_sched_lock();

#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 1
		if (surface->buf_count > 1) {
			surface->post_idx ^= 1;
			surface->draw_idx ^= 1;
		}
#endif /* CONFIG_SURFACE_MAX_BUFFER_COUNT > 1 */

		_surface_invoke_post_start(surface, &surface->dirty_area, SURFACE_FIRST_DRAW | SURFACE_LAST_DRAW);
		os_sched_unlock();
	}

#else /* CONFIG_SURFACE_MAX_BUFFER_COUNT > 0 */
	while (atomic_get(&surface->post_cnt) >= surface->buf_count) {
		SYS_LOG_DBG("%p wait post", surface);
		os_sem_take(&surface->post_sem, OS_FOREVER);
	}

	if (++surface->draw_idx == surface->buf_count)
		surface->draw_idx = 0;

	graphic_buffer_init(&surface->buffers[surface->draw_idx],
			ui_region_get_width(area), ui_region_get_height(area), pixel_format,
			GRAPHIC_BUFFER_HW_COMPOSER, stride, (void *)buf);

	_surface_invoke_post_start(surface, area, surface->draw_flags);
#endif /* CONFIG_SURFACE_MAX_BUFFER_COUNT > 0 */

	return 0;
}

static void _surface_invoke_draw_ready(surface_t *surface)
{
	if (surface->callback[SURFACE_CB_DRAW]) {
		surface->callback[SURFACE_CB_DRAW](SURFACE_EVT_DRAW_READY,
				NULL, surface->user_data[SURFACE_CB_DRAW]);
	}
}

static void _surface_invoke_post_start(surface_t *surface, const ui_region_t *area, uint8_t flags)
{
	surface_post_data_t data = { .flags = flags, .area = area, };

	atomic_inc(&surface->refcount);
	atomic_inc(&surface->post_cnt);
	SYS_LOG_DBG("%p post inprog %d", surface, atomic_get(&surface->post_cnt));

	if (surface->callback[SURFACE_CB_POST]) {
		surface->callback[SURFACE_CB_POST](SURFACE_EVT_POST_START,
				&data, surface->user_data[SURFACE_CB_POST]);
	}
}

static void _surface_frame_wait_end(surface_t *surface)
{
	while (surface->in_frame) {
		os_sem_take(&surface->frame_sem, OS_FOREVER);
	}
}

#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 0
#ifdef CONFIG_DMA2D_HAL
static void _surface_draw_wait_finish(surface_t *surface)
{
	os_mutex_lock(&delayed_update.mutex, OS_FOREVER);

	while (delayed_update.surface != NULL) {
		os_sem_take(&delayed_update.sem, OS_FOREVER);
	}

	os_mutex_unlock(&delayed_update.mutex);
}

static void _surface_delay_update_work_handler(os_work *work)
{
	_surface_end_draw_internal(delayed_update.surface, &delayed_update.area, NULL, 0, 0);

	delayed_update.surface = NULL;
	os_sem_give(&delayed_update.sem);
}

static void _surface_dma2d_xfer_cb(hal_dma2d_handle_t *hdma2d, uint16_t cmd_seq, uint32_t error_code)
{
	delayed_update.cplt_seq = cmd_seq;

	if (delayed_update.surface && cmd_seq == delayed_update.draw_seq) {
		os_work_q *queue = os_get_display_work_queue();

		if (queue) {
			os_work_submit_to_queue(queue, &delayed_update.work);
		} else {
			os_work_submit(&delayed_update.work);
		}
	}
}

static void _surface_dma2d_poll(void)
{
	if (dma2d_inited) {
		os_mutex_lock(&delayed_update.mutex, OS_FOREVER);
		hal_dma2d_poll_transfer(&hdma2d, -1);
		os_mutex_unlock(&delayed_update.mutex);
	}
}

static void _surface_dma2d_init(void)
{
	if (dma2d_inited)
		return;

	if (hal_dma2d_init(&hdma2d, DMA2D_OPEN_MODE) == 0) {
		/* register hdma2d callback */
		hal_dma2d_register_callback(&hdma2d, _surface_dma2d_xfer_cb);

		hdma2d.layer_cfg[1].input_alpha = 0xff000000;

		os_work_init(&delayed_update.work, _surface_delay_update_work_handler);
		os_sem_init(&delayed_update.sem, 0, 1);
		os_mutex_init(&delayed_update.mutex);

		dma2d_inited = true;
	}
}
#endif /* CONFIG_DMA2D_HAL */

static int _surface_buffer_copy(graphic_buffer_t *dstbuf,
		const ui_region_t *dst_region, const uint8_t *src,
		uint32_t src_pixel_format, uint16_t src_stride, uint8_t flags)
{
	uint8_t *dst = (uint8_t *)graphic_buffer_get_bufptr(dstbuf, dst_region->x1, dst_region->y1);
	uint8_t dst_px_size = graphic_buffer_get_bits_per_pixel(dstbuf) / 8;
	uint16_t dst_pitch = graphic_buffer_get_stride(dstbuf) * dst_px_size;
	uint16_t dst_w = ui_region_get_width(dst_region);
	uint16_t dst_h = ui_region_get_height(dst_region);
	uint8_t src_px_size = hal_pixel_format_get_bits_per_pixel(src_pixel_format) / 8;
	uint16_t src_pitch = src_stride * src_px_size;
	int res = -EINVAL;

#ifdef CONFIG_DMA2D_HAL
	if (dma2d_inited) {
		os_mutex_lock(&delayed_update.mutex, OS_FOREVER);

		hdma2d.output_cfg.mode = (flags & SURFACE_ROTATED_MASK) ?
				HAL_DMA2D_M2M_TRANSFORM : HAL_DMA2D_M2M;
		hdma2d.output_cfg.output_pitch = dst_pitch;
		hdma2d.output_cfg.color_format = graphic_buffer_get_pixel_format(dstbuf);
		hal_dma2d_config_output(&hdma2d);

		if (flags & SURFACE_ROTATED_90) {
			hdma2d.layer_cfg[1].input_width = dst_h;
			hdma2d.layer_cfg[1].input_height = dst_w;
		} else {
			hdma2d.layer_cfg[1].input_width = dst_w;
			hdma2d.layer_cfg[1].input_height = dst_h;
		}

		hdma2d.layer_cfg[1].color_format = src_pixel_format;
		hdma2d.layer_cfg[1].input_pitch = src_pitch;
		hal_dma2d_config_layer(&hdma2d, HAL_DMA2D_FOREGROUND_LAYER);

		if (flags & SURFACE_ROTATED_MASK) {
			hdma2d.trans_cfg.mode = (flags & SURFACE_ROTATED_90) ? HAL_DMA2D_ROT_90 : 0;
			if (flags & SURFACE_ROTATED_180)
				hdma2d.trans_cfg.mode += HAL_DMA2D_ROT_180;

			hal_dma2d_config_transform(&hdma2d);

			res = hal_dma2d_transform_start(&hdma2d, (uint32_t)src,
			        (uint32_t)dst, 0, 0, dst_w, dst_h);
		} else {
			res = hal_dma2d_start(&hdma2d, (uint32_t)src, (uint32_t)dst,
					hdma2d.layer_cfg[1].input_width, hdma2d.layer_cfg[1].input_height);
		}

		if (res < 0) {
			/* fallback to CPU, so make sure previous dma2d ops finished */
			hal_dma2d_poll_transfer(&hdma2d, -1);
		}

		os_mutex_unlock(&delayed_update.mutex);
	}
#endif /* CONFIG_DMA2D_HAL */

	/**
	 * TODO: add pixel format software convertion
	 */
	if (res < 0) {
		if (flags & SURFACE_ROTATED_MASK) {
			SYS_LOG_ERR("no sw rotation");
			return -ENOSYS;
		}

		uint16_t copy_bytes = dst_w * src_px_size;
		dst = mem_addr_to_uncache(dst);

		if (src_pixel_format == graphic_buffer_get_pixel_format(dstbuf)) {
			if (copy_bytes == dst_pitch && copy_bytes == src_pitch) {
				ui_memcpy(dst, mem_addr_to_uncache(src), copy_bytes * dst_h);
				ui_memsetcpy_wait_finish(5000);
			} else {
				for (int j = dst_h; j > 0; j--) {
					mem_dcache_flush(src, copy_bytes);
					memcpy(dst, src, copy_bytes);

					dst += dst_pitch;
					src += src_pitch;
				}
			}
		} else {
			for (int j = dst_h; j > 0; j--) {
				mem_dcache_flush(src, copy_bytes);
				int ret = sw_convert_color_buffer(dst, graphic_buffer_get_pixel_format(dstbuf),
								src, src_pixel_format, dst_w);
				if (ret < 0) {
					SYS_LOG_ERR("sw pixel format convert failed: %x -> %x", src_pixel_format,
								graphic_buffer_get_pixel_format(dstbuf));
					break;
				}

				dst += dst_pitch;
				src += src_pitch;
			}
		}

		mem_writebuf_clean_all();
	}

	return res;
}

#endif /* CONFIG_SURFACE_MAX_BUFFER_COUNT > 0 */

#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 1

static void _surface_swapbuf(surface_t *surface)
{
	graphic_buffer_t *backbuf = surface->buffers[surface->draw_idx];
	graphic_buffer_t *frontbuf = surface->buffers[surface->draw_idx ? 0 : 1] ;
	uint8_t *frontptr;
	uint16_t pitch, stride;
	uint8_t bytes_per_pixel;

#if defined(CONFIG_TRACING) && defined(CONFIG_UI_SERVICE)
	ui_view_context_t *view = surface->user_data[SURFACE_CB_POST];
	os_strace_u32(SYS_TRACE_ID_VIEW_SWAPBUF, view->entry->id);
#endif

	frontptr = (uint8_t *)graphic_buffer_get_bufptr(
			frontbuf, surface->dirty_area.x1, surface->dirty_area.y1);
	bytes_per_pixel = graphic_buffer_get_bits_per_pixel(frontbuf) / 8;
	stride = graphic_buffer_get_stride(frontbuf);
	pitch = stride * bytes_per_pixel;

#ifdef CONFIG_DMA2D_HAL
	surface->swapping = 1;
	SYS_LOG_DBG("%p swap pending", surface);
#endif /* CONFIG_DMA2D_HAL */

	_surface_buffer_copy(backbuf, &surface->dirty_area, frontptr, surface->pixel_format, stride, 0);

#if defined(CONFIG_TRACING) && defined(CONFIG_UI_SERVICE)
	os_strace_end_call_u32(SYS_TRACE_ID_VIEW_SWAPBUF, view->entry->id);
#endif
}

#ifdef CONFIG_DMA2D_HAL
static void _surface_swapbuf_wait_finish(surface_t *surface)
{
	if (surface->swapping) {
		_surface_dma2d_poll();
		surface->swapping = 0;
	}
}
#endif /* CONFIG_DMA2D_HAL */

#endif /* CONFIG_SURFACE_MAX_BUFFER_COUNT > 1 */
