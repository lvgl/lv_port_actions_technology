/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <string.h>
#include <sys/slist.h>
#include <zephyr.h>
#include <drivers/display/display_engine.h>
#include <display/display_composer.h>

#include <logging/log.h>

#define SYS_LOG_DOMAIN "composer"

#define CONFIG_COMPOSER_POST_NO_WAIT
#define CONFIG_COMPOSER_NUM_POST_PARTS (7)

/* must be power of 2; and must be 1 if both DMA and DE post available */
#define NUM_POST_ENTRIES  (CONFIG_COMPOSER_NUM_POST_PARTS * 2)
#define NUM_POST_LAYERS   (2)

typedef struct post_entry {
	uint32_t flags;

	display_layer_t ovls[NUM_POST_LAYERS];
	display_buffer_t bufs[NUM_POST_LAYERS];

	graphic_buffer_t *graphic_bufs[NUM_POST_LAYERS];
	display_composer_post_cleanup_t cleanup_cb[NUM_POST_LAYERS];
	void *cleanup_data[NUM_POST_LAYERS];
} post_entry_t;

typedef struct display_composer {
	/* supported maximum layers */
	uint8_t max_layers;
	uint8_t post_pending : 1;
	uint8_t post_inprog : 1;
	uint8_t disp_active : 1;

	/* post entries */
	uint8_t free_idx;
	uint8_t post_idx;
	uint8_t complete_idx;
	uint8_t post_cnt;

	post_entry_t post_entries[NUM_POST_ENTRIES];
	struct k_spinlock post_lock;
#ifndef CONFIG_COMPOSER_POST_NO_WAIT
	struct k_sem post_sem;
#endif

	/* display engine device */
	const struct device *de_dev;
	int de_inst;

	/* display (panel) device */
	const struct device *disp_dev;
	/* display device callback */
	struct display_callback disp_cb;
	/* display write pixel format */
	uint32_t disp_pixel_formats;

	/* user display callback */
	const struct display_callback *user_cb;

#ifdef CONFIG_DISPLAY_COMPOSER_DEBUG_FPS
	uint32_t frame_timestamp;
	uint16_t frame_cnt;
#endif

#ifdef CONFIG_DISPLAY_COMPOSER_DEBUG_VSYNC
	uint16_t vsync_cnt;
	uint32_t vsync_timestamp; /* measure in cycles */
	uint32_t vsync_period; /* measure in cycles */
#endif
} display_composer_t;

/* global prototypes */
extern const ui_region_t * display_composer_opt_full_screen_areas(int *num_regions);

/* static prototypes */
static void _composer_display_vsync_handler(const struct display_callback *callback, uint32_t timestamp);
static void _composer_display_complete_handler(const struct display_callback *callback);
static void _composer_display_pm_notify_handler(const struct display_callback *callback, uint32_t pm_action);
static uint8_t _composer_num_free_entries_get(display_composer_t *composer);
static post_entry_t *_composer_find_free_entry(display_composer_t *composer);
static int _composer_post_top_entry(display_composer_t *composer, bool require_not_first);
static void _composer_cleanup_entry(display_composer_t *composer, post_entry_t *entry);

/* static variables */
static display_composer_t display_composer __in_section_unique(ram.noinit.display_composer);

static inline display_composer_t *_composer_get(void)
{
	return &display_composer;
}

int display_composer_init(void)
{
	display_composer_t *composer = _composer_get();
	union {
		struct display_capabilities panel;
		struct display_engine_capabilities engine;
	} capabilities;

	memset(composer, 0, sizeof(*composer));
	composer->disp_active = 1;

	composer->disp_dev = device_get_binding(CONFIG_LCD_DISPLAY_DEV_NAME);
	if (composer->disp_dev == NULL) {
		SYS_LOG_ERR("cannot find display " CONFIG_LCD_DISPLAY_DEV_NAME);
		return -ENODEV;
	}

	composer->de_dev = device_get_binding(CONFIG_DISPLAY_ENGINE_DEV_NAME);
	if (composer->de_dev) {
		composer->de_inst = display_engine_open(composer->de_dev,
				DISPLAY_ENGINE_FLAG_HIGH_PRIO | DISPLAY_ENGINE_FLAG_POST);
	} else {
		composer->de_inst = -1;
	}

	if (composer->de_inst >= 0) {
		display_engine_get_capabilities(composer->de_dev, &capabilities.engine);
		composer->max_layers = MIN(capabilities.engine.num_overlays, NUM_POST_LAYERS);
	} else {
		composer->max_layers = 1;
	}

	SYS_LOG_INF("supported layer num %d\n", composer->max_layers);

#ifndef CONFIG_COMPOSER_POST_NO_WAIT
	k_sem_init(&composer->post_sem, NUM_POST_ENTRIES, NUM_POST_ENTRIES);
#endif

	display_get_capabilities(composer->disp_dev, &capabilities.panel);
	composer->disp_pixel_formats = capabilities.panel.supported_pixel_formats;
	if (!(capabilities.panel.screen_info & SCREEN_INFO_VSYNC)) {
		SYS_LOG_WRN("vsync unsupported\n");
	}

	composer->disp_cb.vsync = _composer_display_vsync_handler;
	composer->disp_cb.complete = _composer_display_complete_handler;
	composer->disp_cb.pm_notify = _composer_display_pm_notify_handler;
	display_register_callback(composer->disp_dev, &composer->disp_cb);

	SYS_LOG_INF("composer initialized\n");
	return 0;
}

void display_composer_destroy(void)
{
	display_composer_t *composer = _composer_get();

	if (composer->disp_dev == NULL) {
		return;
	}

	display_unregister_callback(composer->disp_dev, &composer->disp_cb);

	if (composer->de_inst >= 0) {
		display_engine_close(composer->de_dev, composer->de_inst);
	}

	SYS_LOG_INF("composer finalized\n");
}

void display_composer_register_callback(const struct display_callback *callback)
{
	display_composer_t *composer = _composer_get();

	composer->user_cb = callback;
}

uint32_t display_composer_get_vsync_period(void)
{
	display_composer_t *composer = _composer_get();
	struct display_capabilities capabilities;

	if (composer->disp_dev == NULL) {
		return -ENODEV;
	}

	capabilities.vsync_period = 0;
	display_get_capabilities(composer->disp_dev, &capabilities);

	/* fallback to refresh rate 60 Hz */
	return capabilities.vsync_period ? capabilities.vsync_period : 16667;
}

int display_composer_get_geometry(
		uint16_t *width, uint16_t *height, uint32_t *pixel_format)
{
	display_composer_t *composer = _composer_get();
	struct display_capabilities capabilities;

	if (composer->disp_dev == NULL) {
		return -ENODEV;
	}

	display_get_capabilities(composer->disp_dev, &capabilities);

	if (width) {
		*width = capabilities.x_resolution;
	}

	if (height) {
		*height = capabilities.y_resolution;
	}

	if (pixel_format) {
		*pixel_format = capabilities.current_pixel_format;
	}

	return 0;
}

int display_composer_set_blanking(bool blanking_on)
{
	display_composer_t *composer = _composer_get();
	int res = 0;

	if (composer->disp_dev == NULL) {
		return -ENODEV;
	}

	if (blanking_on) {
		res = display_blanking_on(composer->disp_dev);
	} else {
		res = display_blanking_off(composer->disp_dev);
	}

	return res;
}

int display_composer_set_brightness(uint8_t brightness)
{
	display_composer_t *composer = _composer_get();

	if (composer->disp_dev == NULL) {
		return -ENODEV;
	}

	return display_set_brightness(composer->disp_dev, brightness);
}

int display_composer_set_contrast(uint8_t contrast)
{
	display_composer_t *composer = _composer_get();

	if (composer->disp_dev == NULL) {
		return -ENODEV;
	}

	return display_set_contrast(composer->disp_dev, contrast);
}

void display_composer_round(ui_region_t *region)
{
	/*
	 * display_write() (DMA path) requires the buffer address, and the byte length
	 * corresponding to with and stride respectively are 4-byte aligned.
	 *
	 * Also some LCD driver IC, like GC9C01, requires the position and dims are both even.
	 */
	region->x1 &= ~0x1;
	region->y1 &= ~0x1;
	region->x2 |= 0x1;
	region->y2 |= 0x1;
}

static void _composer_display_vsync_handler(const struct display_callback *callback, uint32_t timestamp)
{
	display_composer_t *composer = CONTAINER_OF(callback, display_composer_t, disp_cb);

	if (composer->post_cnt > 0 && !composer->post_inprog) {
		_composer_post_top_entry(composer, false);
	}

	if (composer->user_cb && composer->user_cb->vsync) {
		composer->user_cb->vsync(composer->user_cb, timestamp);
	}

#ifdef CONFIG_DISPLAY_COMPOSER_DEBUG_VSYNC
	composer->vsync_period = timestamp - composer->vsync_timestamp;
	composer->vsync_timestamp = timestamp;

	if (++composer->vsync_cnt == 1024) {
		LOG_INF("vsync period %u us\n", k_cyc_to_us_near32(composer->vsync_period));
		composer->vsync_cnt = 0;
	}
#endif
}

static void _composer_display_complete_handler(const struct display_callback *callback)
{
	display_composer_t *composer = CONTAINER_OF(callback, display_composer_t, disp_cb);

	composer->post_inprog = 0;
	if (--composer->post_cnt > 0) {
		_composer_post_top_entry(composer, true);
	}

	post_entry_t *entry = &composer->post_entries[composer->complete_idx];
	_composer_cleanup_entry(composer, entry);
	if (++composer->complete_idx >= NUM_POST_ENTRIES)
		composer->complete_idx = 0;

	if (composer->post_cnt == 0 && composer->user_cb && composer->user_cb->complete) {
		composer->user_cb->complete(composer->user_cb);
	}
}

static void _composer_display_pm_notify_handler(const struct display_callback *callback, uint32_t pm_action)
{
	display_composer_t *composer = CONTAINER_OF(callback, display_composer_t, disp_cb);

	composer->disp_active = (pm_action == PM_DEVICE_ACTION_LATE_RESUME);

	if (pm_action == PM_DEVICE_ACTION_EARLY_SUSPEND) {
		while (composer->post_cnt > 0) {
			SYS_LOG_INF("post cnt %d", composer->post_cnt);
			os_sleep(2);
		}
	}

	if (composer->user_cb && composer->user_cb->pm_notify) {
		composer->user_cb->pm_notify(composer->user_cb, pm_action);
	}
}

static uint8_t _composer_num_free_entries_get(display_composer_t *composer)
{
	return NUM_POST_ENTRIES - composer->post_cnt;
}

static post_entry_t *_composer_find_free_entry(display_composer_t *composer)
{
	post_entry_t *entry;

#ifdef CONFIG_COMPOSER_POST_NO_WAIT
	if (composer->post_cnt >= NUM_POST_ENTRIES)
		return NULL;
#else
	if (k_sem_take(&composer->post_sem, k_is_in_isr() ? K_NO_WAIT : K_FOREVER))
		return NULL;
#endif

	entry = &composer->post_entries[composer->free_idx];
	if (++composer->free_idx >= NUM_POST_ENTRIES)
		composer->free_idx = 0;

	memset(entry, 0, sizeof(*entry));

	return entry;
}

static int _composer_post_top_entry(display_composer_t *composer, bool require_not_first)
{
	post_entry_t *entry = &composer->post_entries[composer->post_idx];
	display_layer_t *ovls = entry->ovls;
	uint8_t num_layers = ovls[1].buffer ? 2 : 1;
	int res = -EINVAL;

	if (require_not_first && (entry->flags & FIRST_POST_IN_FRAME)) {
		return -EINVAL;
	}

#if 0
	/* Try display_write() first, then display_engine_compose */
	if (num_layers == 1 && ovls[0].buffer != NULL &&
		(ovls[0].buffer->desc.pixel_format & composer->disp_pixel_formats)) {
		res = display_write(composer->disp_dev, ovls[0].frame.x, ovls[0].frame.y,
				&ovls[0].buffer->desc, (void *)ovls[0].buffer->addr);
	}

	if (res < 0 && composer->de_inst >= 0) {
		res = display_engine_compose(composer->de_dev, composer->de_inst,
				NULL, ovls, num_layers, false);
	}
#else
	if (entry->flags & POST_PATH_BY_DE) {
		res = display_engine_compose(composer->de_dev, composer->de_inst,
				NULL, ovls, num_layers, false);
	} else {
		res = display_write(composer->disp_dev, ovls[0].frame.x, ovls[0].frame.y,
				&ovls[0].buffer->desc, (void *)ovls[0].buffer->addr);
	}
#endif

	assert(res >= 0);

	composer->post_inprog = (res >= 0);
	if (++composer->post_idx >= NUM_POST_ENTRIES)
		composer->post_idx = 0;

	return res;
}

static void _composer_cleanup_entry(display_composer_t *composer, post_entry_t *entry)
{
	for (int i = 0; i < ARRAY_SIZE(entry->graphic_bufs); i++) {
		if (entry->graphic_bufs[i]) {
			graphic_buffer_unref(entry->graphic_bufs[i]);
		}

		if (entry->cleanup_cb[i]) {
			entry->cleanup_cb[i](entry->cleanup_data[i]);
		}
	}

#ifndef CONFIG_COMPOSER_POST_NO_WAIT
	k_sem_give(&composer->post_sem);
#endif
}

int display_composer_simple_post(graphic_buffer_t *buffer,
		ui_region_t *crop, uint16_t x, uint16_t y)
{
	ui_layer_t layer;

	memset(&layer, 0, sizeof(layer));
	layer.buffer = buffer;

	if (crop) {
		memcpy(&layer.crop, crop, sizeof(*crop));
	} else {
		layer.crop.x2 = graphic_buffer_get_width(buffer) - 1;
		layer.crop.y2 = graphic_buffer_get_height(buffer) - 1;
	}

	layer.frame.x1 = x;
	layer.frame.y1 = y;
	layer.frame.x2 = x + ui_region_get_width(&layer.crop) - 1;
	layer.frame.y2 = y + ui_region_get_height(&layer.crop) - 1;

	return display_composer_post(&layer, 1, FIRST_POST_IN_FRAME | LAST_POST_IN_FRAME);
}

static int _composer_post_inner(const ui_layer_t *layers, int num_layers, uint32_t post_flags)
{
	display_composer_t *composer = _composer_get();
	post_entry_t *entry = NULL;
	int res = -EINVAL;
	int i;

	/* Get the free entry */
	entry = _composer_find_free_entry(composer);
	if (entry == NULL) {
		goto fail_cleanup_cb;
	}

	entry->flags = post_flags;

	/* Validate the buffer */
	for (i = 0; i < num_layers; i++) {
		if (layers[i].buffer) {
			entry->bufs[i].addr = (uint32_t)graphic_buffer_get_bufptr(
					layers[i].buffer, layers[i].crop.x1, layers[i].crop.y1);
			//if (entry->bufs[i].addr & 0x3)
			//	entry->flags |= POST_PATH_BY_DE;

			entry->bufs[i].desc.pitch = graphic_buffer_get_stride(layers[i].buffer);
			entry->bufs[i].desc.pixel_format =
					graphic_buffer_get_pixel_format(layers[i].buffer);
			entry->bufs[i].desc.buf_size = entry->bufs[i].desc.pitch * entry->bufs[i].desc.height *
					display_format_get_bits_per_pixel(entry->bufs[i].desc.pixel_format) / 8;
		} else {
			entry->bufs[i].addr = 0;
			entry->bufs[i].desc.pitch = 0;
			entry->bufs[i].desc.pixel_format = 0;
		}

		entry->bufs[i].desc.width = ui_region_get_width(&layers[i].frame);
		entry->bufs[i].desc.height = ui_region_get_height(&layers[i].frame);

		entry->ovls[i].buffer = (entry->bufs[i].addr > 0) ? &entry->bufs[i] : NULL;
		entry->ovls[i].frame.x = layers[i].frame.x1;
		entry->ovls[i].frame.y = layers[i].frame.y1;
		entry->ovls[i].frame.w = entry->bufs[i].desc.width;
		entry->ovls[i].frame.h = entry->bufs[i].desc.height;
		entry->ovls[i].color.full = layers[i].color.full;
		entry->ovls[i].blending = layers[i].blending;

		if (!layers[i].buf_resident) {
			entry->graphic_bufs[i] = layers[i].buffer;
			if (layers[i].buffer)
				graphic_buffer_ref(layers[i].buffer);
		}

		entry->cleanup_cb[i] = layers[i].cleanup_cb;
		entry->cleanup_data[i] = layers[i].cleanup_data;

		SYS_LOG_DBG("L-%d: ptr=0x%08x fmt=0x%02x stride=%u color=0x%08x blend=0x%x frame=(%d,%d,%d,%d)%s",
				i, entry->bufs[i].addr, entry->bufs[i].desc.pixel_format, entry->bufs[i].desc.pitch,
				layers[i].color.full, layers[i].blending, layers[i].frame.x1,
				layers[i].frame.y1, layers[i].frame.x2, layers[i].frame.y2,
				i == num_layers - 1 ? "\n" : "");
	}

	k_spinlock_key_t key = k_spin_lock(&composer->post_lock);
	composer->post_cnt++;
	if (!composer->post_inprog) {
		_composer_post_top_entry(composer, true);
	}
	k_spin_unlock(&composer->post_lock, key);

	return 0;
fail_cleanup_cb:
	for (i = 0; i < num_layers; i++) {
		if (layers[i].cleanup_cb)
			layers[i].cleanup_cb(layers[i].cleanup_data);
	}
	return res;
}

static int _composer_post_part(const ui_layer_t *layers, int num_layers,
		uint32_t *post_flags, const ui_region_t *window)
{
	ui_layer_t tmp_layers[NUM_POST_LAYERS];
	int i, j = 0;

	for (i = 0; i < num_layers; i++) {
		if (window->y1 <= layers[i].frame.y2 && window->y2 >= layers[i].frame.y2) {
			tmp_layers[j].cleanup_cb = layers[i].cleanup_cb;
			tmp_layers[j].cleanup_data = layers[i].cleanup_data;
		} else {
			tmp_layers[j].cleanup_cb = NULL;
		}

		if (ui_region_intersect(&tmp_layers[j].frame, &layers[i].frame, window) == false) {
			if (tmp_layers[j].cleanup_cb) {
				tmp_layers[j].cleanup_cb(tmp_layers[j].cleanup_data);
				tmp_layers[j].cleanup_cb = NULL;
			}

			continue;
		}

		tmp_layers[j].buffer = layers[i].buffer;
		tmp_layers[j].buf_resident = layers[i].buf_resident;
		tmp_layers[j].color = layers[i].color;
		tmp_layers[j].blending = layers[i].blending;
		if (layers[i].buffer) {
			tmp_layers[j].crop.x1 = layers[i].crop.x1 + tmp_layers[j].frame.x1 - layers[i].frame.x1;
			tmp_layers[j].crop.y1 = layers[i].crop.y1 + tmp_layers[j].frame.y1 - layers[i].frame.y1;
		}

		j++;
	}

	if (j > 0 && _composer_post_inner(tmp_layers, j, *post_flags) == 0) {
		*post_flags &= ~FIRST_POST_IN_FRAME;
		return 0;
	}

	return -EINVAL;
}

int display_composer_post(const ui_layer_t *layers, int num_layers, uint32_t post_flags)
{
	display_composer_t *composer = _composer_get();
	const ui_region_t * clip_areas;
	int res, num, i;

#ifdef CONFIG_DISPLAY_COMPOSER_DEBUG_FPS
	if (post_flags & LAST_POST_IN_FRAME) {
		uint32_t timestamp = k_cycle_get_32();

		++composer->frame_cnt;
		if ((timestamp - composer->frame_timestamp) >= sys_clock_hw_cycles_per_sec()) {
			LOG_INF("post fps %u\n", composer->frame_cnt);
			composer->frame_cnt = 0;
			composer->frame_timestamp = timestamp;
		}
	}
#endif

	if (composer->disp_dev == NULL) {
		SYS_LOG_ERR("composer not initialized");
		goto fail_cleanup_cb;
	}

	if (composer->disp_active == 0) {
		SYS_LOG_DBG("display inactive");
		goto fail_cleanup_cb;
	}

	if (num_layers <= 0 || num_layers > composer->max_layers) {
		SYS_LOG_ERR("invalid layer num %d", num_layers);
		goto fail_cleanup_cb;
	}

	if (num_layers > 1 || layers[0].buffer == NULL ||
		!(graphic_buffer_get_pixel_format(layers[0].buffer) & composer->disp_pixel_formats)) {
		post_flags |= POST_PATH_BY_DE;

		if (composer->de_inst < 0) {
			SYS_LOG_ERR("DE path not unavailable");
			goto fail_cleanup_cb;
		}
	}

	clip_areas = display_composer_opt_full_screen_areas(&num);
	if (clip_areas == NULL) num = 1;

#ifdef CONFIG_COMPOSER_POST_NO_WAIT
	if (_composer_num_free_entries_get(composer) < num)
#else
	if (k_is_in_isr() && _composer_num_free_entries_get(composer) < num)
#endif
	{
#ifdef CONFIG_DISPLAY_COMPOSER_DEBUG_VSYNC
		SYS_LOG_WRN("drop 1 frame (last vsync %u)", k_cyc_to_us_near32(composer->vsync_period));
#else
		SYS_LOG_WRN("drop 1 frame");
#endif
		goto fail_cleanup_cb;
	}

	if (clip_areas && (post_flags & POST_FULL_SCREEN_OPT)) {
		uint32_t flags = post_flags & ~LAST_POST_IN_FRAME;
		_composer_post_part(layers, num_layers, &flags, &clip_areas[0]);

		for (i = 1; i < num - 1; i++)
			_composer_post_part(layers, num_layers, &flags, &clip_areas[i]);

		flags |= (post_flags & LAST_POST_IN_FRAME);

		res = _composer_post_part(layers, num_layers, &flags, &clip_areas[num - 1]);
	} else {
		return _composer_post_inner(layers, num_layers, post_flags);
	}

	return 0;
fail_cleanup_cb:
	for (i = 0; i < num_layers; i++) {
		if (layers[i].cleanup_cb)
			layers[i].cleanup_cb(layers[i].cleanup_data);
	}
	return -EINVAL;
}
