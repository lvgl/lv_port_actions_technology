/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager spp profile.
 */
#define SYS_LOG_DOMAIN "bt manager"

#include <os_common_api.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>
#include <sys_event.h>
#include <bt_manager.h>
#include "bt_manager_inner.h"
#include "btservice_api.h"

#define BTMGR_MAX_SPP_NUM	6
#define MGRSPP_INDEX_TO_ID(x)		((x) | 0x80)
#define MGRSPP_ID_TO_INDEX(x)		((x) & (~0x80))

struct btmgr_uuid_cb {
	uint8_t connected:1;
	uint8_t active_connect:1;
	uint8_t app_id;			/* manager in this module, return to app still like channel */
	uint8_t *uuid;
	struct btmgr_spp_cb *cb;
};

struct btmgr_uuid_cb spp_uuid_cb[BTMGR_MAX_SPP_NUM];
OS_MUTEX_DEFINE(sppmgr_lock);

static struct btmgr_uuid_cb *bt_manager_spp_find(uint8_t app_id, uint8_t *uuid)
{
	int i;
	struct btmgr_uuid_cb *spp_info = NULL;

	for (i = 0; i < BTMGR_MAX_SPP_NUM; i++) {
		if (app_id) {
			if (spp_uuid_cb[i].app_id == app_id) {
				spp_info = &spp_uuid_cb[i];
				break;
			}
		} else if (uuid) {
			if (!memcmp(spp_uuid_cb[i].uuid, uuid, SPP_UUID_LEN)) {
				spp_info = &spp_uuid_cb[i];
				break;
			}
		} else {
			if (!spp_uuid_cb[i].app_id) {
				spp_info = &spp_uuid_cb[i];
				spp_info->app_id = MGRSPP_INDEX_TO_ID(i);
				spp_info->active_connect = 0;
				break;
			}
		}
	}

	return spp_info;
}

static void bt_manager_spp_free(struct btmgr_uuid_cb *spp_info)
{
	memset(spp_info, 0, sizeof(struct btmgr_uuid_cb));
}

/* uuid pointer must keep valued is used */
int bt_manager_spp_reg_uuid(uint8_t *uuid, struct btmgr_spp_cb *spp_cb)
{
	int ret = 0;
	struct btmgr_uuid_cb *spp_info;
	struct bt_spp_reg_param param;

	os_mutex_lock(&sppmgr_lock, OS_FOREVER);
	spp_info = bt_manager_spp_find(0, uuid);
	if (spp_info) {
		SYS_LOG_INF("Already register");
		ret = -EALREADY;
		goto reg_exit;
	}

	spp_info = bt_manager_spp_find(0, NULL);
	if (!spp_info) {
		SYS_LOG_INF("Not more");
		ret = -ENOMEM;
		goto reg_exit;
	}

	spp_info->connected = 0;
	spp_info->uuid = uuid;
	spp_info->cb = spp_cb;

	param.app_id = spp_info->app_id;
	param.uuid = uuid;
	ret = btif_spp_reg(&param);
	if (ret) {
		bt_manager_spp_free(spp_info);
		SYS_LOG_INF("Failed %d\n", ret);
	}

reg_exit:
	os_mutex_unlock(&sppmgr_lock);
	return ret;
}

int bt_manager_spp_send_data(uint8_t chl, uint8_t *data, uint32_t len)
{
	struct btmgr_uuid_cb *spp_info = bt_manager_spp_find(chl, NULL);

	if (spp_info) {
		return btif_spp_send_data(spp_info->app_id, data, len);
	} else {
		return -EIO;
	}
}

uint8_t bt_manager_spp_connect(bd_address_t *bd, uint8_t *uuid, struct btmgr_spp_cb *spp_cb)
{
	int ret;
	struct btmgr_uuid_cb *spp_info;
	struct bt_spp_connect_param param;

	spp_info = bt_manager_spp_find(0, NULL);
	if (spp_info == NULL) {
		return 0;
	}

	spp_info->connected = 0;
	spp_info->active_connect = 1;
	spp_info->uuid = uuid;
	spp_info->cb = spp_cb;

	param.app_id = spp_info->app_id;
	param.uuid = uuid;
	memcpy(&param.bd, bd, sizeof(bd_address_t));
	ret = btif_spp_connect(&param);
	if (ret) {
		bt_manager_spp_free(spp_info);
		return 0;
	} else {
		return spp_info->app_id;
	}
}

#ifdef CONFIG_OTA_PRODUCT_SUPPORT
uint8_t bt_manager_spp_connect_by_uuid(bd_address_t *bd, uint8_t *uuid)
{
	int ret;
	struct btmgr_uuid_cb *spp_info;
	struct bt_spp_connect_param param;

	spp_info = bt_manager_spp_find(0, uuid);
	if (spp_info == NULL) {
		SYS_LOG_INF("spp_info is null\n");
		return 0;
	}

	spp_info->connected = 0;
	spp_info->active_connect = 1;
	spp_info->uuid = uuid;

	param.app_id = spp_info->app_id;
	param.uuid = uuid;
	memcpy(&param.bd, bd, sizeof(bd_address_t));
	ret = btif_spp_connect(&param);
	if (ret) {
		bt_manager_spp_free(spp_info);
		return 0;
	} else {
		return spp_info->app_id;
	}
}
#endif

int bt_manager_spp_disconnect(uint8_t chl)
{
	struct btmgr_uuid_cb *spp_info = bt_manager_spp_find(chl, NULL);

	if (spp_info) {
		return btif_spp_disconnect(spp_info->app_id);
	} else {
		return -EIO;
	}
}

static void bt_manager_spp_callback(btsrv_spp_event_e event, uint8_t app_id, void *packet, int size)
{
	struct btmgr_uuid_cb *spp_info;

	os_mutex_lock(&sppmgr_lock, OS_FOREVER);

	spp_info = bt_manager_spp_find(app_id, NULL);
	if (!spp_info) {
		goto callback_exit;
	}

	switch (event) {
		case BTSRV_SPP_REGISTER_FAILED:
			SYS_LOG_WRN("BTSRV_SPP_REGISTER_FAILED\n");
			bt_manager_spp_free(spp_info);
			break;
		case BTSRV_SPP_REGISTER_SUCCESS:
			break;
		case BTSRV_SPP_CONNECT_FAILED:
			SYS_LOG_WRN("BTSRV_SPP_CONNECT_FAILED\n");
			if (spp_info->cb && spp_info->cb->connect_failed) {
				spp_info->cb->connect_failed(spp_info->app_id);
			}
			bt_manager_spp_free(spp_info);
			break;
		case BTSRV_SPP_CONNECTED:
			if (!spp_info->connected) {
				spp_info->connected = 1;
				if (spp_info->cb && spp_info->cb->connected) {
					spp_info->cb->connected(spp_info->app_id, spp_info->uuid);
				}
			}
			break;
		case BTSRV_SPP_DISCONNECTED:
			if (spp_info->connected) {
				spp_info->connected = 0;
				if (spp_info->cb && spp_info->cb->disconnected) {
					spp_info->cb->disconnected(spp_info->app_id);
				}

				if (spp_info->active_connect) {
					bt_manager_spp_free(spp_info);
				}
			}
			break;
		case BTSRV_SPP_DATA_INDICATED:
			if (spp_info->cb && spp_info->cb->receive_data) {
				spp_info->cb->receive_data(spp_info->app_id, (uint8_t *)packet, (uint32_t)size);
			}
			break;
		default:
			break;
	}

callback_exit:
	os_mutex_unlock(&sppmgr_lock);
}

int bt_manager_spp_profile_start(void)
{
	return btif_spp_start(&bt_manager_spp_callback);
}

int bt_manager_spp_profile_stop(void)
{
	return btif_spp_stop();
}

