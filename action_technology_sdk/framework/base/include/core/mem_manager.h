/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file mem manager interface
 */

#ifndef __MEM_MANAGER_H__
#define __MEM_MANAGER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
/**
 * @defgroup mem_manager_apis Mem Manager APIs
 * @ingroup system_apis
 * @{
 */
/**
 * @cond INTERNAL_HIDDEN
 */
/**
 * @brief Allocate memory from system mem heap .
 *
 * This routine provides traditional malloc() semantics. Memory is
 * allocated from the heap memory pool.
 *
 * @param num_bytes Amount of memory requested (in bytes).
 *
 * @return Address of the allocated memory if successful; otherwise NULL.
 */
void *mem_malloc_debug(size_t size,const char *func);
#define mem_malloc(size) mem_malloc_debug(size, __func__)

/**
 * @brief Free memory allocated  system mem heap.
 *
 * This routine provides traditional free() semantics. The memory being
 * returned must have been allocated from the heap memory pool.
 *
 * If @a ptr is NULL, no operation is performed.
 *
 * @param ptr Pointer to previously allocated memory.
 *
 * @return N/A
 */
void mem_free(void *ptr);
/**
 * INTERNAL_HIDDEN @endcond
 */
/**
 * @brief Allocate memory from app mem heap .
 *
 * This routine provides traditional malloc() semantics. Memory is
 * allocated from the heap memory pool.
 *
 * @param num_bytes Amount of memory requested (in bytes).
 *
 * @return Address of the allocated memory if successful; otherwise NULL.
 */
#define app_mem_malloc mem_malloc
//void * app_mem_malloc(unsigned int num_bytes);

/**
 * @brief Allocate memory from app mem heap and mark app tag .
 *
 * This routine provides traditional malloc() semantics. Memory is
 * allocated from the heap memory pool,and mark app tag for memroy
 *
 * @param num_bytes Amount of memory requested (in bytes).
 * @param malloc_tag malloc tag from app.
 *
 * @return Address of the allocated memory if successful; otherwise NULL.
 */
#define app_mem_malloc_ext mem_malloc_ext
//void * app_mem_malloc_ext(unsigned int num_bytes, int malloc_tag);

/**
 * @brief Free memory allocated  app mem heap.
 *
 * This routine provides traditional free() semantics. The memory being
 * returned must have been allocated from the heap memory pool.
 *
 * If @a ptr is NULL, no operation is performed.
 *
 * @param ptr Pointer to previously allocated memory.
 *
 * @return N/A
 */
#define app_mem_free mem_free
//void app_mem_free(void *ptr);


/**
 * @brief dump mem info.
 *
 * This routine provides dump mem info .
 *
 * @return N/A
 */

void mem_manager_set_user_info(int user_info);
int mem_manager_get_user_info(void);

void mem_manager_dump(void);
int mem_manager_check_overwrite(void);
int mem_manager_init(void);

/**
 * @} end defgroup mem_manager_apis
 */

 #ifdef __cplusplus
}
#endif
#endif
