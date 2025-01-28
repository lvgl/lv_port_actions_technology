/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file file stream interface
 */
#define SYS_LOG_DOMAIN "filestream"
#include <mem_manager.h>
#include <fs_manager.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "file_stream.h"
#include "stream_internal.h"


/** file info ,used for file stream */
typedef struct {
	/** hanlde of file fp*/
	struct fs_file_t fp;
	/** mutex used for sync*/
	os_mutex lock;

} file_stream_info_t;


int fstream_open(io_stream_t handle, stream_mode mode)
{
	file_stream_info_t *info = (file_stream_info_t *)handle->data;

	assert(info);

	handle->mode = mode;
	handle->write_finished = 0;
	handle->cache_size = 0;
	handle->total_size = 0;

	if (!fs_seek(&info->fp, 0, FS_SEEK_END)) {
		handle->total_size = fs_tell(&info->fp);

		/* seek back to file begin */
		fs_seek(&info->fp, 0, FS_SEEK_SET);
	}

	if ((handle->mode & MODE_IN_OUT) == MODE_OUT) {
		handle->wofs = 0;
	} else {
		handle->wofs = handle->total_size;
	}

	SYS_LOG_INF("handle %p total_size %d mode %x \n",handle, handle->total_size, mode);
	return 0;
}

int fstream_read(io_stream_t handle, unsigned char *buf,int num)
{
	int brw;
	file_stream_info_t *info = (file_stream_info_t *)handle->data;

	assert(info);

	brw = os_mutex_lock(&info->lock, OS_FOREVER);
	if (brw < 0){
		SYS_LOG_ERR("lock failed %d \n",brw);
		return brw;
	}

	if ((handle->mode & MODE_IN_OUT) == MODE_IN_OUT) {
		brw = fs_seek(&info->fp, handle->rofs, FS_SEEK_SET);
		if (brw) {
			SYS_LOG_ERR("seek failed %d\n", brw);
			goto err_out;
		}
	}

	brw = fs_read(&info->fp, buf, num);
	if (brw < 0) {
		SYS_LOG_ERR(" failed %d\n", brw);
		goto err_out;
	}

	handle->rofs += brw;

err_out:
	os_mutex_unlock(&info->lock);
	return brw;
}

int fstream_write(io_stream_t handle, unsigned char *buf,int num)
{
	int brw;

	file_stream_info_t *info = (file_stream_info_t *)handle->data;

	assert(info);

	if ((handle->mode & MODE_IN_OUT) == MODE_IN_OUT) {
		if (num == 0) {
			handle->write_finished = 1;
		#if 0
			if (fs_sync(&info->fp)) {
				SYS_LOG_ERR("sync failed\n");
			}
		#endif
			return num;
		}
	}

	brw = os_mutex_lock(&info->lock, OS_FOREVER);
	if (brw < 0) {
		SYS_LOG_ERR("lock failed %d \n",brw);
		return brw;
	}

	if ((handle->mode & MODE_IN_OUT) == MODE_IN_OUT) {
		brw = fs_seek(&info->fp, handle->wofs, FS_SEEK_SET);
		if (brw) {
			SYS_LOG_ERR("seek failed %d\n", brw);
			goto err_out;
		}
	}

	brw = fs_write(&info->fp, buf, num);
	if (brw < 0) {
		SYS_LOG_ERR("write %d \n", brw);
		goto err_out;
	}

	handle->wofs += brw;
	if (handle->wofs > handle->total_size)
		handle->total_size = handle->wofs;

	if ((handle->mode & MODE_IN_OUT) == MODE_IN_OUT) {
	#if 0
		if (fs_sync(fp)) {
			SYS_LOG_ERR("sync failed \n");
			goto err_out;
		}
	#endif
	}

err_out:
	os_mutex_unlock(&info->lock);
	return brw;
}

int fstream_seek(io_stream_t handle, int offset, seek_dir origin)
{
	int whence = FS_SEEK_SET;
	int brw = 0;

	file_stream_info_t *info = (file_stream_info_t *)handle->data;

	assert(info);

	switch (origin) {
	case SEEK_DIR_CUR:
		if ((handle->mode & MODE_IN_OUT) == MODE_OUT) {
			offset = handle->wofs + offset;
		} else {
			offset = handle->rofs + offset;
		}
		break;
	case SEEK_DIR_END:
		whence = FS_SEEK_END;
		break;
	case SEEK_DIR_BEG:
	default:
		break;
	}

	brw = fs_seek(&info->fp, offset, whence);
	if (brw) {
		SYS_LOG_ERR("seek failed %d\n", brw);
		return -1;
	}

	offset = fs_tell(&info->fp);
	if ((handle->mode & MODE_IN_OUT) == MODE_OUT) {
		handle->wofs = offset;
	} else {
		handle->rofs = offset;
	}

	return 0;
}

int fstream_tell(io_stream_t handle)
{
	file_stream_info_t *info = (file_stream_info_t *)handle->data;

	assert(info);

	return fs_tell(&info->fp);
}

int fstream_flush(io_stream_t handle)
{
	int res;
	file_stream_info_t *info = (file_stream_info_t *)handle->data;

	assert(info);

	res = os_mutex_lock(&info->lock, OS_FOREVER);
	if (res < 0){
		SYS_LOG_ERR("lock failed %d \n",res);
		return res;
	}

	res = fs_sync(&info->fp);

	os_mutex_unlock(&info->lock);
	return res;
}

int fstream_close(io_stream_t handle)
{
	int res;
	file_stream_info_t *info = (file_stream_info_t *)handle->data;

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

int fstream_destroy(io_stream_t handle)
{
	int res;
	file_stream_info_t *info = (file_stream_info_t *)handle->data;

	assert(info);

	res = os_mutex_lock(&info->lock, OS_FOREVER);
	if (res < 0) {
		SYS_LOG_ERR("lock failed %d \n",res);
		return res;
	}

	res = fs_close(&info->fp);
	if (res) {
		SYS_LOG_ERR("close failed %d\n", res);
	}

	os_mutex_unlock(&info->lock);

	mem_free(info);
	return res;
}

int fstream_get_space(io_stream_t handle)
{
	return INT_MAX;
}

static int file_name_has_cluster(char *file_name, char *dir, int max_dir_len, uint32_t *clust, uint32_t *blk_ofs)
{
	char *str = NULL;
	char *cluster = NULL;
	char *blk = NULL;
	char *temp_ptr = NULL;
	int res = 0;

	char *temp_url = mem_malloc(strlen(file_name) + 1);

	if (!temp_url)
		goto exit;

	strcpy(temp_url, file_name);

	str = strstr(temp_url,"bycluster:");
	if (!str)
		goto exit;

	str += strlen("bycluster:");
	temp_ptr = (void *)str;
	/*for dir is /SD:*/
	if (*str == '/')
		str += 1;

	str = strchr(str, '/');
	if (!str)
		goto exit;

	str[0] = 0;
	if(strlen(temp_ptr) > max_dir_len) {
		memcpy(dir, temp_ptr, max_dir_len);
	} else {
		memcpy(dir, temp_ptr, strlen(temp_ptr));
	}

	str++;
	str = strstr(str,"cluster:");
	if (!str)
		goto exit;
	str += strlen("cluster:");
	cluster = str;
	str = strchr(str, '/');
	if (!str)
		goto exit;
	str[0] = 0;

	str++;
	blk = str;
	str = strchr(str, '/');
	if (!str)
		goto exit;
	str[0] = 0;

	*clust = atoi(cluster);
	*blk_ofs = atoi(blk);
	SYS_LOG_DBG("dir=%s,clust=%d,blk_ofs=%d\n", *dir, *clust, *blk_ofs);
	res = 1;
exit:
	if (temp_url)
		mem_free(temp_url);
	return res;
}

int fstream_init(io_stream_t handle, void *param)
{
	int res = 0;
	file_stream_info_t *info = NULL;
	char *file_name = (char *)param;
	char dir[16] = {0};
	uint32_t cluster = 0;
	uint32_t blk_ofs = 0;

	info = mem_malloc(sizeof(file_stream_info_t));
	if (!info) {
		SYS_LOG_ERR("no memory\n");
		return -ENOMEM;
	}

	if (file_name_has_cluster(file_name,  dir, sizeof(dir), &cluster, &blk_ofs)) {
		res = fs_open_cluster(&info->fp, dir, cluster, blk_ofs);
		if (res) {
			SYS_LOG_ERR("open Failed %d\n", res);
			goto failed;
		}
	} else {
		res = fs_open(&info->fp, file_name, FA_READ | FA_WRITE | FA_OPEN_ALWAYS);
		if (res) {
			SYS_LOG_ERR("open Failed %d\n", res);
			goto failed;
		}
	}
	os_mutex_init(&info->lock);

	handle->data = info;
	return res;

failed:
	mem_free(info);
	return res;
}

const stream_ops_t file_stream_ops = {
	.init = fstream_init,
	.open = fstream_open,
	.read = fstream_read,
	.seek = fstream_seek,
	.tell = fstream_tell,
	.write = fstream_write,
	.flush = fstream_flush,
	.get_space = fstream_get_space,
	.close = fstream_close,
	.destroy = fstream_destroy,
};

io_stream_t file_stream_create(const char *param)
{
	return stream_create(&file_stream_ops, (void *)param);
}
