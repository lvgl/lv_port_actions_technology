/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief mem pool manager.
*/

#include <os_common_api.h>
#include <mem_manager.h>
#include <string.h>
#include "mem_inner.h"
#include <sys/sys_heap.h>

#define SYS_MEM_POOL_SIZE (CONFIG_RAM_POOL_PAGE_NUM * 2048)

#ifdef CONFIG_SOC_NO_PSRAM
__in_section_unique(sys.noinit.heap)
#endif

__aligned(4) static char sys_pool_buffer[SYS_MEM_POOL_SIZE];

#ifdef CONFIG_SIMULATOR
#include <kheap.h>
struct k_heap sys_mem_pool = {
				.heap = {
					.init_mem = sys_pool_buffer,
					.init_bytes = SYS_MEM_POOL_SIZE,
				},
};
void* mem_pool_malloc(unsigned int num_bytes)
{
	void* ptr = k_heap_aligned_alloc(&sys_mem_pool, sizeof(void*), num_bytes, OS_FOREVER);
	if (ptr == NULL) {
		sys_heap_dump(&sys_mem_pool.heap);
	}
	else {
		memset(ptr, 0, num_bytes);
	}
	return ptr;
}
#else
STRUCT_SECTION_ITERABLE(k_heap, sys_mem_pool) = {
				.heap = {
					.init_mem = sys_pool_buffer,
					.init_bytes = SYS_MEM_POOL_SIZE,
				},
};

void *mem_pool_malloc(unsigned int num_bytes)
{
	void *ptr =  k_heap_aligned_alloc(&sys_mem_pool, sizeof(void *), num_bytes, K_FOREVER);
	if (ptr == NULL) {
		sys_heap_dump(&sys_mem_pool.heap);
	} else {
		memset(ptr, 0, num_bytes);
	}
	return ptr;
}
#endif

void mem_pool_free(void *ptr)
{
	if (ptr) {
		k_heap_free(&sys_mem_pool, ptr);
	}
}

void mem_pool_dump(void)
{
	sys_heap_dump(&sys_mem_pool.heap);
	os_printk("total size of pool %d \n", SYS_MEM_POOL_SIZE);
}

void mem_pool_init(void)
{
#ifdef CONFIG_SIMULATOR
	k_heap_init(&sys_mem_pool, sys_pool_buffer, SYS_MEM_POOL_SIZE);
#endif
}
