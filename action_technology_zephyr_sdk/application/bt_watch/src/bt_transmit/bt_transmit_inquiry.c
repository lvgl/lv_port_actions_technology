/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief lccal player
 */

#define LOG_MODULE_CUSTOMER

#include <os_common_api.h>
#include <audio_system.h>
#include <ringbuff_stream.h>
#include <assert.h>

#include "bt_transmit.h"

LOG_MODULE_DECLARE(bt_transmit, LOG_LEVEL_INF);

#define MAX_BT_DEVICE_CNT	4
#define FAKE_CONNECTED_RSSI 0

static struct bt_device_info_t bt_device_info[MAX_BT_DEVICE_CNT];
static uint8_t bt_device_count = 0;
static bt_transmit_inquiry_handler_t bt_inquiry_handler = NULL;

bool bt_transmit_check_dev_connected(uint8_t *addr)
{
	struct btmgr_connect_dev_info info[2];
	int connected_count = 0;

	memset(info, 0, sizeof(struct btmgr_connect_dev_info)*2);
	connected_count = bt_manager_get_connected_dev_info(info, 2);

	if (connected_count > 2) {
		connected_count = 2;
	}

	while (connected_count) {
		if (addr[0] == info[connected_count - 1].addr.val[0] &&
			addr[1] == info[connected_count - 1].addr.val[1] &&
			addr[2] == info[connected_count - 1].addr.val[2] &&
			addr[3] == info[connected_count - 1].addr.val[3] &&
			addr[4] == info[connected_count - 1].addr.val[4] &&
			addr[5] == info[connected_count - 1].addr.val[5] &&
			info[connected_count - 1].is_trs)
			return true;

		connected_count--;
	}
	return false;
}

/*
 * Sort bt_device_info_t comparing function
 *
 * @retval > 0 a sorted before b
 * @retval < 0 a sorted after b
 * @retval 0 keep the sorting sequence
 */
static int _sorted_before(const struct bt_device_info_t *a, const struct bt_device_info_t *b)
{
	if (a->connected == b->connected) {
		return a->rssi - b->rssi;
	}

	return a->connected ? 1 : -1;
}

static void _bt_transmit_sort_result(void)
{
	struct bt_device_info_t tmp;
	int i, j;

	if (bt_device_count < 2)
		return;

	for (i = bt_device_count - 1; i > 0; i--) {
		bool sorted = true;

		for (j = bt_device_count - 1; j > bt_device_count - 1 - i; j--) {
			if (_sorted_before(&bt_device_info[j], &bt_device_info[j - 1]) > 0) {
				memcpy(&tmp, &bt_device_info[j], sizeof(tmp));
				memcpy(&bt_device_info[j], &bt_device_info[j - 1], sizeof(tmp));
				memcpy(&bt_device_info[j - 1], &tmp, sizeof(tmp));

				sorted = false;
			}
		}

		if (sorted)
			break;
	}
}

static void _bt_transmit_add_connected_device(void)
{
	struct btmgr_connect_dev_info connect_info[2];
	int i, count = 0;

	memset(connect_info, 0, sizeof(connect_info));
	count = bt_manager_get_connected_dev_info(connect_info, 2);

	if (count > 2) {
		count = 2;
	}

	for (i = 0; i < count; i++) {
		struct bt_device_info_t *info = &bt_device_info[bt_device_count];
		if (bt_device_count >= MAX_BT_DEVICE_CNT)
			break;

		if (connect_info[i].is_trs == 0)
			continue;

		int len = strlen(connect_info[i].name);
		info->name = app_mem_malloc(len + 1);
		assert(info->name != NULL);

		strcpy(info->name, connect_info[i].name);
		memcpy(&info->addr, &connect_info[i].addr, sizeof(info->addr));

		info->rssi = FAKE_CONNECTED_RSSI; /* fake the rssi */
		info->connected = 1;
		info->paired = 1;

		bt_device_count++;
	}
}

static bool _bt_transmit_append_inquiry_result(struct btsrv_discover_result *result)
{
	struct bt_device_info_t *info = NULL;
	int8_t weakest_rssi = 127;
	int weakest_pos = -1;
	int i;

	for (i = 0; i < bt_device_count; i++) {
		if (!memcmp(bt_device_info[i].addr.val, result->addr.val, 6)) {
			/* if the rssi is fake or getting stronger, upate the value */
			if (result->rssi > bt_device_info[i].rssi || bt_device_info[i].rssi == FAKE_CONNECTED_RSSI) {
				bt_device_info[i].rssi = result->rssi;
				SYS_LOG_INF("%s update rssi %d", bt_device_info[i].name, bt_device_info[i].rssi);
			}

			return true;
		}

		/* donot replace connected device */
		if (!bt_device_info[i].connected && bt_device_info[i].rssi < weakest_rssi) {
			weakest_rssi = bt_device_info[i].rssi;
			weakest_pos = i;
		}
	}

	if (bt_device_count < MAX_BT_DEVICE_CNT) {
		info = &bt_device_info[bt_device_count++];
	} else if ((result->rssi > weakest_rssi) && (weakest_pos != -1)) {
		info = &bt_device_info[weakest_pos];
		app_mem_free(info->name);

		SYS_LOG_INF("Replace by mac %02x:%02x:%02x:%02x:%02x:%02x rssi %i\n",
			result->addr.val[5], result->addr.val[4], result->addr.val[3],
			result->addr.val[2], result->addr.val[1], result->addr.val[0],
			result->rssi);
	} else {
		return false;
	}

	info->name = app_mem_malloc(result->len + 1);
	assert(info->name != NULL);

	memcpy(info->name, result->name, result->len);
	info->name[result->len] = 0;

	memcpy(&info->addr, &result->addr, sizeof(result->addr));

	info->rssi = result->rssi;
	info->connected = bt_transmit_check_dev_connected(result->addr.val) ? 1 : 0;
	info->paired = bt_manager_get_addr_linkkey(&result->addr, NULL) ? 0 : 1;

	SYS_LOG_INF("Name %s, connected %d, paired %d, rssi %d", info->name,
		info->connected, info->paired, info->rssi);

	return true;
}

static void bt_transmit_inquiry_result(void *result)
{
	struct btsrv_discover_result *cb_result = result;
	int i;

	if (cb_result->discover_finish) {
		SYS_LOG_INF("Discover finish cnt %d", bt_device_count);
		bt_manager_br_stop_discover();

		for (i = 0; i < bt_device_count; i++) {
			assert (bt_device_info[i].name != NULL);

			SYS_LOG_INF("%s: connected %d, paired %d, rssi %d",
				bt_device_info[i].name, bt_device_info[i].connected,
				bt_device_info[i].paired, bt_device_info[i].rssi);

			app_mem_free(bt_device_info[i].name);
			bt_device_info[i].name = NULL;
		}

		bt_device_count = 0;
	} else if (cb_result->len) {
		if (_bt_transmit_append_inquiry_result(cb_result) == true) {
			_bt_transmit_sort_result();

			if (bt_inquiry_handler)
				bt_inquiry_handler(bt_device_info, bt_device_count);
		}
	}
}

void bt_transmit_inquiry_restart(bt_transmit_inquiry_handler_t handler)
{
	struct btsrv_discover_param param;

	bt_manager_br_stop_discover();

	SYS_LOG_INF("Discover start (handler=%p)", handler);

	bt_inquiry_handler = handler;

	/* update connected device first */
	_bt_transmit_add_connected_device();
	if (bt_device_count > 0 && bt_inquiry_handler) {
		bt_inquiry_handler(bt_device_info, bt_device_count);
	}

	param.cb = &bt_transmit_inquiry_result;
	param.length = 5;
	param.num_responses = 0;

	bt_manager_br_start_discover(&param);
}

void bt_transmit_inquiry_start(bt_transmit_inquiry_handler_t handler)
{
	bt_transmit_inquiry_restart(handler);
}

void bt_transmit_inquiry_stop(void)
{
	bt_manager_br_stop_discover();

#ifdef CONFIG_OTA_PRODUCT_SUPPORT
	int i;

	SYS_LOG_INF("Discover finish cnt %d", bt_device_count);
	for (i = 0; i < bt_device_count; i++) {
		assert (bt_device_info[i].name != NULL);
	
		SYS_LOG_INF("%s: connected %d, paired %d, rssi %d",
			bt_device_info[i].name, bt_device_info[i].connected,
			bt_device_info[i].paired, bt_device_info[i].rssi);
	
		app_mem_free(bt_device_info[i].name);
		bt_device_info[i].name = NULL;
	}
	
	bt_device_count = 0;
#endif
}

void bt_transmit_inquiry_start_notify(bt_transmit_inquiry_handler_t handler)
{
	char *current_app = app_manager_get_current_app();
	struct app_msg new_msg = { 0 };

	new_msg.type = MSG_BT_EVENT;
	new_msg.cmd = BT_TRS_INQUIRY_START_EVENT;
	new_msg.ptr = handler;

	if (current_app) {
		send_async_msg(current_app, &new_msg);
	}
}

void bt_transmit_inquiry_restart_notify(bt_transmit_inquiry_handler_t handler)
{
	char *current_app = app_manager_get_current_app();
	struct app_msg new_msg = { 0 };

	new_msg.type = MSG_BT_EVENT;
	new_msg.cmd = BT_TRS_INQUIRY_RESTART_EVENT;
	new_msg.ptr = handler;

	if (current_app) {
		send_async_msg(current_app, &new_msg);
	}
}

void bt_transmit_inquiry_stop_notify(void)
{
	char *current_app = app_manager_get_current_app();
	struct app_msg new_msg = { 0 };

	new_msg.type = MSG_BT_EVENT;
	new_msg.cmd = BT_TRS_INQUIRY_STOP_EVENT;

	if (current_app) {
		send_async_msg(current_app, &new_msg);
	}
}


