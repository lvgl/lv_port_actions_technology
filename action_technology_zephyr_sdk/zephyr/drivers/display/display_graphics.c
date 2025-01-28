/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <spicache.h>
#include <drivers/display/display_graphics.h>

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

int display_buffer_fill_color(const display_buffer_t *buffer, display_color_t color, int pad_byte)
{
	uint8_t bits_per_pixel = display_format_get_bits_per_pixel(buffer->desc.pixel_format);
	uint32_t bytes_per_line = buffer->desc.pitch;
	uint32_t bytes_to_fill = (buffer->desc.width * bits_per_pixel + 7) / 8;
	uint8_t *buf8 = (uint8_t *)(buffer->addr);
	int i;

	if (buf_is_nor((void *)buffer->addr)) {
		return -EFAULT;
	}

#ifdef CONFIG_DISPLAY_MMU
	/* cannot read back the pixels from pevious lines if MMU enabled */
	uint8_t *line_buf8 = (uint8_t *)(buffer->addr);
	int j = buffer->desc.height;

loop_next_line:
	buf8 = line_buf8;
#endif /* CONFIG_DISPLAY_MMU */

	if (buffer->desc.pixel_format == PIXEL_FORMAT_BGR_565) {
		uint8_t c8[2];
		c8[0] = ((color.g & 0x1c) << 3) | (color.b >> 3);
		c8[1] = ((color.r & 0xf8) << 0) | (color.g >> 5);

		for (i = buffer->desc.width; i > 0; i--) {
			*buf8++ = c8[0];
			*buf8++ = c8[1];
		}
	} else if (buffer->desc.pixel_format == PIXEL_FORMAT_RGB_565) {
		uint8_t c8[2];
		c8[1] = ((color.g & 0x1c) << 3) | (color.b >> 3);
		c8[0] = ((color.r & 0xf8) << 0) | (color.g >> 5);

		for (i = buffer->desc.width; i > 0; i--) {
			*buf8++ = c8[0];
			*buf8++ = c8[1];
		}
	} else if (buffer->desc.pixel_format == PIXEL_FORMAT_BGRA_5551) {
		uint8_t c8[2];
		c8[0] =  ((color.g & 0x38) << 2) | (color.b >> 3);
		c8[1] = (color.a & 0x80) | ((color.r & 0xf8) >> 1) | ((color.g & 0xc0) >> 6);

		for (i = buffer->desc.width; i > 0; i--) {
			*buf8++ = c8[0];
			*buf8++ = c8[1];
		}
	} else if (buffer->desc.pixel_format == PIXEL_FORMAT_ARGB_8888) {
		for (i = buffer->desc.width; i > 0; i--) {
			*buf8++ = color.b;
			*buf8++ = color.g;
			*buf8++ = color.r;
			*buf8++ = color.a;
		}
	} else if (buffer->desc.pixel_format == PIXEL_FORMAT_XRGB_8888) {
		for (i = buffer->desc.width; i > 0; i--) {
			*buf8++ = color.b;
			*buf8++ = color.g;
			*buf8++ = color.r;
			*buf8++ = 0xff;
		}
	} else if (buffer->desc.pixel_format == PIXEL_FORMAT_RGB_888) {
		for (i = buffer->desc.width; i > 0; i--) {
			*buf8++ = color.r;
			*buf8++ = color.g;
			*buf8++ = color.b;
		}
	} else if (buffer->desc.pixel_format == PIXEL_FORMAT_BGR_888) {
		for (i = buffer->desc.width; i > 0; i--) {
			*buf8++ = color.b;
			*buf8++ = color.g;
			*buf8++ = color.r;
		}
	} else if (buffer->desc.pixel_format == PIXEL_FORMAT_BGRA_6666) {
		uint8_t c8[3];
		c8[0] = ((color.g & 0x0c) << 4) | (color.b >> 2);
		c8[1] = ((color.r & 0x3c) << 2) | (color.g >> 4);
		c8[2] = ((color.a & 0xfc) << 0) | (color.r >> 6);

		for (i = buffer->desc.width; i > 0; i--) {
			*buf8++ = c8[0];
			*buf8++ = c8[1];
			*buf8++ = c8[2];
		}
	} else if (buffer->desc.pixel_format == PIXEL_FORMAT_RGBA_6666) {
		uint8_t c8[3];
		c8[0] = ((color.g & 0x0c) << 4) | (color.r >> 2);
		c8[1] = ((color.b & 0x3c) << 2) | (color.g >> 4);
		c8[2] = ((color.a & 0xfc) << 0) | (color.b >> 6);

		for (i = buffer->desc.width; i > 0; i--) {
			*buf8++ = c8[0];
			*buf8++ = c8[1];
			*buf8++ = c8[2];
		}
	} else if (buffer->desc.pixel_format == PIXEL_FORMAT_BGRA_5658) {
		uint8_t c8[3];
		c8[0] = ((color.g & 0x1c) << 3) | (color.b >> 3);
		c8[1] = ((color.r & 0xf8) << 0) | (color.g >> 5);
		c8[2] = color.a;

		for (i = buffer->desc.width; i > 0; i--) {
			*buf8++ = c8[0];
			*buf8++ = c8[1];
			*buf8++ = c8[2];
		}
	} else if (buffer->desc.pixel_format == PIXEL_FORMAT_A8 ||
		       buffer->desc.pixel_format == PIXEL_FORMAT_I8) {
		memset(buf8, color.a, bytes_to_fill);
	} else if (buffer->desc.pixel_format == PIXEL_FORMAT_A4 ||
		       buffer->desc.pixel_format == PIXEL_FORMAT_A4_LE ||
		       buffer->desc.pixel_format == PIXEL_FORMAT_I4) {
		uint8_t a8 = (color.a & 0xf0) | (color.a >> 4);

		memset(buf8, a8, bytes_to_fill);
	} else if (buffer->desc.pixel_format == PIXEL_FORMAT_A2 ||
		       buffer->desc.pixel_format == PIXEL_FORMAT_I2) {
		uint8_t a8 = (color.a & 0xc0) | ((color.a & 0xc0) >> 2) |
				 ((color.a & 0xc0) >> 4) | (color.a >> 6);

		memset(buf8, a8, bytes_to_fill);
	}else if (buffer->desc.pixel_format == PIXEL_FORMAT_A1 ||
		      buffer->desc.pixel_format == PIXEL_FORMAT_A1_LE ||
		      buffer->desc.pixel_format == PIXEL_FORMAT_I1) {
		uint8_t a8 = (color.a & 0x80) ? 0xff : 0x00;

		memset(buf8, a8, bytes_to_fill);
	} else if (buffer->desc.pixel_format == PIXEL_FORMAT_MONO01) { /* 0=Black 1=White */
		uint8_t c8 = ((color.r | color.g | color.b) & 0x80) ? 0xff : 0x00;

		memset(buf8, c8, bytes_to_fill);
	} else if (buffer->desc.pixel_format == PIXEL_FORMAT_MONO10) { /* 1=Black 0=White */
		uint8_t c8 = ((color.r | color.g | color.b) & 0x80) ? 0x00 : 0xff;

		memset(buf8, c8, bytes_to_fill);
	} else {
		return -EINVAL;
	}

	/* fill the padded bytes */
	if (pad_byte >= 0 && bytes_per_line != bytes_to_fill) {
		buf8 = (uint8_t *)(buffer->addr) + bytes_to_fill;
		memset(buf8, pad_byte & 0xFF, bytes_per_line - bytes_to_fill);
	}

#ifdef CONFIG_DISPLAY_MMU
	if (--j > 0) {
		line_buf8 += bytes_per_line;
		goto loop_next_line;
	}
#else
	/* point to the 2nd row */
	buf8 = (uint8_t *)(buffer->addr) + bytes_per_line;

	for (i = buffer->desc.height - 1; i > 0; i--) {
		memcpy(buf8, (uint8_t *)buffer->addr, bytes_per_line);
		buf8 += bytes_per_line;
	}
#endif /* CONFIG_DISPLAY_MMU */

	display_buffer_cpu_write_flush(buffer);
	return 0;
}

int display_buffer_fill_image(const display_buffer_t *buffer, const void *data, uint32_t data_stride_bytes, int pad_byte)
{
	uint8_t bits_per_pixel = display_format_get_bits_per_pixel(buffer->desc.pixel_format);
	uint32_t bytes_per_line = buffer->desc.pitch;
	uint32_t bytes_to_copy = (buffer->desc.width * bits_per_pixel + 7) / 8;
	uint8_t *buf8 = (uint8_t *)buffer->addr;
	const uint8_t *data8 = data;

	if (buf_is_nor((void *)buffer->addr)) {
		return -EFAULT;
	}

	if (data_stride_bytes == 0) {
		data_stride_bytes = bytes_to_copy;
	}

	for (int i = buffer->desc.height; i > 0; i--) {
		memcpy(buf8, data8, bytes_to_copy);
		if (pad_byte >= 0 && bytes_per_line != bytes_to_copy) {
			memset(buf8 + bytes_to_copy, pad_byte & 0xFF, bytes_per_line - bytes_to_copy);
		}

		buf8 += bytes_per_line;
		data8 += data_stride_bytes;
	}

	display_buffer_cpu_write_flush(buffer);
	return 0;
}

void display_buffer_cpu_write_flush(const display_buffer_t *buffer)
{
	if (buf_is_psram(buffer->addr) || buf_is_psram_un(buffer->addr)) {
		spi1_cache_ops(SPI_WRITEBUF_FLUSH, (void *)buffer->addr, buffer->desc.pitch * buffer->desc.height);
		if (buf_is_psram_cache(buffer->addr))
			spi1_cache_ops(SPI_CACHE_FLUSH, (void *)buffer->addr, buffer->desc.pitch * buffer->desc.height);

		spi1_cache_ops_wait_finshed();
	}
}

void display_buffer_dev_write_flush(const display_buffer_t *buffer)
{
	if (buf_is_psram(buffer->addr)) {
		spi1_cache_ops(SPI_CACHE_FLUSH_INVALID, (void *)buffer->addr,
				buffer->desc.pitch * buffer->desc.height);
		spi1_cache_ops_wait_finshed();
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
