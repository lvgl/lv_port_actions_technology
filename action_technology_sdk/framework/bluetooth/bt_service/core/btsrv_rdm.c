/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt device manager interface
 */
#define SYS_LOG_DOMAIN "btsrv_rdm"
#include "btsrv_os_common.h"
#include "btsrv_inner.h"

enum {
	BTSERV_DEV_DEACTIVE,
	BTSERV_DEV_ACTIVED,
	BTSERV_DEV_ACTIVE_PENDING,
};

typedef enum {
	BTSRV_CONNECT_ACL,
	BTSRV_CONNECT_A2DP,
	BTSRV_CONNECT_AVRCP,
	BTSRV_CONNECT_HFP,
	BTSRV_CONNECT_SPP,
	BTSRV_CONNECT_PBAP,
	BTSRV_CONNECT_HID,
	BTSRV_CONNECT_MAP,
} btsrv_rdm_connect_type;

struct rdm_device {
	sys_snode_t node;

	bd_address_t bt_addr;
	struct bt_conn *base_conn;
	struct bt_conn *sco_conn;
	uint16_t acl_hdl;
	struct thread_timer sco_disconnect_timer;
	uint32_t sco_creat_time;

	uint16_t connected:1;
	uint16_t wait_to_disconnect:1;
	uint16_t security_changed:1;
	uint16_t controler_role:1;
	uint16_t tws:3;
	uint16_t trs:1;
	uint16_t a2dp_connected:1;
	uint16_t a2dp_stream_opened:1;
	uint16_t avrcp_connected:1;
	uint16_t hfp_connected:1;
	uint16_t hfp_notify_phone_num:1;
	uint16_t spp_connected:3;
	uint16_t hid_connected:1;
	uint16_t hid_plug:1;
	uint16_t pbap_connected:3;
	uint16_t map_connected:3;
	uint16_t a2dp_active_state:3;
	uint16_t a2dp_pending_ahead_start:1;
	uint16_t hfp_active_state:3;
	uint16_t hfp_role:1;
	uint16_t hfp_service:1;
	uint16_t format:8;
	uint16_t sample_rate:8;
	uint8_t sbc_max_bitpool;
	uint8_t cp_type;
	uint8_t hfp_format;
	uint8_t hfp_sample_rate;
	uint8_t hfp_state:6;
	uint8_t sco_state:2;
	uint8_t sco_connect_reject:1;
	uint8_t active_call:4;
	uint8_t held_call:1;
	uint8_t incoming_call:1;
	uint8_t outgoing_call:1;
	uint8_t a2dp_switch_locked:1;
	uint8_t switch_sbc_state:4;
	uint8_t avrcp_getting_pos:1;
	uint8_t avrcp_play_status;
    int8_t rssi;
	uint16_t link_time;
	uint32_t a2dp_start_time;
#ifdef CONFIG_BT_A2DP_TRS
	uint16_t a2dp_mtu;
	struct bt_a2dp_media_codec a2dp_codec;
	struct thread_timer trs_discover_timer;
#endif
	uint8_t device_name[CONFIG_MAX_BT_NAME_LEN + 1];

	struct btsrv_rdm_avrcp_pass_info avrcp_pass_info;
	/* Sniff manager info */
	struct rdm_sniff_info sniff;
};

#define __RMT_DEV(_node) CONTAINER_OF(_node, struct rdm_device, node)

struct btsrv_rdm_priv {
	/* TODO, protect me, no need, ensure all opration in btsrv thread */
	sys_slist_t dev_list;	/* connected device list */
	uint8_t role_type:3;		/* Current device role: tws master, tws slave, tws none */
	uint8_t trs_mode:1;		/* Current device mode: trs, trs none */
};

static struct btsrv_rdm_priv *p_rdm;
static struct btsrv_rdm_priv btsrv_rdm;

int btsrv_rdm_hid_actived(struct bt_conn *base_conn, uint8_t actived);

static struct rdm_device *btsrv_rdm_find_dev_by_addr(bd_address_t *addr)
{
	struct rdm_device *dev;
	sys_snode_t *node;

	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);
		if (dev->connected == 1 &&
			memcmp(addr, dev->bt_addr.val, sizeof(bt_addr_t)) == 0)
			return dev;
	}

	return NULL;
}

static struct rdm_device *btsrv_rdm_find_dev_by_conn(struct bt_conn *base_conn)
{
	struct rdm_device *dev;
	sys_snode_t *node;

	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);
		if (dev->base_conn == base_conn)
			return dev;
	}

	return NULL;
}

static struct rdm_device *_btsrv_rdm_find_dev_by_sco_conn(struct bt_conn *sco_conn)
{
	struct rdm_device *dev;
	sys_snode_t *node;

	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);
		if (dev->sco_conn == sco_conn)
			return dev;
	}

	return NULL;
}

static struct rdm_device *btsrv_rdm_find_dev_by_connect_type(struct bt_conn *base_conn, int type)
{
	struct rdm_device *dev;
	sys_snode_t *node;
	bool paired = false;

	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);

		if (type == BTSRV_CONNECT_ACL) {
			paired = (dev->connected == 1);
		} else if (type == BTSRV_CONNECT_A2DP) {
			paired = (dev->a2dp_connected == 1);
		} else if (type == BTSRV_CONNECT_AVRCP) {
			paired = (dev->avrcp_connected == 1);
		} else if (type == BTSRV_CONNECT_HFP) {
			paired = (dev->hfp_connected == 1);
		} else if (type == BTSRV_CONNECT_SPP) {
			paired = (dev->spp_connected != 0);
		} else if (type == BTSRV_CONNECT_PBAP) {
			paired = (dev->pbap_connected != 0);
		} else if (type == BTSRV_CONNECT_HID) {
			paired = (dev->hid_connected != 0);
		} else if (type == BTSRV_CONNECT_MAP) {
			paired = (dev->map_connected != 0);
		}

		if (paired && dev->base_conn == base_conn)
			return dev;
	}

	return NULL;
}

static struct rdm_device *btsrv_rdm_find_dev_by_connect_type_tws(struct bt_conn *base_conn, int type, uint8_t role)
{
	struct rdm_device *dev;
	sys_snode_t *node;
	bool paired = false;

	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);

		if (type == BTSRV_CONNECT_ACL) {
			paired = (dev->connected == 1);
		} else if (type == BTSRV_CONNECT_A2DP) {
			paired = (dev->a2dp_connected == 1);
		} else if (type == BTSRV_CONNECT_AVRCP) {
			paired = (dev->avrcp_connected == 1);
		} else if (type == BTSRV_CONNECT_HFP) {
			paired = (dev->hfp_connected == 1);
		}else if (type == BTSRV_CONNECT_HID) {
			paired = (dev->hid_connected == 1);
		}

		if (paired && dev->base_conn == base_conn && dev->trs == 0 && dev->tws == role)
			return dev;
	}

	return NULL;
}

/* Only for BTSRV_TWS_NONE */
static struct rdm_device *btsrv_rdm_find_second_dev_by_connect_type(struct bt_conn *base_conn, int type)
{
	struct rdm_device *dev;
	sys_snode_t *node;
	bool paired = false;

	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);

		if (type == BTSRV_CONNECT_ACL) {
			paired = (dev->connected == 1);
		} else if (type == BTSRV_CONNECT_A2DP) {
			paired = (dev->a2dp_connected == 1);
		} else if (type == BTSRV_CONNECT_AVRCP) {
			paired = (dev->avrcp_connected == 1);
		} else if (type == BTSRV_CONNECT_HFP) {
			paired = (dev->hfp_connected == 1);
		}else if (type == BTSRV_CONNECT_HID) {
			paired = (dev->hid_connected == 1);
		}

		if (paired && dev->base_conn != base_conn && dev->trs == 0 && dev->tws == BTSRV_TWS_NONE)
			return dev;
	}

	return NULL;
}

static struct rdm_device *btsrv_rdm_a2dp_get_actived_device(void)
{
	struct rdm_device *dev;
	sys_snode_t *node;

	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);
		if ((dev->tws == BTSRV_TWS_NONE) && dev->trs == 0 && dev->a2dp_connected &&
			(dev->a2dp_active_state == BTSERV_DEV_ACTIVED)) {
			return dev;
		}
	}

	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);
		if ((dev->tws == BTSRV_TWS_NONE) && dev->trs == 0 && dev->a2dp_connected &&
			(dev->a2dp_active_state == BTSERV_DEV_ACTIVE_PENDING)) {
			return dev;
		}
	}

	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);
		if ((dev->tws == BTSRV_TWS_NONE) && dev->trs == 0 && dev->a2dp_connected) {
			return dev;
		}
	}

	return NULL;
}

static struct rdm_device *btsrv_rdm_hfp_get_actived_device(void)
{
	struct rdm_device *dev;
	sys_snode_t *node;

	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);
		if ((dev->tws == BTSRV_TWS_NONE) && dev->trs == 0 && dev->hfp_connected &&
			(dev->hfp_active_state == BTSERV_DEV_ACTIVED)) {
			return dev;
		}
	}

	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);
		if ((dev->tws == BTSRV_TWS_NONE) && dev->trs == 0 && dev->hfp_connected &&
			(dev->hfp_active_state == BTSERV_DEV_ACTIVE_PENDING)) {
			return dev;
		}
	}
	/* active dev change during connect */
	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);
		if ((dev->tws == BTSRV_TWS_NONE) && dev->trs == 0 /*&& dev->hfp_connected*/ &&
			(dev->hfp_active_state == BTSERV_DEV_ACTIVED)) {
			return dev;
		}
	}

	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);
		if ((dev->tws == BTSRV_TWS_NONE) && dev->trs == 0 /*&& dev->hfp_connected*/ &&
			(dev->hfp_active_state == BTSERV_DEV_ACTIVE_PENDING)) {
			return dev;
		}
	}

	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);
		if ((dev->tws == BTSRV_TWS_NONE) && dev->trs == 0 && dev->hfp_connected) {
			return dev;
		}
	}

	/* Need this ?? */
	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);
		if (dev->tws == BTSRV_TWS_NONE && dev->trs == 0) {
			return dev;
		}
	}

	return NULL;
}

bool btsrv_rdm_need_high_performance(void)
{
	bool high_performance = false;
	struct rdm_device *dev;
	sys_snode_t *node;

	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);
		if (dev->connected) {
			if (!dev->hfp_connected && !dev->a2dp_connected &&
				!dev->avrcp_connected && !dev->spp_connected &&
				!dev->pbap_connected && !dev->hid_connected &&
				!dev->map_connected ) {
				high_performance = true;
			}
		}
	}

	return high_performance;
}

struct bt_conn *btsrv_rdm_find_conn_by_addr(bd_address_t *addr)
{
	struct rdm_device *dev = btsrv_rdm_find_dev_by_addr(addr);

	if (dev)
		return dev->base_conn;

	return NULL;
}

struct bt_conn *btsrv_rdm_find_conn_by_hdl(uint16_t hdl)
{
	struct rdm_device *dev;
	sys_snode_t *node;

	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);
		if (dev->connected == 1 && dev->acl_hdl == hdl)
			return dev->base_conn;
	}

	return NULL;
}

int btsrv_rdm_get_connected_dev(rdm_connected_dev_cb cb, void *cb_param)
{
	struct rdm_device *dev;
	sys_snode_t *node;
	int count = 0;

	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);
		if (dev->connected) {
			count++;
			if (cb) {
				cb(dev->base_conn, dev->tws, cb_param);
			}
		}
	}

	return count;
}

int btsrv_rdm_get_connected_dev_cnt_by_type(int type)
{
	struct rdm_device *dev;
	sys_snode_t *node;
	int count = 0;

	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);
		if (dev->connected) {
			if (BTSRV_DEVICE_ALL == type
				|| (BTSRV_DEVICE_PHONE == type && dev->tws == BTSRV_TWS_NONE)
				|| (BTSRV_DEVICE_TWS == type && dev->tws != BTSRV_TWS_NONE)) {
				count++;
			}
		}
	}

	return count;
}
int btsrv_rdm_get_dev_state(struct bt_dev_rdm_state *state)
{
	int ret = 0;
	struct rdm_device *dev = btsrv_rdm_find_dev_by_addr(&state->addr);

	if (dev == NULL) {
		state->acl_connected = 0;
		state->hfp_connected = 0;
		state->a2dp_connected = 0;
		state->avrcp_connected = 0;
		state->hid_connected = 0;
		state->a2dp_stream_started = 0;
		state->sco_connected = 0;
		ret = -1;
	} else {
		state->acl_connected = dev->connected;
		state->hfp_connected = dev->hfp_connected;
		state->a2dp_connected = dev->a2dp_connected;
		state->avrcp_connected = dev->avrcp_connected;
		state->hid_connected = dev->hid_connected;
		state->a2dp_stream_started = dev->a2dp_stream_opened;
		state->sco_connected = (dev->sco_conn) ? 1 : 0;
	}

	return ret;
}

int btsrv_rdm_get_autoconn_dev(struct autoconn_info *info, int max_dev)
{
	struct rdm_device *dev;
	sys_snode_t *node;
	int cnt = 0;

	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);
		if (dev->connected && (dev->a2dp_connected |
			dev->avrcp_connected | dev->hfp_connected | dev->hid_connected)) {
			memcpy(&info[cnt].addr.val, dev->bt_addr.val, sizeof(bd_address_t));
			info[cnt].addr_valid = 1;
			info[cnt].tws_role = dev->tws;
#ifdef CONFIG_BT_A2DP_TRS
			info[cnt].trs_mode = dev->trs;
#endif
			info[cnt].a2dp = dev->a2dp_connected;
			info[cnt].avrcp = dev->avrcp_connected;
			info[cnt].hfp = dev->hfp_connected;
			info[cnt].hid = dev->hid_connected;
			if (dev->a2dp_connected && (!dev->hfp_connected)) {
				info[cnt].hfp_first = 0;
			} else {
				info[cnt].hfp_first = 1;
			}
			cnt++;
			if (cnt == max_dev) {
				return cnt;
			}
		}
	}

	return cnt;
}

int btsrv_rdm_base_disconnected(struct bt_conn *base_conn)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_connect_type(base_conn, BTSRV_CONNECT_ACL);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected or already disconnected\n");
		return -ENODEV;
	}

	dev->connected = 0;
	return 0;
}

int btsrv_rdm_add_dev(struct bt_conn *base_conn)
{
	char addr_str[BT_ADDR_STR_LEN];
	struct rdm_device *dev;
	int i;
	struct autoconn_info *tmpInfo;
	int8_t rssi_val;
	bd_address_t *addr = (bd_address_t *)GET_CONN_BT_ADDR(base_conn);

	dev = btsrv_rdm_find_dev_by_connect_type(base_conn, BTSRV_CONNECT_ACL);
	if (dev != NULL) {
		SYS_LOG_WRN("already connected??\n");
		return -EEXIST;
	}

	dev = bt_mem_malloc(sizeof(struct rdm_device));
	if (dev == NULL) {
		SYS_LOG_ERR("malloc failed!!\n");
		return -ENODEV;
	}

	memset(dev, 0, sizeof(struct rdm_device));
	memcpy(dev->bt_addr.val, addr, sizeof(bt_addr_t));
	dev->connected = 1;
	dev->base_conn = hostif_bt_conn_ref(base_conn);
	dev->acl_hdl = hostif_bt_conn_get_handle(base_conn);
	sys_slist_append(&p_rdm->dev_list, &dev->node);
	dev->hfp_format = BT_CODEC_ID_CVSD;
	dev->hfp_sample_rate = 8;
    dev->rssi = 0x7F;
    if(!hostif_bt_conn_read_rssi(base_conn,&rssi_val)){
        dev->rssi = rssi_val;
    }
	tmpInfo = bt_mem_malloc(sizeof(struct autoconn_info)*BTSRV_SAVE_AUTOCONN_NUM);
	if (!tmpInfo) {
		SYS_LOG_ERR("malloc failed");
		return -ENOMEM;
	}
	btsrv_connect_get_auto_reconnect_info(tmpInfo,BTSRV_SAVE_AUTOCONN_NUM);
	for (i = 0; i < BTSRV_SAVE_AUTOCONN_NUM; i++) {
		if (tmpInfo[i].addr_valid) {
			if (!memcmp(tmpInfo[i].addr.val, addr->val, sizeof(bd_address_t))) {
				if(tmpInfo[i].hid){
					btsrv_rdm_hid_actived(base_conn,1);
				}
				break;
			}
		}
	}
	if (tmpInfo) {
		bt_mem_free(tmpInfo);
	}

	hostif_bt_addr_to_str((const bt_addr_t *)addr, addr_str, BT_ADDR_STR_LEN);
	SYS_LOG_INF("add_dev %s rssi:%d", addr_str,dev->rssi);
	return 0;
}

int btsrv_rdm_remove_dev(uint8_t *mac)
{
	char addr_str[BT_ADDR_STR_LEN];
	struct rdm_device *dev;
	sys_snode_t *node;
	uint8_t find = 0;

	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);
		if (memcmp(mac, dev->bt_addr.val, sizeof(bt_addr_t)) == 0) {
			find = 1;
			break;
		}
	}

	if (!find) {
		SYS_LOG_WRN("not connected??\n");
		return -EALREADY;
	}

	if (dev->connected || dev->hfp_connected || dev->a2dp_connected || dev->hid_connected ||
		dev->avrcp_connected || dev->spp_connected || dev->pbap_connected) {
		SYS_LOG_WRN("Still connected %d %d %d %d %d %d %d\n", dev->connected,
					dev->hfp_connected, dev->a2dp_connected, dev->hid_connected,
					dev->avrcp_connected, dev->spp_connected, dev->pbap_connected);
		return -EBUSY;
	}

	/* Tws disconnected, set device rolt to BTSRV_TWS_NONE */
	if (dev->tws != BTSRV_TWS_NONE) {
		p_rdm->role_type = BTSRV_TWS_NONE;
	}

#ifdef CONFIG_BT_A2DP_TRS
	/* Trs disconnected, set device mode to TRS NONE */
	if (dev->trs != 0) {
		p_rdm->trs_mode = 0;
	}
#endif

	hostif_bt_conn_unref(dev->base_conn);
	sys_slist_find_and_remove(&p_rdm->dev_list, &dev->node);
	bt_mem_free(dev);

	hostif_bt_addr_to_str((const bt_addr_t *)mac, addr_str, BT_ADDR_STR_LEN);
	SYS_LOG_INF("remove_dev %s\n", addr_str);
	return 0;
}

void btsrv_rdm_set_security_changed(struct bt_conn *base_conn)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return;
	}

	dev->security_changed = 1;
}

bool btsrv_rdm_is_security_changed(struct bt_conn *base_conn)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return false;
	}

	return (dev->security_changed) ? true : false;
}

bool btsrv_rdm_is_connected(struct bt_conn *base_conn)
{
	return (btsrv_rdm_find_dev_by_connect_type(base_conn, BTSRV_CONNECT_ACL) != NULL);
}

bool btsrv_rdm_is_a2dp_connected(struct bt_conn *base_conn)
{
	return (btsrv_rdm_find_dev_by_connect_type(base_conn, BTSRV_CONNECT_A2DP) != NULL);
}

bool btsrv_rdm_is_avrcp_connected(struct bt_conn *base_conn)
{
	return (btsrv_rdm_find_dev_by_connect_type(base_conn, BTSRV_CONNECT_AVRCP) != NULL);
}

bool btsrv_rdm_is_hfp_connected(struct bt_conn *base_conn)
{
	return (btsrv_rdm_find_dev_by_connect_type(base_conn, BTSRV_CONNECT_HFP) != NULL);
}

bool btsrv_rdm_is_spp_connected(struct bt_conn *base_conn)
{
	return (btsrv_rdm_find_dev_by_connect_type(base_conn, BTSRV_CONNECT_SPP) != NULL);
}

bool btsrv_rdm_is_hid_connected(struct bt_conn *base_conn)
{
	return (btsrv_rdm_find_dev_by_connect_type(base_conn, BTSRV_CONNECT_HID) != NULL);
}

int btsrv_rdm_set_a2dp_connected(struct bt_conn *base_conn, bool connected)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		SYS_LOG_WRN("dev not add\n");
		return -ENODEV;
	}

	if ((connected && dev->a2dp_connected) ||
		((!connected) && (!dev->a2dp_connected))) {
		SYS_LOG_WRN("a2dp already %s\n", connected ? "connected" : "disconnected");
		return -EALREADY;
	}

	if (connected) {
		dev->a2dp_connected = 1;
	} else {
		dev->a2dp_connected = 0;
	}
	return 0;
}

int btsrv_rdm_a2dp_actived_switch_lock(struct bt_conn *base_conn, uint8_t lock)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		SYS_LOG_WRN("dev not add\n");
		return -ENODEV;
	}

	if (lock) {
		dev->a2dp_switch_locked = 1;
	} else {
		dev->a2dp_switch_locked = 0;
	}
	return 0;
}

int btsrv_rdm_a2dp_actived(struct bt_conn *base_conn, uint8_t actived)
{
	struct rdm_device *dev;
	struct rdm_device *others;

	dev = btsrv_rdm_find_dev_by_connect_type_tws(base_conn, BTSRV_CONNECT_A2DP, BTSRV_TWS_NONE);
	if (dev == NULL) {
		SYS_LOG_WRN("Not tws_none dev or a2dp not connected\n");
		return -ENODEV;
	}
	SYS_LOG_INF("active %d dev %p a2dp_active_state %d lock %d\n", actived, dev->base_conn, dev->a2dp_active_state,
		dev->a2dp_switch_locked);

	if (actived) {
		dev->a2dp_stream_opened = 1;
		dev->a2dp_start_time = os_uptime_get_32();
	} else {
		dev->a2dp_stream_opened = 0;
		dev->a2dp_start_time = 0;
	}

	others = btsrv_rdm_find_second_dev_by_connect_type(base_conn, BTSRV_CONNECT_A2DP);

	/* only one phone , this phone always actived */
	if (!others) {
		dev->a2dp_active_state = BTSERV_DEV_ACTIVED;
		return 0;
	}

	SYS_LOG_INF("active %d others %p a2dp_active_state %d lock %d\n", actived, others->base_conn, others->a2dp_active_state,
		others->a2dp_switch_locked);

	/** we not switch when a2dp switch locked */
	if (dev->a2dp_switch_locked)
		return 0;

	/* double phone
	 * improve: we can call back to upper layer with dev active state,
	 * upper layer decide how change active state.
	 */
	if (actived) {
		if (others->a2dp_switch_locked
			|| (!btsrv_is_preemption_mode() 
				&& others->a2dp_active_state == BTSERV_DEV_ACTIVED 
				&& others->a2dp_stream_opened)) {
			dev->a2dp_active_state = BTSERV_DEV_ACTIVE_PENDING;
		} else {
			if (dev->a2dp_active_state != BTSERV_DEV_ACTIVED) {
				dev->a2dp_active_state = BTSERV_DEV_ACTIVED;
				if (others->a2dp_stream_opened)
					others->a2dp_active_state = BTSERV_DEV_ACTIVE_PENDING;
				else
					others->a2dp_active_state = BTSERV_DEV_DEACTIVE;
				btsrv_event_notify(MSG_BTSRV_A2DP, MSG_BTSRV_A2DP_ACTIVED_DEV_CHANGED, dev->base_conn);
			}
		}
	} else {
		if (others->a2dp_active_state == BTSERV_DEV_ACTIVED) {
			dev->a2dp_active_state = BTSERV_DEV_DEACTIVE;
		} else if (others->a2dp_active_state == BTSERV_DEV_ACTIVE_PENDING || others->a2dp_stream_opened) {
			dev->a2dp_active_state = BTSERV_DEV_DEACTIVE;
			others->a2dp_active_state = BTSERV_DEV_ACTIVED;
			if (others->a2dp_stream_opened)
				btsrv_event_notify(MSG_BTSRV_A2DP, MSG_BTSRV_A2DP_ACTIVED_DEV_CHANGED, others->base_conn);
		} else {
			dev->a2dp_active_state = BTSERV_DEV_ACTIVE_PENDING;
		}
	}

	return 0;
}

struct bt_conn *btsrv_rdm_a2dp_get_actived(void)
{
	struct rdm_device *dev = btsrv_rdm_a2dp_get_actived_device();

	if (dev) {
		return dev->base_conn;
	}

	return NULL;
}

struct bt_conn *btsrv_rdm_avrcp_get_actived(void)
{
#ifdef CONFIG_BT_A2DP
    return btsrv_rdm_a2dp_get_actived();
#else
    return btsrv_rdm_avrcp_get_connected_dev();
#endif
}

struct bt_conn *btsrv_rdm_a2dp_get_second_dev(void)
{
	struct bt_conn *conn = btsrv_rdm_a2dp_get_actived();
	struct rdm_device *dev = btsrv_rdm_find_second_dev_by_connect_type(conn, BTSRV_CONNECT_ACL);

	if (dev) {
		return dev->base_conn;
	}

	return NULL;
}

int btsrv_rdm_is_actived_a2dp_stream_open(void)
{
	struct rdm_device *dev = btsrv_rdm_a2dp_get_actived_device();

	if (dev) {
		return dev->a2dp_stream_opened;
	}

	return 0;
}

int btsrv_rdm_is_a2dp_stream_open(struct bt_conn *base_conn)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_connect_type(base_conn, BTSRV_CONNECT_ACL);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return 0;
	}

	return dev->a2dp_stream_opened;
}

int btsrv_rdm_a2dp_set_codec_info(struct bt_conn *base_conn, uint8_t format, uint8_t sample_rate, uint8_t cp_type)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_connect_type(base_conn, BTSRV_CONNECT_ACL);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}

	dev->format = format;
	dev->sample_rate = sample_rate;
	dev->cp_type = cp_type;

	return 0;
}

int btsrv_rdm_a2dp_get_codec_info(struct bt_conn *base_conn, uint8_t *format, uint8_t *sample_rate, uint8_t *cp_type)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_connect_type(base_conn, BTSRV_CONNECT_ACL);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}

	if (format) {
		*format = dev->format;
	}

	if (sample_rate) {
		*sample_rate = dev->sample_rate;
	}

	if (cp_type) {
		*cp_type = dev->cp_type;
	}

	return 0;
}

int btsrv_rdm_a2dp_set_bitpool(struct bt_conn *base_conn, uint8_t bitpool)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_connect_type(base_conn, BTSRV_CONNECT_ACL);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}

	dev->sbc_max_bitpool = bitpool;
	return 0;
}

uint8_t btsrv_rdm_a2dp_get_bitpool(struct bt_conn *base_conn)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_connect_type(base_conn, BTSRV_CONNECT_ACL);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return 0;
	}

	return dev->sbc_max_bitpool;
}


int btsrv_rdm_get_a2dp_start_time(struct bt_conn *base_conn, uint32_t *start_time)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}

	*start_time = dev->a2dp_start_time;
	return 0;
}

void btsrv_rdm_get_a2dp_acitve_mac(bd_address_t *addr)
{
	struct rdm_device *dev = btsrv_rdm_a2dp_get_actived_device();

	if (dev) {
		memcpy(addr, &dev->bt_addr, sizeof(bd_address_t));
	} else {
		memset(addr, 0, sizeof(bd_address_t));
	}
}

int btsrv_rdm_set_a2dp_pending_ahead_start(struct bt_conn *base_conn, uint8_t start)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}

	dev->a2dp_pending_ahead_start = start ? 1 : 0;
	return 0;
}

uint8_t btsrv_rdm_get_a2dp_pending_ahead_start(struct bt_conn *base_conn)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return 0;
	}

	return dev->a2dp_pending_ahead_start;
}

int btsrv_rdm_set_avrcp_connected(struct bt_conn *base_conn, bool connected)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}

	if ((connected && dev->avrcp_connected) ||
		((!connected) && (!dev->avrcp_connected))) {
		SYS_LOG_WRN("avrcp already %s\n", connected ? "connected" : "disconnected");
		return -EALREADY;
	}

	if (connected) {
		dev->avrcp_connected = 1;
	} else {
		dev->avrcp_connected = 0;
	}
	return 0;
}

int btsrv_rdm_avrcp_update_play_status(struct bt_conn *base_conn, uint8_t status)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_connect_type(base_conn, BTSRV_CONNECT_AVRCP);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}

	SYS_LOG_INF("status %d\n", status);
	dev->avrcp_play_status = status;

	return 0;
}

struct bt_conn *btsrv_rdm_acl_get_connected_dev(void)
{
	struct rdm_device *dev;
	sys_snode_t *node;

	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);
		if (dev->connected == 1 && dev->trs == 0) {
			return dev->base_conn;
		}
	}

	return NULL;
}

struct bt_conn *btsrv_rdm_avrcp_get_connected_dev(void)
{
	struct rdm_device *dev;
	sys_snode_t *node;

	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);
		if (dev->avrcp_connected == 1 && (dev->trs == 0)) {
			return dev->base_conn;
		}
	}

	return NULL;
}

struct bt_conn *btsrv_rdm_a2dp_get_connected_dev(void)
{
	struct rdm_device *dev;
	sys_snode_t *node;

	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);
		if (dev->connected == 1 && dev->a2dp_connected == 1 && dev->trs == 0) {
			return dev->base_conn;
		}
	}

	return NULL;
}

void btsrv_rdm_avrcp_set_getting_pos_time(struct bt_conn *base_conn, uint8_t getting)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		return;
	}

	if (getting) {
		dev->avrcp_getting_pos = 1;
	} else {
		dev->avrcp_getting_pos = 0;
	}
}

int btsrv_rdm_avrcp_get_getting_pos_time(struct bt_conn *base_conn)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		return -EIO;
	}

	return dev->avrcp_getting_pos;
}

void *btsrv_rdm_avrcp_get_pass_info(struct bt_conn *base_conn)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return NULL;
	}

	return &dev->avrcp_pass_info;
}

int btsrv_rdm_set_hfp_connected(struct bt_conn *base_conn, bool connected)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}

	if ((connected && dev->hfp_connected) ||
		((!connected) && (!dev->hfp_connected))) {
		SYS_LOG_WRN("hfp already %s\n", connected ? "connected" : "disconnected");
		return -EALREADY;
	}

	if (connected) {
		dev->hfp_connected = 1;
	} else {
		dev->hfp_connected = 0;
	}
	return 0;
}

int btsrv_rdm_set_hfp_role(struct bt_conn *base_conn, uint8_t role)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}

	dev->hfp_role = role;
	return 0;
}

int btsrv_rdm_get_hfp_role(struct bt_conn *base_conn)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return BTSRV_HFP_ROLE_HF;
	}

	return dev->hfp_role;
}

int btsrv_rdm_hfp_actived(struct bt_conn *base_conn, uint8_t actived, uint8_t force)
{
	struct rdm_device *dev;
	struct rdm_device *others;

	SYS_LOG_INF("base_conn: %p actived %d\n", base_conn, actived);
	/* active state may update during connect  */
	dev = btsrv_rdm_find_dev_by_connect_type_tws(base_conn, BTSRV_CONNECT_ACL, BTSRV_TWS_NONE);
	if (dev == NULL) {
		SYS_LOG_WRN("Not tws_none or not connected\n");
		return -ENODEV;
	}

	others = btsrv_rdm_find_second_dev_by_connect_type(base_conn, BTSRV_CONNECT_ACL);

	/* only one phone , this phone always actived */
	if (!others) {
		dev->hfp_active_state = BTSERV_DEV_ACTIVED;
		return 0;
	}

	SYS_LOG_INF("dev %p hfp_active_state %d state %d\n", dev, dev->hfp_active_state, dev->hfp_state);
	SYS_LOG_INF("others %p hfp_active_state %d state %d\n", others, others->hfp_active_state, others->hfp_state);

	/* double phone
	 * improve: we can call back to upper layer with dev active state,
	 * upper layer decide how change active state.
	 */
	if (actived) {
		if ((dev->hfp_state >= BTSRV_HFP_STATE_CALL_INCOMING
			&& dev->hfp_state <= BTSRV_HFP_STATE_SCO_ESTABLISHED) || force){
			if (others->hfp_active_state == BTSERV_DEV_ACTIVED && (others->hfp_state >= BTSRV_HFP_STATE_CALL_INCOMING
				&& others->hfp_state <= BTSRV_HFP_STATE_SCO_ESTABLISHED) && !force) {
				dev->hfp_active_state = BTSERV_DEV_ACTIVE_PENDING;
			} else {
				if (dev->hfp_active_state != BTSERV_DEV_ACTIVED) {
					dev->hfp_active_state = BTSERV_DEV_ACTIVED;
					if (others->hfp_state >= BTSRV_HFP_STATE_CALL_INCOMING
						&& others->hfp_state <= BTSRV_HFP_STATE_SCO_ESTABLISHED)
						others->hfp_active_state = BTSERV_DEV_ACTIVE_PENDING;
					else
						others->hfp_active_state = BTSERV_DEV_DEACTIVE;
					if (others->hfp_state >= BTSRV_HFP_STATE_CALL_INCOMING &&
						others->hfp_state <= BTSRV_HFP_STATE_SCO_ESTABLISHED)
						btsrv_event_notify(MSG_BTSRV_HFP, MSG_BTSRV_HFP_ACTIVED_DEV_CHANGED, dev->base_conn);
				}
			}
		}
	} else {
		if (others->hfp_active_state == BTSERV_DEV_ACTIVED) {
			dev->hfp_active_state = BTSERV_DEV_DEACTIVE;
		} else if (others->hfp_active_state == BTSERV_DEV_ACTIVE_PENDING) {
			others->hfp_active_state = BTSERV_DEV_ACTIVED;
			dev->hfp_active_state = BTSERV_DEV_DEACTIVE;
			btsrv_event_notify(MSG_BTSRV_HFP, MSG_BTSRV_HFP_ACTIVED_DEV_CHANGED, others->base_conn);
		} else {
			if (dev->hfp_state != BTSRV_HFP_STATE_INIT)
				dev->hfp_active_state = BTSERV_DEV_ACTIVE_PENDING;
			else
				dev->hfp_active_state = BTSERV_DEV_DEACTIVE;
		}
	}

	return 0;
}

int btsrv_rdm_hfp_set_codec_info(struct bt_conn *base_conn, uint8_t format, uint8_t sample_rate)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_connect_type(base_conn, BTSRV_CONNECT_ACL);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}

	dev->hfp_format = format;
	dev->hfp_sample_rate = sample_rate;

	return 0;
}

int btsrv_rdm_hfp_set_notify_phone_num_state(struct bt_conn *base_conn, uint8_t state)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_connect_type(base_conn, BTSRV_CONNECT_ACL);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}
	dev->hfp_notify_phone_num = state;
	return 0;
}

int btsrv_rdm_hfp_get_notify_phone_num_state(struct bt_conn *base_conn)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_connect_type(base_conn, BTSRV_CONNECT_ACL);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}

	return dev->hfp_notify_phone_num;
}

int btsrv_rdm_hfp_set_state(struct bt_conn *base_conn, uint8_t state)
{
	struct rdm_device *dev;
	int old_state;

	dev = btsrv_rdm_find_dev_by_connect_type(base_conn, BTSRV_CONNECT_ACL);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}
	old_state = dev->hfp_state;
	dev->hfp_state = state;
	if ((old_state < BTSRV_HFP_STATE_CALL_INCOMING || old_state > BTSRV_HFP_STATE_SCO_ESTABLISHED) &&
		(state >= BTSRV_HFP_STATE_CALL_INCOMING && state <= BTSRV_HFP_STATE_SCO_ESTABLISHED))
		btsrv_rdm_hfp_actived(base_conn, 1, 0);
	else if ((state < BTSRV_HFP_STATE_CALL_INCOMING || state > BTSRV_HFP_STATE_SCO_ESTABLISHED) &&
		(old_state >= BTSRV_HFP_STATE_CALL_INCOMING && old_state <= BTSRV_HFP_STATE_SCO_ESTABLISHED))
		btsrv_rdm_hfp_actived(base_conn, 0, 0);
	return 0;
}

int btsrv_rdm_hfp_get_state(struct bt_conn *base_conn)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_connect_type(base_conn, BTSRV_CONNECT_ACL);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}

	return dev->hfp_state;
}

int btsrv_rdm_hfp_set_sco_state(struct bt_conn *base_conn, uint8_t state)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_connect_type(base_conn, BTSRV_CONNECT_ACL);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}
	SYS_LOG_INF("set_sco_state %d to %d\n", dev->sco_state, state);
	dev->sco_state = state;
	return 0;
}

int btsrv_rdm_hfp_get_sco_state(struct bt_conn *base_conn)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_connect_type(base_conn, BTSRV_CONNECT_ACL);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}
	SYS_LOG_INF("get_sco_state %d\n", dev->sco_state);

	return dev->sco_state;
}


int btsrv_rdm_hfp_set_call_state(struct bt_conn *base_conn, uint8_t active, uint8_t held, uint8_t in, uint8_t out)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_connect_type(base_conn, BTSRV_CONNECT_ACL);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}

	dev->active_call = active;
	dev->held_call = held;
	dev->incoming_call = in;
	dev->outgoing_call = out;
	return 0;
}

int btsrv_rdm_hfp_get_call_state(struct bt_conn *base_conn, uint8_t *active, uint8_t *held, uint8_t *in, uint8_t *out)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_connect_type(base_conn, BTSRV_CONNECT_ACL);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}

	*active = dev->active_call;
	*held = dev->held_call;
	*in = dev->incoming_call;
	*out = dev->outgoing_call;
	return 0;
}

int btsrv_rdm_hfp_get_codec_info(struct bt_conn *base_conn, uint8_t *format, uint8_t *sample_rate)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_connect_type(base_conn, BTSRV_CONNECT_ACL);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}

	*format = dev->hfp_format;
	*sample_rate = dev->hfp_sample_rate;

	return 0;
}

struct bt_conn *btsrv_rdm_hfp_get_actived(void)
{
	struct rdm_device *dev = btsrv_rdm_hfp_get_actived_device();
	if (dev) {
		/* SYS_LOG_INF("base_conn : %p actived %d\n", dev->base_conn, dev->hfp_actived); */
		return dev->base_conn;
	}

	return NULL;
}

struct bt_conn *btsrv_rdm_hfp_get_second_dev(void)
{
	struct bt_conn *conn = btsrv_rdm_hfp_get_actived();
	struct rdm_device *dev = btsrv_rdm_find_second_dev_by_connect_type(conn, BTSRV_CONNECT_ACL);

	if (dev) {
		/* SYS_LOG_INF("base_conn : %p second %d\n", dev->base_conn, dev->hfp_actived); */
		return dev->base_conn;
	}

	return NULL;
}

struct bt_conn *btsrv_rdm_hfp_get_actived_sco(void)
{
	struct rdm_device *dev = btsrv_rdm_hfp_get_actived_device();

	if (dev) {
		/* SYS_LOG_INF("base_conn : %p actived %d\n", dev->base_conn, dev->hfp_actived); */
		return dev->sco_conn;
	}

	return NULL;
}

bool btsrv_rdm_hfp_in_call_state(struct bt_conn *base_conn)
{
	struct rdm_device *dev = btsrv_rdm_find_dev_by_conn(base_conn);

	if (dev && (dev->hfp_state > BTSRV_HFP_STATE_LINKED)) {
		return true;
	} else {
		return false;
	}
}

void btsrv_rdm_get_hfp_acitve_mac(bd_address_t *addr)
{
	struct rdm_device *dev = btsrv_rdm_hfp_get_actived_device();

	if (dev) {
		memcpy(addr, &dev->bt_addr, sizeof(bd_address_t));
	} else {
		memset(addr, 0, sizeof(bd_address_t));
	}
}

void btsrv_rdm_set_hfp_service(struct bt_conn *base_conn, uint8_t service)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_connect_type(base_conn, BTSRV_CONNECT_ACL);
	if (dev == NULL) {
		return;
	}

	dev->hfp_service = service ? 1 : 0;
}

uint8_t btsrv_rdm_get_hfp_service(void)
{
	struct rdm_device *dev = btsrv_rdm_hfp_get_actived_device();

	if (dev) {
		return dev->hfp_service;
	} else {
		return 0;
	}
}

int btsrv_rdm_get_sco_creat_time(struct bt_conn *base_conn, uint32_t *creat_time)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}

	*creat_time = dev->sco_creat_time;
	return 0;
}

struct thread_timer * btsrv_rdm_get_sco_disconnect_timer(struct bt_conn *base_conn)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return NULL;
	}

	return &dev->sco_disconnect_timer;
}

int btsrv_rdm_sco_connected(struct bt_conn *base_conn, struct bt_conn *sco_conn)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		SYS_LOG_WRN("dev not add\n");
		return -ENODEV;
	}
	dev->sco_creat_time = os_uptime_get_32();
	dev->sco_conn = sco_conn;
	return 0;
}

int btsrv_rdm_sco_disconnected(struct bt_conn *sco_conn)
{
	struct rdm_device *dev;

	dev = _btsrv_rdm_find_dev_by_sco_conn(sco_conn);
	if (dev == NULL) {
		SYS_LOG_WRN("dev not add\n");
		return -ENODEV;
	}
	dev->sco_creat_time = 0;
	dev->sco_conn = NULL;
	return 0;
}

bool btsrv_rdm_is_sco_connected(struct bt_conn *acl_conn)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(acl_conn);
	if (dev == NULL) {
		return false;
	}

	return (dev->sco_conn) ? true : false;
}

void btsrv_rdm_set_sco_connected_reject(struct bt_conn *acl_conn, uint8_t reject)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(acl_conn);
	if (dev == NULL) {
		return;
	}

	dev->sco_connect_reject = reject ? 1 : 0;
}

bool btsrv_rdm_is_sco_connected_reject(struct bt_conn *acl_conn)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(acl_conn);
	if (dev == NULL) {
		return false;
	}

	return dev->sco_connect_reject ? true : false;
}

struct bt_conn *btsrv_rdm_get_base_conn_by_sco(struct bt_conn *sco_conn)
{
	struct rdm_device *dev;

	dev = _btsrv_rdm_find_dev_by_sco_conn(sco_conn);
	if (dev == NULL) {
		SYS_LOG_ERR("not found\n");
		return NULL;
	}

	return dev->base_conn;
}

int btsrv_rdm_set_spp_connected(struct bt_conn *base_conn, bool connected)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		SYS_LOG_WRN("dev not add\n");
		return -ENODEV;
	}

	if (!connected && !dev->spp_connected) {
		SYS_LOG_WRN("All spp already disconnected\n");
		return -EALREADY;
	}

	if (connected) {
		dev->spp_connected++;
	} else {
		dev->spp_connected--;
	}
	return 0;
}

int btsrv_rdm_set_pbap_connected(struct bt_conn *base_conn, bool connected)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		SYS_LOG_WRN("dev not add\n");
		return -ENODEV;
	}

	if (!connected && !dev->pbap_connected) {
		SYS_LOG_WRN("All pbap already disconnected\n");
		return -EALREADY;
	}

	if (connected) {
		dev->pbap_connected++;
	} else {
		dev->pbap_connected--;
	}
	return 0;
}

int btsrv_rdm_set_map_connected(struct bt_conn *base_conn, bool connected)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		SYS_LOG_WRN("dev not add\n");
		return -ENODEV;
	}

	if (!connected && !dev->map_connected) {
		SYS_LOG_WRN("All map already disconnected\n");
		return -EALREADY;
	}

	if (connected) {
		dev->map_connected++;
	} else {
		dev->map_connected--;
	}
	return 0;
}


int btsrv_rdm_hid_actived(struct bt_conn *base_conn, uint8_t actived)
{
	struct rdm_device *dev;
	struct rdm_device *others;

	dev = btsrv_rdm_find_dev_by_connect_type_tws(base_conn, BTSRV_CONNECT_ACL, BTSRV_TWS_NONE);
	if (dev == NULL) {
		SYS_LOG_WRN("No tws_none dev connected\n");
		return -ENODEV;
	}
	
	others = btsrv_rdm_find_second_dev_by_connect_type(base_conn, BTSRV_CONNECT_ACL);

	/* only one phone , this phone always actived */
	if (!others) {
		dev->hid_plug = 1;
		return 0;
	}else{
		if(dev->hid_connected || !others->hid_connected){
			dev->hid_plug = actived;
			others->hid_plug = !actived;
		}
	}

	return 0;
}

int btsrv_rdm_set_hid_connected(struct bt_conn *base_conn, bool connected)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		SYS_LOG_WRN("dev not add\n");
		return -ENODEV;
	}

	if (!connected && !dev->hid_connected) {
		SYS_LOG_WRN("All hid already disconnected\n");
		return -EALREADY;
	}

	if (connected) {
		dev->hid_connected = 1;
		btsrv_rdm_hid_actived(base_conn,1);
	} else {
		dev->hid_connected = 0;
		btsrv_rdm_hid_actived(base_conn,0);
	}
	return 0;
}

//TO DO
static struct rdm_device *btsrv_rdm_hid_get_actived_device(void)
{
	struct rdm_device *dev;
	sys_snode_t *node;

	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);
		if ((dev->tws == BTSRV_TWS_NONE) && dev->hid_connected) {
			return dev;
		}
	}

	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);
		if ((dev->tws == BTSRV_TWS_NONE) && dev->hid_plug) {
			return dev;
		}
	}
	//hid connect info may be clear,so just use a2dp info
	return btsrv_rdm_a2dp_get_actived_device();	
}

struct bt_conn *btsrv_rdm_hid_get_actived(void)
{
	struct rdm_device *dev = btsrv_rdm_hid_get_actived_device();

	if (dev) {
		return dev->base_conn;
	}

	return NULL;
}

int btsrv_rdm_set_tws_role(struct bt_conn *base_conn, uint8_t role)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_connect_type(base_conn, BTSRV_CONNECT_ACL);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}

	if (dev->connected != 1) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}

	dev->tws = role&0x7;
	if (role != BTSRV_TWS_NONE) {
		p_rdm->role_type = role;
		SYS_LOG_INF("set_tws_role %d\n", role);
	}
	return 0;
}

struct bt_conn *btsrv_rdm_get_tws_by_role(uint8_t role)
{
	struct rdm_device *dev;
	sys_snode_t *node;

	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);
		if (dev->tws == role)
			return dev->base_conn;
	}

	return NULL;
}

int btsrv_rdm_get_conn_role(struct bt_conn *base_conn)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}

	return dev->tws;
}

int btsrv_rdm_get_dev_role(void)
{
	if (p_rdm) {
		return p_rdm->role_type;
	}

	return 0;
}

int btsrv_rdm_set_controler_role(struct bt_conn *base_conn, uint8_t role)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}

	dev->controler_role = role ? 1 : 0;
	return 0;
}

int btsrv_rdm_get_controler_role(struct bt_conn *base_conn, uint8_t *role)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}

	*role = dev->controler_role;
	return 0;
}

int btsrv_rdm_set_link_time(struct bt_conn *base_conn, uint16_t link_time)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}

	dev->link_time = link_time;
	return 0;
}

uint16_t btsrv_rdm_get_link_time(struct bt_conn *base_conn)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return 0;
	}

	return dev->link_time;
}

void btsrv_rdm_set_dev_name(struct bt_conn *base_conn, uint8_t *name)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return;
	}

	memcpy(dev->device_name, name, strlen(name));
}

uint8_t *btsrv_rdm_get_dev_name(struct bt_conn *base_conn)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return NULL;
	}

	return dev->device_name;
}

void btsrv_rdm_set_wait_to_diconnect(struct bt_conn *base_conn, bool set)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return;
	}

	dev->wait_to_disconnect = set ? 1 : 0;
}

bool btsrv_rdm_is_wait_to_diconnect(struct bt_conn *base_conn)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return false;
	}

	return dev->wait_to_disconnect ? true : false;
}

void btsrv_rdm_set_switch_sbc_state(struct bt_conn *base_conn, uint8_t state)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev) {
		dev->switch_sbc_state = state;
	}
}

uint8_t btsrv_rdm_get_switch_sbc_state(struct bt_conn *base_conn)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev) {
		return dev->switch_sbc_state;
	} else {
		return 0;
	}
}

int btsrv_rdm_get_active_dev_rssi(void)
{
	struct rdm_device *dev = btsrv_rdm_hfp_get_actived_device();

	if (dev) {
		return dev->rssi;
	} else {
		return 0x7F;
	}
}

void *btsrv_rdm_get_sniff_info(struct bt_conn *base_conn)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		return NULL;
	}

	return &dev->sniff;
}

uint16_t btsrv_rdm_get_actived_phone_hdl(void)
{
	struct rdm_device *dev = btsrv_rdm_hfp_get_actived_device();

	if (dev) {
		return dev->acl_hdl;
	} else {
		return 0;
	}
}

int btsrv_rdm_init(void)
{
	p_rdm = &btsrv_rdm;

	memset(p_rdm, 0, sizeof(struct btsrv_rdm_priv));
	sys_slist_init(&p_rdm->dev_list);
	return 0;
}

void btsrv_rdm_deinit(void)
{
	struct rdm_device *dev;
	sys_snode_t *node;

	if (p_rdm == NULL)
		return;

	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);

		if (dev->connected == 1) {
			bt_mem_free(dev);
			break;
		}
	}

	p_rdm = NULL;
}

void btsrv_rdm_dump_info(void)
{
	char addr_str[BT_ADDR_STR_LEN];
	struct rdm_device *dev;
	sys_snode_t *node;

	if (p_rdm == NULL) {
		printk("rdm not init\n");
		return;
	}

	printk("Rdm info p_rdm->role_type: %d\n", p_rdm->role_type);
	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);
		hostif_bt_addr_to_str((const bt_addr_t *)&dev->bt_addr, addr_str, BT_ADDR_STR_LEN);
		printk("MAC %s name %s base conn %p soc conn %p\n", addr_str, dev->device_name, dev->base_conn, dev->sco_conn);
		printk("\t Connected %d tws %d, controle_role %d\n", dev->connected, dev->tws, dev->controler_role);
		printk("\t Connected a2dp %d avrcp %d hfp %d spp %d pbap %d hid %d map %d\n", dev->a2dp_connected, dev->avrcp_connected,
				dev->hfp_connected, dev->spp_connected, dev->pbap_connected, dev->hid_connected,dev->map_connected);
		printk("\t A2dp active_state %d stream_opened %d codec %d sample %d cp %d\n", dev->a2dp_active_state,
				dev->a2dp_stream_opened, dev->format, dev->sample_rate, dev->cp_type);
		printk("\t Hfp active_state %d, format %d, sample %d\n", dev->hfp_active_state, dev->hfp_format, dev->hfp_sample_rate);
		printk("\t Hfp state %d sco state %d notify state %d hfp role %d\n", dev->hfp_state, dev->sco_state,
				dev->hfp_notify_phone_num, dev->hfp_role);
		printk("\t Avrcp status %d Hid plug %d\n", dev->avrcp_play_status, dev->hid_plug);
		printk("\t Sniff mode %d interval %d entering %d exiting %d\n", dev->sniff.sniff_mode,
				dev->sniff.sniff_interval, dev->sniff.sniff_entering, dev->sniff.sniff_exiting);
#ifdef CONFIG_BT_A2DP_TRS
		printk("\t trs_mode %d trs %d a2dp_mtu %d codec %d\n", p_rdm->trs_mode, dev->trs, dev->a2dp_mtu, dev->a2dp_codec.head.codec_type);
#endif
	}
}

#ifdef CONFIG_BT_A2DP_TRS
struct bt_conn *btsrv_rdm_trs_avrcp_get_actived(void)
{
	struct rdm_device *dev;
	sys_snode_t *node;

	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);
		if (dev->connected == 1 && dev->trs == 1 && dev->avrcp_connected)
			return dev->base_conn;
	}

	return NULL;
}

struct bt_conn *btsrv_rdm_trs_a2dp_get_actived(void)
{
	struct rdm_device *dev;
	sys_snode_t *node;

	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);
		if (dev->connected == 1 && dev->a2dp_connected == 1 && dev->trs == 1)
			return dev->base_conn;
	}

	return NULL;
}

int btsrv_rdm_set_trs_mode(bd_address_t *addr, uint8_t trs_mode)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_addr(addr);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}

	if (dev->connected != 1) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}

	dev->trs = trs_mode;

	if (dev->trs == 1) {
		p_rdm->trs_mode = 1;
	}
	return 0;
}

int btsrv_rdm_get_trs_mode(struct bt_conn *base_conn)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		return 0;
	}

	return dev->trs;
}

int btsrv_rdm_get_direction(bd_address_t *addr)
{
	struct bt_conn *conn;

	if((conn = btsrv_rdm_find_conn_by_addr(addr)) != NULL) {
		return hostif_bt_conn_get_direction(conn);
	}

	return 0;
}

int btsrv_rdm_trs_a2dp_set_codec_info(struct bt_conn *base_conn, struct bt_a2dp_media_codec *codec)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_connect_type(base_conn, BTSRV_CONNECT_ACL);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}

	dev->a2dp_codec = *codec;

	return 0;
}

int btsrv_rdm_trs_a2dp_set_mtu(struct bt_conn *base_conn, uint16_t mtu)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_connect_type(base_conn, BTSRV_CONNECT_ACL);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}

	dev->a2dp_mtu = mtu;

	return 0;
}

int btsrv_rdm_trs_a2dp_get_codec_info(struct bt_conn *base_conn, struct bt_a2dp_media_codec *codec, uint16_t *media_mtu)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_connect_type(base_conn, BTSRV_CONNECT_ACL);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return -ENODEV;
	}

	if (codec) {
		*codec = dev->a2dp_codec;
	}

	if (media_mtu) {
		*media_mtu = dev->a2dp_mtu;
	}

	return 0;
}

int btsrv_rdm_get_dev_trs_mode(void)
{
	if (p_rdm) {
		return p_rdm->trs_mode;
	}

	return 0;
}

int btsrv_rdm_trs_a2dp_stream_open(struct bt_conn *base_conn, bool open)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_connect_type(base_conn, BTSRV_CONNECT_ACL);
	if (dev == NULL) {
		return -ENODEV;
	}

	dev->a2dp_stream_opened = open ? 1 : 0;
	return 0;
}

struct bt_conn *btsrv_rdm_trs_get_conn(void)
{
	struct rdm_device *dev;
	sys_snode_t *node;

	SYS_SLIST_FOR_EACH_NODE(&p_rdm->dev_list, node) {
		dev = __RMT_DEV(node);
		if (dev->connected == 1 && dev->trs == 1)
			return dev->base_conn;
	}

	return NULL;
}

struct thread_timer * btsrv_rdm_get_trs_discover_timer(struct bt_conn *base_conn)
{
	struct rdm_device *dev;

	dev = btsrv_rdm_find_dev_by_conn(base_conn);
	if (dev == NULL) {
		SYS_LOG_WRN("not connected??\n");
		return NULL;
	}

	return &dev->trs_discover_timer;
}
#endif
