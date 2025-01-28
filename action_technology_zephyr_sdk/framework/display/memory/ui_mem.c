/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_CUSTOMER

#include <os_common_api.h>
#include <assert.h>
#include <string.h>
#include <ui_mem.h>
#ifdef CONFIG_UI_MEMORY_DEBUG
#  include <mem_guard.h>
#endif
#ifdef CONFIG_ACTIONS_PRINTK_DMA
#  include <sys/printk.h>
#endif

LOG_MODULE_REGISTER(ui_mem, LOG_LEVEL_INF);

#define FORCE_CALLER_STRING 1
#if FORCE_CALLER_STRING
#  define PRI_CALLER "%s"
#else
#  define PRI_CALLER "%p"
#endif

#if CONFIG_UI_MEM_NUMBER_BLOCKS > 0
extern void * ui_mem_fb_alloc(size_t size);
extern void * ui_mem_fb_aligned_alloc(size_t align, size_t size);
extern void ui_mem_fb_free(void *ptr);
extern size_t ui_mem_fb_get_size(void);
extern void ui_mem_fb_dump(void);
extern bool ui_mem_is_fb(const void * ptr);
#endif /* CONFIG_UI_MEM_NUMBER_BLOCKS */

#if CONFIG_UI_GUI_MEM_POOL_SIZE > 0
extern int ui_mem_gui_init(void);
extern void * ui_mem_gui_alloc(size_t size);
extern void * ui_mem_gui_aligned_alloc(size_t align, size_t size);
extern void * ui_mem_gui_realloc(void *ptr, size_t size);
extern void ui_mem_gui_free(void *ptr);
extern size_t ui_mem_gui_get_size(void);
extern void ui_mem_gui_dump(void);
extern bool ui_mem_is_gui(const void * ptr);
#endif /* CONFIG_UI_GUI_MEM_POOL_SIZE */

#if CONFIG_UI_RES_MEM_POOL_SIZE > 0
extern int ui_mem_res_init(void);
extern void * ui_mem_res_alloc(size_t size);
extern void * ui_mem_res_aligned_alloc(size_t align, size_t size);
extern void * ui_mem_res_realloc(void * ptr, size_t size);
extern void ui_mem_res_free(void *ptr);
extern size_t ui_mem_res_get_size(void);
extern void ui_mem_res_dump(void);
extern bool ui_mem_is_res(const void * ptr);
#endif /* CONFIG_UI_RES_MEM_POOL_SIZE */

struct ui_mem_func {
	const char * name;
	int (* init_fn)(void);
	void * (* alloc_fn)(size_t size);
	void * (* aligned_alloc_fn)(size_t align, size_t size);
	void * (* realloc_fn)(void * ptr, size_t size);
	void (* free_fn)(void * ptr);
	void (* dump_fn)(void);
	bool (* is_type_fn)(const void * ptr);
	size_t (* get_size)(void);
};

static const struct ui_mem_func mem_table[NUM_UI_MEM_TYPES] = {
	[MEM_FB] = {
		.name = "FB",
#if CONFIG_UI_MEM_NUMBER_BLOCKS > 0
		.alloc_fn = ui_mem_fb_alloc,
		.aligned_alloc_fn = ui_mem_fb_aligned_alloc,
		.free_fn = ui_mem_fb_free,
		.dump_fn = ui_mem_fb_dump,
		.is_type_fn = ui_mem_is_fb,
		.get_size = ui_mem_fb_get_size,
#endif /* CONFIG_UI_MEM_NUMBER_BLOCKS > 0 */
	},

	[MEM_GUI] = {
		.name = "GUI",
#if CONFIG_UI_GUI_MEM_POOL_SIZE > 0
		.init_fn = ui_mem_gui_init,
		.alloc_fn = ui_mem_gui_alloc,
		.aligned_alloc_fn = ui_mem_gui_aligned_alloc,
		.realloc_fn = ui_mem_gui_realloc,
		.free_fn = ui_mem_gui_free,
		.dump_fn = ui_mem_gui_dump,
		.is_type_fn = ui_mem_is_gui,
		.get_size = ui_mem_gui_get_size,
#endif /* CONFIG_UI_GUI_MEM_POOL_SIZE > 0 */
	},

	[MEM_RES] = {
		.name = "RES",
#if CONFIG_UI_RES_MEM_POOL_SIZE > 0
		.init_fn = ui_mem_res_init,
		.alloc_fn = ui_mem_res_alloc,
		.aligned_alloc_fn = ui_mem_res_aligned_alloc,
		.realloc_fn = ui_mem_res_realloc,
		.free_fn = ui_mem_res_free,
		.dump_fn = ui_mem_res_dump,
		.is_type_fn = ui_mem_is_res,
		.get_size = ui_mem_res_get_size,
#elif CONFIG_UI_GUI_MEM_POOL_SIZE > 0
		.init_fn = NULL,
		.alloc_fn = ui_mem_gui_alloc,
		.aligned_alloc_fn = ui_mem_gui_aligned_alloc,
		.realloc_fn = ui_mem_gui_realloc,
		.free_fn = ui_mem_gui_free,
		.dump_fn = NULL,
		.is_type_fn = NULL, /* mixed res mem should be considered as gui mem */
		.get_size = ui_mem_gui_get_size,
#endif /* CONFIG_UI_RES_MEM_POOL_SIZE > 0 */
	},
};

#ifdef CONFIG_UI_MEMORY_DEBUG
/* Exclude MEM_FB to reduce memory footprint */
static struct mem_guard_head mem_guard[NUM_UI_MEM_TYPES - 1];
#endif

int ui_mem_init(void)
{
	int i;

	for (i = 0; i < NUM_UI_MEM_TYPES; i++) {
		if (mem_table[i].init_fn) {
			mem_table[i].init_fn();
		}

#ifdef CONFIG_UI_MEMORY_DEBUG
		if (i > 0 && mem_table[i].get_size != NULL) {
			mem_guard_init(&mem_guard[i - 1], mem_table[i].get_size());
		}
#endif
	}

	return 0;
}

void * ui_mem_alloc(uint8_t type, size_t size, const void * caller)
{
	void * ptr = NULL;

	if (size == 0) {
		return NULL;
	}

	if (type >= NUM_UI_MEM_TYPES || mem_table[type].alloc_fn == NULL) {
		SYS_LOG_ERR("mem %d alloc %u unsupported (caller " PRI_CALLER ")\n",
				type, (uint32_t)size, (char *)caller);
		return NULL;
	}

#ifdef CONFIG_UI_MEMORY_DEBUG
	if (type == MEM_FB) {
		ptr = mem_table[type].alloc_fn(size);
	} else {
		ptr = mem_guard_malloc(&mem_guard[type - 1],
					mem_table[type].alloc_fn, size, caller);
	}

#else
	ptr = mem_table[type].alloc_fn(size);
#endif /* CONFIG_UI_MEMORY_DEBUG */

	if (ptr == NULL) {
		SYS_LOG_ERR("UI-%s alloc %u failed (caller " PRI_CALLER ")\n",
				mem_table[type].name, (uint32_t)size, (char *)caller);
	}

	return ptr;
}

void * ui_mem_aligned_alloc(uint8_t type, size_t align, size_t size, const void * caller)
{
	void * ptr = NULL;

	if (size == 0) {
		return NULL;
	}

	if (type >= NUM_UI_MEM_TYPES || mem_table[type].aligned_alloc_fn == NULL) {
		SYS_LOG_ERR("mem %d aligned_alloc %u-%u unsupported (caller " PRI_CALLER ")\n",
				type, (uint32_t)align, (uint32_t)size, (char *)caller);
		return NULL;
	}

	assert((align & (align - 1)) == 0);

#ifdef CONFIG_UI_MEMORY_DEBUG
	if (type == MEM_FB) {
		ptr = mem_table[type].aligned_alloc_fn(align, size);
	} else {
		ptr = mem_guard_aligned_alloc(&mem_guard[type - 1],
					mem_table[type].aligned_alloc_fn, align, size, caller);
	}
#else
	ptr = mem_table[type].aligned_alloc_fn(align, size);
#endif /* CONFIG_UI_MEMORY_DEBUG */

	if (ptr == NULL) {
		SYS_LOG_ERR("UI-%s aligned_alloc %u-%u failed (caller " PRI_CALLER ")\n",
				mem_table[type].name, (uint32_t)align, (uint32_t)size, (char *)caller);
	}

	return ptr;
}

void * ui_mem_realloc(uint8_t type, void * ptr, size_t size, const void * caller)
{
	if (type >= NUM_UI_MEM_TYPES) {
		SYS_LOG_ERR("mem %d realloc %u unsupported (caller " PRI_CALLER ")\n",
				type, (uint32_t)size, (char *)caller);
		return NULL;
	}

	if (mem_table[type].realloc_fn == NULL) {
		void * ptr_new = ui_mem_alloc(type, size, caller);
		if (ptr_new && ptr) {
			memcpy(ptr_new, ptr, size);
		}

		ui_mem_free(type, ptr);
		ptr = ptr_new;
	} else {
#ifdef CONFIG_UI_MEMORY_DEBUG
		if (type == MEM_FB) {
			ptr = mem_table[type].realloc_fn(ptr, size);
		} else {
			ptr = mem_guard_realloc(&mem_guard[type - 1],
						mem_table[type].realloc_fn, ptr, size, caller);
		}
#else
		ptr = mem_table[type].realloc_fn(ptr, size);
#endif /* CONFIG_UI_MEMORY_DEBUG */
	}

	if (ptr == NULL) {
		SYS_LOG_ERR("UI-%s realloc %u failed (caller " PRI_CALLER ")\n",
				mem_table[type].name, (uint32_t)size, (char *)caller);
	}

	return ptr;
}

void * ui_mem_calloc(uint8_t type, size_t nmemb, size_t size, const void * caller)
{
	void * ptr = NULL;

	size *= nmemb;

	ptr = ui_mem_alloc(type, size, caller);
	if (ptr) {
		memset(ptr, 0, size);
	}

	return ptr;
}

void ui_mem_free(uint8_t type, void * ptr)
{
	if (ptr == NULL) {
		return;
	}

	if (type >= NUM_UI_MEM_TYPES) {
		SYS_LOG_ERR("mem %d free unsupported\n", type);
		return;
	}

	if (mem_table[type].free_fn) {
#ifdef CONFIG_UI_MEMORY_DEBUG
		if (type == MEM_FB) {
			mem_table[type].free_fn(ptr);
		} else {
			mem_guard_free(&mem_guard[type - 1], mem_table[type].free_fn, ptr);
		}
#else
		mem_table[type].free_fn(ptr);
#endif /* CONFIG_UI_MEMORY_DEBUG */
	}
}

void ui_mem_free2(void * ptr)
{
	int i;

	if (ptr == NULL) {
		return;
	}

	for (i = 0; i < NUM_UI_MEM_TYPES; i++) {
		if (!mem_table[i].is_type_fn || !mem_table[i].is_type_fn(ptr)) {
			continue;
		}

		if (mem_table[i].free_fn) {
#ifdef CONFIG_UI_MEMORY_DEBUG
			if (i == MEM_FB) {
				mem_table[i].free_fn(ptr);
			} else {
				mem_guard_free(&mem_guard[i - 1], mem_table[i].free_fn, ptr);
			}
#else
			mem_table[i].free_fn(ptr);
#endif /* CONFIG_UI_MEMORY_DEBUG */
		}

		return;
	}

	SYS_LOG_ERR("unknown ptr %p to free", ptr);
}

bool ui_mem_is_type(uint8_t type, const void * ptr)
{
	bool is_type = false;

	if (type < NUM_UI_MEM_TYPES && mem_table[type].is_type_fn) {
		is_type = mem_table[type].is_type_fn(ptr);
	}

	return is_type;
}

void ui_mem_dump(uint8_t type)
{
	if (type >= NUM_UI_MEM_TYPES) {
		return;
	}

#ifdef CONFIG_ACTIONS_PRINTK_DMA
	printk_dma_switch(0);
#endif /* CONFIG_ACTIONS_PRINTK_DMA */

	if (mem_table[type].dump_fn) {
		os_printk("UI-%s dump:\n", mem_table[type].name);

#ifdef CONFIG_UI_MEMORY_DEBUG
		if (type != MEM_FB) {
			mem_guard_dump(&mem_guard[type - 1]);
		}
#endif /* CONFIG_UI_MEMORY_DEBUG */

		mem_table[type].dump_fn();
	}

	os_printk("\n");

#ifdef CONFIG_ACTIONS_PRINTK_DMA
	printk_dma_switch(1);
#endif /* CONFIG_ACTIONS_PRINTK_DMA */
}

void ui_mem_dump_all(void)
{
	int i;

	for (i = 0; i < NUM_UI_MEM_TYPES; i++) {
		ui_mem_dump(i);
	}
}

void ui_mem_safe_check(uint16_t view_id)
{
#ifdef CONFIG_UI_MEMORY_DEBUG
	int i;

	os_printk("view_id %d mem check:\n", view_id);

	for (i = MEM_FB + 1; i < NUM_UI_MEM_TYPES; i++) {
		os_printk("checking %s:\n", mem_table[i].name);
		mem_guard_leak_check(&mem_guard[i - 1], view_id);
		os_printk("check end\n");
	}
#endif /* CONFIG_UI_MEMORY_DEBUG */
}

#ifdef CONFIG_UI_MEM_VDB_SHARE_SURFACE_BUFFER

#ifndef CONFIG_SIMULATOR
__in_section_unique(sram.noinit.sufacebuffer) __aligned(64)
#endif
static uint8_t globle_share_surface_buffer[CONFIG_UI_MEM_BLOCK_SIZE];
static bool g_share_surface_buffer_painted = false;

void *ui_mem_get_share_surface_buffer(void)
{
	return &globle_share_surface_buffer;
}

int ui_mem_get_share_surface_buffer_size(void)
{
	return CONFIG_UI_MEM_BLOCK_SIZE;
}

bool ui_mem_set_share_surface_buffer_painted(bool painted)
{
	bool prev = g_share_surface_buffer_painted;
	g_share_surface_buffer_painted = painted;
	return prev;
}

#else

void *ui_mem_get_share_surface_buffer(void) { return NULL; }
int ui_mem_get_share_surface_buffer_size(void) { return 0; }
bool ui_mem_set_share_surface_buffer_painted(bool painted) { return false; }

#endif /* CONFIG_UI_MEM_VDB_SHARE_SURFACE_BUFFER */
