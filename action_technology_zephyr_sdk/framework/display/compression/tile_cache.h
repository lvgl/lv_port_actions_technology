/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TILE_CACHE_H__
#define __TILE_CACHE_H__

#include <compress_api.h>

//#define CONFIG_TILE_CACHE_NUM 1

//#define CONFIG_CACHE_PROFILE  1

typedef struct tile_cache_item {
	uint8_t tile_data[TILE_MAX_H * TILE_MAX_W * CONFIG_TILE_BYTES_PER_PIXELS];
	const uint8_t *pic_addr;
	uint16_t tile_index;
	uint16_t tile_size;
	uint16_t cache_valid;
	uint16_t cache_ref_cnt;	
} tile_cache_item_t;

int tile_cache_init(void);

int tile_cache_is_valid(tile_cache_item_t * cache_item);

int tile_cache_set_valid(tile_cache_item_t *cache_item, const uint8_t *pic_src, uint16_t tile_index, uint16_t tile_size);

tile_cache_item_t * tile_cache_get(const uint8_t *pic_src, uint16_t tile_index);

#endif

