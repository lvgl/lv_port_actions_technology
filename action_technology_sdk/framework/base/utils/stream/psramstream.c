/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file psram stream interface
 */

#include <mem_manager.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "psram_stream.h"
#include "stream_internal.h"

/** cace info ,used for cache stream */
typedef struct
{
	/** hanlde of file fp*/
	int base_off;
	/** size of cache file */
	int size ;
	/** mutex used for sync*/
	os_mutex lock;
} cache_info_t;

int psram_stream_open(io_stream_t handle,stream_mode mode)
{
	handle->mode = mode;
	SYS_LOG_DBG("mode %d \n",mode);
	return 0;
}

int psram_stream_read(io_stream_t handle, unsigned char *buf,int num)
{
	int brw;
	int read_len = 0;
	int file_off = 0;
	cache_info_t *info = (cache_info_t *)handle->data;

	assert(info);

	brw = os_mutex_lock(&info->lock, OS_FOREVER);
	if (brw < 0) {
		SYS_LOG_ERR("lock failed %d \n",res);
		return brw;
	}

	file_off = handle->rofs % info->size;

	if (file_off + num > info->size)	{
		brw = info->size - file_off;
		psram_read(info->base_off + file_off, buf, brw);
		num -= brw;
		handle->rofs += brw;
		read_len += brw;
		file_off = handle->rofs % info->size;
	}

	psram_read(info->base_off + file_off,
					buf + read_len , num);

	handle->rofs += num;
	read_len += num;

err_out:
	os_mutex_unlock(&info->lock);
	return read_len;
}

int psram_stream_write(io_stream_t handle, unsigned char *buf,int num)
{
	uint32_t brw = 0;
	int wirte_len = 0;
	int file_off = 0;

	cache_info_t *info = (cache_info_t *)handle->data;

	assert(info);

	if (os_mutex_lock(&info->lock, OS_FOREVER) < 0) {
		SYS_LOG_ERR("lock failed %d \n",res);
		return -1;
	}

	file_off = handle->wofs % info->size;

	if (file_off + num > info->size) {
		brw = info->size - file_off;
		psram_write(info->base_off + file_off , buf, brw);

		num -= brw;
		handle->wofs += brw;
		wirte_len += brw;
		file_off = handle->wofs % info->size ;
	}

	psram_write(info->base_off + file_off,
			buf + wirte_len , num);

	handle->wofs += num;
	wirte_len += num;

err_out:
	os_mutex_unlock(&info->lock);
	return wirte_len;
}

int psram_stream_seek(io_stream_t handle, int offset, seek_dir origin)
{
	SYS_LOG_DBG("handle->rofs %d ,handle->wofs %d offset %d target_off %d \n",
							handle->rofs ,handle->wofs, offset, offset);

	handle->rofs = offset;
	return 0;
}

int psram_stream_tell(io_stream_t handle)
{
	int res;

	cache_info_t *info = (cache_info_t *)handle->data;

	assert(info);

	res = os_mutex_lock(&info->lock, OS_FOREVER);
	if (res < 0) {
		SYS_LOG_ERR("lock failed %d \n",res);
		res = res;
		goto exit;
	}

	res = handle->rofs;

exit:
	os_mutex_unlock(&info->lock);

	return res ;
}

int psram_stream_close(io_stream_t handle)
{
	int res;

	cache_info_t *info = (cache_info_t *)handle->data;

	assert(info);

	res = os_mutex_lock(&info->lock, OS_FOREVER);
	if (res < 0) {
		SYS_LOG_ERR("lock failed %d \n",res);
		return res;
	}

	handle->state = STATE_CLOSE;

exit:
	os_mutex_unlock(&info->lock);
	return res;
}

int psram_stream_destroy(io_stream_t handle)
{
	int res;
	cache_info_t *info = (cache_info_t *)handle->data;

	assert(info);

	res = os_mutex_lock(&info->lock, OS_FOREVER);
	if (res < 0) {
		SYS_LOG_ERR("lock failed %d \n",res);
		return res;
	}

	psram_free(info->base_off);

	os_mutex_unlock(&info->lock);

	mem_free(info);
	handle->data = NULL;

	return res;
}

int psram_stream_init(io_stream_t handle, void *param)
{
	cache_info_t *info = NULL;
	struct cache_t *cache = (struct cache_t *)param;

	info = mem_malloc(sizeof(cache_info_t));
	if (!info) {
		SYS_LOG_ERR("malloc failed \n");
		return -ENOMEM;
	}

	info->base_off = psram_alloc(cache->size);
	if(info->base_off < 0)
	{
		mem_free(info);
		return -ENOMEM;
	}
	info->size = cache->size;	

	os_mutex_init(&info->lock);
	os_sem_init(&info->data_sem, 0, 1);
	os_sem_init(&info->can_write_sem, 0, 1);

	SYS_LOG_INF("size:%d, name: %s base_off: 0x%x\n",
				cache->size,cache->name,info->base_off);

	handle->cache_size = cache->min_size;
	handle->write_pending = cache->write_pending;
	handle->total_size = cache->size;
	handle->data = info;

	return 0;
}

const stream_ops_t psram_stream_ops = {
	.init = psram_stream_init,
	.open = psram_stream_open,
    .read = psram_stream_read,
    .seek = psram_stream_seek,
    .tell = psram_stream_tell,
    .write = psram_stream_write,
    .close = psram_stream_close,
	.destroy = psram_stream_destroy,
};

io_stream_t psram_stream_create(struct psram_info_t *param)
{
	return stream_create(&psram_stream_ops, param);

}