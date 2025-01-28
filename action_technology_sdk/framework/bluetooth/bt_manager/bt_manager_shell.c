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
#define SYS_LOG_DOMAIN "bt shell"

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

#define DEVICE_NAME "ble_test_308a"
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

#if CONFIG_BT_BR_ACTS
static int shell_dump_bt_info(const struct shell *shell, size_t argc, char *argv[])
{
	bt_manager_dump_info();

	return 0;
}

static int shell_cmd_br_test(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 2) {
		SYS_LOG_INF("Used: bt br_test on/off");
		return 0;
	}

	if (!strcmp(argv[1], "on")) {
		bt_manager_set_visible(true);
		bt_manager_set_connectable(true);
		SYS_LOG_INF("Br test on");
	} else if (!strcmp(argv[1], "off"))  {
		bt_manager_set_visible(false);
		bt_manager_set_connectable(false);
		SYS_LOG_INF("Br test off");
	} else if (!strcmp(argv[1], "visible") && argc > 2) {
		if (!strcmp(argv[2], "on")) {
			bt_manager_set_visible(true);
		} else if (!strcmp(argv[2], "off"))  {
			bt_manager_set_visible(false);
		} else {
			SYS_LOG_INF("Unknow visible cmd %s", argv[2]);
		}
	} else if (!strcmp(argv[1], "connectable") && argc > 2) {
		if (!strcmp(argv[2], "on")) {
			bt_manager_set_connectable(true);
		} else if (!strcmp(argv[2], "off"))  {
			bt_manager_set_connectable(false);
		} else {
			SYS_LOG_INF("Unknow connectable cmd %s", argv[2]);
		}
	} else if (!strcmp(argv[1], "passkey") && argc > 2) {
			void btsrv_adapter_passkey_display(bool mode);
			if (!strcmp(argv[2], "on")) {
				btsrv_adapter_passkey_display(true);
			} else if (!strcmp(argv[2], "off"))  {
				btsrv_adapter_passkey_display(false);
			} else {
				SYS_LOG_INF("Unknow connectable cmd %s", argv[2]);
			}
	} else {
		SYS_LOG_INF("Unknow cmd %s", argv[1]);
	}

	return 0;
}
#endif

static int shell_cmd_ble_test(const struct shell *shell, size_t argc, char *argv[])
{
#ifdef CONFIG_BT_BLE
	if (argc < 2) {
		SYS_LOG_INF("Used: bt ble_test on/off");
		return 0;
	}

	if (!strcmp(argv[1], "on")) {
		bt_manager_ble_adv_start();
		SYS_LOG_INF("ble test on");
	} else if (!strcmp(argv[1], "off"))  {
		bt_manager_ble_adv_stop();
		SYS_LOG_INF("ble test off");
	} else if (!strcmp(argv[1], "adv_set"))  {
		size_t len = ARRAY_SIZE(ad);
		bt_manager_ble_set_adv_data(ad, len, NULL, 0);
		SYS_LOG_INF("ble test adv_reset");
	}  else if (!strcmp(argv[1], "passkey") && argc > 2) {
			void bt_manager_ble_passkey_display(bool mode);
			if (!strcmp(argv[2], "on")) {
				bt_manager_ble_passkey_display(true);
			} else if (!strcmp(argv[2], "off"))  {
				bt_manager_ble_passkey_display(false);
			} else {
				SYS_LOG_INF("Unknow connectable cmd %s", argv[2]);
			}
	} else {
		SYS_LOG_INF("Unknow cmd %s", argv[1]);
	}
#endif
	return 0;
}

static int shell_cmd_set_ble_speed(const struct shell *shell, size_t argc, char *argv[])
{
	uint32_t val;

	if (argc < 2) {
		SYS_LOG_INF("Used: bt ble_speed 0/1/2");
		return 0;
	}

	val = strtoul(argv[1], NULL, 16);
	if (val <= 2) {
		bt_manager_ble_super_set_speed(val);
	} else {
		SYS_LOG_INF("Unknow speed %s", argv[1]);
	}

	return 0;
}

#if CONFIG_BT_BR_ACTS
static int shell_cmd_send_hfp_cmd(const struct shell *shell, size_t argc, char *argv[])
{
	char cmd_buf[32];
	uint8_t len;

	if (argc < 2) {
		SYS_LOG_INF("Used: bt hfp cmd");
		return 0;
	}

	memset(cmd_buf, 0, sizeof(cmd_buf));

	if (!strcmp(argv[1], "DTMF")) {
		/* Send DTMF one by one,
		 * Cmd: bt hfp DTMF 1/2/3/...
		 */
		len = strlen("AT+VTS=");
		memcpy(cmd_buf, "AT+VTS=", len);
		if (argc >= 3) {
			cmd_buf[len] = argv[2][0];
		} else {
			cmd_buf[len] = '1';
		}
		SYS_LOG_INF("Hfp send DTMF %s", &cmd_buf[len]);
#ifdef CONFIG_BT_HFP_HF
		bt_manager_hfp_send_at_command(cmd_buf, 1);
#endif
	}

	return 0;
}
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(bt_cmds,
#if CONFIG_BT_BR_ACTS
	SHELL_CMD(info, NULL, "dump bt info", shell_dump_bt_info),
	SHELL_CMD(br_test, NULL, "br power test", shell_cmd_br_test),
#endif
	SHELL_CMD(ble_test, NULL, "ble power test", shell_cmd_ble_test),
	SHELL_CMD(ble_speed, NULL, "Set ble speed", shell_cmd_set_ble_speed),
#if CONFIG_BT_BR_ACTS
	SHELL_CMD(hfp, NULL, "send hfp cmd", shell_cmd_send_hfp_cmd),
#endif
	SHELL_SUBCMD_SET_END
);

static int cmd_bt_shell(const struct shell *shell, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(shell);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(shell, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

SHELL_CMD_REGISTER(bt, &bt_cmds, "Bluetooth manager commands", cmd_bt_shell);
