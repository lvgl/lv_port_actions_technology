/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_CUSTOMER

#include <os_common_api.h>
#include <sys/sys_heap.h>
#include <ui_mem.h>

LOG_MODULE_DECLARE(ui_mem, LOG_LEVEL_INF);

#if defined(CONFIG_UI_RES_MEM_POOL_SIZE) && CONFIG_UI_RES_MEM_POOL_SIZE > 0

__in_section_unique(RES_PSRAM_REGION) __aligned(64) static uint8_t res_heap_mem[CONFIG_UI_RES_MEM_POOL_SIZE];
__in_section_unique(RES_PSRAM_REGION) static struct sys_heap res_heap;
static OS_MUTEX_DEFINE(res_mutex);

int ui_mem_res_init(void)
{
	sys_heap_init(&res_heap, res_heap_mem, CONFIG_UI_RES_MEM_POOL_SIZE);
	return 0;
}

void * ui_mem_res_alloc(size_t size)
{
	void * ptr = NULL;

	os_mutex_lock(&res_mutex, OS_FOREVER);
	ptr = sys_heap_alloc(&res_heap, size);
	os_mutex_unlock(&res_mutex);

	return ptr;
}

void * ui_mem_res_aligned_alloc(size_t align, size_t size)
{
	void * ptr = NULL;

	os_mutex_lock(&res_mutex, OS_FOREVER);
	ptr = sys_heap_aligned_alloc(&res_heap, align, size);
	os_mutex_unlock(&res_mutex);

	return ptr;
}

void * ui_mem_res_realloc(void * ptr, size_t size)
{
	os_mutex_lock(&res_mutex, OS_FOREVER);
	ptr = sys_heap_realloc(&res_heap, ptr, size);
	os_mutex_unlock(&res_mutex);

	return ptr;
}

void ui_mem_res_free(void * ptr)
{
	os_mutex_lock(&res_mutex, OS_FOREVER);
	sys_heap_free(&res_heap, ptr);
	os_mutex_unlock(&res_mutex);
}

size_t ui_mem_res_get_size(void)
{
	return CONFIG_UI_RES_MEM_POOL_SIZE;
}

void ui_mem_res_dump(void)
{
	sys_heap_dump(&res_heap);
}

bool ui_mem_is_res(const void * ptr)
{
	const uint8_t *res_mem_end = res_heap_mem + CONFIG_UI_RES_MEM_POOL_SIZE;
	const uint8_t *ptr8 = ptr;

	return (ptr8 >= res_heap_mem && ptr8 < res_mem_end) ? true : false;
}

#endif /* CONFIG_UI_RES_MEM_POOL_SIZE > 0 */
