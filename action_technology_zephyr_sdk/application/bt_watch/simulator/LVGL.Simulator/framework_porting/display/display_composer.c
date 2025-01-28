/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <lvgl/lvgl.h>
#include <display/display_composer.h>
#include <native_window.h>

typedef struct display_composer {
	/* user display callback */
	const struct display_callback *user_cb;
} display_composer_t;

/* static prototypes */
static void _composer_display_vsync_handler(void *);

static display_composer_t display_composer;

#if CONFIG_PANEL_ROUND_SHAPE
static void _compute_round_shage(int16_t y, int16_t* x1, int16_t* x2);

static int16_t row_range[CONFIG_PANEL_VER_RES][2];
#endif /* CONFIG_PANEL_ROUND_SHAPE */

static inline display_composer_t *_composer_get(void)
{
	return &display_composer;
}

int display_composer_init(void)
{
	display_composer_t *composer = _composer_get();

	memset(composer, 0, sizeof(*composer));

    native_window_register_callback(NWIN_CB_VSYNC, _composer_display_vsync_handler, composer);

#if CONFIG_PANEL_ROUND_SHAPE
	for (int16_t y = 0; y < CONFIG_PANEL_VER_RES; y++) {
		_compute_round_shage(y, &row_range[y][0], &row_range[y][1]);
	}
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
}

uint8_t display_composer_get_refresh_rate(void)
{
	return 1000 / LV_DEF_REFR_PERIOD;
}

int display_composer_get_geometry(uint16_t *width, uint16_t *height,
			uint32_t *pixel_format, uint8_t *round_screen)
{
	display_composer_t *composer = _composer_get();

	if (width) {
		*width = CONFIG_PANEL_HOR_RES;
	}

	if (height) {
		*height = CONFIG_PANEL_VER_RES;
	}

	if (pixel_format) {
		*pixel_format = (LV_COLOR_DEPTH == 16)?
            HAL_PIXEL_FORMAT_RGB_565 : HAL_PIXEL_FORMAT_XRGB_8888;
	}

	if (round_screen) {
#if CONFIG_PANEL_ROUND_SHAPE
		*round_screen = 1;
#else
		*round_screen = 0;
#endif
	}

	return 0;
}

uint16_t display_composer_get_orientation(void)
{
	return 0;
}

uint8_t display_composer_get_num_layers(void)
{
	return NUM_POST_LAYERS;
}

int display_composer_set_brightness(uint8_t brightness)
{
	return 0;
}

int display_composer_set_contrast(uint8_t contrast)
{
	return 0;
}

void display_composer_round(ui_region_t *region)
{
}

static void _composer_display_vsync_handler(void *user_data)
{
	display_composer_t *composer = _composer_get();

    if (composer->user_cb && composer->user_cb->vsync) {
        composer->user_cb->vsync(composer->user_cb, 0);
    }
}


int display_composer_post(const ui_layer_t *layers, int num_layers, uint32_t post_flags)
{
	display_composer_t *composer = _composer_get();
	uint8_t *fb = native_window_get_framebuffer();
	ui_region_t region = { CONFIG_PANEL_HOR_RES, CONFIG_PANEL_VER_RES, 0, 0 };
	int16_t i, j;

	if (num_layers <= 0 || num_layers > NUM_POST_LAYERS) {
		SYS_LOG_ERR("invalid layer num %d", num_layers);
		goto cleanup_cb;
	}

	for (i = 0; i < num_layers; i++) {
		uint8_t *fb_tmp = fb + (layers[i].frame.y1 * CONFIG_PANEL_HOR_RES + layers[i].frame.x1) * LV_COLOR_DEPTH / 8;
		uint32_t copy_bytes = ui_region_get_width(&layers[i].frame) * LV_COLOR_DEPTH / 8;

		ui_region_merge(&region, &region, &layers[i].frame);

		if (layers[i].buffer == NULL) {
			for (j = layers[i].frame.y1; j <= layers[i].frame.y2; j++) {
				memset(fb_tmp, 0, copy_bytes);
				fb_tmp += CONFIG_PANEL_HOR_RES * LV_COLOR_DEPTH / 8;
			}
			continue;
		}

		uint8_t* ly_fb = (uint8_t*)graphic_buffer_get_bufptr(layers[i].buffer, layers[i].crop.x1, layers[i].crop.y1);

		for (j = layers[i].frame.y1; j <= layers[i].frame.y2; j++) {
			int16_t x1 = layers[i].frame.x1;
			int16_t x2 = layers[i].frame.x2;
			uint8_t *dst = fb_tmp, *src = ly_fb;

#if CONFIG_PANEL_ROUND_SHAPE
			x1 = UI_MAX(x1, row_range[j][0]);
			x2 = UI_MIN(x2, row_range[j][1]);
#endif

			if (x1 <= x2) {
				if (x1 != layers[i].frame.x1) {
					dst += (x1 - layers[i].frame.x1) * LV_COLOR_DEPTH / 8;
					src += (x1 - layers[i].frame.x1) * LV_COLOR_DEPTH / 8;
				}

				memcpy(dst, src, (x2 - x1 + 1) * LV_COLOR_DEPTH / 8);
			}

			ly_fb += graphic_buffer_get_stride(layers[i].buffer) * LV_COLOR_DEPTH / 8;
			fb_tmp += CONFIG_PANEL_HOR_RES * LV_COLOR_DEPTH / 8;
		}
	}

	native_window_flush_framebuffer(region.x1, region.y1, ui_region_get_width(&region), ui_region_get_height(&region));

cleanup_cb:
	for (i = 0; i < num_layers; i++) {
		if (layers[i].cleanup_cb)
			layers[i].cleanup_cb(layers[i].cleanup_data);
	}
	return 0;
}

int display_composer_flush(unsigned int timeout)
{
    return 0;
}

#if CONFIG_PANEL_ROUND_SHAPE
static void _compute_round_shage(int16_t y, int16_t* x1, int16_t* x2)
{
	const int16_t cx = CONFIG_PANEL_HOR_RES - 1; // .1 fixed point
	const int16_t cy = CONFIG_PANEL_VER_RES - 1; // .1 fixed point
	const int16_t radius = UI_MIN(cx, cy);
	lv_sqrt_res_t res;

	y *= 2; // .1 fixed point

	if (y - cy < -radius || y - cy > radius) {
		*x1 = 0;
		*x2 = 0;
		return;
	}

	lv_sqrt((int)radius * radius - (int)(y - cy) * (y - cy), &res, 0x8000);
	*x1 = UI_MAX((cx - res.i) / 2, 0);
	*x2 = UI_MIN((cx + res.i) / 2, CONFIG_PANEL_HOR_RES - 1);
}
#endif /* CONFIG_PANEL_ROUND_SHAPE */
