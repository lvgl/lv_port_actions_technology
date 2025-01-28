/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_CUSTOMER

#include <os_common_api.h>
#include <sys/sys_heap.h>
#include <ui_mem.h>
#include <assert.h>
#ifdef CONFIG_LVGL
#  include <lvgl/lvgl.h>
#endif

LOG_MODULE_DECLARE(ui_mem, LOG_LEVEL_INF);

#if defined(CONFIG_UI_GUI_MEM_POOL_SIZE) && CONFIG_UI_GUI_MEM_POOL_SIZE > 0

__in_section_unique(lvgl.noinit.malloc) __aligned(4) static uint8_t gui_heap_mem[CONFIG_UI_GUI_MEM_POOL_SIZE];
__in_section_unique(lvgl.noinit.malloc) static struct sys_heap gui_heap;

static void _assert_in_gui_tid(const char *tag)
{
	static uintptr_t gui_tid = 0;
	uintptr_t tid = (uintptr_t)os_current_get();

#ifdef CONFIG_LVGL
	if (lv_is_initialized() == false) {
		return;
	}
#endif

	if (gui_tid == 0) {
		gui_tid = tid;
		return;
	}

	if (gui_tid != tid) {
		os_printk("%s called in thread 0x%x\n", tag, tid);
		//assert(gui_tid == tid);

#ifndef CONFIG_SIMULATOR
		//k_panic();
#endif /* CONFIG_SIMULATOR */
	}
}

int ui_mem_gui_init(void)
{
	sys_heap_init(&gui_heap, gui_heap_mem, CONFIG_UI_GUI_MEM_POOL_SIZE);
	return 0;
}

void * ui_mem_gui_alloc(size_t size)
{
	_assert_in_gui_tid("gui_malloc");

	return sys_heap_alloc(&gui_heap, size);
}

void * ui_mem_gui_aligned_alloc(size_t align, size_t size)
{
	return sys_heap_aligned_alloc(&gui_heap, align, size);
}

void * ui_mem_gui_realloc(void * ptr, size_t size)
{
	_assert_in_gui_tid("gui_realloc");

	return sys_heap_realloc(&gui_heap, ptr, size);
}

void ui_mem_gui_free(void * ptr)
{
	_assert_in_gui_tid("gui_free");

	sys_heap_free(&gui_heap, ptr);
}

size_t ui_mem_gui_get_size(void)
{
	return CONFIG_UI_GUI_MEM_POOL_SIZE;
}

void ui_mem_gui_dump(void)
{
	sys_heap_dump(&gui_heap);
}

bool ui_mem_is_gui(const void * ptr)
{
	const uint8_t *gui_mem_end = gui_heap_mem + CONFIG_UI_GUI_MEM_POOL_SIZE;
	const uint8_t *ptr8 = ptr;

	return (ptr8 >= gui_heap_mem && ptr8 < gui_mem_end) ? true : false;
}

#endif /* CONFIG_UI_GUI_MEM_POOL_SIZE > 0 */
