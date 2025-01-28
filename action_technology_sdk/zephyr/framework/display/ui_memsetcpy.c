/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*********************
 *      INCLUDES
 *********************/

#include <assert.h>
#include <display/ui_memsetcpy.h>
#include <memory/mem_cache.h>
#ifdef CONFIG_DMA2D_HAL
#  include <dma2d_hal.h>
#endif

/**********************
 *  STATIC VARIABLES
 **********************/

#ifdef CONFIG_DMA2D_HAL
static bool dma2d_inited = false;
static hal_dma2d_handle_t hdma2d __in_section_unique(ram.noinit.surface);
#endif

/**********************
 *   STATIC FUNCTIONS
 **********************/

#ifdef CONFIG_DMA2D_HAL
static int dma_transfer_init(void)
{
	int ret = 0;
	if (dma2d_inited)
		return 0;

	if ((ret = hal_dma2d_init(&hdma2d, HAL_DMA2D_M2M)) == 0) {
		/* initialize dma2d constant fields */
		hdma2d.output_cfg.mode = HAL_DMA2D_M2M;
		hdma2d.layer_cfg[1].alpha_mode = HAL_DMA2D_NO_MODIF_ALPHA;
		dma2d_inited = true;
	}
	return 0;
}
#endif /* CONFIG_DMA2D_HAL */

static void ui_memset_sw(void * buf, uint8_t c, size_t size)
{
	buf = mem_addr_to_uncache(buf);

	memset(buf, c, size);
	mem_writebuf_clean_all();
}

static void ui_memset32_sw(void * buf, uint32_t c32, size_t n32)
{
	uint32_t *buf32 = mem_addr_to_uncache(buf);

	assert(((uintptr_t)buf32 & 0x3) == 0);

	for (; n32 >= 8; n32 -= 8) {
		*buf32++ = c32;
		*buf32++ = c32;
		*buf32++ = c32;
		*buf32++ = c32;
		*buf32++ = c32;
		*buf32++ = c32;
		*buf32++ = c32;
		*buf32++ = c32;
	}

	for (; n32 >= 4; n32 -= 4) {
		*buf32++ = c32;
		*buf32++ = c32;
		*buf32++ = c32;
		*buf32++ = c32;
	}

	for (; n32 > 0; n32 -= 1) {
		*buf32++ = c32;
	}

	mem_writebuf_clean_all();
}

static void ui_memset16_sw(void * buf, uint16_t c16, size_t n16)
{
	uint32_t *buf16 = mem_addr_to_uncache(buf);

	assert(((uintptr_t)buf16 & 0x1) == 0);

	if (n16 == 0)
		return;

	/* unaligned head */
	if ((uintptr_t)buf16 & 0x3) {
		*buf16++ = c16;
		n16--;
	}

	if (n16 >= 2) {
		uint32_t c32 = ((uint32_t)c16 << 16) | c16;
		ui_memset32_sw(buf16, c32, n16 >> 1);
	}

	/* unaligned tail */
	if (n16 & 0x1) {
		buf16 += n16 & ~0x1;
		*buf16 = c16;
	}

	mem_writebuf_clean_all();
}

void ui_memset24_sw(void * buf, const uint8_t c24[3], size_t n24)
{
	uint8_t *buf8 = mem_addr_to_uncache(buf);
	size_t ofs = (4 - ((uintptr_t)buf8 & 0x3)) & 0x3;
	size_t bytes = n24 * 3;
	union {
		uint32_t v32[3];
		uint8_t v8[12];
	} pattern;
	uint32_t i;

	if (bytes == 0)
		return;

	/* unaligned head */
	for (i = 0; i < ofs; i++) {
		buf8[i] = c24[i];
	}

	buf8 += ofs;
	bytes -= ofs;
	if (bytes == 0)
		return;

	/* create pattern */
	for (i = 0; i < 12; i++, ofs++) {
		if (ofs >= 3) ofs = 0;
		pattern.v8[i] = c24[ofs];
	}

	if (bytes >= 12) {
		uint32_t *buf32 = (uint32_t *)buf8;
		for (; bytes >= 12; bytes -= 12) {
			*buf32++ = pattern.v32[0];
			*buf32++ = pattern.v32[1];
			*buf32++ = pattern.v32[2];
		}

		buf8 = (uint8_t *)buf32;
	}

	/* unaligned tail */
	for (i = 0; bytes > 0; bytes--, i++) {
		*buf8++ = pattern.v8[i];
	}

	mem_writebuf_clean_all();
}

static void ui_memset2d_sw(void * buf, uint16_t stride, uint8_t c, uint16_t len, uint16_t lines)
{
	uint8_t *buf8 = mem_addr_to_uncache(buf);

	if (len == stride) {
		memset(buf8, c, (uint32_t)len * lines);
	} else {
		for (; lines > 0; lines--) {
			memset(buf8, c, len);
			buf8 += stride;
		}
	}

	mem_writebuf_clean_all();
}

static void ui_memset2d_16_sw(void * buf, uint16_t stride, uint16_t c16, uint16_t len16, uint16_t lines)
{
	if (len16 * 2 == stride) {
		ui_memset16_sw(buf, c16, (uint32_t)len16 * lines);
	} else {
		uint8_t *buf16 = buf;

		for (; lines > 0; lines--) {
			ui_memset16_sw(buf16, c16, len16);
			buf16 += stride;
		}
	}
}

static void ui_memset2d_24_sw(void * buf, uint16_t stride, const uint8_t c24[3], uint16_t len24, uint16_t lines)
{
	if (len24 * 3 == stride) {
		ui_memset24_sw(buf, c24, (uint32_t)len24 * lines);
	} else {
		uint8_t *buf8 = buf;

		for (; lines > 0; lines--) {
			ui_memset24_sw(buf8, c24, len24);
			buf8 += stride;
		}
	}
}

static void ui_memset2d_32_sw(void * buf, uint16_t stride, uint32_t c32, uint16_t len32, uint16_t lines)
{
	if (len32 * 4 == stride) {
		ui_memset16_sw(buf, c32, (uint32_t)len32 * lines);
	} else {
		uint8_t *buf32 = buf;

		for (; lines > 0; lines--) {
			ui_memset32_sw(buf32, c32, len32);
			buf32 += stride;
		}
	}
}

static void ui_memcpy_sw(void * dest, const void * src, size_t size)
{
	dest = mem_addr_to_uncache(dest);

	memcpy(dest, src, size);
	mem_writebuf_clean_all();
}

static void ui_memcpy2d_sw(void * dest, uint16_t dest_stride, const void * src,
			uint16_t src_stride, uint16_t len, uint16_t lines)
{
	uint8_t *dest8 = mem_addr_to_uncache(dest);
	const uint8_t *src8 = src;

	if (len == dest_stride && len == src_stride) {
		memcpy(dest8, src8, (uint32_t)len * lines);
	} else {
		for (; lines > 0; lines--) {
			memcpy(dest8, src8, len);
			dest8 += dest_stride;
			src8 += src_stride;
		}
	}

	mem_writebuf_clean_all();
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

#ifdef CONFIG_DMA2D_HAL

static int ui_memset_hw(void * buf, uint8_t c, size_t len, uint16_t lines, size_t stride)
{
	if (dma_transfer_init() == 0) {
		hdma2d.output_cfg.mode = HAL_DMA2D_R2M;
		hdma2d.output_cfg.color_format = HAL_PIXEL_FORMAT_A8;
		hdma2d.output_cfg.output_pitch = stride;
		hal_dma2d_config_output(&hdma2d);

		return hal_dma2d_start(&hdma2d, (uint32_t)c << 24, (uint32_t)buf, len, lines);
	}

	return -ENOTSUP;
}

static int ui_memset16_hw(void * buf, uint16_t c16, size_t len16, uint16_t lines, size_t stride)
{
	if (dma_transfer_init() == 0) {
		hdma2d.output_cfg.mode = HAL_DMA2D_R2M;
		hdma2d.output_cfg.color_format = HAL_PIXEL_FORMAT_RGB_565;
		hdma2d.output_cfg.output_pitch = stride;
		hal_dma2d_config_output(&hdma2d);

		uint32_t c32 = ((c16 & 0xf800) << 8) | ((c16 & 0x07e0) << 5) | ((c16 & 0x001f) << 3);
		return hal_dma2d_start(&hdma2d, c32, (uint32_t)buf, len16, lines);
	}

	return -ENOTSUP;
}

static int ui_memset24_hw(void * buf, const uint8_t c24[3], size_t len24, uint16_t lines, size_t stride)
{
	if (dma_transfer_init() == 0) {
		hdma2d.output_cfg.mode = HAL_DMA2D_R2M;
		hdma2d.output_cfg.color_format = HAL_PIXEL_FORMAT_BGR_888;
		hdma2d.output_cfg.output_pitch = stride;
		hal_dma2d_config_output(&hdma2d);

		uint32_t c32 = ((uint32_t)c24[0] << 16) | ((uint32_t)c24[1] << 8) | c24[2];
		return hal_dma2d_start(&hdma2d, c32, (uint32_t)buf, len24, lines);
	}

	return -ENOTSUP;
}

static int ui_memset32_hw(void * buf, uint32_t c32, size_t len32, uint16_t lines, size_t stride)
{
	if (dma_transfer_init() == 0) {
		hdma2d.output_cfg.mode = HAL_DMA2D_R2M;
		hdma2d.output_cfg.color_format = HAL_PIXEL_FORMAT_ARGB_8888;
		hdma2d.output_cfg.output_pitch = stride;
		hal_dma2d_config_output(&hdma2d);

		return hal_dma2d_start(&hdma2d, c32, (uint32_t)buf, len32, lines);
	}

	return -ENOTSUP;
}

void ui_memset(void * buf, uint8_t c, size_t n)
{
	if (ui_memset_hw(buf, c, n, 1, n) < 0) {
		ui_memset_sw(buf, c, n);
	}
}

void ui_memset16(void * buf, uint16_t c16, size_t n16)
{
	if (ui_memset16_hw(buf, c16, n16, 1, n16 * 2) < 0) {
		ui_memset16_sw(buf, c16, n16);
	}
}

void ui_memset24(void * buf, const uint8_t c24[3], size_t n24)
{
	if (ui_memset24_hw(buf, c24, n24, 1, n24 * 3) < 0) {
		ui_memset24_sw(buf, c24, n24);
	}
}

void ui_memset32(void * buf, uint32_t c32, size_t n32)
{
	if (ui_memset32_hw(buf, c32, n32, 1, n32 * 4) < 0) {
		ui_memset32_sw(buf, c32, n32);
	}
}

void ui_memset2d(void * buf, uint16_t stride, uint8_t c, uint16_t len, uint16_t lines)
{
	if (ui_memset_hw(buf, c, len, lines, stride) < 0) {
		ui_memset2d_sw(buf, stride, c, len, lines);
	}
}

void ui_memset2d_16(void * buf, uint16_t stride, uint16_t c16, uint16_t len16, uint16_t lines)
{
	if (ui_memset16_hw(buf, c16, len16, lines, stride) < 0) {
		ui_memset2d_16_sw(buf, stride, c16, len16, lines);
	}
}

void ui_memset2d_24(void * buf, uint16_t stride, const uint8_t c24[3], uint16_t len24, uint16_t lines)
{
	if (ui_memset24_hw(buf, c24, len24, lines, stride) < 0) {
		ui_memset2d_24_sw(buf, stride, c24, len24, lines);
	}
}

void ui_memset2d_32(void * buf, uint16_t stride, uint32_t c32, uint16_t len32, uint16_t lines)
{
	if (ui_memset32_hw(buf, c32, len32, lines, stride) < 0) {
		ui_memset2d_32_sw(buf, stride, c32, len32, lines);
	}
}

void ui_memcpy(void * dest, const void * src, size_t n)
{
	int res = -ENOTSUP;

	if (dma_transfer_init() == 0) {
		hdma2d.output_cfg.mode = HAL_DMA2D_M2M;
		hdma2d.output_cfg.output_pitch = n;
		hdma2d.output_cfg.color_format = HAL_PIXEL_FORMAT_A8;
		hal_dma2d_config_output(&hdma2d);

		hdma2d.layer_cfg[1].color_format = HAL_PIXEL_FORMAT_A8;
		hdma2d.layer_cfg[1].input_pitch = n;
		hdma2d.layer_cfg[1].input_width = n;
		hdma2d.layer_cfg[1].input_height = 1;
		hal_dma2d_config_layer(&hdma2d, HAL_DMA2D_FOREGROUND_LAYER);

		res = hal_dma2d_start(&hdma2d, (uint32_t)src, (uint32_t)dest, n, 1);
	}

	if (res < 0) {
		ui_memcpy_sw(dest, src, n);
	}
}

void ui_memcpy2d(void * dest, uint16_t dest_stride, const void * src,
				uint16_t src_stride, uint16_t len, uint16_t lines)
{
	int res = -ENOTSUP;

	if (dma_transfer_init() == 0) {
		hdma2d.output_cfg.mode = HAL_DMA2D_M2M;
		hdma2d.output_cfg.output_pitch = dest_stride;
		hdma2d.output_cfg.color_format = HAL_PIXEL_FORMAT_A8;
		hal_dma2d_config_output(&hdma2d);

		hdma2d.layer_cfg[1].color_format = HAL_PIXEL_FORMAT_A8;
		hdma2d.layer_cfg[1].input_pitch = src_stride;
		hdma2d.layer_cfg[1].input_width = len;
		hdma2d.layer_cfg[1].input_height = lines;
		hal_dma2d_config_layer(&hdma2d, HAL_DMA2D_FOREGROUND_LAYER);

		res = hal_dma2d_start(&hdma2d, (uint32_t)src, (uint32_t)dest, len, lines);
	}

	if (res < 0) {
		ui_memcpy2d_sw(dest, dest_stride, src, src_stride, len, lines);
	}
}

int ui_memsetcpy_wait_finish(int timeout_ms)
{
	if (dma2d_inited) {
		return hal_dma2d_poll_transfer(&hdma2d, timeout_ms);
	}

	return 0;
}

#else /* CONFIG_DMA2D_HAL */

void ui_memset(void * buf, uint8_t c, size_t n)
{
	ui_memset_sw(buf, c, n);
}

void ui_memset16(void * buf, uint16_t c16, size_t n16)
{
	ui_memset16_sw(buf, c16, n16);
}

void ui_memset24(void * buf, const uint8_t c24[3], size_t n24)
{
	ui_memset24_sw(buf, c24, n24);
}

void ui_memset32(void * buf, uint32_t c32, size_t n32)
{
	ui_memset32_sw(buf, c32, n32);
}

void ui_memset2d(void * buf, uint16_t stride, uint8_t c, uint16_t len, uint16_t lines)
{
	ui_memset2d_sw(buf, stride, c, len, lines);
}

void ui_memset2d_16(void * buf, uint16_t stride, uint16_t c16, uint16_t len16, uint16_t lines)
{
	ui_memset2d_16_sw(buf, stride, c16, len16, lines);
}

void ui_memset2d_24(void * buf, uint16_t stride, const uint8_t c24[3], uint16_t len24, uint16_t lines)
{
	ui_memset2d_24_sw(buf, stride, c24, len24, lines);
}

void ui_memset2d_32(void * buf, uint16_t stride, uint32_t c32, uint16_t len32, uint16_t lines)
{
	ui_memset2d_32_sw(buf, stride, c32, len32, lines);
}

void ui_memcpy(void * dest, const void * src, size_t n)
{
	ui_memcpy_sw(dest, src, n);
}

void ui_memcpy2d(void * dest, uint16_t dest_stride, const void * src,
				uint16_t src_stride, uint16_t len, uint16_t lines)
{
	ui_memcpy2d_sw(dest, dest_stride, src, src_stride, len, lines);
}

int ui_memsetcpy_wait_finish(int timeout_ms)
{
	return 0;
}

#endif /* CONFIG_DMA2D_HAL */
