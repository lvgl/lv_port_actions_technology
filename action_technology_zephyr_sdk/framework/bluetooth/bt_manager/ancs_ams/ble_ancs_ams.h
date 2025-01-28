/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _APP_BLE_H_
#define _APP_BLE_H_
#include <zephyr/types.h>
#include <os_common_api.h>
/* ble event */

#define BLE_ANCS_ATTR_REQ_MAX    8
#define BLE_ANCS_APP_NO_UUID	-1
enum {
	BLE_ANCS_AMS_EVENT_NULL,
	BLE_ANCS_AMS_CONN_OPEN_IND,
	BLE_ANCS_AMS_CONN_SEC_OPEN_IND,
	
	BLE_ANCS_AMS_DISC_PRIM_SERV_IND,
	BLE_ANCS_AMS_DISC_SERVICE_CHAR_IND,
	BLE_ANCS_AMS_DISC_SERVICE_CHAR_DESC_IND,
	BLE_ANCS_AMS_DISCOVERY_COMPELTE_IND,
	
    BLE_ANCS_EVENT_NOTIFICATION_ATTR_REQ,
    BLE_ANCS_EVENT_NOTIFICATION_ATTR_REQ_COMPLETE,
    BLE_ANCS_EVENT_APP_ATTR_REQ,
    BLE_ANCS_EVENT_NOTIFICATION_MSG_FINISH,

    BLE_AMS_EVENT_WRITE_ENTITY_UPDATE,
    BLE_AMS_EVENT_READ_ENTITY_UPDATE,

    BLE_GAP_EVENT_SERVICE_CHANGED,
};

enum {
	BLE_SERVICE_NULL,
	BLE_SERVICE_ANCS_AMS,
	BLE_SERVICE_GAP,
};

enum 
{
	/* Initial state */
	app_init = 0,

	/* Fast undirected advertisements configured */
	app_fast_advertising,

	/* Slow undirected advertisements configured */
	app_slow_advertising,

	/* Enters when application is in connected state */
	app_connected,

	app_paired,

	app_discovered,

	app_configured,

	app_disconnected,

	/* Enters when disconnect is initiated by the application */
	app_disconnecting,

	/* Idle state */
	app_idle,

	/* Unknown state */
	app_state_unknown
} ;

struct ancs_attr_request_data_t
{
    uint32_t uuid;
    uint8_t valid;
};

struct ancs_ams_data_t {
	struct bt_conn *pconn;
	bool remote_gatt_handles_present;
	bool ancs_configured;
	bool ams_configured;
	bool gap_configured;
	bool service_discovered;
	bool ios_phone;
	uint8_t state:4;
	uint8_t connect_index:4;
	uint8_t getting_pos:1;
	int8_t ancs_attr_request_rd_index;
	uint8_t ancs_attr_request_wr_index;
	os_delayed_work ble_ancs_ams_discover_work;
	struct ancs_attr_request_data_t ancs_attr_request_list[BLE_ANCS_ATTR_REQ_MAX];
};

/**
 * @brief Callback for notifying that Bluetooth has been enabled.
 *
 *  A function will be called by the bt_enable() function
 * @param err zero on success or (negative) error code otherwise.
 */

struct ancs_ams_data_t *ble_get_ancs_ams(void);

void ble_ancs_ams_send_msg_to_app(uint8_t event_code, int status);

int ble_ancs_read_notification_request_from_list(uint32_t *notify_uuid);

void ble_ancs_write_notification_request_to_list(uint32_t notify_uuid);

int ble_ancs_ams_notify_playback_pos(float elapsed_time);

#endif
