/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_CUSTOMER

/*********************
 *      INCLUDES
 *********************/

#include <assert.h>
#include <string.h>
#include <sys/slist.h>
#include <zephyr.h>
#include <drivers/display/display_engine.h>
#ifdef CONFIG_TRACING
#  include <tracing/tracing.h>
#endif

#include <display/display_composer.h>
#include <board_cfg.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(composer, LOG_LEVEL_INF);

/*********************
 *      DEFINES
 *********************/

#define CONFIG_COMPOSER_POST_NO_WAIT

#ifndef CONFIG_PANEL_ROUND_SHAPE
#  define CONFIG_PANEL_ROUND_SHAPE  0
#endif

#ifndef CONFIG_PANEL_NUM_SCREEN_AREAS
#  define CONFIG_PANEL_NUM_SCREEN_AREAS  1
#endif

#ifdef CONFIG_PANEL_FULL_SCREEN_OPT_AREA
static const ui_region_t g_screen_areas[] = CONFIG_PANEL_FULL_SCREEN_OPT_AREA;

#  define NUM_SCREEN_AREAS ARRAY_SIZE(g_screen_areas)
#  define HAS_SCREEN_AREAS (1)

#elif CONFIG_PANEL_ROUND_SHAPE
#  if CONFIG_PANEL_NUM_SCREEN_AREAS >= 7
#    define NUM_SCREEN_AREAS (7)
#  elif CONFIG_PANEL_NUM_SCREEN_AREAS >= 3
#    define NUM_SCREEN_AREAS (3)
#  else
#    define NUM_SCREEN_AREAS (1)
#  endif

#  if NUM_SCREEN_AREAS > 1
#    define HAS_SCREEN_AREAS (1)
static ui_region_t g_screen_areas[NUM_SCREEN_AREAS];
#  endif

#else
#  define NUM_SCREEN_AREAS (1)
#  define HAS_SCREEN_AREAS (0)
#endif /* CONFIG_PANEL_FULL_SCREEN_OPT_AREA */

/* 3 frames */
#if defined(CONFIG_UI_SERVICE) && defined(CONFIG_SURFACE_ZERO_BUFFER) && defined(CONFIG_LVGL)
#  define NUM_POST_ENTRIES  (NUM_SCREEN_AREAS * CONFIG_LV_VDB_NUM)
#else
#  define NUM_POST_ENTRIES  (NUM_SCREEN_AREAS * 3)
#endif

/**********************
 *      TYPEDEFS
 **********************/

typedef struct post_entry {
	uint16_t flags;
	uint8_t n_ovls;

	display_layer_t ovls[NUM_POST_LAYERS];
	display_buffer_t bufs[NUM_POST_LAYERS];

	graphic_buffer_t *graphic_bufs[NUM_POST_LAYERS];
	display_composer_post_cleanup_t cleanup_cb[NUM_POST_LAYERS];
	void *cleanup_data[NUM_POST_LAYERS];
} post_entry_t;

typedef struct display_composer {
	/* display (panel) device */
	const struct device *disp_dev;
	/* display device callback */
	struct display_callback disp_cb;
	/* display capabilities */
	struct display_capabilities disp_cap;
	/* display framebuffer */
	display_buffer_t disp_fb;

	/* display engine device */
	const struct device *de_dev;
	int de_inst;

	/* user display callback */
	const struct display_callback *user_cb;

	/* post entries */
	post_entry_t post_entries[NUM_POST_ENTRIES];
	uint8_t cplt_idx;
	uint8_t free_idx;
	uint8_t post_idx;
	/* post state */
	uint8_t post_cnt; /* number of posts transferring and waiting to progress */
	uint8_t post_inprog; /* any post is transferring */
	uint8_t post_eof; /* flag to indicate End of Frame, used to check post integrity */

	/* control post frame rate */
	uint8_t post_frame_period;
	uint8_t post_frame_cnt;

	/* display state */
	uint8_t max_layers : 3;     /* supported maximum layers */
	uint8_t disp_has_vsync : 1; /* test really has vsyn signal */
	uint8_t disp_active : 1;    /* has power or not */
	uint8_t disp_lowpower : 1;  /* in low power state or not */
	uint8_t disp_state_prechanged : 1; /* going to change state */
	uint8_t first_frame : 1;    /* first frame after resume */

	/* test frame crossing vsync or not */
	uint8_t vsync_counter;
	uint8_t frame_start_vsync_cnt;
	uint32_t frame_start_cycle;

#ifdef CONFIG_DISPLAY_COMPOSER_DEBUG_FPS
	uint32_t frame_timestamp;
	uint16_t frame_cnt;
#endif

#ifdef CONFIG_DISPLAY_COMPOSER_DEBUG_VSYNC
	uint32_t vsync_timestamp; /* measure in cycles */
	uint32_t vsync_print_timestamp; /* measure in cycles */
#endif

	struct k_spinlock post_lock;
#ifndef CONFIG_COMPOSER_POST_NO_WAIT
	struct k_sem post_sem;
#endif
} display_composer_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void _composer_display_vsync_handler(const struct display_callback *callback, uint32_t timestamp);
static void _composer_display_complete_handler(const struct display_callback *callback);
static void _composer_display_pm_notify_handler(const struct display_callback *callback, uint32_t pm_action);
static uint8_t _composer_num_free_entries_get(display_composer_t *composer);
static post_entry_t *_composer_find_free_entry(display_composer_t *composer);
static int _composer_post_top_entry(display_composer_t *composer, bool require_not_first);
static void _composer_cleanup_entry(display_composer_t *composer, post_entry_t *entry);
static void _composer_dump_entry(post_entry_t *entry);

static int _composer_post_entry_noram(display_composer_t *composer);
static void _composer_de_complete_handler(int status, uint16_t cmd_seq, void *user_data);

#if !defined(CONFIG_PANEL_FULL_SCREEN_OPT_AREA) && HAS_SCREEN_AREAS
static void _compute_round_screen_areas(uint16_t screen_size, ui_region_t areas[], uint8_t n_areas);
#endif

/**********************
 *  STATIC VARIABLES
 **********************/

static display_composer_t display_composer __in_section_unique(ram.noinit.display_composer);

/**********************
 *  INLINE FUNCTIONS
 **********************/

static inline display_composer_t *_composer_get(void)
{
	return &display_composer;
}

static inline bool _composer_has_gram(display_composer_t *composer)
{
	return (!composer->disp_fb.addr && (composer->disp_cap.screen_info & SCREEN_INFO_ZERO_BUFFER)) ? false : true;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

int display_composer_init(void)
{
	display_composer_t *composer = _composer_get();

	memset(composer, 0, sizeof(*composer));
	composer->disp_active = 1;
	composer->first_frame = 1;

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
		struct display_engine_capabilities de_cap;
		display_engine_get_capabilities(composer->de_dev, &de_cap);
		composer->max_layers = MIN(de_cap.num_overlays, NUM_POST_LAYERS);
	} else {
		composer->max_layers = 1;
	}

	SYS_LOG_INF("supported layer num %d", composer->max_layers);

#ifndef CONFIG_COMPOSER_POST_NO_WAIT
	k_sem_init(&composer->post_sem, NUM_POST_ENTRIES, NUM_POST_ENTRIES);
#endif

	display_get_capabilities(composer->disp_dev, &composer->disp_cap);
	composer->post_frame_period = 1;
	composer->post_eof = LAST_POST_IN_FRAME;

	composer->disp_fb.desc.pixel_format = composer->disp_cap.current_pixel_format;
	composer->disp_fb.desc.width = composer->disp_cap.x_resolution;
	composer->disp_fb.desc.height = composer->disp_cap.y_resolution;
	composer->disp_fb.desc.pitch = composer->disp_cap.x_resolution *
			display_format_get_bits_per_pixel(composer->disp_cap.current_pixel_format) / 8;
	composer->disp_fb.desc.buf_size = composer->disp_fb.desc.pitch * composer->disp_fb.desc.height;
	composer->disp_fb.addr = (uintptr_t)display_get_framebuffer(composer->disp_dev);

	if (composer->disp_fb.addr || (composer->disp_cap.screen_info & SCREEN_INFO_ZERO_BUFFER)) {
		assert(composer->de_inst >= 0);
		display_engine_register_callback(composer->de_dev, composer->de_inst,
			_composer_de_complete_handler, composer);
	}

	composer->disp_cb.vsync = _composer_display_vsync_handler;
	composer->disp_cb.complete = _composer_display_complete_handler;
	composer->disp_cb.pm_notify = _composer_display_pm_notify_handler;
	display_register_callback(composer->disp_dev, &composer->disp_cb);

#ifdef CONFIG_PANEL_FULL_SCREEN_OPT_AREA
	if (g_screen_areas[0].y1 != 0 ||
		g_screen_areas[NUM_SCREEN_AREAS - 1].y2 != composer->disp_cap.y_resolution - 1) {
		SYS_LOG_ERR("composer clip areas invalid\n");
	}
#elif HAS_SCREEN_AREAS
	_compute_round_screen_areas(composer->disp_cap.x_resolution, g_screen_areas, NUM_SCREEN_AREAS);
#endif

	SYS_LOG_INF("composer initialized\n");
	return 0;
}

void display_composer_destroy(void)
{
	SYS_LOG_INF("composer finalized\n");
}

void display_composer_register_callback(const struct display_callback *callback)
{
	display_composer_t *composer = _composer_get();

	composer->user_cb = callback;
}

void display_composer_set_post_period(uint8_t multiple)
{
	display_composer_t *composer = _composer_get();

	unsigned int key = os_irq_lock();

	composer->post_frame_period = multiple;
	composer->post_frame_cnt = 0;

	os_irq_unlock(key);
}

uint8_t display_composer_get_refresh_rate(void)
{
	display_composer_t *composer = _composer_get();

	return composer->disp_cap.refresh_rate;
}

int display_composer_get_geometry(uint16_t *width, uint16_t *height,
			uint32_t *pixel_format, uint8_t *round_screen)
{
	display_composer_t *composer = _composer_get();

	if (width) {
		*width = composer->disp_cap.x_resolution;
	}

	if (height) {
		*height = composer->disp_cap.y_resolution;
	}

	if (pixel_format) {
		*pixel_format = composer->disp_cap.current_pixel_format;
	}

	if (round_screen) {
		*round_screen = (composer->disp_cap.screen_info & SCREEN_INFO_ROUND_SHAPE) ? 1 : 0;
	}

	return 0;
}

uint16_t display_composer_get_orientation(void)
{
	display_composer_t *composer = _composer_get();
	return (uint16_t)composer->disp_cap.current_orientation * 90;
}

uint8_t display_composer_get_num_layers(void)
{
	display_composer_t *composer = _composer_get();

	return composer->max_layers;
}

int display_composer_set_brightness(uint8_t brightness)
{
	display_composer_t *composer = _composer_get();

	if (composer->disp_dev == NULL) {
		return -ENODEV;
	}

	return display_set_brightness(composer->disp_dev, brightness);
}

void display_composer_round(ui_region_t *region)
{
	display_composer_t *composer = _composer_get();
	struct display_capabilities *cap = &composer->disp_cap;

	/* Some LCD driver IC require even pixel alignment, so set even area if possible */

	if (cap->screen_info & SCREEN_INFO_X_ALIGNMENT_WIDTH) {
		region->x1 = 0;
		region->x2 = cap->x_resolution - 1;
	} else 	if (!(cap->x_resolution & 0x1)) {
		region->x1 &= ~0x1;
		region->x2 |= 0x1;
	}

	if (cap->screen_info & SCREEN_INFO_Y_ALIGNMENT_HEIGHT) {
		region->y1 = 0;
		region->y2 = cap->y_resolution - 1;
	} else 	if (!(cap->y_resolution & 0x1)) {
		region->y1 &= ~0x1;
		region->y2 |= 0x1;
	}
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#if !defined(CONFIG_PANEL_FULL_SCREEN_OPT_AREA) && HAS_SCREEN_AREAS
/**
 * Get the square root of a number
 * @param x integer which square root should be calculated
 * @param mask optional to skip some iterations if the magnitude of the root is known.
 * Set to 0x8000 by default.
 * If root < 16: mask = 0x80
 * If root < 256: mask = 0x800
 * Else: mask = 0x8000
 */
static uint32_t _sqrt_internal(uint32_t x, uint32_t mask)
{
	x = x << 8; /*To get 4 bit precision. (sqrt(256) = 16 = 4 bit)*/

	uint32_t root = 0;
	uint32_t trial;
	// http://ww1.microchip.com/...en/AppNotes/91040a.pdf
	do {
		trial = root + mask;
		if (trial * trial <= x) root = trial;
		mask = mask >> 1;
	} while (mask);

	return root >> 4;
}

static void _compute_round_screen_areas(uint16_t screen_size, ui_region_t areas[], uint8_t n_areas)
{
	const uint16_t align = (screen_size & 0x1) ? 1 : 2;
	uint16_t radius = (screen_size + 1) / 2;
	uint16_t x1 = (uint16_t)(radius * 0.29289f) & ~(align - 1);
	uint16_t y1 = x1;
	uint16_t x2 = 0;
	uint16_t y2 = 0;
	uint32_t area2 = 0;

	if (n_areas == 3) {
		areas[0] = (ui_region_t) { x1, 0, screen_size - x1 - 1, y1 - 1 };
		areas[1] = (ui_region_t) { 0, y1, screen_size - 1, screen_size - y1 - 1 };
		areas[2] = (ui_region_t) { x1, screen_size - y1, screen_size - x1 - 1, screen_size - 1 };
	} else {
		for (uint32_t y = align; y < y1; y += align) {
			uint16_t x = (radius - _sqrt_internal((2 * radius - y) * y, 0x800) - 1) & ~(align - 1);
			uint32_t area = (x - x1) * y;
			if (area > area2) {
				area2 = area;
				x2 = x;
				y2 = y;
			}
		}

		areas[0] = (ui_region_t) { x2, 0, screen_size - x2 - 1, y2 - 1 };
		areas[1] = (ui_region_t) { x1, y2, screen_size - x1 - 1, y1 - 1 };
		areas[2] = (ui_region_t) { y2, y1, screen_size - y2 - 1, x2 - 1 };
		areas[3] = (ui_region_t) { 0, y1 + x2 - x1, screen_size - - 1, screen_size - (y1 + x2 - x1) - 1 };
		areas[4] = (ui_region_t) { y2, screen_size - (y1 + x2 - x1), screen_size - y2 - 1, screen_size - y1 - 1 };
		areas[5] = (ui_region_t) { x1, screen_size - y1, screen_size - x1 - 1, screen_size - y2 - 1 };
		areas[6] = (ui_region_t) { x2, screen_size - y2, screen_size - x2 - 1, screen_size - 1 };
	}

	for (int i = 0; i < n_areas; i++) {
#ifdef CONFIG_PANEL_OFFSET_X
		areas[i].x1 -= CONFIG_PANEL_OFFSET_X;
		areas[i].x2 -= CONFIG_PANEL_OFFSET_X;
#endif
#ifdef CONFIG_PANEL_OFFSET_Y
		areas[i].y1 -= CONFIG_PANEL_OFFSET_Y;
		areas[i].y2 -= CONFIG_PANEL_OFFSET_Y;
#endif
		SYS_LOG_INF("screen_area[%d]: %d %d %d %d", i, areas[i].x1, areas[i].y1, areas[i].x2, areas[i].y2);
	}
}
#endif /* !defined(CONFIG_PANEL_FULL_SCREEN_OPT_AREA) && HAS_SCREEN_AREAS */

static void _composer_display_vsync_handler(const struct display_callback *callback, uint32_t timestamp)
{
	display_composer_t *composer = CONTAINER_OF(callback, display_composer_t, disp_cb);

	if (!composer->disp_has_vsync) {
		composer->disp_has_vsync = 1;
		LOG_INF("has vsync\n");
	}

	composer->vsync_counter++;

	if (++composer->post_frame_cnt >= composer->post_frame_period) {
		composer->post_frame_cnt = 0;

		if (_composer_has_gram(composer) && composer->post_cnt > 0 && !composer->post_inprog) {
			_composer_post_top_entry(composer, false);
		}
	}

	if (composer->user_cb && composer->user_cb->vsync) {
		composer->user_cb->vsync(composer->user_cb, timestamp);
	}

#ifdef CONFIG_DISPLAY_COMPOSER_DEBUG_VSYNC
	if (timestamp - composer->vsync_print_timestamp >= sys_clock_hw_cycles_per_sec() * 8u) {
		LOG_INF("vsync %u us\n", k_cyc_to_us_near32(timestamp - composer->vsync_timestamp));
		composer->vsync_print_timestamp = timestamp;
	}

	composer->vsync_timestamp = timestamp;
#endif
}

static void _composer_complete_one_entry(display_composer_t *composer)
{
	post_entry_t *entry = &composer->post_entries[composer->cplt_idx];

	_composer_cleanup_entry(composer, entry);
	if (++composer->cplt_idx >= NUM_POST_ENTRIES) {
		composer->cplt_idx = 0;
	}

	--composer->post_cnt;
}

static void _composer_display_complete_handler(const struct display_callback *callback)
{
	display_composer_t *composer = CONTAINER_OF(callback, display_composer_t, disp_cb);
	post_entry_t *entry = &composer->post_entries[composer->cplt_idx];

	composer->post_inprog = 0;

	if (entry->flags & LAST_POST_IN_FRAME) {
		if (composer->vsync_counter != composer->frame_start_vsync_cnt) {
			uint32_t frame_cycles = k_cycle_get_32() - composer->frame_start_cycle;
			SYS_LOG_WRN("frame refresh over vsync: %u us\n", k_cyc_to_us_floor32(frame_cycles));

			sys_trace_void(SYS_TRACE_ID_COMPOSER_OVERVSYNC);
		}
	}

	if (--composer->post_cnt > 0) {
		_composer_post_top_entry(composer, true);
	}

	_composer_cleanup_entry(composer, entry);
	if (++composer->cplt_idx >= NUM_POST_ENTRIES) {
		composer->cplt_idx = 0;
	}

	if (composer->post_cnt == 0 && composer->user_cb && composer->user_cb->complete) {
		composer->user_cb->complete(composer->user_cb);
	}
}

static void _composer_de_complete_handler(int status, uint16_t cmd_seq, void *user_data)
{
	display_composer_t *composer = user_data;
	display_buffer_t *fb = &composer->disp_fb;

	/* TODO: recovery the display */
	assert(status == 0);

	if (fb->addr) {
		_composer_display_complete_handler(&composer->disp_cb);
		return;
	}

	_composer_complete_one_entry(composer);

	if (composer->post_cnt == 0 && composer->user_cb && composer->user_cb->complete) {
		composer->user_cb->complete(composer->user_cb);
	}
}

static void _composer_drop_all_entries(display_composer_t *composer)
{
	k_spinlock_key_t key = k_spin_lock(&composer->post_lock);

	while (composer->post_cnt > 0) {
		_composer_complete_one_entry(composer);
	}

	composer->post_inprog = 0;
	composer->post_idx = composer->free_idx;
	composer->cplt_idx =composer->free_idx;

	if (composer->user_cb && composer->user_cb->complete) {
		composer->user_cb->complete(composer->user_cb);
	}

	k_spin_unlock(&composer->post_lock, key);
}

static void _composer_display_pm_notify_handler(const struct display_callback *callback, uint32_t pm_action)
{
	display_composer_t *composer = CONTAINER_OF(callback, display_composer_t, disp_cb);

	if (pm_action == PM_DEVICE_ACTION_EARLY_SUSPEND ||
		pm_action == PM_DEVICE_ACTION_LOW_POWER ||
		pm_action == PM_DEVICE_ACTION_TURN_OFF) {
		uint8_t wait_post_cnt = _composer_has_gram(composer) ? 0 : 1;
		int try_cnt = 500;

		composer->disp_state_prechanged = 1;

		for (; (composer->post_cnt > wait_post_cnt || !composer->post_eof) && try_cnt > 0; try_cnt--) {
			SYS_LOG_INF("post cnt %d", composer->post_cnt);
			os_sleep(2);
		}

		composer->disp_active = 0; /* lock the post temporarily */
		composer->disp_state_prechanged = 0;

		if (_composer_has_gram(composer) && composer->post_cnt > 0) {
			SYS_LOG_ERR("wait timeout, drop all post entries");
			_composer_drop_all_entries(composer);
		}

		/* notify the user to lock the draw and stop posting */
		if (pm_action != PM_DEVICE_ACTION_TURN_OFF &&
			composer->user_cb && composer->user_cb->pm_notify) {
			composer->user_cb->pm_notify(composer->user_cb, pm_action);
		}

		if (pm_action == PM_DEVICE_ACTION_LOW_POWER) {
			composer->disp_active = 1;
			composer->disp_lowpower = 1;
		} else {
			composer->disp_has_vsync = 0;
		}

		composer->post_eof = LAST_POST_IN_FRAME;
	} else if (composer->disp_lowpower) {
		composer->disp_lowpower = 0;
	} else {
		composer->disp_active = 1;
		composer->first_frame = 1;

		if (composer->user_cb && composer->user_cb->pm_notify) {
			composer->user_cb->pm_notify(composer->user_cb, pm_action);
		}
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
	display_buffer_t *fb = &composer->disp_fb;
	post_entry_t *entry = &composer->post_entries[composer->post_idx];
	display_layer_t *ovls = entry->ovls;
	int res;

	if (require_not_first && (entry->flags & FIRST_POST_IN_FRAME)) {
		return -EINVAL;
	}

	composer->post_inprog = 1;

	if (entry->flags & FIRST_POST_IN_FRAME) {
		composer->frame_start_vsync_cnt = composer->vsync_counter;
		composer->frame_start_cycle = k_cycle_get_32();
	}

	if (entry->flags & POST_PATH_BY_DE) {
		res = display_engine_compose(composer->de_dev, composer->de_inst,
				fb->addr ? fb : NULL, ovls, entry->n_ovls);
	} else {
		res = display_write(composer->disp_dev, ovls[0].frame.x, ovls[0].frame.y,
				&ovls[0].buffer->desc, (void *)ovls[0].buffer->addr);
	}

	if (res < 0) {
		SYS_LOG_ERR("post failed\n");
		_composer_dump_entry(entry);
		assert(0);
		return res;
	}

	if (++composer->post_idx >= NUM_POST_ENTRIES)
		composer->post_idx = 0;

	return 0;
}

static int _composer_post_entry_noram(display_composer_t *composer)
{
	post_entry_t *entry = &composer->post_entries[composer->post_idx];
	int res;

	res = display_engine_compose(composer->de_dev, composer->de_inst,
			NULL, entry->ovls, entry->n_ovls);
	if (res < 0) {
		SYS_LOG_ERR("post failed\n");
		_composer_dump_entry(entry);
		assert(0);
		return res;
	}

	if (++composer->post_idx >= NUM_POST_ENTRIES)
		composer->post_idx = 0;

	return 0;
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

static void _composer_dump_entry(post_entry_t *entry)
{
	display_layer_t *ovls = entry->ovls;
	uint8_t num_layers = ovls[1].buffer ? 2 : 1;
	uint8_t i;

	for (i = 0; i < num_layers; i++) {
		SYS_LOG_INF("L-%d: ptr=0x%08x fmt=0x%02x stride=%u color=0x%08x blend=0x%x frame=(%d,%d-%d,%d)%s",
			i, entry->bufs[i].addr, entry->bufs[i].desc.pixel_format, entry->bufs[i].desc.pitch,
			ovls[i].color.full, ovls[i].blending, ovls[i].frame.x,
			ovls[i].frame.y, ovls[i].frame.w, ovls[i].frame.h,
			i == num_layers - 1 ? "\n" : "");
	}
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

	entry->flags = (uint16_t)post_flags;
	entry->n_ovls = (uint8_t)num_layers;

	/* Validate the buffer */
	for (i = 0; i < num_layers; i++) {
		entry->graphic_bufs[i] = layers[i].buffer;
		if (layers[i].buffer) {
			graphic_buffer_ref(layers[i].buffer);
		}

		if (layers[i].buffer) {
			entry->ovls[i].buffer = &entry->bufs[i];
			entry->bufs[i].addr = (uint32_t)graphic_buffer_get_bufptr(
					layers[i].buffer, layers[i].crop.x1, layers[i].crop.y1);
			entry->bufs[i].desc.pixel_format =
					graphic_buffer_get_pixel_format(layers[i].buffer);
			entry->bufs[i].desc.pitch = graphic_buffer_get_stride(layers[i].buffer) *
					display_format_get_bits_per_pixel(entry->bufs[i].desc.pixel_format) / 8;
			//entry->bufs[i].desc.buf_size = entry->bufs[i].desc.pitch * entry->bufs[i].desc.height;
		} else {
			entry->ovls[i].buffer = NULL;
			entry->bufs[i].addr = 0;
			entry->bufs[i].desc.pitch = 0;
			entry->bufs[i].desc.pixel_format = 0;
		}

		entry->bufs[i].desc.width = ui_region_get_width(&layers[i].frame);
		entry->bufs[i].desc.height = ui_region_get_height(&layers[i].frame);

		entry->ovls[i].frame.x = layers[i].frame.x1;
		entry->ovls[i].frame.y = layers[i].frame.y1;
		entry->ovls[i].frame.w = entry->bufs[i].desc.width;
		entry->ovls[i].frame.h = entry->bufs[i].desc.height;
		entry->ovls[i].color.a = layers[i].plane_alpha;
		entry->ovls[i].blending = layers[i].blending;

		assert(layers[i].frame.x1 >= 0 && layers[i].frame.y1 >= 0);
		assert(entry->ovls[i].frame.w > 0 && layers[i].frame.x2 < composer->disp_cap.x_resolution);
		assert(entry->ovls[i].frame.h > 0 && layers[i].frame.y2 < composer->disp_cap.y_resolution);

		entry->cleanup_cb[i] = layers[i].cleanup_cb;
		entry->cleanup_data[i] = layers[i].cleanup_data;
	}

	k_spinlock_key_t key = k_spin_lock(&composer->post_lock);
	composer->post_cnt++;

	if (!_composer_has_gram(composer)) {
		_composer_post_entry_noram(composer);
	} else if (!composer->post_inprog) {
		_composer_post_top_entry(composer, composer->disp_lowpower ? false : true);
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

#if HAS_SCREEN_AREAS
static int _composer_post_areas(const ui_layer_t *layers, int num_layers, uint32_t post_flags)
{
	const uint32_t las_post_flag = post_flags & LAST_POST_IN_FRAME;
	ui_layer_t tmp_layers[NUM_POST_LAYERS];
	int8_t max_clip_idx[NUM_POST_LAYERS];
	int8_t max_post_idx = -1;
	int8_t i, j;

	for (j = num_layers - 1; j >= 0; j--) {
		max_clip_idx[j] = -1;

		for (i = NUM_SCREEN_AREAS - 1; i >= 0; i--) {
			if (ui_region_is_on(&layers[j].frame, &g_screen_areas[i])) {
				max_clip_idx[j] = i;
				break;
			}
		}

		if (max_clip_idx[j] < 0) {
			if (layers[j].cleanup_cb)
				layers[j].cleanup_cb(layers[j].cleanup_data);
		} else if (max_clip_idx[j] > max_post_idx) {
			max_post_idx = max_clip_idx[j];
		}
	}

	if (max_post_idx < 0)
		return -EINVAL;

	post_flags &= ~LAST_POST_IN_FRAME;

	for (i = 0; i <= max_post_idx; i++) {
		int8_t num = 0;

		for (j = 0; j < num_layers; j++) {
			if (max_clip_idx[j] < 0)
				continue;

			if (!ui_region_intersect(&tmp_layers[num].frame, &layers[j].frame, &g_screen_areas[i]))
				continue;

			if (i == max_clip_idx[j]) {
				tmp_layers[num].cleanup_cb = layers[j].cleanup_cb;
				tmp_layers[num].cleanup_data = layers[j].cleanup_data;
			} else {
				tmp_layers[num].cleanup_cb = NULL;
			}

			tmp_layers[num].blending = layers[j].blending;
			tmp_layers[num].plane_alpha = layers[j].plane_alpha;
			tmp_layers[num].buffer = layers[j].buffer;
			if (tmp_layers[num].buffer) {
				tmp_layers[num].crop.x1 = layers[j].crop.x1 +
						tmp_layers[num].frame.x1 - layers[j].frame.x1;
				tmp_layers[num].crop.y1 = layers[j].crop.y1 +
						tmp_layers[num].frame.y1 - layers[j].frame.y1;
			}

			num++;
		}

		if (num > 0) {
			if (i == max_post_idx)
				post_flags |= las_post_flag;

			_composer_post_inner(tmp_layers, num, post_flags);
			post_flags &= ~FIRST_POST_IN_FRAME;
		}
	}

	return 0;
}
#endif /* HAS_SCREEN_AREAS */

int display_composer_post(const ui_layer_t *layers, int num_layers, uint32_t post_flags)
{
	display_composer_t *composer = _composer_get();
	uint8_t num_free_entries;
	int res = 0, i;

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

	if (composer->disp_has_vsync == 0 || composer->disp_active == 0 ||
		(composer->disp_state_prechanged && composer->post_eof)) {
		SYS_LOG_DBG("display active %d, has_vsync %d",
				composer->disp_active, composer->disp_has_vsync);
		goto fail_cleanup_cb;
	}

	if (num_layers <= 0 || num_layers > composer->max_layers) {
		SYS_LOG_ERR("invalid layer num %d", num_layers);
		goto fail_cleanup_cb;
	}

	if (num_layers > 1 || layers[0].buffer == NULL ||
		!(layers[0].buffer->pixel_format & composer->disp_cap.supported_pixel_formats)) {
		post_flags |= POST_PATH_BY_DE;

		if (composer->de_inst < 0) {
			SYS_LOG_ERR("DE path not unavailable");
			goto fail_cleanup_cb;
		}
	}

	num_free_entries = _composer_num_free_entries_get(composer);
#ifdef CONFIG_COMPOSER_POST_NO_WAIT
	if (num_free_entries < NUM_SCREEN_AREAS)
#else
	if (k_is_in_isr() && num_free_entries < NUM_SCREEN_AREAS)
#endif
	{
		SYS_LOG_WRN("drop 1 frame (%d)", num_free_entries);
		sys_trace_void(SYS_TRACE_ID_COMPOSER_OVERFLOW);
		goto fail_cleanup_cb;
	}

#if HAS_SCREEN_AREAS
	if (NUM_SCREEN_AREAS > 1 && !composer->first_frame && _composer_has_gram(composer)) {
		res = _composer_post_areas(layers, num_layers, post_flags);
	} else {
		res = _composer_post_inner(layers, num_layers, post_flags);
	}
#else
	res = _composer_post_inner(layers, num_layers, post_flags);
#endif

	if (res == 0) {
		composer->first_frame = 0;
		composer->post_eof = (post_flags & LAST_POST_IN_FRAME);
	}

	return res;
fail_cleanup_cb:
	for (i = 0; i < num_layers; i++) {
		if (layers[i].cleanup_cb)
			layers[i].cleanup_cb(layers[i].cleanup_data);
	}
	return -EINVAL;
}

int display_composer_flush(unsigned int timeout)
{
	display_composer_t *composer = _composer_get();
	int8_t wait_post_cnt = _composer_has_gram(composer) ? 0 : 1;
	int n_frames;
	uint32_t uptime;

	if (composer->disp_dev == NULL) {
		return -ENODEV;
	}

	n_frames = (int)composer->post_cnt - wait_post_cnt;
	if (n_frames <= 0) {
		return 0;
	}

	uptime = os_uptime_get_32();

	do {
		if (!_composer_has_gram(composer) || composer->post_inprog ||
			(composer->disp_active && !composer->disp_lowpower)) {
			k_msleep(1);
		} else {
			k_spinlock_key_t key = k_spin_lock(&composer->post_lock);
			if (!composer->post_inprog)
				composer->disp_cb.vsync(&composer->disp_cb, os_cycle_get_32());
			k_spin_unlock(&composer->post_lock, key);
		}

		if (composer->post_cnt <= wait_post_cnt) {
			return n_frames;
		}
	} while (composer->post_inprog || (os_uptime_get_32() - uptime < timeout));

	return -ETIME;
}
