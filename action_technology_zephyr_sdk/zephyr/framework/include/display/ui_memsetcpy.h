/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief UI specific memeset/memcpy
 */

#ifndef ZEPHYR_FRAMEWORK_INCLUDE_DISPLAY_UI_MEMSETCPY_H_
#define ZEPHYR_FRAMEWORK_INCLUDE_DISPLAY_UI_MEMSETCPY_H_

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/**
 * @defgroup display-util Display Utilities
 * @ingroup display_libraries
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * @brief memset 8-bit pattern
 *
 * @note This function is asynchronous. Call ui_memsetcpy_wait_finish() to synchronize.
 *       buf may acccess uncached, cache invalidation on buf may be required.
 *
 * @param buf buf address
 * @param c 8-bit pattern to set
 * @param n number of elements in 8-bit
 *
 * @retval N/A
 */
void ui_memset(void * buf, uint8_t c, size_t n);

/*
 * @brief memset 16-bit pattern
 *
 * @note This function is asynchronous. Call ui_memsetcpy_wait_finish() to synchronize.
 *       buf may acccess uncached, cache invalidation on buf may be required.
 *
 * @param buf buf address
 * @param c16 16-bit pattern to set
 * @param n16 number of elements in 16-bit
 *
 * @retval N/A
 */
void ui_memset16(void * buf, uint16_t c16, size_t n16);

/*
 * @brief memset 24-bit pattern
 *
 * @note This function is asynchronous. Call ui_memsetcpy_wait_finish() to synchronize.
 *       buf may acccess uncached, cache invalidation on buf may be required.
 *
 * @param buf buf address
 * @param c16 24-bit pattern to set
 * @param n24 number of elements in 24-bit
 *
 * @retval N/A
 */
void ui_memset24(void * buf, const uint8_t c24[3], size_t n24);

/*
 * @brief memset 32-bit pattern
 *
 * @note This function is asynchronous. Call ui_memsetcpy_wait_finish() to synchronize.
 *       buf may acccess uncached, cache invalidation on buf may be required.
 *
 * @param buf buf address
 * @param c32 32-bit pattern to set
 * @param n32 number of elements in 32-bit
 *
 * @retval N/A
 */
void ui_memset32(void * buf, uint32_t c32, size_t n32);

/*
 * @brief ui_memset2d
 *
 * @note This function is asynchronous. Call ui_memsetcpy_wait_finish() to synchronize.
 *       dest may acccess uncached, cache invalidation on buf may be required.
 *
 * @param buf buffer address
 * @param stride number of bytes between consecutive rows in buffer
 * @param c value to set
 * @param len number of bytes in each line to clear zero
 * @param lines number of lines
 *
 * @retval N/A
 */
void ui_memset2d(void * buf, uint16_t stride, uint8_t c, uint16_t len, uint16_t lines);

/*
 * @brief ui_memset2d
 *
 * @note This function is asynchronous. Call ui_memsetcpy_wait_finish() to synchronize.
 *       dest may acccess uncached, cache invalidation on buf may be required.
 *
 * @param buf buffer address
 * @param stride number of bytes between consecutive rows in buffer
 * @param c16 16-bit value to set
 * @param len16 number of elements in 16-bit in each line to clear zero
 * @param len number of bytes in each line to clear zero
 * @param lines number of lines
 *
 * @retval N/A
 */
void ui_memset2d_16(void * buf, uint16_t stride, uint16_t c16, uint16_t len16, uint16_t lines);

/*
 * @brief ui_memset2d
 *
 * @note This function is asynchronous. Call ui_memsetcpy_wait_finish() to synchronize.
 *       dest may acccess uncached, cache invalidation on buf may be required.
 *
 * @param buf buffer address
 * @param stride number of bytes between consecutive rows in buffer
 * @param c24 24-bit value to set
 * @param len24 number of elements in 24-bit in each line to clear zero
 * @param lines number of lines
 *
 * @retval N/A
 */
void ui_memset2d_24(void * buf, uint16_t stride, const uint8_t c24[3], uint16_t len24, uint16_t lines);

/*
 * @brief ui_memset2d
 *
 * @note This function is asynchronous. Call ui_memsetcpy_wait_finish() to synchronize.
 *       dest may acccess uncached, cache invalidation on buf may be required.
 *
 * @param buf buffer address
 * @param stride number of bytes between consecutive rows in buffer
 * @param c32 32-bit value to set
 * @param len32 number of elements in 32-bit in each line to clear zero
 * @param lines number of lines
 *
 * @retval N/A
 */
void ui_memset2d_32(void * buf, uint16_t stride, uint32_t c32, uint16_t len32, uint16_t lines);

/*
 * @brief memcpy
 *
 * @note This function is asynchronous. Call ui_memsetcpy_wait_finish() to synchronize.
 *       dest and src may acccess uncached, cache invalidation on buf may be required.
 *
 * @param dest dest address
 * @param src src address
 * @param n number of bytes
 *
 * @retval N/A
 */
void ui_memcpy(void * dest, const void * src, size_t n);

/*
 * @brief ui_memcpy2d
 *
 * @note This function is asynchronous. Call ui_memsetcpy_wait_finish() to synchronize.
 *       dest and src may acccess uncached, cache invalidation on buf may be required.
 *
 * @param dest dest address
 * @param dest_stride number of bytes between consecutive rows in dest buffer
 * @param src src address
 * @param src_stride number of bytes between consecutive rows in src buffer
 * @param len number of bytes in each line to copy
 * @param lines number of lines
 *
 * @retval N/A
 */
void ui_memcpy2d(void * dest, uint16_t dest_stride, const void * src,
				uint16_t src_stride, uint16_t len, uint16_t lines);

/*
 * @brief ui_memcpy_region
 *
 * @note This function is asynchronous. Call ui_memsetcpy_wait_finish() to synchronize.
 *       dest and src may acccess uncached, cache invalidation on buf may be required.
 *
 * @param dest dest address
 * @param src src address
 * @param w number of bytes
 * @param h number of bytes
 *
 * @retval N/A
 */
static inline void ui_memcpy_region(void * dest, const void * src, uint16_t w, uint16_t h)
{
	ui_memcpy2d(dest, w, src, w, w, h);
}

/*
 * @brief wait prevous memset/memcpy finished
 *
 * @param timeout_ms time to wait, set negative value to wait forever.
 *
 * @retval 0 Finished.
 * @retval -EAGAIN Waiting period timed out.
 */
int ui_memsetcpy_wait_finish(int timeout_ms);

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* ZEPHYR_FRAMEWORK_INCLUDE_DISPLAY_UI_MEMSETCPY_H_ */
