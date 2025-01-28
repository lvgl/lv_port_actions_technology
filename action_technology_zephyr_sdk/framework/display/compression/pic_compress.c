/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <os_common_api.h>
#include <ui_region.h>
#include <compress_api.h>
#include "tile_cache.h"
#include "lz4/lz4.h"
#include "lz4/lz4hc.h"
#include "rle/rle.h"

#ifndef CONFIG_SIMULATOR
#include "brom_interface.h"
#endif

#include <display/display_hal.h>
#ifdef CONFIG_DMA2D_HAL
#include <dma2d_hal.h>
#endif
#include <memory/mem_cache.h>

#if defined(__GNUC__) && !defined(__clang__)
#  define LZ4_FORCE_O3  __attribute__((optimize("O3")))
#else
#  define LZ4_FORCE_O3
#endif

#ifdef CONFIG_PIC_COMPRESS
__aligned(4) __in_section_unique(decompress.bss.cache)
static uint8_t tile_temp[TILE_MAX_H * TILE_MAX_W * PIC_BYTES_PER_PIXEL];
#endif

#ifdef CONFIG_DMA2D_HAL
static bool dma2d_inited = false;
static hal_dma2d_handle_t dma2d;
#endif

__ramfunc int hardware_copy(char *dest, int16_t d_stride, const char *src,
		int16_t s_width, int16_t s_height, int16_t s_stride, uint8_t bytes_per_pixel)
{
	int copy_line_len = bytes_per_pixel * s_width;
	int ret = -1;

#ifdef CONFIG_DMA2D_HAL
	static const uint32_t hal_formats[] = {
		0, HAL_PIXEL_FORMAT_A8, HAL_PIXEL_FORMAT_RGB_565,
		HAL_PIXEL_FORMAT_BGR_888, HAL_PIXEL_FORMAT_ARGB_8888,
	};

	if (!dma2d_inited) {
		if (hal_dma2d_init(&dma2d, HAL_DMA2D_FULL_MODES)) {
			printk("hal_dma2d_init failed\n");
			goto out_sw_copy;
		}

		dma2d_inited = true;
		dma2d.output_cfg.mode = HAL_DMA2D_M2M;
		dma2d.layer_cfg[1].alpha_mode = HAL_DMA2D_NO_MODIF_ALPHA;
	}

#ifndef CONFIG_NO_PSRAM
	mem_dcache_clean(src, s_stride * s_height);
	mem_dcache_sync();
#endif

	dma2d.output_cfg.output_pitch = d_stride;
	dma2d.output_cfg.color_format = hal_formats[bytes_per_pixel];
	hal_dma2d_config_output(&dma2d);

	dma2d.layer_cfg[1].input_pitch = s_stride;
	dma2d.layer_cfg[1].input_width = s_width;
	dma2d.layer_cfg[1].input_height = s_height;
	dma2d.layer_cfg[1].color_format = hal_formats[bytes_per_pixel];
	hal_dma2d_config_layer(&dma2d, 1);

	ret = hal_dma2d_start(&dma2d, (uint32_t)src, (uint32_t)dest, s_width, s_height);

out_sw_copy:
#endif /* CONFIG_DMA2D_HAL */

	if (ret < 0) {
		if (copy_line_len == s_stride && s_stride == d_stride) {
			memcpy(dest, src, copy_line_len * s_height);
		} else {
			for (int j = s_height; j > 0; j--) {
				memcpy(dest, src, copy_line_len);
				dest += d_stride;
				src += s_stride;
			}
		}
	}

	return copy_line_len * s_height;
}

static inline void hardware_wait_finish(void)
{
#ifdef CONFIG_DMA2D_HAL
	if (dma2d_inited)
		hal_dma2d_poll_transfer(&dma2d, -1);
#endif
}
#ifdef CONFIG_PIC_COMPRESS

int pic_compress(const char* picSrc, char* picDst, int srcWidth, int srcHight,
		int tileWidth, int tileHight, int maxOutputSize, uint8_t format, uint8_t compress_format)
{
	const char *tile_start;
	int tile_width = 0, tile_height = 0, tile_cnt = 0;
	int total_size = 0;
	int data_size = 0;
	compress_pic_head_t *pic_head = (compress_pic_head_t *)picDst;
	uint8_t bytes_per_pixel = 0;
	int tile_x_num = 0, tile_y_num = 0;
	tile_head_t *tile_head_info = NULL;
	char* base_picDst = picDst;
	int src_length = 0;

	switch (format) {
	case COMPRESSED_PIC_CF_RGB_565:
	case COMPRESSED_PIC_CF_ARGB_1555:
		bytes_per_pixel = 2;
		break;
	case COMPRESSED_PIC_CF_ARGB_8565:
	case COMPRESSED_PIC_CF_ARGB_6666:
		bytes_per_pixel = 3;
		break;
	case COMPRESSED_PIC_CF_ARGB_8888:
		bytes_per_pixel = 4;
		break;
	case COMPRESSED_PIC_CF_A8:
		bytes_per_pixel = 1;
		break;
	default:
		return -1;
	}
	src_length = srcWidth * srcHight * bytes_per_pixel;
	// pic_head
	if (compress_format == COMPRESSED_PIC_FORMAT_LZ4) {
		pic_head->magic = LZ4_PIC_MAGIC;
	} else if (compress_format == COMPRESSED_PIC_FORMAT_RLE) {
		pic_head->magic = RLE_PIC_MAGIC;
	} else if (compress_format == COMPRESSED_PIC_FORMAT_RAW) {
		pic_head->magic = RAW_PIC_MAGIC;
	}

	pic_head->width = srcWidth;
	pic_head->height = srcHight;
	pic_head->tile_width = tileWidth;
	pic_head->tile_height = tileHight;
	pic_head->format = format;
	pic_head->bytes_per_pixel = bytes_per_pixel;

	picDst += sizeof(compress_pic_head_t);
	total_size += sizeof(compress_pic_head_t);

	//tile_info
	tile_x_num = (srcWidth + tileWidth - 1) / tileWidth;
	tile_y_num = (srcHight + tileHight - 1) / tileHight;
	tile_head_info = (tile_head_t *)picDst;
	picDst += sizeof(tile_head_t) * tile_x_num * tile_y_num;
	total_size += sizeof(tile_head_t) * tile_x_num * tile_y_num;
	// comprassed_data
	for(uint16_t j = 0; j < tile_y_num;j ++) {
		for (uint16_t i = 0; i < tile_x_num; i++) {
			tile_head_t* new_tile = &tile_head_info[j * tile_x_num + i];
			if((i + 1) * tileWidth > srcWidth) {
				tile_width = srcWidth - i * tileWidth;
			} else {
				tile_width = tileWidth;
			}

			if((j + 1) * tileHight > srcHight) {
				tile_height = srcHight - j * tileHight;
			} else {
				tile_height = tileHight;
			}

			new_tile->tile_addr = picDst - base_picDst;
			tile_start = picSrc + i * tileWidth * bytes_per_pixel
							 + j * tileHight * srcWidth * bytes_per_pixel;

			for(int k = 0; k < tile_height; k++) {
				memcpy(&tile_temp[k * bytes_per_pixel * tile_width],
					tile_start + k * bytes_per_pixel * srcWidth,
					bytes_per_pixel * tile_width);
			}

			if (compress_format == COMPRESSED_PIC_FORMAT_LZ4) {
				new_tile->tile_size = LZ4_compress_HC(tile_temp,
										new_tile->tile_addr + base_picDst,
										tile_height * tile_width * bytes_per_pixel,
										maxOutputSize, 12);
			} else if (compress_format == COMPRESSED_PIC_FORMAT_RLE) {
				new_tile->tile_size = rle_compress(tile_temp,
										new_tile->tile_addr + base_picDst,
										tile_height * tile_width,
										maxOutputSize, bytes_per_pixel);
			} else if (compress_format == COMPRESSED_PIC_FORMAT_RAW) {
				new_tile->tile_size = tile_height * tile_width * bytes_per_pixel;
				memcpy(new_tile->tile_addr + base_picDst,tile_temp, new_tile->tile_size);
			}

			if (new_tile->tile_size <= 0) {
				printf("Failed to compress the data\n");
				return -1;
			}
			tile_cnt++;
			maxOutputSize -= new_tile->tile_size;
			picDst += new_tile->tile_size;
			total_size += new_tile->tile_size;
			data_size += new_tile->tile_size;
		}
	}
	pic_head->tile_num = tile_cnt;
	// compressed size maxed source size
	if (data_size > src_length) {
		pic_head->tile_width = srcWidth;
		pic_head->tile_height = srcHight;
		pic_head->tile_num = 1;
		pic_head->magic = RAW_PIC_MAGIC;
		total_size = bytes_per_pixel * srcWidth * srcHight + sizeof(compress_pic_head_t);
		memcpy((void*)(base_picDst + sizeof(compress_pic_head_t)), (void *)picSrc, src_length);
	}

	return total_size;
}

int pic_compress_size(const char* picSource)
{
	compress_pic_head_t* pic_head = (compress_pic_head_t*)picSource;

	return pic_head->width * pic_head->height * pic_head->bytes_per_pixel;
}

int pic_compress_format(const char* picSource)
{
	compress_pic_head_t* pic_head = (compress_pic_head_t*)picSource;

	return pic_head->format;
}
#endif

LZ4_FORCE_O3
__ramfunc int pic_decompress(const char* picSource, char* picDst, int compressedSize,
		int maxDecompressedSize, int out_stride, int x, int y, int w, int h)
{
	compress_pic_head_t* pic_head = (compress_pic_head_t*)picSource;

	os_strace_u32x6(SYS_TRACE_ID_PIC_DECOMPRESS, pic_head->magic, pic_head->format, x, y, w, h);

	tile_head_t* tile_head_info = (tile_head_t*)(picSource + sizeof(compress_pic_head_t));
	int tile_x_num = (pic_head->width + pic_head->tile_width - 1) / pic_head->tile_width;
	int x_start_tile = x / pic_head->tile_width;
	int x_end_tile =   (x + w - 1) / pic_head->tile_width;
	int y_start_tile = y / pic_head->tile_height;
	int y_end_tile =   (y + h - 1) / pic_head->tile_height;
	int out_size = 0;
	//bool dec = false;
	//int get_cache_time = 0;
	//int copy_time = 0;
	//int decompress_time = 0;
	if (!out_stride) {
		out_stride = w * pic_head->bytes_per_pixel;
	}

	ui_region_t copy_region;
	ui_region_t crop_region = {
		.x1 = x,
		.y1 = y,
		.x2 = x + w - 1,
		.y2 = y + h - 1,
	};

	if (pic_head->magic == RAW_PIC_MAGIC) {
		char* temp_picSource = (char *)picSource + sizeof(compress_pic_head_t)
					+ y * pic_head->bytes_per_pixel * pic_head->width
					+ x * pic_head->bytes_per_pixel;

		out_size = hardware_copy(picDst, out_stride, temp_picSource, w,
				h, pic_head->bytes_per_pixel * pic_head->width, pic_head->bytes_per_pixel);
		hardware_wait_finish();

		os_strace_end_call_u32(SYS_TRACE_ID_PIC_DECOMPRESS, 1);
		return out_size;
	}

	if (y_start_tile < 0)
		y_start_tile = 0;
	if (x_start_tile < 0)
		x_start_tile = 0;

	for (int j = y_start_tile; j <= y_end_tile; j++) {
		for (int i = x_start_tile; i <= x_end_tile; i++) {
			int tile_index = i + j * tile_x_num;
			//uint32_t get_cache_start = k_cycle_get_32();
			tile_cache_item_t * cache_item = tile_cache_get(picSource, tile_index);
			//get_cache_time +=  (k_cycle_get_32() - get_cache_start);
			//if (!tile_cache_is_valid(cache_item)) {
				//dec = true;

			if (pic_head->magic == LZ4_PIC_MAGIC) {
#ifdef CONFIG_SOC_SERIES_LEOPARD
				p_brom_misc_api->p_decompress(picSource + tile_head_info[tile_index].tile_addr,
					cache_item->tile_data,
					tile_head_info[tile_index].tile_size,
					sizeof(cache_item->tile_data));
#else

				LZ4_decompress_safe(picSource + tile_head_info[tile_index].tile_addr,
					cache_item->tile_data,
					tile_head_info[tile_index].tile_size,
					sizeof(cache_item->tile_data));

#endif
			} else if (pic_head->magic == RLE_PIC_MAGIC) {
				rle_decompress(picSource + tile_head_info[tile_index].tile_addr,
						 cache_item->tile_data,
						 tile_head_info[tile_index].tile_size,
						 sizeof(cache_item->tile_data), pic_head->bytes_per_pixel);
			} else {
				return -ENOEXEC;
			}

				//tile_cache_set_valid(cache_item, picSource, tile_index,
									 //tile_head_info[tile_index].tile_size);
			//}

			ui_region_t tile_region = {
				.x1 = i * pic_head->tile_width,
				.y1 = j * pic_head->tile_height,
				.x2 = (i + 1) * pic_head->tile_width  - 1,
				.y2 = (j + 1) * pic_head->tile_height - 1,
			};

			if (tile_region.x2 >= pic_head->width) {
				tile_region.x2 = pic_head->width - 1;
			}

			if (tile_region.y2 >= pic_head->height) {
				tile_region.y2 = pic_head->height - 1;
			}

			if (ui_region_intersect(&copy_region, &crop_region, &tile_region) == false) {
				continue;
			}

			int src_stride = pic_head->bytes_per_pixel * ui_region_get_width(&tile_region);

			char* tile_dest_addr = picDst + (copy_region.x1 - x) * pic_head->bytes_per_pixel
											 				+ (copy_region.y1 - y) * out_stride;

			char* tile_src_addr = cache_item->tile_data + (copy_region.x1 - tile_region.x1) * pic_head->bytes_per_pixel
									+ (copy_region.y1 - tile_region.y1) * src_stride;

			out_size += hardware_copy(tile_dest_addr, out_stride, tile_src_addr,
					ui_region_get_width(&copy_region), ui_region_get_height(&copy_region),
					src_stride, pic_head->bytes_per_pixel);
#if CONFIG_TILE_CACHE_NUM == 1
			hardware_wait_finish();
#endif

			//copy_time += (k_cycle_get_32() - copy_start);
		}
	}

#if CONFIG_TILE_CACHE_NUM > 1
	hardware_wait_finish();
#endif

	os_strace_end_call_u32(SYS_TRACE_ID_PIC_DECOMPRESS, (x_end_tile - x_start_tile + 1) * (y_end_tile - y_start_tile + 1));

	//printk("decompress:src %p (%d %d %d %d) dec %d cost (%d = %d + %d + %d)\n",picSource, x, y, w, h, dec, k_cyc_to_us_floor32(k_cycle_get_32() - timestamp),k_cyc_to_us_floor32(get_cache_time), k_cyc_to_us_floor32(decompress_time),k_cyc_to_us_floor32(copy_time));
	return out_size;
}
