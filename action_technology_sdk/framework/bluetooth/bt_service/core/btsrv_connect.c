/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt connect/disconnect service
 */

#define SYS_LOG_DOMAIN "btsrv_connect"
#include "btsrv_os_common.h"
#include "btsrv_inner.h"

#define AUTOCONN_START_TIME						(100)	/* 100ms */
#define AUTOCONN_GET_NAME_TIME					(200)	/* 200ms */
#define AUTOCONN_MASTER_DELAY					(2000)	/* 1000ms */
#define AUTOCONN_DELAY_TIME_MASK				(5000)	/* 5000ms */
#ifdef CONFIG_BT_A2DP_TRS
#define AUTOCONN_PRIFILE_DELAY					(3000)	/* 3000ms */
#define RECORD_A2DP_TRS_DEV						"TRS_DEV_MAC"
#define RECORD_TRS_DEV_MAX_NUM					4
#endif
#define MONITOR_A2DP_TIMES						(8)
#define MONITOR_AVRCP_TIMES						(6)
#define MONITOR_AVRCP_CONNECT_INTERVAL			(3)
#define MONITOR_PRIFILE_INTERVEL				(1000)	/* 1000ms */

#define BT_SUPERVISION_TIMEOUT					(8000)		/* 8000*0.625ms = 5000ms */

enum {
	AUTOCONN_STATE_IDLE,
	AUTOCONN_STATE_DELAY_CONNECTING,
	AUTOCONN_STATE_BASE_CONNECTING,
	AUTOCONN_STATE_BASE_CONNECTED,
	AUTOCONN_STATE_PROFILE_CONNETING,
};

enum {
	AUTOCONN_PROFILE_IDLE,
	AUTOCONN_PROFILE_HFP_CONNECTING,
	AUTOCONN_PROFILE_A2DP_CONNECTING,
	AUTOCONN_PROFILE_AVRCP_CONNECTING,
	AUTOCONN_PROFILE_HID_CONNECTING,
	AUTOCONN_PROFILE_CONNECTING_MAX,
};

enum {
	AUTOCONN_RECONNECT_CLEAR_ALL,
	AUTOCONN_RECONNECT_CLEAR_PHONE,
	AUTOCONN_RECONNECT_CLEAR_TWS,
};

enum {
	SWITCH_SBC_STATE_IDLE,
	SWITCH_SBC_STATE_DISCONNECTING_A2DP,
	SWITCH_SBC_STATE_CONNECTING_A2DP,
};

struct auto_conn_t {
	bd_address_t addr;
	uint8_t addr_valid:1;
	uint8_t tws_role:3;
	uint8_t trs_mode:1;
	uint8_t a2dp:1;
	uint8_t avrcp:1;
	uint8_t hfp:1;
	uint8_t hfp_first:1;
	uint8_t hid:1;
	uint8_t strategy;
	uint8_t base_try;
	uint8_t profile_try;
	uint8_t curr_connect_profile;
	uint8_t state;
	uint16_t base_interval;
	uint16_t profile_interval;
};

struct profile_conn_t {
	bd_address_t addr;
	uint8_t valid:1;
	uint8_t avrcp_times:4;
	uint8_t a2dp_times:4;
};

struct btsrv_connect_priv {
	struct autoconn_info nvram_reconn_info[BTSRV_SAVE_AUTOCONN_NUM];	/* Reconnect info save in nvram */
	struct auto_conn_t auto_conn[BTSRV_SAVE_AUTOCONN_NUM];				/* btsvr connect use for doing reconnect bt */
	struct thread_timer auto_conn_timer;
	uint8_t connecting_index:3;
	uint8_t auto_connect_running:1;
	uint8_t clear_list_disconnecting:1;
	uint8_t clear_list_mode:3;
	/* Monitor connect profile, connect by phone */
	struct profile_conn_t monitor_conn[BTSRV_SAVE_AUTOCONN_NUM];
	struct thread_timer monitor_conn_timer;
	uint8_t monitor_timer_running:1;
	uint8_t curr_req_performance:1;
	uint8_t reconnect_req_high_performance:1;
#ifdef CONFIG_BT_A2DP_TRS
    struct bt_conn *clear_trs_dev_conn;
    struct bt_paired_list_dev_t paired_dev_list[RECORD_TRS_DEV_MAX_NUM];
#endif
};

static void btsrv_proc_link_change(uint8_t *mac, uint8_t type);
static struct btsrv_connect_priv *p_connect;
static struct btsrv_connect_priv p_btsrv_connect;

static void btsrv_update_performance_req(void)
{
	uint8_t need_high;
	bool rdm_need_high_performance = btsrv_rdm_need_high_performance();

	if (rdm_need_high_performance || p_connect->reconnect_req_high_performance) {
		need_high = 1;
	} else {
		need_high = 0;
	}

	if (need_high && !p_connect->curr_req_performance) {
		p_connect->curr_req_performance = 1;
		SYS_LOG_INF("BAC req high perf");
		btsrv_adapter_callback(BTSRV_REQ_HIGH_PERFORMANCE, NULL);
	} else if (!need_high && p_connect->curr_req_performance) {
		p_connect->curr_req_performance = 0;
		SYS_LOG_INF("BAC release high perf");
		btsrv_adapter_callback(BTSRV_RELEASE_HIGH_PERFORMANCE, NULL);
	}
}

static void btsrv_update_nvram_auto_conn_info(void)
{
	btsrv_property_set(CFG_AUTOCONN_INFO, (void *)p_connect->nvram_reconn_info,
				sizeof(p_connect->nvram_reconn_info));
}

int btsrv_connect_get_auto_reconnect_info(struct autoconn_info *info, uint8_t max_cnt)
{
	int dev_cnt = 0, read_cnt, i;

	read_cnt = (max_cnt > BTSRV_SAVE_AUTOCONN_NUM) ? BTSRV_SAVE_AUTOCONN_NUM : max_cnt;
	memcpy((char *)info, (char *)p_connect->nvram_reconn_info, (sizeof(struct autoconn_info)*read_cnt));
	for (i = 0; i < read_cnt; i++) {
		if (info[i].addr_valid) {
			dev_cnt++;
		}
	}

	return dev_cnt;
}

void btsrv_connect_set_auto_reconnect_info(struct autoconn_info *info, uint8_t max_cnt)
{
	int write_cnt;

	write_cnt = (max_cnt > BTSRV_SAVE_AUTOCONN_NUM) ? BTSRV_SAVE_AUTOCONN_NUM : max_cnt;
	memcpy((char *)p_connect->nvram_reconn_info, (char *)info, (sizeof(struct autoconn_info)*write_cnt));
	btsrv_update_nvram_auto_conn_info();
}

void btsrv_autoconn_info_update(void)
{
	struct autoconn_info *info, *tmpInfo;
	int connected_cnt, i, j, have_tws = 0;
#ifdef CONFIG_BT_A2DP_TRS
    int have_trs = 0;
#endif

	info = bt_mem_malloc(sizeof(struct autoconn_info)*BTSRV_SAVE_AUTOCONN_NUM*2);
	if (!info) {
		SYS_LOG_ERR("BAC info_update failed");
		goto update_exit;
	}

	memset(info, 0, (sizeof(struct autoconn_info)*BTSRV_SAVE_AUTOCONN_NUM*2));
	connected_cnt = btsrv_rdm_get_autoconn_dev(info, BTSRV_SAVE_AUTOCONN_NUM);
	if (connected_cnt == 0) {
		goto update_exit;
	}

	/* Only save one tws device info */
	for (i = 0; i < connected_cnt; i++) {
		if (info[i].tws_role != BTSRV_TWS_NONE) {
			have_tws = 1;
		}
#ifdef CONFIG_BT_A2DP_TRS
		/* Only save one trs device info */
		if (info[i].trs_mode != 0) {
			have_trs = 1;
		}
#endif
	}

	tmpInfo = info;
	tmpInfo += BTSRV_SAVE_AUTOCONN_NUM;
	btsrv_connect_get_auto_reconnect_info(tmpInfo, BTSRV_SAVE_AUTOCONN_NUM);
	for (i = 0; i < BTSRV_SAVE_AUTOCONN_NUM; i++) {
		if (tmpInfo[i].addr_valid) {
			for (j = 0; j < connected_cnt; j++) {
				if (!memcmp(tmpInfo[i].addr.val, info[j].addr.val, sizeof(bd_address_t))) {
					info[j].a2dp |= tmpInfo[i].a2dp;
					info[j].avrcp |= tmpInfo[i].avrcp;
					info[j].hfp |= tmpInfo[i].hfp;
					info[j].hid |= tmpInfo[i].hid;
					tmpInfo[i].addr_valid = 0;
					break;
				}
			}

			if (have_tws && tmpInfo[i].tws_role != BTSRV_TWS_NONE) {
				continue;
			}

#ifdef CONFIG_BT_A2DP_TRS
			if (have_trs && tmpInfo[i].trs_mode != 0) {
				continue;
			}
#endif

			if ((j == connected_cnt) && (connected_cnt < BTSRV_SAVE_AUTOCONN_NUM)) {
				memcpy(&info[connected_cnt], &tmpInfo[i], sizeof(struct autoconn_info));
				connected_cnt++;
			}
		}
	}

	btsrv_connect_set_auto_reconnect_info(info, BTSRV_SAVE_AUTOCONN_NUM);

update_exit:
	if (info) {
		bt_mem_free(info);
	}
}

static void btsrv_autoconn_info_clear(void)
{
	for (int i = 0; i < BTSRV_SAVE_AUTOCONN_NUM; i++) {
		if ((p_connect->nvram_reconn_info[i].tws_role != BTSRV_TWS_NONE && p_connect->clear_list_mode == BTSRV_DEVICE_TWS)
			|| (p_connect->nvram_reconn_info[i].tws_role == BTSRV_TWS_NONE && p_connect->clear_list_mode == BTSRV_DEVICE_PHONE)
			|| (p_connect->clear_list_mode == BTSRV_DEVICE_ALL)) {
			memset(&p_connect->nvram_reconn_info[i], 0, sizeof(struct autoconn_info));
			break;
		}
	}
	p_connect->clear_list_mode = 0;
	btsrv_update_nvram_auto_conn_info();
}

static void btsrv_connect_auto_connection_stop(void)
{
	int i;

	SYS_LOG_INF("BAC auto_connection_stop");
	for (i = 0; i < BTSRV_SAVE_AUTOCONN_NUM; i++) {
		if (p_connect->auto_conn[i].addr_valid &&
			(p_connect->auto_conn[i].state == AUTOCONN_STATE_BASE_CONNECTING)) {
			btsrv_adapter_check_cancal_connect(&p_connect->auto_conn[i].addr);
		}
	}

	memset(&p_connect->auto_conn, 0, sizeof(p_connect->auto_conn));
	thread_timer_stop(&p_connect->auto_conn_timer);
	btsrv_adapter_set_clear_wake_lock(BTSRV_WAKE_LOCK_RECONNECT, 0);
	btsrv_scan_update_mode(false);
	p_connect->reconnect_req_high_performance = 0;
	btsrv_update_performance_req();
}

static void btsrv_connect_auto_connection_restart(int32_t duration, int32_t period)
{
	SYS_LOG_INF("BAC connection_restart %d, %d", duration, period);
	thread_timer_start(&p_connect->auto_conn_timer, duration, period);
	btsrv_adapter_set_clear_wake_lock(BTSRV_WAKE_LOCK_RECONNECT, 1);
	p_connect->reconnect_req_high_performance = 1;
	btsrv_update_performance_req();
}

static void btsrv_connect_monitor_profile_stop(void)
{
	SYS_LOG_INF("BAC monitor_profile_stop");
	thread_timer_stop(&p_connect->monitor_conn_timer);
	btsrv_adapter_set_clear_wake_lock(BTSRV_WAKE_LOCK_WAIT_CONNECT, 0);
}

static void btsrv_connect_monitor_profile_start(int32_t duration, int32_t period)
{
	if (thread_timer_is_running(&p_connect->monitor_conn_timer)) {
		return;
	}

	SYS_LOG_INF("BAC monitor_profile_start %d, %d", duration, period);
	thread_timer_start(&p_connect->monitor_conn_timer, duration, period);
	btsrv_adapter_set_clear_wake_lock(BTSRV_WAKE_LOCK_WAIT_CONNECT, 1);
}

static struct auto_conn_t *btsrv_autoconn_find_wait_reconn(void)
{
	uint8_t i, start_index, end_index;
	struct auto_conn_t *auto_conn = NULL;

	start_index = p_connect->connecting_index;
	end_index = BTSRV_SAVE_AUTOCONN_NUM;

try_again:
	for (i = start_index; i < end_index; i++) {
		if (p_connect->auto_conn[i].addr_valid) {
			auto_conn = &p_connect->auto_conn[i];
			SYS_LOG_INF("BAC wait_reconn %d, %d, %d, %d%d%d%d", i, auto_conn->state, auto_conn->tws_role,
						auto_conn->a2dp, auto_conn->avrcp, auto_conn->hfp, auto_conn->hid);
			p_connect->connecting_index = i;
			break;
		}
	}

	if ((auto_conn == NULL) && (start_index != 0)) {
		start_index = 0;
		end_index = p_connect->connecting_index;
		goto try_again;
	}

	return auto_conn;
}

static void btsrv_update_autoconn_state(uint8_t *addr, uint8_t event)
{
	uint8_t i, master;
	uint8_t index = BTSRV_SAVE_AUTOCONN_NUM;

	for (i = 0; i < BTSRV_SAVE_AUTOCONN_NUM; i++) {
		if (p_connect->auto_conn[i].addr_valid &&
			!memcmp(p_connect->auto_conn[i].addr.val, addr, sizeof(bt_addr_t))) {
			index = i;
			break;
		}
	}

	if (index == BTSRV_SAVE_AUTOCONN_NUM) {
		return;
	}

	switch (event) {
	case BTSRV_LINK_BASE_CONNECTED:
		if (p_connect->auto_conn[index].state < AUTOCONN_STATE_BASE_CONNECTED) {
			p_connect->auto_conn[index].state = AUTOCONN_STATE_BASE_CONNECTED;
#ifdef CONFIG_BT_A2DP_TRS
			btsrv_rdm_set_trs_mode(&p_connect->auto_conn[index].addr, p_connect->auto_conn[index].trs_mode);
#endif
		}

		if (index == p_connect->connecting_index) {
			/* After base connected, wait get name finish to trigger next step, set get name time */
			btsrv_connect_auto_connection_restart(AUTOCONN_GET_NAME_TIME, 0);
		}
		break;

	case BTSRV_LINK_BASE_DISCONNECTED:
		p_connect->auto_conn[index].addr_valid = 0;
		p_connect->auto_conn[index].state = AUTOCONN_STATE_IDLE;
		master = (p_connect->auto_conn[index].tws_role == BTSRV_TWS_MASTER) ? 1 : 0;
		if (index == p_connect->connecting_index) {
			p_connect->connecting_index++;
			p_connect->connecting_index %= BTSRV_SAVE_AUTOCONN_NUM;
			/* As master connect to slave, but disconnect by slave, means slave want to as master,
			 * let some time for slave reconnect master.
			 */
			btsrv_connect_auto_connection_restart((master ? AUTOCONN_MASTER_DELAY : AUTOCONN_START_TIME), 0);
		}
		break;

	case BTSRV_LINK_BASE_GET_NAME:
		if (p_connect->auto_conn[index].state < AUTOCONN_STATE_PROFILE_CONNETING) {
			p_connect->auto_conn[index].state = AUTOCONN_STATE_PROFILE_CONNETING;
		}

		if (index == p_connect->connecting_index) {
#ifdef CONFIG_BT_A2DP_TRS
			if(p_connect->auto_conn[index].trs_mode == 1) {
				if(btsrv_rdm_get_direction(&p_connect->auto_conn[index].addr) == 1) {
					btsrv_connect_auto_connection_restart(AUTOCONN_PRIFILE_DELAY, 0);
				} else {
					btsrv_connect_auto_connection_restart(AUTOCONN_START_TIME, 0);
				}
                break;
            }
#endif
			btsrv_connect_auto_connection_restart(AUTOCONN_START_TIME, 0);
		}
		break;

	case BTSRV_LINK_BASE_CONNECTED_TIMEOUT:
	case BTSRV_LINK_BASE_CONNECTED_FAILED:
		if (p_connect->auto_conn[index].state == AUTOCONN_STATE_BASE_CONNECTING &&
			index == p_connect->connecting_index) {
			p_connect->auto_conn[index].base_try--;
			p_connect->auto_conn[index].state = AUTOCONN_STATE_IDLE;
			if (p_connect->auto_conn[index].base_try == 0) {
				p_connect->auto_conn[index].addr_valid = 0;
			}

			if (p_connect->auto_conn[index].base_try == 0 ||
				p_connect->auto_conn[index].strategy == BTSRV_AUTOCONN_ALTERNATE) {
				/* Try next device */
				p_connect->connecting_index++;
				p_connect->connecting_index %= BTSRV_SAVE_AUTOCONN_NUM;
			}

			/* Connect failed event, just let timeout do next process */
			if (event == BTSRV_LINK_BASE_CONNECTED_TIMEOUT) {
				btsrv_connect_auto_connection_restart(AUTOCONN_START_TIME, 0);
			}
		}
		break;

	case BTSRV_LINK_HFP_CONNECTED:
	case BTSRV_LINK_A2DP_CONNECTED:
	case BTSRV_LINK_AVRCP_CONNECTED:
	case BTSRV_LINK_HID_CONNECTED:
		if (p_connect->auto_conn[index].state == AUTOCONN_STATE_PROFILE_CONNETING &&
			index == p_connect->connecting_index) {
			p_connect->auto_conn[index].profile_try++;
			btsrv_connect_auto_connection_restart(AUTOCONN_START_TIME, 0);
		}
		break;

	case BTSRV_LINK_HFP_DISCONNECTED:
	case BTSRV_LINK_A2DP_DISCONNECTED:
	case BTSRV_LINK_AVRCP_DISCONNECTED:
	case BTSRV_LINK_HID_DISCONNECTED:
		/* What todo!!! */
		break;
	default:
		break;
	}
}

static void btsrv_autoconn_check_clear_auto_info(uint8_t clear_type)
{
	uint8_t i;
	char addr[BT_ADDR_STR_LEN];

	for (i = 0; i < BTSRV_SAVE_AUTOCONN_NUM; i++) {
		if (p_connect->auto_conn[i].addr_valid) {
			if ((clear_type == AUTOCONN_RECONNECT_CLEAR_PHONE) &&
				(p_connect->auto_conn[i].tws_role != BTSRV_TWS_NONE)) {
				continue;
			} else if ((clear_type == AUTOCONN_RECONNECT_CLEAR_TWS) &&
				(p_connect->auto_conn[i].tws_role == BTSRV_TWS_NONE)) {
				continue;
			}

#ifdef CONFIG_BT_A2DP_TRS
			if ((clear_type == AUTOCONN_RECONNECT_CLEAR_PHONE) &&
				(p_connect->auto_conn[i].trs_mode != 0)) {
				continue;
			}
#endif
			if (btsrv_rdm_find_conn_by_addr(&p_connect->auto_conn[i].addr) == NULL) {
				/* Not tws slave or one of the two connect phone */
				hostif_bt_addr_to_str((const bt_addr_t *)&p_connect->auto_conn[i].addr, addr, BT_ADDR_STR_LEN);
				SYS_LOG_INF("BAC clear_auto_info %s", addr);
				memset(&p_connect->auto_conn[i], 0, sizeof(struct auto_conn_t));
			}
		}
	}
}

static void btsrv_autoconn_idle_proc(void)
{
	char addr[BT_ADDR_STR_LEN];
	struct auto_conn_t *auto_conn;
	int32_t next_time = 0;
	uint8_t index;

	if ((btsrv_rdm_get_dev_role() == BTSRV_TWS_SLAVE) ||
		(btsrv_rdm_get_connected_dev(NULL, NULL) == btsrv_max_conn_num())) {
		btsrv_autoconn_check_clear_auto_info(AUTOCONN_RECONNECT_CLEAR_ALL);
	}

	if (btsrv_rdm_get_dev_role() == BTSRV_TWS_MASTER) {
		btsrv_autoconn_check_clear_auto_info(AUTOCONN_RECONNECT_CLEAR_TWS);
	}

	if ((btsrv_rdm_get_dev_role() == BTSRV_TWS_NONE) &&
#ifdef CONFIG_BT_A2DP_TRS
		(btsrv_rdm_get_dev_trs_mode() == 0) &&
#endif
		(btsrv_rdm_get_connected_dev(NULL, NULL) == btsrv_max_phone_num())) {
		btsrv_autoconn_check_clear_auto_info(AUTOCONN_RECONNECT_CLEAR_PHONE);
	}

	auto_conn = btsrv_autoconn_find_wait_reconn();
	if (auto_conn == NULL) {
		SYS_LOG_INF("BAC auto connect finished");
		btsrv_connect_auto_connection_stop();
		return;
	}

	index = p_connect->connecting_index;
	if (btsrv_rdm_find_conn_by_addr(&auto_conn->addr) == NULL) {
		hostif_bt_addr_to_str((const bt_addr_t *)&auto_conn->addr, addr, BT_ADDR_STR_LEN);
		SYS_LOG_INF("BAC auto_connect: %s tws_role %d, try %d", addr, auto_conn->tws_role, auto_conn->base_try);

		if (auto_conn->tws_role == BTSRV_TWS_NONE) {
			p_connect->auto_conn[index].state = AUTOCONN_STATE_BASE_CONNECTING;
			btsrv_scan_update_mode(true);
			if (auto_conn->trs_mode) {
				btsrv_adapter_connect(&auto_conn->addr, BT_BR_CONN_PARAM_DEFAULT);
			} else {
				btsrv_adapter_connect(&auto_conn->addr, BT_BR_CONN_PARAM_NAC_LINKKEY);
			}
			next_time = auto_conn->base_interval;
		} else {
			p_connect->auto_conn[index].state = AUTOCONN_STATE_DELAY_CONNECTING;
			btsrv_scan_update_mode(true);
			next_time = bt_rand32_get()%AUTOCONN_DELAY_TIME_MASK;
			SYS_LOG_INF("BAC Delay time %d", next_time);
		}
	} else {
		SYS_LOG_INF("BAC idle_proc state %d", p_connect->auto_conn[index].state);
		if (p_connect->auto_conn[index].state < AUTOCONN_STATE_BASE_CONNECTED) {
			p_connect->auto_conn[index].state = AUTOCONN_STATE_BASE_CONNECTED;
#ifdef CONFIG_BT_A2DP_TRS
			btsrv_rdm_set_trs_mode(&p_connect->auto_conn[index].addr, p_connect->auto_conn[index].trs_mode);
#endif
		}
		btsrv_scan_update_mode(true);
		next_time = AUTOCONN_START_TIME;
	}

	btsrv_connect_auto_connection_restart(next_time, 0);
}

static void btsrv_autoconn_delay_connecting_proc(void)
{
	struct auto_conn_t *auto_conn = &p_connect->auto_conn[p_connect->connecting_index];
	int32_t next_time = 0;

	if (!auto_conn->addr_valid) {
		auto_conn->state = AUTOCONN_STATE_IDLE;
		btsrv_connect_auto_connection_restart(AUTOCONN_START_TIME, 0);
		return;
	}

	if (btsrv_rdm_find_conn_by_addr(&auto_conn->addr) == NULL) {
		auto_conn->state = AUTOCONN_STATE_BASE_CONNECTING;
		btsrv_scan_update_mode(true);
	#ifdef CONFIG_SUPPORT_TWS
		/* Base connect do reconnect, no need tws module try more times, just set try_times to 0 */
		btsrv_tws_connect_to((bt_addr_t *)&auto_conn->addr, 0, auto_conn->tws_role);
	#endif
		next_time = auto_conn->base_interval;
	} else {
		SYS_LOG_INF("BAC connecting_proc state %d", auto_conn->state);
		if (auto_conn->state < AUTOCONN_STATE_BASE_CONNECTED) {
			auto_conn->state = AUTOCONN_STATE_BASE_CONNECTED;
#ifdef CONFIG_BT_A2DP_TRS
			btsrv_rdm_set_trs_mode(&auto_conn->addr, auto_conn->trs_mode);
#endif
		}
		btsrv_scan_update_mode(true);
		next_time = AUTOCONN_START_TIME;
	}

	btsrv_connect_auto_connection_restart(next_time, 0);
}

static void btsrv_autoconn_base_connecting_proc(void)
{
	/* Base connect timeout */
	btsrv_proc_link_change(p_connect->auto_conn[p_connect->connecting_index].addr.val,
								BTSRV_LINK_BASE_CONNECTED_TIMEOUT);
}

static void btsrv_autoconn_base_connected_proc(void)
{
	uint8_t index = p_connect->connecting_index;

	/* In base connected state, process get name,
	 * after get name, trigger enter AUTOCONN_STATE_PROFILE_CONNETING
	 */

	p_connect->auto_conn[index].base_try--;
	if (p_connect->auto_conn[index].base_try == 0) {
		p_connect->auto_conn[index].addr_valid = 0;
		p_connect->auto_conn[index].state = AUTOCONN_STATE_IDLE;
		p_connect->connecting_index++;
		p_connect->connecting_index %= BTSRV_SAVE_AUTOCONN_NUM;
		btsrv_connect_auto_connection_restart(AUTOCONN_START_TIME, 0);
	} else {
		btsrv_connect_auto_connection_restart(p_connect->auto_conn[index].base_interval, 0);
	}
}

static int btsrv_autoconn_check_connect_profile(struct bt_conn *conn, struct auto_conn_t *auto_conn)
{
	int ret, i;
	uint8_t next_connect_profile;
	bool hfp_connected = btsrv_rdm_is_hfp_connected(conn);
	bool a2dp_connected = btsrv_rdm_is_a2dp_connected(conn);
	bool avrcp_connected = btsrv_rdm_is_avrcp_connected(conn);
	bool hid_connected = btsrv_rdm_is_hid_connected(conn);

	if ((auto_conn->hfp && !hfp_connected) ||
		(auto_conn->a2dp && !a2dp_connected) ||
		(auto_conn->avrcp && !avrcp_connected)||
		(auto_conn->hid && !hid_connected)) {
		/* Still have profile to connect */
		ret = 0;
	} else {
		/* Try other device */
		ret = 1;
		goto exit_check;
	}

	if (auto_conn->tws_role != BTSRV_TWS_NONE) {
		/* TWS a2dp/avrcp connect by master in base connected callback,
		 * here just check a2dp/avrcp is connected and try other device.
		 */
		goto exit_check;
	}

	SYS_LOG_INF("BAC curr:%d%d, hfp:%d%d, a2dp:%d%d, avrcp:%d%d hid:%d%d", auto_conn->curr_connect_profile, auto_conn->hfp_first,
			auto_conn->hfp, hfp_connected, auto_conn->a2dp, a2dp_connected, auto_conn->avrcp, avrcp_connected
			, auto_conn->hid, hid_connected);

	if (auto_conn->curr_connect_profile == AUTOCONN_PROFILE_IDLE) {
		if (auto_conn->hfp_first) {
			next_connect_profile = AUTOCONN_PROFILE_HFP_CONNECTING;
		} else {
#ifdef CONFIG_BT_A2DP
            next_connect_profile = AUTOCONN_PROFILE_A2DP_CONNECTING;
#else
			next_connect_profile = auto_conn->curr_connect_profile + 1;
#endif
		}
	} else {
		next_connect_profile = auto_conn->curr_connect_profile + 1;
#ifndef CONFIG_BT_A2DP
		if(next_connect_profile == AUTOCONN_PROFILE_A2DP_CONNECTING){
			next_connect_profile += 1;
		}
#endif
	}

	for (i = (AUTOCONN_PROFILE_IDLE+1); i < AUTOCONN_PROFILE_CONNECTING_MAX; i++) {
		if (next_connect_profile >= AUTOCONN_PROFILE_CONNECTING_MAX) {
			next_connect_profile = AUTOCONN_PROFILE_HFP_CONNECTING;
		}

		if ((next_connect_profile == AUTOCONN_PROFILE_HFP_CONNECTING) &&
			auto_conn->hfp && !hfp_connected) {
			break;
		}

#ifdef CONFIG_BT_A2DP
		else if ((next_connect_profile == AUTOCONN_PROFILE_A2DP_CONNECTING) &&
					auto_conn->a2dp && !a2dp_connected) {
            break;
		}
#endif

#ifdef CONFIG_BT_AVRCP
		else if ((next_connect_profile == AUTOCONN_PROFILE_AVRCP_CONNECTING) &&
					auto_conn->avrcp && !avrcp_connected) {
					btsrv_avrcp_connect(conn);
#ifdef CONFIG_BT_A2DP_TRS
			if(btsrv_rdm_get_trs_mode(conn) == 1 && a2dp_connected == 0) {
				/* For trs,  only connect avrcp after a2dp connected */
				next_connect_profile++;
				continue;
			}
#endif
			break;
		}
#endif
        else if ((next_connect_profile == AUTOCONN_PROFILE_HID_CONNECTING) &&
					auto_conn->hid && !hid_connected) {
			break;
		}

		next_connect_profile++;
	}

	if (next_connect_profile == AUTOCONN_PROFILE_HFP_CONNECTING) {
#ifdef CONFIG_BT_A2DP_TRS
		if(btsrv_rdm_get_trs_mode(conn) == 1) {
#ifdef CONFIG_BT_HFP_AG
			btsrv_hfp_ag_connect(conn);
#else
			SYS_LOG_ERR("Hfp AG not open");
#endif
		} else {
			btsrv_hfp_connect(conn);
		}
#else
		btsrv_hfp_connect(conn);
#endif
	}

#ifdef CONFIG_BT_A2DP
	else if (next_connect_profile == AUTOCONN_PROFILE_A2DP_CONNECTING) {
#ifdef CONFIG_BT_A2DP_TRS
		if(btsrv_rdm_get_trs_mode(conn) == 1) {
			btsrv_a2dp_connect(conn, BT_A2DP_CH_SOURCE);
		} else {
			btsrv_a2dp_connect(conn, BT_A2DP_CH_SINK);
		}
#else
		btsrv_a2dp_connect(conn, BT_A2DP_CH_SINK);
#endif
	} 
#endif

#ifdef CONFIG_BT_AVRCP
    else if (next_connect_profile == AUTOCONN_PROFILE_AVRCP_CONNECTING) {
		btsrv_avrcp_connect(conn);
	}
#endif

	else if (next_connect_profile == AUTOCONN_PROFILE_HID_CONNECTING) {
		btsrv_hid_connect(conn);
	}

	auto_conn->curr_connect_profile = next_connect_profile;
exit_check:
	return ret;
}

static void btsrv_autoconn_profile_connecting_proc(void)
{
	uint8_t index = p_connect->connecting_index;
	struct auto_conn_t *auto_conn = &p_connect->auto_conn[index];
	struct bt_conn *conn;

	conn = btsrv_rdm_find_conn_by_addr(&auto_conn->addr);
	if (conn == NULL) {
		SYS_LOG_ERR("BAC connecting_proc need to fix!!!");
		return;
	}

	if (auto_conn->profile_try == 0) {
		SYS_LOG_WRN("BAC Failed to connect %d(%d), %d(%d), %d(%d), %d(%d)",
					auto_conn->hfp, btsrv_rdm_is_hfp_connected(conn),
					auto_conn->a2dp, btsrv_rdm_is_a2dp_connected(conn),
					auto_conn->avrcp, btsrv_rdm_is_avrcp_connected(conn),
					auto_conn->hid, btsrv_rdm_is_hid_connected(conn));
		/* I have linkkey but phone clear linkkey, when do reconnect,
		 * phone will arise one connect notify, but phone do nothing,
		 * then profile will not connect, need active do disconnect.
		 * Better TODO: host check this case, notify upper layer
		 *                          clear linkkey and auto connect info.
		 */
		if (!btsrv_rdm_is_hfp_connected(conn) &&
			!btsrv_rdm_is_a2dp_connected(conn) &&
			!btsrv_rdm_is_avrcp_connected(conn) &&
			!btsrv_rdm_is_spp_connected(conn)  &&
			!btsrv_rdm_is_hid_connected(conn)) {
			btsrv_adapter_disconnect(conn);
		}
		goto try_other_dev;
	}
	auto_conn->profile_try--;

	if (btsrv_autoconn_check_connect_profile(conn, auto_conn)) {
		goto try_other_dev;
	}

	btsrv_connect_auto_connection_restart(auto_conn->profile_interval, 0);
	return;

try_other_dev:
	p_connect->auto_conn[index].addr_valid = 0;
	p_connect->auto_conn[index].state = AUTOCONN_STATE_IDLE;
	p_connect->connecting_index++;
	p_connect->connecting_index %= BTSRV_SAVE_AUTOCONN_NUM;
	btsrv_connect_auto_connection_restart(AUTOCONN_START_TIME, 0);
}

static void btsrv_autoconn_timer_handler(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	uint8_t state, index;

	p_connect->auto_connect_running = 1;
	index = p_connect->connecting_index;
	if (p_connect->auto_conn[index].addr_valid) {
		state = p_connect->auto_conn[index].state;
	} else {
		state = AUTOCONN_STATE_IDLE;
	}
	SYS_LOG_INF("BAC autoconn index %d, state: %d", index, state);

	switch (state) {
	case AUTOCONN_STATE_IDLE:
		btsrv_autoconn_idle_proc();
		break;
	case AUTOCONN_STATE_DELAY_CONNECTING:
		btsrv_autoconn_delay_connecting_proc();
		break;
	case AUTOCONN_STATE_BASE_CONNECTING:
		btsrv_autoconn_base_connecting_proc();
		break;
	case AUTOCONN_STATE_BASE_CONNECTED:
		btsrv_autoconn_base_connected_proc();
		break;
	case AUTOCONN_STATE_PROFILE_CONNETING:
		btsrv_autoconn_profile_connecting_proc();
		break;
	default:
		btsrv_connect_auto_connection_stop();
		break;
	}

	p_connect->auto_connect_running = 0;
}

static void btsrv_monitor_timer_handler(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	int i, stop_timer = 1;
	struct bt_conn *conn;

	p_connect->monitor_timer_running = 1;

	for (i = 0; i < BTSRV_SAVE_AUTOCONN_NUM; i++) {
		if (p_connect->monitor_conn[i].valid) {
			conn = btsrv_rdm_find_conn_by_addr(&p_connect->monitor_conn[i].addr);
			if (conn == NULL) {
				p_connect->monitor_conn[i].valid = 0;
				continue;
			}

			/* Now only monitor avrcp after a2dp connected,
			 * some phone active connect a2dp but not connect avrcp
			 */
#ifdef CONFIG_BT_A2DP
			if (btsrv_rdm_is_a2dp_connected(conn)){
#endif

#ifdef CONFIG_BT_AVRCP
				if(btsrv_rdm_is_avrcp_connected(conn)) {
					p_connect->monitor_conn[i].valid = 0;
					continue;
				}
                else {
                    if(p_connect->monitor_conn[i].avrcp_times) {
                        p_connect->monitor_conn[i].avrcp_times--;
                    }
                    if ((p_connect->monitor_conn[i].avrcp_times%MONITOR_AVRCP_CONNECT_INTERVAL) == 0){
                        SYS_LOG_INF("BAC Do avrcp connect");
                        btsrv_avrcp_connect(conn);
                    }
                    if(p_connect->monitor_conn[i].avrcp_times == 0) {
                        p_connect->monitor_conn[i].valid = 0;
                        continue;
                    }
                }
#endif

#ifdef CONFIG_BT_A2DP
			} 
            else {
				/* Just find hongmi2A not do a2dp connect when connect from phone.
				 * if need add this active connect ???
				 */
				if (p_connect->monitor_conn[i].a2dp_times) {
					p_connect->monitor_conn[i].a2dp_times--;
					if (p_connect->monitor_conn[i].a2dp_times == 0) {
						SYS_LOG_INF("BAC Phone not do a2dp connect");
						if (bt_register_a2dp_status_get()) {
 #ifdef CONFIG_BT_A2DP_TRS
							if(btsrv_rdm_get_trs_mode(conn) == 1) {
								btsrv_a2dp_connect(conn, BT_A2DP_CH_SOURCE);
							} else {
								btsrv_a2dp_connect(conn, BT_A2DP_CH_SINK);
							}
#else
							btsrv_a2dp_connect(conn, BT_A2DP_CH_SINK);
#endif
						}
					}
				}
			}
#endif
			stop_timer = 0;
		}
	}

	p_connect->monitor_timer_running = 0;
	if (stop_timer) {
		btsrv_connect_monitor_profile_stop();
	}
}

static void btsrv_add_monitor(struct bt_conn *conn)
{
	int i, index = BTSRV_SAVE_AUTOCONN_NUM;
	bd_address_t *addr = (bd_address_t *)GET_CONN_BT_ADDR(conn);

	if (btsrv_is_pts_test()) {
		return;
	}

	for (i = 0; i < BTSRV_SAVE_AUTOCONN_NUM; i++) {
		if (p_connect->auto_conn[i].addr_valid &&
			!memcmp(p_connect->auto_conn[i].addr.val, addr->val, sizeof(bd_address_t))) {
			/* Reconnect device, not need monitor profile connect */
			return;
		}
	}

	for (i = 0; i < BTSRV_SAVE_AUTOCONN_NUM; i++) {
		if (!p_connect->monitor_conn[i].valid && index == BTSRV_SAVE_AUTOCONN_NUM) {
			index = i;
		}

		if (p_connect->monitor_conn[i].valid &&
			!memcmp(p_connect->monitor_conn[i].addr.val, addr->val, sizeof(bd_address_t))) {
			/* Already in monitor */
			return;
		}
	}

	if (index == BTSRV_SAVE_AUTOCONN_NUM) {
		return;
	}

	SYS_LOG_INF("BAC Add profile monitor");

	memcpy(p_connect->monitor_conn[index].addr.val, addr->val, sizeof(bd_address_t));
	p_connect->monitor_conn[index].valid = 1;
	p_connect->monitor_conn[index].a2dp_times = MONITOR_A2DP_TIMES;
	p_connect->monitor_conn[index].avrcp_times = MONITOR_AVRCP_TIMES;
	btsrv_connect_monitor_profile_start(MONITOR_PRIFILE_INTERVEL, MONITOR_PRIFILE_INTERVEL);
}

static void btsrv_update_monitor(uint8_t *addr, uint8_t type)
{
	int i, index = BTSRV_SAVE_AUTOCONN_NUM;

	for (i = 0; i < BTSRV_SAVE_AUTOCONN_NUM; i++) {
		if (p_connect->monitor_conn[i].valid &&
			!memcmp(p_connect->monitor_conn[i].addr.val, addr, sizeof(bd_address_t))) {
			index = i;
			break;
		}
	}

	if (index == BTSRV_SAVE_AUTOCONN_NUM) {
		return;
	}

	/* Current only monitor avrcp */
	switch (type) {
	case BTSRV_LINK_AVRCP_CONNECTED:
	case BTSRV_LINK_BASE_DISCONNECTED:
		p_connect->monitor_conn[index].valid = 0;
		break;
	}
}

static void btsrv_proc_link_change(uint8_t *mac, uint8_t type)
{
	uint8_t need_update = 0;

	switch (type) {
	case BTSRV_LINK_BASE_GET_NAME:
	case BTSRV_LINK_BASE_CONNECTED_TIMEOUT:
		btsrv_update_autoconn_state(mac, type);
		break;

	case BTSRV_LINK_BASE_CONNECTED:
		btsrv_update_autoconn_state(mac, type);
		btsrv_scan_update_mode(true);
		break;

	case BTSRV_LINK_BASE_CONNECTED_FAILED:
	case BTSRV_LINK_BASE_DISCONNECTED:
	case BTSRV_LINK_HFP_DISCONNECTED:
	case BTSRV_LINK_A2DP_DISCONNECTED:
	case BTSRV_LINK_AVRCP_DISCONNECTED:
	case BTSRV_LINK_HID_DISCONNECTED:
		btsrv_rdm_remove_dev(mac);
		btsrv_update_autoconn_state(mac, type);
		btsrv_update_monitor(mac, type);
		btsrv_scan_update_mode(false);
		if ((type == BTSRV_LINK_HFP_DISCONNECTED) ||
			(type == BTSRV_LINK_A2DP_DISCONNECTED)||
			(type == BTSRV_LINK_HID_DISCONNECTED)) {
			need_update = 1;
		}
		break;

	case BTSRV_LINK_HFP_CONNECTED:
	case BTSRV_LINK_A2DP_CONNECTED:
	case BTSRV_LINK_AVRCP_CONNECTED:
	case BTSRV_LINK_HID_CONNECTED:
		btsrv_update_autoconn_state(mac, type);
		btsrv_update_monitor(mac, type);
		need_update = 1;
		break;

	case BTSRV_LINK_SPP_DISCONNECTED:
	case BTSRV_LINK_PBAP_DISCONNECTED:
		btsrv_rdm_remove_dev(mac);
		btsrv_scan_update_mode(false);
		break;

	case BTSRV_LINK_SPP_CONNECTED:
	case BTSRV_LINK_PBAP_CONNECTED:
	default:
		break;
	}

	if (need_update) {
		btsrv_autoconn_info_update();
	}

	btsrv_update_performance_req();
}

static void btsrv_notify_link_event(struct bt_conn *base_conn, uint8_t event, uint8_t param)
{
	struct bt_link_cb_param cb_param;

	memset(&cb_param, 0, sizeof(struct bt_link_cb_param));
	cb_param.link_event = event;
	cb_param.hdl = hostif_bt_conn_get_handle(base_conn);
	cb_param.addr = (bd_address_t *)GET_CONN_BT_ADDR(base_conn);

	switch (event) {
	case BT_LINK_EV_GET_NAME:
		cb_param.name = btsrv_rdm_get_dev_name(base_conn);
		cb_param.is_tws = param ? 1 : 0;
#ifdef CONFIG_BT_A2DP_TRS
		cb_param.is_trs = btsrv_rdm_get_trs_mode(base_conn);
#endif
		break;
	case BT_LINK_EV_ACL_DISCONNECTED:
		cb_param.reason = param;
		break;
#ifdef CONFIG_BT_A2DP_TRS
	case BT_LINK_EV_A2DP_CONNECTED:
	case BT_LINK_EV_HF_CONNECTED:
		cb_param.is_trs = btsrv_rdm_get_trs_mode(base_conn);
		break;
#endif
	default:
		break;
	}

	btsrv_sniff_update_idle_time(base_conn);
	btsrv_adapter_callback(BTSRV_LINK_EVENT, &cb_param);
}

static int btsrv_connect_connected(struct bt_conn *base_conn)
{
	int ret = 0;
	bd_address_t *addr = (bd_address_t *)GET_CONN_BT_ADDR(base_conn);

	ret = btsrv_rdm_add_dev(base_conn);
	if (ret < 0) {
		SYS_LOG_ERR("BAC Should not run to here!!!!");
		return ret;
	}

	btsrv_proc_link_change(addr->val, BTSRV_LINK_BASE_CONNECTED);

#ifdef CONFIG_BT_PNP_INFO_SEARCH
	SYS_LOG_INF("pnp info search req.\n");
	btsrv_pnp_info_search(base_conn);
#endif

	btsrv_notify_link_event(base_conn, BT_LINK_EV_ACL_CONNECTED, 0);
	return ret;
}

static void btsrv_connect_security_changed(struct bt_conn *base_conn)
{
	if (btsrv_rdm_get_conn_role(base_conn) == BTSRV_TWS_NONE &&
		!btsrv_rdm_is_security_changed(base_conn)) {
		btsrv_rdm_set_security_changed(base_conn);
#ifdef CONFIG_BT_A2DP
		btsrv_add_monitor(base_conn);
#endif
	}
}

static int btsrv_connect_disconnected(struct bt_conn *base_conn, uint8_t reason)
{
	int role;
	struct bt_disconnect_reason bt_disparam;
	bd_address_t *addr = (bd_address_t *)GET_CONN_BT_ADDR(base_conn);

	role = btsrv_rdm_get_conn_role(base_conn);
	if (role == BTSRV_TWS_NONE) {
		/* Connected, but still not change to tws pending or tws role,
		 * receive disconnect, need notify tws exit connecting state.
		 */
		btsrv_event_notify_malloc(MSG_BTSRV_TWS, MSG_BTSRV_TWS_DISCONNECTED_ADDR, (uint8_t *)addr, sizeof(bd_address_t), 0);
	} else if (role > BTSRV_TWS_NONE) {
		btsrv_event_notify_ext(MSG_BTSRV_TWS, MSG_BTSRV_TWS_DISCONNECTED, base_conn, reason);
	}

	memcpy(&bt_disparam.addr, addr, sizeof(bd_address_t));
	bt_disparam.reason = reason;
	bt_disparam.tws_role = role;
	btsrv_event_notify_malloc(MSG_BTSRV_BASE, MSG_BTSRV_DISCONNECTED_REASON, (uint8_t *)&bt_disparam, sizeof(bt_disparam), 0);

	btsrv_notify_link_event(base_conn, BT_LINK_EV_ACL_DISCONNECTED, reason);
	btsrv_rdm_base_disconnected(base_conn);
	btsrv_proc_link_change(addr->val, BTSRV_LINK_BASE_DISCONNECTED);

	if (p_connect->clear_list_disconnecting) {
		if (btsrv_rdm_get_connected_dev_cnt_by_type(p_connect->clear_list_mode) == 0) {
			p_connect->clear_list_disconnecting = 0;
			btsrv_event_notify(MSG_BTSRV_CONNECT, MGS_BTSRV_CLEAR_AUTO_INFO, NULL);
			SYS_LOG_INF("BAC clear list finish %d",p_connect->clear_list_mode);
		}
	}

#ifdef CONFIG_BT_A2DP_TRS
	if (p_connect->clear_trs_dev_conn == base_conn) {
		p_connect->clear_trs_dev_conn = NULL;
		btsrv_event_notify_malloc(MSG_BTSRV_CONNECT, MSG_BTSRV_CLEAR_PAIRED_INFO, (uint8_t *)&bt_disparam.addr, sizeof(bt_disparam.addr), 0);
	}
#endif
	return 0;
}

/* Be careful, it can work well when connect avrcp conflict with phone ??? */
static void btsrv_connect_quick_connect_avrcp(struct bt_conn *base_conn)
{
	int i;
	bd_address_t *addr = (bd_address_t *)GET_CONN_BT_ADDR(base_conn);

	if (btsrv_rdm_is_avrcp_connected(base_conn)) {
		return;
	}

	for (i = 0; i < BTSRV_SAVE_AUTOCONN_NUM; i++) {
		if (p_connect->auto_conn[i].addr_valid &&
			!memcmp(p_connect->auto_conn[i].addr.val, addr->val, sizeof(bd_address_t))) {
			/* Reconnect device, reconnect will process avrcp connect */
			return;
		}
	}

	SYS_LOG_INF("BAC quick connect avrcp\n");
	btsrv_avrcp_connect(base_conn);
}

#ifdef CONFIG_BT_A2DP_TRS
static void btsrv_update_trs_dev_record(bd_address_t *addr,uint8_t *name)
{
	int i, index = (RECORD_TRS_DEV_MAX_NUM - 1);

	for (i = 0; i < RECORD_TRS_DEV_MAX_NUM; i++) {
		if (!memcmp(addr, &p_connect->paired_dev_list[i].addr, sizeof(bd_address_t))) {
			index = i;
			break;
		}
	}

	if (index == 0) {
		return;
	}

	for (i = index; i > 0; i--) {
		memcpy(&p_connect->paired_dev_list[i], &p_connect->paired_dev_list[i - 1], sizeof(struct bt_paired_list_dev_t));
	}

	memcpy(&p_connect->paired_dev_list[0].addr, addr, sizeof(bd_address_t));
	memcpy(&p_connect->paired_dev_list[0].name, name, CONFIG_MAX_BT_NAME_LEN + 1);
    p_connect->paired_dev_list[0].valid = 1;
	btsrv_property_set(RECORD_A2DP_TRS_DEV, (char *)p_connect->paired_dev_list,
						sizeof(p_connect->paired_dev_list));
}

bool btsrv_is_record_trs_dev(bd_address_t *addr)
{
	int i;

	for (i = 0; i < RECORD_TRS_DEV_MAX_NUM; i++) {
		if (!memcmp(addr, &p_connect->paired_dev_list[i].addr, sizeof(bd_address_t))) {
			return true;
		}
	}
	return false;
}

uint8_t btsrv_get_trs_dev_record(struct bt_paired_list_dev_t *record, uint8_t max)
{
	uint8_t i, index = 0;
    uint8_t limit = MIN(max, RECORD_TRS_DEV_MAX_NUM);

    for (i = 0; i < RECORD_TRS_DEV_MAX_NUM; i++) {
        if(p_connect->paired_dev_list[i].valid){
            memcpy(record, &p_connect->paired_dev_list[i], sizeof(struct bt_paired_list_dev_t));
            index++;
            record++;
            if(index == limit){
                break;
            }
        }
    }
    return index;
}

/* return 0: continue a2dp connected process, other: exit a2dp connected process */
static int btsrv_connect_check_trs_a2dp_connected(struct bt_conn *base_conn)
{
	bd_address_t *addr;
    uint8_t *name;
	if ((btsrv_rdm_get_conn_role(base_conn) == BTSRV_TWS_NONE) &&
		(hostif_bt_a2dp_get_a2dp_role(base_conn) == BT_A2DP_CH_SOURCE)) {
		if (btsrv_rdm_get_trs_mode(base_conn)) {
			addr = (bd_address_t *)GET_CONN_BT_ADDR(base_conn);
            name = btsrv_rdm_get_dev_name(base_conn);
			btsrv_update_trs_dev_record(addr,name);
			btsrv_scan_update_mode(false);
		} else {
			SYS_LOG_INF("BAC Trs device not in record, do disconnect");
			btsrv_adapter_disconnect(base_conn);
			return -ENODEV;
		}
	}

	return 0;
}

static void btsrv_trs_autoconn_paired_info_clear(bd_address_t *addr)
{
	for (int i = 0; i < BTSRV_SAVE_AUTOCONN_NUM; i++) {
		if (!memcmp(addr,&(p_connect->nvram_reconn_info[i].addr),sizeof(bd_address_t))){
			memset(&p_connect->nvram_reconn_info[i], 0, sizeof(struct autoconn_info));
			btsrv_update_nvram_auto_conn_info();
			break;
		}
	}
}

static void btsrv_clear_trs_dev_record(bd_address_t *addr)
{
    struct bt_conn *base_conn = btsrv_rdm_find_conn_by_addr(addr);
    uint8_t i, index = 0, last_valid = 0xFF;

    if (!addr) {
        return;
    }

    if (base_conn) {
        btsrv_adapter_disconnect(base_conn);
        p_connect->clear_trs_dev_conn = base_conn;
        return;
    }
	p_connect->clear_trs_dev_conn = NULL;

    btsrv_trs_autoconn_paired_info_clear(addr);
    hostif_bt_store_clear_linkkey((const bt_addr_t *)addr);
    for (i = 0; i < RECORD_TRS_DEV_MAX_NUM; i++) {
        if (!memcmp(addr, &(p_connect->paired_dev_list[i].addr), sizeof(bd_address_t))) {
            index = i;
            break;
        }
    }
    if (index == RECORD_TRS_DEV_MAX_NUM) {
        return;
    }

    memset(&(p_connect->paired_dev_list[index]), 0, sizeof(struct bt_paired_list_dev_t));
    for (i = index; i < RECORD_TRS_DEV_MAX_NUM - 1; i++) {
        if (p_connect->paired_dev_list[i + 1].valid) {
            memcpy(&p_connect->paired_dev_list[i], &p_connect->paired_dev_list[i + 1], sizeof(struct bt_paired_list_dev_t));
            last_valid = i + 1;
        }
    }

    if (last_valid != 0xFF) {
        memset(&(p_connect->paired_dev_list[last_valid]), 0, sizeof(struct bt_paired_list_dev_t));
    }
    btsrv_property_set(RECORD_A2DP_TRS_DEV, (char *)p_connect->paired_dev_list,sizeof(p_connect->paired_dev_list));

	SYS_LOG_INF("BAC clr trs index %d addr %02x:%02x:%02x:%02x:%02x:%02x",
				i, addr->val[5], addr->val[4], addr->val[3],
				addr->val[2], addr->val[1],addr->val[0]);
}
#endif

static int btsrv_connect_a2dp_connected(struct bt_conn *base_conn)
{
	bd_address_t *addr = (bd_address_t *)GET_CONN_BT_ADDR(base_conn);

	btsrv_rdm_set_a2dp_connected(base_conn, true);
	btsrv_notify_link_event(base_conn, BT_LINK_EV_A2DP_CONNECTED, 0);
	btsrv_proc_link_change(addr->val, BTSRV_LINK_A2DP_CONNECTED);

	/* Same phone not connect avrcp when close/open bluetooth media */
	if (btsrv_rdm_get_conn_role(base_conn) == BTSRV_TWS_NONE) {
		btsrv_connect_quick_connect_avrcp(base_conn);
		btsrv_add_monitor(base_conn);
	}
	return 0;
}

static int btsrv_connect_a2dp_disconnected(struct bt_conn *base_conn, bool need_re_connect)
{
	bd_address_t *addr = (bd_address_t *)GET_CONN_BT_ADDR(base_conn);

	btsrv_rdm_a2dp_actived(base_conn, 0);
	btsrv_rdm_set_a2dp_connected(base_conn, false);
	btsrv_notify_link_event(base_conn, BT_LINK_EV_A2DP_DISCONNECTED, 0);
	btsrv_proc_link_change(addr->val, BTSRV_LINK_A2DP_DISCONNECTED);

#ifdef CONFIG_BT_A2DP_TRS
	if (btsrv_rdm_get_trs_mode(base_conn)) {
		btsrv_rdm_trs_a2dp_stream_open(base_conn, false);
	}
#endif

	if (need_re_connect) {
		/* Wait todo: reconnect profile, if phone active disconnect, why need reconnect? */
	}
	return 0;
}

static int btsrv_connect_avrcp_connected(struct bt_conn *base_conn)
{
	bd_address_t *addr = (bd_address_t *)GET_CONN_BT_ADDR(base_conn);

	btsrv_rdm_set_avrcp_connected(base_conn, true);
	btsrv_notify_link_event(base_conn, BT_LINK_EV_AVRCP_CONNECTED, 0);
	btsrv_proc_link_change(addr->val, BTSRV_LINK_AVRCP_CONNECTED);
	return 0;
}

static int btsrv_connect_avrcp_disconnected(struct bt_conn *base_conn, bool need_re_connect)
{
	bd_address_t *addr = (bd_address_t *)GET_CONN_BT_ADDR(base_conn);

	btsrv_rdm_set_avrcp_connected(base_conn, false);
	btsrv_notify_link_event(base_conn, BT_LINK_EV_AVRCP_DISCONNECTED, 0);
	btsrv_proc_link_change(addr->val, BTSRV_LINK_AVRCP_DISCONNECTED);

	if (need_re_connect) {
		/* Wait todo: reconnect profile, if phone active disconnect, why need reconnect? */
	}

	return 0;
}

static int btsrv_connect_hfp_connected(struct bt_conn *base_conn)
{
	bd_address_t *addr = (bd_address_t *)GET_CONN_BT_ADDR(base_conn);

	btsrv_rdm_set_hfp_connected(base_conn, true);
	btsrv_notify_link_event(base_conn, BT_LINK_EV_HF_CONNECTED, 0);
	btsrv_proc_link_change(addr->val, BTSRV_LINK_HFP_CONNECTED);
	return 0;
}

static int btsrv_connect_hfp_disconnected(struct bt_conn *base_conn, bool need_re_connect)
{
	bd_address_t *addr = (bd_address_t *)GET_CONN_BT_ADDR(base_conn);

	btsrv_rdm_set_hfp_connected(base_conn, false);
	btsrv_notify_link_event(base_conn, BT_LINK_EV_HF_DISCONNECTED, 0);
	btsrv_proc_link_change(addr->val, BTSRV_LINK_HFP_DISCONNECTED);

	if (need_re_connect) {
		/* Wait todo: reconnect profile, if phone active disconnect, why need reconnect? */
	}

	return 0;
}

static int btsrv_connect_spp_connected(struct bt_conn *base_conn)
{
	bd_address_t *addr = (bd_address_t *)GET_CONN_BT_ADDR(base_conn);

	btsrv_rdm_set_spp_connected(base_conn, true);
	btsrv_notify_link_event(base_conn, BT_LINK_EV_SPP_CONNECTED, 0);
	btsrv_proc_link_change(addr->val, BTSRV_LINK_SPP_CONNECTED);

#if defined(CONFIG_BT_DEVICE_ENCRYPT_FORCE)
#ifdef CONFIG_BT_HFP_HF
	if (!btsrv_rdm_is_hfp_connected(base_conn)) {
		btsrv_hfp_connect(base_conn);
	}
#endif

#ifdef CONFIG_BT_A2DP
	if (bt_register_a2dp_status_get()) {
		if (!btsrv_rdm_is_a2dp_connected(base_conn)) {
			btsrv_a2dp_connect(base_conn, BT_A2DP_CH_SINK);
		}
	}
#endif
#endif

	return 0;
}

static int btsrv_connect_spp_disconnected(struct bt_conn *base_conn)
{
	bd_address_t *addr = (bd_address_t *)GET_CONN_BT_ADDR(base_conn);

	btsrv_rdm_set_spp_connected(base_conn, false);
	btsrv_notify_link_event(base_conn, BT_LINK_EV_SPP_DISCONNECTED, 0);
	btsrv_proc_link_change(addr->val, BTSRV_LINK_SPP_DISCONNECTED);
	return 0;
}

static int btsrv_connect_pbap_connected(struct bt_conn *base_conn)
{
	bd_address_t *addr = (bd_address_t *)GET_CONN_BT_ADDR(base_conn);

	btsrv_rdm_set_pbap_connected(base_conn, true);
	btsrv_proc_link_change(addr->val, BTSRV_LINK_PBAP_CONNECTED);
	return 0;
}

static int btsrv_connect_pbap_disconnected(struct bt_conn *base_conn)
{
	bd_address_t *addr = (bd_address_t *)GET_CONN_BT_ADDR(base_conn);

	btsrv_rdm_set_pbap_connected(base_conn, false);
	btsrv_proc_link_change(addr->val, BTSRV_LINK_PBAP_DISCONNECTED);
	return 0;
}

static int btsrv_connect_map_connected(struct bt_conn *base_conn)
{
	bd_address_t *addr = (bd_address_t *)GET_CONN_BT_ADDR(base_conn);

	btsrv_rdm_set_map_connected(base_conn, true);
	btsrv_proc_link_change(addr->val, BTSRV_LINK_MAP_CONNECTED);
	return 0;
}

static int btsrv_connect_map_disconnected(struct bt_conn *base_conn)
{
	bd_address_t *addr = (bd_address_t *)GET_CONN_BT_ADDR(base_conn);

	btsrv_rdm_set_map_connected(base_conn, false);
	btsrv_proc_link_change(addr->val, BTSRV_LINK_MAP_DISCONNECTED);
	return 0;
}

static int btsrv_connect_hid_connected(struct bt_conn *base_conn)
{
	bd_address_t *addr = (bd_address_t *)GET_CONN_BT_ADDR(base_conn);

	btsrv_rdm_set_hid_connected(base_conn, true);
	btsrv_notify_link_event(base_conn, BT_LINK_EV_HID_CONNECTED, 0);
	btsrv_proc_link_change(addr->val, BTSRV_LINK_HID_CONNECTED);
	return 0;
}

static int btsrv_connect_hid_disconnected(struct bt_conn *base_conn)
{
	bd_address_t *addr = (bd_address_t *)GET_CONN_BT_ADDR(base_conn);

	btsrv_rdm_set_hid_connected(base_conn, false);
	btsrv_notify_link_event(base_conn, BT_LINK_EV_HID_DISCONNECTED, 0);
	btsrv_proc_link_change(addr->val, BTSRV_LINK_HID_DISCONNECTED);
	return 0;
}

static int btsrv_connect_hid_unplug(struct bt_conn *base_conn)
{
	int i;
	struct autoconn_info *tmpInfo;
	bd_address_t *addr = (bd_address_t *)GET_CONN_BT_ADDR(base_conn);
	
	tmpInfo = bt_mem_malloc(sizeof(struct autoconn_info)*BTSRV_SAVE_AUTOCONN_NUM);
	if (!tmpInfo) {
		SYS_LOG_ERR("BAC hid malloc failed");
		return -ENOMEM;
	}
	
	btsrv_connect_get_auto_reconnect_info(tmpInfo, BTSRV_SAVE_AUTOCONN_NUM);
	for (i = 0; i < BTSRV_SAVE_AUTOCONN_NUM; i++) {
		if (tmpInfo[i].addr_valid) {
			if (!memcmp(tmpInfo[i].addr.val, addr->val, sizeof(bd_address_t))) {
				if(tmpInfo[i].hid && !tmpInfo[i].a2dp && !tmpInfo[i].hfp
					&& !tmpInfo[i].avrcp){
					tmpInfo[i].addr_valid = 0;
					SYS_LOG_INF("BAC hid clear info");
				}
				break;
			}
		}
	}

	btsrv_connect_set_auto_reconnect_info(tmpInfo,BTSRV_SAVE_AUTOCONN_NUM);
	if (tmpInfo) {
		bt_mem_free(tmpInfo);
	}
	return 0;
}

static void btsrv_controler_role_discovery(struct bt_conn *conn)
{
	uint8_t role;

	if (hostif_bt_conn_role_discovery(conn, &role)) {
		SYS_LOG_ERR("BAC Failed to discovery role");
		return;
	}

	btsrv_rdm_set_controler_role(conn, role);

	if (role == CONTROLER_ROLE_MASTER) {
		hostif_bt_conn_set_supervision_timeout(conn, BT_SUPERVISION_TIMEOUT);
	}
}

void btsrv_connect_set_phone_controler_role(bd_address_t *bd, uint8_t role)
{
	struct bt_conn *conn = btsrv_rdm_find_conn_by_addr(bd);
	uint8_t dev_role, dev_exp_role;

	if (conn == NULL) {
		return;
	}

	dev_exp_role = (role == CONTROLER_ROLE_MASTER) ? CONTROLER_ROLE_SLAVE : CONTROLER_ROLE_MASTER;
	btsrv_rdm_get_controler_role(conn, &dev_role);

	if ((btsrv_rdm_get_conn_role(conn) == BTSRV_TWS_NONE) && (dev_role != dev_exp_role)) {
		/* Controler request do role swith after security */
		SYS_LOG_INF("BAC Do role(%d) switch", dev_exp_role);
		hostif_bt_conn_switch_role(conn, dev_exp_role);
	}
}

static bool btsrv_check_connectable(uint8_t role)
{
	bool connectable = true;
	uint8_t dev_role = btsrv_rdm_get_dev_role();
	uint8_t connected_cnt = btsrv_rdm_get_connected_dev(NULL, NULL);

	SYS_LOG_INF("BAC check_connectable:%d,%d,%d,%d,%d",
		role, dev_role, connected_cnt, btsrv_max_conn_num(), btsrv_max_phone_num());

	if (connected_cnt > btsrv_max_conn_num()) {
		/* Already connect max conn */
		connectable = false;
	} else if ((dev_role == BTSRV_TWS_NONE) && (role == BTSRV_TWS_NONE) &&
#ifdef CONFIG_BT_A2DP_TRS
				(btsrv_rdm_get_dev_trs_mode() == 0) &&
#endif
				(connected_cnt > btsrv_max_phone_num())) {
		/* Already connect max phone number, can't connect any phone */
		connectable = false;
	} else if (dev_role != BTSRV_TWS_NONE && role != BTSRV_TWS_NONE) {
		/* Already connect as tws device, Can't connect another tws device */
		connectable = false;
	}

	return connectable;
}

static void btsrv_get_name_finish(void *info, uint8_t role)
{
	struct bt_conn *conn;
	struct btsrv_addr_name mac_addr_info;

	memcpy(&mac_addr_info, info, sizeof(mac_addr_info));

	conn = btsrv_rdm_find_conn_by_addr(&mac_addr_info.mac);
	if (conn == NULL) {
		SYS_LOG_ERR("BAC Can't find conn for addr");
		return;
	}

#ifdef CONFIG_BT_A2DP_TRS
	if (!btsrv_rdm_get_trs_mode(conn)) {
		if (btsrv_is_record_trs_dev(&mac_addr_info.mac)) {
			btsrv_rdm_set_trs_mode(&mac_addr_info.mac, 1);
		}
	}
#endif

	if (!btsrv_check_connectable(role)) {
		SYS_LOG_INF("BAC Disconnect role:%d", role);
		btsrv_adapter_disconnect(conn);
		return;
	}

	btsrv_rdm_set_dev_name(conn, mac_addr_info.name);
	btsrv_notify_link_event(conn, BT_LINK_EV_GET_NAME, role);
	btsrv_proc_link_change(mac_addr_info.mac.val, BTSRV_LINK_BASE_GET_NAME);
	if (role != BTSRV_TWS_NONE) {
		btsrv_event_notify(MSG_BTSRV_TWS, MSG_BTSRV_GET_NAME_FINISH, conn);
	} else {
#ifdef CONFIG_BT_A2DP_TRS
		if (btsrv_rdm_get_trs_mode(conn)) {
			return;
		}
#endif
		uint8_t controler_role;

		if (!btsrv_rdm_get_controler_role(conn, &controler_role) &&
			(controler_role == CONTROLER_ROLE_MASTER)) {
			/* Controler request as slave connected to phone */
			hostif_bt_conn_switch_role(conn, CONTROLER_ROLE_SLAVE);
		}
	}
}

static void connected_dev_cb_do_disconnect(struct bt_conn *base_conn, uint8_t tws_dev, void *cb_param)
{
	int disconnect_mode = (int)cb_param;

	if (disconnect_mode == BTSRV_DISCONNECT_ALL_MODE) {
		if (tws_dev) {
			btsrv_event_notify_malloc(MSG_BTSRV_TWS, MSG_BTSRV_TWS_DISCONNECT, (uint8_t *)GET_CONN_BT_ADDR(base_conn), sizeof(bd_address_t), 0);
		} else {
			btsrv_adapter_disconnect(base_conn);
		}
	} else if (disconnect_mode == BTSRV_DISCONNECT_PHONE_MODE) {
		if (!tws_dev) {
			btsrv_adapter_disconnect(base_conn);
		}
	} else if (disconnect_mode == BTSRV_DISCONNECT_TWS_MODE) {
		if (tws_dev) {
			btsrv_event_notify_malloc(MSG_BTSRV_TWS, MSG_BTSRV_TWS_DISCONNECT, (uint8_t *)GET_CONN_BT_ADDR(base_conn), sizeof(bd_address_t), 0);
		}
	} else if (disconnect_mode == BTSRV_DISCONNECT_TRS_MODE) {
#ifdef CONFIG_BT_A2DP_TRS
		if (btsrv_rdm_get_trs_mode(base_conn)) {
			btsrv_adapter_disconnect(base_conn);
		}
#endif
	}
}

static void btsrv_connect_disconnect_device(int disconnect_mode)
{
	SYS_LOG_INF("BAC disconnect mode %d", disconnect_mode);
	btsrv_rdm_get_connected_dev(connected_dev_cb_do_disconnect, (void *)disconnect_mode);
}

static void btsrv_connect_clear_list(int mode)
{
	int count;
	int disconnec_mode = 0;
	btsrv_connect_auto_connection_stop();
#ifdef CONFIG_SUPPORT_TWS
	btsrv_tws_cancal_auto_connect();
#endif

	if (mode == BTSRV_DEVICE_ALL) {
		disconnec_mode = BTSRV_DISCONNECT_ALL_MODE;
	} else if (mode == BTSRV_DEVICE_PHONE) {
		disconnec_mode = BTSRV_DISCONNECT_PHONE_MODE;
	} else if (mode == BTSRV_DEVICE_TWS) {
		disconnec_mode = BTSRV_DISCONNECT_TWS_MODE;
	}

	count = btsrv_rdm_get_connected_dev(connected_dev_cb_do_disconnect, (void *)disconnec_mode);
	if (count == 0) {
		btsrv_scan_update_mode(true);
		btsrv_event_notify(MSG_BTSRV_CONNECT, MGS_BTSRV_CLEAR_AUTO_INFO, NULL);
		SYS_LOG_INF("BAC clear list finish");
	} else {
		p_connect->clear_list_disconnecting = 1;
		p_connect->clear_list_mode = mode;
	}
}

static void btsrv_auto_connect_proc(struct bt_set_autoconn *param)
{
	int i, idle_pos = BTSRV_SAVE_AUTOCONN_NUM, need_start_timer = 0;
	uint8_t alread_connected = (btsrv_rdm_find_conn_by_addr(&param->addr)) ? 1 : 0;

	if (alread_connected && (param->force_connect == 0)) {
		SYS_LOG_INF("BAC already connected");
		return;
	} else {
		SYS_LOG_INF("BAC add auto connect");
	}

	for (i = 0; i < BTSRV_SAVE_AUTOCONN_NUM; i++) {
		if (!p_connect->auto_conn[i].addr_valid &&
			idle_pos == BTSRV_SAVE_AUTOCONN_NUM) {
			idle_pos = i;
		}

		if (p_connect->auto_conn[i].addr_valid &&
			!memcmp(param->addr.val, p_connect->auto_conn[i].addr.val, sizeof(bd_address_t))) {
			SYS_LOG_INF("BAC device already in reconnect list");
			return;
		}
	}

	if (idle_pos == BTSRV_SAVE_AUTOCONN_NUM) {
		SYS_LOG_ERR("BAC not more position for reconnect device");
		return;
	}

	if (alread_connected && param->force_connect) {
		goto direct_connect;
	}

	for (i = 0; i < BTSRV_SAVE_AUTOCONN_NUM; i++) {
		if (p_connect->nvram_reconn_info[i].addr_valid &&
			!memcmp(p_connect->nvram_reconn_info[i].addr.val, param->addr.val, sizeof(bd_address_t))) {
			memcpy(p_connect->auto_conn[idle_pos].addr.val, param->addr.val, sizeof(bd_address_t));
			p_connect->auto_conn[idle_pos].addr_valid = p_connect->nvram_reconn_info[i].addr_valid;
			p_connect->auto_conn[idle_pos].tws_role = p_connect->nvram_reconn_info[i].tws_role;
#ifdef CONFIG_BT_A2DP_TRS
			p_connect->auto_conn[idle_pos].trs_mode = p_connect->nvram_reconn_info[i].trs_mode;
#endif

#ifdef CONFIG_BT_A2DP
			p_connect->auto_conn[idle_pos].a2dp = p_connect->nvram_reconn_info[i].a2dp;
#else
			p_connect->auto_conn[idle_pos].a2dp = 0;
#endif
			p_connect->auto_conn[idle_pos].hfp = p_connect->nvram_reconn_info[i].hfp;

#ifdef CONFIG_BT_AVRCP
			p_connect->auto_conn[idle_pos].avrcp = p_connect->nvram_reconn_info[i].avrcp;
#else
            p_connect->auto_conn[idle_pos].avrcp = 0;
#endif
			p_connect->auto_conn[idle_pos].hid = p_connect->nvram_reconn_info[i].hid;
			p_connect->auto_conn[idle_pos].hfp_first = p_connect->nvram_reconn_info[i].hfp_first;
			p_connect->auto_conn[idle_pos].strategy = param->strategy;
			p_connect->auto_conn[idle_pos].base_try = param->base_try;
			p_connect->auto_conn[idle_pos].profile_try = param->profile_try;
			p_connect->auto_conn[idle_pos].curr_connect_profile = AUTOCONN_PROFILE_IDLE;
			p_connect->auto_conn[idle_pos].state = AUTOCONN_STATE_IDLE;
			p_connect->auto_conn[idle_pos].base_interval = param->base_interval;
			p_connect->auto_conn[idle_pos].profile_interval = param->profile_interval;
			need_start_timer = 1;
			SYS_LOG_INF("BAC add one");
			break;
		}
	}

	if (i == BTSRV_SAVE_AUTOCONN_NUM) {
		SYS_LOG_ERR("BAC device not in AUTOCONN_INFO_NVRAM");
#ifdef CONFIG_BT_A2DP_TRS
		if(param->trs_mode) {
			SYS_LOG_INF("BAC trs connect");
			memcpy(p_connect->auto_conn[idle_pos].addr.val, param->addr.val, sizeof(bd_address_t));
			p_connect->auto_conn[idle_pos].addr_valid = 1;
			p_connect->auto_conn[idle_pos].tws_role = 0;
			p_connect->auto_conn[idle_pos].trs_mode = 1;
			p_connect->auto_conn[idle_pos].a2dp = 1;
			p_connect->auto_conn[idle_pos].hfp = 0;
			p_connect->auto_conn[idle_pos].avrcp = 1;
			p_connect->auto_conn[idle_pos].hfp_first = 0;
			p_connect->auto_conn[idle_pos].strategy = param->strategy;
			p_connect->auto_conn[idle_pos].base_try = param->base_try;
			p_connect->auto_conn[idle_pos].profile_try = param->profile_try;
			p_connect->auto_conn[idle_pos].curr_connect_profile = AUTOCONN_PROFILE_IDLE;
			p_connect->auto_conn[idle_pos].state = AUTOCONN_STATE_IDLE;
			p_connect->auto_conn[idle_pos].base_interval = param->base_interval;
			p_connect->auto_conn[idle_pos].profile_interval = param->profile_interval;
			need_start_timer = 1;
		} else
#endif
		if (param->force_connect) {
direct_connect:
			SYS_LOG_INF("BAC force connect");
			memcpy(p_connect->auto_conn[idle_pos].addr.val, param->addr.val, sizeof(bd_address_t));
			p_connect->auto_conn[idle_pos].addr_valid = 1;
			p_connect->auto_conn[idle_pos].tws_role = 0;
			p_connect->auto_conn[idle_pos].trs_mode = 0;
			p_connect->auto_conn[idle_pos].a2dp = param->connect_a2dp;
			p_connect->auto_conn[idle_pos].hfp = param->connect_hfp;
			p_connect->auto_conn[idle_pos].avrcp = param->connect_avrcp;
			p_connect->auto_conn[idle_pos].hfp_first = param->connect_hfp_first;
			p_connect->auto_conn[idle_pos].strategy = param->strategy;
			p_connect->auto_conn[idle_pos].base_try = param->base_try;
			p_connect->auto_conn[idle_pos].profile_try = param->profile_try;
			p_connect->auto_conn[idle_pos].curr_connect_profile = AUTOCONN_PROFILE_IDLE;
			p_connect->auto_conn[idle_pos].state = alread_connected ? AUTOCONN_STATE_PROFILE_CONNETING : AUTOCONN_STATE_IDLE;
			p_connect->auto_conn[idle_pos].base_interval = param->base_interval;
			p_connect->auto_conn[idle_pos].profile_interval = param->profile_interval;
			need_start_timer = 1;
		}
	}

	if (!bt_register_a2dp_status_get()) {
		if(p_connect->auto_conn[idle_pos].trs_mode) {
			SYS_LOG_INF("don't connect and clear trs device info");
			memset(&p_connect->auto_conn[idle_pos], 0, sizeof(struct auto_conn_t));
			return;
		}
		p_connect->auto_conn[idle_pos].a2dp = 0;
		p_connect->auto_conn[idle_pos].avrcp = 0;
	}

	/* Only need start timer when is stop */
	if (need_start_timer && (!thread_timer_is_running(&p_connect->auto_conn_timer))) {
		p_connect->connecting_index = idle_pos;
		thread_timer_start(&p_connect->auto_conn_timer, AUTOCONN_START_TIME, 0);
		btsrv_adapter_set_clear_wake_lock(BTSRV_WAKE_LOCK_RECONNECT, 1);
		p_connect->reconnect_req_high_performance = 1;
		btsrv_update_performance_req();
	}
}

static void btsrv_connect_check_switch_sbc(void)
{
#ifdef CONFIG_SUPPORT_TWS
	uint8_t codec = BT_A2DP_SBC;
	struct bt_conn *conn = btsrv_rdm_a2dp_get_actived();

	if ((btsrv_rdm_get_dev_role() != BTSRV_TWS_MASTER) ||
		btsrv_tws_check_support_feature(TWS_FEATURE_A2DP_AAC)) {
		return;
	}

	/* As tws master, tws can't use AAC */
	if (conn == NULL) {
		btsrv_a2dp_halt_aac_endpoint(true);
		return;
	}

	btsrv_rdm_a2dp_get_codec_info(conn, &codec, NULL, NULL);
	if (codec == BT_A2DP_MPEG2) {
		SYS_LOG_INF("BAC Need switch to SBC");
		btsrv_rdm_set_switch_sbc_state(conn, SWITCH_SBC_STATE_DISCONNECTING_A2DP);
		if (btsrv_a2dp_disconnect(conn)) {
			btsrv_rdm_set_switch_sbc_state(conn, SWITCH_SBC_STATE_IDLE);
		}
	}
#endif
}

static void btsrv_connect_proc_switch_sbc_state(struct bt_conn *conn, uint8_t cmd)
{
#ifdef CONFIG_SUPPORT_TWS
	if (btsrv_rdm_get_conn_role(conn) != BTSRV_TWS_NONE) {
		return;
	}

	switch (btsrv_rdm_get_switch_sbc_state(conn)) {
	case SWITCH_SBC_STATE_IDLE:
		break;
	case SWITCH_SBC_STATE_DISCONNECTING_A2DP:
		if (cmd == MSG_BTSRV_A2DP_DISCONNECTED) {
			if (btsrv_rdm_get_dev_role() == BTSRV_TWS_MASTER) {
				btsrv_a2dp_halt_aac_endpoint(true);
			}
			SYS_LOG_INF("BAC Switch connect SBC");
			btsrv_rdm_set_switch_sbc_state(conn, SWITCH_SBC_STATE_CONNECTING_A2DP);
			btsrv_a2dp_connect(conn, BT_A2DP_CH_SINK);
		} else {
			SYS_LOG_WRN("BAC Unexpect case!\n");
		}
		break;
	case SWITCH_SBC_STATE_CONNECTING_A2DP:
		SYS_LOG_INF("BAC Switch SBC ok");
		btsrv_rdm_set_switch_sbc_state(conn, SWITCH_SBC_STATE_IDLE);
		break;
	}
#endif
}

int btsrv_connect_process(struct app_msg *msg)
{
	int ret = 0;

	switch (_btsrv_get_msg_param_cmd(msg)) {
	case MSG_BTSRV_CONNECTED:
		SYS_LOG_INF("BAC acl connected");
		btsrv_connect_connected(msg->ptr);
		btsrv_controler_role_discovery(msg->ptr);
		btsrv_adapter_callback(BTSRV_RELEASE_HIGH_PERFORMANCE, NULL);
		/* ref for bt servcie process MSG_BTSRV_CONNECTED,
		 * need unref conn after process MSG_BTSRV_CONNECTED.
		 */
		hostif_bt_conn_unref(msg->ptr);
		break;
	case MSG_BTSRV_CONNECTED_FAILED:
		SYS_LOG_INF("BAC acl connect failed");
		btsrv_proc_link_change(msg->ptr, BTSRV_LINK_BASE_CONNECTED_FAILED);
		btsrv_event_notify_malloc(MSG_BTSRV_TWS, _btsrv_get_msg_param_cmd(msg), msg->ptr, sizeof(bd_address_t), 0);
		break;
	case MSG_BTSRV_SECURITY_CHANGED:
		btsrv_connect_security_changed(msg->ptr);
		break;
	case MSG_BTSRV_ROLE_CHANGE:
		btsrv_rdm_set_controler_role(msg->ptr, _btsrv_get_msg_param_reserve(msg));
		if (_btsrv_get_msg_param_reserve(msg) == CONTROLER_ROLE_MASTER) {
			hostif_bt_conn_set_supervision_timeout(msg->ptr, BT_SUPERVISION_TIMEOUT);
		}
		btsrv_event_notify_ext(MSG_BTSRV_TWS, MSG_BTSRV_ROLE_CHANGE, msg->ptr, _btsrv_get_msg_param_reserve(msg));
		break;
	case MSG_BTSRV_MODE_CHANGE:
		btsrv_sniff_mode_change(msg->ptr);
		break;
	case MSG_BTSRV_DISCONNECTED:
		SYS_LOG_INF("BAC acl disconnected");
		btsrv_connect_disconnected(msg->ptr, _btsrv_get_msg_param_reserve(msg));
		break;
	case MSG_BTSRV_A2DP_CONNECTED:
		SYS_LOG_INF("BAC a2dp connected");
#ifdef CONFIG_BT_A2DP_TRS
		if (btsrv_connect_check_trs_a2dp_connected(msg->ptr)) {
			break;
		}
#endif
		btsrv_connect_a2dp_connected(msg->ptr);
		if (btsrv_rdm_get_conn_role(msg->ptr) >= BTSRV_TWS_PENDING) {
			btsrv_event_notify(MSG_BTSRV_TWS, _btsrv_get_msg_param_cmd(msg), msg->ptr);
		} else {
			/* Phone a2dp may in connecting when receive MSG_BTSRV_A2DP_CHECK_SWITCH_SBC
			 * check agine when phone a2dp connected.
			 */
			btsrv_connect_check_switch_sbc();
		}

		btsrv_event_notify(MSG_BTSRV_A2DP, _btsrv_get_msg_param_cmd(msg), msg->ptr);
		btsrv_connect_proc_switch_sbc_state(msg->ptr, MSG_BTSRV_A2DP_CONNECTED);
		break;
	case MSG_BTSRV_A2DP_DISCONNECTED:
		SYS_LOG_INF("BAC a2dp disconnected");
		if (btsrv_rdm_get_conn_role(msg->ptr) >= BTSRV_TWS_PENDING) {
			btsrv_event_notify(MSG_BTSRV_TWS, _btsrv_get_msg_param_cmd(msg), msg->ptr);
			btsrv_a2dp_halt_aac_endpoint(false);
		}

		btsrv_connect_a2dp_disconnected(msg->ptr, true);
		btsrv_connect_proc_switch_sbc_state(msg->ptr, MSG_BTSRV_A2DP_DISCONNECTED);
		break;
	case MSG_BTSRV_AVRCP_CONNECTED:
		SYS_LOG_INF("BAC avrcp connected");
		btsrv_connect_avrcp_connected(msg->ptr);
		btsrv_event_notify(MSG_BTSRV_AVRCP, _btsrv_get_msg_param_cmd(msg), msg->ptr);
		break;
	case MSG_BTSRV_AVRCP_DISCONNECTED:
		SYS_LOG_INF("BAC avrcp disconnected");
		btsrv_connect_avrcp_disconnected(msg->ptr, true);
		btsrv_event_notify(MSG_BTSRV_AVRCP, _btsrv_get_msg_param_cmd(msg), msg->ptr);
		break;
	case MSG_BTSRV_HID_CONNECTED:
		SYS_LOG_INF("BAC hid connected");
		btsrv_connect_hid_connected(msg->ptr);
		btsrv_event_notify(MSG_BTSRV_HID, _btsrv_get_msg_param_cmd(msg), msg->ptr);
		break;
	case MSG_BTSRV_HID_DISCONNECTED:
		SYS_LOG_INF("BAC hid disconnected");
		btsrv_connect_hid_disconnected(msg->ptr);
		btsrv_event_notify(MSG_BTSRV_HID, _btsrv_get_msg_param_cmd(msg), msg->ptr);
		break;
	case MSG_BTSRV_HID_UNPLUG:
		SYS_LOG_INF("BAC hdi unplug");
		btsrv_connect_hid_unplug(msg->ptr);
		break;
	case MSG_BTSRV_HFP_AG_CONNECTED:
		SYS_LOG_INF("BAC hfp ag connected");
		btsrv_rdm_set_hfp_role(msg->ptr,BTSRV_HFP_ROLE_AG);
		btsrv_connect_hfp_connected(msg->ptr);
		if (btsrv_rdm_get_conn_role(msg->ptr) >= BTSRV_TWS_PENDING) {
			btsrv_event_notify(MSG_BTSRV_TWS, _btsrv_get_msg_param_cmd(msg), msg->ptr);
		}
		btsrv_event_notify(MSG_BTSRV_HFP_AG, _btsrv_get_msg_param_cmd(msg), msg->ptr);
		break;
	case MSG_BTSRV_HFP_CONNECTED:
		SYS_LOG_INF("BAC hfp connected");
		btsrv_rdm_set_hfp_role(msg->ptr,BTSRV_HFP_ROLE_HF);
		btsrv_connect_hfp_connected(msg->ptr);
		if (btsrv_rdm_get_conn_role(msg->ptr) >= BTSRV_TWS_PENDING) {
			btsrv_event_notify(MSG_BTSRV_TWS, _btsrv_get_msg_param_cmd(msg), msg->ptr);
		}
		btsrv_event_notify(MSG_BTSRV_HFP, _btsrv_get_msg_param_cmd(msg), msg->ptr);
		break;
	case MSG_BTSRV_HFP_AG_DISCONNECTED:
		SYS_LOG_INF("BAC hfp ag disconnected");
		btsrv_rdm_set_hfp_role(msg->ptr,BTSRV_HFP_ROLE_HF);
	case MSG_BTSRV_HFP_DISCONNECTED:
		SYS_LOG_INF("BAC hfp disconnected");
		/* Why need check btsrv_rdm_is_hfp_connected ?? */
		if (btsrv_rdm_is_hfp_connected(msg->ptr)) {
			btsrv_connect_hfp_disconnected(msg->ptr, true);
		}
		if (btsrv_rdm_get_conn_role(msg->ptr) >= BTSRV_TWS_PENDING) {
			btsrv_event_notify(MSG_BTSRV_TWS, _btsrv_get_msg_param_cmd(msg), msg->ptr);
		}
		break;
	case MSG_BTSRV_SPP_CONNECTED:
		btsrv_connect_spp_connected(msg->ptr);
		break;
	case MSG_BTSRV_SPP_DISCONNECTED:
		btsrv_connect_spp_disconnected(msg->ptr);
		break;
	case MSG_BTSRV_PBAP_CONNECTED:
		btsrv_connect_pbap_connected(msg->ptr);
		break;
	case MSG_BTSRV_PBAP_DISCONNECTED:
		btsrv_connect_pbap_disconnected(msg->ptr);
		break;
	case MSG_BTSRV_MAP_CONNECTED:
		btsrv_connect_map_connected(msg->ptr);
		break;
	case MSG_BTSRV_MAP_DISCONNECTED:
		btsrv_connect_map_disconnected(msg->ptr);
		break;
	case MSG_BTSRV_GET_NAME_FINISH:
		btsrv_get_name_finish(msg->ptr, _btsrv_get_msg_param_reserve(msg));
		break;
	case MSG_BTSRV_CLEAR_LIST_CMD:
		btsrv_connect_clear_list((int)(_btsrv_get_msg_param_ptr(msg)));
		break;
	case MGS_BTSRV_CLEAR_AUTO_INFO:
		btsrv_autoconn_info_clear();
		break;
	case MSG_BTSRV_AUTO_RECONNECT:
		btsrv_auto_connect_proc(msg->ptr);
		break;
	case MSG_BTSRV_AUTO_RECONNECT_STOP:
		btsrv_connect_auto_connection_stop();
#ifdef CONFIG_SUPPORT_TWS
		btsrv_tws_cancal_auto_connect();
#endif
		break;
	case MSG_BTSRV_DISCONNECT_DEVICE:
		btsrv_connect_disconnect_device((int)(_btsrv_get_msg_param_ptr(msg)));
		break;
	case MSG_BTSRV_A2DP_CHECK_SWITCH_SBC:
		btsrv_connect_check_switch_sbc();
		break;
#ifdef CONFIG_BT_A2DP_TRS
	case MSG_BTSRV_CLEAR_PAIRED_INFO:
        btsrv_clear_trs_dev_record(msg->ptr);
        break;
#endif
	default:
		break;
	}

	return ret;
}

bool btsrv_autoconn_is_reconnecting(void)
{
	uint8_t index = p_connect->connecting_index;
	struct auto_conn_t *auto_conn = NULL;
	uint8_t i;

	SYS_LOG_DBG("addr_valid %d %d %d %d.", index,
		(u8_t)p_connect->auto_conn[index].addr_valid,
		p_connect->auto_conn[index].state,
		p_connect->auto_conn[index].base_try);

	//if (!p_connect->auto_conn[index].addr_valid) {
	//	return false;
	//}

	if ((p_connect->auto_conn[index].addr_valid) && 
		(p_connect->auto_conn[index].state != AUTOCONN_STATE_IDLE) &&
		(p_connect->auto_conn[index].state != AUTOCONN_STATE_DELAY_CONNECTING)) {
		return true;
	}

	for (i = 0; i < BTSRV_SAVE_AUTOCONN_NUM; i++) {
		auto_conn = &p_connect->auto_conn[i];
		if ((auto_conn->addr_valid) &&
			(thread_timer_is_running(&p_connect->auto_conn_timer)) &&
			(auto_conn->state == AUTOCONN_STATE_IDLE) &&
			(auto_conn->base_try > 0)) {
			SYS_LOG_INF("base_try %d.",auto_conn->base_try);
			return true;
		}
	}

	return false;
}

bool btsrv_autoconn_is_runing(void)
{
	return p_connect->reconnect_req_high_performance ? true : false;
}

void btsrv_connect_tws_role_confirm(void)
{
	/* Already as tws slave, stop autoconnect to phone  and disconnect connected phone */
	if (btsrv_rdm_get_dev_role() == BTSRV_TWS_SLAVE) {
		btsrv_connect_auto_connection_stop();
		btsrv_connect_disconnect_device(BTSRV_DISCONNECT_PHONE_MODE);
	}
}

int btsrv_connect_init(void)
{
	p_connect = &p_btsrv_connect;

	memset(p_connect, 0, sizeof(struct btsrv_connect_priv));
	thread_timer_init(&p_connect->auto_conn_timer, btsrv_autoconn_timer_handler, NULL);
	thread_timer_init(&p_connect->monitor_conn_timer, btsrv_monitor_timer_handler, NULL);

	btsrv_property_get(CFG_AUTOCONN_INFO, (char *)p_connect->nvram_reconn_info,
						(sizeof(p_connect->nvram_reconn_info)));
#ifdef CONFIG_BT_A2DP_TRS
	btsrv_property_get(RECORD_A2DP_TRS_DEV, (char *)p_connect->paired_dev_list,
						sizeof(p_connect->paired_dev_list));
#endif
	return 0;
}

void btsrv_connect_deinit(void)
{
	if (p_connect == NULL) {
		return;
	}

	while (p_connect->auto_connect_running) {
		os_sleep(10);
	}

	if (thread_timer_is_running(&p_connect->auto_conn_timer)) {
		thread_timer_stop(&p_connect->auto_conn_timer);
		btsrv_adapter_set_clear_wake_lock(BTSRV_WAKE_LOCK_RECONNECT, 0);
		p_connect->reconnect_req_high_performance = 0;
		btsrv_update_performance_req();
	}

	while (p_connect->monitor_timer_running) {
		os_sleep(10);
	}

	if (thread_timer_is_running(&p_connect->monitor_conn_timer)) {
		thread_timer_stop(&p_connect->monitor_conn_timer);
		btsrv_adapter_set_clear_wake_lock(BTSRV_WAKE_LOCK_WAIT_CONNECT, 0);
	}

	p_connect = NULL;
}

void btsrv_connect_dump_info(void)
{
	char addr_str[BT_ADDR_STR_LEN];
	int i;
	struct auto_conn_t *auto_conn;
	struct autoconn_info *info;

	if (p_connect == NULL) {
		printk("Auto reconnect not init\n");
		return;
	}

	printk("Btsrv connect info index: %d\n", p_connect->connecting_index);
	for (i = 0; i < BTSRV_SAVE_AUTOCONN_NUM; i++) {
		auto_conn = &p_connect->auto_conn[i];
		if (auto_conn->addr_valid) {
			hostif_bt_addr_to_str((const bt_addr_t *)&auto_conn->addr, addr_str, BT_ADDR_STR_LEN);
			printk("Dev index %d, state %d, mac: %s\n", i, auto_conn->state, addr_str);
			printk("\t a2dp %d, avrcp %d, hfp %d hid %d\n", auto_conn->a2dp, auto_conn->avrcp, auto_conn->hfp, auto_conn->hid);
			printk("\t tws_role %d, strategy %d\n", auto_conn->tws_role, auto_conn->strategy);
			printk("\t base_try %d, profile_try %d\n", auto_conn->base_try, auto_conn->profile_try);
			printk("\t base_interval %d, profile_interval %d\n", auto_conn->base_interval, auto_conn->profile_interval);
		}
	}

	printk("nvram_reconn_info\n");
	for (i = 0; i < BTSRV_SAVE_AUTOCONN_NUM; i++) {
		info = &p_connect->nvram_reconn_info[i];
		if (info->addr_valid) {
			hostif_bt_addr_to_str((const bt_addr_t *)&info->addr, addr_str, BT_ADDR_STR_LEN);
			printk("\t (%d)%s tws_role %d trs %d profile %d %d %d %d %d\n", i, addr_str, info->tws_role,
					info->trs_mode, info->hfp, info->a2dp, info->avrcp, info->hid, info->hfp_first);
		}
	}
}
