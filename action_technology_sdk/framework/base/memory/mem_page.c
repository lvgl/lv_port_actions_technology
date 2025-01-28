/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief mem page manager.
*/
#include <os_common_api.h>
#include <mem_manager.h>
#include <kernel.h>
#include <kernel_structs.h>
#include <sys/atomic.h>
#include <init.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <string.h>
#include <mem_buddy.h>

#ifdef CONFIG_APP_USED_MEM_PAGE
#include "page_buddy/include/page_inner.h"
#include "page_buddy/include/buddy_inner.h"
#endif

struct mem_info sys_meminfo;

void *mem_page_malloc(unsigned int num_bytes, void *caller)
{
    return mem_buddy_malloc(num_bytes, num_bytes, &sys_meminfo, caller);
}

void check_mem_debug(void *where, struct mem_info *mem_info, void *caller, struct buddy_debug_info *buddy_debug, uint32_t size)
{
    
}

void mem_page_free(void *ptr, void *caller)
{
    return mem_buddy_free(ptr, &sys_meminfo, caller);
}

void mem_page_init(void)
{
#ifdef CONFIG_APP_USED_MEM_PAGE
	pagepool_init();
	buddy_rom_data_init();
#endif

	memset(&sys_meminfo, 0, sizeof(sys_meminfo));
	memset(sys_meminfo.buddys, -1, sizeof(sys_meminfo.buddys));
}



void mem_page_dump(uint32_t dump_detail, const char* match_str)
{
	mem_buddy_dump_info(dump_detail, match_str);
}
