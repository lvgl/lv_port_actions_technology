/** @file common_internel.c
 * @brief Bluetooth common internel used.
 *
 * Copyright (c) 2019 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include "stdlib.h"
#include <acts_bluetooth/bluetooth.h>
#include <acts_bluetooth/conn.h>
#include <acts_bluetooth/hfp_hf.h>
#include <acts_bluetooth/l2cap.h>
#include <acts_bluetooth/a2dp.h>
#include <acts_bluetooth/avrcp_cttg.h>

#include "hci_core.h"
#include "conn_internal.h"
#include "l2cap_internal.h"
#include "rfcomm_internal.h"
#include "hfp_internal.h"
#include "avdtp_internal.h"
#include "a2dp_internal.h"
#include "avrcp_internal.h"
#include "hid_internal.h"
#include "keys.h"

#include "common_internal.h"
#include "bt_porting_inner.h"

#ifdef CONFIG_PROPERTY
#include <property_manager.h>
#endif

/* Le max connect conn, reserve one conn for le advertise. */
#if (CONFIG_BT_MAX_CONN > CONFIG_BT_MAX_BR_CONN)
#define BT_LE_MAX_CONNECT_CONN	(CONFIG_BT_MAX_CONN - CONFIG_BT_MAX_BR_CONN - 1)
#else
#define BT_LE_MAX_CONNECT_CONN	0
#endif

/* For controler acl tx
 * must large than(le acl data pkts + br acl data pkts)
 */
#define BT_ACL_TX_MAX		CONFIG_BT_CONN_TX_MAX
#define BT_BR_SIG_COUNT		(CONFIG_BT_MAX_BR_CONN*2)
#define BT_BR_RESERVE_PKT	3		/* Avoid send data used all pkts */
#define BT_LE_RESERVE_PKT	1		/* Avoid send data used all pkts */
#ifdef CONFIG_LE_RANDOM_ADDR
#ifdef CONFIG_BT_STATIC_RANDOM_ADDR
#define CFG_LE_ADDR_TYPE		BT_ADDR_LE_RANDOM		/* Static random address */
#else
#define CFG_LE_ADDR_TYPE		BT_ADDR_LE_RANDOM_PRIV	/* Random privacy address */
#endif
#else
#define CFG_LE_ADDR_TYPE		BT_ADDR_LE_PUBLIC		/* Public address */
#endif

/* rfcomm */
#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
#define RFCOMM_MAX_CREDITS_BUF         (CONFIG_BT_ACL_RX_COUNT - 1)
#else
#define RFCOMM_MAX_CREDITS_BUF         (CONFIG_BT_RX_BUF_COUNT - 1)
#endif

#if (RFCOMM_MAX_CREDITS_BUF > 7)
#define RFCOMM_MAX_CREDITS			7
#else
#define RFCOMM_MAX_CREDITS			RFCOMM_MAX_CREDITS_BUF
#endif

#if defined(CONFIG_BT_A2DP) && defined(CONFIG_BT_AVRCP_VOL_SYNC)
#define CONFIG_SUPPORT_AVRCP_VOL_SYNC		1
#else
#define CONFIG_SUPPORT_AVRCP_VOL_SYNC		0
#endif

#ifdef CONFIG_BT_PTS_TEST
#define BT_PTS_TEST_MODE		1
#else
#define BT_PTS_TEST_MODE		0
#endif

#if (CONFIG_ACTS_BT_LOG_LEVEL >= 3)
#define BT_STACK_DEBUG_LOG		1
#else
#define BT_STACK_DEBUG_LOG		0
#endif

#ifdef CONFIG_BT_RFCOMM
#define BT_RFCOMM_L2CAP_MTU		CONFIG_BT_RFCOMM_L2CAP_MTU
#else
#define BT_RFCOMM_L2CAP_MTU		650		/* Just for compile */
#endif

#define L2CAP_BR_MIN_MTU	48

/* hfp */
#define HFP_POOL_COUNT		(CONFIG_BT_MAX_BR_CONN*2)
#ifdef CONFIG_BT_PNP_INFO_SEARCH
#define SDP_CLIENT_DISCOVER_USER_BUF_LEN		512
#else
#define SDP_CLIENT_DISCOVER_USER_BUF_LEN		256
#endif

/* Bt rx/tx data pool size */
#ifdef CONFIG_SOC_NO_PSRAM
#define BT_DATA_POOL_RX_SIZE		(4*1024)		/* TODO: Better  calculate by CONFIG */
#define BT_DATA_POOL_TX_SIZE		(4*1024)		/* TODO: Better  calculate by CONFIG */
#else
#define BT_DATA_POOL_RX_SIZE		(6*1024)		/* TODO: Better  calculate by CONFIG */
#define BT_DATA_POOL_TX_SIZE		(4*1024)		/* TODO: Better  calculate by CONFIG */
#endif

typedef void (*property_flush_cb)(const char *key);
static property_flush_cb flush_cb_func;

const struct bt_inner_value_t bt_inner_value = {
	.max_conn = CONFIG_BT_MAX_CONN,
	.br_max_conn = CONFIG_BT_MAX_BR_CONN,
	.le_max_conn = BT_LE_MAX_CONNECT_CONN,
	.br_reserve_pkts = BT_BR_RESERVE_PKT,
	.le_reserve_pkts = BT_LE_RESERVE_PKT,
	.acl_tx_max = BT_ACL_TX_MAX,
	.rfcomm_max_credits = RFCOMM_MAX_CREDITS,
	.avrcp_vol_sync = CONFIG_SUPPORT_AVRCP_VOL_SYNC,
	.pts_test_mode = BT_PTS_TEST_MODE,
	.debug_log = BT_STACK_DEBUG_LOG,
	.le_addr_type = CFG_LE_ADDR_TYPE,
	.disable_hf_signal = 0,
	.l2cap_tx_mtu = CONFIG_BT_L2CAP_TX_MTU,
	.rfcomm_l2cap_mtu = BT_RFCOMM_L2CAP_MTU,
	.avdtp_rx_mtu = BT_AVDTP_MAX_MTU,
	.hf_features = BT_HFP_HF_SUPPORTED_FEATURES,
	.ag_features = BT_HFP_AG_SUPPORTED_FEATURES,
};

BT_BUF_POOL_ALLOC_DATA_DEFINE(host_rx_pool, BT_DATA_POOL_RX_SIZE);
BT_BUF_POOL_ALLOC_DATA_DEFINE(host_tx_pool, BT_DATA_POOL_TX_SIZE);

/* Pool for outgoing BR/EDR signaling packets, min MTU is 48 */
BT_BUF_POOL_DEFINE(br_sig_pool, BT_BR_SIG_COUNT,
		    BT_L2CAP_BUF_SIZE(L2CAP_BR_MIN_MTU), 4, NULL, &host_tx_pool);

/* hfp */
BT_BUF_POOL_DEFINE(hfp_pool, HFP_POOL_COUNT,
			 BT_RFCOMM_BUF_SIZE(BT_HF_CLIENT_MAX_PDU), 4, NULL, &host_tx_pool);

BT_BUF_POOL_DEFINE(sdp_client_discover_pool, CONFIG_BT_MAX_BR_CONN,
			 SDP_CLIENT_DISCOVER_USER_BUF_LEN, 4, NULL, &host_tx_pool);

/* Ble */
struct bt_keys key_pool[BT_LE_MAX_CONNECT_CONN];

/* L2cap br */
struct bt_l2cap_br bt_l2cap_br_pool[CONFIG_BT_MAX_BR_CONN] __IN_BT_SECTION;
struct bt_keys_link_key br_key_pool[CONFIG_BT_MAX_BR_CONN] __IN_BT_SECTION;

/* conn */
struct bt_conn acl_conns[CONFIG_BT_MAX_CONN] __IN_BT_SECTION;
/* CONFIG_BT_MAX_SCO_CONN == CONFIG_BT_MAX_BR_CONN */
struct bt_conn sco_conns[CONFIG_BT_MAX_BR_CONN] __IN_BT_SECTION;

/* hfp, Wait todo: Can use union to manager  hfp_hf_connection and hfp_ag_connection, for reduce memory */
struct bt_hfp_hf hfp_hf_connection[CONFIG_BT_MAX_BR_CONN] __IN_BT_SECTION;
struct bt_hfp_ag hfp_ag_connection[CONFIG_BT_MAX_BR_CONN] __IN_BT_SECTION;

/* a2dp */
struct bt_avdtp_conn avdtp_conn[CONFIG_BT_MAX_BR_CONN] __IN_BT_SECTION;

/* avrcp */
struct bt_avrcp avrcp_connection[CONFIG_BT_MAX_BR_CONN] __IN_BT_SECTION;

/* rfcomm */
struct bt_rfcomm_session bt_rfcomm_connection[CONFIG_BT_MAX_BR_CONN] __IN_BT_SECTION;

/* hid */
struct bt_hid_conn hid_conn[CONFIG_BT_MAX_BR_CONN] __IN_BT_SECTION;

void *bt_malloc(size_t size)
{
	return bt_mem_malloc(size);
}

void bt_free(void *ptr)
{
	return bt_mem_free(ptr);
}

int bt_property_set(const char *key, char *value, int value_len)
{
	int ret = -EIO;

#ifdef CONFIG_PROPERTY
	ret = property_set(key, value, value_len);
    if(!ret){
        property_flush(key);
    }
#endif
	return ret;
}

int bt_property_get(const char *key, char *value, int value_len)
{
	int ret = -EIO;

#ifdef CONFIG_PROPERTY
	ret = property_get(key, value, value_len);
#endif
	return ret;
}

int bt_property_reg_flush_cb(void *cb)
{
	flush_cb_func = cb;

	return 0;
}
