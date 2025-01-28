/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager discover.
 */
#define SYS_LOG_NO_NEWLINE
#define SYS_LOG_DOMAIN "bt discover"

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
#include <sys_manager.h>
#include <btservice_api.h>
#include <acts_bluetooth/host_interface.h>
#include "bt_porting_inner.h"

#define BT_DISCOVER_LIST_MAX          4

struct bt_discover_list {
	bd_address_t addr;
	int8_t rssi;
	uint8_t valid:1;
	uint8_t wait_req_name:1;
};

struct bt_discover_info {
	uint8_t discovering:1;
	btsrv_discover_result_cb cb;
	struct bt_discover_list dev_list[BT_DISCOVER_LIST_MAX];
} discover_info;

static void bt_discover_check_finish(void);
static OS_MUTEX_DEFINE(bt_discover_mutex);
static struct bt_discover_info *p_discover = &discover_info;

/*
Minor Device Class field - Audio/Video Major Class
7	6	5	4	3	2	Minor Device Class bit no. of CoD
0	0	0	0	0	0	Uncategorized, code not assigned
0	0	0	0	0	1	Wearable Headset Device
0	0	0	0	1	0	Hands-free Device
0	0	0	0	1	1	(Reserved)
0	0	0	1	0	0	Microphone
0	0	0	1	0	1	Loudspeaker
0	0	0	1	1	0	Headphones
0	0	0	1	1	1	Portable Audio
0	0	1	0	0	0	Car audio
0	0	1	0	0	1	Set-top box
0	0	1	0	1	0	HiFi Audio Device
0	0	1	0	1	1	VCR
0	0	1	1	0	0	Video Camera
0	0	1	1	0	1	Camcorder
0	0	1	1	1	0	Video Monitor
0	0	1	1	1	1	Video Display and Loudspeaker
0	1	0	0	0	0	Video Conferencing
0	1	0	0	0	1	(Reserved)
0	1	0	0	1	0	Gaming/Toy
*/

#define BT_COD_RENDERING_SERVICE_BTI	(0x1 << 18)
#define BT_COD_AUDIO_SERVICE_BTI		(0x1 << 21)
#define BT_COD_MAJOR_DC_MASK			(0x1F00)
#define BT_COD_MAJOR_DC_AUDIO			(0x400)
#define BT_COD_MAJOR_DC_WEARABLE		(0x700)
#define BT_COD_MINOR_DC_MASK			(0xFC)
static const uint8_t bt_support_mdc[] = {0x04, 0x14, 0x18, 0x1C, 0x20, 0x28};

static bool bt_discover_transmit_check_device(int cod)
{
	uint8_t mdc, i;
#ifdef CONFIG_OTA_PRODUCT_SUPPORT
	if ((cod & BT_COD_MAJOR_DC_WEARABLE) != BT_COD_MAJOR_DC_WEARABLE) {
		return false;
	}	
#else
	if (!(cod & (BT_COD_AUDIO_SERVICE_BTI | BT_COD_RENDERING_SERVICE_BTI))) {
		return false;
	}

	if ((cod & BT_COD_MAJOR_DC_MASK) != BT_COD_MAJOR_DC_AUDIO) {
		return false;
	}
#endif

	mdc = (uint8_t)(cod & BT_COD_MINOR_DC_MASK);

	for (i=0; i<sizeof(bt_support_mdc); i++) {
		if (mdc == bt_support_mdc[i]) {
			return true;
		}
	}

	return false;
}

static void bt_discover_add_list(struct btsrv_discover_result *result)
{
	uint8_t i, idle_pos = 0xFF, weakest_pos = 0xFF, save_pos = 0xFF;
	int8_t weakest_rssi = 127;

	for (i=0; i<BT_DISCOVER_LIST_MAX; i++) {
		if (p_discover->dev_list[i].valid) {
			if (!memcmp(&p_discover->dev_list[i].addr, &result->addr, sizeof(result->addr))) {
				if (p_discover->dev_list[i].wait_req_name && result->len) {
					p_discover->dev_list[i].wait_req_name = 0;
				}

				if (result->rssi > p_discover->dev_list[i].rssi) {
					p_discover->dev_list[i].rssi = result->rssi;
				}

				return;
			}

			if (weakest_rssi > p_discover->dev_list[i].rssi) {
				weakest_rssi = p_discover->dev_list[i].rssi;
				weakest_pos = i;
			}
		} else {
			if (idle_pos == 0xFF) {
				idle_pos = i;
			}
		}
	}

	if (idle_pos != 0xFF) {
		save_pos = idle_pos;
	} else {
		if (result->rssi > weakest_rssi) {
			save_pos = weakest_pos;
		}
	}

	if (save_pos != 0xFF) {
		memcpy(&p_discover->dev_list[save_pos].addr, &result->addr, sizeof(result->addr));
		p_discover->dev_list[save_pos].valid = 1;
		p_discover->dev_list[save_pos].rssi = result->rssi;
		if (result->len) {
			p_discover->dev_list[save_pos].wait_req_name = 0;
		} else {
			p_discover->dev_list[save_pos].wait_req_name = 1;
		}
	}
}

static void bt_discover_remote_name_cb(bt_addr_t *addr, uint8_t *name)
{
	int i;
	struct btsrv_discover_result result;

	os_mutex_lock(&bt_discover_mutex, OS_FOREVER);

	if (addr && name) {
		for (i=0; i<BT_DISCOVER_LIST_MAX; i++) {
			if (p_discover->dev_list[i].valid &&
				(!memcmp(&p_discover->dev_list[i].addr, addr, sizeof(bt_addr_t)))) {
				memset(&result, 0, sizeof(result));
				memcpy(&result.addr, addr, sizeof(bt_addr_t));
				result.len = strlen(name);
				result.name = name;
				result.rssi = p_discover->dev_list[i].rssi;
				if (p_discover->cb) {
					p_discover->cb(&result);
				}
			}
		}
	}

	bt_discover_check_finish();
	os_mutex_unlock(&bt_discover_mutex);
}

static void bt_discover_check_finish(void)
{
	int i, ret;
	struct btsrv_discover_result result;

	for (i=0; i<BT_DISCOVER_LIST_MAX; i++) {
		if (p_discover->dev_list[i].valid) {
			if (p_discover->dev_list[i].wait_req_name) {
				SYS_LOG_INF("Req name %2x%2x%2x%2x%2x%2x", p_discover->dev_list[i].addr.val[5],
					p_discover->dev_list[i].addr.val[4], p_discover->dev_list[i].addr.val[3],
					p_discover->dev_list[i].addr.val[2], p_discover->dev_list[i].addr.val[1],
					p_discover->dev_list[i].addr.val[0]);
				p_discover->dev_list[i].wait_req_name = 0;
				ret = btif_br_remote_name_req(&p_discover->dev_list[i].addr, bt_discover_remote_name_cb);
				if (ret) {
					continue;
				} else {
					return;
				}
			}
		}
	}

	/* Discover all finish */
	memset(&result, 0, sizeof(result));
	p_discover->discovering = 0;
	result.discover_finish = 1;
	if (p_discover->cb) {
		p_discover->cb(&result);
	}
	p_discover->cb = NULL;
}

static void bt_discover_result_cb(void *result)
{
	int cod;
	struct btsrv_discover_result *cb_result = result;

	os_mutex_lock(&bt_discover_mutex, OS_FOREVER);

	if (!p_discover->discovering) {
		goto cb_exit;
	}

	if (cb_result->discover_finish) {
		bt_discover_check_finish();
	} else {
		cod = (cb_result->cod[2]<<16) + (cb_result->cod[1]<<8) + cb_result->cod[0];
		if (!bt_discover_transmit_check_device(cod)) {
			goto cb_exit;
		}

		bt_discover_add_list(cb_result);
		if (p_discover->cb) {
			p_discover->cb(result);
		}
	}

cb_exit:
	os_mutex_unlock(&bt_discover_mutex);
}

int bt_manager_br_start_discover(struct btsrv_discover_param *param)
{
	int ret;
	struct btsrv_discover_param dis_param;

	os_mutex_lock(&bt_discover_mutex, OS_FOREVER);

	if (p_discover->discovering) {
		ret = -EBUSY;
		goto start_exit;
	}

	dis_param.length = param->length;
	dis_param.num_responses = param->num_responses;
	dis_param.cb = &bt_discover_result_cb;
	ret = btif_br_start_discover(&dis_param);
	if (ret == 0) {
		p_discover->cb = param->cb;
		p_discover->discovering = 1;
		memset(p_discover->dev_list, 0, sizeof(p_discover->dev_list));
	}

start_exit:
	os_mutex_unlock(&bt_discover_mutex);
	return ret;
}

int bt_manager_br_stop_discover(void)
{
	int ret = 0;

	os_mutex_lock(&bt_discover_mutex, OS_FOREVER);

	if (!p_discover->discovering) {
		goto stop_exit;
	}

	p_discover->discovering = 0;
	p_discover->cb = NULL;
	ret = btif_br_stop_discover();

stop_exit:
	os_mutex_unlock(&bt_discover_mutex);
	return ret;
}
