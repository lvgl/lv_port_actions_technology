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

    multi_file_info_t info;
    uint32_t *end_rofs_list;
    int cur_file_index;
} mfile_stream_info_t;

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

static int _open_file(mfile_stream_info_t *info, multi_file_info_t *file_info, int index)
{
	char dir[16] = {0};
	uint32_t cluster = 0;
	uint32_t blk_ofs = 0;
    int ret;
    char sbuf[25] = {0};
    
    file_info->get_file_name(file_info->usr_data, index, sbuf, sizeof(sbuf));

	if (file_name_has_cluster(sbuf, dir, sizeof(dir), &cluster, &blk_ofs)) {
		ret = fs_open_cluster(&info->fp, dir, cluster, blk_ofs);
		if (ret) {
			SYS_LOG_ERR("open Failed %d\n", ret);
			return ret;
		}
	} else {
		ret = fs_open(&info->fp, sbuf, FA_READ);
		if (ret) {
			SYS_LOG_ERR("open Failed %d\n", ret);
			return ret;
		}
	}

    return 0;
}

static int mfstream_open(io_stream_t handle, stream_mode mode)
{
	mfile_stream_info_t *info = (mfile_stream_info_t *)handle->data;
    multi_file_info_t *file_info = &info->info;
    int i;
    int ret;

	assert(info);

	handle->mode = mode;
	handle->write_finished = 0;
	handle->cache_size = 0;
	handle->total_size = 0;

    for (i=0; i<file_info->file_count; i++) {
        ret = _open_file(info, file_info, i);
        if(ret) {
            goto failed;
        }

        if (!fs_seek(&info->fp, 0, FS_SEEK_END)) {
            handle->total_size += fs_tell(&info->fp);
            info->end_rofs_list[i] = handle->total_size;
    	}

        fs_close(&info->fp);
    }

	handle->wofs = handle->total_size;
    _open_file(info, file_info, 0);
    
	SYS_LOG_INF("handle %p total_size %d mode %x \n",handle, handle->total_size, mode);
	return 0;

failed:
    return ret;
}

static int mfstream_read(io_stream_t handle, unsigned char *buf, int num)
{
	int brw;
	mfile_stream_info_t *info = (mfile_stream_info_t *)handle->data;
    multi_file_info_t *file_info = &info->info;
    int read_num;
    uint32_t rofs = handle->rofs;

	assert(info);

	brw = os_mutex_lock(&info->lock, OS_FOREVER);
	if (brw < 0){
		SYS_LOG_ERR("lock failed %d \n",brw);
		return brw;
	}

    if ((handle->rofs + num) <= info->end_rofs_list[info->cur_file_index]) {
        read_num = num;
    } else if (handle->rofs < info->end_rofs_list[info->cur_file_index]) {
        read_num = info->end_rofs_list[info->cur_file_index] - handle->rofs;
    } else {
        read_num = 0;
    }
    
    if (info->cur_file_index >= file_info->file_count) {
        goto err_out;
    }

    if(read_num) {
        brw = fs_read(&info->fp, buf, read_num);
        if (brw < 0) {
            SYS_LOG_ERR(" failed %d\n", brw);
            goto err_out;
        }
        
        handle->rofs += brw;
    }

    if ((read_num < num) && (info->cur_file_index < (file_info->file_count - 1))) {
        info->cur_file_index++;
        fs_close(&info->fp);
        _open_file(info, file_info, info->cur_file_index);

        brw = fs_read(&info->fp, (buf + read_num), (num - read_num));
        if (brw < 0) {
            SYS_LOG_ERR(" failed %d\n", brw);
            goto err_out;
        }

        handle->rofs += brw;
    }

    os_mutex_unlock(&info->lock);
    return handle->rofs - rofs;

err_out:
	os_mutex_unlock(&info->lock);
	return brw;
}

static int mfstream_seek(io_stream_t handle, int offset, seek_dir origin)
{
	mfile_stream_info_t *info = (mfile_stream_info_t *)handle->data;
    multi_file_info_t *file_info = &info->info;
    int offset_local = 0;
	int brw = 0;
    uint8_t fp_index = 0;

	assert(info);

	switch (origin) {
	case SEEK_DIR_CUR:
		offset_local = handle->rofs + offset;
		break;
	case SEEK_DIR_END:
        offset_local = handle->total_size + offset;
		break;
	case SEEK_DIR_BEG:
        offset_local = offset;
        break;
	default:
		break;
	}
    
    for (fp_index = 0; fp_index < file_info->file_count; fp_index++) {
        if (offset_local <= info->end_rofs_list[fp_index]) {
            if (fp_index) {
                offset_local -= info->end_rofs_list[fp_index - 1];
            }
            break;
        }
    }

    if (fp_index >= file_info->file_count) {
        return -1;
    }

    if(fp_index != info->cur_file_index) {
        info->cur_file_index = fp_index;
        fs_close(&info->fp);
        _open_file(info, file_info, info->cur_file_index);
    }

	brw = fs_seek(&info->fp, offset_local, SEEK_DIR_BEG);
	if (brw) {
		SYS_LOG_ERR("seek failed %d\n", brw);
		return -1;
	}

	offset_local = fs_tell(&info->fp);
    if (fp_index) {
        offset_local += info->end_rofs_list[fp_index - 1];
    }
    
	handle->rofs = offset_local;
	return 0;
}

static int mfstream_tell(io_stream_t handle)
{
    return handle->rofs;
}

static int mfstream_close(io_stream_t handle)
{
	int res;
	mfile_stream_info_t *info = (mfile_stream_info_t *)handle->data;

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

static int mfstream_destroy(io_stream_t handle)
{
	int res;
	mfile_stream_info_t *info = (mfile_stream_info_t *)handle->data;

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

static int mfstream_init(io_stream_t handle, void *param)
{
	mfile_stream_info_t *info;
    multi_file_info_t *file_info = (multi_file_info_t*)param;

	info = mem_malloc(sizeof(mfile_stream_info_t) + file_info->file_count * sizeof(uint32_t));
	if (!info) {
		SYS_LOG_ERR("no memory\n");
		return -ENOMEM;
	}

    info->end_rofs_list = (uint32_t*)(info + 1);
    memcpy(&info->info, file_info, sizeof(multi_file_info_t));
    os_mutex_init(&info->lock);
	handle->data = info;
	return 0;
}

static const stream_ops_t file_stream_ops = {
	.init = mfstream_init,
	.open = mfstream_open,
	.read = mfstream_read,
	.seek = mfstream_seek,
	.tell = mfstream_tell,
	.write = NULL,
	.flush = NULL,
	.get_space = NULL,
	.close = mfstream_close,
	.destroy = mfstream_destroy,
};

io_stream_t multi_file_stream_create(multi_file_info_t *param)
{
	return stream_create(&file_stream_ops, (void *)param);
}

