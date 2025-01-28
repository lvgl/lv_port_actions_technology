/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file buffer stream interface
 */
#define SYS_LOG_DOMAIN "bufferstream"
#include <mem_manager.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "buffer_stream.h"
#include "stream_internal.h"

/** buffer info ,used by buffer stream */
typedef struct 
{
	/** len of buffer*/
	int length;
	/** pointer to base of buffer */
	char * buffer_base;
	/** mutex used for sync*/
	os_mutex lock;
}buffer_info_t;

int buffer_stream_open(io_stream_t handle, stream_mode mode)
{
	handle->mode = mode;
	SYS_LOG_INF("mode %d \n",mode);
	return 0;
}

int buffer_stream_read(io_stream_t handle, unsigned char * buf, int num)
{
	int brw = 0;
	int read_len = 0;
	int file_off = 0;
	buffer_info_t *info = (buffer_info_t *)handle->data;

	assert(info);

	brw = os_mutex_lock(&info->lock, OS_FOREVER);
	if (brw < 0) {
		SYS_LOG_ERR("lock failed %d \n",brw);
		return -brw;
	}
	if(handle->state != STATE_OPEN) {
		goto err_out;
	}

	if ((handle->mode & MODE_IN_OUT) == MODE_IN_OUT) {
		file_off = handle->rofs % info->length;

		if (file_off + num > info->length) {
			brw = info->length - file_off;
			memcpy(buf, info->buffer_base + file_off, brw);
			num -= brw;
			handle->rofs += brw;
			read_len += brw;
		}

		file_off = handle->rofs % info->length;

		memcpy((char *)(buf + read_len),
				info->buffer_base + file_off, num);

		handle->rofs += num;
		read_len += num;

	} else {
		if (handle->rofs  +  num > info->length) {
			read_len = info->length - handle->rofs;
		} else {
			read_len = num;
		}

		memcpy(buf, info->buffer_base + handle->rofs, read_len);

		handle->rofs += read_len;
	}

err_out:
	os_mutex_unlock(&info->lock);
	return read_len;
}

int buffer_stream_seek(io_stream_t handle, int offset,seek_dir origin)
{
	if(handle->state != STATE_OPEN) {
		SYS_LOG_ERR("check handle failed \n");
		return -EPERM;
	}

	handle->rofs = offset;
	return 0;
}

int buffer_stream_tell(io_stream_t handle)
{
	return handle->rofs;
}

int buffer_stream_get_length(io_stream_t handle)
{
    if ((handle->mode & MODE_IN_OUT) == MODE_IN_OUT) {
		return handle->wofs - handle->rofs;
	} else {
		return handle->total_size - handle->rofs;
	}
}

int buffer_stream_write(io_stream_t handle, unsigned char * buf, int num)
{
	int brw = 0;
	int wirte_len = 0;
	int file_off = 0;
	buffer_info_t *info = (buffer_info_t *)handle->data;

	assert(info);

	brw = os_mutex_lock(&info->lock, OS_FOREVER);
	if (brw < 0) {
		SYS_LOG_ERR("lock failed %d \n",brw);
		return -brw;
	}

	file_off = handle->wofs % info->length;

	if (file_off + num > info->length) {
		brw = info->length - file_off;
		memcpy(info->buffer_base + file_off , buf, brw);
		num -= brw;
		handle->wofs += brw;
		wirte_len += brw;
	}

	file_off = handle->wofs % info->length;

	memcpy(info->buffer_base + file_off,
			buf + wirte_len , num);

	handle->wofs += num;
	wirte_len += num;

	os_mutex_unlock(&info->lock);
	return wirte_len;
}

int buffer_stream_close(io_stream_t handle)
{
	int res;

	buffer_info_t *info = (buffer_info_t *)handle->data;

	assert(info);

	res = os_mutex_lock(&info->lock, OS_FOREVER);
	if (res < 0) {
		SYS_LOG_ERR("lock failed %d \n",res);
		return res;
	}

	handle->wofs = 0;
	handle->rofs = 0;
	handle->state = STATE_CLOSE;

	os_mutex_unlock(&info->lock);
	return res;

}

int buffer_stream_destory(io_stream_t handle)
{
	buffer_info_t *info = (buffer_info_t *)handle->data;

	assert(info);

	mem_free(info);
	handle->data = NULL;

	return 0;
}

int buffer_stream_init(io_stream_t handle, void *param)
{
	buffer_info_t *info = NULL;
	struct buffer_t *buffer = (struct buffer_t *)param;

	info = mem_malloc(sizeof(buffer_info_t));
	if (!info) {
		SYS_LOG_ERR(" malloc failed \n");
		return -ENOMEM;
	}

	info->buffer_base = buffer->base;
	info->length = buffer->length;

	os_mutex_init(&info->lock);

	handle->data = info;
	handle->cache_size = buffer->cache_size;
	handle->total_size = buffer->length;
	return 0;
}

const stream_ops_t buffer_stream_ops = {
	.init = buffer_stream_init,
	.open = buffer_stream_open,
    .read = buffer_stream_read,
    .seek = buffer_stream_seek,
    .tell = buffer_stream_tell,
    .get_length = buffer_stream_get_length,
    .write = buffer_stream_write,
    .close = buffer_stream_close,
	.destroy = buffer_stream_destory,
};

io_stream_t buffer_stream_create(struct buffer_t *param)
{
	return stream_create(&buffer_stream_ops, param);

}