/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager.
 */
#include <os_common_api.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <msg_manager.h>
#include <mem_manager.h>
#include <bt_manager.h>
#include <sys_event.h>
#include <btservice_api.h>
#include <shell/shell.h>
#include <acts_bluetooth/host_interface.h>
#include <property_manager.h>
#include "ble_master_inner.h"

struct le_scan_param shell_le_scan_cb;

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

/* Ble address type
 * #define BT_ADDR_LE_PUBLIC       0x00
 * #define BT_ADDR_LE_RANDOM       0x01
 * #define BT_ADDR_LE_PUBLIC_ID    0x02
 * #define BT_ADDR_LE_RANDOM_ID    0x03
 */
static int shell_cmd_lemgr_master_connect(const struct shell *shell, size_t argc, char *argv[])
{
	bt_addr_le_t addr;
	bd_address_t addrn;
	int err;

	if (argc < 3) {
		SYS_LOG_INF("CMD link: blemaster le_connect CB:4E:FD:EC:1E:AC 0");
		return -EINVAL;
	}

	err = mgr_str2bt_addr(argv[1], &addrn);
	if (err) {
		SYS_LOG_INF("Invalid peer address (err %d)\n", err);
		return err;
	}

	memcpy(&(addr.a), &addrn, sizeof(addrn));
	addr.type = argv[2][0] - 0x30;
	if (addr.type > 3) {
		addr.type = 0;
	}

	if (le_master_get_scan_status())
	{
		le_master_scan_stop();
	}

	return le_master_dev_connect(&addr);
}


static int shell_cmd_lemgr_master_disconnect(const struct shell *shell, size_t argc, char *argv[])
{
	bt_addr_le_t addr;
	bd_address_t addrn;
	int err;

	if (argc < 3) {
		SYS_LOG_INF("CMD link: blemaster le_disconnect CB:4E:FD:EC:1E:AC 0");
		return -EINVAL;
	}

	err = mgr_str2bt_addr(argv[1], &addrn);
	if (err) {
		SYS_LOG_INF("Invalid peer address (err %d)\n", err);
		return err;
	}

	memcpy(&(addr.a), &addrn, sizeof(addrn));
	addr.type = argv[2][0] - 0x30;
	if (addr.type > 3) {
		addr.type = 0;
	}

	if (le_master_get_scan_status())
	{
		le_master_scan_stop();
	}

	return le_master_dev_disconnect(&addr);
}


void shell_device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char dev[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, dev, sizeof(dev));

	if (rssi < -50) {
	    return;
	}

	if (BLE_CONN_CONNECTED == conn_master_get_state_by_addr(addr)) {
		SYS_LOG_INF("Ignore connected device (%s)", dev);
		return;
	}

    SYS_LOG_INF("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %d", dev, type, ad->len, rssi);
}

static int shell_cmd_lemgr_master_scanstart(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 1) {
		SYS_LOG_INF("CMD scan start: blemaster scan_start\n");
		return -EINVAL;
	}

	shell_le_scan_cb.cb = &shell_device_found;
	return le_master_scan_start(&shell_le_scan_cb);
}

static int shell_cmd_lemgr_master_scanstop(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 1) {
		SYS_LOG_INF("CMD scan start: blemaster scan_stop\n");
		return -EINVAL;
	}

	return le_master_scan_stop();
}

#define GATT_TEST_WRITE_DATA                    "1234567890"
#define GATT_TEST_CONTINUE_WRITE_INTERVAL       50		/* 50ms */

static void ble_master_write_work_handle(struct k_work *work);
static OS_DELAY_WORK_DEFINE(ble_master_write_delaywork, ble_master_write_work_handle);
static uint8_t test_continue_flag;

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

static int ble_master_test_write_one(void)
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
		if (ble_dev->test_write_wait_finish) {
			continue;
		}

		ble_dev->test_write_params.func = write_func_cb;
		ble_dev->test_write_params.offset = 0;
		ble_dev->test_write_params.data = GATT_TEST_WRITE_DATA;
		ble_dev->test_write_params.length = strlen(GATT_TEST_WRITE_DATA);
		err = bt_gatt_write(ble_dev->conn, &ble_dev->test_write_params);
		if (err) {
			ble_dev->test_write_params.func = NULL;
			SYS_LOG_ERR("Gatt write failed (err %d)", err);
		} else {
			ble_dev->test_write_wait_finish = 1;
			SYS_LOG_INF("Gatt write data");
		}
	}

	return ret;
}

static void ble_master_write_work_handle(struct k_work *work)
{
	if (test_continue_flag == 0) {
		return;
	}

	if (ble_master_test_write_one() == 0) {
		os_delayed_work_submit(&ble_master_write_delaywork, GATT_TEST_CONTINUE_WRITE_INTERVAL);
	} else {
		test_continue_flag = 0;
		SYS_LOG_INF("Not ready for gatt continue write");
	}
}

static int shell_cmd_master_test_write(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 2) {
		ble_master_test_write_one();
		return 0;
	}

	if (!strcmp(argv[1], "continue_start")) {
		test_continue_flag = 1;
		os_delayed_work_submit(&ble_master_write_delaywork, GATT_TEST_CONTINUE_WRITE_INTERVAL);
		SYS_LOG_INF("Start gatt continue write");
	} else if (!strcmp(argv[1], "continue_stop")) {
		test_continue_flag = 0;
		os_delayed_work_cancel(&ble_master_write_delaywork);
		SYS_LOG_INF("Stop gatt continue write");
	} else {
		SYS_LOG_INF("Unknow cmd %s", argv[1]);
	}

	return 0;
}

static int shell_cmd_slave_super_test_notify(const struct shell *shell, size_t argc, char *argv[])
{
	bt_manager_ble_super_test_notify();
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(ble_master_cmds,
	SHELL_CMD(scan_start, NULL, "BLEMASTER scan start", shell_cmd_lemgr_master_scanstart),
	SHELL_CMD(scan_stop, NULL, "BLEMASTER scan stop", shell_cmd_lemgr_master_scanstop),
	SHELL_CMD(le_connect, NULL, "BLEMASTER connect", shell_cmd_lemgr_master_connect),
	SHELL_CMD(le_disconnect, NULL, "BLEMASTER disconnect", shell_cmd_lemgr_master_disconnect),
	SHELL_CMD(test_write, NULL, "Test write", shell_cmd_master_test_write),
	SHELL_CMD(slave_send, NULL, "Slave super test notify", shell_cmd_slave_super_test_notify),
	SHELL_SUBCMD_SET_END
);

static int cmd_ble_master(const struct shell *shell, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(shell);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(shell, "%s unknown parameter: %s", argv[0], argv[1]);
	return -EINVAL;
}

SHELL_CMD_REGISTER(blemaster, &ble_master_cmds, "BLE Master test shell commands", cmd_ble_master);
