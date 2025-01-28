/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>

#if defined(CONFIG_VG_LITE_MALLOC_SYS_HEAP)
#include <init.h>
#include <sys/sys_heap.h>

__attribute__((__aligned__(4), section(".vglite.noinit.malloc")))
static uint8_t vglite_malloc_mem[CONFIG_VG_LITE_MALLOC_HEAP_SIZE];

static struct sys_heap vglite_heap;
static struct k_spinlock vglite_heap_lock;

void * vg_lite_os_malloc(size_t size)
{
	k_spinlock_key_t key;
	void *ptr;

	key = k_spin_lock(&vglite_heap_lock);
	ptr = sys_heap_alloc(&vglite_heap, size);
	k_spin_unlock(&vglite_heap_lock, key);

	if (ptr == NULL) {
		printf("vg_lite_os_malloc %u failed\n", (unsigned int)size);
	}

	return ptr;
}

void * vg_lite_os_realloc(void * ptr, size_t size)
{
	k_spinlock_key_t key;

	key = k_spin_lock(&vglite_heap_lock);
	ptr = sys_heap_realloc(&vglite_heap, ptr, size);
	k_spin_unlock(&vglite_heap_lock, key);

	if (ptr == NULL) {
		printf("vg_lite_os_realloc %u failed\n", (unsigned int)size);
	}

	return ptr;
}

void vg_lite_os_free(void *memory)
{
	k_spinlock_key_t key;

	key = k_spin_lock(&vglite_heap_lock);
	sys_heap_free(&vglite_heap, memory);
	k_spin_unlock(&vglite_heap_lock, key);
}

static int _vg_lite_osal_init(const struct device *unused)
{
	sys_heap_init(&vglite_heap, vglite_malloc_mem, ARRAY_SIZE(vglite_malloc_mem));
	return 0;
}

SYS_INIT(_vg_lite_osal_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#elif defined(CONFIG_VG_LITE_MALLOC_GUI_HEAP)

#include <ui_mem.h>

void * vg_lite_os_malloc(size_t size)
{
	return ui_mem_alloc(MEM_GUI, size, __func__);
}

void * vg_lite_os_realloc(void * ptr, size_t size)
{
	return ui_mem_realloc(MEM_GUI, ptr, size, __func__);
}

void vg_lite_os_free(void *memory)
{
	ui_mem_free(MEM_GUI, memory);
}

#else

void * vg_lite_os_malloc(size_t size)
{
	return malloc(size);
}

void * vg_lite_os_realloc(void * ptr, size_t size)
{
	return realloc(ptr, size);
}

void vg_lite_os_free(void *memory)
{
	free(memory);
}

#endif /* CONFIG_VG_LITE_MALLOC_SYS_HEAP */
