/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include "errno.h"
#include <zephyr.h>
#include <net/buf.h>
#include <stdlib.h>
#include <msg_manager.h>
#include <acts_bluetooth/bluetooth.h>
#include <acts_bluetooth/hci.h>
#include <acts_bluetooth/conn.h>
#include <acts_bluetooth/uuid.h>
#include <acts_bluetooth/gatt.h>
#include <sys/byteorder.h>

#include <bt_manager.h>
#include "ble_master_manager.h"

#if 1
/* Connect ble slave by UUID */
#define BLE_DEV_MATCH_UUID  BT_UUID_CTS
#else
/* Connect ble slave by name */
#define BT_NAME_STR_LEN 31
static char ble_name[BT_NAME_STR_LEN] = {0};
#define BLE_DEV_MATCH_NAME  "ANCS"
#endif

#define BLE_DEV_RSSI_SIGNAL_FITER -50		/* Scan signal threshold */

#define MATER_DISCOVER_CCC 1
#if MATER_DISCOVER_CCC
#ifdef	CONFIG_OTA_BLE_MASTER_SUPPORT
/* BLE */
#define MATER_SUPER_SERVICE_UUID BT_UUID_DECLARE_128( \
				0xd1, 0x9f, 0x1b, 0x1f, 0x80, 0xf2, 0xb2, 0x8e, \
				0xe8, 0x11, 0x9a, 0xf6, 0xf8, 0x25, 0x9a, 0xe4)


/* "e49a25e0-f69a-11e8-8eb2-f2801f1b9fd1" reverse order */
#define MATER_SUPER_TEST_UUID BT_UUID_DECLARE_128( \
				0xd1, 0x9f, 0x1b, 0x1f, 0x80, 0xf2, 0xb2, 0x8e, \
				0xe8, 0x11, 0x9a, 0xf6, 0xe0, 0x25, 0x9a, 0xe4)

/* "e49a28e1-f69a-11e8-8eb2-f2801f1b9fd1" reverse order */
#define MATER_SUPER_NOTIFY_UUID BT_UUID_DECLARE_128( \
				0xd1, 0x9f, 0x1b, 0x1f, 0x80, 0xf2, 0xb2, 0x8e, \
				0xe8, 0x11, 0x9a, 0xf6, 0xe1, 0x28, 0x9a, 0xe4)

#define BLE_MATSER_CHANNEL_VIRTUAL (0x80)
struct btmgr_spp_cb *le_master_cb;
static struct bt_uuid_128 uuid_ccc = BT_UUID_INIT_128(0);
static struct bt_uuid_16 uuid_ccc_notify = BT_UUID_INIT_16(0);

#else
#define MATER_SUPER_SERVICE_UUID   BT_UUID_DECLARE_16(0xFFD0)
#define MATER_SUPER_TEST_UUID      BT_UUID_DECLARE_16(0xFFD3)
static struct bt_uuid_16 uuid_ccc = BT_UUID_INIT_16(0);
#endif
static struct bt_gatt_subscribe_params subscribe_params;
#endif

static struct bt_gatt_discover_params discover_params;
struct le_scan_param le_scan_cb;
struct le_master_info master_info;

static void le_master_send_msg_to_app(uint8_t event_code, int status)
{
	struct app_msg msg = {0};

	msg.cmd = event_code;
	msg.value = status;
#ifdef	CONFIG_OTA_BLE_MASTER_SUPPORT
	msg.type = MSG_BLE_OTA_MASTER_EVENT;
	send_async_msg("ota_product", &msg);
#else
	msg.type = MSG_BLE_MASTER_EVENT;
	send_async_msg(BLE_MASTER, &msg);
#endif
	SYS_LOG_INF("le master send event_code %d", event_code);
}

static u8_t discover_func(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				struct bt_gatt_discover_params *params)
{
	int err, i;
	struct bt_gatt_chrc *g_chrc;
	struct bt_gatt_service_val *gatt_service;
	struct ble_connection *ble_conn;

	ble_conn = ble_conn_find(conn);
	if (ble_conn == NULL) {
		SYS_LOG_ERR("Can't find ble_conn!");
		return BT_GATT_ITER_STOP;
	}

	if (!attr) {
		switch (params->type) {
		case BT_GATT_DISCOVER_PRIMARY:
			for (i = 0; i < BLE_MAX_SERVICE; i++) {
				if (ble_conn->service[i].start_handle != 0) {
					break;
				}
			}

			if (i == BLE_MAX_SERVICE) {
				SYS_LOG_INF("service found maxlimit!");
				memset(params, 0, sizeof(*params));
				return BT_GATT_ITER_STOP;
			}

			ble_conn->sd_index = i;
			params->type = BT_GATT_DISCOVER_CHARACTERISTIC;

            le_master_send_msg_to_app(BLE_PRIMARY_DISCOVER_COMPLETE, (int)conn);

			break;
		case BT_GATT_DISCOVER_CHARACTERISTIC:
			SYS_LOG_INF("discover finish!");
			memset(params, 0, sizeof(*params));

            le_master_send_msg_to_app(BLE_CHARACTERISTIC_DISCOVER_COMPLETE,  (int)conn);

			return BT_GATT_ITER_STOP;
		}

		params->func = discover_func;
		params->start_handle = 0x0001;
		params->end_handle = 0xffff;
		err = bt_gatt_discover(conn, params);
		if (err)  {
			SYS_LOG_ERR("Discover failed (err %d)", err);
		} else {
			SYS_LOG_INF("Discover pending");
		}
		return BT_GATT_ITER_STOP;
	}


	switch (params->type) {
	case BT_GATT_DISCOVER_PRIMARY:
         gatt_service = attr->user_data;
		 SYS_LOG_INF("[%p]Service anaysis",ble_conn);
		 le_master_parse_service(ble_conn, gatt_service, attr);
		break;
	case BT_GATT_DISCOVER_CHARACTERISTIC:
        g_chrc = attr->user_data;
		SYS_LOG_INF("[%p]Characteristic anaysis",ble_conn);
		le_master_parse_charicteristic(ble_conn, g_chrc, attr);
		break;
	default:
		break;
	}

	return BT_GATT_ITER_CONTINUE;
}

void start_discover(struct bt_conn *conn)
{
    SYS_LOG_INF("start_discover:BT_GATT_DISCOVER_PRIMARY_ALL");
    int err;
	memset(&discover_params,0,sizeof(discover_params));
    discover_params.uuid = NULL;
	discover_params.func = discover_func;
	discover_params.start_handle = 0x0001;
	discover_params.end_handle = 0xffff;
	discover_params.type = BT_GATT_DISCOVER_PRIMARY;

	err = bt_gatt_discover(conn, &discover_params);
	if (err) {
		SYS_LOG_ERR("Discover failed(err %d)", err);
		return;
	}
}

#if MATER_DISCOVER_CCC
static uint8_t notify_func(struct bt_conn *conn,
			   struct bt_gatt_subscribe_params *params,
			   const void *data, uint16_t length)
{
	if (!data) {
		SYS_LOG_INF("[UNSUBSCRIBED]");
		params->value_handle = 0U;
		return BT_GATT_ITER_STOP;
	}

	SYS_LOG_INF("[NOTIFICATION] data %p length %u", data, length);
#ifdef	CONFIG_OTA_BLE_MASTER_SUPPORT
	if (le_master_cb->receive_data)
		le_master_cb->receive_data(BLE_MATSER_CHANNEL_VIRTUAL, (void *)data, length);
#endif
	return BT_GATT_ITER_CONTINUE;
}

static u8_t discover_func_ccc(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				struct bt_gatt_discover_params *params)
{
	int err = -1;

	if (!attr) {
		SYS_LOG_INF("Discover complete");
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	SYS_LOG_INF("[ATTRIBUTE] handle 0x%x", attr->handle);

	if (!bt_uuid_cmp(discover_params.uuid, MATER_SUPER_SERVICE_UUID)) {
		memcpy(&uuid_ccc, MATER_SUPER_TEST_UUID, sizeof(uuid_ccc));
		discover_params.uuid = &uuid_ccc.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		struct bt_gatt_service_val *ccc_gatt_service;
		ccc_gatt_service = attr->user_data;
		discover_params.end_handle = ccc_gatt_service->end_handle;

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			SYS_LOG_ERR("Discover failed (err %d)", err);
		}
#ifdef	CONFIG_OTA_BLE_MASTER_SUPPORT
	} else if (!bt_uuid_cmp(discover_params.uuid, MATER_SUPER_TEST_UUID)) {
		memcpy(&uuid_ccc, MATER_SUPER_NOTIFY_UUID, sizeof(uuid_ccc));		/* CCC characteristic */
		discover_params.uuid = &uuid_ccc.uuid;
		discover_params.start_handle = attr->handle + 2;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		conn_master_add_test_write_hdl(conn, (attr->handle + 1));

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			SYS_LOG_ERR("Discover failed (err %d)", err);
		}
	} else if (!bt_uuid_cmp(discover_params.uuid, MATER_SUPER_NOTIFY_UUID)) {
		memcpy(&uuid_ccc_notify, BT_UUID_GATT_CCC, sizeof(uuid_ccc_notify));		/* CCC characteristic */
		discover_params.uuid = &uuid_ccc_notify.uuid;
		discover_params.start_handle = attr->handle + 2;
		discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
		subscribe_params.value_handle = bt_gatt_attr_value_handle(attr);
		SYS_LOG_INF("value_handle %d",subscribe_params.value_handle);

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			SYS_LOG_ERR("Discover failed (err %d)", err);
		}
#else
	} else if (!bt_uuid_cmp(discover_params.uuid, MATER_SUPER_TEST_UUID)) {
		memcpy(&uuid_ccc, BT_UUID_GATT_CCC, sizeof(uuid_ccc));		/* CCC characteristic */
		discover_params.uuid = &uuid_ccc.uuid;
		discover_params.start_handle = attr->handle + 2;
		discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
		subscribe_params.value_handle = bt_gatt_attr_value_handle(attr);

		conn_master_add_test_write_hdl(conn, (attr->handle + 1));

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			SYS_LOG_ERR("Discover failed (err %d)", err);
		}
#endif
	} else if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_GATT_CCC)) {
		subscribe_params.notify = notify_func;
		subscribe_params.value = BT_GATT_CCC_NOTIFY;
		subscribe_params.ccc_handle = attr->handle;

		err = bt_gatt_subscribe(conn, &subscribe_params);
		if (err && err != -EALREADY) {
			SYS_LOG_ERR("Subscribe failed (err %d)", err);
		} else {
			SYS_LOG_INF("[SUBSCRIBED]");
		}
		le_master_send_msg_to_app(BLE_CCC_DISCOVER_COMPLETE,  (int)conn);
#ifdef	CONFIG_OTA_BLE_MASTER_SUPPORT
		if (le_master_cb->connected)
			le_master_cb->connected(BLE_MATSER_CHANNEL_VIRTUAL, NULL);
#endif
		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_STOP;
}

void start_discover_ccc(struct bt_conn *conn)
{
	int err;

	SYS_LOG_INF("start_discover:BT_GATT_DISCOVER_CCC");
	memset(&discover_params,0,sizeof(discover_params));
	memcpy(&uuid_ccc, MATER_SUPER_SERVICE_UUID, sizeof(uuid_ccc));
	discover_params.uuid = &uuid_ccc.uuid;
	discover_params.func = discover_func_ccc;
	discover_params.start_handle = 0x0001;
	discover_params.end_handle = 0xffff;
	discover_params.type = BT_GATT_DISCOVER_PRIMARY;

	err = bt_gatt_discover(conn, &discover_params);
	if (err) {
		SYS_LOG_ERR("Discover failed(err %d)", err);
		return;
	}
}
#endif
static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err,i;
	struct conn_interval_t* p_conn_interval = NULL;
	struct bt_conn_info info;

	if ((bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_LE) || (info.role != BT_HCI_ROLE_MASTER)) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	for (i = 0; i < ARRAY_SIZE(conn_interval); i++) {
		if (conn_interval[i].state == 1) {
			if(conn==conn_interval[i].conn){
			    p_conn_interval = &conn_interval[i];
				break;
			}
		}
	}

	if(p_conn_interval == NULL){
       SYS_LOG_INF("CONN master connected not matching!");
	   return;
	}

	if (conn_err) {
		SYS_LOG_ERR("Failed to connect to %s (%u)", addr, conn_err);
		le_master_send_msg_to_app(BLE_CON_FAIL_IND, conn_err);
		p_conn_interval->state = 0;
        conn_release(conn);
		bt_conn_unref(conn);
		conn_interval_put(conn);
		return;
	}

	p_conn_interval->state = 2;

    SYS_LOG_INF("Ble master connected: %s, conn_p:%p", addr,conn);

	err = conn_alloc(conn, BLE_CONN_CONNECTED);
	if (err) {
		conn_release(conn);
		bt_conn_unref(conn);
        conn_interval_put(conn);
		SYS_LOG_ERR("conn_create failed!");
		return;
	}

    le_master_send_msg_to_app(BLE_CON_CONECTED_IND,  (int)conn);

	SYS_LOG_INF("Ble master connected done!");
}

void disconnected(struct bt_conn *conn, u8_t reason)
{
	struct conn_interval_t* p_conn_interval = NULL;
	char addr[BT_ADDR_LE_STR_LEN];
    int i;
	struct bt_conn_info info;

	if ((bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_LE) || (info.role != BT_HCI_ROLE_MASTER)) {
		return;
	}

	for (i = 0; i < ARRAY_SIZE(conn_interval); i++) {
		if (conn_interval[i].state == 2) {
			if(conn==conn_interval[i].conn){
			    p_conn_interval = &conn_interval[i];
				break;
			}
		}
	}
	if(p_conn_interval == NULL){
       SYS_LOG_INF("CONN master disconnected not matching!");
	   return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	SYS_LOG_INF("[warnning]Disconnected: %s (reason %x)", addr, reason);
	conn_release(conn);
	bt_conn_unref(conn);
	conn_interval_put(conn);
#ifdef	CONFIG_OTA_BLE_MASTER_SUPPORT
	if (le_master_cb->disconnected)
		le_master_cb->disconnected(BLE_MATSER_CHANNEL_VIRTUAL);
#endif
	le_master_send_msg_to_app(BLE_CON_DISCONECTED_IND, 0);
}

static bool eir_found(struct bt_data *data, void *user_data)
{
	int i;
	int err;
	bool result_found = 0;

	bt_addr_le_t *addr = user_data;

	SYS_LOG_INF("[AD]: %u data_len %u addr->type %u", data->type, data->data_len,addr->type);

	switch (data->type) {
#ifdef BLE_DEV_MATCH_NAME
		case BT_DATA_NAME_SHORTENED:
		case BT_DATA_NAME_COMPLETE:
			memset(ble_name,0,sizeof(ble_name));
			if (data->data_len > sizeof(ble_name) - 1) {
				memcpy(ble_name, data->data, sizeof(ble_name) - 1);
			} else {
				memcpy(ble_name, data->data, data->data_len);
			}
			ble_name[data->data_len] = '\0';
			SYS_LOG_INF("[AD]ble_name: %s!",  ble_name);
			if (!strcmp(ble_name, BLE_DEV_MATCH_NAME)) {
				result_found = 1;
				SYS_LOG_INF("Name Match Controllor found: %s", ble_name);
			}
			break;
#endif

#ifdef BLE_DEV_MATCH_UUID
       case BT_DATA_UUID16_SOME:
	   case BT_DATA_UUID16_ALL:
		 if (data->data_len % sizeof(uint16_t) != 0U) {
			SYS_LOG_INF("AD malformed");
			return true;
		 }
		 for (i = 0; i < data->data_len; i += sizeof(uint16_t)) {
			struct bt_uuid *uuid;
			uint16_t u16;
			memcpy(&u16, &data->data[i], sizeof(u16));
			uuid = BT_UUID_DECLARE_16(sys_le16_to_cpu(u16));
			SYS_LOG_INF("i:%d, sys_le16_to_cpu(u16):%x", i,sys_le16_to_cpu(u16));
			if (bt_uuid_cmp(uuid, BLE_DEV_MATCH_UUID)) {
				continue;
			}
	        result_found = 1;
	        SYS_LOG_INF("UUID Match Controllor found: %x", sys_le16_to_cpu(u16));
		}
        break;
#endif
	}

    if(result_found == 1)
	{
		le_master_scan_stop();
		err = le_master_dev_connect(addr);
		if (err) {
			le_master_send_msg_to_app(BLE_CREAT_CON_FAIL_IND, err);
			return true;
		}
        SYS_LOG_INF("eir_found ok");
		return false;
	}else
	{
		return true;
	}
}

void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char dev[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, dev, sizeof(dev));

	if(rssi < BLE_DEV_RSSI_SIGNAL_FITER) {
	    return;
	}

	if (BLE_CONN_CONNECTED == conn_master_get_state_by_addr(addr)) {
		SYS_LOG_INF("Ignore connected device (%s)", dev);
		return;
	}

    SYS_LOG_INF("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %i",dev, type, ad->len, rssi);

	/* We're only interested in connectable events */
	if (type == BT_HCI_ADV_IND || type == BT_HCI_ADV_DIRECT_IND || type == BT_HCI_ADV_SCAN_RSP) {
		bt_data_parse(ad, eir_found, (void *)addr);
	}
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	struct bt_conn_info info;
	if ((bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_LE) || (info.role != BT_HCI_ROLE_MASTER)) {
		return false;
	}
	SYS_LOG_INF("master le_param_req (0x%04x, 0x%04x) lat %d to %d", param->interval_min,
		param->interval_max, param->latency, param->timeout);

	return true;
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval,
			     uint16_t latency, uint16_t timeout,uint8_t status)
{
	struct bt_conn_info info;
	if ((bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_LE) || (info.role != BT_HCI_ROLE_MASTER)) {
		return;
	}
	SYS_LOG_INF("master le_param_updated status %d inv %d lat %d to %d", status, interval, latency, timeout);
	return;
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_req = le_param_req,
	.le_param_updated = le_param_updated,
};

void le_master_env_init(void)
{

	SYS_LOG_INF("Bluetooth master initialized");

	memset(&master_info, 0, sizeof(master_info));

    conn_manager_init();

    bt_conn_cb_register(&conn_callbacks);

    master_info.env_init = 1;
}

void le_master_event_handle(uint8_t event_code, void *event_data)
{
	int err=-1;
    char addr[BT_ADDR_LE_STR_LEN];

	SYS_LOG_INF("[IND]ble_master_event_handle %d", event_code);
    switch (event_code) {
	   case BLE_CREAT_CON_FAIL_IND:
	   {
			err = le_master_scan_start(&le_scan_cb);
			if (err) {
				SYS_LOG_ERR("[IND]Scanning failed to start (err %d)", err);
			 return;
			}
		}
		break;
		case BLE_CON_DISCONECTED_IND:
		{
           SYS_LOG_INF("[IND]Dev disconnected");
		}
		break;
		case BLE_CON_FAIL_IND:
		{
			SYS_LOG_INF("[IND]Dev fail connect");
		}
		break;
        case BLE_CON_CONECTED_IND:
        {
            SYS_LOG_INF("[IND]Dev success connect");
            struct bt_conn *conn = (struct bt_conn *)event_data;
            os_sleep(100);
#if MATER_DISCOVER_CCC
            start_discover_ccc(conn);
#else
			start_discover(conn);
#endif
        }
        break;
        case BLE_CHARACTERISTIC_DISCOVER_COMPLETE:
        {
            struct bt_conn *conn = (struct bt_conn *)event_data;
	        bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
            SYS_LOG_INF("[IND]Dev %s characteristic discover complete",addr);
			#if 0
            if(conn_have_value_conn()){
                 err = le_master_scan_start(&le_scan_cb);
			     if (err) {
				   SYS_LOG_ERR("[IND]Scanning failed to start (err %d)", err);
                }
            }
			#endif
        }
        break;
		case BLE_CCC_DISCOVER_COMPLETE:
		{
            struct bt_conn *conn = (struct bt_conn *)event_data;
	        bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
            SYS_LOG_INF("[IND]Dev %s ccc discover complete",addr);
			#if 0
            if(conn_have_value_conn()){
                 err = le_master_scan_start(&le_scan_cb);
			     if (err)
				   SYS_LOG_ERR("[IND]Scanning failed to start (err %d)", err);
                }
            }
			#endif
        }
        break;
        case BLE_PRIMARY_DISCOVER_COMPLETE:
        {
            struct bt_conn *conn = (struct bt_conn *)event_data;
	        bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
            SYS_LOG_INF("[IND]Dev %s primary discover complete",addr);
        }
        break;
		case BLE_PROCESS_CLOSE_SCAN_IND:
		{
            le_master_scan_stop();
            SYS_LOG_INF("[IND]process le_master_scan_stop");
        }
        break;
	default:
		SYS_LOG_INF("unknow ble event %d", event_code);
		break;
	}
   return;
}

uint8_t le_master_get_scan_status(void)
{
	SYS_LOG_INF("get le master status:%d", master_info.scan_state);
	return master_info.scan_state;
}

#ifdef	CONFIG_OTA_BLE_MASTER_SUPPORT
/* uuid pointer must keep valued is used */
int ble_master_manager_cb_reg(struct btmgr_spp_cb *spp_cb)
{
	SYS_LOG_INF("spp_cb:0x%x", spp_cb);
	le_master_cb = spp_cb;
	return 0;
}
#if 0
static void write_func_cb(struct bt_conn *conn, uint8_t err,
		       struct bt_gatt_write_params *params)
{
	struct ble_connection *ble_dev = ble_conn_find(conn);
	uint16_t hdl = params ? params->handle : 0;

	SYS_LOG_INF("Gatt write cb hdl 0x%x err %d", hdl, err);
	if (ble_dev) {
		ble_dev->test_write_wait_finish = 0;
	}
}
#endif
int ble_master_manager_send_data(uint8_t chl, uint8_t *data, uint32_t len)
{
	int err, ret = -EIO;
	uint8_t i;
	struct ble_connection *ble_dev;

	for (i=0; i<CONN_NUM; i++) {
		ble_dev = ble_conn_get_by_index(i);
		if (ble_dev == NULL || ble_dev->conn == NULL ||
			ble_dev->test_write_params.handle == 0) {
			continue;
		}

		ret = 0;		/* For test continue write */
		//if (ble_dev->test_write_wait_finish) {
		//	continue;
		//}

		//ble_dev->test_write_params.func = write_func_cb;
		//ble_dev->test_write_params.offset = 0;
		//ble_dev->test_write_params.data = data;
		//ble_dev->test_write_params.length = len;
		//err = bt_gatt_write(ble_dev->conn, &ble_dev->test_write_params);
		err = bt_gatt_write_without_response(ble_dev->conn, 
			ble_dev->test_write_params.handle, data, len, false);
		if (err) {
			//ble_dev->test_write_params.func = NULL;
			SYS_LOG_ERR("Gatt write failed (err %d)", err);
		} else {
			//ble_dev->test_write_wait_finish = 1;
			SYS_LOG_INF("Gatt write data");
		}
	}

	return ret;
}
#endif
