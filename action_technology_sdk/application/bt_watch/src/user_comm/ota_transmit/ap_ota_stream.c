
#include <os_common_api.h>
#include <mem_manager.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "stream.h"
#include "ap_ota_private.h"

#define APOTA_BUFF_SIZE		(1024+1532)
#define APOTA_READ_INTERVAL	(5)		/* 5ms */

#define APOTA_STOP		0
#define APOTA_START		1

io_stream_t apota_stream = NULL;

//static u8_t apota_buff[APOTA_BUFF_SIZE];

struct apota_stream_info_t {
	u8_t connect_type;
	u8_t spp_chl;
	u32_t r_total_len;
	u32_t r_offset_len;
	u32_t head_xml_len;
	u32_t offset_start;
	s32_t read_timeout;
	s32_t write_timeout;
	u8_t *buff;
	os_mutex read_mutex;
	os_sem read_sem;
	os_mutex write_mutex;
	void (*connect_cb)(bool connected);
};

static int apota_stream_init(io_stream_t handle, void *param)
{
	int ret = 0;
	struct apota_stream_info_t *info = NULL;
	struct apota_stream_init_param *init_param = (struct apota_stream_init_param *)param;

	info = app_mem_malloc(sizeof(struct apota_stream_info_t));
	if (!info) {
		SYS_LOG_ERR("cache stream info malloc failed\n");
		ret = -ENOMEM;
		goto err_exit;
	}

	memset(info, 0, sizeof(struct apota_stream_info_t));

	info->read_timeout = init_param->read_timeout;
	info->write_timeout = init_param->write_timeout;
	info->connect_cb = init_param->connect_cb;
	info->connect_type = APOTA_STOP;
	os_mutex_init(&info->read_mutex);
	os_sem_init(&info->read_sem, 0, 1);
	os_mutex_init(&info->write_mutex);

	handle->data = info;

	return 0;

err_exit:
	if (info) {
		app_mem_free(info);
	}
	return ret;
}

static int apota_stream_open(io_stream_t handle, stream_mode mode)
{
	struct apota_stream_info_t *info = NULL;

	info = (struct apota_stream_info_t *)handle->data;

	os_mutex_lock(&info->read_mutex, OS_FOREVER);

	info->buff = app_mem_malloc(APOTA_BUFF_SIZE);
	if (!info->buff) {
		os_mutex_unlock(&info->read_mutex);
		return -ENOMEM;
	}

	info->r_total_len = 0;
	info->r_offset_len = 0;
	handle->total_size = APOTA_BUFF_SIZE;
	handle->cache_size = 0;
	handle->rofs = 0;
	handle->wofs = 0;
	os_mutex_unlock(&info->read_mutex);

	return 0;
}

static int apota_stream_read(io_stream_t handle, u8_t *buf, int num)
{
	struct apota_stream_info_t *info = NULL;
	u16_t r_len, rr_len;

	info = (struct apota_stream_info_t *)handle->data;

	info->r_offset_len = ota_backend_apota_get_offset();

	os_mutex_lock(&info->read_mutex, OS_FOREVER);
	if ((handle->cache_size == 0) && (info->read_timeout != OS_NO_WAIT)) {
		//os_sem_reset(&info->read_sem);
		os_mutex_unlock(&info->read_mutex);
		os_sem_take(&info->read_sem, info->read_timeout);
		os_mutex_lock(&info->read_mutex, OS_FOREVER);
	}

	while (info->r_total_len < info->r_offset_len) {
		if (handle->cache_size == 0) {
			break;
		}

		if (handle->rofs < (handle->total_size - 1)) {
			handle->rofs++;
		} else {
			handle->rofs = 0;
		}

		handle->cache_size--;
		info->r_total_len++;
	}

	if (handle->cache_size < num) {
		os_mutex_unlock(&info->read_mutex);
		return 0;
	}

	r_len = (handle->cache_size > num) ? num : handle->cache_size;
	if ((handle->rofs + r_len) > handle->total_size) {
		rr_len = handle->total_size - handle->rofs;
		memcpy(&buf[0], &info->buff[handle->rofs], rr_len);
		memcpy(&buf[rr_len], &info->buff[0], (r_len - rr_len));

		handle->cache_size -= r_len;
		handle->rofs = r_len - rr_len;
	} else {
		memcpy(&buf[0], &info->buff[handle->rofs], r_len);

		handle->cache_size -= r_len;
		handle->rofs += r_len;
	}

	info->r_total_len += r_len;

	if (r_len && (info->head_xml_len >= r_len))
	{
		info->head_xml_len -= r_len;
		if (0 == info->head_xml_len)
		{
			info->r_total_len = info->offset_start;
		}
	}

	os_mutex_unlock(&info->read_mutex);

	return r_len;
}

static int apota_stream_write(io_stream_t handle, u8_t *buf, int len)
{
	struct apota_stream_info_t *info = NULL;
	u16_t w_len, r_len;

	info = (struct apota_stream_info_t *)handle->data;

	os_mutex_lock(&info->read_mutex, OS_FOREVER);
	if ((handle->cache_size + len) <= handle->total_size) {
		if ((handle->wofs + len) > handle->total_size) {
			w_len = handle->total_size - handle->wofs;
			memcpy(&info->buff[handle->wofs], &buf[0], w_len);
			r_len = len - w_len;
			memcpy(&info->buff[0], &buf[w_len], r_len);
			handle->wofs = r_len;
		} else {
			memcpy(&info->buff[handle->wofs], buf, len);
			handle->wofs += len;
		}
		handle->cache_size += len;

		os_sem_give(&info->read_sem);
	} else {
		SYS_LOG_ERR("Not enough buffer: %d, %d, %d", handle->cache_size, len, handle->total_size);
	}

	os_mutex_unlock(&info->read_mutex);

	return handle->cache_size;
}

static int apota_stream_close(io_stream_t handle)
{
	struct apota_stream_info_t *info = NULL;

	info = (struct apota_stream_info_t *)handle->data;

	os_mutex_lock(&info->read_mutex, OS_FOREVER);
	if (info->buff) {
		app_mem_free(info->buff);
		info->buff = NULL;
		handle->rofs = 0;
		handle->wofs = 0;
		handle->cache_size = 0;
		handle->total_size = 0;
	}

	os_mutex_unlock(&info->read_mutex);

	return 0;
}

void apota_stream_connected(u32_t len, u32_t offset)
{
	if (!apota_stream)
	{
		SYS_LOG_WRN("apota_stream IS NULL");
		return;
	}

	struct apota_stream_info_t *info = NULL;

	info = (struct apota_stream_info_t *)apota_stream->data;
	if (info) {
		if (info->connect_type == APOTA_STOP) {
			info->connect_type = APOTA_START;
			info->head_xml_len = len;
			info->offset_start = offset;
			if (info->connect_cb) {
				info->connect_cb(true);
			}
		} else {
			SYS_LOG_WRN("Had connecte: %d", info->connect_type);
		}
	}
}

void apota_stream_disconnected(void)
{
	if (!apota_stream)
	{
		SYS_LOG_WRN("apota_stream IS NULL");
		return;
	}

	struct apota_stream_info_t *info = NULL;

	info = (struct apota_stream_info_t *)apota_stream->data;
	if (info) {
		if (info->connect_type == APOTA_START) {
			info->connect_type = APOTA_STOP;
			os_sem_give(&info->read_sem);
			if (info->connect_cb) {
				info->connect_cb(false);
			}
		} else {
			SYS_LOG_WRN("Not connecte %d", info->connect_type);
		}
	}
}

static int apota_stream_destroy(io_stream_t handle)
{
	apota_stream = NULL;

	SYS_LOG_WRN("Sppble stream not support destroy new!!");
	return -EIO;
}


static const stream_ops_t apota_stream_ops = {
	.init = apota_stream_init,
	.open = apota_stream_open,
	.read = apota_stream_read,
	.seek = NULL,
	.tell = NULL,
	.write = apota_stream_write,
	.close = apota_stream_close,
	.destroy = apota_stream_destroy,
};

io_stream_t apota_stream_create(void *param)
{
	apota_stream = stream_create(&apota_stream_ops, param);
	return apota_stream;
}


