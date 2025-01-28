/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <memory/mem_cache.h>

void mem_dcache_sync(void)
{
}

void * mem_addr_to_uncache(void * addr)
{
	return addr;
}

void mem_dcache_invalidate(const void *addr, uint32_t length)
{
}

void mem_dcache_invalidate_all(void)
{
}

void mem_dcache_clean(const void *addr, uint32_t length)
{
}

void mem_dcache_clean_all(void)
{
}

void mem_dcache_flush(const void *addr, uint32_t length)
{
}

void mem_dcache_flush_all(void)
{
}

bool mem_is_cacheable(const void *addr)
{
	return true;
}

void mem_writebuf_clean(const void *addr, uint32_t length)
{
}

void mem_writebuf_clean_all(void)
{
}

bool mem_is_bufferable(const void *addr)
{
	return false;
}
