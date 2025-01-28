/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BLE_MASTER_MANAGER_H__
#define __BLE_MASTER_MANAGER_H__
#include <zephyr/types.h>
#include "ble_master_inner.h"

/* ble mater event */
enum {
	BLE_EVENT_NULL                         = 0,
	BLE_SCAN_FAIL_IND                      = 1,
	BLE_CREAT_CON_FAIL_IND                 = 2,
	BLE_CON_FAIL_IND                       = 3,
	BLE_CON_DISCONECTED_IND                = 4,
	BLE_CON_CONECTED_IND                   = 5,
	BLE_CHARACTERISTIC_DISCOVER_COMPLETE   = 6,
	BLE_PRIMARY_DISCOVER_COMPLETE          = 7,
	BLE_CCC_DISCOVER_COMPLETE              = 8,
	BLE_PROCESS_CLOSE_SCAN_IND             = 9,

	//BLE_MATSER_SCAN_START                  = 0x80,
	//BLE_MATSER_SCAN_STOP                   = 0x81,
};

void le_master_env_init(void);

void le_master_event_handle(uint8_t event_code, void *event_data);

#ifdef	CONFIG_OTA_BLE_MASTER_SUPPORT
int ble_master_manager_cb_reg(struct btmgr_spp_cb *spp_cb);
int ble_master_manager_send_data(uint8_t chl, uint8_t *data, uint32_t len);
#endif

#endif
