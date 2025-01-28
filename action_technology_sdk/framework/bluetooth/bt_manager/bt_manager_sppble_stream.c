/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager genarate bt mac and name.
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
#include "bt_manager_inner.h"
#include <sys_event.h>
#include <stream.h>
#include "bt_porting_inner.h"

#ifdef	CONFIG_OTA_BLE_MASTER_SUPPORT
#include "ble_master/ble_master_manager.h"
#endif

#define MAX_SPPBLE_STREAM		(5)
#define SPPBLE_BUFF_SIZE		(512)   /* default size */
#define SPPBLE_SEND_LEN_ONCE	(680)
#define SPPBLE_SEND_INTERVAL	(5)		/* 5ms */
static int sppble_open(io_stream_t handle, stream_mode mode);

struct sppble_info_t {
	uint8_t *spp_uuid;
	struct ble_reg_manager le_mgr;
	struct bt_gatt_attr	*tx_chrc_attr;
	struct bt_gatt_attr	*tx_attr;
	struct bt_gatt_attr	*tx_ccc_attr;
	struct bt_gatt_attr	*rx_attr;
	void (*connect_cb)(bool connected, uint8_t connect_type);
#if defined(CONFIG_OTA_PRODUCT_SUPPORT) || defined(CONFIG_OTA_BLE_MASTER_SUPPORT)
	void(*rxdata_cb)(void);
#endif
	uint8_t connect_type;
	uint8_t spp_chl;
	uint8_t notify_ind_enable;
	int32_t read_timeout;
	int32_t write_timeout;
	int32_t read_buf_size;
	uint8_t *buff;
	os_mutex read_mutex;
	os_sem read_sem;
	os_mutex write_mutex;
};

static void sppble_rx_date(io_stream_t handle, uint8_t *buf, uint16_t len);
static io_stream_t sppble_create_stream[MAX_SPPBLE_STREAM] __IN_BT_SECTION;
static OS_MUTEX_DEFINE(g_sppble_mutex);

static int sppble_add_stream(io_stream_t handle)
{
	int i;

	os_mutex_lock(&g_sppble_mutex, OS_FOREVER);
	for (i = 0; i < MAX_SPPBLE_STREAM; i++) {
		if (sppble_create_stream[i] == NULL) {
			sppble_create_stream[i] = handle;
			break;
		}
	}
	os_mutex_unlock(&g_sppble_mutex);

	if (i == MAX_SPPBLE_STREAM) {
		SYS_LOG_ERR("Failed to add stream handle %p", handle);
		return -EIO;
	}

	return 0;
}

static int sppble_remove_stream(io_stream_t handle)
{
	int i;

	os_mutex_lock(&g_sppble_mutex, OS_FOREVER);
	for (i = 0; i < MAX_SPPBLE_STREAM; i++) {
		if (sppble_create_stream[i] == handle) {
			sppble_create_stream[i] = NULL;
			break;
		}
	}
	os_mutex_unlock(&g_sppble_mutex);

	if (i == MAX_SPPBLE_STREAM) {
		SYS_LOG_ERR("Failed to remove stream handle %p", handle);
		return -EIO;
	}

	return 0;
}

#ifdef	CONFIG_OTA_BLE_MASTER_SUPPORT
static io_stream_t find_streame_ble_master(void)
{
	io_stream_t stream;
	struct sppble_info_t *info;
	int i;

	os_mutex_lock(&g_sppble_mutex, OS_FOREVER);

	for (i = 0; i < MAX_SPPBLE_STREAM; i++) {
		stream = sppble_create_stream[i];
		if (stream) {
			info = (struct sppble_info_t *)stream->data;
			if (info->rxdata_cb) {
				os_mutex_unlock(&g_sppble_mutex);
				return stream;
			}
		}
	}

	os_mutex_unlock(&g_sppble_mutex);
	return NULL;
}
#else
#ifdef CONFIG_BT_SPP
static io_stream_t find_streame_by_spp_uuid(uint8_t *uuid)
{
	io_stream_t stream;
	struct sppble_info_t *info;
	int i;

	os_mutex_lock(&g_sppble_mutex, OS_FOREVER);

	for (i = 0; i < MAX_SPPBLE_STREAM; i++) {
		stream = sppble_create_stream[i];
		if (stream) {
			info = (struct sppble_info_t *)stream->data;
			if (!memcmp(info->spp_uuid, uuid, SPP_UUID_LEN)) {
				os_mutex_unlock(&g_sppble_mutex);
				return stream;
			}
		}
	}

	os_mutex_unlock(&g_sppble_mutex);
	return NULL;
}
#endif
#endif

#ifdef CONFIG_BT_SPP
static io_stream_t find_streame_by_spp_channel(uint8_t channel)
{
	io_stream_t stream;
	struct sppble_info_t *info;
	int i;

	os_mutex_lock(&g_sppble_mutex, OS_FOREVER);

	for (i = 0; i < MAX_SPPBLE_STREAM; i++) {
		stream = sppble_create_stream[i];
		if (stream) {
			info = (struct sppble_info_t *)stream->data;
			if (info->spp_chl == channel) {
				os_mutex_unlock(&g_sppble_mutex);
				return stream;
			}
		}
	}

	os_mutex_unlock(&g_sppble_mutex);
	return NULL;
}
#endif

static io_stream_t find_streame_by_ble_attr(const struct bt_gatt_attr *attr)
{
	io_stream_t stream;
	struct sppble_info_t *info;
	int i;

	os_mutex_lock(&g_sppble_mutex, OS_FOREVER);

	for (i = 0; i < MAX_SPPBLE_STREAM; i++) {
		stream = sppble_create_stream[i];
		if (stream) {
			info = (struct sppble_info_t *)stream->data;
			if ((info->tx_chrc_attr == attr) || (info->tx_attr == attr) ||
				(info->tx_ccc_attr == attr) || (info->rx_attr == attr)) {
				os_mutex_unlock(&g_sppble_mutex);
				return stream;
			}
		}
	}

	os_mutex_unlock(&g_sppble_mutex);

	return NULL;
}

#ifdef CONFIG_BT_SPP
static void stream_spp_connected_cb(uint8_t channel, uint8_t *uuid)
{
	struct sppble_info_t *info;
#ifdef	CONFIG_OTA_BLE_MASTER_SUPPORT
	io_stream_t handle = find_streame_ble_master();
#else
	io_stream_t handle = find_streame_by_spp_uuid(uuid);
#endif

	SYS_LOG_INF("channel:%d", channel);

	if (handle && handle->data) {
		info = (struct sppble_info_t *)handle->data;
		info->spp_chl = channel;
		if (info->connect_type == NONE_CONNECT_TYPE) {
			info->connect_type = SPP_CONNECT_TYPE;
			if (info->connect_cb) {
				info->connect_cb(true, SPP_CONNECT_TYPE);
			}
		} else {
			SYS_LOG_WRN("Had connecte: %d", info->connect_type);
		}
	}
}

static void stream_spp_disconnected_cb(uint8_t channel)
{
	struct sppble_info_t *info;
	io_stream_t handle = find_streame_by_spp_channel(channel);

	SYS_LOG_INF("channel:%d", channel);

	if (handle && handle->data) {
		info = (struct sppble_info_t *)handle->data;
		if (info->connect_type == SPP_CONNECT_TYPE) {
			info->connect_type = NONE_CONNECT_TYPE;
			os_sem_give(&info->read_sem);
			if (info->connect_cb) {
				info->connect_cb(false, SPP_CONNECT_TYPE);
			}
		} else {
			SYS_LOG_WRN("Not connecte spp: %d", info->connect_type);
		}
	}
}

static void stream_spp_receive_data_cb(uint8_t channel, uint8_t *data, uint32_t len)
{
	struct sppble_info_t *info;
	io_stream_t handle = find_streame_by_spp_channel(channel);

	SYS_LOG_DBG("channel:%d, len:%d", channel, len);
	if (handle && handle->data) {
		info = (struct sppble_info_t *)handle->data;
		if (info->connect_type == SPP_CONNECT_TYPE) {
			sppble_open(handle, MODE_IN_OUT);
			sppble_rx_date(handle, data, len);
		}
	}
}

static const struct btmgr_spp_cb stream_spp_cb = {
	.connected = stream_spp_connected_cb,
	.disconnected = stream_spp_disconnected_cb,
	.receive_data = stream_spp_receive_data_cb,
};
#endif

static ssize_t stream_ble_rx_data(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len, uint16_t offset,
			      uint8_t flags)
{
	struct sppble_info_t *info;
	io_stream_t stream = find_streame_by_ble_attr(attr);

	SYS_LOG_DBG("BLE rx data len %d", len);
	if (stream && stream->data) {
		info = (struct sppble_info_t *)stream->data;
		if (info->connect_type == BLE_CONNECT_TYPE) {
			sppble_open(stream, MODE_IN_OUT);
			sppble_rx_date(stream, (uint8_t *)buf, len);
		}
	}

	return len;
}

static void stream_ble_rx_set_notifyind(const struct bt_gatt_attr *attr, uint16_t value)
{
	struct sppble_info_t *info;
	io_stream_t stream = find_streame_by_ble_attr(attr);

	if (!stream) {
		SYS_LOG_WRN("Stream is NULL!");
		return;
	}

	if (!stream->data) {
		SYS_LOG_WRN("Stream data is NULL!");
		return;
	}

	SYS_LOG_INF("attr:%p, enable:%d", attr, value);
	info = (struct sppble_info_t *)stream->data;
	info->notify_ind_enable = (uint8_t)value;

	if (stream && stream->data && value) {
		if (info->connect_type == NONE_CONNECT_TYPE) {
			//info->connect_type = BLE_CONNECT_TYPE;
			//if (info->connect_cb) {
			//	info->connect_cb(true);
			}
		} else {
			SYS_LOG_WRN("Had connecte: %d", info->connect_type);
		}
	}

static ssize_t stream_ble_rx_write_state(struct bt_conn* conn, const struct bt_gatt_attr *attr, uint16_t value)
{
	if(conn == NULL)
		return -EINVAL;
	
	struct sppble_info_t *info;
	io_stream_t stream = find_streame_by_ble_attr(attr);

	if (!stream) {
		SYS_LOG_WRN("Stream is NULL!");
		return sizeof(value);
	}

	info = (struct sppble_info_t *)stream->data;

	if (stream && stream->data && value) {
		if (info->connect_type == NONE_CONNECT_TYPE) {
			info->connect_type = BLE_CONNECT_TYPE;
			if (info->connect_cb) {
				info->connect_cb(true, BLE_CONNECT_TYPE);
			}
		} else {
			SYS_LOG_WRN("Had connecte: %d", info->connect_type);
		}
	}
	return sizeof(value);
}

static void stream_ble_connect_cb(uint8_t *mac, uint8_t connected)
{
	io_stream_t stream;
	struct sppble_info_t *info;
	int i;

	SYS_LOG_INF("BLE %s", connected ? "connected" : "disconnected");
	SYS_LOG_INF("MAC %2x:%2x:%2x:%2x:%2x:%2x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	os_mutex_lock(&g_sppble_mutex, OS_FOREVER);

	if (!connected) {
		for (i = 0; i < MAX_SPPBLE_STREAM; i++) {
			stream = sppble_create_stream[i];
			if (stream) {
				info = (struct sppble_info_t *)stream->data;
				if (info->connect_type == BLE_CONNECT_TYPE) {
					info->connect_type = NONE_CONNECT_TYPE;
					os_sem_give(&info->read_sem);
					if (info->connect_cb) {
						info->connect_cb(false, BLE_CONNECT_TYPE);
					}
				}
			}
		}
	}

	os_mutex_unlock(&g_sppble_mutex);
}

static int sppble_register(struct sppble_info_t *info)
{
	struct _bt_gatt_ccc *ccc;

#ifndef CONFIG_OTA_PRODUCT_SUPPORT
#ifdef CONFIG_BT_SPP
	int ret;

	if (info->spp_uuid)
	{
		ret = bt_manager_spp_reg_uuid(info->spp_uuid, (struct btmgr_spp_cb *)&stream_spp_cb);
		if (ret < 0) {
			SYS_LOG_ERR("Failed register spp uuid");
			return ret;
		}
	}
#endif
#endif

	if (info->rx_attr)
		info->rx_attr->write = stream_ble_rx_data;
	if (info->tx_ccc_attr) {
		ccc = (struct _bt_gatt_ccc *)info->tx_ccc_attr->user_data;
		ccc->cfg_changed = stream_ble_rx_set_notifyind;
		ccc->cfg_write = stream_ble_rx_write_state;
		info->le_mgr.link_cb = stream_ble_connect_cb;
#ifdef CONFIG_OTA_GATT_OVER_EDR_TEST
		extern uint16_t bt_gobr_sdp_handle_get(void);
		info->le_mgr.gatt_svc.sdp_handle = bt_gobr_sdp_handle_get();
#endif
#ifdef CONFIG_BT_BLE
		bt_manager_ble_service_reg(&info->le_mgr);
#endif
	}
	return 0;
}

static int sppble_init(io_stream_t handle, void *param)
{
	int ret = 0;
	struct sppble_info_t *info = NULL;
	struct sppble_stream_init_param *init_param = (struct sppble_stream_init_param *)param;

	if (sppble_add_stream(handle)) {
		ret = -EIO;
		goto err_exit;
	}

	info = bt_mem_malloc(sizeof(struct sppble_info_t));
	if (!info) {
		SYS_LOG_ERR("cache stream info malloc failed\n");
		ret = -ENOMEM;
		goto err_exit;
	}

	memset(info, 0, sizeof(struct sppble_info_t));
	info->spp_uuid = init_param->spp_uuid;
	info->le_mgr.gatt_svc.attrs = init_param->gatt_attr;
	info->le_mgr.gatt_svc.attr_count = init_param->attr_size;
	info->tx_chrc_attr = init_param->tx_chrc_attr;
	info->tx_attr = init_param->tx_attr;
	info->tx_ccc_attr = init_param->tx_ccc_attr;
	info->rx_attr = init_param->rx_attr;
	info->connect_cb = init_param->connect_cb;
#if defined(CONFIG_OTA_PRODUCT_SUPPORT) || defined(CONFIG_OTA_BLE_MASTER_SUPPORT)
	info->rxdata_cb = init_param->rxdata_cb;
#endif
	info->read_timeout = init_param->read_timeout;
	info->write_timeout = init_param->write_timeout;
	info->read_buf_size = init_param->read_buf_size;
	if (info->read_buf_size == 0) {
		info->read_buf_size = SPPBLE_BUFF_SIZE;
	}
	os_mutex_init(&info->read_mutex);
	os_sem_init(&info->read_sem, 0, 1);
	os_mutex_init(&info->write_mutex);

	handle->data = info;

#ifdef	CONFIG_OTA_BLE_MASTER_SUPPORT
	if (info->rxdata_cb) {
		ret = ble_master_manager_cb_reg((struct btmgr_spp_cb *)&stream_spp_cb);
		if (ret < 0) {
			SYS_LOG_ERR("Failed register ble matser callback fail!");
			ret = -EIO;
			goto err_exit;
		}
		
		return 0;
	}
#endif

	if (sppble_register(info)) {
		ret = -EIO;
		goto err_exit;
	}

	return 0;

err_exit:
	if (info) {
		bt_mem_free(info);
	}

	sppble_remove_stream(handle);
	return ret;
}

static int sppble_open(io_stream_t handle, stream_mode mode)
{
	struct sppble_info_t *info = NULL;

	info = (struct sppble_info_t *)handle->data;
	if (info->connect_type == NONE_CONNECT_TYPE) {
		return -EIO;
	}

	os_mutex_lock(&info->read_mutex, OS_FOREVER);
	if (info->buff) {
		os_mutex_unlock(&info->read_mutex);
		return 0;
	}

	info->buff = bt_mem_malloc(info->read_buf_size);
	if (!info->buff) {
		os_mutex_unlock(&info->read_mutex);
		return -ENOMEM;
	}

	handle->total_size = info->read_buf_size;
	handle->cache_size = 0;
	handle->rofs = 0;
	handle->wofs = 0;
	os_mutex_unlock(&info->read_mutex);

	return 0;
}

static void sppble_rx_date(io_stream_t handle, uint8_t *buf, uint16_t len)
{
	struct sppble_info_t *info = NULL;
	uint16_t w_len, r_len;
	uint8_t count;

	for (count = 0; count < 5; count++) {
		info = (struct sppble_info_t *)handle->data;
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
#if defined(CONFIG_OTA_PRODUCT_SUPPORT) || defined(CONFIG_OTA_BLE_MASTER_SUPPORT)
			if (info->rxdata_cb) {
				info->rxdata_cb();
			}
#endif
			os_mutex_unlock(&info->read_mutex);
			break;
		} else {
			os_mutex_unlock(&info->read_mutex);
			// release time slice, try 5 times.
			SYS_LOG_WRN("Not enough buffer: %d, %d, %d", handle->cache_size, len, handle->total_size);
			os_sleep(10);
			if (0 == handle->total_size) {
				// stream close already
				break;
			}
			continue;
		}
	}
}

static int sppble_read(io_stream_t handle, uint8_t *buf, int num)
{
	struct sppble_info_t *info = NULL;
	uint16_t r_len, rr_len;

	info = (struct sppble_info_t *)handle->data;

	os_mutex_lock(&info->read_mutex, OS_FOREVER);
	if ((info->connect_type == NONE_CONNECT_TYPE) ||
		(info->buff == NULL)) {
		os_mutex_unlock(&info->read_mutex);
		return -EIO;
	}

	if ((info->connect_type != NONE_CONNECT_TYPE) && (handle->cache_size == 0) &&
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

static int sppble_tell(io_stream_t handle)
{
	int ret = 0;
	struct sppble_info_t *info = NULL;

	info = (struct sppble_info_t *)handle->data;
	if (info->connect_type == NONE_CONNECT_TYPE) {
		return -EIO;
	}

	os_mutex_lock(&info->read_mutex, OS_FOREVER);
	ret = handle->cache_size;
	os_mutex_unlock(&info->read_mutex);

	return ret;
}

static int sppble_spp_send_data(struct sppble_info_t *info, uint8_t *buf, int num)
{
#ifdef CONFIG_BT_SPP
	int send_len = 0, w_len;
	int32_t timeout = 0;

	while ((info->connect_type == SPP_CONNECT_TYPE) && (send_len < num)) {
		w_len = ((num - send_len) > SPPBLE_SEND_LEN_ONCE) ? SPPBLE_SEND_LEN_ONCE : (num - send_len);
#ifdef	CONFIG_OTA_BLE_MASTER_SUPPORT
		if (ble_master_manager_send_data(info->spp_chl, &buf[send_len], w_len) == 0) {
			send_len += w_len;
#else
		if (bt_manager_spp_send_data(info->spp_chl, &buf[send_len], w_len) > 0) {
			send_len += w_len;
#endif
		} else {
			if (info->write_timeout == OS_NO_WAIT) {
				break;
			} else if (info->write_timeout == OS_FOREVER) {
				os_sleep(SPPBLE_SEND_INTERVAL);
			} else {
				if (timeout >= info->write_timeout) {
					break;
				}

				timeout += SPPBLE_SEND_INTERVAL;
				os_sleep(SPPBLE_SEND_INTERVAL);
			}
		}
	}

	return send_len;
#else
	return 0;
#endif
}
#ifdef CONFIG_BT_BLE
static int sppble_ble_send_data(struct sppble_info_t *info, uint8_t *buf, int num)
{
	uint16_t mtu;
	int send_len = 0, w_len, le_send, cur_len;
	int ret = 0;
	int32_t timeout = 0;

	while ((info->connect_type == BLE_CONNECT_TYPE) &&
			(info->notify_ind_enable) && (send_len < num)) {
		w_len = ((num - send_len) > SPPBLE_SEND_LEN_ONCE) ? SPPBLE_SEND_LEN_ONCE : (num - send_len);

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
				os_sleep(SPPBLE_SEND_INTERVAL);
				continue;
			} else {
				if (timeout >= info->write_timeout) {
					break;
				}

				timeout += (int32_t)SPPBLE_SEND_INTERVAL;
				os_sleep(SPPBLE_SEND_INTERVAL);
				continue;
			}
		}
	}

	return send_len;
}
#endif

static int sppble_write(io_stream_t handle, uint8_t *buf, int num)
{
	int ret = 0;
	struct sppble_info_t *info = NULL;

	info = (struct sppble_info_t *)handle->data;
	if (info->connect_type == NONE_CONNECT_TYPE) {
		return -EIO;
	}

	os_mutex_lock(&info->write_mutex, OS_FOREVER);
	if (info->connect_type == SPP_CONNECT_TYPE) {
		ret = sppble_spp_send_data(info, buf, num);
	} else if (info->connect_type == BLE_CONNECT_TYPE) {
	#ifdef CONFIG_BT_BLE
		ret = sppble_ble_send_data(info, buf, num);
	#endif
	}
	os_mutex_unlock(&info->write_mutex);

	return ret;
}

static int sppble_close(io_stream_t handle)
{
	struct sppble_info_t *info = NULL;

	info = (struct sppble_info_t *)handle->data;
	if (info->connect_type != NONE_CONNECT_TYPE) {
		SYS_LOG_INF("Active do %d disconnect", info->connect_type);
		if (info->connect_type == SPP_CONNECT_TYPE) {
		#ifdef CONFIG_BT_SPP
			bt_manager_spp_disconnect(info->spp_chl);
			#endif
		} else if (info->connect_type == BLE_CONNECT_TYPE) {
		#ifdef CONFIG_BT_BLE
			/* Shell not disconnect ble, may other servcie still use ble */
			//bt_manager_ble_disconnect();
		#endif
		}
	}

	os_mutex_lock(&info->read_mutex, OS_FOREVER);
	if (info->buff) {
		bt_mem_free(info->buff);
		info->buff = NULL;
		handle->rofs = 0;
		handle->wofs = 0;
		handle->cache_size = 0;
		handle->total_size = 0;
	}
	os_mutex_unlock(&info->read_mutex);

	return 0;
}

static int sppble_destroy(io_stream_t handle)
{
	SYS_LOG_WRN("Sppble stream not support destroy new!!");
	return -EIO;
}

const stream_ops_t sppble_stream_ops = {
	.init = sppble_init,
	.open = sppble_open,
	.read = sppble_read,
	.seek = NULL,
	.tell = sppble_tell,
	.write = sppble_write,
	.close = sppble_close,
	.destroy = sppble_destroy,
};

io_stream_t sppble_stream_create(void *param)
{
	return stream_create(&sppble_stream_ops, param);
}

#ifdef CONFIG_OTA_PRODUCT_SUPPORT
int bt_trans_ota_connect(bd_address_t *bd, uint8_t *uuid)
{
	int channel;
	channel = bt_manager_spp_connect(bd, uuid, (struct btmgr_spp_cb *)&stream_spp_cb);	

	return channel;
}
#endif
