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
 * @retval N/A
 */
void mem_dcache_invalidate(const void *addr, uint32_t length);

/**
 * @brief Invalidate all memory dcache
 *
 * @note This function is asynchronous. Call mem_dcache_sync() to synchronize.
 *
 * @retval N/A
 */
void mem_dcache_invalidate_all(void);

/**
 * @brief Clean memory dcache
 *
 * @note This function is asynchronous. Call mem_dcache_sync() to synchronize.
 *
 * @param addr memory base address
 * @param length memory length
 *
 * @retval N/A
 */
void mem_dcache_clean(const void *addr, uint32_t length);

/**
 * @brief Clean all memory dcache
 *
 * @note This function is asynchronous. Call mem_dcache_sync() to synchronize.
 *
 * @retval N/A
 */
void mem_dcache_clean_all(void);

/**
 * @brief Flush (clean, then invalidate) memory dcache
 *
 * @note This function is asynchronous. Call mem_dcache_sync() to synchronize.
 *
 * @param addr memory base address
 * @param length memory length
 *
 * @retval N/A
 */
void mem_dcache_flush(const void *addr, uint32_t length);

/**
 * @brief Flush (clean, then invalidate) all memory dcache
 *
 * @note This function is asynchronous. Call mem_dcache_sync() to synchronize.
 *
 * @retval N/A
 */
void mem_dcache_flush_all(void);

/**
 * @brief Synchronize the dcache ops, that is, wait until all the dcache ops finished
 *
 * @retval N/A
 */
void mem_dcache_sync(void);

/**
 * @brief Synchronize the dcache ops, that is, wait until all the dcache ops finished
 *
 * @retval N/A
 */
void * mem_addr_to_uncache(void * addr);

/**
 * @brief Query whether any part of memory is cacheable
 *
 * @param addr memory base address
 * @param length memory length
 *
 * @retval query result
 */
bool mem_is_cacheable(const void *addr);

/**
 * @brief Clean the memory write buffer
 *
 * @retval N/A
 */
void mem_writebuf_clean(const void *addr, uint32_t length);

/**
 * @brief Clean all memory write buffer
 *
 * @param addr memory base address
 * @param length memory length
 *
 * @retval N/A
 */
void mem_writebuf_clean_all(void);

/**
 * @brief Query whether any part of memory is bufferable
 *
 * @param addr memory base address
 * @param length memory length
 *
 * @retval query result
 */
bool mem_is_bufferable(const void *addr);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_FRAMEWORK_INCLUDE_MEMORY_MEM_CACHE_H_ */
