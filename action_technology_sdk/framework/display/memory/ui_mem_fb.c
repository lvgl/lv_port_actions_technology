/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief mem slab manager.
*/

#define LOG_MODULE_CUSTOMER

#include <os_common_api.h>
#include <ui_mem.h>
#include <ui_math.h>
#ifdef CONFIG_UI_MEMORY_DEBUG
#  include <mem_guard.h>
#endif

LOG_MODULE_DECLARE(ui_mem, LOG_LEVEL_INF);

#if defined(CONFIG_UI_MEM_NUMBER_BLOCKS) && CONFIG_UI_MEM_NUMBER_BLOCKS > 0

#if CONFIG_UI_MEM_NUMBER_BLOCKS >= 256
#  error "CONFIG_UI_MEM_NUMBER_BLOCKS >= 256"
#endif

/*
 * memory alignment
 * 1) must align to psram cache line size (32 bytes)
 * 2) Verisilicon vg_lite buffer memory requires 64 bytes aligned
 */
#ifdef CONFIG_VG_LITE
#  define UI_MEM_BLOCK_ALIGN 64
#else
#  define UI_MEM_BLOCK_ALIGN 32
#endif

/* align to psram cache line size (32 bytes) */
#ifdef CONFIG_UI_MEMORY_DEBUG
#  define UI_MEM_BLOCK_SIZE   UI_ROUND_UP(CONFIG_UI_MEM_BLOCK_SIZE + MEM_STAKE_SIZE, UI_MEM_BLOCK_ALIGN)
#else
#  define UI_MEM_BLOCK_SIZE   UI_ROUND_UP(CONFIG_UI_MEM_BLOCK_SIZE, UI_MEM_BLOCK_ALIGN)
#endif

#define UI_MEM_SIZE         (CONFIG_UI_MEM_NUMBER_BLOCKS * UI_MEM_BLOCK_SIZE)

static uint8_t __aligned(UI_MEM_BLOCK_ALIGN) ui_mem_base[UI_MEM_SIZE] __in_section_unique(UI_PSRAM_REGION);
/* store the allocated continuous block counts (max 255) from the index */
static uint8_t alloc_count[CONFIG_UI_MEM_NUMBER_BLOCKS];

#ifndef CONFIG_SIMULATOR
static struct k_spinlock alloc_spinlock;
#else
static OS_MUTEX_DEFINE(alloc_mutex);
#endif

static void *ui_mem_fb_alloc_internal(size_t size)
{
	uint8_t nbr_blks = 1;
	uint8_t free_blks = 0;
	uint8_t first_blk = 0;

#ifndef CONFIG_SIMULATOR
	k_spinlock_key_t key;
#endif

	if (size == 0) {
		return NULL;
	}

	if (size > UI_MEM_BLOCK_SIZE) {
		nbr_blks = (size + UI_MEM_BLOCK_SIZE - 1) / UI_MEM_BLOCK_SIZE;
		if (nbr_blks > CONFIG_UI_MEM_NUMBER_BLOCKS) {
			return NULL;
		}
	}

#ifndef CONFIG_SIMULATOR
	key = k_spin_lock(&alloc_spinlock);
#else
	os_mutex_lock(&alloc_mutex, OS_FOREVER);
#endif

	for (int i = 0; i < CONFIG_UI_MEM_NUMBER_BLOCKS;) {
		if (alloc_count[i] == 0) {
			if (free_blks == 0) {
				first_blk = i;
			}

			if (++free_blks == nbr_blks) {
				alloc_count[first_blk] = nbr_blks;
				break;
			}

			i++;
		} else {
			free_blks = 0;
			i += alloc_count[i];
			if (i + nbr_blks > CONFIG_UI_MEM_NUMBER_BLOCKS) {
				break;
			}
		}
	}

#ifndef CONFIG_SIMULATOR
	k_spin_unlock(&alloc_spinlock, key);
#else
	os_mutex_unlock(&alloc_mutex);
#endif

	if (free_blks == nbr_blks) {
		return ui_mem_base + first_blk * UI_MEM_BLOCK_SIZE;
	}

	return NULL;
}

void *ui_mem_fb_alloc(size_t size)
{
	void *ptr = NULL;
	int try = 30;

	do {
		ptr = ui_mem_fb_alloc_internal(size);
		if (ptr)
			break;

		os_sleep(2);
	} while (--try > 0);

	return ptr;
}

void * ui_mem_fb_aligned_alloc(size_t align, size_t size)
{
	return (align <= UI_MEM_BLOCK_ALIGN) ? ui_mem_fb_alloc(size) : NULL;
}

void ui_mem_fb_free(void *ptr)
{
	uint8_t *ptr8 = ptr;
	uint8_t blkidx;

#ifndef CONFIG_SIMULATOR
	k_spinlock_key_t key;
#endif

	if (ptr8 < ui_mem_base || ptr8 >= ui_mem_base + UI_MEM_SIZE) {
		return;
	}

	blkidx = (uint32_t)(ptr8 - ui_mem_base) / UI_MEM_BLOCK_SIZE;

#ifndef CONFIG_SIMULATOR
	key = k_spin_lock(&alloc_spinlock);

	__ASSERT(alloc_count[blkidx] > 0, "double-free for memory at %p", ptr8);
#else
	os_mutex_lock(&alloc_mutex, OS_FOREVER);
#endif

	alloc_count[blkidx] = 0;

#ifndef CONFIG_SIMULATOR
	k_spin_unlock(&alloc_spinlock, key);
#else
	os_mutex_unlock(&alloc_mutex);
#endif
}

size_t ui_mem_fb_get_size(void)
{
	return UI_MEM_SIZE;
}

void ui_mem_fb_dump(void)
{
	os_printk("FB heap at %p, block size %lu, count %u\n", ui_mem_base,
			UI_MEM_BLOCK_SIZE, CONFIG_UI_MEM_NUMBER_BLOCKS);

	for (int i = 0; i < CONFIG_UI_MEM_NUMBER_BLOCKS;) {
		if (alloc_count[i] > 0) {
			void *ptr = ui_mem_base + i * UI_MEM_BLOCK_SIZE;

			os_printk("ptr %p, %u blocks\n", ptr, alloc_count[i]);

			i += alloc_count[i];
		} else {
			i += 1;
		}
	}
}

bool ui_mem_is_fb(const void * ptr)
{
	const uint8_t *mem_end = ui_mem_base + UI_MEM_SIZE;
	const uint8_t *ptr8 = ptr;

	return (ptr8 >= ui_mem_base && ptr8 < mem_end) ? true : false;
}

#endif /* CONFIG_UI_MEM_NUMBER_BLOCKS > 0 */

size_t ui_mem_fb_get_num(void)
{
	return CONFIG_UI_MEM_NUMBER_BLOCKS;
}
