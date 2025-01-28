/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt service adapter interface
 */

#define SYS_LOG_DOMAIN "btsrv_adapter"
#include "btsrv_os_common.h"
#include "btsrv_inner.h"
#include "bt_porting_inner.h"

#define CHECK_WAIT_DISCONNECT_INTERVAL      (100)	/* 100ms */
#define WAKE_LOCK_TIMER_INTERVAL            (1000)	/* 1s */
#define WAKE_LOCK_IDLE_TIMEOUT              (1000*10)	/* 10s */
//#define CALL_BUSY_KEEP_WAKE_LOCK            1

struct btsrv_info_t *btsrv_info;
static struct btsrv_info_t btsrv_btinfo;

#if CONFIG_BT_BR_ACTS
static btsrv_discover_result_cb btsrv_discover_cb;

static uint8_t btsrv_adapter_check_role(bt_addr_t *addr, uint8_t *name)
{
	int i;
	uint8_t role = BTSRV_TWS_NONE;
	struct autoconn_info info[BTSRV_SAVE_AUTOCONN_NUM];
	struct btsrv_check_device_role_s param;

	memset(info, 0, sizeof(info));
	btsrv_connect_get_auto_reconnect_info(info, BTSRV_SAVE_AUTOCONN_NUM);
	for (i = 0; i < BTSRV_SAVE_AUTOCONN_NUM; i++) {
		if (info[i].addr_valid && !memcmp(&info[i].addr, addr, sizeof(bd_address_t))) {
			role = (info[i].tws_role == BTSRV_TWS_NONE) ? BTSRV_TWS_NONE : BTSRV_TWS_PENDING;
			break;
		}
	}

	if ((i == BTSRV_SAVE_AUTOCONN_NUM) || (role == BTSRV_TWS_NONE)) {
		memcpy(&param.addr, addr, sizeof(bd_address_t));
		param.len = strlen(name);
		param.name = name;
		if (btsrv_adapter_callback(BTSRV_CHECK_NEW_DEVICE_ROLE, (void *)&param)) {
			role = BTSRV_TWS_PENDING;
		}
	}

	return role;
}

static void _btsrv_adapter_connected_remote_name_cb(bt_addr_t *addr, uint8_t *name)
{
	uint8_t role = BTSRV_TWS_NONE;
	char addr_str[BT_ADDR_STR_LEN];
	struct btsrv_addr_name info;
	uint32_t name_len;

	if (addr == NULL) {
		SYS_LOG_ERR("Requet name error");
		return;
	}

	hostif_bt_addr_to_str(addr, addr_str, BT_ADDR_STR_LEN);
	role = btsrv_adapter_check_role(addr, name);
	SYS_LOG_INF("name_cb %s %s %d", name, addr_str, role);

	memset(&info, 0, sizeof(info));
	memcpy(info.mac.val, addr->val, sizeof(bd_address_t));
	name_len = MIN(CONFIG_MAX_BT_NAME_LEN, strlen(name));
	memcpy(info.name, name, name_len);
	btsrv_event_notify_malloc(MSG_BTSRV_CONNECT, MSG_BTSRV_GET_NAME_FINISH, (uint8_t *)&info, sizeof(info), role);
}

static bool _btsrv_adapter_connect_req_cb(bt_addr_t *peer)
{
	int i;
	uint8_t role = BTSRV_TWS_NONE;
	uint8_t trs_mode = 0;
	struct autoconn_info info[BTSRV_SAVE_AUTOCONN_NUM];
	struct bt_link_cb_param param;

	btsrv_event_notify(MSG_BTSRV_BASE, MSG_BTSRV_GET_WAKE_LOCK, NULL);

	memset(info, 0, sizeof(info));
	btsrv_connect_get_auto_reconnect_info(info, BTSRV_SAVE_AUTOCONN_NUM);
	for (i = 0; i < BTSRV_SAVE_AUTOCONN_NUM; i++) {
		if (info[i].addr_valid && !memcmp(&info[i].addr, peer, sizeof(bd_address_t))) {
			role = info[i].tws_role;
			break;
		}
	}

#ifdef CONFIG_BT_A2DP_TRS
	bool btsrv_is_record_trs_dev(bd_address_t *addr);
	trs_mode = btsrv_is_record_trs_dev((bd_address_t *)peer) ? 1 : 0;
	if (trs_mode && btsrv_rdm_get_dev_trs_mode()) {
		return false;
	}
#endif

	memset(&param, 0, sizeof(param));
	param.link_event = BT_LINK_EV_ACL_CONNECT_REQ;
	param.addr = (bd_address_t *)peer;
	param.new_dev = (i == BTSRV_SAVE_AUTOCONN_NUM) ? 1 : 0;
	param.is_tws = (role == BTSRV_TWS_NONE) ? 0 : 1;
	param.is_trs = trs_mode;
	if (btsrv_adapter_callback(BTSRV_LINK_EVENT, &param)) {
		return false;
	} else {
		return true;
	}
}

static void _btsrv_adapter_connected_cb(struct bt_conn *conn, uint8_t err)
{
	char addr_str[BT_ADDR_STR_LEN];
	bt_addr_t *bt_addr = NULL;

	if (!conn || hostif_bt_conn_get_type(conn) != BT_CONN_TYPE_BR) {
		btsrv_event_notify(MSG_BTSRV_BASE, MSG_BTSRV_GET_WAKE_LOCK, NULL);
		return;
	}

	bt_addr = (bt_addr_t *)GET_CONN_BT_ADDR(conn);
	hostif_bt_addr_to_str(bt_addr, addr_str, BT_ADDR_STR_LEN);
	SYS_LOG_INF("connected_cb %s, 0x%x", addr_str, err);

	if (err) {
		btsrv_event_notify_malloc(MSG_BTSRV_CONNECT, MSG_BTSRV_CONNECTED_FAILED, bt_addr->val, sizeof(bd_address_t), 0);
	} else {
		/* In MSG_BTSRV_CONNECTED req high performance is too late,
		 * request in stack rx thread, can't block in callback.
		 * and do release after process MSG_BTSRV_CONNECTED message.
		 */
		btsrv_adapter_callback(BTSRV_REQ_HIGH_PERFORMANCE, NULL);
		/* ref for bt servcie process MSG_BTSRV_CONNECTED,
		 * need unref conn after process MSG_BTSRV_CONNECTED.
		 */
		hostif_bt_conn_ref(conn);

		btsrv_event_notify(MSG_BTSRV_CONNECT, MSG_BTSRV_CONNECTED, conn);
		hostif_bt_remote_name_request(bt_addr, _btsrv_adapter_connected_remote_name_cb);
	}
}

static void _btsrv_adapter_disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	if (!conn || hostif_bt_conn_get_type(conn) != BT_CONN_TYPE_BR) {
		btsrv_event_notify(MSG_BTSRV_BASE, MSG_BTSRV_GET_WAKE_LOCK, NULL);
		return;
	}

	SYS_LOG_INF("disconnected_cb reason 0x%x", reason);
	btsrv_event_notify_ext(MSG_BTSRV_CONNECT, MSG_BTSRV_DISCONNECTED, conn, reason);
}

#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR)
static void _btsrv_adapter_security_changed_cb(struct bt_conn *conn, bt_security_t level,
				 enum bt_security_err err)
{
	char addr[BT_ADDR_STR_LEN];

	if (!conn || hostif_bt_conn_get_type(conn) != BT_CONN_TYPE_BR) {
		return;
	}

	hostif_bt_addr_to_str((const bt_addr_t *)GET_CONN_BT_ADDR(conn), addr, BT_ADDR_STR_LEN);
	SYS_LOG_INF("security_cb %s %d", addr, level);
	btsrv_event_notify(MSG_BTSRV_CONNECT, MSG_BTSRV_SECURITY_CHANGED, conn);
}

static void _btsrv_adapter_role_change_cb(struct bt_conn *conn, uint8_t role)
{
	char addr[BT_ADDR_STR_LEN];

	if (!conn || hostif_bt_conn_get_type(conn) != BT_CONN_TYPE_BR) {
		return;
	}

	hostif_bt_addr_to_str((const bt_addr_t *)GET_CONN_BT_ADDR(conn), addr, BT_ADDR_STR_LEN);
	SYS_LOG_INF("role_change_cb %s %d", addr, role);
	btsrv_event_notify_ext(MSG_BTSRV_CONNECT, MSG_BTSRV_ROLE_CHANGE, conn, role);
}

void _btsrv_adapter_mode_change_cb(struct bt_conn *conn, uint8_t mode, uint16_t interval)
{
	struct btsrv_mode_change_param param;

	if (!conn || hostif_bt_conn_get_type(conn) != BT_CONN_TYPE_BR) {
		return;
	}

	SYS_LOG_INF("mode change hdl 0x%x mode %d interval %d", hostif_bt_conn_get_handle(conn), mode, interval);
	param.conn = conn;
	param.mode = mode;
	param.interval = interval;
	btsrv_event_notify_malloc(MSG_BTSRV_CONNECT, MSG_BTSRV_MODE_CHANGE, (uint8_t *)&param, sizeof(param), 0);
}
#endif

static void _btsrv_connectionless_data_cb(struct bt_conn *conn, uint8_t *data, uint16_t len)
{
#ifdef CONFIG_SUPPORT_TWS
	if (btsrv_rdm_get_dev_role() != BTSRV_TWS_NONE) {
	}
#endif
}

static struct bt_conn_cb conn_callbacks = {
	.connect_req = _btsrv_adapter_connect_req_cb,
	.connected = _btsrv_adapter_connected_cb,
	.disconnected = _btsrv_adapter_disconnected_cb,
#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR)
	.security_changed = _btsrv_adapter_security_changed_cb,
	.role_change = _btsrv_adapter_role_change_cb,
	.mode_change = _btsrv_adapter_mode_change_cb,
#endif
	.rx_connectionless_data = _btsrv_connectionless_data_cb,
};

//#define BR_AUTH_PASSKEY_DISPLAY		1

// #ifdef BR_AUTH_PASSKEY_DISPLAY
static void br_auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	struct bt_conn_info info;
	char addr[13];

	if ((hostif_bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_BR)){
		return;
	}

	memset(addr, 0, 13);
	bin2hex(info.br.dst->val, 6, addr, 12);
	SYS_LOG_INF("addr: %s key:%d", addr,passkey);
}

//#define AUTH_APP_CONFIRM_CANCLE		1

/* Auth operate by app sample,
 * if open AUTH_APP_CONFIRM_CANCLE,
 * app must call auth confirm/cancel link br_auth_test_confirm/br_auth_test_cancel.
 */
#ifdef AUTH_APP_CONFIRM_CANCLE
static bd_address_t wait_auto_addr;		/* Just for inner test */

/* App call with addr */
void br_auth_test_confirm(bd_address_t *addr)
{
	struct bt_conn *conn = btsrv_rdm_find_conn_by_addr(&wait_auto_addr);	/* Actual, use addr find conn */

	if (conn) {
		hostif_bt_conn_auth_pairing_confirm(conn);
	}
	memset(&wait_auto_addr, 0, sizeof(wait_auto_addr));
}

/* App call with addr */
void br_auth_test_cancel(bd_address_t *addr)
{
	struct bt_conn *conn = btsrv_rdm_find_conn_by_addr(&wait_auto_addr); /* Actual, use addr find conn */

	if (conn) {
		hostif_bt_conn_auth_cancel(conn);
	}
	memset(&wait_auto_addr, 0, sizeof(wait_auto_addr));
}
#endif

static void br_auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	struct bt_conn_info info;
	char addr[13];

	if ((hostif_bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_BR)){
		return;
	}

#ifdef AUTH_APP_CONFIRM_CANCLE
	/* Actual,  send wait_auto_addr to app, app call confirm/cancel with wait_auto_addr */
	memcpy(&wait_auto_addr, info.br.dst, sizeof(wait_auto_addr));
#else
	hostif_bt_conn_auth_pairing_confirm(conn);
#endif

	memset(addr, 0, 13);
	bin2hex(info.br.dst->val, 6, addr, 12);
	SYS_LOG_INF("addr: %s br passkey:%d", addr, passkey);

	btsrv_event_notify(MSG_BTSRV_BASE, MSG_BTSRV_PAIR_PASSKEY_DISPLAY, (void *)passkey);
}
//#endif

static void br_auth_cancel(struct bt_conn *conn)
{
	struct bt_conn_info info;
	char addr[13];

	if ((hostif_bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_BR)){
		return;
	}

	memset(addr, 0, 13);
	bin2hex(info.br.dst->val, 6, addr, 12);
	SYS_LOG_INF("addr: %s", addr);
}

static void br_auth_pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	struct bt_conn_info info;
	char addr[13];

	if ((hostif_bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_BR)){
		return;
	}

	memset(addr, 0, 13);
	bin2hex(info.br.dst->val, 6, addr, 12);
	SYS_LOG_INF("addr %s reason %d", addr,reason);
}

static void br_auth_pairing_complete(struct bt_conn *conn, bool bonded)
{
	struct bt_conn_info info;
	char addr[13];
	bool is_bond;

	if ((hostif_bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_BR)){
		return;
	}

	is_bond = bonded;
	memset(addr, 0, 13);
	bin2hex(info.br.dst->val, 6, addr, 12);
	SYS_LOG_INF("addr %s bonded %d", addr,bonded);
	btsrv_event_notify(MSG_BTSRV_BASE, MSG_BTSRV_PAIRING_COMPLETE,(void *)is_bond);
}

static struct bt_conn_auth_cb auth_cb_display_yes_no = {
#ifdef BR_AUTH_PASSKEY_DISPLAY
	.passkey_display = br_auth_passkey_display,
	.passkey_confirm = br_auth_passkey_confirm,
#else
	.passkey_display = NULL,
	.passkey_confirm = NULL,
#endif
	.passkey_entry = NULL,
	.cancel = br_auth_cancel,
	.pairing_failed = br_auth_pairing_failed,
	.pairing_complete = br_auth_pairing_complete,
};

void btsrv_adapter_passkey_display(bool mode)
{
	// Only for test.
	if (mode) {
// #ifdef BR_AUTH_PASSKEY_DISPLAY
		auth_cb_display_yes_no.passkey_display = br_auth_passkey_display;
		auth_cb_display_yes_no.passkey_confirm = br_auth_passkey_confirm;
// #endif
	} else {
		auth_cb_display_yes_no.passkey_display = NULL;
		auth_cb_display_yes_no.passkey_confirm = NULL;
	}
	hostif_bt_conn_auth_cb_register(&auth_cb_display_yes_no);
}

void btsrv_br_resolve_addr_cb(bt_addr_t *addr)
{
	btsrv_event_notify_malloc(MSG_BTSRV_BASE, MSG_BTSRV_BR_RESOLVE_ADDR, (void *)addr, sizeof(bt_addr_t), 0);
}
#endif

static void _btsrv_adapter_ready(int err)
{
	SYS_LOG_DBG("Bt init");
	if (!err) {
#if CONFIG_BT_BR_ACTS
		hostif_bt_conn_cb_register(&conn_callbacks);
		hostif_bt_conn_auth_cb_register(&auth_cb_display_yes_no);
#ifndef CONFIG_BT_BREDR_DISABLE
		hostif_bt_br_reg_resolve_cb(btsrv_br_resolve_addr_cb);
#endif
#endif
		bt_service_set_bt_ready();
	}
	btsrv_event_notify(MSG_BTSRV_BASE, MSG_BTSRV_READY, (void *)err);
}

#if CONFIG_BT_BR_ACTS
static void btsrv_adapter_start_wait_disconnect_timer(void)
{
	if (thread_timer_is_running(&btsrv_info->wait_disconnect_timer)) {
		return;
	}

	SYS_LOG_INF("start_wait_disconnect_timer");
	thread_timer_start(&btsrv_info->wait_disconnect_timer, CHECK_WAIT_DISCONNECT_INTERVAL, CHECK_WAIT_DISCONNECT_INTERVAL);
}

static void btsrv_adapter_stop_wait_disconnect_timer(void)
{
	SYS_LOG_INF("stop_wait_disconnect_timer");
	thread_timer_stop(&btsrv_info->wait_disconnect_timer);
}

static void connected_dev_cb_check_wait_disconnect(struct bt_conn *base_conn, uint8_t tws_dev, void *cb_param)
{
	int err;
	int *wait_diconnect_cnt = cb_param;

	if (btsrv_rdm_is_wait_to_diconnect(base_conn)) {
		if (btsrv_sniff_in_sniff_mode(base_conn) == false) {
			hostif_bt_conn_ref(base_conn);
			err = hostif_bt_conn_disconnect(base_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
			if (err) {
				SYS_LOG_ERR("Disconnect failed %d", err);
			} else {
				btsrv_rdm_set_wait_to_diconnect(base_conn, false);
			}
			hostif_bt_conn_unref(base_conn);
		} else {
			(*wait_diconnect_cnt)++;
		}
	}
}

static void btsrv_adapter_wait_disconnect_timer_handler(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	int wait_diconnect_cnt = 0;

	btsrv_rdm_get_connected_dev(connected_dev_cb_check_wait_disconnect, &wait_diconnect_cnt);
	if (wait_diconnect_cnt == 0) {
		btsrv_adapter_stop_wait_disconnect_timer();
	}
}

struct btsrv_conn_call_info {
	uint8_t call_busy:1;
	uint8_t active_mode:1;
	uint32_t conn_rxtx_cnt;
};

static void connected_dev_cb_check_wake_lock(struct bt_conn *base_conn, uint8_t tws_dev, void *cb_param)
{
	struct btsrv_conn_call_info *info = cb_param;

	info->conn_rxtx_cnt += hostif_bt_conn_get_rxtx_cnt(base_conn);

#ifdef CALL_BUSY_KEEP_WAKE_LOCK
	if (btsrv_rdm_hfp_in_call_state(base_conn)) {
		info->call_busy = 1;
	}
#endif

	if (!btsrv_sniff_in_sniff_mode(base_conn)) {
		info->active_mode = 1;
	}
}

static void btsrv_adapter_wake_lock_check(bool srv_runing_check)
{
	static uint8_t wake_lock_flag = 0;
	static uint32_t conn_idle_start_time;
	static uint32_t cmr_idle_start_time;
	static uint32_t pre_conn_rxtx_cnt;
	struct btsrv_conn_call_info info;
	uint32_t curr_time;

	if (srv_runing_check) {
		if (wake_lock_flag) {
			return;
		} else {
			btsrv_adapter_set_clear_wake_lock(BTSRV_WAKE_LOCK_BTSRV_RUNING, 1);
			cmr_idle_start_time = os_uptime_get_32();
		}
	}

	memset(&info, 0, sizeof(info));
	btsrv_rdm_get_connected_dev(connected_dev_cb_check_wake_lock, &info);

	curr_time = os_uptime_get_32();

	if (pre_conn_rxtx_cnt != info.conn_rxtx_cnt) {
		conn_idle_start_time = curr_time;
		pre_conn_rxtx_cnt = info.conn_rxtx_cnt;
		btsrv_adapter_set_clear_wake_lock(BTSRV_WAKE_LOCK_CONN_BUSY, 1);
	} else {
		if ((curr_time - conn_idle_start_time) > WAKE_LOCK_IDLE_TIMEOUT) {
			btsrv_adapter_set_clear_wake_lock(BTSRV_WAKE_LOCK_CONN_BUSY, 0);
		}
	}

	if (info.call_busy) {
		cmr_idle_start_time = curr_time;
		btsrv_adapter_set_clear_wake_lock(BTSRV_WAKE_LOCK_CALL_BUSY, 1);
	} else if (info.active_mode) {
		cmr_idle_start_time = curr_time;
		btsrv_adapter_set_clear_wake_lock(BTSRV_WAKE_LOCK_ACTIVE_MODE, 1);
	} else {
		if ((curr_time - cmr_idle_start_time) > WAKE_LOCK_IDLE_TIMEOUT) {
			btsrv_adapter_set_clear_wake_lock(BTSRV_WAKE_LOCK_CALL_BUSY, 0);
			btsrv_adapter_set_clear_wake_lock(BTSRV_WAKE_LOCK_ACTIVE_MODE, 0);
			btsrv_adapter_set_clear_wake_lock(BTSRV_WAKE_LOCK_BTSRV_RUNING, 0);
		}
	}

	if (btsrv_adapter_get_wake_lock()) {
		if (!wake_lock_flag) {
			wake_lock_flag = 1;
			bt_wake_lock();
		}
	} else {
		if (wake_lock_flag) {
			wake_lock_flag = 0;
			bt_wake_unlock();
		}
	}
}

static void btsrv_adapter_wake_lock_timer_handler(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	btsrv_adapter_wake_lock_check(false);
}

void btsrv_adapter_srv_get_wake_lock(void)
{
	btsrv_adapter_wake_lock_check(true);
}
#endif

struct btsrv_info_t *btsrv_adapter_init(btsrv_callback cb)
{
	int err;
#if CONFIG_BT_BR_ACTS
	char addr_str[BT_ADDR_STR_LEN];
	uint8_t i, value;
#endif
	memset(&btsrv_btinfo, 0, sizeof(struct btsrv_info_t));

	btsrv_info = &btsrv_btinfo;
	btsrv_info->callback = cb;
#if CONFIG_BT_BR_ACTS
	btsrv_info->allow_sco_connect = 1;
	thread_timer_init(&btsrv_info->wait_disconnect_timer, btsrv_adapter_wait_disconnect_timer_handler, NULL);
	thread_timer_init(&btsrv_info->wake_lock_timer, btsrv_adapter_wake_lock_timer_handler, NULL);
#ifndef	CONFIG_BT_BREDR_DISABLE
	thread_timer_start(&btsrv_info->wake_lock_timer, WAKE_LOCK_TIMER_INTERVAL, WAKE_LOCK_TIMER_INTERVAL);
#endif
#endif
	btsrv_storage_init();
	
#if CONFIG_BT_BR_ACTS
#ifdef CONFIG_BT_3M
	void bt_l2cap_mtu_max_set(uint16_t mtu_max);
	bt_l2cap_mtu_max_set(CONFIG_BT_L2CAP_RX_MTU);
#endif
	btsrv_rdm_init();
	btsrv_connect_init();
	btsrv_scan_init();
	btsrv_link_adjust_init();
	btsrv_sniff_init();
#endif
	err = hostif_bt_enable(_btsrv_adapter_ready);

	if (err) {
		SYS_LOG_INF("Bt init failed %d", err);
		goto err;
	}
#if CONFIG_BT_BR_ACTS
	if (btsrv_property_get(CFG_BT_NAME, btsrv_info->device_name, sizeof(btsrv_info->device_name)) <= 0) {
		SYS_LOG_WRN("failed to get bt name");
	} else {
		SYS_LOG_INF("bt name: %s", (char *)btsrv_info->device_name);
	}

#ifdef CONFIG_PROPERTY
	if (property_get_byte_array(CFG_BT_MAC, btsrv_info->device_addr, sizeof(btsrv_info->device_addr), NULL)) {
		SYS_LOG_WRN("failed to get BT_MAC");
	} else
#endif
	{
		/* Like stack, low mac address save in low memory address */
		for (i=0; i<3; i++) {
			value = btsrv_info->device_addr[i];
			btsrv_info->device_addr[i] = btsrv_info->device_addr[5 -i];
			btsrv_info->device_addr[5 -i] = value;
		}

		memset(addr_str, 0, sizeof(addr_str));
		hostif_bt_addr_to_str((bt_addr_t *)btsrv_info->device_addr, addr_str, BT_ADDR_STR_LEN);
		SYS_LOG_INF("BT_MAC: %s", addr_str);
	}
#endif
	return btsrv_info;

err:
	return NULL;
}

#if CONFIG_BT_BR_ACTS
int btsrv_adapter_set_config_info(void *param)
{
	int ret = -EIO;

	if (btsrv_info) {
		memcpy(&btsrv_info->cfg, param, sizeof(btsrv_info->cfg));
		SYS_LOG_INF("btsrv config info: %d, %d, %d", btsrv_info->cfg.max_conn_num,
						btsrv_info->cfg.max_phone_num, btsrv_info->cfg.pts_test_mode);
		ret = 0;
	}

	return ret;
}

void btsrv_adapter_set_clear_wake_lock(uint16_t wake_lock, uint8_t set)
{
	if (set) {
		btsrv_info->bt_wake_lock |= wake_lock;
	} else {
		btsrv_info->bt_wake_lock &= (~wake_lock);
	}
}

int btsrv_adapter_get_wake_lock(void)
{
	return (btsrv_info->bt_wake_lock) ? 1 : 0;
}

static void btsrv_adapter_discovery_result(struct bt_br_discovery_result *result)
{
	struct btsrv_discover_result cb_result;

	if (!btsrv_discover_cb) {
		return;
	}

	memset(&cb_result, 0, sizeof(cb_result));
	if (result) {
		memcpy(&cb_result.addr, &result->addr, sizeof(bd_address_t));
		memcpy(cb_result.cod,  result->cod, 3);
		cb_result.rssi = result->rssi;
		if (result->name) {
			cb_result.name = result->name;
			cb_result.len = result->len;
			memcpy(cb_result.device_id, result->device_id, sizeof(cb_result.device_id));
		}
	} else {
		cb_result.discover_finish = 1;
	}

	btsrv_discover_cb(&cb_result);
	if (cb_result.discover_finish) {
		btsrv_discover_cb = NULL;
	}
}

int btsrv_adapter_start_discover(struct btsrv_discover_param *param)
{
	int ret;
	struct bt_br_discovery_param discovery_param;

	btsrv_discover_cb = param->cb;
	discovery_param.length = param->length;
	discovery_param.num_responses = param->num_responses;
	discovery_param.limited = false;
	ret = hostif_bt_br_discovery_start((const struct bt_br_discovery_param *)&discovery_param,
										btsrv_adapter_discovery_result);
	if (ret) {
		btsrv_discover_cb = NULL;
	}

	return ret;
}

int btsrv_adapter_stop_discover(void)
{
	hostif_bt_br_discovery_stop();
	return 0;
}

int btsrv_adapter_remote_name_request(bd_address_t *addr, bt_br_remote_name_cb_t cb)
{
	return hostif_bt_remote_name_request((const bt_addr_t *)addr, cb);
}

int btsrv_adapter_connect(bd_address_t *addr, const struct bt_br_conn_param *param)
{
	struct bt_conn *conn = hostif_bt_conn_create_br((const bt_addr_t *)addr, param);

	if (!conn) {
		SYS_LOG_ERR("Connection failed");
	} else {
		SYS_LOG_INF("Connection pending");
		/* unref connection obj in advance as app user */
		hostif_bt_conn_unref(conn);
	}

	return 0;
}

int btsrv_adapter_check_cancal_connect(bd_address_t *addr)
{
	int err;
	struct bt_conn *conn;

	conn = hostif_bt_conn_br_acl_connecting((const bt_addr_t *)addr);
	if (conn) {
		/* In connecting, hostif_bt_conn_disconnect will cancal connect */
		err = hostif_bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		hostif_bt_conn_unref(conn);
		SYS_LOG_INF("Cancal connect %d", err);
	}

	return 0;
}

int btsrv_adapter_disconnect(struct bt_conn *conn)
{
	int err = 0;

	hostif_bt_conn_check_exit_sniff(conn);

	if (btsrv_sniff_in_sniff_mode(conn)) {
		btsrv_rdm_set_wait_to_diconnect(conn, true);
		btsrv_adapter_start_wait_disconnect_timer();
	} else {
		hostif_bt_conn_ref(conn);

		err = hostif_bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		if (err) {
			SYS_LOG_ERR("Disconnect failed %d", err);
		}

		hostif_bt_conn_unref(conn);
	}

	return err;
}
#endif

void btsrv_adapter_run(void)
{
	btsrv_info->running = 1;
}

#if CONFIG_BT_BR_ACTS
void btsrv_adapter_allow_sco_connect(bool allow)
{
	btsrv_info->allow_sco_connect = allow;
}
#endif

int btsrv_adapter_stop(void)
{
#if CONFIG_BT_BR_ACTS
#ifndef	CONFIG_BT_BREDR_DISABLE
	thread_timer_stop(&btsrv_info->wake_lock_timer);
#endif
	btsrv_adapter_stop_wait_disconnect_timer();
#endif
	btsrv_info->running = 0;
	/* TODO: Call btsrv connect to disconnect all connection,
	 * other deinit must wait all disconnect finish.
	 */

	/* Wait TODO:  */
#if CONFIG_BT_BR_ACTS
	btsrv_sniff_deinit();
	btsrv_link_adjust_deinit();
	btsrv_scan_deinit();
	btsrv_connect_deinit();
#endif
	hostif_bt_disable();
#if CONFIG_BT_BR_ACTS
	btsrv_a2dp_deinit();
	btsrv_avrcp_deinit();
	btsrv_rdm_deinit();
#endif
	btsrv_info = NULL;
	return 0;
}

int btsrv_adapter_callback(btsrv_event_e event, void *param)
{
	if (btsrv_info && btsrv_info->callback) {
		return btsrv_info->callback(event, param);
	}

	return 0;
}

#if CONFIG_BT_BR_ACTS
static void btsrv_active_disconnect(bd_address_t *addr)
{
	struct bt_conn *conn;

	conn = btsrv_rdm_find_conn_by_addr(addr);
	if (!conn) {
		SYS_LOG_INF("Device not connected!");
		return;
	}

	btsrv_adapter_disconnect(conn);
}

static void btsrv_adapter_get_connected_dev_rssi(struct bt_get_rssi_param *param)
{
	struct bt_conn *conn = btsrv_rdm_find_conn_by_addr(&param->addr);
    struct bt_get_rssi_result result;
    int ret;
    int8_t rssi;
	result.status = -1;
	result.rssi = 0x7F;

	if (!conn) {
        SYS_LOG_INF("Device not connected!");
	}
    else{
        ret = hostif_bt_conn_read_rssi(conn, &rssi);
        if (!ret) {
            result.status = 0;
            result.rssi = rssi;
        }
    }
    if(param->cb){
		param->cb(&result);
    }
}

static void btsrv_adapter_get_actived_dev_rssi(struct bt_get_rssi_param *param)
{
	struct bt_conn *conn = btsrv_rdm_hfp_get_actived();
    struct bt_get_rssi_result result;
    int ret;
    int8_t rssi;
	result.status = -1;
	result.rssi = 0x7F;

	if (!conn) {
        SYS_LOG_INF("Device not connected!");
	}
    else{
        ret = hostif_bt_conn_read_rssi(conn, &rssi);
        if (!ret) {
            result.status = 0;
            result.rssi = rssi;
        }
    }
    if(param->cb){
        param->cb(&result);
    }
}

static void btsrv_br_resolve_proc(void *param)
{
	struct bt_br_resolve_param resolve_param;

	memcpy(&resolve_param.addr, param, sizeof(resolve_param.addr));
	if (btsrv_rdm_find_conn_by_addr(&resolve_param.addr)) {
		resolve_param.br_connected = 1;
	} else {
		resolve_param.br_connected = 0;
	}

	btsrv_adapter_callback(BTSRV_BR_RESOLVE_ADDR, &resolve_param);
}
#endif

int btsrv_adapter_process(struct app_msg *msg)
{
	int ret = 0;

	switch (_btsrv_get_msg_param_cmd(msg)) {
	case MSG_BTSRV_READY:
		btsrv_adapter_callback(BTSRV_READY, msg->ptr);
		break;
#if CONFIG_BT_BR_ACTS
	case MSG_BTSRV_REQ_FLUSH_NVRAM:
		btsrv_adapter_callback(BTSRV_REQ_FLUSH_PROPERTY, msg->ptr);
		break;
	case MSG_BTSRV_DISCONNECTED_REASON:
		btsrv_adapter_callback(BTSRV_DISCONNECTED_REASON, msg->ptr);
		break;
	case MSG_BTSRV_SET_DEFAULT_SCAN_PARAM:
	case MSG_BTSRV_SET_ENHANCE_SCAN_PARAM:
		btsrv_scan_set_param(msg->ptr, (_btsrv_get_msg_param_cmd(msg) == MSG_BTSRV_SET_ENHANCE_SCAN_PARAM));
		break;
	case MSG_BTSRV_SET_DISCOVERABLE:
		if (_btsrv_get_msg_param_value(msg)) {
			btsrv_scan_set_user_discoverable(true, false);
		} else {
			btsrv_scan_set_user_discoverable(false, true);
		}
		break;
	case MSG_BTSRV_SET_CONNECTABLE:
		if (_btsrv_get_msg_param_value(msg)) {
			btsrv_scan_set_user_connectable(true, false);
		} else {
			btsrv_scan_set_user_connectable(false, true);
		}
		break;
	case MSG_BTSRV_CONNECT_TO:
		btsrv_adapter_connect(msg->ptr, BT_BR_CONN_PARAM_DEFAULT);
		break;
	case MSG_BTSRV_DISCONNECT:
		btsrv_active_disconnect(msg->ptr);
		break;
	case MSG_BTSRV_GET_CONNECTED_DEV_RSSI:
		SYS_LOG_INF("MSG_BTSRV_GET_CONNECTED_DEV_RSSI");
		btsrv_adapter_get_connected_dev_rssi(msg->ptr);
		break;
    case MSG_BTSRV_GET_ACTIVED_DEV_RSSI:
		SYS_LOG_INF("MSG_BTSRV_GET_ACTIVED_DEV_RSSI");
	    btsrv_adapter_get_actived_dev_rssi(msg->ptr);
        break;
	case MSG_BTSRV_PAIR_PASSKEY_DISPLAY:
		btsrv_adapter_callback(BTSRV_PAIR_PASSKEY_DISPLAY,msg->ptr);
		break;
	case MSG_BTSRV_PAIRING_FAIL:
		btsrv_adapter_callback(BTSRV_PAIRING_FAIL, NULL);
		break;
	case MSG_BTSRV_PAIRING_COMPLETE:
		btsrv_adapter_callback(BTSRV_PAIRING_COMPLETE, msg->ptr);
		break;
	case MSG_BTSRV_BR_RESOLVE_ADDR:
		btsrv_br_resolve_proc(msg->ptr);
		break;
#endif
	default:
		break;
	}

	return ret;
}

#if CONFIG_BT_BR_ACTS
void btsrv_adapter_dump_info(void)
{
	if (!btsrv_info) {
		return;
	}

	printk("Btsrv adpter info wake_lock 0x%x allow sco %d\n", btsrv_info->bt_wake_lock, btsrv_info->allow_sco_connect);
}

void btsrv_dump_info_proc(void)
{
	btsrv_adapter_dump_info();
	btsrv_rdm_dump_info();
	btsrv_connect_dump_info();
	btsrv_scan_dump_info();
#ifdef CONFIG_SUPPORT_TWS
	btsrv_tws_dump_info();
#endif
}
#endif
