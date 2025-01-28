/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_CUSTOMER

#include <assert.h>
#include <string.h>
#include <memory/mem_cache.h>
#include <display/display_hal.h>
#include <display/graphic_buffer.h>

LOG_MODULE_REGISTER(graphic_buf, LOG_LEVEL_INF);

#define usage_sw(usage)       (usage & GRAPHIC_BUFFER_SW_MASK)
#define usage_sw_read(usage)  (usage & GRAPHIC_BUFFER_SW_READ)
#define usage_sw_write(usage) (usage & GRAPHIC_BUFFER_SW_WRITE)

#define usage_hw(usage)       (usage & GRAPHIC_BUFFER_HW_MASK)
#define usage_hw_read(usage)  (usage & (GRAPHIC_BUFFER_HW_TEXTURE | GRAPHIC_BUFFER_HW_COMPOSER))
#define usage_hw_write(usage) (usage & GRAPHIC_BUFFER_HW_RENDER)

#define usage_read(usage)     (usage & GRAPHIC_BUFFER_READ_MASK)
#define usage_write(usage)    (usage & GRAPHIC_BUFFER_WRITE_MASK)

int graphic_buffer_init(graphic_buffer_t *buffer, uint16_t w, uint16_t h,
		uint32_t pixel_format, uint32_t usage, uint16_t stride, void *data)
{
	assert(buffer != NULL);

	memset(buffer, 0, sizeof(*buffer));
	buffer->width = w;
	buffer->height = h;
	buffer->stride = stride;
	buffer->bits_per_pixel = display_format_get_bits_per_pixel(pixel_format);
	buffer->pixel_format = pixel_format;
	buffer->data = data;
	buffer->usage = usage;

	atomic_set(&buffer->refcount, 1);
	return 0;
}

int graphic_buffer_ref(graphic_buffer_t *buffer)
{
	assert(buffer != NULL);
	assert(atomic_get(&buffer->refcount) >= 0);

	return atomic_inc(&buffer->refcount);
}

int graphic_buffer_unref(graphic_buffer_t *buffer)
{
	assert(buffer != NULL);
	assert(atomic_get(&buffer->refcount) > 0);

	int refcnt = atomic_dec(&buffer->refcount);
	if (refcnt == 1 && buffer->destroy_cb) {
		buffer->destroy_cb(buffer);
	}

	return refcnt;
}

const void *graphic_buffer_get_bufptr(graphic_buffer_t *buffer, int16_t x, int16_t y)
{
	assert(buffer != NULL && buffer->data != NULL);

	return buffer->data + ((int32_t)y * buffer->stride + x) * buffer->bits_per_pixel / 8;
}

int graphic_buffer_lock(graphic_buffer_t *buffer, uint32_t usage,
		const ui_region_t *rect, void **vaddr)
{
	assert(buffer != NULL);
	assert((usage & buffer->usage) == usage);

	if (usage_sw(usage) && !vaddr) {
		SYS_LOG_ERR("vaddr must provided");
		return -EINVAL;
	}

	if (usage_write(usage) && usage_write(buffer->lock_usage)) {
		SYS_LOG_ERR("multi-write disallowed");
		return -EINVAL;
	}

	if (usage_write(usage)) {
		if (rect) {
			memcpy(&buffer->lock_wrect, rect, sizeof(*rect));
		} else {
			buffer->lock_wrect.x1 = 0;
			buffer->lock_wrect.y1 = 0;
			buffer->lock_wrect.x2 = graphic_buffer_get_width(buffer) - 1;
			buffer->lock_wrect.y2 = graphic_buffer_get_height(buffer) - 1;
		}
	}

	if (vaddr) {
		*vaddr = buffer->data;
		if (rect) {
			*vaddr = (void *)graphic_buffer_get_bufptr(buffer, rect->x1, rect->y1);
		}
	}

	buffer->lock_usage |= usage;

	graphic_buffer_ref(buffer);

	return 0;
}

int graphic_buffer_unlock(graphic_buffer_t *buffer)
{
	bool (*cache_ops)(const void *, uint32_t) = NULL;

	assert(buffer != NULL && buffer->lock_usage > 0);

	if (mem_is_cacheable(buffer->data) == false) {
		goto out_exit;
	}

	if (usage_sw_write(buffer->lock_usage) && usage_hw(buffer->usage)) {
		cache_ops = mem_dcache_clean;
	} else if (usage_hw_write(buffer->lock_usage) && usage_sw(buffer->usage)) {
		cache_ops = mem_dcache_flush;
	}

	if (cache_ops) {
		uint8_t *lock_vaddr = (uint8_t *)graphic_buffer_get_bufptr(buffer,
				buffer->lock_wrect.x1, buffer->lock_wrect.y1);
		uint16_t lock_w = ui_region_get_width(&buffer->lock_wrect);
		uint16_t lock_h = ui_region_get_height(&buffer->lock_wrect);
		uint16_t bytes_per_line = buffer->stride * buffer->bits_per_pixel / 8;
		uint16_t line_len = (lock_w * buffer->bits_per_pixel + 7) / 8;

		if (lock_w == buffer->width) {
			cache_ops(lock_vaddr, lock_h * bytes_per_line);
		} else {
			for (int i = lock_h; i > 0; i--) {
				cache_ops(lock_vaddr, line_len);
				lock_vaddr += bytes_per_line;
			}
		}

		mem_dcache_sync();
	}

out_exit:
	buffer->lock_usage &= ~GRAPHIC_BUFFER_WRITE_MASK;
	graphic_buffer_unref(buffer);
	return 0;
}

void graphic_buffer_destroy(graphic_buffer_t *buffer)
{
	graphic_buffer_unref(buffer);
}
