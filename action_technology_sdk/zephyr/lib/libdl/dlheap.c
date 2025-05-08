/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/sys_heap.h>

/* heap memory for module */
static struct sys_heap _module_heap;
static uint8_t _module_heap_mem[CONFIG_LIBDL_MODULE_HEAP_SIZE] __aligned(4);
static K_MUTEX_DEFINE(_module_heap_mutex);

void dlheap_init(void)
{
	sys_heap_init(&_module_heap, _module_heap_mem, CONFIG_LIBDL_MODULE_HEAP_SIZE);
}

void *dlheap_malloc(uint32_t size)
{
	void *ptr;

	k_mutex_lock(&_module_heap_mutex, K_FOREVER);
	ptr = sys_heap_aligned_alloc(&_module_heap, 8, size);
	k_mutex_unlock(&_module_heap_mutex);

	return ptr;
}

void dlheap_free(void *ptr)
{
	k_mutex_lock(&_module_heap_mutex, K_FOREVER);
	sys_heap_free(&_module_heap, ptr);
	k_mutex_unlock(&_module_heap_mutex);
}

void dlheap_dump(void)
{
	k_mutex_lock(&_module_heap_mutex, K_FOREVER);
	sys_heap_dump(&_module_heap);
	k_mutex_unlock(&_module_heap_mutex);
}

