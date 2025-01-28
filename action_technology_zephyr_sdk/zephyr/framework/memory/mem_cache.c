/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <spicache.h>
#include <memory/mem_cache.h>

static K_MUTEX_DEFINE(cache_mutex);

static inline void _mem_dcache_ops_unlocked(uint8_t ops, const void *addr, uint32_t length)
{
	spi1_cache_ops(ops, (void *)addr, length);
}

static void _mem_dcache_ops(uint8_t ops, const void *addr, uint32_t length)
{
	k_mutex_lock(&cache_mutex, K_FOREVER);
	_mem_dcache_ops_unlocked(ops, addr, length);
	k_mutex_unlock(&cache_mutex);
}

bool mem_dcache_invalidate(const void *addr, uint32_t length)
{
	if (buf_is_psram(addr)) {
		_mem_dcache_ops(SPI_CACHE_INVALIDATE, addr, length);
	}

	return false;
}

bool mem_dcache_invalidate_all(void)
{
	_mem_dcache_ops(SPI_CACHE_INVALID_ALL, (void *)SPI1_BASE_ADDR, 0);
	return false;
}

bool mem_dcache_clean(const void *addr, uint32_t length)
{
	bool ret = false;

	k_mutex_lock(&cache_mutex, K_FOREVER);

	if (buf_is_psram_cache(addr)) {
		_mem_dcache_ops_unlocked(SPI_WRITEBUF_FLUSH, addr, length);
		_mem_dcache_ops_unlocked(SPI_CACHE_FLUSH, addr, length);
		ret = true;
	} else if (buf_is_psram_wt_wna(addr) || buf_is_psram_un(addr)) {
		_mem_dcache_ops_unlocked(SPI_WRITEBUF_FLUSH, addr, length);
	}

	k_mutex_unlock(&cache_mutex);

	return ret;
}

bool mem_dcache_clean_all(void)
{
	k_mutex_lock(&cache_mutex, K_FOREVER);
	_mem_dcache_ops_unlocked(SPI_WRITEBUF_FLUSH, (void *)SPI1_UNCACHE_ADDR, 0);
	_mem_dcache_ops_unlocked(SPI_CACHE_FLUSH_ALL, (void *)SPI1_BASE_ADDR, 0);
	k_mutex_unlock(&cache_mutex);

	return true;
}

bool mem_dcache_flush(const void *addr, uint32_t length)
{
	k_mutex_lock(&cache_mutex, K_FOREVER);

	if (buf_is_psram(addr)) {
		_mem_dcache_ops_unlocked(SPI_WRITEBUF_FLUSH, addr, length);
		_mem_dcache_ops_unlocked(SPI_CACHE_FLUSH_INVALID, addr, length);
	} else if (buf_is_psram_un(addr)) {
		_mem_dcache_ops_unlocked(SPI_WRITEBUF_FLUSH, addr, length);
	}

	k_mutex_unlock(&cache_mutex);

	return false;
}

bool mem_dcache_flush_all(void)
{
	k_mutex_lock(&cache_mutex, K_FOREVER);
	_mem_dcache_ops_unlocked(SPI_WRITEBUF_FLUSH, (void *)SPI1_UNCACHE_ADDR, 0);
	_mem_dcache_ops_unlocked(SPI_CACHE_FLUSH_INVALID_ALL, (void *)SPI1_BASE_ADDR, 0);
	k_mutex_unlock(&cache_mutex);

	return true;
}

bool mem_writebuf_clean(const void *addr, uint32_t length)
{
	if (buf_is_psram(addr) || buf_is_psram_un(addr)) {
		_mem_dcache_ops(SPI_WRITEBUF_FLUSH, addr, length);
	}

	return false;
}

bool mem_writebuf_clean_all(void)
{
	_mem_dcache_ops(SPI_WRITEBUF_FLUSH, (void *)SPI1_UNCACHE_ADDR, 0);
	return false;
}

void mem_dcache_sync(void)
{
	k_mutex_lock(&cache_mutex, K_FOREVER);
	spi1_cache_ops_wait_finshed();
	k_mutex_unlock(&cache_mutex);
}

void * mem_addr_to_uncache(const void * addr)
{
	if (buf_is_psram(addr)) {
		return cache_to_uncache((void *)addr);
	} else {
		return (void *)addr;
	}
}

void * mem_addr_to_cache(const void * addr)
{
	if (buf_is_psram_wt_wna(addr)) {
		return wt_wna_cache_to_cache((void *)addr);
	} else if (buf_is_psram_un(addr)) {
		return uncache_to_cache((void *)addr);
	} else {
		return (void *)addr;
	}
}

void * mem_addr_to_cache_wt(const void * addr)
{
	if (buf_is_psram_cache(addr)) {
		return cache_to_wt_wna_cache((void *)addr);
	} else if (buf_is_psram_un(addr)) {
		return uncache_to_wt_wna_cache((void *)addr);
	} else {
		return (void *)addr;
	}
}

bool mem_is_cacheable(const void *addr)
{
	return buf_is_psram(addr);
}

bool mem_is_bufferable(const void *addr)
{
	return buf_is_psram_un(addr) || buf_is_psram_wt_wna(addr);
}
