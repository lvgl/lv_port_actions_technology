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
#include "tile_cache.h"
#define CONFIG_TILE_CACHE_LOOP_MODE 1

__aligned(32) __in_section_unique(tile.bss.cache)
static tile_cache_item_t tile_cache[CONFIG_TILE_CACHE_NUM];

static bool cache_init = false;

#ifdef CONFIG_TILE_CACHE_LOOP_MODE
static uint16_t current_index = 0;
#endif

#ifdef CONFIG_CACHE_PROFILE
static int total_cnt = 0;
static int hit_cnt = 0;
#endif

int tile_cache_init(void)
{
	for (int i = 0 ; i < CONFIG_TILE_CACHE_NUM; i++) {
		tile_cache[i].cache_valid = 0;
		tile_cache[i].pic_addr = 0;
		tile_cache[i].cache_ref_cnt = 0;
	}
	current_index = 0;
	return 0;
}

__ramfunc int tile_cache_is_valid(tile_cache_item_t * cache_item)
{
	if (!cache_item)
		return 0;

	return cache_item->cache_valid;
}

__ramfunc int tile_cache_set_valid(tile_cache_item_t *cache_item, const uint8_t *pic_src, uint16_t tile_index, uint16_t tile_size)
{
	if (!cache_item)
		return -EINVAL;

	cache_item->pic_addr = pic_src;
	cache_item->tile_index = tile_index;
	cache_item->tile_size = tile_size;
	cache_item->cache_valid = 1;
	cache_item->cache_ref_cnt = 0;
	return 0;
}

#ifdef CONFIG_TILE_CACHE_LOOP_MODE
__ramfunc tile_cache_item_t *tile_cache_get(const uint8_t *pic_src, uint16_t tile_index)
{
	if (!cache_init) {
		tile_cache_init();
		cache_init = true;
	}

	tile_cache_item_t *replace_cache_item = &tile_cache[current_index];


	if (++current_index >= CONFIG_TILE_CACHE_NUM) {
		current_index = 0;
	}

	replace_cache_item->cache_valid = 0;

	return replace_cache_item;
}
#else
__ramfunc tile_cache_item_t *tile_cache_get(const uint8_t *pic_src, uint16_t tile_index)
{
	if (!cache_init) {
		tile_cache_init();
		cache_init = true;
	}

	tile_cache_item_t *new_cache_item = NULL;
	tile_cache_item_t *replace_cache_item = &tile_cache[0];
#ifdef CONFIG_CACHE_PROFILE
	int replace_cache_index = 0;
#endif

#ifdef CONFIG_CACHE_PROFILE
	total_cnt ++;
	printk("cache hit %d %% \n",hit_cnt * 100 / total_cnt);
	printk("\n");
	for (int i = 0 ; i < CONFIG_TILE_CACHE_NUM; i++) {
		printk(" %d ",tile_cache[i].cache_ref_cnt);
	}
	printk("\n");
#endif

	for (int i = 0 ; i < CONFIG_TILE_CACHE_NUM; i++) {
		if (tile_cache[i].cache_valid && tile_cache[i].pic_addr == pic_src
				&& tile_cache[i].tile_index == tile_index) {
			new_cache_item = &tile_cache[i];
		#ifdef CONFIG_CACHE_PROFILE
			hit_cnt++;
		#endif
		} else {
			if (!tile_cache[i].cache_valid) {
				new_cache_item = &tile_cache[i];
				printk("new cache %d \n",i);
			} else {
				if (replace_cache_item->cache_ref_cnt < tile_cache[i].cache_ref_cnt) {
					replace_cache_item = &tile_cache[i];
#ifdef CONFIG_CACHE_PROFILE
					replace_cache_index  = i;
#endif
				}
				tile_cache[i].cache_ref_cnt += 1;
			}
		}
	}
	if (new_cache_item) {
		return new_cache_item;
	} else if (replace_cache_item) {
		replace_cache_item->cache_valid = 0;
#ifdef CONFIG_CACHE_PROFILE
		printk("replace cache %d \n",replace_cache_index);
#endif
		return replace_cache_item;
	}
	return NULL;
}
#endif
