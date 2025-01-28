/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>
#include <zephyr.h>
#include <device.h>
#include <board_cfg.h>
#include <drivers/display.h>
#include <drivers/display/display_graphics.h>

#define LCD_WIDTH	CONFIG_PANEL_TIMING_HACTIVE
#define LCD_HEIGHT	CONFIG_PANEL_TIMING_VACTIVE
#if CONFIG_PSRAM_SIZE > 0
#define LCD_LINE_BUFFER  LCD_HEIGHT
#else
#define LCD_LINE_BUFFER  32
#endif

static K_SEM_DEFINE(display_start_sem, 0, 1);
static K_SEM_DEFINE(display_complete_sem, 0, 1);

static __attribute__((aligned(4))) uint8_t dst_psram[LCD_WIDTH * LCD_LINE_BUFFER * 2];

static const struct device *display_dev = NULL;

static display_buffer_t dst_buf = {
	.desc = {
		.pixel_format = PIXEL_FORMAT_BGR_565,
		.pitch = LCD_WIDTH * 2,
		.width = LCD_WIDTH,
		.height = LCD_LINE_BUFFER,
	},

	.addr = (uint32_t)dst_psram,
};

static display_buffer_t img_buf = {
	.desc = {
		.pixel_format = PIXEL_FORMAT_BGR_565,
		.pitch = LCD_WIDTH * 2,
		.width = 0,
		.height = 0,
	},

	.addr = (uint32_t)NULL,
};

static void display_vsync(const struct display_callback *callback,
			  uint32_t timestamp)
{
	k_sem_give(&display_start_sem);
}

static void display_complete(const struct display_callback *callback)
{
	k_sem_give(&display_complete_sem);
}

static const struct display_callback display_callback = {
	.vsync = display_vsync,
	.complete = display_complete,
};

/** @brief The smaller value between @p a and @p b. */
#define UI_MIN(a, b) ((a) < (b) ? (a) : (b))

/** @brief The larger value between @p a and @p b. */
#define UI_MAX(a, b) ((a) > (b) ? (a) : (b))

/**
 * @struct ui_region
 * @brief Structure holding region [x1,x2] x [y1,y2]
 *
 */
typedef struct ui_region {
	int16_t x1;
	int16_t y1;
	int16_t x2;
	int16_t y2;
} ui_region_t;

static bool ui_region_intersect(ui_region_t * result, const ui_region_t * region1, const ui_region_t * region2)
{
	bool union_ok = true;

	/* Get the smaller region */
	result->x1 = UI_MAX(region1->x1, region2->x1);
	result->y1 = UI_MAX(region1->y1, region2->y1);
	result->x2 = UI_MIN(region1->x2, region2->x2);
	result->y2 = UI_MIN(region1->y2, region2->y2);

	/*If x1 or y1 greater then x2 or y2 then the areas union is empty*/
	if ((result->x1 > result->x2) || (result->y1 > result->y2)) {
		union_ok = false;
	}

	return union_ok;
}

void display_pic(void *pic_addr, int pic_width, int pic_height, uint32_t bg_color)
{
	uint8_t bits_per_pixel = display_format_get_bits_per_pixel(img_buf.desc.pixel_format);
	uint16_t bytes_per_line = img_buf.desc.pitch;
	display_color_t color = display_color_hex(bg_color);
	uint16_t draw_line = LCD_LINE_BUFFER;

	ui_region_t img_region = {
		.x1 = (LCD_WIDTH - pic_width) / 2,
		.y1 = (LCD_HEIGHT - pic_height) / 2,
		.x2 = (LCD_WIDTH + pic_width) / 2 - 1,
		.y2 = (LCD_HEIGHT + pic_height) / 2 -1,
	};

	ui_region_t draw_region = {
		.x1 = 0,
		.y1 = 0,
		.x2 = LCD_WIDTH - 1,
		.y2 = 0,
	};

	ui_region_t copy_region;

	k_sem_reset(&display_start_sem);
	k_sem_take(&display_start_sem, K_FOREVER);
	for(int i = 0; i < LCD_HEIGHT;) {
		if (draw_line + i >  LCD_HEIGHT) {
			draw_line = LCD_HEIGHT - i;
		}

		display_buffer_fill_color(&dst_buf, color, -1);
		draw_region.y1 = i;
		draw_region.y2 = i + draw_line - 1;
		if (ui_region_intersect(&copy_region, &draw_region, &img_region)) {
				img_buf.desc.width = pic_width;
				img_buf.desc.height = copy_region.y2 - copy_region.y1 + 1;
				img_buf.addr = (uint32_t)dst_psram + (copy_region.x1 - draw_region.x1) * bits_per_pixel / 8
					+ (copy_region.y1 - draw_region.y1) * bytes_per_line;
				display_buffer_fill_image(&img_buf, 
						(void *)((uint32_t)pic_addr + (copy_region.y1 - img_region.y1) * pic_width * bits_per_pixel / 8),
						pic_width * bits_per_pixel / 8, -1);
		}

		display_write(display_dev, 0, i, &dst_buf.desc, (void *)dst_buf.addr);
		k_sem_take(&display_complete_sem, K_FOREVER);

		i += draw_line;
	}

#if 0
	img_buf.desc.width = pic_width;
	img_buf.desc.height = pic_height;
	img_buf.addr = (uint32_t)dst_psram + (LCD_WIDTH - pic_width) / 2 * bits_per_pixel / 8
					+ (LCD_HEIGHT - pic_height) / 2 * bytes_per_line;
	
	display_buffer_fill_color(&dst_buf, color, -1);
	display_buffer_fill_image(&img_buf, pic_addr);

	k_sem_reset(&display_start_sem);
	k_sem_take(&display_start_sem, K_FOREVER);

	display_write(display_dev, 0, 0, &dst_buf.desc, (void *)dst_buf.addr);

	k_sem_take(&display_complete_sem, K_FOREVER);
#endif
}

void display_init(void)
{
	display_dev = device_get_binding(CONFIG_LCD_DISPLAY_DEV_NAME);
	if (!display_dev) {
		printk("%s: display " CONFIG_LCD_DISPLAY_DEV_NAME " not found\n", __func__);
		return;
	}

	display_register_callback(display_dev, &display_callback);
}

