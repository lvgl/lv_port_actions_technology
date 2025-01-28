/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file ui memory interface 
 */

#ifndef FRAMEWORK_DISPLAY_INCLUDE_RES_MEMPOOL_H_
#define FRAMEWORK_DISPLAY_INCLUDE_RES_MEMPOOL_H_

/**
 * @defgroup view_cache_apis View Cache APIs
 * @ingroup system_apis
 * @{
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the res manager pool memory
 *
 * @retval 0 on success else negative code.
 */
void res_mem_init(void);

/**
 * @brief Alloc res buffer memory
 *
 * @param type mempool type
 * @param size allocation size in bytes
 *
 * @retval pointer to the allocation memory.
 */
void *res_mem_alloc_debug(uint32_t type, size_t size,const char *func);
#define res_mem_alloc(type, size) res_mem_alloc_debug(type, size, __func__)

/**
 * @brief Free res buffer memory
 *
 * @param type mempool type 
 * @param ptr pointer to the allocated memory
 *
 * @retval N/A
 */
void res_mem_free(uint32_t type, void *ptr);

/**
 * @brief Dump res manger memory allocation detail.
 *
 *
 * @retval N/A
 */
void res_mem_dump(void);

/**
 * @brief Alloc fixed size res buffer memory
 *
 * @param type mem type
 * @param size allocation size in bytes
 *
 * @retval pointer to the allocation memory.
 */
void* res_array_alloc_debug(int32_t type, size_t size, const char *func);
#define res_array_alloc(type, size) res_array_alloc_debug(type, size,__func__)

/**
 * @brief Free fixed size res buffer memory
 *
 * @param type mem type 
 * @param ptr pointer to the allocated memory
 *
 * @retval 1 on success else ptr is not found
 */
uint32_t res_array_free(void* ptr);

/**
 * @brief check if auto searching enabled
 *
 *
 * @retval N/A
 */
int res_is_auto_search_files(void);

int res_debug_load_bitmap_is_on(void);

int res_mem_get_max_resource_sets(void);

uint32_t res_mem_get_max_compact_block_size(void);


void * res_mem_aligned_alloc_debug(uint8_t type, size_t align, size_t size, const void* caller);
#define res_mem_aligned_alloc(type, align, size) res_mem_aligned_alloc_debug(type, align, size, __func__)

void* res_mem_alloc_block(size_t size, const char* func);
void res_mem_free_block(void* ptr);

size_t res_mem_get_align(void);


#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* FRAMEWORK_DISPLAY_INCLUDE_RES_MEMPOOL_H_ */


