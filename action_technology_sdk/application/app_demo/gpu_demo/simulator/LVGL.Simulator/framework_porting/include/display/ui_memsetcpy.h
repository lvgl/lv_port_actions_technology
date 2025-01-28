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

#ifdef CONFIG_UI_DMA_MEMSETCPY

/*
 * @brief memset 8-bit pattern
 *
 * @note This function is asynchronous. Call ui_memsetcpy_wait_finish() to synchronize.
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
 *
 * @param buf buf address
 * @param c16 16-bit pattern to set
 * @param n16 number of elements in 16-bit
 *
 * @retval 0 on success else negative code
 */
int ui_memset16(void * buf, uint16_t c16, size_t n16);

/*
 * @brief memset 32-bit pattern
 *
 * @note This function is asynchronous. Call ui_memsetcpy_wait_finish() to synchronize.
 *
 * @param buf buf address
 * @param c32 32-bit pattern to set
 * @param n32 number of elements in 32-bit
 *
 * @retval 0 on success else negative code
 */
int ui_memset32(void * buf, uint32_t c32, size_t n32);

/*
 * @brief memcpy
 *
 * @note This function is asynchronous. Call ui_memsetcpy_wait_finish() to synchronize.
 *
 * @param dest dest address
 * @param src src address
 * @param n number of bytes
 *
 * @retval N/A
 */
void ui_memcpy(void * dest, const void * src, size_t n);

/*
 * @brief wait prevous memset/memcpy finished
 *
 * @param timeout_ms time to wait, set negative value to wait forever.
 *
 * @retval 0 Finished.
 * @retval -EAGAIN Waiting period timed out.
 */
int ui_memsetcpy_wait_finish(int timeout_ms);

#else /* CONFIG_UI_DMA_MEMSETCPY */

static inline void ui_memset(void * buf, uint8_t c, size_t n)
{
	memset(buf, c, n);
}

static inline int ui_memset16(void * buf, uint16_t c16, size_t n16)
{
	return -ENOTSUP;
}

static inline int ui_memset32(void * buf, uint32_t c32, size_t n32)
{
	return -ENOTSUP;
}

static inline void ui_memcpy(void * dest, const void * src, size_t n)
{
	memcpy(dest, src, n);
}

static inline int ui_memsetcpy_wait_finish(int timeout_ms)
{
	return 0;
}

#endif /* CONFIG_UI_DMA_MEMSETCPY */

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* ZEPHYR_FRAMEWORK_INCLUDE_DISPLAY_UI_MEMSETCPY_H_ */
