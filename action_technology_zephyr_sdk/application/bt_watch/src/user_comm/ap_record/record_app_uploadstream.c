/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <msg_manager.h>
#include <mem_manager.h>
#include <sys_event.h>
#include <stream.h>
#include <file_stream.h>
#include <user_comm/ap_status.h>

#ifdef CONFIG_BLUETOOTH
#include <bt_manager.h>
#endif
#include "ap_record_private.h"

/** cace info ,used for cache stream */
typedef struct
{
	record_send_pkg_to_stream stream_send_cb;
	record_resource_release release_cb;

} record_upload_info_t;

record_upload_info_t *info = NULL;

// io_stream_t record_upload_stream = NULL;

int record_upload_open(io_stream_t handle,stream_mode mode)
{
	record_upload_info_t *info = (record_upload_info_t *)handle->data;
	if(!info)
		return -EACCES;

	handle->mode = mode;
	return 0;
}

int record_upload_read(io_stream_t handle, unsigned char *buf,int len)
{
	record_upload_info_t *info = (record_upload_info_t *)handle->data;
	int ret = 0;
	if (!info)
		return -EACCES;

	return ret;
}

int record_upload_write(io_stream_t handle, unsigned char *buf,int data_size)
{
	record_upload_info_t *info = (record_upload_info_t *)handle->data;

	if ((!info) || (!info->stream_send_cb)){
		return 0;
	}

	if ((!data_size) /*|| (0 != (data_size%40))*/) {
		return 0;
	}

	if(ap_status_get() != AP_STATUS_RECORDING) {
		return 0;
	}
	//print_hex("d1:",buf,16);
	info->stream_send_cb(buf, data_size, FALSE);

	return data_size;
}

int record_upload_tell(io_stream_t handle)
{
	record_upload_info_t *info = (record_upload_info_t *)handle->data;
	int ret = 80;

	if(!info)
		return -EACCES;
	return ret;
}

int record_upload_get_length(io_stream_t handle)
{
	record_upload_info_t *info = (record_upload_info_t *)handle->data;
	/* it must return A positive number,so we return 160 */
	int ret = 160;

	if (!info)
		return -EACCES;
	return ret;
}

int record_upload_get_space(io_stream_t handle)
{
	record_upload_info_t *info = (record_upload_info_t *)handle->data;
	/* it must return A positive number,so we return 255 */
	int ret = 255;

	if (!info)
		return -EACCES;
	return ret;
}

int record_upload_close(io_stream_t handle)
{
	record_upload_info_t *info = (record_upload_info_t *)handle->data;
	int ret = 0;

	if (!info)
		return -EACCES;

	if (info->release_cb)
		info->release_cb();

	return ret;
}

int record_upload_destroy(io_stream_t handle)
{
	int ret = 0;
	record_upload_info_t *info = (record_upload_info_t *)handle->data;
	if (!info)
		return -EACCES;

	if (info)
		app_mem_free(info);
	return ret;
}

int record_upload_init(io_stream_t handle, void *param)
{
	int ret = 0;
	record_upload_info_t *info = NULL;
	record_stream_init_param *init_param = (record_stream_init_param *)param;

	info = app_mem_malloc(sizeof(record_upload_info_t));
	if (!info) {
		SYS_LOG_ERR("cache stream info malloc failed \n");
		return -ENOMEM;
	}
	
	info->stream_send_cb = init_param->stream_send_cb;
	info->release_cb = init_param->release_cb;
	handle->data = info;
	return ret;
}

const stream_ops_t record_upload_stream_ops = {
	.init = record_upload_init,
	.open = record_upload_open,
	.read = record_upload_read,
	.seek = NULL,
	.tell = record_upload_tell,
 	.get_length = record_upload_get_length,
	.get_space = record_upload_get_space,
	.write = record_upload_write,
	.close = record_upload_close,
	.destroy = record_upload_destroy,
};

io_stream_t record_upload_stream_create(void *param)
{
	return stream_create(&record_upload_stream_ops, param);
}


