/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager multiple.
 */
#define SYS_LOG_NO_NEWLINE
#define SYS_LOG_DOMAIN "bt manager"

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
#include <btservice_api.h>
#include <shell/shell.h>
#include <acts_bluetooth/host_interface.h>
#include <property_manager.h>
#include "hci_core.h"

#define BT_MULTI_CONNECT_MAX	(1)//(8)

struct bt_le_ext_adv *s_adv[CONFIG_BT_ID_MAX];
struct bt_gatt_service s_svc[CONFIG_BT_ID_MAX];
struct bt_conn *multi_conn[BT_MULTI_CONNECT_MAX];
const char *multi_adv_data = "Actions-ADV.";
bool multi_init = false;
static OS_MUTEX_DEFINE(multi_mutex);

struct multi_mgr_info {
	struct bt_conn *ble_conn;
	uint8_t device_mac[6];
	uint16_t mtu;
	uint8_t ble_state;
	int8_t rssi;
	struct bt_le_conn_param le_conn_param;
};

static struct multi_mgr_info conn_info[BT_MULTI_CONNECT_MAX];

static int multi_add_st(void *conn)
{
	int i;

	if (!conn)
		return -ESRCH;

	os_mutex_lock(&multi_mutex, OS_FOREVER);
	for (i = 0; i < BT_MULTI_CONNECT_MAX; i++) {
		if (conn_info[i].ble_conn == NULL) {
			conn_info[i].ble_conn = conn;
			SYS_LOG_INF("a conn (%p) i %d\n", conn, i);
			break;
		}
	}
	os_mutex_unlock(&multi_mutex);

	if (i == BT_MULTI_CONNECT_MAX) {
		SYS_LOG_ERR("Failed to add conn %p", conn);
		return -EIO;
	}

	return 0;
}

static int multi_remove_st(void *conn)
{
	int i;

	if (!conn)
		return -ESRCH;

	os_mutex_lock(&multi_mutex, OS_FOREVER);
	for (i = 0; i < BT_MULTI_CONNECT_MAX; i++) {
		if (conn_info[i].ble_conn == conn) {
			SYS_LOG_INF("r conn (%p) i %d\n", conn, i);
			memset(&conn_info[i], 0 ,sizeof(struct multi_mgr_info));
			break;
		}
	}
	os_mutex_unlock(&multi_mutex);

	if (i == BT_MULTI_CONNECT_MAX) {
		SYS_LOG_ERR("Failed to remove conn %p", conn);
		return -EIO;
	}

	return 0;
}

static struct multi_mgr_info *multi_st_get(void *conn)
{
	int i;

	if (!conn)
		return NULL;

	os_mutex_lock(&multi_mutex, OS_FOREVER);
	for (i = 0; i < BT_MULTI_CONNECT_MAX; i++) {
		if (conn_info[i].ble_conn == conn) {
			SYS_LOG_INF("exist conn (%p) i %d\n", conn, i);
			os_mutex_unlock(&multi_mutex);
			return &conn_info[i];
		}
	}

	os_mutex_unlock(&multi_mutex);
	return NULL;
}

static int mgr_char2hex(const char *c, uint8_t *x)
{
	if (*c >= '0' && *c <= '9') {
		*x = *c - '0';
	} else if (*c >= 'a' && *c <= 'f') {
		*x = *c - 'a' + 10;
	} else if (*c >= 'A' && *c <= 'F') {
		*x = *c - 'A' + 10;
	} else {
		return -EINVAL;
	}

	return 0;
}

static int mgr_str2bt_addr(const char *str, bd_address_t *addr)
{
	int i, j;
	uint8_t tmp;

	if (strlen(str) != 17) {
		return -EINVAL;
	}

	for (i = 5, j = 1; *str != '\0'; str++, j++) {
		if (!(j % 3) && (*str != ':')) {
			return -EINVAL;
		} else if (*str == ':') {
			i--;
			continue;
		}

		addr->val[i] = addr->val[i] << 4;

		if (mgr_char2hex(str, &tmp) < 0) {
			return -EINVAL;
		}

		addr->val[i] |= tmp;
	}

	return 0;
}

#define MULTI_BLE_SERVICE_UUID1		BT_UUID_DECLARE_16(0xFFB0)
#define MULTI_BLE_WRITE_UUID1		BT_UUID_DECLARE_16(0xFFB1)
#define MULTI_BLE_READ_UUID1		BT_UUID_DECLARE_16(0xFFB2)

static ssize_t multi_write_cb1(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len, uint16_t offset,
			      uint8_t flags)
{
	SYS_LOG_INF("conn %p\n", conn);

	printk("mRx: %d byte\n", len);
	return len;
}

static void multi_ccc_cfg_changed1(const struct bt_gatt_attr *attr, uint16_t value)
{
	SYS_LOG_INF("mvalue: %d\n", value);
}

static struct bt_gatt_attr ble_multi_attrs1[] = {
	BT_GATT_PRIMARY_SERVICE(MULTI_BLE_SERVICE_UUID1),

	BT_GATT_CHARACTERISTIC(MULTI_BLE_WRITE_UUID1, BT_GATT_CHRC_WRITE|BT_GATT_CHRC_WRITE_WITHOUT_RESP,
						BT_GATT_PERM_WRITE, NULL, multi_write_cb1, NULL),

	BT_GATT_CHARACTERISTIC(MULTI_BLE_READ_UUID1, BT_GATT_CHRC_NOTIFY,
				BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, NULL, NULL),
	BT_GATT_CCC(multi_ccc_cfg_changed1, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
};
#if 0
#define MULTI_BLE_SERVICE_UUID2		BT_UUID_DECLARE_16(0xFEB0)
#define MULTI_BLE_WRITE_UUID2		BT_UUID_DECLARE_16(0xFEB1)
#define MULTI_BLE_READ_UUID2			BT_UUID_DECLARE_16(0xFEB2)

static ssize_t multi_write_cb2(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len, uint16_t offset,
			      uint8_t flags)
{
	SYS_LOG_INF("conn %p\n", conn);

	printk("mRx: %d byte\n", len);
	return len;
}

static void multi_ccc_cfg_changed2(const struct bt_gatt_attr *attr, uint16_t value)
{
	SYS_LOG_INF("mvalue: %d\n", value);
}

static struct bt_gatt_attr ble_multi_attrs2[] = {
	BT_GATT_PRIMARY_SERVICE(MULTI_BLE_SERVICE_UUID2),

	BT_GATT_CHARACTERISTIC(MULTI_BLE_WRITE_UUID2, BT_GATT_CHRC_WRITE|BT_GATT_CHRC_WRITE_WITHOUT_RESP,
						BT_GATT_PERM_WRITE, NULL, multi_write_cb2, NULL),

	BT_GATT_CHARACTERISTIC(MULTI_BLE_READ_UUID2, BT_GATT_CHRC_NOTIFY,
				BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, NULL, NULL),
	BT_GATT_CCC(multi_ccc_cfg_changed2, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
};
#endif

static int shell_multi_create_id(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 3) {
		SYS_LOG_ERR(" ");
		return -EINVAL;
	}

	int err;
	bt_addr_le_t addr;
	int id;
	int id_count = CONFIG_BT_ID_MAX;

	addr.type = strtoul(argv[1], NULL, 16);
	err = mgr_str2bt_addr(argv[2], (bd_address_t *)&addr.a);
	if (err) {
		SYS_LOG_INF("Invalid peer address (err %d)\n", err);
		return err;
	}

	bt_id_get(NULL, &id_count);
	if (id_count > CONFIG_BT_ID_MAX) {
		SYS_LOG_ERR("id_count %d.", id_count);
		return -EINVAL;
	}

	id = bt_id_create(&addr, NULL);
	if (id < 0) {
		SYS_LOG_ERR("Create id failed (%d)\n", id);
		return -EINVAL;
	} else {
		SYS_LOG_INF("New id: %d\n", id);
	}

	// current ID <= 1, add gatt service for test.
	if (1 == id) {
		s_svc[0].attrs = ble_multi_attrs1;
		s_svc[0].attr_count = ARRAY_SIZE(ble_multi_attrs1);
		hostif_bt_gatt_service_register(&s_svc[0]);
	}
#if 0
	if (2 == id) {
		s_svc[1].attrs = ble_multi_attrs2;
		s_svc[1].attr_count = ARRAY_SIZE(ble_multi_attrs2);
		hostif_bt_gatt_service_register(&s_svc[1]);
	}
#endif
	SYS_LOG_INF("Register ble service\n");

	return 0;
}

static int shell_multi_delete_id(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 2) {
		SYS_LOG_ERR(" ");
		return -EINVAL;
	}

	int err;
	int id;

	id = strtoul(argv[1], NULL, 16);
	if (id >= CONFIG_BT_ID_MAX) {
		SYS_LOG_ERR("id %d.", id);
		return -EINVAL;
	}

	err = bt_id_delete(id);
	if (err < 0) {
		SYS_LOG_ERR("delete id failed (%d) err %d\n", id, err);
	} else {
		SYS_LOG_INF("delete id: %d\n", id);
	}
	if (id > 0) {
		hostif_bt_gatt_service_unregister(&s_svc[id-1]);
	}

	return 0;
}

static int shell_multi_reset_id(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 3) {
		SYS_LOG_ERR(" ");
		return -EINVAL;
	}

	int err;
	bt_addr_le_t addr;
	int id;

	id = strtoul(argv[1], NULL, 16);
	if (id >= CONFIG_BT_ID_MAX) {
		SYS_LOG_ERR("id %d.", id);
		return -EINVAL;
	}

	addr.type = strtoul(argv[2], NULL, 16);
	err = mgr_str2bt_addr(argv[3], (bd_address_t *)&addr.a);
	if (err) {
		SYS_LOG_INF("Invalid peer address (err %d)\n", err);
		return err;
	}

	id = bt_id_reset(id, &addr, NULL);
	if (id < 0) {
		SYS_LOG_ERR("ReCreate id failed (%d)\n", id);
	} else {
		SYS_LOG_INF("Reset id: %d\n", id);
	}

	return 0;
}

#if 0
static void le_auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	SYS_LOG_INF("le addr:%s",addr);
}

static void le_pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	struct bt_conn_info info;
	char addr[13];

	if ((hostif_bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_LE)){
		return;
	}

	memset(addr, 0, 13);
	bin2hex(info.le.dst->a.val, 6, addr, 12);
	SYS_LOG_INF("le addr %s reason %d", addr,reason);
}

static void le_pairing_complete(struct bt_conn *conn, bool bonded)
{
	struct bt_conn_info info;
	char addr[13];

	if ((hostif_bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_LE)){
		return;
	}

	memset(addr, 0, 13);
	bin2hex(info.le.dst->a.val, 6, addr, 12);
	SYS_LOG_INF("le addr %s bonded %d", addr, bonded);
}

/* IO_NO_INPUT_OUTPUT for le */
static struct bt_conn_auth_cb multi_auth_cb_confirm = {
	.passkey_display = NULL,
	.passkey_confirm = NULL,
	.cancel = le_auth_cancel,
	.pairing_failed = le_pairing_failed,
	.pairing_complete = le_pairing_complete,
};
#endif

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[13];
	struct bt_conn_info info;

	if (err) {
		SYS_LOG_ERR("slave fail to connect(%u)", err);
		return;
	}

	if ((hostif_bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_LE) || (info.role != BT_HCI_ROLE_SLAVE)) {
		return;
	}

	if (BT_ID_DEFAULT == info.id) {
		SYS_LOG_ERR("info.id %d.", info.id);
		return;
	}

	if (multi_st_get(conn)) {
		return;
	}

	if (multi_add_st(conn)) {
		return;
	}

	struct multi_mgr_info *st = multi_st_get(conn);

	memcpy(st->device_mac, info.le.dst->a.val, 6);
	memset(addr, 0, 13);
	bin2hex(st->device_mac, 6, addr, 12);
	SYS_LOG_INF("MULTIBle connected MAC: %s inv %d lat %d timeout %d",
		addr, info.le.interval, info.le.latency, info.le.timeout);
	st->ble_conn = hostif_bt_conn_ref(conn);
	st->ble_state = BT_STATUS_BLE_CONNECTED;
	st->le_conn_param.interval_min = info.le.interval;
	st->le_conn_param.interval_max = info.le.interval;
	st->le_conn_param.latency = info.le.latency;
	st->le_conn_param.timeout = info.le.timeout;
}

void adv_connected_cb(struct bt_le_ext_adv *adv,
		  struct bt_le_ext_adv_connected_info *info)
{
	if (adv->id < CONFIG_BT_ID_MAX) {
		hostif_bt_le_ext_adv_delete(s_adv[adv->id]);
		s_adv[adv->id] = NULL;
	}
}

static struct bt_le_ext_adv_cb adv_callbacks = {
	.connected = adv_connected_cb,
};

static int __multi_adv_start(u8_t id)
{
	int err;
	struct bt_le_adv_param ext_adv_params = {
		/* BT_LE_EXT_ADV_NCONN */
		.id = id,
		/* [100ms, 100ms] by default */
		.interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
		.interval_max = BT_GAP_ADV_FAST_INT_MIN_2,
		.options = BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_IDENTITY,
		.sid = 2,
	};

	err = hostif_bt_le_ext_adv_create(&ext_adv_params, &adv_callbacks, &(s_adv[id]));
	/* Create a non-connectable non-scannable advertising set */
	if (err) {
		SYS_LOG_ERR("Failed to create advertising set (err %d)\n", err);
		return -EALREADY;
	}

	err = hostif_bt_le_ext_adv_start(s_adv[id], BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		SYS_LOG_ERR("Failed to start extended advertising (err %d)\n", err);
		return -EALREADY;
	}

	struct bt_data ad[1];
	struct bt_data sd[1];
	int ad_items = 0;
	int sd_items = 0;
	char adv_name[28] = "ADV-TEST_ID_X";

	ad[ad_items].type = BT_DATA_MANUFACTURER_DATA;
	ad[ad_items].data_len = strlen(multi_adv_data)+1;
	ad[ad_items].data = (uint8_t *)multi_adv_data;
	ad_items++;

	adv_name[strlen(adv_name)-1] = 0x30 + id;
	sd[sd_items].type = BT_DATA_NAME_COMPLETE;
	sd[sd_items].data_len = strlen(adv_name)+1;
	sd[sd_items].data = (uint8_t *)adv_name;
	sd_items++;

	bt_set_name_by_id(id, adv_name);

	err = hostif_bt_le_ext_adv_set_data(s_adv[id], ad,
				ad_items, sd, sd_items);
	if (err) {
		SYS_LOG_INF("set data: %d", err);
		return err;
	}

	return 0;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[13];
	int id_count = CONFIG_BT_ID_MAX;
	struct bt_conn_info info;
	if ((hostif_bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_LE) || (info.role != BT_HCI_ROLE_SLAVE)) {
		return;
	}

	struct multi_mgr_info *st = multi_st_get(conn);

	if (!st) {
		return;
	}

	memset(addr, 0, sizeof(addr));
	bin2hex(st->device_mac, 6, addr, 12);
	SYS_LOG_INF("multiBle disconnected MAC: %s, reason: %d", addr, reason);
	hostif_bt_conn_unref(st->ble_conn);
	multi_remove_st(st->ble_conn);
	st->ble_conn = NULL;
	st->ble_state = BT_STATUS_BLE_DISCONNECTED;
	st->rssi = 0x7F;

	if (info.id > 0) {
		// restart adv
		bt_id_get(NULL, &id_count);
		if (id_count > CONFIG_BT_ID_MAX) {
			SYS_LOG_ERR("id_count %d.", id_count);
			return;
		}
		
		if (info.id >= id_count) {
			SYS_LOG_ERR("id %d, id_count %d.", info.id, id_count);
			return;
		}
		
		if (s_adv[info.id]) {
			SYS_LOG_ERR("id %d, id_count %d.", info.id, id_count);
			return;
		}
		
		__multi_adv_start(info.id);
	}
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	struct bt_conn_info info;

	if ((hostif_bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_LE) || (info.role != BT_HCI_ROLE_SLAVE)) {
		return false;
	}

	SYS_LOG_INF("int (0x%04x, 0x%04x) lat %d to %d", param->interval_min,
				param->interval_max, param->latency, param->timeout);
	return true;
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval,
			     uint16_t latency, uint16_t timeout,uint8_t status)
{
	struct bt_conn_info info;

	if ((hostif_bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_LE) || (info.role != BT_HCI_ROLE_SLAVE)) {
		return;
	}

	struct multi_mgr_info *st = multi_st_get(conn);

	if (!st) {
		return;
	}

	st->le_conn_param.interval_min = interval;
	st->le_conn_param.interval_max = interval;
	st->le_conn_param.latency = latency;
	st->le_conn_param.timeout = timeout;
	SYS_LOG_INF("status %d inv %d lat %d to %d", status, interval, latency, timeout);
}

static struct bt_conn_cb multi_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_req = le_param_req,
	.le_param_updated = le_param_updated,
};

static int shell_multi_create_adv(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 2) {
		SYS_LOG_ERR(" ");
		return -EINVAL;
	}

	int id;
	int id_count = CONFIG_BT_ID_MAX;

	id = strtoul(argv[1], NULL, 16);
	bt_id_get(NULL, &id_count);
	if (id_count > CONFIG_BT_ID_MAX) {
		SYS_LOG_ERR("id_count %d.", id_count);
		return -EINVAL;
	}

	if (id >= id_count) {
		SYS_LOG_ERR("id %d, id_count %d.", id, id_count);
		return -EINVAL;
	}

	if (s_adv[id]) {
		SYS_LOG_ERR("id %d, id_count %d.", id, id_count);
		return -EINVAL;
	}

	__multi_adv_start(id);

	if (false == multi_init) {
		hostif_bt_conn_cb_register(&multi_callbacks);
		//hostif_bt_conn_le_auth_cb_register(&multi_auth_cb_confirm);
		multi_init = true;
	}

	return 0;
}

static int shell_multi_delete_adv(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 2) {
		SYS_LOG_ERR(" ");
		return -EINVAL;
	}

	int err;
	int id;
	int id_count = CONFIG_BT_ID_MAX;

	id = strtoul(argv[1], NULL, 16);
	bt_id_get(NULL, &id_count);
	if (id_count > CONFIG_BT_ID_MAX) {
		SYS_LOG_ERR("id_count %d.", id_count);
		return -EINVAL;
	}

	if (id >= id_count) {
		SYS_LOG_ERR("id %d, id_count %d.", id, id_count);
		return -EINVAL;
	}

	if (!s_adv[id]) {
		SYS_LOG_ERR("id %d, id_count %d.", id, id_count);
		return -EINVAL;
	}

	/* Stop extended advertising */
	err = hostif_bt_le_ext_adv_stop(s_adv[id]);
	if (err) {
		SYS_LOG_ERR("ext_adv: %d", err);
		//return err;
	}
	hostif_bt_le_ext_adv_delete(s_adv[id]);
	s_adv[id] = NULL;

	return 0;
}

char *multi_send_data1 = "notify1 data test.";
//char *multi_send_data2 = "notify2 data test.";

static int shell_multi_gatts_send(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 2) {
		SYS_LOG_ERR(" ");
		return -EINVAL;
	}

	int num = strtoul(argv[1], NULL, 16);
	int ret = -1;

	if (BT_MULTI_CONNECT_MAX <= num) {
		SYS_LOG_ERR("num %d.", num);
		return -EINVAL;
	}

	if (!conn_info[num].ble_conn) {
		SYS_LOG_ERR("not connect %d.", num);
		return -EINVAL;
	}

	struct bt_conn_info info;

	if ((hostif_bt_conn_get_info(conn_info[num].ble_conn, &info) < 0) || (info.type != BT_CONN_TYPE_LE) || (info.role != BT_HCI_ROLE_SLAVE)) {
		return -EINVAL;
	}

	if (1 == info.id) {
		ret = hostif_bt_gatt_notify(conn_info[num].ble_conn, &ble_multi_attrs1[4], multi_send_data1, strlen(multi_send_data1));
	} //else if (2 == info.id) {
	//	ret = hostif_bt_gatt_notify(conn_info[num].ble_conn, &ble_multi_attrs2[4], multi_send_data2, strlen(multi_send_data2));
	//}
	SYS_LOG_INF("ret %d.", ret);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(bt_multi_cmds,
	SHELL_CMD(multi_create_id, NULL, "LE multi_create_id", shell_multi_create_id),
	SHELL_CMD(multi_delete_id, NULL, "LE multi_delete_id", shell_multi_delete_id),
	SHELL_CMD(multi_reset_id, NULL, "LE multi_reset_id", shell_multi_reset_id),
	SHELL_CMD(multi_create_adv, NULL, "LE multi_create_adv", shell_multi_create_adv),
	SHELL_CMD(multi_delete_adv, NULL, "LE multi_delete_adv", shell_multi_delete_adv),
	SHELL_CMD(multi_gatts_send, NULL, "LE multi_gatts_send", shell_multi_gatts_send),

	SHELL_SUBCMD_SET_END
);

static int cmd_bt_multi(const struct shell *shell, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(shell);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(shell, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

SHELL_CMD_REGISTER(btmulti, &bt_multi_cmds, "Bluetooth multiple test shell commands", cmd_bt_multi);
