/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <spicache.h>
#include <memory/mem_cache.h>

static K_MUTEX_DEFINE(cache_mutex);

static void _mem_dcache_ops(uint8_t ops, const void *addr, uint32_t length)
{
	if (buf_is_psram(addr)) {
		k_mutex_lock(&cache_mutex, K_FOREVER);
		spi1_cache_ops(ops, (void *)addr, length);
		k_mutex_unlock(&cache_mutex);
	}
}

void mem_dcache_sync(void)
{
	k_mutex_lock(&cache_mutex, K_FOREVER);
	spi1_cache_ops_wait_finshed();
	k_mutex_unlock(&cache_mutex);
}

void * mem_addr_to_uncache(void * addr)
{
	if (buf_is_psram(addr)) {
		return cache_to_uncache(addr);
	}

	return addr;
}

void mem_dcache_invalidate(const void *addr, uint32_t length)
{
	_mem_dcache_ops(SPI_CACHE_INVALIDATE, addr, length);
}

void mem_dcache_invalidate_all(void)
{
#ifdef CONFIG_PSRAM_SIZE
	_mem_dcache_ops(SPI_CACHE_INVALID_ALL, (void *)CONFIG_PSRAM_BASE_ADDRESS, CONFIG_PSRAM_SIZE * 1024);
#endif
}

void mem_dcache_clean(const void *addr, uint32_t length)
{
	_mem_dcache_ops(SPI_CACHE_FLUSH, addr, length);
}

void mem_dcache_clean_all(void)
{
#ifdef CONFIG_PSRAM_SIZE
	_mem_dcache_ops(SPI_CACHE_FLUSH_ALL, (void *)CONFIG_PSRAM_BASE_ADDRESS, CONFIG_PSRAM_SIZE * 1024);
#endif
}

void mem_dcache_flush(const void *addr, uint32_t length)
{
	_mem_dcache_ops(SPI_CACHE_FLUSH_INVALID, addr, length);
}

void mem_dcache_flush_all(void)
{
#ifdef CONFIG_PSRAM_SIZE
	_mem_dcache_ops(SPI_CACHE_FLUSH_INVALID_ALL, (void *)CONFIG_PSRAM_BASE_ADDRESS, CONFIG_PSRAM_SIZE * 1024);
#endif
}

bool mem_is_cacheable(const void *addr)
{
	return buf_is_psram(addr);
}

void mem_writebuf_clean(const void *addr, uint32_t length)
{
	if (buf_is_psram_un(addr)) {
		spi1_cache_ops(SPI_WRITEBUF_FLUSH, (void *)addr, length);
	}
}

void mem_writebuf_clean_all(void)
{
#ifdef CONFIG_PSRAM_SIZE
	mem_writebuf_clean((void *)SPI1_UNCACHE_ADDR, CONFIG_PSRAM_SIZE * 1024);
#endif
}

bool mem_is_bufferable(const void *addr)
{
	return buf_is_psram_un(addr);
}
