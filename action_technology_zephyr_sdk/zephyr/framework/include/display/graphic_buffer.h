/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief graphic buffer interface
 */

#ifndef FRAMEWORK_PORTING_INCLUDE_DISPLAY_GRAPHIC_BUFFER_H_
#define FRAMEWORK_PORTING_INCLUDE_DISPLAY_GRAPHIC_BUFFER_H_

/**
 * @defgroup graphic_buffer_apis Graphic Buffer APIs
 * @ingroup system_apis
 * @{
 */

#include <os_common_api.h>
#include <sys/atomic.h>
#include <sys/util.h>
#include <display/display_hal.h>
#include "ui_region.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @enum graphic_buffer_usage
 * @brief Enumeration with possible graphic buffer usage
 *
 */
enum graphic_buffer_usage {
	GRAPHIC_BUFFER_SW_READ  = BIT(0),
	GRAPHIC_BUFFER_SW_WRITE = BIT(1),
	GRAPHIC_BUFFER_SW_MASK  = GRAPHIC_BUFFER_SW_READ | GRAPHIC_BUFFER_SW_WRITE,

	GRAPHIC_BUFFER_HW_TEXTURE  = BIT(2),
	GRAPHIC_BUFFER_HW_RENDER   = BIT(3),
	GRAPHIC_BUFFER_HW_COMPOSER = BIT(4),
	GRAPHIC_BUFFER_HW_MASK     = GRAPHIC_BUFFER_HW_TEXTURE | GRAPHIC_BUFFER_HW_RENDER | GRAPHIC_BUFFER_HW_COMPOSER,

	GRAPHIC_BUFFER_READ_MASK   = GRAPHIC_BUFFER_SW_READ | GRAPHIC_BUFFER_HW_TEXTURE | GRAPHIC_BUFFER_HW_COMPOSER,
	GRAPHIC_BUFFER_WRITE_MASK  = GRAPHIC_BUFFER_SW_WRITE | GRAPHIC_BUFFER_HW_RENDER,
};

/**
 * @struct graphic_buffer
 * @brief Structure holding graphic buffer
 *
 */
typedef struct graphic_buffer {
	uint8_t *data;          /* data address */
	uint32_t pixel_format;  /* pixel format */
	uint16_t width;         /* width in pixels */
	uint16_t height;        /* height in pixels */
	uint16_t stride;        /* number of pixels per row */
	uint8_t bits_per_pixel; /* bits per pixel */
	uint8_t usage;          /* usage */

	uint8_t lock_usage;     /* lock usage */
	ui_region_t lock_wrect; /* lock write area */

	atomic_t refcount;  /* reference count */

	void (*destroy_cb)(struct graphic_buffer *buffer);
} graphic_buffer_t;

/**
 * @brief Initialize a graphic buffer
 *
 * After initialization, the refcount will be 1.
 * And destroy_cb can be set to free buffer structure and buffer data when
 * buffer destroyed by graphic_buffer_set_destroy_cb().
 *
 * @param buffer pointer to graphic_buffer structure
 * @param w width in pixels
 * @param h height in pixels
 * @param pixel_format display pixel format, see enum display_pixel_format
 * @param buffer usage
 * @param stride number of pixels per row
 * @param data pre-allocated data address
 *
 * @retval 0 on success else negative errno code.
 */
int graphic_buffer_init(graphic_buffer_t *buffer, uint16_t w, uint16_t h,
		uint32_t pixel_format, uint32_t usage, uint16_t stride, void *data);

/**
 * @brief Helper function to set destroy_cb() for a graphic buffer
 *
 * destroy_cb() will be invoked when buffer is destroyed (refcount becomes 0),
 * where user can free the buffer structure and buffer data if necessary.
 *
 * Must be invoked after graphic_buffer_init()
 *
 * @param destroy_cb pointer to graphic_buffer structure
 *
 * @retval N/A
 */
static inline void graphic_buffer_set_destroy_cb(graphic_buffer_t *buffer,
		void (*destroy_cb)(struct graphic_buffer *))
{
	buffer->destroy_cb = destroy_cb;
}

/**
 * @brief Get width of graphic buffer
 *
 * @param buffer pointer to graphic buffer structure
 *
 * @return width in pixels.
 */
static inline uint16_t graphic_buffer_get_width(const graphic_buffer_t *buffer)
{
	return buffer->width;
}

/**
 * @brief Get height of graphic buffer
 *
 * @param buffer pointer to graphic buffer structure
 *
 * @return height in pixels.
 */
static inline uint16_t graphic_buffer_get_height(const graphic_buffer_t *buffer)
{
	return buffer->height;
}

/**
 * @brief Get stride of graphic buffer
 *
 * @param buffer pointer to graphic buffer structure
 *
 * @return stride in pixels.
 */
static inline uint16_t graphic_buffer_get_stride(const graphic_buffer_t *buffer)
{
	return buffer->stride;
}

/**
 * @brief Get bits per pixel of graphic buffer
 *
 * @param buffer pointer to graphic buffer structure
 *
 * @return bits per pixel.
 */
static inline uint16_t graphic_buffer_get_bits_per_pixel(const graphic_buffer_t *buffer)
{
	return buffer->bits_per_pixel;
}

/**
 * @brief Get bytes per line of graphic buffer
 *
 * @param buffer pointer to graphic buffer structure
 *
 * @return bytes per line.
 */
static inline uint16_t graphic_buffer_get_bytes_per_line(const graphic_buffer_t *buffer)
{
	return buffer->stride * buffer->bits_per_pixel / 8;
}

/**
 * @brief Get pixel format of graphic buffer
 *
 * @param buffer pointer to graphic buffer structure
 *
 * @return pixel format.
 */
static inline uint32_t graphic_buffer_get_pixel_format(const graphic_buffer_t *buffer)
{
	return buffer->pixel_format;
}

/**
 * @brief Increase reference of graphic buffer
 *
 * @param buffer pointer to graphic buffer structure
 *
 * @retval 0 on success else negative errno code.
 */
static inline int graphic_buffer_get_refcnt(const graphic_buffer_t *buffer)
{
	return atomic_get(&buffer->refcount);
}

/**
 * @brief Increase reference of graphic buffer
 *
 * @param buffer pointer to graphic buffer structure
 *
 * @retval ref count before the call.
 */
int graphic_buffer_ref(graphic_buffer_t *buffer);

/**
 * @brief Decrease reference of graphic buffer
 *
 * @param buffer pointer to graphic buffer structure
 *
 * @retval ref count before the call.
 */
int graphic_buffer_unref(graphic_buffer_t *buffer);

/**
 * @brief Lock graphic buffer
 *
 * @param buffer pointer to graphic buffer structure
 * @param usage usage, see enum graphic_buffer_usage
 * @param rect the locked area
 * @param vaddr store the address of locked area
 *
 * @return 0 on success else negative errno code.
 */
int graphic_buffer_lock(graphic_buffer_t *buffer, uint32_t usage,
		const ui_region_t *rect, void **vaddr);

/**
 * @brief Unlock graphic buffer
 *
 * @param buffer pointer to graphic buffer structure
 *
 * @retval 0 on success else negative errno code.
 */
int graphic_buffer_unlock(graphic_buffer_t *buffer);

/**
 * @brief Get graphic buffer native buffer ptr at pixel (x, y)
 *
 * This routine don't increases the ref of buffer, and do any cache ops. It mainly
 * cooperates with graphic_buffer_ref(), like for display composer.
 *
 * @param buffer pointer to graphic buffer structure
 * @param x x-coord of pixel
 * @param y y-coord of pixel
 *
 * @return native buffer ptr.
 */
const void *graphic_buffer_get_bufptr(graphic_buffer_t *buffer, int16_t x, int16_t y);

/**
 * @brief Destroy graphic buffer
 *
 * @param buffer pointer to graphic buffer structure
 *
 * @retval N/A.
 */
void graphic_buffer_destroy(graphic_buffer_t *buffer);

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* FRAMEWORK_PORTING_INCLUDE_DISPLAY_GRAPHIC_BUFFER_H_ */
