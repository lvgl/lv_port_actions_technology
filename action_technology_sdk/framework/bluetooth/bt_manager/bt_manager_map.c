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
#include <shell/shell.h>
#ifdef CONFIG_ALARM_MANAGER
#include <alarm_manager.h>
#endif
#define BTMGR_MAX_MAP_NUM	2
#define MGR_MAP_INDEX_TO_APPID(x)		((x)|0x80)
#define MGR_MAP_APPID_TO_INDEX(x)		((x)&(~0x80))

struct btmgr_map_info {
	uint8_t app_id;
	struct btmgr_map_cb *cb;
};

static struct btmgr_map_info mgr_map_info[BTMGR_MAX_MAP_NUM];
static struct btmgr_map_time map_tm;

extern void *app_manager_get_current_app(void);

static void *btmgr_map_find_free_info(void)
{
	uint8_t i;

	for (i = 0; i < BTMGR_MAX_MAP_NUM; i++) {
		if (mgr_map_info[i].app_id == 0) {
			mgr_map_info[i].app_id = MGR_MAP_INDEX_TO_APPID(i);
			return &mgr_map_info[i];
		}
	}
	return NULL;
}

static void btmgr_map_free_info(struct btmgr_map_info *info)
{
	memset(info, 0, sizeof(struct btmgr_map_info));
}

static void *btmgr_map_find_info_by_app_id(uint8_t app_id)
{
	uint8_t i;

	for (i = 0; i < BTMGR_MAX_MAP_NUM; i++) {
		if (mgr_map_info[i].app_id == app_id) {
			return &mgr_map_info[i];
		}
	}
	return NULL;
}

static void btmgr_map_callback(btsrv_map_event_e event, uint8_t app_id, void *data, uint8_t size)
{
	struct btmgr_map_info *info = btmgr_map_find_info_by_app_id(app_id);

	if (!info) {
		return;
	}

	switch (event) {
	case BTSRV_MAP_CONNECT_FAILED:
		if (info->cb->connect_failed) {
			info->cb->connect_failed(app_id);
		}
		btmgr_map_free_info(info);
		break;
	case BTSRV_MAP_CONNECTED:
		if (info->cb->connected) {
			info->cb->connected(app_id);
		}
		break;
	case BTSRV_MAP_DISCONNECTED:
		if (info->cb->disconnected) {
			info->cb->disconnected(app_id);
		}
		btmgr_map_free_info(info);
		break;
	case BTSRV_MAP_SET_PATH_FINISHED:
		if (info->cb->set_path_finished) {
			info->cb->set_path_finished(app_id);
		}
		break;
	case BTSRV_MAP_MESSAGES_RESULT:
		if (info->cb->result) {
			info->cb->result(app_id, data, size);
		}
		break;
	}
}

uint8_t btmgr_map_client_connect(bd_address_t *bd, char *path, struct btmgr_map_cb *cb)
{
	int ret;
	struct btmgr_map_info *info;
	struct bt_map_connect_param param;

	if (!path || !cb) {
		return 0;
	}

	info = btmgr_map_find_free_info();
	if (!info) {
		return 0;
	}

	info->cb = cb;

	memcpy(&param.bd, bd, sizeof(bd_address_t));
	param.app_id = info->app_id;
	param.map_path = path;
	param.cb = &btmgr_map_callback;
	ret = btif_map_client_connect(&param);
	if (ret) {
		btmgr_map_free_info(info);
		return 0;
	}

	return info->app_id;
}

uint8_t btmgr_map_client_set_folder(uint8_t app_id,char *path, uint8_t flags)
{
	struct bt_map_set_folder_param param;
	struct btmgr_map_info *info = btmgr_map_find_info_by_app_id(app_id);
	
	if (!info) {
		return -EIO;
	}
	
	param.app_id = info->app_id;
	param.map_path = path;
	param.flags = flags;
	
	return btif_map_client_set_folder(&param);
}

uint8_t btmgr_map_get_messsage(bd_address_t *bd, char *path, struct btmgr_map_cb *cb)
{
	int ret;
	struct btmgr_map_info *info;
	struct bt_map_get_param param;

	if (!path || !cb) {
		return 0;
	}

	info = btmgr_map_find_free_info();
	if (!info) {
		return 0;
	}

	info->cb = cb;

	memcpy(&param.bd, bd, sizeof(bd_address_t));
	param.app_id = info->app_id;
	param.map_path = path;
	param.cb = &btmgr_map_callback;
	ret = btif_map_get_message(&param);
	if (ret) {
		btmgr_map_free_info(info);
		return 0;
	}

	return info->app_id;
}

int btmgr_map_get_folder_listing(uint8_t app_id)
{
	struct btmgr_map_info *info = btmgr_map_find_info_by_app_id(app_id);
	
	if (!info) {
		return -EIO;
	}

	return btif_map_get_folder_listing(info->app_id);
}

int btmgr_map_get_messages_listing(uint8_t app_id,uint16_t max_cn,uint32_t mask)
{
	struct btmgr_map_info *info = btmgr_map_find_info_by_app_id(app_id);
	struct bt_map_get_messages_listing_param param;
	
	if (!info) {
		return -EIO;
	}

    param.app_id = info->app_id;
    param.max_list_count = max_cn;
    param.parameter_mask = mask;

	return btif_map_get_messages_listing(&param);
}

int btmgr_map_abort_get(uint8_t app_id)
{
	struct btmgr_map_info *info = btmgr_map_find_info_by_app_id(app_id);

	if (!info) {
		return -EIO;
	}

	return btif_map_abort_get(info->app_id);
}

int btmgr_map_client_disconnect(uint8_t app_id)
{
	struct btmgr_map_info *info = btmgr_map_find_info_by_app_id(app_id);

	if (!info) {
		return -EIO;
	}

	return btif_map_client_disconnect(info->app_id);
}

#define BT_MAP_TIME_PATH        "telecom/msg/inbox"

#define MAP_APP_PARAMETER_DATETIME  1

static void time_map_connect_failed_cb(uint8_t app_id)
{
	SYS_LOG_INF("%d\n", app_id);
}

static void time_map_connected_cb(uint8_t app_id)
{
	SYS_LOG_INF("%d\n", app_id);
}

static void time_map_disconnected_cb(uint8_t app_id)
{
	SYS_LOG_INF("%d\n", app_id);
}

static void time_map_set_path_finished(uint8_t app_id)
{
	SYS_LOG_INF("%d\n", app_id);

	btmgr_map_get_messages_listing(app_id, 1, 0x0A);
}


//20210112T200327+0800
static int _bt_map_set_time(uint8_t *time)
{

	uint8_t tmp_buf[5];

	memset(&map_tm, 0, sizeof(map_tm));

	/**year*/
	memset(tmp_buf, 0 , sizeof(tmp_buf));
	memcpy( tmp_buf, time, 4);
	map_tm.tm_year = atoi(tmp_buf);

	/**month*/
	memset(tmp_buf, 0 , sizeof(tmp_buf));
	memcpy(tmp_buf, time + 4, 2);
	map_tm.tm_mon = atoi(tmp_buf);

	/**data*/
	memset(tmp_buf, 0 , sizeof(tmp_buf));
	memcpy(tmp_buf, time + 6, 2);
	map_tm.tm_mday = atoi(tmp_buf);


	/**hour*/
	memset(tmp_buf, 0 , sizeof(tmp_buf));
	memcpy(tmp_buf, time + 9, 2);
	map_tm.tm_hour = atoi(tmp_buf);

	/**min*/
	memset(tmp_buf, 0 , sizeof(tmp_buf));
	memcpy(tmp_buf, time + 11, 2);
	map_tm.tm_min = atoi(tmp_buf);

	/**sec*/
	memset(tmp_buf, 0 , sizeof(tmp_buf));
	memcpy(tmp_buf, time + 13, 2);
	map_tm.tm_sec = atoi(tmp_buf);


	SYS_LOG_INF("time %s year %d mon %d dat %d hour %d min %d sec %d \n",time,
				 map_tm.tm_year, map_tm.tm_mon, map_tm.tm_mday,
				 map_tm.tm_hour,map_tm.tm_min,map_tm.tm_sec);

	struct app_msg  msg = {0};
	msg.type = MSG_BT_MGR_EVENT;
    msg.cmd = BT_MAP_SET_TIME_EVENT;
    msg.ptr = &map_tm;
	send_async_msg(app_manager_get_current_app(), &msg);

	return 0;
}
static void time_map_result_cb(uint8_t app_id, struct mgr_map_result *result, uint8_t size)
{
	int i;
    uint8_t datetime_buf[24];

    if(size > 0){
        for (i = 0; i < size; i++) {
            if((result[i].type == MAP_APP_PARAMETER_DATETIME) && (result[i].len < 24)){
                memcpy(datetime_buf,result[i].data,result[i].len);
                datetime_buf[result[i].len] = 0;
				_bt_map_set_time(datetime_buf);
            }
        }
    }
}

static const struct btmgr_map_cb time_map_cb = {
	.connect_failed = time_map_connect_failed_cb,
	.connected = time_map_connected_cb,
	.disconnected = time_map_disconnected_cb,
	.set_path_finished = time_map_set_path_finished,
	.result = time_map_result_cb,
};

int btmgr_map_time_client_connect(bd_address_t *bd)
{
	return btmgr_map_client_connect(bd, BT_MAP_TIME_PATH, (struct btmgr_map_cb *)&time_map_cb);
}
