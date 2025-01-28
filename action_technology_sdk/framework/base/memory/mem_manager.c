/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief mem manager.
*/
#include <mem_manager.h>
#include "mem_inner.h"
#ifdef CONFIG_ACTIONS_PRINTK_DMA
#include <sys/printk.h>
#endif
#ifdef CONFIG_SYS_MEMORY_DEBUG
#include "mem_guard.h"
static struct mem_guard_head mem_guard;
#endif

void *mem_malloc_debug(size_t size,const char *func)
{
#ifdef CONFIG_SYS_MEMORY_DEBUG
    return mem_guard_malloc(&mem_guard, mem_pool_malloc, size, func);
#else
	return mem_pool_malloc(size);
#endif
}

void mem_free(void *ptr)
{
	if (ptr == NULL)
		return;

#ifdef CONFIG_SYS_MEMORY_DEBUG
    mem_guard_free(&mem_guard, mem_pool_free, ptr);
#else
	mem_pool_free(ptr);
#endif
}

void mem_manager_dump(void)
{
#ifdef CONFIG_ACTIONS_PRINTK_DMA
	printk_dma_switch(0);
#endif /* CONFIG_ACTIONS_PRINTK_DMA */
#ifdef CONFIG_SYS_MEMORY_DEBUG
	mem_guard_dump(&mem_guard);
#else
	mem_pool_dump();
#endif
#ifdef CONFIG_ACTIONS_PRINTK_DMA
	printk_dma_switch(1);
#endif /* CONFIG_ACTIONS_PRINTK_DMA */
}

void mem_manager_dump_ext(int dump_detail, const char* match_value)
{

}

int mem_manager_check_overwrite(void)
{
#ifdef CONFIG_SYS_MEMORY_DEBUG
	mem_guard_check_overwrite(&mem_guard);
#endif
	return 0;
}

int mem_manager_init(void)
{
	mem_pool_init();
#ifdef CONFIG_SYS_MEMORY_DEBUG
	mem_guard_init(&mem_guard, CONFIG_RAM_POOL_PAGE_NUM * 2048);
#endif
	return 0;
}


static int current_user_info = -1;

void mem_manager_set_user_info(int user_info)
{
	current_user_info = user_info;

#ifdef CONFIG_MEM_GUARD
	extern void mem_guard_set_user_info(int user_info);
	mem_guard_set_user_info(current_user_info);
#endif
}

int mem_manager_get_user_info(void)
{
	return current_user_info;
}
