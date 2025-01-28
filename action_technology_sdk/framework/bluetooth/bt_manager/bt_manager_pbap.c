/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager PBAP profile.
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

#define BTMGR_MAX_PBAP_NUM	2
#define MGR_PBAP_INDEX_TO_APPID(x)		((x)|0x80)
#define MGR_PBAP_APPID_TO_INDEX(x)		((x)&(~0x80))

struct btmgr_pbap_info {
	uint8_t app_id;
	struct btmgr_pbap_cb *cb;
};

static struct btmgr_pbap_info mgr_pbap_info[BTMGR_MAX_PBAP_NUM];

static void *btmgr_pbap_find_free_info(void)
{
	int i;

	for (i = 0; i < BTMGR_MAX_PBAP_NUM; i++) {
		if (mgr_pbap_info[i].app_id == 0) {
			mgr_pbap_info[i].app_id = MGR_PBAP_INDEX_TO_APPID(i);
			return &mgr_pbap_info[i];
		}
	}

	return NULL;
}

static void btmgr_pbap_free_info(struct btmgr_pbap_info *info)
{
	memset(info, 0, sizeof(struct btmgr_pbap_info));
}

static void *btmgr_pbap_find_info_by_app_id(uint8_t app_id)
{
	int i;

	for (i = 0; i < BTMGR_MAX_PBAP_NUM; i++) {
		if (mgr_pbap_info[i].app_id == app_id) {
			return &mgr_pbap_info[i];
		}
	}

	return NULL;
}

static void btmgr_pbap_callback(btsrv_pbap_event_e event, uint8_t app_id, void *data, uint8_t size)
{
	struct btmgr_pbap_info *info = btmgr_pbap_find_info_by_app_id(app_id);
	uint16_t *max_size;

	if (!info) {
		return;
	}

	switch (event) {
	case BTSRV_PBAP_CONNECT_FAILED:
		if (info->cb->connect_failed) {
			info->cb->connect_failed(app_id);
		}
		btmgr_pbap_free_info(info);
		break;
	case BTSRV_PBAP_CONNECTED:
		if (info->cb->connected) {
			info->cb->connected(app_id);
		}
		break;
	case BTSRV_PBAP_DISCONNECTED:
		if (info->cb->disconnected) {
			info->cb->disconnected(app_id);
		}
		btmgr_pbap_free_info(info);
		break;
	case BTSRV_PBAP_MAX_SIZE:
		if (info->cb->max_size) {
			max_size = data;
			info->cb->max_size(app_id, (*max_size));
		}
		break;
	case BTSRV_PBAP_VCARD_RESULT:
		if (info->cb->result) {
			info->cb->result(app_id, data, size);
		}
		break;
	case BTSRV_PBAP_SETPATH_FINISH:
		if (info->cb->setpath_finish) {
			info->cb->setpath_finish(app_id);
		}
		break;
	case BTSRV_PBAP_SEARCH_RESULT:
		if (info->cb->search_result) {
			info->cb->search_result(app_id, data, size);
		}
		break;
	case BTSRV_PBAP_GET_VCARD_FINISH:
		if (info->cb->get_vcard_finish) {
			info->cb->get_vcard_finish(app_id);
		}
		break;
	case BTSRV_PBAP_END_OF_BODY:
		if (info->cb->end_of_body) {
			info->cb->end_of_body(app_id);
		}
		break;
	case BTSRV_PBAP_ABORT_FINISH:
		if (info->cb->abort) {
			info->cb->abort(app_id);
		}
		break;
	}
}

uint8_t btmgr_pbap_connect(bd_address_t *bd, struct btmgr_pbap_cb *cb)
{
	int ret;
	struct btmgr_pbap_info *info;
	struct bt_pbap_op_param param;

	if (!bd || !cb) {
		return 0;
	}

	info = btmgr_pbap_find_free_info();
	if (!info) {
		return 0;
	}

	info->cb = cb;

	memcpy(&param.bd, bd, sizeof(bd_address_t));
	param.cmd = BTSRV_PBAP_OP_CMD_CONNECT;
	param.app_id = info->app_id;
	param.cb = &btmgr_pbap_callback;
	ret = btif_pbap_cmd_op(&param);
	if (ret) {
		btmgr_pbap_free_info(info);
		return 0;
	}

	return info->app_id;
}

int btmgr_pbap_disconnect(uint8_t app_id)
{
	struct bt_pbap_op_param param;
	struct btmgr_pbap_info *info = btmgr_pbap_find_info_by_app_id(app_id);

	if (!info) {
		return -EIO;
	}

	param.cmd = BTSRV_PBAP_OP_CMD_DISCONNECT;
	param.app_id = info->app_id;
	return btif_pbap_cmd_op(&param);
}

int btmgr_pbap_get_size(uint8_t app_id, char *path)
{
	struct bt_pbap_op_param param;
	struct btmgr_pbap_info *info = btmgr_pbap_find_info_by_app_id(app_id);

	if (!path || !info) {
		return -EIO;
	}

	param.cmd = BTSRV_PBAP_OP_CMD_GET_SIZE;
	param.pb_path = path;
	param.app_id = info->app_id;
	return btif_pbap_cmd_op(&param);
}

int btmgr_pbap_get_pb(uint8_t app_id, char *path, uint32_t filter)
{
	struct bt_pbap_op_param param;
	struct btmgr_pbap_info *info = btmgr_pbap_find_info_by_app_id(app_id);

	if (!path || !info) {
		return -EIO;
	}

	param.cmd = BTSRV_PBAP_OP_CMD_GET_PB;
	param.pb_path = path;
	param.pb_filter = filter;
	param.app_id = info->app_id;
	return btif_pbap_cmd_op(&param);
}

int btmgr_pbap_setpath(uint8_t app_id, char *path)
{
	struct bt_pbap_op_param param;
	struct btmgr_pbap_info *info = btmgr_pbap_find_info_by_app_id(app_id);

	if (!path || !info) {
		return -EIO;
	}

	param.cmd = BTSRV_PBAP_OP_CMD_SET_PATH;
	param.pb_path = path;
	param.app_id = info->app_id;
	return btif_pbap_cmd_op(&param);
}

int btmgr_pbap_get_vcard(uint8_t app_id, char *name, uint8_t len, uint32_t filter)
{
	struct bt_pbap_op_param param;
	struct btmgr_pbap_info *info = btmgr_pbap_find_info_by_app_id(app_id);

	if (!name || !info || (len > sizeof(param.vcard_name))) {
		return -EIO;
	}

	param.cmd = BTSRV_PBAP_OP_CMD_GET_VCARD;
	param.app_id = info->app_id;
	param.vcard_name_len = len;
	param.vc_filter = filter;
	if (len) {
		memcpy(param.vcard_name, name, len);
	}
	return btif_pbap_cmd_op(&param);
}

int btmgr_pbap_get_vcard_continue(uint8_t app_id)
{
	struct bt_pbap_op_param param;
	struct btmgr_pbap_info *info = btmgr_pbap_find_info_by_app_id(app_id);

	if (!info) {
		return -EIO;
	}

	param.cmd = BTSRV_PBAP_OP_CMD_GET_CONTINUE;
	param.app_id = info->app_id;
	return btif_pbap_cmd_op(&param);
}

int btmgr_pbap_listing(uint8_t app_id, uint8_t order, uint8_t attr, char *value, uint8_t len)
{
	struct bt_pbap_op_param param;
	struct btmgr_pbap_info *info = btmgr_pbap_find_info_by_app_id(app_id);

	if (!info || (len > sizeof(param.search_value))) {
		return -EIO;
	}

	param.cmd = BTSRV_PBAP_OP_CMD_LISTING;
	param.app_id = info->app_id;
	param.order = order;
	param.search_attr = attr;
	param.search_len = len;
	if (len) {
		memcpy(param.search_value, value, len);
	}
	return btif_pbap_cmd_op(&param);
}

int btmgr_pbap_abort(uint8_t app_id)
{
	struct bt_pbap_op_param param;
	struct btmgr_pbap_info *info = btmgr_pbap_find_info_by_app_id(app_id);

	if (!info) {
		return -EIO;
	}

	param.cmd = BTSRV_PBAP_OP_CMD_ABORT;
	param.app_id = info->app_id;
	return btif_pbap_cmd_op(&param);
}
