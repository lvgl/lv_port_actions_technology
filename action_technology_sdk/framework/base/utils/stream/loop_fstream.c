/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file file stream interface
 */
#define SYS_LOG_DOMAIN "loopstream"
#include <mem_manager.h>
#include <fs_manager.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "loop_fstream.h"
#include "stream_internal.h"


#define ENERGY_FILTER_TIME	(500)	//uint ms

typedef struct {
	/** hanlde of file fp*/
	struct fs_file_t fp;
	/** mutex used for sync*/
	os_mutex lock;
} loop_fstream_info_t;

/* loop file info structure */
struct loop_file_info {
	uint32_t flen;			//actual file length
	uint32_t flen_head;	//length of file head(tag id3v2)
	uint32_t cur_rofs;		//a virtual value to store current offset

	uint32_t flen_energy_clac;	//the data that need calc energy
	uint32_t flen_notail;	//file length without file tail(tag id3v1 and apev2)
	uint32_t flen_data;	//file length without file head and tail
	uint32_t flen_remain;	//the rest music data for next decode(a fake value)
	loopfs_energy_cb energy_filt_enable;	//callback func to enable energy filter
};

static struct loop_file_info lp;


/*eg. file_name: bycluster:SD:LOOP/cluster:2/320/12992420*/
static bool get_cluster_by_name(char *file_name, char **dir, uint32_t *clust, uint32_t *blk_ofs)
{
	char *str = NULL;
	char *cluster = NULL;
	char *blk = NULL;
	bool res = false;

	char *temp_name = mem_malloc(strlen(file_name) + 1);
	if (!temp_name)
		goto exit;

	strcpy(temp_name, file_name);

	str = strstr(temp_name, "bycluster:");
	if (!str)
		goto exit;

	str += strlen("bycluster:");
	*dir = (void *)str;
	/*for dir is /SD:*/
	if (*str == '/')
		str += 1;

	str = strchr(str, '/');
	if (!str)
		goto exit;

	str[0] = 0;
	str++;
	str = strstr(str, "cluster:");
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

	res = true;
exit:
	if (temp_name)
		mem_free(temp_name);
	return res;
}

static void loop_fstream_clac_info(uint32_t head, uint32_t tail)
{
	/* flen and flen_remain have be initialized */
	lp.flen_head = head;
	lp.flen_notail = lp.flen - tail;
	lp.flen_data = lp.flen_notail - lp.flen_head;
	lp.flen_remain -= lp.flen_head + tail;
}

int loop_fstream_init(io_stream_t handle, void *param)
{
	int res = 0;
	loop_fstream_info_t *info = NULL;
	char *file_name = (char *)param;
	char *dir = NULL;
	uint32_t cluster = 0, blk_ofs = 0;

	info = mem_malloc(sizeof(loop_fstream_info_t));
	if (!info) {
		SYS_LOG_ERR("no memory\n");
		return -ENOMEM;
	}

	if (get_cluster_by_name(file_name, &dir, &cluster, &blk_ofs)) {
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

int loop_fstream_open(io_stream_t handle, stream_mode mode)
{
	loop_fstream_info_t *info = (loop_fstream_info_t *)handle->data;

	assert(info);

	/* loop fstream is read only */
	if (mode != MODE_IN)
		return -EINVAL;

	handle->mode = mode;
	handle->write_finished = 0;
	handle->cache_size = 0;
	/* asume it a fake file length */
	handle->total_size = 0x7FFFFFFF;

	memset(&lp, 0, sizeof(struct loop_file_info));

	/* get actual file length and then seek back to file begin */
	if (!fs_seek(&info->fp, 0, FS_SEEK_END)) {
		lp.flen = fs_tell(&info->fp);
		fs_seek(&info->fp, 0, FS_SEEK_SET);
	}

	lp.flen_remain = handle->total_size;
	loop_fstream_clac_info(0, 0);

	handle->wofs = handle->total_size;

	SYS_LOG_INF("handle %p length %d mode %x \n", handle, lp.flen, mode);
	return 0;
}

int loop_fstream_read(io_stream_t handle, unsigned char *buf, int num)
{
	int brw, cur_offset, read_len = num;
	unsigned char *ptr = buf;
	bool read_head_flag = false;
	loop_fstream_info_t *info = (loop_fstream_info_t *)handle->data;

	assert(info);

	brw = os_mutex_lock(&info->lock, OS_FOREVER);
	if (brw < 0){
		SYS_LOG_ERR("lock failed %d \n", brw);
		return brw;
	}

	cur_offset = fs_tell(&info->fp);

	/* enable energy limiter when file head or file tail */
	if (lp.energy_filt_enable) {
		if (cur_offset + lp.flen_energy_clac >= lp.flen
			|| cur_offset - lp.flen_head <= lp.flen_energy_clac) {
			lp.energy_filt_enable(1);
		} else {
			lp.energy_filt_enable(0);
		}
	}

	/* reach file tail: read the rest data and ready for 2nd read */
	if (cur_offset + read_len > lp.flen_notail) {
		read_len = lp.flen_notail - cur_offset;

		/* make sure that it finally ends at the actual end of file */
		if (lp.flen_remain > lp.flen_data * 2) {
			lp.flen_remain -= lp.flen_data;
			read_head_flag = true;
		}
	}

	read_len = fs_read(&info->fp, ptr, read_len);
	if (read_len < 0) {
		SYS_LOG_ERR(" failed %d\n", read_len);
		goto err_out;
	}

	/* 2nd read data from acutal file head to get enough quantity data of "count" */
	ptr += read_len;

	if (read_head_flag && read_len == 0) {
		brw = fs_seek(&info->fp, lp.flen_head, FS_SEEK_SET);
		if (brw < 0) {
			SYS_LOG_ERR(" seek head failed %d\n", brw);
			goto err_out;
		}

		brw = fs_read(&info->fp, ptr, num - read_len);
		if (brw < 0) {
			SYS_LOG_ERR(" failed %d\n", brw);
			goto err_out;
		}
		read_len += brw;
	}

	handle->rofs = fs_tell(&info->fp);
	lp.cur_rofs += read_len;
	if (lp.cur_rofs > handle->total_size)
		lp.cur_rofs -= handle->total_size;

err_out:
	os_mutex_unlock(&info->lock);

	return read_len;
}

int loop_fstream_tell(io_stream_t handle)
{
	/* return the read offset of virtual file */
	return lp.cur_rofs;
}

int loop_fstream_seek(io_stream_t handle, int offset, seek_dir origin)
{
	/* must update lp.cur_rof, to indicate the virtual rofs */

	int whence = FS_SEEK_SET;
	int brw = 0;

	loop_fstream_info_t *info = (loop_fstream_info_t *)handle->data;

	assert(info);

	switch (origin) {
	case SEEK_DIR_CUR:
		if (lp.cur_rofs + offset > handle->total_size)
			return -1;//over length

		lp.cur_rofs += offset;
		/* convert to true file range */
		offset = lp.cur_rofs % lp.flen;
		break;

	case SEEK_DIR_END:
		if (offset > 0)
			return -1;//over length

		lp.cur_rofs = handle->total_size + offset;
		whence = FS_SEEK_END;
		break;

	case SEEK_DIR_BEG:
		if (offset > handle->total_size)
			return -1;//over length

		lp.cur_rofs = offset;
		/*do this because stream_seek has change SEEK_DIR_END to SEEK_DIR_BEG */
		if (handle->total_size - offset < lp.flen) {
			offset = lp.flen - (handle->total_size - offset);
		} else {
			/* calc the true offset because most cases "offset" is NOT in file */
			offset %= lp.flen;
		}

	default:
		break;
	}

	brw = fs_seek(&info->fp, offset, whence);
	if (brw) {
		SYS_LOG_ERR("seek failed %d,origin %d\n", brw, origin);
		return -1;
	}

	/* update the true rof */
	offset = fs_tell(&info->fp);
	handle->rofs = offset;

	return 0;
}

int loop_fstream_close(io_stream_t handle)
{
	int res;
	loop_fstream_info_t *info = (loop_fstream_info_t *)handle->data;

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


int loop_fstream_destroy(io_stream_t handle)
{
	int res;
	loop_fstream_info_t *info = (loop_fstream_info_t *)handle->data;

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

/* get info from app who uses this stream */
void loop_fstream_get_info(void *info, uint8_t info_id)
{
	switch (info_id) {
	case LOOPFS_FILE_INFO:
	{
		struct loopfs_file_info *lps = (struct loopfs_file_info *)(void *)info;
		lp.flen_energy_clac = lps->bitrate / 8 * ENERGY_FILTER_TIME;
		loop_fstream_clac_info(lps->head, lps->tail);
		break;
	}
	case LOOPFS_CALLBACK:
	{
		lp.energy_filt_enable = (loopfs_energy_cb)info;
		break;
	}
	}
}

const stream_ops_t loop_fstream_ops = {
	.init = loop_fstream_init,
	.open = loop_fstream_open,
	.read = loop_fstream_read,
	.seek = loop_fstream_seek,
	.tell = loop_fstream_tell,
	.close = loop_fstream_close,
	.destroy = loop_fstream_destroy,
};

io_stream_t loop_fstream_create(const char *param)
{
	return stream_create(&loop_fstream_ops, (void *)param);
}
