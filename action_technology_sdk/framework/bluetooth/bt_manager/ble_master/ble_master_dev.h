/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BLE_MASTER_DEV_H__
#define __BLE_MASTER_DEV_H__

#include <zephyr/types.h>
#include <bt_manager.h>
#include "ble_master_inner.h"

struct conn_interval_t {
	struct bt_conn *conn;
	u8_t index; /*le_slave_conn_param index*/
	u8_t state; /* 0: IDLE  1: CONNECTING 2: CONNECTED*/
};

extern struct conn_interval_t conn_interval[8];

struct le_master_info
{
	uint8_t scan_state;
	uint8_t env_init;
};

struct conn_interval_t* conn_interval_get(void);

void conn_interval_put(struct bt_conn *conn);

void le_master_parse_service(struct ble_connection *ble_conn, const struct bt_gatt_service_val *gatt_service,const struct bt_gatt_attr *attr);

void le_master_parse_charicteristic(struct ble_connection *ble_conn, const struct bt_gatt_chrc *gatt_chrc,const struct bt_gatt_attr *attr);

#endif