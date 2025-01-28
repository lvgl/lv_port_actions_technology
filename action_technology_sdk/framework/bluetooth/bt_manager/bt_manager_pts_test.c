/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager.
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


static int pts_connect_acl(const struct shell *shell, size_t argc, char *argv[])
{
	int cnt;
	struct autoconn_info info[3];

	memset(info, 0, sizeof(info));
	cnt = btif_br_get_auto_reconnect_info(info, 1);
	if (cnt == 0) {
		SYS_LOG_WRN("Never connect to pts dongle\n");
		return -1;
	}

	info[0].addr_valid = 1;
	info[0].tws_role = 0;
	info[0].a2dp = 0;
	info[0].hfp = 0;
	info[0].avrcp = 0;
	info[0].hfp_first = 0;
	btif_br_set_auto_reconnect_info(info, 3);

	btif_br_connect(&info[0].addr);
	return 0;
}

static int pts_connect_acl_a2dp_avrcp(const struct shell *shell, size_t argc, char *argv[])
{
	int cnt;
	struct autoconn_info info[3];

	memset(info, 0, sizeof(info));
	cnt = btif_br_get_auto_reconnect_info(info, 1);
	if (cnt == 0) {
		SYS_LOG_WRN("Never connect to pts dongle\n");
		return -1;
	}

	info[0].addr_valid = 1;
	info[0].tws_role = 0;
	info[0].a2dp = 1;
	info[0].hfp = 0;
	info[0].avrcp = 1;
	info[0].hfp_first = 0;
	btif_br_set_auto_reconnect_info(info, 3);
#ifdef CONFIG_PROPERTY
	property_flush(NULL);
#endif

	bt_manager_startup_reconnect();
	return 0;
}

static int pts_connect_acl_hfp(const struct shell *shell, size_t argc, char *argv[])
{
	int cnt;
	struct autoconn_info info[3];

	memset(info, 0, sizeof(info));
	cnt = btif_br_get_auto_reconnect_info(info, 1);
	if (cnt == 0) {
		SYS_LOG_WRN("Never connect to pts dongle\n");
		return -1;
	}

	info[0].addr_valid = 1;
	info[0].tws_role = 0;
	info[0].a2dp = 0;
	info[0].hfp = 1;
	info[0].avrcp = 0;
	info[0].hfp_first = 1;
	btif_br_set_auto_reconnect_info(info, 3);
#ifdef CONFIG_PROPERTY
	property_flush(NULL);
#endif
	bt_manager_startup_reconnect();
	return 0;
}

static int pts_hfp_cmd(const struct shell *shell, size_t argc, char *argv[])
{
	char *cmd;

	if (argc < 2) {
		SYS_LOG_INF("Used: pts hfp_cmd xx");
		return -EINVAL;
	}

	cmd = argv[1];
	SYS_LOG_INF("hfp cmd:%s", cmd);

	/* AT cmd
	 * "ATA"			Answer call
	 * "AT+CHUP"		rejuet call
	 * "ATD1234567;"	Place a Call with the Phone Number Supplied by the HF.
	 * "ATD>1;"			Memory Dialing from the HF.
	 * "AT+BLDN"		Last Number Re-Dial from the HF.
	 * "AT+CHLD=0"	Releases all held calls or sets User Determined User Busy (UDUB) for a waiting call.
	 * "AT+CHLD=x"	refer HFP_v1.7.2.pdf.
	 * "AT+NREC=x"	Noise Reduction and Echo Canceling.
	 * "AT+BVRA=x"	Bluetooth Voice Recognition Activation.
	 * "AT+VTS=x"		Transmit DTMF Codes.
	 * "AT+VGS=x"		Volume Level Synchronization.
	 * "AT+VGM=x"		Volume Level Synchronization.
	 * "AT+CLCC"		List of Current Calls in AG.
	 * "AT+BTRH"		Query Response and Hold Status/Put an Incoming Call on Hold from HF.
	 * "AT+CNUM"		HF query the AG subscriber number.
	 * "AT+BIA="		Bluetooth Indicators Activation.
	 * "AT+COPS?"		Query currently selected Network operator.
	 */

	if (btif_pts_send_hfp_cmd(cmd)) {
		SYS_LOG_WRN("Not ready\n");
	}
	return 0;
}

static int pts_hfp_connect_sco(const struct shell *shell, size_t argc, char *argv[])
{
	if (btif_pts_hfp_active_connect_sco()) {
		SYS_LOG_WRN("Not ready\n");
	}

	return 0;
}

static int pts_disconnect(const struct shell *shell, size_t argc, char *argv[])
{
	btif_br_disconnect_device(BTSRV_DISCONNECT_ALL_MODE);
	return 0;
}

static int pts_a2dp_test(const struct shell *shell, size_t argc, char *argv[])
{
#ifdef CONFIG_BT_A2DP
	char *cmd;
	uint8_t value;

	if (argc < 2) {
		SYS_LOG_INF("Used: pts a2dp xx");
		return -EINVAL;
	}

	cmd = argv[1];
	SYS_LOG_INF("a2dp cmd:%s", cmd);

	if (!strcmp(cmd, "delayreport")) {
		bt_manager_a2dp_send_delay_report(1000);	/* Delay report 100ms */
	} else if (!strcmp(cmd, "cfgerrcode")) {
		if (argc < 3) {
			SYS_LOG_INF("Used: pts a2dp cfgerrcode 0xXX");
		}

		value = strtoul(argv[2], NULL, 16);
		btif_pts_a2dp_set_err_code(value);
		SYS_LOG_INF("Set a2dp err code 0x%x", value);
	}
#endif

	return 0;
}

static int pts_avrcp_test(const struct shell *shell, size_t argc, char *argv[])
{
	char *cmd;
	uint8_t value;

	if (argc < 2) {
		SYS_LOG_INF("Used: pts avrcp xx");
		return -EINVAL;
	}

	cmd = argv[1];
	SYS_LOG_INF("avrcp cmd:%s", cmd);

	if (!strcmp(cmd, "playstatus")) {
		btif_avrcp_get_play_status();
	} else if (!strcmp(cmd, "pass")) {
		if (argc < 3) {
			SYS_LOG_INF("Used: pts avrcp pass 0xXX");
			return -EINVAL;
		}
		value = strtoul(argv[2], NULL, 16);
		btif_pts_avrcp_pass_through_cmd(value);
	} else if (!strcmp(cmd, "volume")) {
		if (argc < 3) {
			SYS_LOG_INF("Used: pts avrcp volume 0xXX");
			return -EINVAL;
		}
		value = strtoul(argv[2], NULL, 16);
		btif_pts_avrcp_notify_volume_change(value);
	} else if (!strcmp(cmd, "regnotify")) {
		btif_pts_avrcp_reg_notify_volume_change();
	}

	return 0;
}

static uint8_t spp_chnnel;

static void pts_spp_connect_failed_cb(uint8_t channel)
{
	SYS_LOG_INF("channel:%d", channel);
	spp_chnnel = 0;
}

static void pts_spp_connected_cb(uint8_t channel, uint8_t *uuid)
{
	SYS_LOG_INF("channel:%d", channel);
	spp_chnnel = channel;
}

static void pts_spp_receive_data_cb(uint8_t channel, uint8_t *data, uint32_t len)
{
	SYS_LOG_INF("channel:%d data len %d", channel, len);
}

static void pts_spp_disconnected_cb(uint8_t channel)
{
	SYS_LOG_INF("channel:%d", channel);
	spp_chnnel = 0;
}

static const struct btmgr_spp_cb pts_spp_cb = {
	.connect_failed = pts_spp_connect_failed_cb,
	.connected = pts_spp_connected_cb,
	.disconnected = pts_spp_disconnected_cb,
	.receive_data = pts_spp_receive_data_cb,
};

static const uint8_t pts_spp_uuid[16] = {0xFB, 0x34, 0x9B, 0x5F, 0x80, \
	0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x01, 0x11, 0x00, 0x00};

static void bt_pts_spp_connect(void)
{
	int cnt;
	struct autoconn_info info[3];

	memset(info, 0, sizeof(info));
	cnt = btif_br_get_auto_reconnect_info(info, 1);
	if (cnt == 0) {
		SYS_LOG_WRN("Never connect to pts dongle\n");
		return;
	}

	spp_chnnel = bt_manager_spp_connect(&info[0].addr, (uint8_t *)pts_spp_uuid, (struct btmgr_spp_cb *)&pts_spp_cb);
	SYS_LOG_INF("channel:%d", spp_chnnel);
}

static int pts_spp_test(const struct shell *shell, size_t argc, char *argv[])
{
	char *cmd;

	if (argc < 2) {
		SYS_LOG_INF("Used: pts spp xx");
		return -EINVAL;
	}

	cmd = argv[1];
	SYS_LOG_INF("spp cmd:%s", cmd);

	if (!strcmp(cmd, "register")) {
		bt_manager_spp_reg_uuid((uint8_t *)pts_spp_uuid, (struct btmgr_spp_cb *)&pts_spp_cb);
	} else if (!strcmp(cmd, "connect")) {
		bt_pts_spp_connect();
	} else if (!strcmp(cmd, "disconnect")) {
		if (spp_chnnel) {
			bt_manager_spp_disconnect(spp_chnnel);
		}
	}

	return 0;
}

#define HID_CLASS_OF_DEVICE	0x2425C0

static int pts_hid_test(const struct shell *shell, size_t argc, char *argv[])
{
	char *cmd;

	if (argc < 2) {
		SYS_LOG_INF("Used: pts hid xx");
		return -EINVAL;
	}

	cmd = argv[1];
	SYS_LOG_INF("hid cmd:%s", cmd);

	if (!strcmp(cmd, "init")) {
		bt_manager_set_visible(false);
		bt_manager_set_connectable(false);
		btif_pts_set_class_of_device(HID_CLASS_OF_DEVICE);
		hostif_bt_br_write_iac(false);
		bt_manager_set_visible(true);
		bt_manager_set_connectable(true);
	} else if (!strcmp(cmd, "liac")) {
		cmd = argv[2];
		SYS_LOG_INF("liac:%s", cmd);
		if (!strcmp(cmd, "on")) {
			hostif_bt_br_write_iac(true);
		} else if (!strcmp(cmd, "off")) {
			hostif_bt_br_write_iac(false);
		}
	}

	return 0;
}

static int pts_scan_test(const struct shell *shell, size_t argc, char *argv[])
{
	char *cmd;

	if (argc < 2) {
		SYS_LOG_INF("Used: pts scan on/off");
		return -EINVAL;
	}

	cmd = argv[1];
	SYS_LOG_INF("scan cmd:%s", cmd);

	if (!strcmp(cmd, "on")) {
		bt_manager_set_visible(true);
		bt_manager_set_connectable(true);
	} else if (!strcmp(cmd, "off")) {
		bt_manager_set_visible(false);
		bt_manager_set_connectable(false);
	}

	return 0;
}

static int pts_clean_test(const struct shell *shell, size_t argc, char *argv[])
{
	char *cmd;

	if (argc < 2) {
		SYS_LOG_INF("Used: pts clean xxx");
		return -EINVAL;
	}

	cmd = argv[1];
	SYS_LOG_INF("clean cmd:%s", cmd);

	if (!strcmp(cmd, "linkkey")) {
		btif_br_clean_linkkey(NULL);
	}

	return 0;
}

static int pts_auth_test(const struct shell *shell, size_t argc, char *argv[])
{
	char *cmd;

	if (argc < 2) {
		SYS_LOG_INF("Used: pts auth xxx");
		return -EINVAL;
	}

	cmd = argv[1];
	SYS_LOG_INF("auth cmd:%s", cmd);

	if (!strcmp(cmd, "register")) {
		btif_pts_register_auth_cb(true);
	} else if (!strcmp(cmd, "unregister")) {
		btif_pts_register_auth_cb(false);
	}

	return 0;
}

static int pts_hrs_test(const struct shell *shell, size_t argc, char *argv[])
{
#ifdef CONFIG_BT_HRS
	char *cmd;
	uint8_t value;

	if (argc < 2) {
		SYS_LOG_INF("Used: pts hrs xxx");
		return -EINVAL;
	}

	cmd = argv[1];
	SYS_LOG_INF("hrs cmd:%s", cmd);

	value = strtoul(argv[2], NULL, 16);
	SYS_LOG_INF("hrs value:%d.", value);
	int bt_hrs_notify(uint16_t heartrate);
	bt_hrs_notify(value);
#endif
	return 0;
}


SHELL_STATIC_SUBCMD_SET_CREATE(bt_mgr_pts_cmds,
	SHELL_CMD(connect_acl, NULL, "pts active connect acl", pts_connect_acl),
	SHELL_CMD(connect_acl_a2dp_avrcp, NULL, "pts active connect acl/a2dp/avrcp", pts_connect_acl_a2dp_avrcp),
	SHELL_CMD(connect_acl_hfp, NULL, "pts active connect acl/hfp", pts_connect_acl_hfp),
	SHELL_CMD(hfp_cmd, NULL, "pts hfp command", pts_hfp_cmd),
	SHELL_CMD(hfp_connect_sco, NULL, "pts hfp active connect sco", pts_hfp_connect_sco),
	SHELL_CMD(disconnect, NULL, "pts do disconnect", pts_disconnect),
	SHELL_CMD(a2dp, NULL, "pts a2dp test", pts_a2dp_test),
	SHELL_CMD(avrcp, NULL, "pts avrcp test", pts_avrcp_test),
	SHELL_CMD(spp, NULL, "pts spp test", pts_spp_test),
	SHELL_CMD(hid, NULL, "pts hid test", pts_hid_test),
	SHELL_CMD(scan, NULL, "pts scan test", pts_scan_test),
	SHELL_CMD(clean, NULL, "pts scan test", pts_clean_test),
	SHELL_CMD(auth, NULL, "pts auth test", pts_auth_test),
	SHELL_CMD(hrs, NULL, "hrs test", pts_hrs_test),
	SHELL_SUBCMD_SET_END
);

static int cmd_bt_pts(const struct shell *shell, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(shell);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(shell, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

SHELL_CMD_REGISTER(pts, &bt_mgr_pts_cmds, "Bluetooth manager pts test shell commands", cmd_bt_pts);
