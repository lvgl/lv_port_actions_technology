/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <drivers/display/display_graphics.h>

bool display_format_is_opaque(uint32_t pixel_format)
{
	return (pixel_format &
		(PIXEL_FORMAT_ARGB_8888 | PIXEL_FORMAT_BGRA_5658 |
			PIXEL_FORMAT_BGRA_6666 | PIXEL_FORMAT_RGBA_6666 |
			PIXEL_FORMAT_BGRA_5551 | PIXEL_FORMAT_I8 |
			PIXEL_FORMAT_I4 | PIXEL_FORMAT_I2 | PIXEL_FORMAT_I1 |
			PIXEL_FORMAT_A8 | PIXEL_FORMAT_A4 | PIXEL_FORMAT_A2 |
			PIXEL_FORMAT_A1 | PIXEL_FORMAT_A4_LE | PIXEL_FORMAT_A1_LE)) ?
		false : true;
}

uint8_t display_format_get_bits_per_pixel(uint32_t pixel_format)
{
	if (pixel_format & (PIXEL_FORMAT_BGR_565 | PIXEL_FORMAT_RGB_565 | PIXEL_FORMAT_BGRA_5551))
		return 16;

	if (pixel_format & (PIXEL_FORMAT_ARGB_8888 | PIXEL_FORMAT_XRGB_8888))
		return 32;

	if (pixel_format & (PIXEL_FORMAT_RGB_888 | PIXEL_FORMAT_BGR_888 | PIXEL_FORMAT_BGRA_5658 |
		                PIXEL_FORMAT_BGRA_6666 | PIXEL_FORMAT_RGBA_6666))
		return 24;

	if (pixel_format & (PIXEL_FORMAT_A8 | PIXEL_FORMAT_I8))
		return 8;

	if (pixel_format & (PIXEL_FORMAT_A4 | PIXEL_FORMAT_A4_LE | PIXEL_FORMAT_I4))
		return 4;

	if (pixel_format & (PIXEL_FORMAT_A2 | PIXEL_FORMAT_I2))
		return 2;

	if (pixel_format & (PIXEL_FORMAT_A1 | PIXEL_FORMAT_A1_LE | PIXEL_FORMAT_I1 |
		                PIXEL_FORMAT_MONO01 | PIXEL_FORMAT_MONO10))
		return 1;

	return 0;
}

const char * display_format_get_name(uint32_t pixel_format)
{
	switch (pixel_format) {
	case PIXEL_FORMAT_ARGB_8888:
		return "ARGB_8888";
	case PIXEL_FORMAT_XRGB_8888:
		return "XRGB_8888";
	case PIXEL_FORMAT_BGR_565:
		return "RGB_565";
	case PIXEL_FORMAT_RGB_565:
		return "RGB_565_BE";
	case PIXEL_FORMAT_BGRA_5551:
		return "ARGB_1555";
	case PIXEL_FORMAT_RGB_888:
		return "BGR_888";
	case PIXEL_FORMAT_BGR_888:
		return "RGB_888";
	case PIXEL_FORMAT_BGRA_5658:
		return "ARGB_8565";
	case PIXEL_FORMAT_BGRA_6666:
		return "ARGB_6666";
	case PIXEL_FORMAT_RGBA_6666:
		return "ABGR_6666";
	case PIXEL_FORMAT_A8:
		return "A8";
	case PIXEL_FORMAT_A4:
		return "A4";
	case PIXEL_FORMAT_A2:
		return "A2";
	case PIXEL_FORMAT_A1:
		return "A1";
	case PIXEL_FORMAT_A4_LE:
		return "A4_LE";
	case PIXEL_FORMAT_A1_LE:
		return "A1_LE";
	case PIXEL_FORMAT_I8:
		return "I8";
	case PIXEL_FORMAT_I4:
		return "I4";
	case PIXEL_FORMAT_I2:
		return "I2";
	case PIXEL_FORMAT_I1:
		return "I1";
	case PIXEL_FORMAT_MONO01: /* 0=Black 1=White */
		return "MONO_01";
	case PIXEL_FORMAT_MONO10: /* 1=Black 0=White */
		return "MONO_10";
	default:
		return "unknown";
	}
}

void * display_buffer_goto_xy(const display_buffer_t *buffer, uint32_t x, uint32_t y)
{
	uint8_t bpp = display_format_get_bits_per_pixel(buffer->desc.pixel_format);
	uint32_t pitch = (buffer->desc.pitch > 0) ?
			buffer->desc.pitch : (buffer->desc.width * bpp / 8);

	return (uint8_t *)buffer->addr + y * pitch + x * bpp / 8;
}

void display_rect_intersect(display_rect_t *dest, const display_rect_t *src)
{
	int16_t x1 = MAX(dest->x, src->x);
	int16_t y1 = MAX(dest->y, src->y);
	int16_t x2 = MIN(dest->x + dest->w, src->x + src->w);
	int16_t y2 = MIN(dest->y + dest->h, src->y + src->h);

	dest->x = x1;
	dest->y = y1;
	dest->w = x2 - x1;
	dest->h = y2 - y1;
}

void display_rect_merge(display_rect_t *dest, const display_rect_t *src)
{
	int16_t x1 = MIN(dest->x, src->x);
	int16_t y1 = MIN(dest->y, src->y);
	int16_t x2 = MAX(dest->x + dest->w, src->x + src->w);
	int16_t y2 = MAX(dest->y + dest->h, src->y + src->h);

	dest->x = x1;
	dest->y = y1;
	dest->w = x2 - x1;
	dest->h = y2 - y1;
}
