/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager genarate bt mac and name.
 */
#define SYS_LOG_DOMAIN "bt manager"

#include <os_common_api.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <msg_manager.h>
#include <mem_manager.h>
#include <sys_manager.h>
#include <bt_manager.h>
#include "bt_manager_inner.h"
#include <sys_event.h>
#include <stream.h>
#include "bt_porting_inner.h"
#include <drivers/bluetooth/bt_drv.h>

#ifdef CONFIG_PROPERTY
#include <property_manager.h>
#endif

enum {
    BLE_SUPER_CTL_GROUP_NONE,
    BLE_SUPER_CTL_GROUP_BT,
    BLE_SUPER_CTL_GROUP_MAX,
};

enum {
    BLE_SUPER_BT_CTL_NONE,
    BLE_SUPER_BT_CTL_ENTER_BQB,
    BLE_SUPER_BT_CTL_PINCODE_ENABLE,
    BLE_SUPER_BT_BLE_SPEED,
};

enum {
	BLE_SUPER_PHONE_TYPE_ANDROID,
	BLE_SUPER_PHONE_TYPE_IOS,
    BLE_SUPER_PHONE_TYPE_MAX,
};

enum {
	BLE_SUPER_SPEED_LEVEL_FAST,
	BLE_SUPER_SPEED_LEVEL_IDLE,
	BLE_SUPER_SPEED_LEVEL_IDLE_NO_LATENCY,
	BLE_SUPER_SPEED_LEVEL_MAX,
};

#define BLE_SUPER_SERVICE_UUID           BT_UUID_DECLARE_16(0xFFD0)
#define BLE_SUPER_CONTROL_UUID           BT_UUID_DECLARE_16(0xFFD1)
#define BLE_SUPER_STATUS_UUID            BT_UUID_DECLARE_16(0xFFD2)
#define BLE_SUPER_TEST_UUID              BT_UUID_DECLARE_16(0xFFD3)

#define BLE_SUPER_CONTROL_HEAD_SIZE    4

struct ble_super_control_t{
    uint8_t group;
    uint8_t code;
    uint8_t param;
    uint8_t len;
    uint8_t *buff;
};

struct ble_super_srv_info_t {
	uint8_t connected:1;
	uint8_t test_cfg_enable:1;
};

static struct ble_super_srv_info_t super_info;

static void super_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	SYS_LOG_INF("super ccc cfg %d", value);
}

static void super_ccc_test_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	SYS_LOG_INF("super ccc test cfg %d", value);
	super_info.test_cfg_enable = value ? 1 : 0;
}

static int ble_super_test_write_cb(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len, uint16_t offset,
			      uint8_t flags)
{
	SYS_LOG_INF("Test write len %d", len);
	return len;
}

static void ble_super_connect_cb(uint8_t *mac, uint8_t connected)
{
	SYS_LOG_INF("BLE super service %s", connected ? "connected" : "disconnected");
	super_info.connected = connected ? 1 : 0;
	if (!connected) {
		super_info.test_cfg_enable = 0;
	}
}

#ifdef CONFIG_BT_CTRL_BQB
int pmuadc_mode_switch(bool always_on);
static void ble_super_enter_bqb(uint8_t bqb_mode)
{
	btdrv_init_param_t param;
    int ret;
	memset(&param, 0, sizeof(param));
	param.set_hosc_cap = 1;
	param.hosc_capacity = 0x64;			/* Wait todo: get for config or nvram */
	param.set_max_rf_power = 1;
	param.bt_max_rf_tx_power = 38;		/* 8db */
	//param.set_ble_rf_power = 1;
	//param.ble_rf_tx_power = 34;
	ret = btdrv_set_init_param(&param);
    if(!ret){
        /* BQB mode [0:BR BQB Test, 1:BLE BQB Test, 2:BR/BLE dual BQB Test] */
        extern int bqb_init(int bqb_mode);

        pmuadc_mode_switch(1);
        bt_wake_lock();
        bqb_init((bqb_mode - 1));
        SYS_LOG_INF("Enter BQB mode %d", (bqb_mode - 1));
    }
}
#endif

void bt_manager_ble_super_enter_bqb(uint8_t mode)
{
    SYS_LOG_INF("mode:%d", mode);

#ifdef CONFIG_BT_CTRL_BQB
    if ((mode > DISABLE_TEST) && (mode <= DUT_LE_TEST)) {
        ble_super_enter_bqb(mode);
    }
#endif

	struct app_msg  msg = {0};
	msg.type = MSG_APP_BLE_SUPER_BQB;
	send_async_msg("main", &msg);
}

static void ble_super_ctrl_enter_bqb(uint8_t mode)
{
	struct app_msg  msg = {0};
	msg.type = MSG_REBOOT;
    if(mode == LE_TEST){
	    msg.cmd = REBOOT_REASON_SUPER_LE_BQB;
	}
    else{
	    msg.cmd = REBOOT_REASON_SUPER_BR_BQB;
    }
	send_async_msg("main", &msg);
}

static int ble_super_ctrl_enable_pincode(uint8_t enable)
{
#ifdef CONFIG_PROPERTY
	int ret = -EIO;
	uint8_t value = enable;

	ret = property_set(CFG_BT_PINCODE_MODE, (char *)&value, 1);
	if (!ret) {
        ret = property_flush(NULL);
		return ret;
	} else {
		return 0;
	}
#else
    return 0;
#endif
}

const struct bt_le_conn_param le_super_conn_param[BLE_SUPER_PHONE_TYPE_MAX][BLE_SUPER_SPEED_LEVEL_MAX] = {
	{
		{16,48,0,400},
		{128,160,4,400},
		{400,420,0,400},
	},
	{
		{12,28,0,600},
		{32,48,4,600},
		{400,420,0,600},
	},
};

static int ble_super_ctrl_set_speed(uint8_t expect_speed)
{
#ifndef CONFIG_BT_BLE
	return 0;
#endif

#ifdef CONFIG_BT_BLE_APP_UPDATE_PARAM
    int ret;
    uint8_t phone_type = bt_manager_ble_get_phone_type();
    uint16_t interval, latency,timeout;
    uint16_t expect_interval_min, expect_interval_max;

    ret = bt_manager_ble_get_param(&interval,&latency,&timeout);
    if(ret){
        return ret;
    }

    if(expect_speed >= BLE_SUPER_SPEED_LEVEL_MAX){
        return -EINVAL;
    }

	interval += interval*latency;
	expect_interval_min = le_super_conn_param[phone_type][expect_speed].interval_min * (1 + le_super_conn_param[phone_type][expect_speed].latency);
	expect_interval_max = le_super_conn_param[phone_type][expect_speed].interval_max * (1 + le_super_conn_param[phone_type][expect_speed].latency);

	if ((interval < expect_interval_min) || (interval > expect_interval_max)) {
		ret = bt_manager_ble_update_param(&le_super_conn_param[phone_type][expect_speed]);
	}
	else{
        ret = -EINVAL;
	}

	SYS_LOG_INF("ret= %d,param:%d %d", ret ,phone_type, expect_speed);
    return ret;
#else
    return 0;
#endif
}

int bt_manager_ble_super_set_speed(uint8_t expect_speed)
{
	return ble_super_ctrl_set_speed(expect_speed);
}

static int ble_super_control_bt_handle(struct bt_conn *conn, struct ble_super_control_t *ctrl)
{
	int ret = 0;

	if (!ctrl) {
		return -EINVAL;
	}

	switch (ctrl->code) {
	case BLE_SUPER_BT_CTL_ENTER_BQB:
		ble_super_ctrl_enter_bqb(ctrl->param);
		break;

	case BLE_SUPER_BT_CTL_PINCODE_ENABLE:
		ble_super_ctrl_enable_pincode(ctrl->param);
		break;

	case BLE_SUPER_BT_BLE_SPEED:
		ble_super_ctrl_set_speed(ctrl->param);
		break;

	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int ble_super_control_cb(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len, uint16_t offset,
			      uint8_t flags)
{
	uint8_t *rx_data = (uint8_t *)buf;
	struct ble_super_control_t srv_cmd;
	int ret = -EINVAL;

	if (len < BLE_SUPER_CONTROL_HEAD_SIZE) {
		return -EINVAL;
	}

	srv_cmd.group = rx_data[0];
	srv_cmd.code = rx_data[1];
	srv_cmd.param = rx_data[2];
	srv_cmd.len = rx_data[3];
	srv_cmd.buff = NULL;
	if ((srv_cmd.len > 0) && (srv_cmd.len <= (len - BLE_SUPER_CONTROL_HEAD_SIZE))) {
		srv_cmd.buff = &rx_data[4];
	}

	SYS_LOG_INF("group %d cpde %d param %d", srv_cmd.group, srv_cmd.code, srv_cmd.param);

	switch (srv_cmd.group) {
	case BLE_SUPER_CTL_GROUP_BT:
		ret = ble_super_control_bt_handle(conn, &srv_cmd);
		break;
	default:
		break;
	}

	return (ret < 0) ? ret : len;
}

static struct ble_reg_manager ble_super_mgr = {
	.link_cb = ble_super_connect_cb,
};

static  struct bt_gatt_attr actions_super_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BLE_SUPER_SERVICE_UUID),

	BT_GATT_CHARACTERISTIC(BLE_SUPER_CONTROL_UUID, BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
						BT_GATT_PERM_WRITE, NULL, ble_super_control_cb, NULL),

	BT_GATT_CHARACTERISTIC(BLE_SUPER_STATUS_UUID, BT_GATT_CHRC_NOTIFY,
				BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, NULL, NULL),

#ifdef CONFIG_ACTIVE_REQ_BLE_ENC
	BT_GATT_CCC(super_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
#else
	BT_GATT_CCC(super_ccc_cfg_changed, BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
#endif

	BT_GATT_CHARACTERISTIC(BLE_SUPER_TEST_UUID, BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
				BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, ble_super_test_write_cb, NULL),
	BT_GATT_CCC(super_ccc_test_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
};

int ble_manager_super_register(void)
{
    ble_super_mgr.gatt_svc.attrs = actions_super_attrs;
    ble_super_mgr.gatt_svc.attr_count = ARRAY_SIZE(actions_super_attrs);
    bt_manager_ble_service_reg(&ble_super_mgr);
	return 0;
}

int bt_manager_ble_super_test_notify(void)
{
	#define SUPER_TEST_NOTIFY_DATA     "Super test notify"

	if (super_info.connected && super_info.test_cfg_enable) {
		return bt_manager_ble_send_data(&actions_super_attrs[6], &actions_super_attrs[7],
							SUPER_TEST_NOTIFY_DATA, strlen(SUPER_TEST_NOTIFY_DATA));
	}

	return 0;
}
