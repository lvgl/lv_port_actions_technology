/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file bluetooth mem interface
 */
#define SYS_LOG_DOMAIN "bt"
#include <os_common_api.h>

#define CONFIG_BT_INNER_HEAP	1
#define CONFIG_BT_MEM_DEBUG		0

#if CONFIG_BT_INNER_HEAP
#ifdef CONFIG_SOC_NO_PSRAM
__in_section_unique(bthost_bss)
#endif
static char __aligned(4) bt_mem_buffer[1024*6];
STRUCT_SECTION_ITERABLE(k_heap, bt_mem_pool) = {
	.heap = {
		.init_mem = bt_mem_buffer,
		.init_bytes = sizeof(bt_mem_buffer),
	},
};
#endif

#if CONFIG_BT_MEM_DEBUG
#define BT_MEM_DEBUG_MAX	100
static uint32_t bt_max_malloc_cut = 0;
static uint32_t bt_cur_malloc_cut = 0;
static uint32_t bt_max_memory_used = 0;
static uint32_t bt_cur_memory_used = 0;

static uint32_t bt_mem_debug[BT_MEM_DEBUG_MAX][2];

static void bt_mem_debug_malloc(void *addr, uint32_t size)
{
	uint32_t i;

	os_sched_lock();

	bt_cur_malloc_cut++;
	if (bt_cur_malloc_cut > bt_max_malloc_cut) {
		bt_max_malloc_cut = bt_cur_malloc_cut;
	}

	bt_cur_memory_used += size;
	if (bt_cur_memory_used > bt_max_memory_used) {
		bt_max_memory_used = bt_cur_memory_used;
	}

	for (i=0; i<BT_MEM_DEBUG_MAX; i++) {
		if (bt_mem_debug[i][0] == 0) {
			bt_mem_debug[i][0] = (uint32_t)addr;
			bt_mem_debug[i][1] = size;
			os_sched_unlock();
			return;
		}
	}

	os_sched_unlock();
	SYS_LOG_INF("No index for debug addr %p", addr);
}

static void bt_mem_debug_free(void *addr)
{
	uint32_t i;

	os_sched_lock();
	bt_cur_malloc_cut--;

	for (i=0; i<BT_MEM_DEBUG_MAX; i++) {
		if (bt_mem_debug[i][0] == (uint32_t)addr) {
			bt_mem_debug[i][0] = 0;
			bt_cur_memory_used -= bt_mem_debug[i][1];
			os_sched_unlock();
			return;
		}
	}

	os_sched_unlock();
	SYS_LOG_INF("Unknow addr %p", addr);
}

void bt_mem_debug_print(void)
{
	uint32_t i, flag = 0;

	SYS_LOG_INF("bt mem max malloc cnt:%d, bt_cur_malloc_cut:%d", bt_max_malloc_cut, bt_cur_malloc_cut);
	SYS_LOG_INF("bt bt_max_memory_used:%d, bt_cur_memory_used:%d", bt_max_memory_used, bt_cur_memory_used);

	for (i=0; i<BT_MEM_DEBUG_MAX; i++) {
		if (bt_mem_debug[i][0] != 0) {
			printk("Unfree mem addr:0x%x, size:%d\n", bt_mem_debug[i][0], bt_mem_debug[i][1]);
			flag = 1;
		}
	}

	if (!flag) {
		printk("All mem free!\n");
	}
}
#endif

void *bt_mem_malloc(int size)
{
	void *p;

#if CONFIG_BT_INNER_HEAP
	p = k_heap_alloc(&bt_mem_pool, size, K_NO_WAIT);
#else
	p= mem_malloc(size);
#endif
	if (!p) {
		SYS_LOG_ERR("bt_mem_malloc %d failed!!", size);
	}

	if (size >= 512) {
		SYS_LOG_INF("bt_mem_malloc block %d", size);
	}

#if CONFIG_BT_MEM_DEBUG
	if (p) {
		bt_mem_debug_malloc(p, size);
	}
#endif

	return p;
}

void bt_mem_free(void *ptr)
{
#if CONFIG_BT_MEM_DEBUG
	if (ptr) {
		bt_mem_debug_free(ptr);
	}
#endif

	if (ptr != NULL) {
#if CONFIG_BT_INNER_HEAP
		k_heap_free(&bt_mem_pool, ptr);
#else
		mem_free(ptr);
#endif
	}
}
