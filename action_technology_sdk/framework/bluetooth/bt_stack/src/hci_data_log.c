/** @file
 * @brief Bluetooth hci data log record.
 *
 * Copyright (c) 2019 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME hci_log
#include <logging/log.h>
LOG_MODULE_DECLARE(os, LOG_LEVEL_INF);

#include <zephyr.h>
#include <shell/shell.h>
#include <sys/printk.h>
#include <stdlib.h>
#include <string.h>

#include <acts_bluetooth/buf.h>
#include "bt_porting_inner.h"

#if CONFIG_HCI_DATA_LOG

#define HCI_LOG_CMD		0x01
#define HCI_LOG_ACL		0x02
#define HCI_LOG_SCO		0x03
#define HCI_LOG_EVT		0x04

#define HCI_LOG_NUM_COMPELET_EVENT          0x13

enum {
	HCI_LOG_DEBUG_CMD			= (0x01 << 0),
	HCI_LOG_DEBUG_EVENT			= (0x01 << 1),
	HCI_LOG_DEBUG_RX_ACL		= (0x01 << 2),
	HCI_LOG_DEBUG_TX_ACL		= (0x01 << 3),
	HCI_LOG_DEBUG_RX_MEDIA		= (0x01 << 4),
	HCI_LOG_DEBUG_TX_MEDIA		= (0x01 << 5),
	HCI_LOG_DEBUG_RX_SCO		= (0x01 << 6),
	HCI_LOG_DEBUG_TX_SCO		= (0x01 << 7),
	HCI_LOG_DEBUG_SNOOP			= (0x01 << 8),
};

#define HCI_LOG_DEBUG_INIT		(HCI_LOG_DEBUG_CMD | \
								HCI_LOG_DEBUG_EVENT | \
								HCI_LOG_DEBUG_RX_ACL | \
								HCI_LOG_DEBUG_TX_ACL)

#if CONFIG_BT_SNOOP
extern int btsnoop_init(void);
extern int btsnoop_write_packet(uint8_t type, const uint8_t *packet, bool is_received);
#endif

#if CONFIG_BT_A2DP
extern bool bt_a2dp_is_media_rx_channel(uint16_t handle, uint16_t cid);
extern bool bt_a2dp_is_media_tx_channel(uint16_t handle, uint16_t cid);
#else
#define bt_a2dp_is_media_rx_channel(a, b)	false
#define bt_a2dp_is_media_tx_channel(a, b)	false
#endif

#define CONFIG_BT_PRINT_MAX_LEN 50

static uint16_t hci_log_flag;

static bool check_acl_data_print(bool send, uint8_t *data)
{
	uint16_t handle, cid;
	uint8_t continue_frag;
	uint8_t tws_protocol;
	bool print_media;

	if ((send && !(hci_log_flag & HCI_LOG_DEBUG_TX_ACL)) ||
		(!send && !(hci_log_flag & HCI_LOG_DEBUG_RX_ACL))) {
		return false;
	}

	if (send && (hci_log_flag & HCI_LOG_DEBUG_TX_MEDIA)) {
		return true;
	} else if (!send && (hci_log_flag & HCI_LOG_DEBUG_RX_MEDIA)) {
		return true;
	} else {
		handle = data[0] | ((data[1]&0x0F) << 8);
		cid = data[6] | (data[7] << 8);
		tws_protocol = (data[8] == 0xEE)? 1 : 0;
		continue_frag = ((data[1]&0x30) == 0x10)? 1 : 0;
		print_media = send ? bt_a2dp_is_media_tx_channel(handle, cid) : bt_a2dp_is_media_rx_channel(handle, cid);

		if (continue_frag) {
			return false;
		} else if (print_media && !tws_protocol) {
			return false;
		} else {
			return true;
		}
	}
}

void hci_data_log_debug(bool send, struct net_buf *buf)
{
	char *prefix;
	uint8_t in_type = bt_buf_get_type(buf);
	uint8_t type = 0xFF, log_print = 0;

	if (send) {
		switch (in_type) {
		case BT_BUF_CMD:
			type = HCI_LOG_CMD;
			prefix = "TX: 01";
			break;
		case BT_BUF_ACL_OUT:
			type = HCI_LOG_ACL;
			prefix = "TX: 02";
			break;
		case BT_BUF_SCO_OUT:
			type = HCI_LOG_SCO;
			prefix = "TX: 03";
			break;
		default:
			LOG_WRN("Unknow TX type %d\n", in_type);
			return;
		}
	} else {
		switch (in_type) {
		case BT_BUF_ACL_IN:
			type = HCI_LOG_ACL;
			prefix = "RX: 02";
			break;
		case BT_BUF_SCO_IN:
			type = HCI_LOG_SCO;
			prefix = "RX: 03";
			break;
		case BT_BUF_EVT:
			type = HCI_LOG_EVT;
			prefix = "RX: 04";
			break;
		default:
			LOG_WRN("Unknow RX type %d\n", in_type);
			return;
		}
	}

	switch (type) {
	case HCI_LOG_CMD:
		if (hci_log_flag & HCI_LOG_DEBUG_CMD) {
			if ((buf->data[0] > 0x80 && buf->data[1] == 0xFC) ||
				(buf->data[0] == 0x03 && buf->data[1] == 0x14) ||
				(buf->data[0] == 0x05 && buf->data[1] == 0x14)) {
			} else {
				log_print = 1;
			}
		}
		break;
	case HCI_LOG_ACL:
		if (check_acl_data_print(send, buf->data)) {
			log_print = 1;
		}
		break;
	case HCI_LOG_SCO:
		if (send && (hci_log_flag & HCI_LOG_DEBUG_TX_SCO)) {
			log_print = 1;
		} else if (!send && (hci_log_flag & HCI_LOG_DEBUG_RX_SCO)) {
			log_print = 1;
		}
		break;
	case HCI_LOG_EVT:
		if ((hci_log_flag & HCI_LOG_DEBUG_EVENT) &&
			(buf->data[0] != HCI_LOG_NUM_COMPELET_EVENT)) {
			if ((buf->data[0] == 0x0e && buf->data[3] > 0x80 && buf->data[4] == 0xFC) ||
				(buf->data[0] == 0x0e && buf->data[3] == 0x03 && buf->data[4] == 0x14) ||
				(buf->data[0] == 0x0e && buf->data[3] == 0x05 && buf->data[4] == 0x14)) {
			} else {
				log_print = 1;
			}
		}
		break;
	default:
		break;
	}

	if (log_print) {
		stack_print_hex(prefix, buf->data, MIN(buf->len, CONFIG_BT_PRINT_MAX_LEN));
#if CONFIG_BT_SNOOP
		if (hci_log_flag & HCI_LOG_DEBUG_SNOOP) {
			btsnoop_write_packet(type, buf->data, !send);
		}
#endif
	}
}

void hci_data_log_init(void)
{
	hci_log_flag = HCI_LOG_DEBUG_INIT;

#if CONFIG_BT_SNOOP
	btsnoop_init();
#endif
}

static int cmd_log_level(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 2) {
		LOG_INF("Current level 0x%x\n", hci_log_flag);
		return 0;
	}

	hci_log_flag = strtoul(argv[1], NULL, 16);
	LOG_INF("Set level 0x%x\n", hci_log_flag);
	return 0;
}

#if CONFIG_BT_SNOOP
extern void hci_snoop_dump(void);
extern void hci_snoop_reinit(void);
extern void hci_snoop_info(void);
extern void hci_snoop_close(void);

static int snoop_cmd_dump(const struct shell *shell, size_t argc, char *argv[])
{
	hci_snoop_dump();
	return 0;
}

static int snoop_cmd_reinit(const struct shell *shell, size_t argc, char *argv[])
{
	hci_snoop_reinit();
	return 0;
}

static int snoop_cmd_info(const struct shell *shell, size_t argc, char *argv[])
{
	hci_snoop_info();
	return 0;
}

static int snoop_cmd_close(const struct shell *shell, size_t argc, char *argv[])
{
	hci_snoop_close();
	return 0;
}
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(hci_log_cmds,
	SHELL_CMD(level, NULL, "set debug log", cmd_log_level),
#if CONFIG_BT_SNOOP
	SHELL_CMD(dump, NULL, "dump snoop data", snoop_cmd_dump),
	SHELL_CMD(reinit, NULL, "Reinitialize capture snoop", snoop_cmd_reinit),
	SHELL_CMD(info, NULL, "Snoop information", snoop_cmd_info),
	SHELL_CMD(close, NULL, "Close capture snoop file", snoop_cmd_close),
#endif
	SHELL_SUBCMD_SET_END
);

static int cmd_hci_log(const struct shell *shell, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(shell);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(shell, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

SHELL_CMD_REGISTER(hci_log, &hci_log_cmds, "Bluetooth hci log debug commands", cmd_hci_log);

#else

void hci_data_log_debug(bool send, struct net_buf *buf) {}
void hci_data_log_init(void) {}

#endif
