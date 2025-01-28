/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager connect.
 */
#define SYS_LOG_DOMAIN "bt manager"

#include <os_common_api.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>
#include <sys_event.h>
#include <acts_bluetooth/host_interface.h>
#include <mem_manager.h>
#include <bt_manager.h>
#include "bt_manager_inner.h"
#include <property_manager.h>
#include "btservice_api.h"
#include "bt_porting_inner.h"

#define BT_MAX_AUTOCONN_DEV				3
#define BT_TRY_AUTOCONN_DEV				2
#define BT_BASE_DEFAULT_RECONNECT_TRY	3
#define BT_BASE_STARTUP_RECONNECT_TRY	3
#define BT_BASE_TIMEOUT_RECONNECT_TRY	15
#define BT_TWS_SLAVE_DISCONNECT_RETRY	10
#define BT_BASE_RECONNECT_INTERVAL		6000
#define BT_PROFILE_RECONNECT_TRY		3
#define BT_PROFILE_RECONNECT_INTERVAL	3000

void bt_manager_startup_reconnect(void)
{
	uint8_t connect_phone_num, connect_phone_cnt;
	int cnt, i;
	struct autoconn_info *info;
	struct bt_set_autoconn reconnect_param;

	info = bt_mem_malloc(sizeof(struct autoconn_info)*BT_MAX_AUTOCONN_DEV);
	if (info == NULL) {
		SYS_LOG_ERR("malloc failed");
		return;
	}

	cnt = btif_br_get_auto_reconnect_info(info, BT_MAX_AUTOCONN_DEV);
	if (cnt == 0) {
		goto reconnnect_ext;
	}

	connect_phone_cnt = 0;
	connect_phone_num = bt_manager_config_connect_phone_num();
	cnt = (cnt > BT_TRY_AUTOCONN_DEV) ? BT_TRY_AUTOCONN_DEV : cnt;

	for (i = 0; i < BT_MAX_AUTOCONN_DEV; i++) {
		/* As tws master/slave, connect first */
		if (info[i].addr_valid && (info[i].tws_role == BTSRV_TWS_MASTER || info[i].tws_role == BTSRV_TWS_SLAVE) &&
			(info[i].a2dp || info[i].avrcp || info[i].hfp || info[i].hid)) {
			memset(&reconnect_param, 0, sizeof(reconnect_param));
			memcpy(reconnect_param.addr.val, info[i].addr.val, sizeof(bd_address_t));
			reconnect_param.strategy = BTSRV_AUTOCONN_ALTERNATE;
			reconnect_param.base_try = BT_BASE_STARTUP_RECONNECT_TRY;
			reconnect_param.profile_try = BT_PROFILE_RECONNECT_TRY;
			reconnect_param.base_interval = BT_BASE_RECONNECT_INTERVAL;
			reconnect_param.profile_interval = BT_PROFILE_RECONNECT_INTERVAL;
			btif_br_auto_reconnect(&reconnect_param);

			info[i].addr_valid = 0;
			cnt--;
			if (info[i].tws_role == BTSRV_TWS_SLAVE) {
				goto reconnnect_ext;
			} else {
				break;
			}
		}
	}

	for (i = 0; i < BT_MAX_AUTOCONN_DEV; i++) {
		/* Connect phone second */
		if (cnt == 0 || connect_phone_cnt == connect_phone_num) {
			break;
		}

		if (info[i].addr_valid &&
			(info[i].a2dp || info[i].avrcp || info[i].hfp|| info[i].hid) &&
			(info[i].tws_role == BTSRV_TWS_NONE) && (info[i].trs_mode == 0)) {
			memset(&reconnect_param, 0, sizeof(reconnect_param));
			memcpy(reconnect_param.addr.val, info[i].addr.val, sizeof(bd_address_t));
			reconnect_param.strategy = BTSRV_AUTOCONN_ALTERNATE;
			reconnect_param.base_try = BT_BASE_STARTUP_RECONNECT_TRY;
			reconnect_param.profile_try = BT_PROFILE_RECONNECT_TRY;
			reconnect_param.base_interval = BT_BASE_RECONNECT_INTERVAL;
			reconnect_param.profile_interval = BT_PROFILE_RECONNECT_INTERVAL;
			btif_br_auto_reconnect(&reconnect_param);

			info[i].addr_valid = 0;
			connect_phone_cnt++;
			cnt--;
		}
	}

	for (i = 0; i < BT_MAX_AUTOCONN_DEV; i++) {
		/* Transmit connect third */
		if (cnt == 0) {
			break;
		}

		if (info[i].addr_valid &&
			(info[i].a2dp || info[i].avrcp || info[i].hfp|| info[i].hid) &&
			(info[i].tws_role == BTSRV_TWS_NONE) && (info[i].trs_mode == 1)) {
			memset(&reconnect_param, 0, sizeof(reconnect_param));
			memcpy(reconnect_param.addr.val, info[i].addr.val, sizeof(bd_address_t));
			reconnect_param.strategy = BTSRV_AUTOCONN_ALTERNATE;
			reconnect_param.base_try = BT_BASE_STARTUP_RECONNECT_TRY;
			reconnect_param.profile_try = BT_PROFILE_RECONNECT_TRY;
			reconnect_param.base_interval = BT_BASE_RECONNECT_INTERVAL;
			reconnect_param.profile_interval = BT_PROFILE_RECONNECT_INTERVAL;
			btif_br_auto_reconnect(&reconnect_param);

			info[i].addr_valid = 0;
			cnt--;
			break;		/* Only connect one device  */
		}
	}

reconnnect_ext:
	bt_mem_free(info);
}

void bt_manager_startup_reconnect_phone(void)
{
	uint8_t connect_phone_num, connect_phone_cnt;
	int cnt, i;
	struct autoconn_info *info;
	struct bt_set_autoconn reconnect_param;

	info = bt_mem_malloc(sizeof(struct autoconn_info)*BT_MAX_AUTOCONN_DEV);
	if (info == NULL) {
		SYS_LOG_ERR("malloc failed");
		return;
	}

	cnt = btif_br_get_auto_reconnect_info(info, BT_MAX_AUTOCONN_DEV);
	if (cnt == 0) {
		goto reconnnect_ext;
	}

	connect_phone_cnt = 0;
	connect_phone_num = bt_manager_config_connect_phone_num();

	cnt = (cnt > BT_TRY_AUTOCONN_DEV) ? BT_TRY_AUTOCONN_DEV : cnt;


	for (i = 0; i < BT_TRY_AUTOCONN_DEV; i++) {
		if (info[i].addr_valid &&
			(info[i].a2dp || info[i].avrcp || info[i].hfp|| info[i].hid)) {
			if ((connect_phone_cnt == connect_phone_num) ||
				(info[i].tws_role != BTSRV_TWS_NONE) || info[i].trs_mode) {
				/* Config connect phone number */
				continue;
			}

			memset(&reconnect_param, 0, sizeof(reconnect_param));
			memcpy(reconnect_param.addr.val, info[i].addr.val, sizeof(bd_address_t));
			reconnect_param.strategy = BTSRV_AUTOCONN_ALTERNATE;
			reconnect_param.base_try = BT_BASE_STARTUP_RECONNECT_TRY;
			reconnect_param.profile_try = BT_PROFILE_RECONNECT_TRY;
			reconnect_param.base_interval = BT_BASE_RECONNECT_INTERVAL;
			reconnect_param.profile_interval = BT_PROFILE_RECONNECT_INTERVAL;
			btif_br_auto_reconnect(&reconnect_param);
			connect_phone_cnt++;
			cnt--;
			if (cnt == 0) {
				/* Reconnect tow device,
				 *  or as tws slave, just reconnect master, not reconnect phone
				 */
				break;
			}
		}
	}

reconnnect_ext:
	bt_mem_free(info);
}

static bool bt_manager_check_reconnect_enable(void)
{
	bool auto_reconnect = true;
	char temp[16];

	memset(temp, 0, sizeof(temp));

#ifdef CONFIG_PROPERTY
	if (property_get(CFG_AUTO_RECONNECT, temp, 16) > 0) {
		if (strcmp(temp, "false") == 0) {
			auto_reconnect = false;
		}
	}
#endif

	return auto_reconnect;
}

void bt_manager_disconnected_reason(void *param)
{
	struct bt_disconnect_reason *p_param = (struct bt_disconnect_reason *)param;
	struct bt_set_autoconn reconnect_param;

	SYS_LOG_INF("BMC tws_role %d", p_param->tws_role);

	if (!bt_manager_check_reconnect_enable()) {
		SYS_LOG_WRN("BMC disable do reconnect\n");
		return;
	}

	if (p_param->tws_role == BTSRV_TWS_MASTER) {
		/* Just let slave device do reconnect */
		return;
	}

	if (p_param->reason != BT_HCI_ERR_REMOTE_USER_TERM_CONN &&
		p_param->reason != BT_HCI_ERR_REMOTE_POWER_OFF &&
		p_param->reason != BT_HCI_ERR_LOCALHOST_TERM_CONN &&
		p_param->reason != BT_HCI_ERR_AUTH_FAIL){
		memset(&reconnect_param, 0, sizeof(reconnect_param));
		memcpy(reconnect_param.addr.val, p_param->addr.val, sizeof(bd_address_t));
		reconnect_param.strategy = BTSRV_AUTOCONN_ALTERNATE;

		if (p_param->tws_role == BTSRV_TWS_SLAVE) {
			reconnect_param.base_try = BT_TWS_SLAVE_DISCONNECT_RETRY;
		} else {
			reconnect_param.base_try = BT_BASE_DEFAULT_RECONNECT_TRY;
		}

		if (p_param->tws_role == BTSRV_TWS_NONE &&
			((p_param->reason == BT_HCI_ERR_CONN_TIMEOUT)
			    || (p_param->reason == BT_HCI_ERR_LL_RESP_TIMEOUT))) {
			reconnect_param.base_try = BT_BASE_TIMEOUT_RECONNECT_TRY;
		}

		reconnect_param.profile_try = BT_PROFILE_RECONNECT_TRY;
		reconnect_param.base_interval = BT_BASE_RECONNECT_INTERVAL;
		reconnect_param.profile_interval = BT_PROFILE_RECONNECT_INTERVAL;
		btif_br_auto_reconnect(&reconnect_param);
	}
}

static bool halted_phone = false;

void bt_manager_halt_phone(void)
{
#ifdef CONFIG_TWS
	/**only master need halt phone*/
	if (bt_manager_tws_get_dev_role() != BTSRV_TWS_MASTER) {
		return;
	}
#endif
	bt_manager_set_visible(false);
	bt_manager_set_connectable(false);
	btif_br_auto_reconnect_stop();
	btif_br_disconnect_device(BTSRV_DISCONNECT_PHONE_MODE);
	halted_phone = true;
	SYS_LOG_INF("BMC halt phone");
}

void bt_manager_resume_phone(void)
{
	if(halted_phone) {
		bt_manager_startup_reconnect_phone();
		bt_manager_set_visible(true);
		bt_manager_set_connectable(true);
		halted_phone = false;
		SYS_LOG_INF("BMC resume phone");
	}
}

void bt_manager_set_visible(bool visible)
{
	btif_br_set_discoverable(visible);
}

void bt_manager_set_connectable(bool connectable)
{
	btif_br_set_connnectable(connectable);
}

bool bt_manager_is_auto_reconnect_runing(void)
{
	return btif_br_is_auto_reconnect_runing();
}

int bt_manager_stop_auto_reconnect(void)
{
	return btif_br_auto_reconnect_stop();
}

int bt_manager_get_addr_linkkey(bd_address_t *addr, uint8_t *linkkey)
{
	int ret;
	uint8_t key[16];

	ret = btif_br_get_addr_linkkey(addr, key);
	if (ret == 0 && linkkey) {
		memcpy(linkkey, key, 16);
	}

	return ret;
}

int bt_manager_get_linkkey(struct bt_linkkey_info *info, uint8_t cnt)
{
	return btif_br_get_linkkey(info, cnt);
}

int bt_manager_update_linkkey(struct bt_linkkey_info *info, uint8_t cnt)
{
	return btif_br_update_linkkey(info, cnt);
}

int bt_manager_write_ori_linkkey(bd_address_t *addr, uint8_t *link_key)
{
	return btif_br_write_ori_linkkey(addr, link_key);
}

int bt_manager_get_connected_dev_rssi(bd_address_t *addr,btsrv_get_rssi_result_cb cb)
{
    struct bt_get_rssi_param param;
    memset(&param,0,sizeof(struct bt_get_rssi_param));
	memcpy(&param.addr,addr,sizeof(bd_address_t));
    param.cb = cb;
    return btif_br_get_connected_device_rssi(&param);
}

int bt_manager_get_actived_dev_rssi(int8_t *rssi)
{
    int ret;
    ret = btif_br_get_actived_device_rssi(rssi);
    SYS_LOG_INF("BMC RSSI %d",*rssi);
    return ret;
}

void bt_manager_clear_linkkey(bd_address_t *addr)
{
    btif_br_clean_linkkey(addr);
}

/* Ble encrypted, phone discover ble use 2s, device discover ancs/ams use 2s. */
#define BR_RESOLVE_CONNECT_DELAY_TIME			(4000)
static uint8_t br_resolve_pending;
static bd_address_t br_resolve_addr;

static void br_resolve_work_handler(struct k_work *work)
{
	struct bt_set_autoconn reconnect_param;

	SYS_LOG_INF("BMC do resolve br connect");

	memset(&reconnect_param, 0, sizeof(reconnect_param));
	memcpy(&reconnect_param.addr, &br_resolve_addr, sizeof(bd_address_t));
	reconnect_param.strategy = BTSRV_AUTOCONN_ALTERNATE;
	reconnect_param.base_try = BT_BASE_DEFAULT_RECONNECT_TRY;
	reconnect_param.profile_try = BT_PROFILE_RECONNECT_TRY;
	reconnect_param.base_interval = BT_BASE_RECONNECT_INTERVAL;
	reconnect_param.profile_interval = BT_PROFILE_RECONNECT_INTERVAL;
	reconnect_param.force_connect = 1;
	reconnect_param.connect_hfp = 1;
	//reconnect_param.connect_a2dp = 1;
	//reconnect_param.connect_avrcp = 1;
	//reconnect_param.connect_hfp_first = 1;

	btif_br_auto_reconnect(&reconnect_param);
	bt_wake_unlock();
	br_resolve_pending = 0;
}

OS_DELAY_WORK_DEFINE(br_resolve_work, br_resolve_work_handler);

void bt_manager_br_resolve_connect(void *param)
{
	struct bt_br_resolve_param *resolve_param = param;

	if (resolve_param->br_connected) {
		SYS_LOG_INF("BMC br resolve already connected!");
	} else {
		if (!br_resolve_pending) {
			br_resolve_pending = 1;
			memcpy(&br_resolve_addr, &resolve_param->addr, sizeof(bd_address_t));
			os_delayed_work_submit(&br_resolve_work, BR_RESOLVE_CONNECT_DELAY_TIME);
			bt_wake_lock();
		} else {
			SYS_LOG_INF("BMC already pending resolve connect");
		}
	}
}
