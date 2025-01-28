/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file ui memory interface
 */

#ifndef FRAMEWORK_DISPLAY_INCLUDE_UI_MEM_H_
#define FRAMEWORK_DISPLAY_INCLUDE_UI_MEM_H_

/**
 * @defgroup view_cache_apis View Cache APIs
 * @ingroup system_apis
 * @{
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief UI Memory Types
 *
 * UI memory type enumeration
 */
enum {
	MEM_FB = 0,
	MEM_GUI,
	MEM_RES,

	NUM_UI_MEM_TYPES,
};

/**
 * @brief Initialize the UI memory
 *
 * @retval 0 on success else negative code.
 */
int ui_mem_init(void);

/**
 * @brief Alloc UI memory
 *
 * @param type UI memory type
 * @param size Amount of memory requested (in bytes).
 * @param caller Caller address for debug; can be NULL
 *
 * @return Address of the allocated memory if successful; otherwise NULL.
 */
void * ui_mem_alloc(uint8_t type, size_t size, const void * caller);

/**
 * @brief Alloc UI memory with a specified alignment.
 *
 * @param type UI memory type
 * @param align Alignment of memory requested (in bytes), must be a power of two
 * @param size Amount of memory requested (in bytes).
 * @param caller Caller address for debug; can be NULL
 *
 * @return Address of the allocated memory if successful; otherwise NULL.
 */
void * ui_mem_aligned_alloc(uint8_t type, size_t align, size_t size, const void * caller);

/**
 * @brief Realloc UI memory
 *
 * @param type UI memory type
 * @param ptr Pointer to original memory
 * @param size Amount of memory requested (in bytes).
 * @param caller Caller address for debug; can be NULL
 *
 * @return Address of the allocated memory if successful; otherwise NULL.
 */
void * ui_mem_realloc(uint8_t type, void * ptr, size_t size, const void * caller);

/**
 * @brief Calloc UI memory
 *
 * @param type UI memory type
 * @param nmemb number of members
 * @param size size of member (in bytes)
 * @param caller Caller address for debug; can be NULL
 *
 * @return Address of the allocated memory if successful; otherwise NULL.
 */
void * ui_mem_calloc(uint8_t type, size_t nmemb, size_t size, const void * caller);

/**
 * @brief Free UI memory
 *
 * If @a ptr is NULL, no operation is performed.
 *
 * @param type UI memory type
 * @param ptr Pointer to previously allocated memory.
 *
 * @retval N/A
 */
void ui_mem_free(uint8_t type, void *ptr);

/**
 * @brief Free UI memory
 *
 * If @a ptr is NULL, no operation is performed.
 *
 * @param ptr Pointer to previously allocated memory.
 *
 * @retval N/A
 */
void ui_mem_free2(void * ptr);

/**
 * @brief Query if memory is allocated from a specific pool.
 *
 * @param type UI memory type
 * @param ptr Pointer to an allocated memory
 *
 * @retval true if of the specific type else false
 */
bool ui_mem_is_type(uint8_t type, const void * ptr);

/**
 * @brief Dump UI memory allocation detail.
 *
 * @param type UI memory type
 *
 * @retval N/A
 */
void ui_mem_dump(uint8_t type);

/**
 * @brief Dump All UI memory allocation detail.
 *
 * @param type UI memory type
 *
 * @retval N/A
 */
void ui_mem_dump_all(void);

/**
 * @brief ui memory safe check
 *
 * @param view_id id of view witch want to check
 *
 * @retval N/A
 */
void ui_mem_safe_check(uint16_t view_id);

/**
 * @cond INTERNAL_HIDDEN
 */

/**
 * @brief get number of fb
 *
 * @retval number of fb
 */
size_t ui_mem_fb_get_num(void);

/**
 * @brief get buffer shared by GUI draw and surface effects
 *
 * @retval the shared buffer pointer
 */
void *ui_mem_get_share_surface_buffer(void);

/**
 * @brief get size of buffer shared by GUI draw and surface effects
 *
 * @retval the shared buffer size
 */
int ui_mem_get_share_surface_buffer_size(void);

/**
 * @brief set share buffer paint flag
 *
 * @param painted paint flag
 *
 * @retval the previous paint flag
 */
bool ui_mem_set_share_surface_buffer_painted(bool painted);

void * ui_mem_gui_alloc(size_t size);
void * ui_mem_gui_aligned_alloc(size_t align, size_t size);
void * ui_mem_gui_realloc(void * ptr, size_t size);
void ui_mem_gui_free(void * ptr);

#if defined(CONFIG_UI_RES_MEM_POOL_SIZE) && CONFIG_UI_RES_MEM_POOL_SIZE > 0
void * ui_mem_res_alloc(size_t size);
void * ui_mem_res_aligned_alloc(size_t align, size_t size);
void ui_mem_res_free(void * ptr);
#else
#define ui_mem_res_alloc ui_mem_gui_alloc
#define ui_mem_res_aligned_alloc ui_mem_gui_aligned_alloc
#define ui_mem_res_free ui_mem_gui_free
#endif

/**
 * INTERNAL_HIDDEN @endcond
 */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* FRAMEWORK_DISPLAY_INCLUDE_UI_MEM_H_ */
