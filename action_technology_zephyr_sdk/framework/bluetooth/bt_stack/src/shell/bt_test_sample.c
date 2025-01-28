/** @file
 * @brief Bluetooth Controller and flash co-operation
 *
 */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME bt_ts
#include <logging/log.h>
LOG_MODULE_DECLARE(os, LOG_LEVEL_INF);

#include <zephyr.h>
#include <shell/shell.h>
#include <sys/printk.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#if CONFIG_PROPERTY
#define TS_USED_TEST_SAVE_KEY	1
#include <property_manager.h>
#endif

#include <acts_bluetooth/host_interface.h>
#include <drivers/ipmsg.h>
#include <drivers/bluetooth/bt_drv.h>


#define BT_A2DP_MAX_ENDPOINT	5	/* 1 source sbc, 2 sink sbc, 2 sink aac */

#define BT_CLASS_OF_DEVICE		0x240404		/* Rendering,Audio, Audio/Video, Wearable Headset Device */
#define MAX_BR_LINK_KEY			3
#define LINK_KEY_LEN			16
#define LINK_KEY_MAC			"link_mac"
#define LINK_KEY_STRING			"link_key"
#define SPP_MAX_CHANNEL			2

enum {
	BT_NO_SCANS_ENABLE = 0,
	BT_INQUIRY_EANBEL_PAGE_DISABLE = 1,
	BT_INQUIRY_DISABLE_PAGE_ENABLE = 2,
	BT_INQUIRY_ENABLE_PAGE_ENABLE = 3,
};

struct store_keys_link_key {
	bt_addr_t		addr;
	uint8_t			val[16];
};

struct br_dev_manager {
	struct bt_conn *brle_conn;
	struct bt_conn *sco_conn;
	uint8_t type;
	uint8_t hf_connected:1;
	uint8_t ag_connected:1;
	uint8_t a2dp_conneted:1;
	uint8_t avrcp_conneted:1;
	uint8_t hid_conneted:1;
	uint8_t hid_unplug:1;
	uint8_t hid_mode:1;
	uint16_t pbap_user_id;
	uint8_t spp_id[SPP_MAX_CHANNEL];
};

static const uint8_t a2dp_sbc_codec[] = {
	BT_A2DP_AUDIO << 4,
	BT_A2DP_SBC,
	0xFF,	/* (SNK mandatory)44100, 48000, mono, dual channel, stereo, join stereo */
			/* (SNK optional) 16000, 32000 */
	0xFF,	/* (SNK mandatory) Block length: 4/8/12/16, subbands:4/8, Allocation Method: SNR, Londness */
	0x02,	/* min bitpool */
	0x35	/* max bitpool */
};

/* Set by app */
static uint8_t reset_a2dp_sbc_codec[] = {
	BT_A2DP_AUDIO << 4,
	BT_A2DP_SBC,
	0x21,	/* 44100, join stereo */
	0x15,	/*  Block length: 16, subbands:8, Allocation Method: Londness */
	0x02,	/* min bitpool */
	0x35	/* max bitpool */
};

static const uint8_t a2dp_aac_codec[] = {
	BT_A2DP_AUDIO << 4,
	BT_A2DP_MPEG2,
	0xF0,	/* MPEG2 AAC LC, MPEG4 AAC LC, MPEG AAC LTP, MPEG4 AAC Scalable */
	0x01,	/* Sampling Frequecy 44100 */
	0x8F,	/* Sampling Frequecy 48000, channels 1, channels 2 */
	0xFF,	/* VBR, bit rate */
	0xFF,	/* bit rate */
	0xFF	/* bit rate */
};

extern uint8_t *bt_test_get_sco_data(uint16_t *len);
extern uint8_t *bt_test_get_sbc_data(uint16_t *len);

static uint8_t bt_init_ready;
static uint8_t br_scan_value;
static uint8_t curr_brdev_index;
static uint8_t curr_spp_index;
static struct br_dev_manager brdev_info[CONFIG_BT_MAX_CONN];
static struct bt_a2dp_endpoint a2dp_endpoint[BT_A2DP_MAX_ENDPOINT];

static struct br_dev_manager *find_free_brdev(void)
{
	int i;

	for (i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (brdev_info[i].brle_conn == NULL) {
			return &brdev_info[i];
		}
	}

	return NULL;
}

static struct br_dev_manager *find_brdev_by_brconn(struct bt_conn *brle_conn)
{
	int i;

	for (i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (brdev_info[i].brle_conn == brle_conn) {
			return &brdev_info[i];
		}
	}

	return NULL;
}

static struct br_dev_manager *find_brdev_by_scoconn(struct bt_conn *sco_conn)
{
	int i;

	for (i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (brdev_info[i].sco_conn == sco_conn) {
			return &brdev_info[i];
		}
	}

	return NULL;
}

static int str2bt_addr(const char *str, bt_addr_t *addr)
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

		if (char2hex(*str, &tmp) < 0) {
			return -EINVAL;
		}

		addr->val[i] |= tmp;
	}

	return 0;
}

static void conn_addr_str(struct bt_conn *conn, char *addr, size_t len)
{
	struct bt_conn_info info;

	if (hostif_bt_conn_get_info(conn, &info) < 0) {
		addr[0] = '\0';
		return;
	}

	switch (info.type) {
#if defined(CONFIG_BT_BREDR)
	case BT_CONN_TYPE_BR:
	case BT_CONN_TYPE_SCO:
		hostif_bt_addr_to_str(info.br.dst, addr, len);
		break;
#endif
	case BT_CONN_TYPE_LE:
		hostif_bt_addr_le_to_str(info.le.dst, addr, len);
		break;
	}
}

static const char *connected_type[] = {
	"",
	"LE",
	"BR",
	""
	"SCO"
};

static void conn_connected(struct bt_conn *conn, uint8_t err)
{
	char *pType = NULL;
	uint8_t type;
	char addr[BT_ADDR_LE_STR_LEN];
	struct br_dev_manager *brdev;

	memset(addr, 0, sizeof(addr));
	type = hostif_bt_conn_get_type(conn);
	if (type == BT_CONN_TYPE_LE || type == BT_CONN_TYPE_ISO) {
		return;
	}

	conn_addr_str(conn, addr, sizeof(addr));

	if (err) {
		LOG_ERR("Failed to connect to %s (%u)", addr, err);
		return;
	}

	if (type <= BT_CONN_TYPE_SCO) {
		pType = (char *)connected_type[type];
	}

	LOG_INF("%s connected: %s, type %d", pType, addr, type);

	switch (type) {
	case BT_CONN_TYPE_BR:
		brdev = find_free_brdev();
		if (brdev) {
			brdev->brle_conn = hostif_bt_conn_ref(conn);
			brdev->type = BT_CONN_TYPE_BR;
		} else {
			LOG_WRN("Had connect one BR");
		}
		break;

	case BT_CONN_TYPE_SCO:
		brdev = find_brdev_by_brconn(hostif_bt_conn_get_acl_conn_by_sco(conn));
		if (!brdev->sco_conn) {
			brdev->sco_conn = hostif_bt_conn_ref(conn);
		} else {
			LOG_WRN("Had connect one BR");
		}
		break;

	default:
		LOG_WRN("Unknow type :%d", type);
		break;
	}
}

static void conn_disconnected(struct bt_conn *conn, uint8_t reason)
{
	char *pType = NULL;
	uint8_t type;
	char addr[BT_ADDR_LE_STR_LEN];
	struct br_dev_manager *brdev;

	memset(addr, 0, sizeof(addr));
	type = hostif_bt_conn_get_type(conn);
	if (type == BT_CONN_TYPE_LE || type == BT_CONN_TYPE_ISO) {
		return;
	}

	conn_addr_str(conn, addr, sizeof(addr));

	if (type <= BT_CONN_TYPE_SCO) {
		pType = (char *)connected_type[type];
	}

	LOG_INF("%s disconnected: %s (reason %u), type %d", pType, addr, reason, type);

	switch (type) {
	case BT_CONN_TYPE_BR:
		brdev = find_brdev_by_brconn(conn);
		if (brdev) {
			hostif_bt_conn_unref(brdev->brle_conn);
			brdev->brle_conn = NULL;
		} else {
			LOG_WRN("Unkown conn %p %p", brdev, conn);
		}
		break;

	case BT_CONN_TYPE_SCO:
		brdev = find_brdev_by_scoconn(conn);
		if (brdev) {
			hostif_bt_conn_unref(brdev->sco_conn);
			brdev->sco_conn = NULL;
		} else {
			LOG_WRN("Unkown conn %p %p", brdev, conn);
		}
		break;

	default:
		LOG_WRN("Unknow type :%d", type);
		break;
	}
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (hostif_bt_conn_get_type(conn) == BT_CONN_TYPE_BR) {
		conn_addr_str(conn, addr, sizeof(addr));
		LOG_INF("Security changed: %s level %u", addr, level);
	}
}

static void conn_rx_sco_data(struct bt_conn *conn, uint8_t *data, uint8_t len, uint8_t pkg_flag)
{
	uint8_t *txdata;
	uint16_t txlen;
	static uint8_t cnt;

	/* Sco data */
	if (cnt++ > 100) {
		cnt = 0;
		printk(".");
	}

	/* Send voice back for test */
	txdata = bt_test_get_sco_data(&txlen);
	hostif_bt_conn_send_sco_data(conn, txdata, (uint8_t)txlen);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = conn_connected,
	.disconnected = conn_disconnected,
	.security_changed = security_changed,
	.rx_sco_data = conn_rx_sco_data,
};

static int cmd_connect_bredr(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_conn *conn;
	bt_addr_t addr;
	int err;

	if (argc < 2) {
		return -EINVAL;
	}

	err = str2bt_addr(argv[1], &addr);
	if (err) {
		LOG_INF("Invalid peer address (err %d)", err);
		return 0;
	}

	conn = hostif_bt_conn_create_br(&addr, BT_BR_CONN_PARAM_DEFAULT);
	if (!conn) {
		LOG_INF("Connection failed");
	} else {
		LOG_INF("Connection pending");
		/* unref connection obj in advance as app user */
		hostif_bt_conn_unref(conn);
	}

	return 0;
}

static int cmd_disconnect_bredr(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_conn *conn = brdev_info[curr_brdev_index].brle_conn;
	int err;

	if (!conn) {
		LOG_INF("Not connected");
		return 0;
	}

	hostif_bt_conn_ref(conn);
	err = hostif_bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		LOG_INF("Disconnection failed (err %d)", err);
	}
	hostif_bt_conn_unref(conn);

	return 0;
}

static void hf_connect_failed(struct bt_conn *conn)
{
	LOG_ERR("hf_connect_failed %p", conn);
}

static void hf_connected(struct bt_conn *conn)
{
	struct br_dev_manager *brdev = find_brdev_by_brconn(conn);

	if (!brdev) {
		LOG_ERR("%p, %p", brdev, conn);
		return;
	}

	LOG_INF("HF Connected!");
	brdev->hf_connected = 1;
}

static void hf_disconnected(struct bt_conn *conn)
{
	struct br_dev_manager *brdev = find_brdev_by_brconn(conn);

	if (!brdev) {
		LOG_ERR("%p, %p", brdev, conn);
		return;
	}

	LOG_INF("HF Disconnected!");
	brdev->hf_connected = 0;
}

static void service(struct bt_conn *conn, uint32_t value)
{
	static const char *service_string[2] = {
		"No Home/Roam network available.",
		"Home/Roam network available."
	};

	if (value < 2) {
		LOG_INF("%s", service_string[value]);
	} else {
		LOG_INF("Unknow service value %d", value);
	}
}

static void call(struct bt_conn *conn, uint32_t value)
{
	static const char *call_string[2] = {
		"there are no calls in progress",
		"at least one call is in progress"
	};

	if (value < 2) {
		LOG_INF("%s", call_string[value]);
	} else {
		LOG_INF("Unknow call value %d", value);
	}
}

static void call_setup(struct bt_conn *conn, uint32_t value)
{
	static const char *callsetup_string[4] = {
		"not currently in call set up",
		"an incoming call process ongoing",
		"an outgoing call set up is ongoing",
		"remote party being alerted in an outgoing call"
	};

	if (value < 4) {
		LOG_INF("%s", callsetup_string[value]);
	} else {
		LOG_INF("Unknow call_setup value %d", value);
	}
}

static void call_held(struct bt_conn *conn, uint32_t value)
{
	static const char *callhelp_string[3] = {
		"No calls held",
		"Call is placed on hold or active/held calls swapped",
		"Call on hold, no active call"
	};

	if (value < 3) {
		LOG_INF("%s", callhelp_string[value]);
	} else {
		LOG_INF("Unknow call_held value %d", value);
	}
}

static void signal(struct bt_conn *conn, uint32_t value)
{
	LOG_INF("%u", value);
}

static void roam(struct bt_conn *conn, uint32_t value)
{
	LOG_INF("Roaming is %s active", value ? "" : "not");
}

static void battery(struct bt_conn *conn, uint32_t value)
{
	LOG_INF("Battery value: %u", value);
}

static void ring_cb(struct bt_conn *conn)
{
	LOG_INF("Incoming Call...");
}

static void bsir_cb(struct bt_conn *conn, uint32_t value)
{
	LOG_INF("%s provide in-band ring tones", value ? "" : "not");
}

static void ccwa_cb(struct bt_conn *conn, uint8_t *buf, uint32_t value)
{
	LOG_INF("Call waiting phone number:%s, type:%d", buf, value);
}

static void clip_cb(struct bt_conn *conn, uint8_t *buf, uint32_t value, uint8_t *name)
{
	LOG_INF("Calling line identification phone number:%s, type:%d", buf, value);
}

static void bcs_cb(struct bt_conn *conn, uint32_t codec_id)
{
	LOG_INF("Sco codec id:%d", codec_id);
}

static void bvra_cb(struct bt_conn *conn, uint32_t active)
{
	LOG_INF("Voice recognition is %s in the AG", active ? "enabled" : "disabled");
}

static void vgs_cb(struct bt_conn *conn, uint32_t value)
{
	LOG_INF("Gain of Speaker %d", value);
}

static void vgm_cb(struct bt_conn *conn, uint32_t value)
{
	LOG_INF("Gain of Microphone %d", value);
}

static void btrh_cb(struct bt_conn *conn, uint32_t value)
{
	static const char *callsetup_string[3] = {
		"Incoming call is put on hold in the AG",
		"Held incoming call is accepted in the AG",
		"Held incoming call is rejected in the AG"
	};

	if (value < 3) {
		LOG_INF("%s", callsetup_string[value]);
	} else {
		LOG_INF("Unknow value %d", value);
	}
}

static void cops_cb(struct bt_conn *conn, uint32_t mode, uint32_t format, uint8_t *opt)
{
	LOG_INF("Cops mode %d, format: %d, operator: %s", mode, format, opt);
}

static void cnum_cb(struct bt_conn *conn, uint8_t *buf, uint32_t value)
{
	LOG_INF("Subscriber Number Information phone number:%s, type:%d", buf, value);
}

static void clcc_cb(struct bt_conn *conn, uint32_t idx, uint32_t dir, uint32_t status, uint32_t mode, uint32_t mpty)
{
	LOG_INF("list current calls result code, idx: %d, dir: %d,"
				"status: %d, mode: %d, mpty: %d", idx, dir, status, mode, mpty);
}

static struct bt_hfp_hf_cb hf_cb = {
	.connect_failed = hf_connect_failed,
	.connected = hf_connected,
	.disconnected = hf_disconnected,
	.service = service,
	.call = call,
	.call_setup = call_setup,
	.call_held = call_held,
	.signal = signal,
	.roam = roam,
	.battery = battery,
	.ring_indication = ring_cb,
	.bsir = bsir_cb,
	.ccwa = ccwa_cb,
	.clip = clip_cb,
	.bcs = bcs_cb,
	.bvra = bvra_cb,
	.vgs = vgs_cb,
	.vgm = vgm_cb,
	.btrh = btrh_cb,
	.cops = cops_cb,
	.cnum = cnum_cb,
	.clcc = clcc_cb,
};

static int cmd_hf_connect_ag(const struct shell *shell, size_t argc, char *argv[])
{
	struct br_dev_manager *brdev = &brdev_info[curr_brdev_index];

	if (!brdev->brle_conn) {
		LOG_INF("BR device not connected!");
		return -EIO;
	}

	LOG_INF("%p", brdev->brle_conn);
	return hostif_bt_hfp_hf_connect(brdev->brle_conn);
}

static int cmd_hf_disconnect_ag(const struct shell *shell, size_t argc, char *argv[])
{
	struct br_dev_manager *brdev = &brdev_info[curr_brdev_index];

	if (!brdev->hf_connected) {
		LOG_INF("hf not connected!");
		return -EIO;
	}

	LOG_INF("%p", brdev->brle_conn);
	return hostif_bt_hfp_hf_disconnect(brdev->brle_conn);
}

static int cmd_hf_batreport(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t bat_val;
	char at_command[32];
	struct br_dev_manager *brdev = &brdev_info[curr_brdev_index];

	if (!brdev->hf_connected) {
		LOG_INF("hfp not connected!");
		return -EIO;
	}

	if (argc < 2) {
		LOG_INF("Used: ts hfp-batreport x(battery value:0~9)");
		return 0;
	}

	bat_val = strtoul(argv[1], NULL, 10);
	memset(at_command, 0, sizeof(at_command));
	snprintf(at_command, sizeof(at_command), "AT+IPHONEACCEV=1,1,%u\r", bat_val);
	LOG_INF("%d", bat_val);

	return hostif_bt_hfp_hf_send_cmd(brdev->brle_conn, at_command);
}

void ag_connect_failed_cb(struct bt_conn *conn)
{
	LOG_INF("AG connect failed\n");
}

static void ag_connected_cb(struct bt_conn *conn)
{
	struct br_dev_manager *brdev = find_brdev_by_brconn(conn);

	if (!brdev) {
		LOG_ERR("%p, %p", brdev, conn);
		return;
	}

	LOG_INF("AG Connected!");
	brdev->ag_connected = 1;
}

static void ag_disconnected_cb(struct bt_conn *conn)
{
	struct br_dev_manager *brdev = find_brdev_by_brconn(conn);

	if (!brdev) {
		LOG_ERR("%p, %p", brdev, conn);
		return;
	}

	LOG_INF("AG Disconnected!");
	brdev->ag_connected = 0;
}

static int ag_at_cmd_cb(struct bt_conn *conn, uint8_t at_type, void *param)
{
	struct at_cmd_param *in_param = param;

	switch (at_type) {
	case AT_CMD_CHLD:
		LOG_INF("CHLD %d", in_param->val_u32t);
		if (in_param->val_u32t == 0) {
			if (hostif_bt_hfp_ag_get_indicator(CIEV_CALL_SETUP_IND) > 0) {
				hostif_bt_hfp_ag_update_indicator(CIEV_CALL_SETUP_IND, 0);
			}
		} else if (in_param->val_u32t == 1) {
			hostif_bt_hfp_ag_update_indicator(CIEV_CALL_IND, 0);

			if (hostif_bt_hfp_ag_get_indicator(CIEV_CALL_HELD_IND) > 0) {
				hostif_bt_hfp_ag_update_indicator(CIEV_CALL_HELD_IND, 0);
			}

			hostif_bt_hfp_ag_update_indicator(CIEV_CALL_IND, 1);

			if (hostif_bt_hfp_ag_get_indicator(CIEV_CALL_SETUP_IND) > 0) {
				hostif_bt_hfp_ag_update_indicator(CIEV_CALL_SETUP_IND, 0);
			}
		} else if (in_param->val_u32t == 2) {
			hostif_bt_hfp_ag_update_indicator(CIEV_CALL_HELD_IND, 2);
			hostif_bt_hfp_ag_update_indicator(CIEV_CALL_IND, 1);
			hostif_bt_hfp_ag_update_indicator(CIEV_CALL_SETUP_IND, 0);
			hostif_bt_hfp_ag_update_indicator(CIEV_CALL_HELD_IND, 1);
		} else if (in_param->val_u32t == 3) {
			hostif_bt_hfp_ag_update_indicator(CIEV_CALL_HELD_IND, 0);
		}
		break;
	case AT_CMD_CCWA:
		LOG_INF("CCWA %d", in_param->val_u32t);
		break;
	case AT_CMD_BCC:
		if (hostif_bt_conn_create_sco(conn)) {
			LOG_INF("Sco connection failed");
		} else {
			LOG_INF("Sco connection pending");
		}
		break;
	case AT_CMD_VGS:
		LOG_INF("VGS %d", in_param->val_u32t);
		break;
	case AT_CMD_VGM:
		LOG_INF("VGM %d", in_param->val_u32t);
		break;
	case AT_CMD_ATA:
		LOG_INF("ATA");
		break;
	case AT_CMD_CHUP:
		LOG_INF("CHUP");
		if (hostif_bt_hfp_ag_get_indicator(CIEV_CALL_IND) == 1) {
			hostif_bt_hfp_ag_update_indicator(CIEV_CALL_IND, 0);
		}

		if (hostif_bt_hfp_ag_get_indicator(CIEV_CALL_SETUP_IND) != 0) {
			hostif_bt_hfp_ag_update_indicator(CIEV_CALL_IND, 1);
			hostif_bt_hfp_ag_update_indicator(CIEV_CALL_SETUP_IND, 0);
		}
		break;
	case AT_CMD_ATD:
		LOG_INF("ATD %c, len %d, is_index %d", in_param->s_val_char[0],
					in_param->s_len_u16t, in_param->is_index);
		break;
	case AT_CMD_BLDN:
		LOG_INF("AT+BLDN");
		break;
	case AT_CMD_NREC:
		LOG_INF("NREC %d", in_param->val_u32t);
		break;
	case AT_CMD_BVRA:
		LOG_INF("BVRA %d", in_param->val_u32t);
		break;
	case AT_CMD_BINP:
		LOG_INF("BINP %d", in_param->val_u32t);
		break;
	case AT_CMD_VTS:
		LOG_INF("VTS %c", in_param->val_char);
		break;
	case AT_CMD_CNUM:
		LOG_INF("CNUM");
		hostif_bt_hfp_ag_send_event(conn, "+CNUM: ,\"1234567\",129,,4", strlen("+CNUM: ,\"1234567\",129,,4"));
		break;
	case AT_CMD_CLCC:
		LOG_INF("CLCC");
		hostif_bt_hfp_ag_send_event(conn, "+CLCC: 1,1,0,0,0", strlen("+CLCC: 1,1,0,0,0"));
		//hostif_bt_hfp_ag_send_event(conn, "+CLCC: 1,1,0,0,1", strlen("+CLCC: 1,1,0,0,1"));
		//hostif_bt_hfp_ag_send_event(conn, "+CLCC: 2,1,0,0,1", strlen("+CLCC: 2,1,0,0,1"));
		break;
	case AT_CMD_BCS:
		LOG_INF("BCS %d", in_param->val_u32t);
		break;
	case AT_CMD_BTRH:
		LOG_INF("BTRH %d", in_param->val_u32t);
		break;
	default:
		LOG_INF("at_type %d\n", at_type);
		break;
	}

	return 0;
}

static struct bt_hfp_ag_cb ag_cb = {
	.connect_failed = ag_connect_failed_cb,
	.connected = ag_connected_cb,
	.disconnected = ag_disconnected_cb,
	.ag_at_cmd = ag_at_cmd_cb,
};

static int cmd_ag_connect_hf(const struct shell *shell, size_t argc, char *argv[])
{
	struct br_dev_manager *brdev = &brdev_info[curr_brdev_index];

	if (!brdev->brle_conn) {
		LOG_INF("BR device not connected!");
		return -EIO;
	}

	LOG_INF("%p", brdev->brle_conn);
	return hostif_bt_hfp_ag_connect(brdev->brle_conn);
}

static int cmd_ag_disconnect_hf(const struct shell *shell, size_t argc, char *argv[])
{
	struct br_dev_manager *brdev = &brdev_info[curr_brdev_index];

	if (!brdev->ag_connected) {
		LOG_INF("ag not connected!");
		return -EIO;
	}

	LOG_INF("%p", brdev->brle_conn);
	return hostif_bt_hfp_ag_disconnect(brdev->brle_conn);
}

static int cmd_sco_connect(const struct shell *shell, size_t argc, char *argv[])
{
	struct br_dev_manager *brdev = &brdev_info[curr_brdev_index];

	if (!brdev->hf_connected && !brdev->ag_connected) {
		LOG_INF("hfp not connected!");
		return -EIO;
	}

	if (hostif_bt_conn_create_sco(brdev->brle_conn)) {
		LOG_INF("Sco connection failed");
	} else {
		LOG_INF("Sco connection pending");
	}

	return 0;
}

static int cmd_sco_disconnect(const struct shell *shell, size_t argc, char *argv[])
{
	int err;
	struct br_dev_manager *brdev = &brdev_info[curr_brdev_index];

	if (!brdev->sco_conn) {
		LOG_INF("Sco not connected");
	}

	hostif_bt_conn_ref(brdev->sco_conn);
	err = hostif_bt_conn_disconnect(brdev->sco_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		LOG_INF("Disconnection failed (err %d)", err);
	}

	hostif_bt_conn_unref(brdev->sco_conn);

	return err;
}

#if defined(CONFIG_BT_PTS_TEST)
extern int bt_pts_conn_creat_add_sco_cmd(struct bt_conn *brle_conn);

static int cmd_sco_add_connect(const struct shell *shell, size_t argc, char *argv[])
{
	struct br_dev_manager *brdev = &brdev_info[curr_brdev_index];

	if (!brdev->hf_connected) {
		LOG_INF("hfp not connected!");
		return -EIO;
	}

	if (bt_pts_conn_creat_add_sco_cmd(brdev->brle_conn)) {
		LOG_INF("Sco connection failed");
	} else {
		LOG_INF("Sco connection pending");
	}

	return 0;
}

static int cmd_hf_pts(const struct shell *shell, size_t argc, char *argv[])
{
	const char *cmd;
	struct br_dev_manager *brdev = &brdev_info[curr_brdev_index];

	if (!brdev->hf_connected) {
		LOG_INF("hf not connected!");
		return -EIO;
	}

	if (argc < 2) {
		LOG_INF("Used: ts hf-pts cmd xx");
		return -EINVAL;
	}

	cmd = argv[1];
	LOG_INF("hf cmd:%s", cmd);

	if (!strcmp(cmd, "ATA")) {
		return hostif_bt_hfp_hf_send_cmd(brdev->brle_conn, "ATA");
	} else if (!strcmp(cmd, "CHUP")) {
		return hostif_bt_hfp_hf_send_cmd(brdev->brle_conn, "AT+CHUP");
	} else if (!strcmp(cmd, "USERAT")) {
		if (argc < 3) {
			LOG_INF("Used: ts hf-pts USERAT xx");
			return -EINVAL;
		}

		LOG_INF("User at cmd: %s", argv[2]);

		/* AT cmd
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
		return hostif_bt_hfp_hf_send_cmd(brdev->brle_conn, argv[2]);
	}

	LOG_INF("Unkown cmd:%s\n", cmd);
	return 0;
}

#define AG_TEST_CLIP	"+CLIP: \"1234567\",129"
#define AG_TEST_CCWA	"+CCWA: \"7654321\",129"

static int cmd_ag_pts(const struct shell *shell, size_t argc, char *argv[])
{
	int i;
	unsigned long index, value;
	const char *cmd;
	struct br_dev_manager *brdev = &brdev_info[curr_brdev_index];

	//if (!brdev->ag_connected) {
	//	LOG_INF("ag not connected!");
	//	return -EIO;
	//}

	if (argc < 2) {
		LOG_INF("Used: ts ag-pts cmd xx");
		return -EINVAL;
	}

	cmd = argv[1];
	LOG_INF("ag cmd:%s", cmd);
	if (!strcmp(cmd, "CIEV")) {
		if (argc < 4) {
			LOG_INF("Used: ts ag-pts CIEV index value");
			return 0;
		}

		/* index value
		 * 0 : CIEV_SERVICE_IND
		 * 1 : CIEV_CALL_IND
		 * 2 : CIEV_CALL_SETUP_IND
		 * 3 : CIEV_CALL_HELD_IND
		 * 4 : CIEV_SINGNAL_IND
		 * 5 : CIEV_ROAM_IND
		 * 6 : CIEV_BATTERY_IND
		 */
		index = strtoul(argv[2], NULL, 16);
		value = strtoul(argv[3], NULL, 16);
		hostif_bt_hfp_ag_update_indicator((uint8_t)index, (uint8_t)value);
		hostif_bt_hfp_ag_display_indicator();
		return 0;
	} else if (!strcmp(cmd, "RING")) {
		hostif_bt_hfp_ag_send_event(brdev->brle_conn, "RING", strlen("RING"));
		return hostif_bt_hfp_ag_send_event(brdev->brle_conn, AG_TEST_CLIP, strlen(AG_TEST_CLIP));
	} else if (!strcmp(cmd, "CCWA")) {
		return hostif_bt_hfp_ag_send_event(brdev->brle_conn, AG_TEST_CCWA, strlen(AG_TEST_CCWA));
	} else if (!strcmp(cmd, "USER")) {
		if (argc < 3) {
			LOG_INF("Used: ts ag-pts USER xx");
			return -EINVAL;
		}

		for (i = 0; i < strlen(argv[2]); i++) {
			/* Shell can't input  space, replace by '^'  */
			if (argv[2][i] == '^') {
				argv[2][i] = ' ';
			}
		}
		return hostif_bt_hfp_ag_send_event(brdev->brle_conn, argv[2], strlen(argv[2]));
	}

	LOG_INF("Unkown cmd:%s\n", cmd);
	return 0;
}
#endif

static int cmd_bredr_a2dp_register(const struct shell *shell, size_t argc, char *argv[])
{
	int ret;

	a2dp_endpoint[0].info.codec = (struct bt_a2dp_media_codec *)&a2dp_sbc_codec;
	ret = hostif_bt_a2dp_register_endpoint(&a2dp_endpoint[0], BT_A2DP_AUDIO, BT_A2DP_EP_SINK);

	a2dp_endpoint[1].info.codec = (struct bt_a2dp_media_codec *)&a2dp_sbc_codec;
	ret |= hostif_bt_a2dp_register_endpoint(&a2dp_endpoint[1], BT_A2DP_AUDIO, BT_A2DP_EP_SINK);

	a2dp_endpoint[2].info.codec = (struct bt_a2dp_media_codec *)&a2dp_sbc_codec;
	ret |= hostif_bt_a2dp_register_endpoint(&a2dp_endpoint[2], BT_A2DP_AUDIO, BT_A2DP_EP_SOURCE);

	a2dp_endpoint[3].info.codec = (struct bt_a2dp_media_codec *)&a2dp_aac_codec;
	ret = hostif_bt_a2dp_register_endpoint(&a2dp_endpoint[3], BT_A2DP_AUDIO, BT_A2DP_EP_SINK);

	a2dp_endpoint[4].info.codec = (struct bt_a2dp_media_codec *)&a2dp_aac_codec;
	ret |= hostif_bt_a2dp_register_endpoint(&a2dp_endpoint[4], BT_A2DP_AUDIO, BT_A2DP_EP_SINK);

	return ret;
}

/* #define TEST_A2DP_SOURCE_TO_SINK	1 */

static const int32_t test_a2dp_audio_interval = 10;
static uint8_t send_audio_start;
static struct k_delayed_work bt_a2dp_send_audio_work;

static void bredr_a2dp_send_audio(struct k_work *work)
{
	uint8_t *data;
	uint16_t len;
	struct br_dev_manager *brdev = &brdev_info[curr_brdev_index];

	if (!brdev->a2dp_conneted) {
		LOG_INF("a2dp not connected!");
		return;
	}

	data = bt_test_get_sbc_data(&len);
	hostif_bt_a2dp_send_audio_data(brdev->brle_conn, data, len);

	if (send_audio_start) {
		k_delayed_work_submit(&bt_a2dp_send_audio_work, K_MSEC(test_a2dp_audio_interval));
	}
}

static int cmd_bredr_a2dp_start(const struct shell *shell, size_t argc, char *argv[])
{
	struct br_dev_manager *brdev = &brdev_info[curr_brdev_index];

	if (!brdev->a2dp_conneted) {
		LOG_INF("a2dp not connected!");
		return -EIO;
	}

	hostif_bt_a2dp_start(brdev->brle_conn);

	send_audio_start = 1;
	k_delayed_work_init(&bt_a2dp_send_audio_work, bredr_a2dp_send_audio);
	k_delayed_work_submit(&bt_a2dp_send_audio_work, K_MSEC(test_a2dp_audio_interval));
	return 0;
}

static int cmd_bredr_a2dp_suspend(const struct shell *shell, size_t argc, char *argv[])
{
	struct br_dev_manager *brdev = &brdev_info[curr_brdev_index];

	if (!brdev->a2dp_conneted) {
		LOG_INF("a2dp not connected!");
		return -EIO;
	}

	send_audio_start = 0;
	k_delayed_work_cancel(&bt_a2dp_send_audio_work);

	hostif_bt_a2dp_suspend(brdev->brle_conn);
	return 0;
}

static int cmd_bredr_a2dp_reconfig(const struct shell *shell, size_t argc, char *argv[])
{
	struct br_dev_manager *brdev = &brdev_info[curr_brdev_index];

	if (!brdev->a2dp_conneted) {
		LOG_INF("a2dp not connected!");
		return -EIO;
	}

	return hostif_bt_a2dp_reconfig(brdev->brle_conn, (struct bt_a2dp_media_codec *)&reset_a2dp_sbc_codec);
}

static int cmd_bredr_switch_role(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_conn *conn = brdev_info[curr_brdev_index].brle_conn;

	LOG_INF("Switch to salve role");
	if (hostif_bt_conn_switch_role(conn, BT_CONN_ROLE_SLAVE)) {
		LOG_INF("hostif_bt_conn_switch_role failed");
	}

	return 0;
}

static int cmd_bredr_a2dp_connect(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t role = BT_A2DP_CH_SINK;
	struct bt_conn *conn = brdev_info[curr_brdev_index].brle_conn;

	if (!conn) {
		LOG_INF("br not connect!!!");
		return -1;
	}

	if (argc > 1) {
		if (!strcmp(argv[1], "source")) {
			role = BT_A2DP_CH_SOURCE;
		}
	}

	if (hostif_bt_a2dp_connect(conn, role) == 0) {
		LOG_INF("Connect a2dp_conn success!");
	} else {
		LOG_INF("cmd_bredr_a2dp_connect failed");
		return -1;
	}

	return 0;
}

static int cmd_bredr_a2dp_disconnect(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_conn *conn = brdev_info[curr_brdev_index].brle_conn;

	hostif_bt_a2dp_disconnect(conn);
	return 0;
}

static void a2dp_connect_cb(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];
	struct br_dev_manager *brdev;

	brdev = find_brdev_by_brconn(conn);
	if (!brdev) {
		LOG_ERR("%p, %p", brdev, conn);
		return;
	}

	conn_addr_str(conn, addr, sizeof(addr));
	LOG_INF("%s: mac:%s", __func__, addr);
	brdev->a2dp_conneted = 1;
}

static void a2dp_disconnected_cb(struct bt_conn *conn)
{
	struct br_dev_manager *brdev;

	brdev = find_brdev_by_brconn(conn);
	if (!brdev) {
		LOG_ERR("%p, %p", brdev, conn);
		return;
	}

	LOG_INF("%s", __func__);
	brdev->a2dp_conneted = 0;
}

static void a2dp_audio_source_to_sink(struct bt_conn *rx_conn, uint8_t *data, uint16_t len)
{
#ifdef TEST_A2DP_SOURCE_TO_SINK
	struct br_dev_manager *brdev = &brdev_info[curr_brdev_index];

	if (!brdev || !brdev->a2dp_conneted || rx_conn == brdev->brle_conn) {
		return;
	}

	hostif_bt_a2dp_send_audio_data(brdev->brle_conn, data, len);
#endif
}

static void a2dp_media_handler_cb(struct bt_conn *conn, uint8_t *data, uint16_t len)
{
	static uint8_t cnt;

	a2dp_audio_source_to_sink(conn, data, len);
	/* A2dp media data */
	if (cnt++ > 100) {
		cnt = 0;
		printk(".");
	}
}

static void a2dp_state_source_to_sink(struct bt_conn *rx_conn, uint8_t state)
{
#ifdef TEST_A2DP_SOURCE_TO_SINK
	struct br_dev_manager *brdev = &brdev_info[curr_brdev_index];

	if (!brdev || !brdev->a2dp_conneted || rx_conn == brdev->brle_conn) {
		return;
	}

	if (state == BT_A2DP_MEDIA_STATE_START) {
		hostif_bt_a2dp_start(brdev->brle_conn);
	} else if (state == BT_A2DP_MEDIA_STATE_SUSPEND) {
		hostif_bt_a2dp_suspend(brdev->brle_conn);
	}
#endif
}

static int a2dp_media_state_req_cb(struct bt_conn *conn, uint8_t state)
{
	LOG_INF("%s", __func__);
	switch (state) {
	case BT_A2DP_MEDIA_STATE_OPEN:
		LOG_INF("BT_A2DP_MEDIA_STATE_OPEN");
		break;
	case BT_A2DP_MEDIA_STATE_START:
		LOG_INF("BT_A2DP_MEDIA_STATE_START");
		break;
	case BT_A2DP_MEDIA_STATE_CLOSE:
		LOG_INF("BT_A2DP_MEDIA_STATE_CLOSE");
		break;
	case BT_A2DP_MEDIA_STATE_SUSPEND:
		LOG_INF("BT_A2DP_MEDIA_STATE_SUSPEND");
		break;
	default:
		break;
	}

	a2dp_state_source_to_sink(conn, state);
	return 0;
}

static void a2dp_seted_codec_cb(struct bt_conn *conn, struct bt_a2dp_media_codec *codec, uint8_t cp_type)
{
	LOG_INF("media_type:%d, codec_type:%d, cp_type %d", codec->head.media_type, codec->head.codec_type, cp_type);
}

static struct bt_a2dp_app_cb a2dp_app_cb = {
	.connected = a2dp_connect_cb,
	.disconnected = a2dp_disconnected_cb,
	.media_handler = a2dp_media_handler_cb,
	.media_state_req = a2dp_media_state_req_cb,
	.seted_codec = a2dp_seted_codec_cb,
};


#if defined(CONFIG_BT_PTS_TEST)
static int cmd_bredr_a2dp_pts(const struct shell *shell, size_t argc, char *argv[])
{
	const char *action;
	struct bt_conn *conn = brdev_info[curr_brdev_index].brle_conn;

	if (argc < 2) {
		LOG_INF("Used: bt br-a2dp-pts discover/getcap/getallcap/xxx");
		return 0;
	}

	action = argv[1];
	LOG_INF("A2dp cmd:%s", action);

	if (!strcmp(action, "discover")) {
		return bt_pts_a2dp_discover(conn);
	} else if (!strcmp(action, "getcap")) {
		return bt_pts_a2dp_get_capabilities(conn);
	} else if (!strcmp(action, "getallcap")) {
		return bt_pts_a2dp_get_all_capabilities(conn);
	} else if (!strcmp(action, "setcfg")) {
		return bt_pts_a2dp_set_configuration(conn);
	} else if (!strcmp(action, "reconfig")) {
		return hostif_bt_a2dp_reconfig(conn, (struct bt_a2dp_media_codec *)&reset_a2dp_sbc_codec);
	} else if (!strcmp(action, "open")) {
		return bt_pts_a2dp_open(conn);
	} else if (!strcmp(action, "start")) {
		return hostif_bt_a2dp_start(conn);
	} else if (!strcmp(action, "suspend")) {
		if (send_audio_start) {
			send_audio_start = 0;
			k_delayed_work_cancel(&bt_a2dp_send_audio_work);
		}
		return hostif_bt_a2dp_suspend(conn);
	} else if (!strcmp(action, "close")) {
		if (send_audio_start) {
			send_audio_start = 0;
			k_delayed_work_cancel(&bt_a2dp_send_audio_work);
		}
		return bt_pts_a2dp_close(conn);
	} else if (!strcmp(action, "abort")) {
		return bt_pts_a2dp_abort(conn);
	} else if (!strcmp(action, "report")) {
		return hostif_bt_a2dp_send_delay_report(conn, 0);
	} else if (!strcmp(action, "disconnectmedia")) {
		return bt_pts_a2dp_disconnect_media_session(conn);
	} else if (!strcmp(action, "connectmedia")) {
		return hostif_bt_a2dp_connect(conn, BT_A2DP_CH_MEDIA);
	} else if (!strcmp(action, "senddata")) {
		send_audio_start = 1;
		k_delayed_work_init(&bt_a2dp_send_audio_work, bredr_a2dp_send_audio);
		k_delayed_work_submit(&bt_a2dp_send_audio_work, K_MSEC(test_a2dp_audio_interval));
	} else {
		LOG_INF("Unkown cmd:%s", action);
	}

	return 0;
}
#endif

static void avrcp_ctrl_connected_cb(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];
	struct br_dev_manager *brdev;

	brdev = find_brdev_by_brconn(conn);
	if (!brdev) {
		LOG_ERR("%p, %p", brdev, conn);
		return;
	}

	conn_addr_str(conn, addr, sizeof(addr));
	LOG_INF("Avrcp connected: %p, mac:%s", conn, addr);
	brdev->avrcp_conneted = 1;
}

static void avrcp_ctrl_disconnected_cb(struct bt_conn *conn)
{
	struct br_dev_manager *brdev;

	brdev = find_brdev_by_brconn(conn);
	if (!brdev) {
		LOG_ERR("%p, %p", brdev, conn);
		return;
	}

	LOG_INF("Avrcp disconnected:%p", conn);
	brdev->avrcp_conneted = 0;
}

static void avrcp_ctrl_event_notify_cb(struct bt_conn *conn,
								uint8_t event_id, uint8_t status)
{
	/* event_id, status defined in avrcp.h */
	LOG_INF("Notify event:%d, status:%d", event_id, status);
}

static void avrcp_ctrl_pass_through_ctrl_cb(struct bt_conn *conn,
						uint8_t op_id, uint8_t state)
{
	LOG_INF("Pass through op_id:0x%x, %s", op_id, (state) ? "released" : "pushed");
}

static uint8_t avrcp_play_state;

static void avrcp_ctrl_get_play_status_cb(struct bt_conn *conn, uint8_t cmd,
						uint32_t *song_len, uint32_t *song_pos, uint8_t *play_state)
{
	if (cmd == 0) {
		return;
	}

	LOG_INF("get play status");

	if (song_len) {
		*song_len = 0x12345678;
	}

	if (song_pos) {
		*song_pos = 0x11223344;
	}

	if (play_state) {
		*play_state = avrcp_play_state;
	}
}

static struct bt_avrcp_app_cb avrcp_ctrl_cb = {
	.connected = avrcp_ctrl_connected_cb,
	.disconnected = avrcp_ctrl_disconnected_cb,
	.notify = avrcp_ctrl_event_notify_cb,
	.pass_ctrl = avrcp_ctrl_pass_through_ctrl_cb,
	.get_play_status = avrcp_ctrl_get_play_status_cb,
};

static int cmd_bredr_avrcp_connect(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_conn *conn = brdev_info[curr_brdev_index].brle_conn;

	if (!conn) {
		LOG_INF("br not connect!!!");
		return -1;
	}

	if (hostif_bt_avrcp_cttg_connect(conn) == 0) {
		LOG_INF("Connect avrcp_conn success");
	} else {
		LOG_INF("cmd_bredr_avrcp_connect failed");
		return -1;
	}

	return 0;
}

static int cmd_bredr_avrcp_disconnect(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_conn *conn = brdev_info[curr_brdev_index].brle_conn;

	if (conn) {
		hostif_bt_avrcp_cttg_disconnect(conn);
	}

	return 0;
}

static int cmd_bredr_avrcp_pass(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t opid;
	struct bt_conn *conn = brdev_info[curr_brdev_index].brle_conn;

	if (argc < 2) {
		LOG_INF("Used: bt br-avrcp-pass 0x44(AVRCP_OPERATION_ID_PLAY)");
		return 0;
	}

	opid = strtoul(argv[1], NULL, 16);

	LOG_INF("Pass through operation id:0x%x", opid);
	hostif_bt_avrcp_ct_pass_through_cmd(conn, opid, true);
	k_sleep(K_MSEC(5));
	hostif_bt_avrcp_ct_pass_through_cmd(conn, opid, false);
	return 0;
}

static int cmd_bredr_avrcp_pass_continue(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t opid;
	struct bt_conn *conn = brdev_info[curr_brdev_index].brle_conn;

	if (argc < 3) {
		LOG_INF("Used: bt br-avrcp-pass 0x49(AVRCP_OPERATION_ID_PLAY) start/stop");
		return 0;
	}

	opid = strtoul(argv[1], NULL, 16);

	LOG_INF("Pass through operation id:0x%x", opid);
	return hostif_bt_avrcp_ct_pass_through_cmd(conn, opid,
					(memcmp(argv[2], "start", 5) == 0) ? true : false);
}

static int cmd_bred_arvcp_notify_change(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_conn *conn = brdev_info[curr_brdev_index].brle_conn;

#if 0
	avrcp_play_state++;
	if (avrcp_play_state > 4) {
		/* 0: Stoped, 1: playing, 2: paused, 3: fwd_seek, 4: rev_seek */
		avrcp_play_state = 0;
	}

	LOG_INF("Target notify controller state change: %d", avrcp_play_state);
#endif
	return hostif_bt_avrcp_tg_notify_change(conn, 0x55);
}

void spp_connected_cb(struct bt_conn *conn, uint8_t spp_id)
{
	struct br_dev_manager *brdev;

	LOG_INF("conn:%p, spp_id: %d", conn, spp_id);

	brdev = find_brdev_by_brconn(conn);
	if (!brdev) {
		LOG_ERR("Can't find btdrv for conn:%p", conn);
		return;
	}

	if (brdev->spp_id[0] == spp_id || brdev->spp_id[1] == spp_id) {
		LOG_INF("Acive connect\n");
	} else if (brdev->spp_id[0] == 0) {
		brdev->spp_id[0] = spp_id;
		LOG_INF("Passive connect spp_id %d\n", spp_id);
	} else if (brdev->spp_id[1] == 0) {
		brdev->spp_id[1] = spp_id;
		LOG_INF("Passive connect spp_id %d\n", spp_id);
	} else {
		LOG_ERR("Can't find free spp channel");
	}
}

void spp_disconnected_cb(struct bt_conn *conn, uint8_t spp_id)
{
	struct br_dev_manager *brdev;

	LOG_INF("conn:%p, spp_id: %d", conn, spp_id);

	/* Wait TODO: When acl disconnect call spp disconnect,
	 * becase spp call back disconnect in thread, spp disconnect event after acl disconnect
	 * at this case, conn had released.
	 */
	brdev = find_brdev_by_brconn(conn);
	if (!brdev) {
		LOG_ERR("Can't find btdrv for conn:%p", conn);
		return;
	}

	if (brdev->spp_id[0] == spp_id) {
		brdev->spp_id[0] = 0;
	} else if (brdev->spp_id[1] == spp_id) {
		brdev->spp_id[1] = 0;
	} else {
		LOG_ERR("Can't find spp spp_id");
	}
}

void spp_recv_cb(struct bt_conn *conn, uint8_t spp_id, uint8_t *data, uint16_t len)
{
	LOG_INF("spp_id %d, data len:%d", spp_id, len);
}

static struct bt_spp_app_cb spp_app_cb = {
	.connect_failed = spp_disconnected_cb,
	.connected = spp_connected_cb,
	.disconnected = spp_disconnected_cb,
	.recv = spp_recv_cb,
};

static const uint8_t test_ota_spp_uuid[16] = {0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
	0x00, 0x10, 0x00, 0x00, 0x01, 0x11, 0x00, 0x00};

static const uint8_t test_speed_spp_uuid[16] = {0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
	0x00, 0x10, 0x00, 0x00, 0x66, 0x66, 0x00, 0x00};

static int cmd_spp_connect(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t spp_id, i;
	uint8_t connect_uuid[16];
	struct bt_conn *conn = brdev_info[curr_brdev_index].brle_conn;

	if (!conn) {
		LOG_INF("br not connect!!!");
		return -1;
	}

	for (i = 0; i < 16; i++) {
		connect_uuid[i] = test_speed_spp_uuid[15 - i];
	}

	spp_id = hostif_bt_spp_connect(conn, connect_uuid);
	if (spp_id) {
		brdev_info[curr_brdev_index].spp_id[curr_spp_index] = spp_id;
		LOG_INF("Start connect spp spp_id %d\n", spp_id);
	} else {
		LOG_INF("failed to start spp connect");
		return -1;
	}

	return 0;
}

static int cmd_spp_disconnect(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_conn *conn = brdev_info[curr_brdev_index].brle_conn;
	uint8_t spp_id = brdev_info[curr_brdev_index].spp_id[curr_spp_index];

	if (!conn || !spp_id) {
		LOG_INF("br(%p) or spp(%d) not connect!!!", conn, spp_id);
		return -1;
	}

	return hostif_bt_spp_disconnect(spp_id);
}

static int cmd_spp_send(const struct shell *shell, size_t argc, char *argv[])
{
	static uint8_t data_value;
	uint8_t send_buf[10], i, j;
	struct bt_conn *conn = brdev_info[curr_brdev_index].brle_conn;
	uint8_t spp_id = brdev_info[curr_brdev_index].spp_id[curr_spp_index];
	int ret, prio;

	if (!conn || !spp_id) {
		LOG_INF("br(%p) or spp(%d) not connect!!!", conn, spp_id);
		return -1;
	}

	/* Can't continue send data in negative thread */
	prio = k_thread_priority_get(k_current_get());
	k_thread_priority_set(k_current_get(), 10);

	for (i = 0; i < 100; i++) {
		for (j = 0; j < 10; j++) {
			send_buf[j] = data_value++;
		}

		ret = hostif_bt_spp_send_data(spp_id, send_buf, 10);
		if (ret < 0) {
			LOG_INF("Failed to send ret:%d", ret);
			k_sleep(K_MSEC(50));
		}
	}

	k_thread_priority_set(k_current_get(), prio);

	return 0;
}

#if defined(CONFIG_BT_PTS_TEST)
static int cmd_bredr_avrcp_pts(const struct shell *shell, size_t argc, char *argv[])
{
	const char *action;
	struct bt_conn *conn = brdev_info[curr_brdev_index].brle_conn;

	if (argc < 2) {
		LOG_INF("Used: bt avrcp-pts getcap/notify");
		return 0;
	}

	action = argv[1];
	LOG_INF("Avrcp cmd:%s", action);

	if (!strcmp(action, "getcap")) {
		return bt_pts_avrcp_ct_get_capabilities(conn);
	} else if (!strcmp(action, "notify")) {
		return bt_pts_avrcp_ct_register_notification(conn);
	} else if (!strcmp(action, "status")) {
		return bt_avrcp_ct_get_play_status(conn);
	}

	LOG_INF("Unkown cmd:%s", action);
	return 0;
}
#endif

static const uint8_t hid_descriptor[] =
{
    0x05, 0x01, // USAGE_PAGE (Generic Desktop Controls)
    0x09, 0x02, //USAGE (Mouse)
    0xa1, 0x01, //COLLECTION (Application (mouse, keyboard))
    0x85, 0x02, //REPORT_ID (2)
    0x09, 0x01, //USAGE (Pointer)
    0xa1, 0x00, //COLLECTION (Physical (group of axes))
    0x05, 0x09, //usage page(Button)
    0x19, 0x01, //Usage Minimum
    0x29, 0x08, //Usage Maximum
    0x15, 0x00, //Logical Minimum
    0x25, 0x01, //Logical Maximum
    0x95, 0x08, //Report Count
    0x75, 0x01, //Report size
    0x81, 0x02, //input()
    0x05, 0x01, //usage page()
    0x09, 0x30, //usage()
    0x09, 0x31, //usage()
    0x09, 0x38, //usage()
    0x15, 0x81, //logical minimum
    0x25, 0x7f, //logical maximum
    0x75, 0x08, //report size
    0x95, 0x03, //report count
    0x81, 0x06, //input
    0xc0,       //END_COLLECTION
    0xc0        // END_COLLECTION
};

static struct bt_sdp_attribute hid_dev_attrs[] = {
	BT_SDP_NEW_RECORD_HANDLE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST_CONST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16_CONST(BT_SDP_HID_SVCLASS)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 13),
		BT_SDP_DATA_ELEM_LIST_CONST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST_CONST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16_CONST(BT_SDP_PROTO_L2CAP)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16_CONST(0x0011)			/* HID-CONTROL */
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
			BT_SDP_DATA_ELEM_LIST_CONST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16_CONST(0x0011)
			},
			)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_LANG_BASE_ATTR_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 9),
		BT_SDP_DATA_ELEM_LIST_CONST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			BT_SDP_ARRAY_8_CONST('n', 'e')
		},
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			BT_SDP_ARRAY_16_CONST(106)
		},
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			BT_SDP_ARRAY_16_CONST(BT_SDP_PRIMARY_LANG_BASE)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST_CONST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST_CONST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16_CONST(BT_SDP_HID_SVCLASS)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16_CONST(0x0101)		/* Version 1.1 */
			},
			)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_ADD_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 15),
		BT_SDP_DATA_ELEM_LIST_CONST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 13),
			BT_SDP_DATA_ELEM_LIST_CONST(
			{
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
				BT_SDP_DATA_ELEM_LIST_CONST(
				{
					BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
					BT_SDP_ARRAY_16_CONST(BT_SDP_PROTO_L2CAP)
				},
				{
					BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
					BT_SDP_ARRAY_16_CONST(0x0013)
				},
				)
				
			},
			{
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
				BT_SDP_DATA_ELEM_LIST_CONST(
				{
					BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
					BT_SDP_ARRAY_16_CONST(0x0011)
				},
				)
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME("HID CONTROL"),
	{
		BT_SDP_ATTR_HID_PARSER_VERSION,
		{ BT_SDP_TYPE_SIZE(BT_SDP_UINT16), BT_SDP_ARRAY_16_CONST(0x0111) }
	},
	{
		BT_SDP_ATTR_HID_DEVICE_SUBCLASS,
		{ BT_SDP_TYPE_SIZE(BT_SDP_UINT8), BT_SDP_ARRAY_16_CONST(0xc0) }
	},
	{
		BT_SDP_ATTR_HID_COUNTRY_CODE,
		{ BT_SDP_TYPE_SIZE(BT_SDP_UINT8), BT_SDP_ARRAY_16_CONST(0x21) }
	},
	{
		BT_SDP_ATTR_HID_VIRTUAL_CABLE,
		{ BT_SDP_TYPE_SIZE(BT_SDP_BOOL), BT_SDP_ARRAY_8_CONST(0x1) }
	},
	{
		BT_SDP_ATTR_HID_RECONNECT_INITIATE,
		{ BT_SDP_TYPE_SIZE(BT_SDP_BOOL), BT_SDP_ARRAY_8_CONST(0x1) }
	},
	BT_SDP_LIST(
		BT_SDP_ATTR_HID_DESCRIPTOR_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ16, sizeof(hid_descriptor) + 8),
		BT_SDP_DATA_ELEM_LIST_CONST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ16, sizeof(hid_descriptor) + 5),
			BT_SDP_DATA_ELEM_LIST_CONST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
				BT_SDP_ARRAY_8_CONST(0x22),
			},
			{
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_TEXT_STR16,sizeof(hid_descriptor)),
				hid_descriptor,
			},
			),
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_HID_LANG_ID_BASE_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST_CONST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST_CONST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16_CONST(0x409),
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16_CONST(0x100),
			},
			),
		},
		)
	),
	{
		BT_SDP_ATTR_HID_BOOT_DEVICE,
		{ BT_SDP_TYPE_SIZE(BT_SDP_BOOL), BT_SDP_ARRAY_8_CONST(0x1) }
	},
	{
		BT_SDP_ATTR_HID_SUPERVISION_TIMEOUT,
		{ BT_SDP_TYPE_SIZE(BT_SDP_UINT16), BT_SDP_ARRAY_16_CONST(1000) }
	},
	{
		BT_SDP_ATTR_HID_MAX_LATENCY,
		{ BT_SDP_TYPE_SIZE(BT_SDP_UINT16), BT_SDP_ARRAY_16_CONST(240) }
	},
	{
		BT_SDP_ATTR_HID_MIN_LATENCY,
		{ BT_SDP_TYPE_SIZE(BT_SDP_UINT16), BT_SDP_ARRAY_16_CONST(0x0) }
	},
};

static void hid_connect_cb(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];
	struct br_dev_manager *brdev;

	brdev = find_brdev_by_brconn(conn);
	if (!brdev) {
		LOG_ERR("%p, %p", brdev, conn);
		return;
	}

	conn_addr_str(conn, addr, sizeof(addr));
	LOG_INF("hid connected: %p, mac:%s", conn, addr);
	brdev->hid_conneted = 1;
	brdev->hid_mode = BT_HID_PROTOCOL_REPORT_MODE;
}

static void hid_disconnected_cb(struct bt_conn *conn)
{
	struct br_dev_manager *brdev;

	brdev = find_brdev_by_brconn(conn);
	if (!brdev) {
		LOG_ERR("%p, %p", brdev, conn);
		return;
	}

	LOG_INF("hid disconnected:%p", conn);
	brdev->hid_conneted = 0;
}

static void hid_event_handler_cb(struct bt_conn *conn, uint8_t event,uint8_t *data, uint16_t len)
{
	/* event_id, status defined in avrcp.h */
	LOG_INF("Notify event:%d, conn:%p", event, conn);
	if(event == BT_HID_EVENT_GET_REPORT){
		struct hid_report* report = (struct hid_report*)data;
		if(report->report_type != BT_HID_REP_TYPE_INPUT || report->data[0] != 1){
			hostif_bt_hid_send_rsp(conn,BT_HID_HANDSHAKE_RSP_ERR_INVALID_REP_ID);
			return;
		}

		uint8_t send_buf[6] = {0};
	    memset(send_buf, 0, sizeof(send_buf));
	    /*
	     report id=1
	     */
	    send_buf[0] = 1;
	    send_buf[1] = 0x01;
	    hostif_bt_hid_send_ctrl_data(conn, BT_HID_REP_TYPE_INPUT,send_buf,2);
	}else if(event == BT_HID_EVENT_GET_PROTOCOL){
		uint8_t mode = brdev_info[curr_brdev_index].hid_mode;
		hostif_bt_hid_send_ctrl_data(conn,BT_HID_REP_TYPE_OTHER,&mode,1);
	}else if(event == BT_HID_EVENT_SET_PROTOCOL){
		brdev_info[curr_brdev_index].hid_mode = data[0];
		LOG_INF("mode:%d", data[0]);
		hostif_bt_hid_send_rsp(conn,BT_HID_HANDSHAKE_RSP_SUCCESS);
	}else if(event == BT_HID_EVENT_SET_REPORT){
		hostif_bt_hid_send_rsp(conn,BT_HID_HANDSHAKE_RSP_ERR_INVALID_REP_ID);
	}else if (event == BT_HID_EVENT_UNPLUG){
		struct br_dev_manager *brdev = find_brdev_by_brconn(conn);
		brdev->hid_unplug = 1;
	}else{
		hostif_bt_hid_send_rsp(conn,BT_HID_HANDSHAKE_RSP_ERR_INVALID_PARAM);
	}
}

static struct bt_hid_app_cb hid_app_cb = {
	.connected = hid_connect_cb,
	.disconnected = hid_disconnected_cb,
	.event_cb = hid_event_handler_cb,
};

static int cmd_hid_connect(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_conn *conn = brdev_info[curr_brdev_index].brle_conn;

	if (!conn) {
		LOG_INF("br not connect!!!");
		return -1;
	}
	if(brdev_info[curr_brdev_index].hid_unplug == 0){
		int ret  = hostif_bt_hid_connect(conn);
		if (ret) {
			LOG_INF("failed to start hid connect");
			return -1;
		}
	}

	return 0;
}

static int cmd_hid_disconnect(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_conn *conn = brdev_info[curr_brdev_index].brle_conn;

	if (!conn) {
		LOG_INF("br(%p) not connect!!!", conn);
		return -1;
	}

	return hostif_bt_hid_disconnect(conn);
}

static int cmd_hid_send_report(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_conn *conn = brdev_info[curr_brdev_index].brle_conn;

	if (!conn) {
		LOG_INF("br(%p) not connect!!!", conn);
		return -1;
	}

	uint8_t send_buf[5];
    memset(send_buf, 0, sizeof(send_buf));
    /*
     report id=1
     */
    send_buf[0] = 2;
    /*
     0x09, 0xea, //USAGE (volume down)
     0x09, 0xe9, //USAGE (volume up)
     0x09, 0xe2, //USAGE (mute)
     0x09, 0xcd, //USAGE (play/pause)
     0x09, 0xb6, //USAGE (scan previous track)
     0x09, 0xb5, //USAGE (scan next track)
     0x09, 0x83, //USAGE (fast forward)
     0x09, 0xb4, //USAGE (rewind)

     */
    send_buf[1] = 0x01;
    hostif_bt_hid_send_intr_data(conn, BT_HID_REP_TYPE_INPUT,send_buf,5);

	return 0;
}

static void bt_ready(int err)
{
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return;
	}

	LOG_INF("Bluetooth initialized");
	hostif_bt_conn_cb_register(&conn_callbacks);
	bt_init_ready = 1;
}

static int bt_start_init(const struct shell *shell)
{
	int err, i = 0;

	hostif_bt_init_class(BT_CLASS_OF_DEVICE);

	bt_init_ready = 0;
	err = hostif_bt_enable(bt_ready);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
	} else {
		while ((i++ < 200) && (!bt_init_ready)) {
			k_sleep(K_MSEC(10));
		}
	}

	extern void bt_le_init(const struct shell *shell);
	bt_le_init(shell);

	return 0;
}

static int cmd_init(const struct shell *shell, size_t argc, char *argv[])
{
	int ret;

	memset(brdev_info, 0, sizeof(brdev_info));

	LOG_INF("bt init");
	ret = bt_start_init(shell);
	if (ret) {
		LOG_ERR("bt init failed");
	}

	return ret;
}

static int cmd_bredr_start(const struct shell *shell, size_t argc, char *argv[])
{
	int ret;
	uint8_t spp_id;

	br_scan_value = BT_INQUIRY_ENABLE_PAGE_ENABLE;
	hostif_bt_br_write_scan_enable(br_scan_value);
	k_sleep(K_MSEC(100));

	LOG_INF("a2dp-register endpoint");
	ret = cmd_bredr_a2dp_register(shell, 0, NULL);
	if (ret) {
		LOG_ERR("a2dp-register failed");
		return ret;
	}

	hostif_bt_a2dp_register_cb(&a2dp_app_cb);
	hostif_bt_avrcp_cttg_register_cb(&avrcp_ctrl_cb);

	LOG_INF("hostif_bt_hfp_hf_register_cb");
	hostif_bt_hfp_hf_register_cb(&hf_cb);
	hostif_bt_hfp_ag_register_cb(&ag_cb);

	hostif_bt_spp_register_cb(&spp_app_cb);
	spp_id = hostif_bt_spp_register_service((uint8_t *)test_ota_spp_uuid);
	LOG_INF("spp register spp_id: %d", spp_id);
	spp_id = hostif_bt_spp_register_service((uint8_t *)test_speed_spp_uuid);
	LOG_INF("spp register spp_id: %d", spp_id);
	hostif_bt_hid_register_sdp(hid_dev_attrs,ARRAY_SIZE(hid_dev_attrs));
	LOG_INF("hostif_bt_hid_register_cb");
	hostif_bt_hid_register_cb(&hid_app_cb);
	/* For test one uuid connect by multi conn */
	//spp_id = hostif_bt_spp_register_service((uint8_t *)test_speed_spp_uuid);
	//LOG_INF("spp register spp_id: %d", spp_id);

	return ret;
}

static int cmd_bredr_discoverable(const struct shell *shell, size_t argc, char *argv[])
{
	int err;
	const char *action;

	if (argc < 2) {
		return -EINVAL;
	}

	action = argv[1];

	if (!strcmp(action, "on")) {
		if (br_scan_value == BT_NO_SCANS_ENABLE ||
			br_scan_value == BT_INQUIRY_EANBEL_PAGE_DISABLE) {
			br_scan_value = BT_INQUIRY_EANBEL_PAGE_DISABLE;
		} else {
			br_scan_value = BT_INQUIRY_ENABLE_PAGE_ENABLE;
		}
		err = hostif_bt_br_write_scan_enable(br_scan_value);
	} else if (!strcmp(action, "off")) {
		if (br_scan_value == BT_NO_SCANS_ENABLE ||
			br_scan_value == BT_INQUIRY_EANBEL_PAGE_DISABLE) {
			br_scan_value = BT_NO_SCANS_ENABLE;
		} else {
			br_scan_value = BT_INQUIRY_DISABLE_PAGE_ENABLE;
		}
		err = hostif_bt_br_write_scan_enable(br_scan_value);
	} else {
		return -EINVAL;
	}

	if (err) {
		LOG_INF("BR/EDR set/reset discoverable failed (err %d)", err);
	} else {
		LOG_INF("BR/EDR set/reset discoverable done");
	}

	return 0;
}

static int cmd_bredr_connectable(const struct shell *shell, size_t argc, char *argv[])
{
	int err;
	const char *action;

	if (argc < 2) {
		return -EINVAL;
	}

	action = argv[1];

	if (!strcmp(action, "on")) {
		if (br_scan_value == BT_NO_SCANS_ENABLE ||
			br_scan_value == BT_INQUIRY_DISABLE_PAGE_ENABLE) {
			br_scan_value = BT_INQUIRY_DISABLE_PAGE_ENABLE;
		} else {
			br_scan_value = BT_INQUIRY_ENABLE_PAGE_ENABLE;
		}
		err = hostif_bt_br_write_scan_enable(br_scan_value);
	} else if (!strcmp(action, "off")) {
		if (br_scan_value == BT_NO_SCANS_ENABLE ||
			br_scan_value == BT_INQUIRY_DISABLE_PAGE_ENABLE) {
			br_scan_value = BT_NO_SCANS_ENABLE;
		} else {
			br_scan_value = BT_INQUIRY_EANBEL_PAGE_DISABLE;
		}
		err = hostif_bt_br_write_scan_enable(br_scan_value);
	} else {
		return -EINVAL;
	}

	if (err) {
		LOG_INF("BR/EDR set/rest connectable failed (err %d)", err);
	} else {
		LOG_INF("BR/EDR set/reset connectable done");
	}

	return 0;
}

static void br_discovery_result(struct bt_br_discovery_result *result)
{
	const uint8_t max_name_len = 32;
	char br_addr[BT_ADDR_STR_LEN];
	char name[max_name_len + 1];
	uint8_t len;

	if (result == NULL) {
		LOG_INF("BR/EDR discovery finish");
		return;
	}

	hostif_bt_addr_to_str((const bt_addr_t *)&result->addr, br_addr, sizeof(br_addr));
	memset(name, 0, sizeof(name));

	len = result->len;
	len = (len < max_name_len) ? len : max_name_len;
	memcpy(name, result->name, len);

	LOG_INF("[DEVICE]: %s, RSSI %i %s", br_addr, result->rssi, name);
}

static int cmd_bredr_discovery(const struct shell *shell, size_t argc, char *argv[])
{
	const char *action;

	if (argc < 2) {
		return -EINVAL;
	}

	action = argv[1];
	if (!strcmp(action, "on")) {
		struct bt_br_discovery_param param;

		param.length = 4;
		param.num_responses = 0;
		param.limited = false;

		if (argc > 2) {
			param.length = atoi(argv[2]);
		}

		if (hostif_bt_br_discovery_start(&param, br_discovery_result) < 0) {
			LOG_ERR("Failed to start discovery");
			return 0;
		}

		LOG_INF("Discovery started");
	} else if (!strcmp(action, "off")) {
		if (hostif_bt_br_discovery_stop()) {
			LOG_ERR("Failed to stop discovery");
			return 0;
		}

		LOG_INF("Discovery stopped");
	} else {
		return -EINVAL;
	}

	return 0;
}

static int cmd_clear_linkkey(const struct shell *shell, size_t argc, char *argv[])
{
	hostif_bt_store_clear_linkkey(NULL);
	LOG_INF("clear linkkey");

	return 0;
}

static int cmd_info(const struct shell *shell, size_t argc, char *argv[])
{
	int i;
	char addr[BT_ADDR_LE_STR_LEN];

	LOG_INF("curr_brdev_index: %d, curr_spp_index: %d, scan %d", curr_brdev_index, curr_spp_index, br_scan_value);

	for (i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (brdev_info[i].brle_conn) {
			conn_addr_str(brdev_info[i].brle_conn, addr, sizeof(addr));
			LOG_INF("index[%d] type[%d] addr[%s] sco[%p] hf[%d] ag[%d] a2dp[%d] avrcp[%d], spp[%d, %d] hid[%d]",
				i, brdev_info[i].type, addr, brdev_info[i].sco_conn,
				brdev_info[i].hf_connected, brdev_info[i].ag_connected,
				brdev_info[i].a2dp_conneted, brdev_info[i].avrcp_conneted,
				brdev_info[i].spp_id[0], brdev_info[i].spp_id[1],brdev_info[i].hid_conneted);
		}
	}

	return 0;
}

static int cmd_set_device(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t index;

	if (argc < 2) {
		LOG_INF("curr_brdev_index: %d, spp index:%d", curr_brdev_index, curr_spp_index);
		return 0;
	}

	index = atoi(argv[1]);
	if (index >= CONFIG_BT_MAX_CONN) {
		LOG_INF("Invalid brdev index: %d", index);
	} else {
		curr_brdev_index = index;
	}

	if (argc > 2) {
		index = atoi(argv[2]);
		if (index >= SPP_MAX_CHANNEL) {
			LOG_INF("Invalid spp index: %d", index);
		} else {
			curr_spp_index = index;
		}
	}

	LOG_INF("Set current device index: %d, spp index:%d", curr_brdev_index, curr_spp_index);
	return 0;
}

/* For test tws irq */
static void test_tws0_isr(void)
{
	static uint8_t count;

	LOG_INF("TWS0 irq callback: %u", count++);
}

static void test_tws1_isr(void)
{
	static uint8_t count;

	LOG_INF("TWS1 irq callback: %u", count++);
}

static int cmd_tws_irq(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_conn *conn = brdev_info[curr_brdev_index].brle_conn;
	uint32_t bt_clk;
	uint16_t bt_intraslot;
	int err = 0;

	err = hostif_bt_vs_get_bt_clock(conn, &bt_clk, &bt_intraslot);
	if (err) {
		LOG_ERR("hostif_bt_vs_get_bt_clock");
		goto tws_irq_exit;
	} else {
		LOG_INF("bt_clk %d, %d", bt_clk, bt_intraslot);
	}

	btdrv_tws_irq_cb_set(BT_TWS_0, test_tws0_isr);
	btdrv_tws_irq_disable(BT_TWS_0);

	bt_clk += (320*5);		/* 500ms delay tws interrupt */
	err = hostif_bt_vs_set_tws_int_clock(conn, bt_clk, 0);
	if (err) {
		LOG_ERR("hostif_bt_vs_set_tws_int_clock");
		goto tws_irq_exit;
	} else {
		btdrv_tws_irq_enable(BT_TWS_0);
		hostif_bt_vs_enable_tws_int(1);
		LOG_INF("TWS irq bt_clk %d", bt_clk);
	}

	k_sleep(K_MSEC(1000));
	LOG_INF("Disable tws int");
	hostif_bt_vs_enable_tws_int(0);

tws_irq_exit:
	return err;
}

static int cmd_acl_flow_control(const struct shell *shell, size_t argc, char *argv[])
{
	int err;
	struct bt_conn *conn = brdev_info[curr_brdev_index].brle_conn;

	if (!strcmp(argv[1], "on")) {
		err = hostif_bt_vs_enable_acl_flow_control(conn, 0x01);
	} else if (!strcmp(argv[1], "off")) {
		err = hostif_bt_vs_enable_acl_flow_control(conn, 0x00);
	} else {
		return -EINVAL;
	}

	if (err) {
		LOG_ERR("acl flow control failed %d", err);
	}

	return err;
}

static int cmd_read_null_cnt(const struct shell *shell, size_t argc, char *argv[])
{
	int err;
	uint16_t cnt;
	struct bt_conn *conn = brdev_info[curr_brdev_index].brle_conn;

	err = hostif_bt_vs_read_add_null_cnt(conn, &cnt);

	if (err) {
		LOG_ERR("read null cnt failed %d", err);
	} else {
		LOG_INF("add null cnt: %u", cnt);
	}

	return err;
}

static int cmd_tws_sync(const struct shell *shell, size_t argc, char *argv[])
{
	int err;
	uint32_t bt_clk = 0;
	uint16_t bt_intraslot;
	uint8_t index, mode;
	struct bt_conn *conn = brdev_info[curr_brdev_index].brle_conn;

	index = strtoul(argv[1], NULL, 10);
	mode = strtoul(argv[2], NULL, 10);

	if (mode == 0) {
		err = hostif_bt_vs_get_bt_clock(conn, &bt_clk, &bt_intraslot);
		if (err) {
			return err;
		}
		LOG_INF("bt_clk %d, %d", bt_clk, bt_intraslot);
		bt_clk += 4;	/* 1.25 ms */
	} else if (mode == 1) {
		err = hostif_bt_vs_read_bt_us_cnt(&bt_clk);
		if (err) {
			return err;
		}
		LOG_INF("bt_clk %dus", bt_clk);
		bt_clk += 1250;
	}

	err = hostif_bt_vs_set_tws_sync_int_time(conn, index, mode, bt_clk, 0);

	if (err) {
		LOG_ERR("set tws sync time failed %d", err);
	}

	return err;
}

static int cmd_tws_play_time(const struct shell *shell, size_t argc, char *argv[])
{
	int err;
	uint32_t bt_clk = 0;
	uint16_t bt_intraslot;
	uint8_t index, mode, per_int;
	struct bt_conn *conn = brdev_info[curr_brdev_index].brle_conn;

	index = strtoul(argv[1], NULL, 10);
	mode = strtoul(argv[2], NULL, 10);
	per_int = strtoul(argv[3], NULL, 10);

	if (mode == 0) {
		err = hostif_bt_vs_get_bt_clock(conn, &bt_clk, &bt_intraslot);
		if (err) {
			return err;
		}
		LOG_INF("bt_clk %d, %d", bt_clk, bt_intraslot);
		bt_clk += 8;	/* 2.5ms */
	} else if (mode == 1) {
		err = hostif_bt_vs_read_bt_us_cnt(&bt_clk);
		if (err) {
			return err;
		}
		LOG_INF("bt_clk %dus", bt_clk);
		bt_clk += 2500;
	}

	err = hostif_bt_vs_set_tws_int_delay_play_time(conn, index, mode, per_int, bt_clk, 0);

	if (err) {
		LOG_ERR("set tws delay play time failed %d", err);
	}

	return err;
}

static int cmd_tws_irq_new(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_conn *conn = brdev_info[curr_brdev_index].brle_conn;
	uint32_t bt_clk, per_clk;
	uint16_t bt_intraslot;
	int err = 0;
	uint8_t index, mode;

	index = strtoul(argv[2], NULL, 10);
	mode = strtoul(argv[3], NULL, 10);

	LOG_INF("index %u, mode %u", index, mode);

	if (!strcmp(argv[1], "on")) {
		LOG_INF("Enable tws int");
	} else if (!strcmp(argv[1], "off")) {
		LOG_INF("Disable tws int");
		btdrv_tws_irq_disable(index);
		hostif_bt_vs_enable_tws_int_new(0, index);
		return 0;
	} else {
		return -EINVAL;
	}

	btdrv_tws_irq_disable(index);
	btdrv_tws_irq_cb_set(BT_TWS_0, test_tws0_isr);
	btdrv_tws_irq_cb_set(BT_TWS_1, test_tws1_isr);

	if (mode == 0) {
		err = hostif_bt_vs_get_bt_clock(conn, &bt_clk, &bt_intraslot);
		if (err) {
			return err;
		}
		LOG_INF("bt_clk %d, %d", bt_clk, bt_intraslot);
		bt_clk += (320*5);		/* 500ms delay tws interrupt */
		per_clk = (320*5);
	} else if (mode == 1) {
		err = hostif_bt_vs_read_bt_us_cnt(&bt_clk);
		if (err) {
			return err;
		}
		LOG_INF("bt_clk %dus", bt_clk);
		bt_clk += (500 * 1000);
		per_clk = (500 * 1000);
	} else {
		return -EINVAL;
	}
	btdrv_tws_set_mode(index, mode);

	err = hostif_bt_vs_set_tws_int_clock_new(conn, index, mode, bt_clk, 0, per_clk, 0);
	if (err) {
		LOG_ERR("hostif_bt_vs_set_tws_int_clock");
		return err;
	} else {
		btdrv_tws_irq_enable(index);
		hostif_bt_vs_enable_tws_int_new(1, index);
		LOG_INF("TWS irq bt_clk %d", bt_clk);
	}

	return err;
}

#define HELP_NONE "[none]"

SHELL_STATIC_SUBCMD_SET_CREATE(acts_bt_cmds,
	SHELL_CMD(info, NULL, "bluetooth connect infomation", cmd_info),
	SHELL_CMD(setdev, NULL, "Set current device", cmd_set_device),

	SHELL_CMD(init, NULL, "bluetooth initialize", cmd_init),
	SHELL_CMD(start, NULL, "bredr start", cmd_bredr_start),
	SHELL_CMD(iscan, NULL, "<value: on, off>", cmd_bredr_discoverable),
	SHELL_CMD(pscan, NULL, "value: on, off", cmd_bredr_connectable),
	SHELL_CMD(discovery, NULL, "<value: on, off> [length: 1-48] [mode: limited]", cmd_bredr_discovery),
	SHELL_CMD(connect, NULL, "<address>", cmd_connect_bredr),
	SHELL_CMD(disconnect, NULL, HELP_NONE, cmd_disconnect_bredr),
	SHELL_CMD(clear, NULL, "clear link key", cmd_clear_linkkey),

	SHELL_CMD(hf-connect, NULL, "hf connect ag", cmd_hf_connect_ag),
	SHELL_CMD(hf-disconnect, NULL, "hf disconnect ag", cmd_hf_disconnect_ag),
	SHELL_CMD(hf-batreport, NULL, "hf battery report", cmd_hf_batreport),
	SHELL_CMD(ag-connect, NULL, "ag connect hf", cmd_ag_connect_hf),
	SHELL_CMD(ag-disconnect, NULL, "ag disconnect hf", cmd_ag_disconnect_hf),
	SHELL_CMD(sco-connect, NULL, "connect sco", cmd_sco_connect),
	SHELL_CMD(sco-disconnect, NULL, "disconnect sco", cmd_sco_disconnect),

	SHELL_CMD(switch-role, NULL, "switch to slave role", cmd_bredr_switch_role),
	SHELL_CMD(a2dp-connect, NULL, "Connect a2dp source", cmd_bredr_a2dp_connect),
	SHELL_CMD(a2dp-disconnect, NULL, "Disonnect a2dp source", cmd_bredr_a2dp_disconnect),
	SHELL_CMD(a2dp-register, NULL, "register a2dp endpoint", cmd_bredr_a2dp_register),
	SHELL_CMD(a2dp-start, NULL, "Start a2dp play", cmd_bredr_a2dp_start),
	SHELL_CMD(a2dp-suspend, NULL, "Suspend a2dp play", cmd_bredr_a2dp_suspend),
	SHELL_CMD(a2dp-reconfig, NULL, "Reconfig a2dp", cmd_bredr_a2dp_reconfig),

	SHELL_CMD(avrcp-connect, NULL, "Connect avrcp target", cmd_bredr_avrcp_connect),
	SHELL_CMD(avrcp-disconnect, NULL, "Disonnect avrcp target", cmd_bredr_avrcp_disconnect),
	SHELL_CMD(avrcp-pass, NULL, "Send avrcp pass command", cmd_bredr_avrcp_pass),
	SHELL_CMD(avrcp-pass_continue, NULL, "Send avrcp pass continue command", cmd_bredr_avrcp_pass_continue),
	SHELL_CMD(avrcp-notify_change, NULL, "Target notify controller state change", cmd_bred_arvcp_notify_change),

	SHELL_CMD(spp-connect, NULL, "Connect spp", cmd_spp_connect),
	SHELL_CMD(spp-disconnect, NULL, "Disonnect spp", cmd_spp_disconnect),
	SHELL_CMD(spp-send, NULL, "Send spp data", cmd_spp_send),

	SHELL_CMD(hid-connect, NULL, "Connect hid", cmd_hid_connect),
	SHELL_CMD(hid-disconnect, NULL, "Disonnect hid", cmd_hid_disconnect),
	SHELL_CMD(hid-send, NULL, "Send hid data", cmd_hid_send_report),

#if defined(CONFIG_BT_PTS_TEST)
	SHELL_CMD(sco-add-connect, NULL, "connect sco by add command", cmd_sco_add_connect),
	SHELL_CMD(hf-pts, NULL, "hf pts test", cmd_hf_pts),
	SHELL_CMD(ag-pts, NULL, "hf pts test", cmd_ag_pts),
	SHELL_CMD(a2dp-pts, NULL, "a2dp pts test", cmd_bredr_a2dp_pts),
	SHELL_CMD(avrcp-pts, NULL, "avrcp pts test", cmd_bredr_avrcp_pts),
#endif

	SHELL_CMD(tws-irq, NULL, "tws irq test", cmd_tws_irq),
	SHELL_CMD(acl-fc, NULL, "acl flow control", cmd_acl_flow_control),
	SHELL_CMD(null-cnt, NULL, "read add null cnt", cmd_read_null_cnt),
	SHELL_CMD(tws-sync, NULL, "tws sync int time", cmd_tws_sync),
	SHELL_CMD(tws-play, NULL, "tws int delay play time", cmd_tws_play_time),
	SHELL_CMD(tws-irq-new, NULL, "tws irq test new", cmd_tws_irq_new),
	SHELL_SUBCMD_SET_END
);

static int cmd_acts_bt(const struct shell *shell, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(shell);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(shell, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

SHELL_CMD_REGISTER(ts, &acts_bt_cmds, "Bluetooth test shell commands", cmd_acts_bt);
