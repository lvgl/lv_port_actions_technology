/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_FRAMEWORK_INCLUDE_MEMORY_MEM_CACHE_H_
#define ZEPHYR_FRAMEWORK_INCLUDE_MEMORY_MEM_CACHE_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Invalidate memory dcache
 *
 * @note This function is asynchronous. Call mem_dcache_sync() to synchronize.
 *
 * @param addr memory base address
 * @param length memory length
 *
 * @retval true if ops is asynchronized else false
 */
bool mem_dcache_invalidate(const void *addr, uint32_t length);

/**
 * @brief Invalidate all memory dcache
 *
 * @note This function is asynchronous. Call mem_dcache_sync() to synchronize.
 *
 * @retval true if ops is asynchronized else false
 */
bool mem_dcache_invalidate_all(void);

/**
 * @brief Clean memory dcache
 *
 * If the addr is uncache or write through memory, it will flush the write buffer.
 *
 * @note This function is asynchronous. Call mem_dcache_sync() to synchronize.
 *
 * @param addr memory base address
 * @param length memory length
 *
 * @retval true if ops is asynchronized else false
 */
bool mem_dcache_clean(const void *addr, uint32_t length);

/**
 * @brief Clean all memory dcache and the write buffer
 *
 * @note This function is asynchronous. Call mem_dcache_sync() to synchronize.
 *
 * @retval true if ops is asynchronized else false
 */
bool mem_dcache_clean_all(void);

/**
 * @brief Flush (clean, then invalidate) memory dcache and the write buffer
 *
 * @note This function is asynchronous. Call mem_dcache_sync() to synchronize.
 *
 * @param addr memory base address
 * @param length memory length
 *
 * @retval true if ops is asynchronized else false
 */
bool mem_dcache_flush(const void *addr, uint32_t length);

/**
 * @brief Flush (clean, then invalidate) all memory dcache
 *
 * @note This function is asynchronous. Call mem_dcache_sync() to synchronize.
 *
 * @retval true if ops is asynchronized else false
 */
bool mem_dcache_flush_all(void);

/**
 * @brief Clean the memory write buffer
 *
 * @retval true if ops is asynchronized else false
 */
bool mem_writebuf_clean(const void *addr, uint32_t length);

/**
 * @brief Clean all memory write buffer
 *
 * @param addr memory base address
 * @param length memory length
 *
 * @retval true if ops is asynchronized else false
 */
bool mem_writebuf_clean_all(void);

/**
 * @brief Synchronize the dcache ops, that is, wait until all the dcache ops finished
 *
 * @retval N/A
 */
void mem_dcache_sync(void);

/**
 * @brief translate the cache address to uncache address
 *
 * @param addr memory cache address
 *
 * @retval the uncache address
 */
void * mem_addr_to_uncache(const void * addr);

/**
 * @brief translate the memory address to write-back and cache write-allocate address
 *
 * @retval the write allocate cache address
 */
void * mem_addr_to_cache(const void * addr);

/**
 * @brief translate the memory address to write-through and cache read-allocate address
 *
 * @retval the write through cache address
 */
void * mem_addr_to_cache_wt(const void * addr);

/**
 * @brief Query whether the memory is cacheable
 *
 * @param addr memory base address
 *
 * @retval query result
 */
bool mem_is_cacheable(const void *addr);

/**
 * @brief Query whether the memory is bufferable
 *
 * @param addr memory base address
 *
 * @retval query result
 */
bool mem_is_bufferable(const void *addr);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_FRAMEWORK_INCLUDE_MEMORY_MEM_CACHE_H_ */
