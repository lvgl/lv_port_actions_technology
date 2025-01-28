/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt ble master device manager.
 */
#include "ble_master_dev.h"

extern struct le_master_info master_info;

const struct bt_le_conn_param le_slave_conn_param[8] = {
#ifdef	CONFIG_OTA_BLE_MASTER_SUPPORT
    {6,9,0,600},
    {10,13,0,600},
    {14,17,0,600},
    {18,21,0,600},
	{22,25,0,600},
	{26,29,0,600},
	{30,33,0,600},
	{34,37,0,600},
#else
	{160,240,0,600},
	{160,240,0,600},
	{160,240,0,600},
	{160,240,0,600},
	{160,240,0,600},
	{160,240,0,600},
	{160,240,0,600},
	{160,240,0,600},
#endif
};

struct conn_interval_t conn_interval[8] = {
 {NULL, 0, 0}, {NULL, 1, 0}, {NULL, 2, 0}, {NULL, 3, 0}, {NULL, 4, 0}, {NULL, 5, 0}, {NULL, 6, 0}, {NULL, 7, 0}
};

struct bt_le_scan_param scan_param = {
		.type       = BT_LE_SCAN_TYPE_ACTIVE,
		.options    = BT_LE_SCAN_OPT_NONE,
		.interval   = 0x40, //BT_GAP_SCAN_FAST_INTERVAL,
		.window     = 0x20, //BT_GAP_SCAN_FAST_WINDOW,
};

struct conn_interval_t* conn_interval_get(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(conn_interval); i++) {
		if (!conn_interval[i].conn && !conn_interval[i].state) {
			conn_interval[i].state = 1;
			printk("conn_interval_get %d!\n", conn_interval[i].index);
			return &conn_interval[i];
		}
	}

	SYS_LOG_ERR("conn_interval_get failed");
	return NULL;
}

void conn_interval_put(struct bt_conn *conn)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(conn_interval); i++) {
		if (conn_interval[i].conn == conn) {
			SYS_LOG_INF("conn_interval_put %d %p!", conn_interval[i].index, conn_interval[i].conn);
			conn_interval[i].state = 0;
			conn_interval[i].conn = NULL;
		}
	}
}

u16_t uuid_len(const struct bt_uuid *uuid){
	switch (uuid->type) {
	case BT_UUID_TYPE_16:
		return sizeof(struct bt_uuid_16);
	case BT_UUID_TYPE_32:
		return sizeof(struct bt_uuid_32);
	case BT_UUID_TYPE_128:
		return sizeof(struct bt_uuid_128);
	}
	return sizeof(struct bt_uuid_16);
}

void le_master_parse_service(struct ble_connection *ble_conn, const struct bt_gatt_service_val *gatt_service,
	const struct bt_gatt_attr *attr)
{
	char uuid[37] = {0};
	int i;

	bt_uuid_to_str(gatt_service->uuid, uuid, sizeof(uuid));

	for (i = 0; i < BLE_MAX_SERVICE; i++) {
		if (ble_conn->service[i].start_handle == 0) {
			ble_conn->service[i].start_handle = attr->handle;
			ble_conn->service[i].end_handle = gatt_service->end_handle;
			ble_conn->service[i].u.uuid.type = gatt_service->uuid->type;
			memcpy(&ble_conn->service[i].u.uuid, gatt_service->uuid, uuid_len(gatt_service->uuid));
			SYS_LOG_INF("service[%d] %s service start handle: %d, end_handle: %d", i, uuid, ble_conn->service[i].start_handle,
				ble_conn->service[i].end_handle);
			break;
		}
	}
}

void le_master_parse_charicteristic(struct ble_connection *ble_conn, const struct bt_gatt_chrc *gatt_chrc,
	const struct bt_gatt_attr *attr)
{
	char uuid[37] = {0};
	int i, j;
	bt_uuid_to_str(gatt_chrc->uuid, uuid, sizeof(uuid));

	for (i = 0; i < BLE_MAX_SERVICE; i++) {
		if (attr->handle >= ble_conn->service[i].start_handle &&
			attr->handle <= ble_conn->service[i].end_handle) {
			break;
		}
	}

	if (i == BLE_MAX_SERVICE) {
		SYS_LOG_INF("No service found for attr handle %d", attr->handle);
		return;
	}

	for (j = 0; j < BLE_MAX_CHRC; j++) {
		if (ble_conn->service[i].chrc[j].chrc_handle == 0) {
			break;
		}
	}

	if (j == BLE_MAX_CHRC) {
		SYS_LOG_INF("No more chrc!");
		return;
	}

	ble_conn->service[i].chrc[j].chrc_handle = attr->handle;
	ble_conn->service[i].chrc[j].value_handle = attr->handle + 1;
	ble_conn->service[i].chrc[j].properties = gatt_chrc->properties;

	if (gatt_chrc->properties & BT_GATT_CHRC_NOTIFY) {
		ble_conn->service[i].chrc[j].ccc_handle = attr->handle + 2;
	}

	ble_conn->service[i].chrc[j].ch_uuid.uuid.type = gatt_chrc->uuid->type;
	memcpy(&ble_conn->service[i].chrc[j].ch_uuid, gatt_chrc->uuid, uuid_len(gatt_chrc->uuid));

	SYS_LOG_INF("service[%d] chrc[%d] chrc_handle: %d, value_handle: %d, ccc_handle: %d, properties: 0x%x, ccc_type:%d",
		i, j,
		ble_conn->service[i].chrc[j].chrc_handle,
		ble_conn->service[i].chrc[j].value_handle,
		ble_conn->service[i].chrc[j].ccc_handle,
		ble_conn->service[i].chrc[j].properties,
		ble_conn->service[i].chrc[j].ccc_type);
}

int le_master_dev_connect(bt_addr_le_t *addr)
{
   int err=-1;
   char dev[BT_ADDR_LE_STR_LEN];
   struct bt_conn *p_conn=NULL;

   if (master_info.env_init != 1)
   {
	   SYS_LOG_INF("attention master init not yet, quit");
	   return err;
   }

   bt_addr_le_to_str(addr, dev, sizeof(dev));

   p_conn = find_ble_conn_use_addr(addr);
   if (p_conn!=NULL)
   {
	   SYS_LOG_INF("attention (%s) already connection exist, quit",dev);
	   return err;
   }

   struct conn_interval_t* p_conn_interval = conn_interval_get();
   if (!p_conn_interval) {
	     SYS_LOG_ERR("conn (%s) interval paramters get fail",dev);
		 return -1;
   }
   err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,&le_slave_conn_param[p_conn_interval->index],&p_conn_interval->conn);
   if (err) {
		SYS_LOG_ERR("create (%s) connection failed (err %d)",dev,err);
		p_conn_interval->conn = NULL;
		return true;
	} else {
		p_conn_interval->state = 1; /*connecting*/
		SYS_LOG_INF("create (%s) connection", dev);
	}
    return err;
}

int le_master_dev_disconnect(bt_addr_le_t *addr)
{
   int err=-1;
   char dev[BT_ADDR_LE_STR_LEN];
   struct bt_conn *p_conn=NULL;

   bt_addr_le_to_str(addr, dev, sizeof(dev));

   p_conn = find_ble_conn_use_addr(addr);
   if(p_conn==NULL)
   {
	   SYS_LOG_INF("attention (%s) already disconnection, quit",dev);
	   return err;
   }
   err=bt_conn_disconnect(p_conn,BT_HCI_ERR_REMOTE_USER_TERM_CONN);
   if (err) {
		SYS_LOG_ERR("disconnection (%s) failed (err %d)",dev,err);
		return true;
   }else{
		SYS_LOG_INF("disconnection (%s)",dev);
   }
   return err;

}

int le_master_scan_start(struct le_scan_param*le_scan_cb)
{
	bool app_enable_adv;

	app_enable_adv = bt_manager_is_app_enable_adv();
	if (app_enable_adv)
	{
		SYS_LOG_ERR("App already enable ble adv");
		return -EBUSY;
	}

	master_info.scan_state=1;
	SYS_LOG_INF("le master scan start");
	return bt_le_scan_start(&scan_param, le_scan_cb->cb);
}

int le_master_scan_stop(void)
{
    int err=0;
    err = bt_le_scan_stop();
	if (err) {
		 SYS_LOG_ERR("Stop LE scan failed (err %d)", err);
	}else
	{
		os_sleep(100);
	}
	master_info.scan_state=0;
	SYS_LOG_INF("le master scan stop");
	return err;
}
