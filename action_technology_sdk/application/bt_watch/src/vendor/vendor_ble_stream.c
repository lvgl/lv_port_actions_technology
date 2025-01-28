/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief vendor ble stream.
 */
#define SYS_LOG_DOMAIN "bt manager"

#include <os_common_api.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <msg_manager.h>
#include <mem_manager.h>
#include <bt_manager.h>
#include <sys_event.h>
#include <stream.h>
#include "vendor_app.h"

#define MAX_BLE_STREAM		(5)
#define BLE_BUFF_SIZE		(1024*4)
#define BLE_SEND_LEN_ONCE	(512)
#define BLE_SEND_INTERVAL	(5)		/* 5ms */

struct ble_info_t {
	struct ble_reg_manager le_mgr;
	struct bt_gatt_attr	*tx_chrc_attr;
	struct bt_gatt_attr	*tx_attr;
	struct bt_gatt_attr	*tx_ccc_attr;
	struct bt_gatt_attr	*rx_attr;
	void (*connect_cb)(bool connected);
	uint8_t connected : 1;
	uint8_t ccc_value : 2;
	int32_t read_timeout;
	int32_t write_timeout;
	uint8_t *buff;
	os_mutex read_mutex;
	os_sem read_sem;
	os_mutex write_mutex;
};

static io_stream_t ble_create_stream[MAX_BLE_STREAM] __IN_BT_SECTION;
static OS_MUTEX_DEFINE(g_ble_mutex);

static int ble_add_stream(io_stream_t handle)
{
	int i;

	os_mutex_lock(&g_ble_mutex, OS_FOREVER);
	for (i = 0; i < MAX_BLE_STREAM; i++) {
		if (ble_create_stream[i] == NULL) {
			ble_create_stream[i] = handle;
			break;
		}
	}
	os_mutex_unlock(&g_ble_mutex);

	if (i == MAX_BLE_STREAM) {
		SYS_LOG_ERR("Failed to add stream handle %p", handle);
		return -EIO;
	}

	return 0;
}

static int ble_remove_stream(io_stream_t handle)
{
	int i;

	os_mutex_lock(&g_ble_mutex, OS_FOREVER);
	for (i = 0; i < MAX_BLE_STREAM; i++) {
		if (ble_create_stream[i] == handle) {
			ble_create_stream[i] = NULL;
			break;
		}
	}
	os_mutex_unlock(&g_ble_mutex);

	if (i == MAX_BLE_STREAM) {
		SYS_LOG_ERR("Failed to remove stream handle %p", handle);
		return -EIO;
	}

	return 0;
}

static io_stream_t find_stream_by_ble_attr(const struct bt_gatt_attr *attr)
{
	io_stream_t stream;
	struct ble_info_t *info;
	int i;

	os_mutex_lock(&g_ble_mutex, OS_FOREVER);

	for (i = 0; i < MAX_BLE_STREAM; i++) {
		stream = ble_create_stream[i];
		if (stream) {
			info = (struct ble_info_t *)stream->data;
			if ((info->tx_chrc_attr == attr) || (info->tx_attr == attr) ||
				(info->tx_ccc_attr == attr) || (info->rx_attr == attr)) {
				os_mutex_unlock(&g_ble_mutex);
				return stream;
			}
		}
	}

	os_mutex_unlock(&g_ble_mutex);

	return NULL;
}

static void ble_rx_data(io_stream_t handle, uint8_t *buf, uint16_t len)
{
	struct ble_info_t *info = NULL;
	uint16_t w_len, r_len;
	static uint32_t lost;

	info = (struct ble_info_t *)handle->data;
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
		lost += len;
		SYS_LOG_WRN("Not enough buffer: %d, %d, %d", handle->cache_size, len, handle->total_size);
		SYS_LOG_INF("Lost data %u", lost);
	}
	os_mutex_unlock(&info->read_mutex);
}

static ssize_t stream_ble_rx_write(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len, uint16_t offset,
			      uint8_t flags)
{
	struct ble_info_t *info;
	io_stream_t stream = find_stream_by_ble_attr(attr);

	//SYS_LOG_INF("BLE rx data len %u", len);
	if (stream && stream->data) {
		info = (struct ble_info_t *)stream->data;
		if (info->connected) {
			ble_rx_data(stream, (uint8_t *)buf, len);
		}
	}

	return len;
}

static void stream_ble_tx_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	struct ble_info_t *info;
	io_stream_t stream = find_stream_by_ble_attr(attr);

	SYS_LOG_INF("attr %p, enable %d", attr, value);
	if (!stream) {
		SYS_LOG_ERR("stream is NULL.");
		return;
	}
	info = (struct ble_info_t *)stream->data;
	info->ccc_value = (uint8_t)value;
}

static void stream_ble_connect_cb(uint8_t *mac, uint8_t connected)
{
	io_stream_t stream;
	struct ble_info_t *info;
	int i;

	SYS_LOG_INF("BLE %s", connected ? "connected" : "disconnected");
	SYS_LOG_INF("MAC %2x:%2x:%2x:%2x:%2x:%2x", mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);

	os_mutex_lock(&g_ble_mutex, OS_FOREVER);

	for (i = 0; i < MAX_BLE_STREAM; i++) {
		stream = ble_create_stream[i];
		if (stream) {
			info = (struct ble_info_t *)stream->data;
			info->connected = connected ? 1 : 0;
			/* give sem when disconnected */
			os_sem_give(&info->read_sem);
			if (info->connect_cb) {
				info->connect_cb(connected);
			}
		}
	}

	os_mutex_unlock(&g_ble_mutex);
}

static int ble_register(struct ble_info_t *info)
{
	struct _bt_gatt_ccc *ccc;

	info->rx_attr->write = stream_ble_rx_write;
	ccc = info->tx_ccc_attr->user_data;
	ccc->cfg_changed = stream_ble_tx_cfg_changed;
	info->le_mgr.link_cb = stream_ble_connect_cb;
#ifdef CONFIG_BT_BLE
	bt_manager_ble_service_reg(&info->le_mgr);
#endif
	return 0;
}

static int ble_stream_init(io_stream_t handle, void *param)
{
	int ret = 0;
	struct ble_info_t *info = NULL;
	struct ble_stream_init_param *init_param = param;

	if (ble_add_stream(handle)) {
		ret = -EIO;
		goto err_exit;
	}

	info = mem_malloc(sizeof(struct ble_info_t));
	if (!info) {
		SYS_LOG_ERR("cache stream info malloc failed\n");
		ret = -ENOMEM;
		goto err_exit;
	}

	memset(info, 0, sizeof(struct ble_info_t));
	info->le_mgr.gatt_svc.attrs = init_param->gatt_attr;
	info->le_mgr.gatt_svc.attr_count = init_param->attr_size;
	info->tx_chrc_attr = init_param->tx_chrc_attr;
	info->tx_attr = init_param->tx_attr;
	info->tx_ccc_attr = init_param->tx_ccc_attr;
	info->rx_attr = init_param->rx_attr;
	info->connect_cb = init_param->connect_cb;
	info->read_timeout = init_param->read_timeout;
	info->write_timeout = init_param->write_timeout;
	os_mutex_init(&info->read_mutex);
	os_mutex_init(&info->write_mutex);
    os_sem_init(&info->read_sem, 0, 1);

	handle->data = info;

	if (ble_register(info)) {
		ret = -EIO;
		goto err_exit;
	}

	return 0;

err_exit:
	if (info) {
		mem_free(info);
	}

	ble_remove_stream(handle);
	return ret;
}

static int ble_stream_open(io_stream_t handle, stream_mode mode)
{
	struct ble_info_t *info = NULL;

	info = (struct ble_info_t *)handle->data;
	if (!info->connected) {
		return -EIO;
	}

	os_mutex_lock(&info->read_mutex, OS_FOREVER);
	info->buff = mem_malloc(BLE_BUFF_SIZE);
	if (!info->buff) {
		os_mutex_unlock(&info->read_mutex);
		return -ENOMEM;
	}

	handle->total_size = BLE_BUFF_SIZE;
	handle->cache_size = 0;
	handle->rofs = 0;
	handle->wofs = 0;
	os_mutex_unlock(&info->read_mutex);

	SYS_LOG_INF("ble buf size %u", handle->total_size);

	return 0;
}

static int ble_stream_read(io_stream_t handle, uint8_t *buf, int num)
{
	struct ble_info_t *info = NULL;
	uint16_t r_len, rr_len;

	info = (struct ble_info_t *)handle->data;

	os_mutex_lock(&info->read_mutex, OS_FOREVER);

	if ((!info->connected) || (!info->buff)) {
		os_mutex_unlock(&info->read_mutex);
		return -EIO;
	}

	if ((info->connected) && (handle->cache_size == 0) &&
		(info->read_timeout != OS_NO_WAIT)) {
		os_sem_reset(&info->read_sem);
		os_mutex_unlock(&info->read_mutex);

		os_sem_take(&info->read_sem, info->read_timeout);
		os_mutex_lock(&info->read_mutex, OS_FOREVER);
	}

	if (handle->cache_size == 0) {
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

	os_mutex_unlock(&info->read_mutex);
	return r_len;
}

static int ble_stream_tell(io_stream_t handle)
{
	int ret = 0;
	struct ble_info_t *info = NULL;

	info = (struct ble_info_t *)handle->data;
	if (!info->connected) {
		return -EIO;
	}

	os_mutex_lock(&info->read_mutex, OS_FOREVER);
	ret = handle->cache_size;
	os_mutex_unlock(&info->read_mutex);

	return ret;
}

#ifdef CONFIG_BT_BLE
static int ble_send_data(struct ble_info_t *info, uint8_t *buf, int num)
{
	uint16_t mtu;
	int send_len = 0, w_len, le_send, cur_len;
	int ret = 0;
	int32_t timeout = 0;

	while ((info->connected) && (info->ccc_value) && (send_len < num)) {
		w_len = ((num - send_len) > BLE_SEND_LEN_ONCE) ? BLE_SEND_LEN_ONCE : (num - send_len);

		mtu = bt_manager_get_ble_mtu() - 3;
		le_send = 0;
		while (le_send < w_len) {
			cur_len = ((w_len - le_send) > mtu) ? mtu : (w_len - le_send);
			ret = bt_manager_ble_send_data(info->tx_chrc_attr, info->tx_attr, &buf[send_len], cur_len);
			if (ret < 0) {
				break;
			}
			send_len += cur_len;
			le_send += cur_len;
		}

		if (ret < 0) {
			if (info->write_timeout == OS_NO_WAIT) {
				break;
			} else if (info->write_timeout == OS_FOREVER) {
				os_sleep(BLE_SEND_INTERVAL);
				continue;
			} else {
				if (timeout >= info->write_timeout) {
					break;
				}

				timeout += (int32_t)BLE_SEND_INTERVAL;
				os_sleep(BLE_SEND_INTERVAL);
				continue;
			}
		}
	}

	return send_len;
}
#endif

static int ble_stream_write(io_stream_t handle, uint8_t *buf, int num)
{
	int ret = 0;
	struct ble_info_t *info = NULL;

	info = (struct ble_info_t *)handle->data;
	if (!info->connected) {
		return -EIO;
	}

#ifdef CONFIG_BT_BLE
	os_mutex_lock(&info->write_mutex, OS_FOREVER);
	ret = ble_send_data(info, buf, num);
	os_mutex_unlock(&info->write_mutex);
#endif

	return ret;
}

static int ble_stream_close(io_stream_t handle)
{
	struct ble_info_t *info = NULL;

	info = (struct ble_info_t *)handle->data;
	if (info->connected) {
		SYS_LOG_INF("Active do disconnect");
#ifdef CONFIG_BT_BLE
		/* Shell not disconnect ble, may other servcie still use ble */
		//bt_manager_ble_disconnect();
#endif
	}

	os_mutex_lock(&info->read_mutex, OS_FOREVER);
	if (info->buff) {
		mem_free(info->buff);
		info->buff = NULL;
		handle->rofs = 0;
		handle->wofs = 0;
		handle->cache_size = 0;
		handle->total_size = 0;
	}
	os_mutex_unlock(&info->read_mutex);

	return 0;
}

static int ble_stream_destroy(io_stream_t handle)
{
	SYS_LOG_WRN("ble stream not support destroy new!!");
	return -EIO;
}

const stream_ops_t ble_stream_ops = {
	.init = ble_stream_init,
	.open = ble_stream_open,
	.read = ble_stream_read,
	.seek = NULL,
	.tell = ble_stream_tell,
	.write = ble_stream_write,
	.close = ble_stream_close,
	.destroy = ble_stream_destroy,
};

io_stream_t ble_stream_create(void *param)
{
	return stream_create(&ble_stream_ops, param);
}

