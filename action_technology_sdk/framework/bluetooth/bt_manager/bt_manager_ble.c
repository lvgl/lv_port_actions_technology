/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt ble manager.
 */
#define SYS_LOG_DOMAIN "btmgr_ble"

#include <os_common_api.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <msg_manager.h>
#include <mem_manager.h>
#include <acts_bluetooth/host_interface.h>
#include <bt_manager.h>
#include "bt_manager_inner.h"
#include <sys_event.h>
#include "bt_porting_inner.h"
#include <drivers/hrtimer.h>
#include <board_cfg.h>
#include <acts_bluetooth/services/hog.h>

/* TODO: New only support one ble connection */

#define BLE_WAKE_LOCK_CHECK_INTERVAL     (1000)		/* 1s */
#define BLE_WAKE_LOCK_CHECK_TIMES        (1)
#define BLE_CONNECT_INTERVAL_CHECK       (5000)		/* 5s */
#define BLE_CONNECT_DELAY_UPDATE_PARAM   (4000)
#define BLE_UPDATE_PARAM_FINISH_TIME     (4000)
#define BLE_DELAY_UPDATE_PARAM_TIME      (50)
#define BLE_CONNECTED_DELAY_NOTIFY       (200)
#define BLE_UPDATE_PARAM_ERR_MAX         (3)
#define BLE_DELAY_ENCRYPT_TIME           (3000)		/* 3s */

#define BLE_ADV_FAST_MAX_TIME            (1000 * 300)  /* 300s */
#define BLE_ADV_DELAY_CHECK              (1000 * 2)    /* 2s */
#define BLE_CONN_NOINIT_INTERVAL         (0xFF)

enum {
	PARAM_UPDATE_IDLE_STATE,
	PARAM_UPDATE_WAITO_UPDATE_STATE,
	PARAM_UPDATE_RUNING,
};

#ifndef CONFIG_BT_BLE_APP_UPDATE_PARAM
enum {
	BLE_PHONE_TYPE_ANDROID,
	BLE_PHONE_TYPE_IOS,
    BLE_PHONE_TYPE_MAX,
};

enum {
	BLE_CONN_SPEED_FAST,
    BLE_CONN_SPEED_BUSY,
	BLE_CONN_SPEED_IDLE,
    BLE_CONN_SPEED_MAX,
};

/*
 * Connect interval:
 *     Range: 0x0006 to 0x0C80
 *     Time = N * 1.25 ms
 *     Time Range: 7.5 ms to 4 s.
 * Latency: Range: 0x0000 to 0x01F3
 * Supervision_Timeout:
 *     Range: 0x000A to 0x0C80
 *     Time = N * 10 ms
 *     Time Range: 100 ms to 32 s
*/
const struct bt_le_conn_param le_conn_param[BLE_PHONE_TYPE_MAX][BLE_CONN_SPEED_MAX] = {
    {
        {6,15,0,600},
        {36,72,0,600},
        {32,48,4,600},
    },
    {
        {12,28,0,600},
        {36,72,0,600},
        {32,48,4,600},
    },
};
#endif

enum {
	BLE_ADV_SPEED_FAST,
	BLE_ADV_SPEED_NORMAL,
	BLE_ADV_SPEED_SLOW,
    BLE_ADV_SPEED_MAX,
};

/* Advertising_Interval_Min and Advertising_Interval_Max
 * Range: 0x0020 to 0x4000
 * Default: 0x0800 (1.28 s)
 * Time = N * 0.625 ms
 * Time Range: 20 ms to 10.24 s
 */
const uint16_t ble_adv_speed[BLE_ADV_SPEED_MAX] = {0x140,0x200,0x320};

static OS_MUTEX_DEFINE(ble_mgr_lock);
static os_sem ble_ind_sem __IN_BT_SECTION;
static struct bt_gatt_indicate_params ble_ind_params __IN_BT_SECTION;
static sys_slist_t ble_list __IN_BT_SECTION;

typedef void (*ble_notify_cb)(struct bt_conn *conn, int err);
struct ble_mgr_info {
	struct bt_conn *ble_conn;
	uint8_t device_mac[6];
	uint16_t mtu;
	uint8_t ble_deinit:1;
	uint8_t br_a2dp_runing:1;
	uint8_t br_hfp_runing:1;
	uint8_t update_work_state:4;
	uint8_t ble_app_enable_adv:1;
	uint8_t ble_advertise_active:1;
	uint8_t ble_advertise_speed_change:1;
	uint8_t ios_phone;
	uint8_t ble_state;
	int8_t rssi;
	uint8_t ble_adv_speed;
	uint8_t ble_conn_speed;
	uint8_t ble_conn_param_update_err;
	uint8_t ble_delaywork_wake_lock;
	uint16_t conn_rxtx_cnt;
	struct bt_le_conn_param le_conn_param;
#ifdef CONFIG_BT_BLE_APP_UPDATE_PARAM
	uint32_t update_time;			/* Update parameter time */
#else
	os_delayed_work param_update_work;
#endif
	os_delayed_work ble_wake_lock_work;
	os_delayed_work ble_connected_notify_work;
	os_delayed_work ble_advertise_manager_work;
	struct hrtimer timer;
	struct hrtimer s3_timer;
#ifdef CONFIG_BT_BLE_NOTIFY_PENDING
	uint8_t ble_send_pending:1;
	os_delayed_work ble_pending_notify_work;
#endif
#ifdef CONFIG_ACTIVE_REQ_BLE_ENC
	os_delayed_work ble_req_enc_work;
#endif
};

static struct ble_mgr_info ble_info;

#ifdef CONFIG_LEBR_DISPLAY_ONE
static const struct bt_data ble_ad_discov[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0x36, 0x55, 0x22, 0x22),
};
#else
static const struct bt_data ble_ad_discov[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		0x0d, 0x18, 0x0f, 0x18, 0x05, 0x18),
};
#endif

#define BLE_ADV_MAX_ARRAY_SIZE	4
#define BLE_ADV_DATA_MAX_LEN 31
struct bt_data ad_reset[BLE_ADV_MAX_ARRAY_SIZE];
size_t ad_reset_len;
struct bt_data sd_reset[BLE_ADV_MAX_ARRAY_SIZE];
size_t sd_reset_len;
OS_MUTEX_DEFINE(ble_adv_mutex);

bool nconn_flag = false;

void ble_set_clear_wake_lock(uint16_t wake_lock, uint8_t set)
{
	if (set) {
		ble_info.ble_delaywork_wake_lock |= wake_lock;
		os_delayed_work_submit(&ble_info.ble_wake_lock_work, 0);
	} else {
		ble_info.ble_delaywork_wake_lock &= (~wake_lock);
	}
}

#ifdef CONFIG_GATT_OVER_BREDR
static bool is_le_conn(struct bt_conn *conn)
{
	struct bt_conn_info info;

	if ((hostif_bt_conn_get_info(conn, &info) == 0) && (info.type == BT_CONN_TYPE_LE)) {
		return true;
	} else {
		return false;
	}
}
#endif

static void ble_advertise_init(void)
{
	if (ble_info.ble_state != BT_STATUS_BLE_CONNECTED) {
		ble_info.ble_state = BT_STATUS_BLE_NONE;
	}

	ble_info.ble_adv_speed = BLE_ADV_SPEED_FAST;
	ble_info.ble_advertise_active = false;
	ble_info.ble_advertise_speed_change = false;

	ble_set_clear_wake_lock(BLE_WAKE_LOCK_ADV_MANANGER, 1);
	os_delayed_work_submit(&ble_info.ble_advertise_manager_work, 0);
}

static void ble_advertise(void)
{
	struct bt_le_adv_param param;
	const struct bt_data *ad;
	size_t ad_len;
	int err;
	uint8_t speed = ble_info.ble_adv_speed;

	memset(&param, 0, sizeof(param));
	param.id = BT_ID_DEFAULT;
	param.interval_min = ble_adv_speed[speed];
	param.interval_max = ble_adv_speed[speed] + 0x50;

	os_mutex_lock(&ble_adv_mutex, OS_FOREVER);
	
	if(ad_reset[0].data)
	{
		for(int i=0; i< BLE_ADV_MAX_ARRAY_SIZE; i++)
		{
			if (ad_reset[i].type == BT_DATA_NAME_COMPLETE || ad_reset[i].type == BT_DATA_NAME_SHORTENED) 
			{
				param.options = (BT_LE_ADV_OPT_CONNECTABLE);
				break;
			} 
			param.options = (BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME);
		}
		err = hostif_bt_le_adv_start(&param, (ad_reset_len ? (const struct bt_data *)ad_reset : NULL), ad_reset_len,
		(sd_reset_len ? (const struct bt_data *)sd_reset : NULL), sd_reset_len);
	}
	else
	{
		param.options = (BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME);
		ad = ble_ad_discov;
		ad_len = ARRAY_SIZE(ble_ad_discov);
		err = hostif_bt_le_adv_start(&param, ad, ad_len,NULL, 0);
	}
	ble_info.ble_state = BT_STATUS_BLE_ADV;
	ble_info.ble_advertise_active = true;
	if (err < 0 && err != (-EALREADY)) {
		SYS_LOG_ERR("Failed to start advertising (err %d)", err);
	} else {
		SYS_LOG_INF("Advertising started speed %d", ble_info.ble_adv_speed);
	}
	
	os_mutex_unlock(&ble_adv_mutex);
}

/* set nconnectable advertise */
void ble_advertise_v2(void)
{
	struct bt_le_adv_param param;
	const struct bt_data *ad;
	size_t ad_len;
	int err;
	uint8_t speed = ble_info.ble_adv_speed;

	memset(&param, 0, sizeof(param));
	param.id = BT_ID_DEFAULT;
	param.interval_min = ble_adv_speed[speed];
	param.interval_max = ble_adv_speed[speed] + 0x50;

	os_mutex_lock(&ble_adv_mutex, OS_FOREVER);
	
	printk("ble_advertise\n");
	if(ad_reset[0].data)
	{
		for(int i=0; i< BLE_ADV_MAX_ARRAY_SIZE; i++)
		{
			if (ad_reset[i].type == BT_DATA_NAME_COMPLETE || ad_reset[i].type == BT_DATA_NAME_SHORTENED) 
			{
				param.options = (BT_LE_ADV_OPT_NONE);
				break;
			} 
			param.options = (BT_LE_ADV_OPT_NONE | BT_LE_ADV_OPT_USE_NAME);
		}
		err = hostif_bt_le_adv_start(&param, (ad_reset_len ? (const struct bt_data *)ad_reset : NULL), ad_reset_len,
		(sd_reset_len ? (const struct bt_data *)sd_reset : NULL), sd_reset_len);
	}
	else
	{
		param.options = (BT_LE_ADV_OPT_NONE | BT_LE_ADV_OPT_USE_NAME);
		ad = ble_ad_discov;
		ad_len = ARRAY_SIZE(ble_ad_discov);
		err = hostif_bt_le_adv_start(&param, ad, ad_len,NULL, 0);
	}
	ble_info.ble_state = BT_STATUS_BLE_ADV;
	ble_info.ble_advertise_active = true;
	if (err < 0 && err != (-EALREADY)) {
		SYS_LOG_ERR("Failed to start advertising (err %d)", err);
	} else {
		SYS_LOG_INF("Advertising started speed %d", ble_info.ble_adv_speed);
	}
	
	os_mutex_unlock(&ble_adv_mutex);
}

void bt_manager_ble_set_nconn_adv(bool enable)
{
	if(enable)
		nconn_flag = true;
	else
		nconn_flag = false;
}

int bt_manager_ble_set_adv_data(struct bt_data ad_data[], uint8_t ad_data_len, struct bt_data sd_data[], uint8_t sd_data_len)
{	
	os_mutex_lock(&ble_adv_mutex, OS_FOREVER);

	int i=0,j=0;

	if(ad_data == NULL && sd_data == NULL)
	{
		memset(ad_reset,0,sizeof(ad_reset));
		memset(sd_reset,0,sizeof(sd_reset));
	}
	
	if(ad_data)
	{
		memset(ad_reset,0,sizeof(ad_reset));
		for(i=0; i<ad_data_len; i++)
		{
			ad_reset[i] = ad_data[i];
		}
		ad_reset_len = ad_data_len;
	}
	
	if(sd_data)
	{
		memset(sd_reset,0,sizeof(sd_reset));
		for(j=0; j<sd_data_len; j++)
		{
			sd_reset[j] = sd_data[j];
		}
		sd_reset_len = sd_data_len;
	}

	os_mutex_unlock(&ble_adv_mutex);

	return 0;
}

#ifdef CONFIG_BT_BLE_APP_UPDATE_PARAM
int bt_manager_ble_update_param(const struct bt_le_conn_param *param)
{
	int prio = 0, ret = 0;

	prio = k_thread_priority_get(k_current_get());
	if (prio >= 0) {
		k_thread_priority_set(k_current_get(), -1);
	}

	if (!ble_info.ble_conn) {
		ret = -EIO;
		goto update_exit;
	}

	if ((os_uptime_get_32() - ble_info.update_time) < BLE_CONNECT_DELAY_UPDATE_PARAM) {
		ret = -EBUSY;
		goto update_exit;
	}

	ret = hostif_bt_conn_le_param_update(ble_info.ble_conn, param);
	if (ret == (-EALREADY)) {
		ret = 0;
	}

	if (!ret) {
		ble_info.update_time = os_uptime_get_32();
	}

	SYS_LOG_INF("Uparam ret %d int 0x%x~0x%x lat %d to %d", ret, param->interval_min,
				param->interval_max, param->latency, param->timeout);

update_exit:
	if (prio >= 0) {
		k_thread_priority_set(k_current_get(), prio);
	}

	return ret;
}

int bt_manager_ble_get_param(uint16_t *interval, uint16_t *latency, uint16_t *timeout)
{
	int prio = 0, ret = 0;

	prio = k_thread_priority_get(k_current_get());
	if (prio >= 0) {
		k_thread_priority_set(k_current_get(), -1);
	}

	if (!ble_info.ble_conn) {
		ret = -EIO;
		goto getp_exit;
	}

	if (ble_info.le_conn_param.interval_min == 0) {
		ret = -EINVAL;
	}

	if (interval) {
		*interval = ble_info.le_conn_param.interval_min;
	}

	if (latency) {
		*latency = ble_info.le_conn_param.latency;
	}

	if (timeout) {
		*timeout = ble_info.le_conn_param.timeout;
	}

getp_exit:
	if (prio >= 0) {
		k_thread_priority_set(k_current_get(), prio);
	}

	return ret;
}

int bt_manager_ble_get_phone_type(void)
{
	if (!ble_info.ble_conn) {
		return 0;
	}

	return ble_info.ios_phone;
}

int bt_manager_ble_is_br_busy(void)
{
	if (ble_info.br_a2dp_runing || ble_info.br_hfp_runing) {
		return 1;
	} else {
		return 0;
	}
}

#define ble_check_update_param(void)
#define ble_send_data_check_interval()
#else

#define BLE_CALC_TXRX_CNT (33)
static bool ble_update_txrx_status_busy(uint16_t pre, uint16_t cur)
{
	SYS_LOG_INF("pre %d, cur %d.", pre, cur);
	if ((cur > pre) && 
		/* (pre < (0xFFFF - BLE_CALC_TXRX_CNT)) &&*/ 
		(cur > pre + BLE_CALC_TXRX_CNT)) {
		return true;
	}

	if ((cur < pre) && 
		(cur < BLE_CALC_TXRX_CNT) /*&& 
		(pre > (0xFFFF - BLE_CALC_TXRX_CNT))*/) {
		return true;
	}

	return false;
}

static void param_update_work_callback(struct k_work *work)
{
    uint8_t req_conn_speed;
	uint16_t conn_rxtx_cnt;
	uint8_t conn_busy = 0;

#ifdef CONFIG_GATT_OVER_BREDR
	if (!is_le_conn(ble_info.ble_conn)) {
		ble_info.update_work_state = PARAM_UPDATE_IDLE_STATE;
		return;
	}
#endif
	if (ble_info.ble_conn) {

		if (ble_info.update_work_state == PARAM_UPDATE_RUNING) {
			ble_info.update_work_state = PARAM_UPDATE_IDLE_STATE;
			ble_info.conn_rxtx_cnt = hostif_bt_conn_get_rxtx_cnt(ble_info.ble_conn);
			os_delayed_work_submit(&ble_info.param_update_work, BLE_CONNECT_INTERVAL_CHECK);
			return;
		}

		if ((ble_info.update_work_state == PARAM_UPDATE_IDLE_STATE)
			    || (ble_info.update_work_state == PARAM_UPDATE_WAITO_UPDATE_STATE)){
			conn_rxtx_cnt = hostif_bt_conn_get_rxtx_cnt(ble_info.ble_conn);
			if (ble_info.conn_rxtx_cnt != conn_rxtx_cnt) {
				if (ble_update_txrx_status_busy(ble_info.conn_rxtx_cnt, conn_rxtx_cnt)) {
					conn_busy = 1;
				}
				ble_info.conn_rxtx_cnt = conn_rxtx_cnt;
			}
		}

		if (ble_info.br_a2dp_runing || ble_info.br_hfp_runing) {
            if(conn_busy){
				if(ble_info.ble_conn_speed == BLE_CONN_SPEED_FAST){
				    req_conn_speed = BLE_CONN_SPEED_BUSY;
				}
				else{
				    req_conn_speed = ble_info.ble_conn_speed;
				}
            }
            else{
                req_conn_speed = BLE_CONN_SPEED_IDLE;
            }
		}
		else if (conn_busy) {
            req_conn_speed = BLE_CONN_SPEED_FAST;
		}
		else {
            req_conn_speed = BLE_CONN_SPEED_IDLE;
		}

        if (req_conn_speed == ble_info.ble_conn_speed) {
			ble_info.update_work_state = PARAM_UPDATE_IDLE_STATE;
			ble_info.conn_rxtx_cnt = hostif_bt_conn_get_rxtx_cnt(ble_info.ble_conn);
			os_delayed_work_submit(&ble_info.param_update_work, BLE_CONNECT_INTERVAL_CHECK);
			return;
		}

        bt_manager_set_ble_connection_param(req_conn_speed);
		ble_info.update_work_state = PARAM_UPDATE_RUNING;
		os_delayed_work_submit(&ble_info.param_update_work, BLE_UPDATE_PARAM_FINISH_TIME);
	} else {
		ble_info.update_work_state = PARAM_UPDATE_IDLE_STATE;
	}
}

#ifndef CONFIG_BT_BLE_APP_UPDATE_PARAM
static int param_update_hrtimer_enable(bool enable,uint32_t time)
{
	printk("htimer en %d t %d\n", enable, time);

    if (enable) {
        hrtimer_start(&ble_info.s3_timer, 1000*time, 0);
        sys_s3_wksrc_set(SLEEP_WK_SRC_T1);
    } else {
	    hrtimer_stop(&ble_info.s3_timer);
    }

    return 0;
}

static void param_update_hrtimer_callback(struct hrtimer *timer, void *expiry_fn_arg)
{
	os_delayed_work_cancel(&ble_info.param_update_work);
	os_delayed_work_submit(&ble_info.param_update_work, 0);
}
#endif

static void ble_check_update_param(void)
{
	if (ble_info.update_work_state == PARAM_UPDATE_IDLE_STATE) {
		ble_info.update_work_state = PARAM_UPDATE_WAITO_UPDATE_STATE;
		os_delayed_work_submit(&ble_info.param_update_work, BLE_DELAY_UPDATE_PARAM_TIME);
	} else if (ble_info.update_work_state == PARAM_UPDATE_RUNING) {
		ble_info.update_work_state = PARAM_UPDATE_WAITO_UPDATE_STATE;
		os_delayed_work_submit(&ble_info.param_update_work, BLE_UPDATE_PARAM_FINISH_TIME);
	} else {
		/* Already in PARAM_UPDATE_WAITO_UPDATE_STATE */
	}
}

static void ble_send_data_check_interval(void)
{
#ifdef CONFIG_GATT_OVER_BREDR
	if (is_le_conn(ble_info.ble_conn))
#endif
	{
		if ((ble_info.ble_conn_speed != BLE_CONN_SPEED_FAST) &&
			!ble_info.br_a2dp_runing && !ble_info.br_hfp_runing) {
			ble_check_update_param();
		}
	}
}
#endif

void bt_manager_ble_update_param_wait(void)
{
#ifndef CONFIG_BT_BLE_APP_UPDATE_PARAM
	if (!ble_info.ble_conn) {
		return;
	}

	if (BLE_CONN_SPEED_IDLE != ble_info.ble_conn_speed) {
		os_delayed_work_cancel(&ble_info.param_update_work);
		param_update_hrtimer_enable(true, 2000);
	}
#endif
}

static void exchange_func(struct bt_conn *conn, uint8_t err,
			  struct bt_gatt_exchange_params *params)
{
    ble_info.mtu = bt_manager_get_ble_mtu();
	SYS_LOG_INF("Exchange %s mtu:%d", err == 0 ? "successful" : "failed",ble_info.mtu);
}

static struct bt_gatt_exchange_params exchange_params = {
	.func = exchange_func,
};

static void notify_ble_connected(uint8_t *mac, uint8_t connected)
{
	struct ble_reg_manager *le_mgr;

	os_mutex_lock(&ble_mgr_lock, OS_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER(&ble_list, le_mgr, node) {
		if (le_mgr->link_cb) {
			le_mgr->link_cb(ble_info.device_mac, connected);
		}
	}

	os_mutex_unlock(&ble_mgr_lock);
}

#ifdef CONFIG_BT_BLE_NOTIFY_PENDING
static void bt_manager_ble_notify_complete(struct k_work *work)
{
	/* Notify app ble send complete, can send next data. */
	//SYS_LOG_INF("pending_cb");
	return;
}

static void bt_manager_ble_pending_cb(struct bt_conn *conn, uint8_t pkts)
{
	if (ble_info.ble_send_pending) {
		ble_info.ble_send_pending = 0;
		os_delayed_work_submit(&ble_info.ble_pending_notify_work, 0);
	}
}
#endif

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[13];
	struct bt_conn_info info;

	if (err) {
		SYS_LOG_ERR("slave fail to connect(%u)", err);
		return;
	}

	if ((hostif_bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_LE) || (info.role != BT_HCI_ROLE_SLAVE)) {
		return;
	}

	if (!ble_info.ble_conn) {
#ifdef CONFIG_BR_SDP_ACTIVE_REGISTER
#ifdef CONFIG_GATT_OVER_BREDR
		if (info.type == BT_CONN_TYPE_LE) {
			void bt_unregister_gobr_sdp(void);
			bt_unregister_gobr_sdp();
		}
#endif
#endif
		memcpy(ble_info.device_mac, info.le.dst->a.val, 6);
		memset(addr, 0, 13);
		bin2hex(ble_info.device_mac, 6, addr, 12);
		SYS_LOG_INF("Ble connected MAC: %s inv %d lat %d timeout %d",
			addr, info.le.interval, info.le.latency, info.le.timeout);

        ble_set_clear_wake_lock(BLE_WAKE_LOCK_CONNECTED_NOTIFY, 1);
		os_delayed_work_submit(&ble_info.ble_connected_notify_work, BLE_CONNECTED_DELAY_NOTIFY);
		ble_info.ble_conn = hostif_bt_conn_ref(conn);
		ble_info.conn_rxtx_cnt = 0;
		ble_info.update_work_state = PARAM_UPDATE_WAITO_UPDATE_STATE;
		ble_info.ble_state = BT_STATUS_BLE_CONNECTED;
		ble_info.ble_conn_speed = BLE_CONN_NOINIT_INTERVAL;
		ble_info.ble_conn_param_update_err = 0;
		ble_info.le_conn_param.interval_min = info.le.interval;
		ble_info.le_conn_param.interval_max = info.le.interval;
		ble_info.le_conn_param.latency = info.le.latency;
		ble_info.le_conn_param.timeout = info.le.timeout;

		ble_set_clear_wake_lock(BLE_WAKE_LOCK_ADV_MANANGER, 1);
		os_delayed_work_submit(&ble_info.ble_advertise_manager_work, 0);

#ifdef CONFIG_BT_BLE_APP_UPDATE_PARAM
		ble_info.update_time = os_uptime_get_32();
#else
		os_delayed_work_submit(&ble_info.param_update_work, BLE_CONNECT_DELAY_UPDATE_PARAM);
#endif
#ifdef CONFIG_BT_BLE_NOTIFY_PENDING
		hostif_bt_conn_reg_tx_pending_cb(conn, bt_manager_ble_pending_cb);
#endif
#ifdef CONFIG_ACTIVE_REQ_BLE_ENC
		os_delayed_work_submit(&ble_info.ble_req_enc_work, BLE_DELAY_ENCRYPT_TIME);
#endif
	} else {
		SYS_LOG_ERR("Already connected");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[13];
	struct bt_conn_info info;
#ifndef CONFIG_BT_BLE_APP_UPDATE_PARAM
	bool result;
#endif
	os_mutex_lock(&ble_mgr_lock, OS_FOREVER);
	if ((hostif_bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_LE) || (info.role != BT_HCI_ROLE_SLAVE)) {
		os_mutex_unlock(&ble_mgr_lock);
		return;
	}

	if (ble_info.ble_conn == conn) {
#ifndef CONFIG_BT_BLE_APP_UPDATE_PARAM
		result = hrtimer_is_running(&ble_info.s3_timer);
		if (result) {
			param_update_hrtimer_enable(false, 0);
		}
		os_delayed_work_cancel(&ble_info.param_update_work);
#endif
#ifdef CONFIG_BT_BLE_NOTIFY_PENDING
		ble_info.ble_send_pending = 0;
#endif

		os_delayed_work_cancel(&ble_info.ble_connected_notify_work);
		ble_set_clear_wake_lock(BLE_WAKE_LOCK_CONNECTED_NOTIFY, 0);

		memset(addr, 0, sizeof(addr));
		bin2hex(ble_info.device_mac, 6, addr, 12);
		SYS_LOG_INF("Ble disconnected MAC: %s, reason: %d", addr, reason);

		hostif_bt_conn_unref(ble_info.ble_conn);
		ble_info.ble_conn = NULL;
		ble_info.ble_state = BT_STATUS_BLE_DISCONNECTED;
		ble_info.rssi = 0x7F;
		ble_info.ble_conn_param_update_err	= 0;
		notify_ble_connected(ble_info.device_mac, false);
		os_sem_give(&ble_ind_sem);

		ble_set_clear_wake_lock(BLE_WAKE_LOCK_ADV_MANANGER,1);
		if (!ble_info.ble_deinit) {
			os_delayed_work_submit(&ble_info.ble_advertise_manager_work, 0);
		}

#ifdef CONFIG_ACTIVE_REQ_BLE_ENC
		os_delayed_work_cancel(&ble_info.ble_req_enc_work);
#endif
#ifdef CONFIG_BR_SDP_ACTIVE_REGISTER
#ifdef CONFIG_GATT_OVER_BREDR
		if (info.type == BT_CONN_TYPE_LE) {
			void bt_register_gobr_sdp(void);
			bt_register_gobr_sdp();
		}
#endif
#endif
	} else {
		SYS_LOG_ERR("Error conn %p(%p)", ble_info.ble_conn, conn);
	}
	os_mutex_unlock(&ble_mgr_lock);
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	struct bt_conn_info info;

	if ((hostif_bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_LE) || (info.role != BT_HCI_ROLE_SLAVE)) {
		return false;
	}

	SYS_LOG_INF("int (0x%04x, 0x%04x) lat %d to %d", param->interval_min,
				param->interval_max, param->latency, param->timeout);
	return true;
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval,
			     uint16_t latency, uint16_t timeout,uint8_t status)
{
	struct bt_conn_info info;

	if ((hostif_bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_LE) || (info.role != BT_HCI_ROLE_SLAVE)) {
		return;
	}

	ble_info.le_conn_param.interval_min = interval;
	ble_info.le_conn_param.interval_max = interval;
	ble_info.le_conn_param.latency = latency;
	ble_info.le_conn_param.timeout = timeout;
	SYS_LOG_INF("status %d inv %d lat %d to %d", status, interval, latency, timeout);

#ifndef CONFIG_BT_BLE_APP_UPDATE_PARAM
	uint16_t inv = interval;
	uint16_t lat = latency;

	if (status) {
		SYS_LOG_ERR("status:%d count:%d",status,ble_info.ble_conn_param_update_err);
		ble_info.ble_conn_param_update_err++;
		if (ble_info.ble_conn_param_update_err >= BLE_UPDATE_PARAM_ERR_MAX) {
			os_delayed_work_cancel(&ble_info.param_update_work);
		}
		return;
	}

	ble_info.ble_conn_param_update_err = 0;

	if (lat) {
		do{
			inv += interval;
			lat--;
		}while(lat);
	}

	if (inv <= le_conn_param[ble_info.ios_phone][BLE_CONN_SPEED_FAST].interval_max) {
		ble_info.ble_conn_speed = BLE_CONN_SPEED_FAST;
	} else if (inv <= le_conn_param[ble_info.ios_phone][BLE_CONN_SPEED_BUSY].interval_max) {
		ble_info.ble_conn_speed = BLE_CONN_SPEED_BUSY;
	} else {
		ble_info.ble_conn_speed = BLE_CONN_SPEED_IDLE;
	}

	SYS_LOG_INF("ble_conn_speed %d", ble_info.ble_conn_speed);
#endif
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_req = le_param_req,
	.le_param_updated = le_param_updated,
};

#ifdef CONFIG_GATT_OVER_BREDR
static void bt_gobr_connected_cb(struct bt_conn *conn, bool connected)
{
	struct bt_conn_info info;

	SYS_LOG_INF("bt_gobr_connected_cb");

	SYS_LOG_INF("Gatt over BREDR %s\n", connected ? "connected" : "disconnected");

	if (connected) {
		hostif_bt_le_adv_stop();

		hostif_bt_conn_get_info(conn, &info);
		memcpy(ble_info.device_mac, info.br.dst->val, 6);

		ble_set_clear_wake_lock(BLE_WAKE_LOCK_CONNECTED_NOTIFY, 1);
		os_delayed_work_submit(&ble_info.ble_connected_notify_work, BLE_CONNECTED_DELAY_NOTIFY);
		ble_info.ble_conn = hostif_bt_conn_ref(conn);
		ble_info.ble_state = BT_STATUS_BLE_CONNECTED;
	} else {

		os_delayed_work_cancel(&ble_info.ble_connected_notify_work);
		ble_set_clear_wake_lock(BLE_WAKE_LOCK_CONNECTED_NOTIFY, 0);

		hostif_bt_conn_unref(ble_info.ble_conn);
		ble_info.ble_conn = NULL;
		ble_info.ble_state = BT_STATUS_BLE_DISCONNECTED;
		ble_info.rssi = 0x7F;

		notify_ble_connected(ble_info.device_mac, false);
		os_sem_give(&ble_ind_sem);
		ble_advertise();
	}
}

static u8_t bt_gatt_connect_status_cb(void)
{
	SYS_LOG_INF("bt_gatt_connect_status_cb");
	if (ble_info.ble_conn)
		return true;

	return false;
}
#endif

static int ble_notify_data(struct bt_gatt_attr *attr, uint8_t *data, uint16_t len)
{
	int ret;

	ret = hostif_bt_gatt_notify(ble_info.ble_conn, attr, data, len);
	if (ret < 0) {
		return ret;
	} else {
		return (int)len;
	}
}

static void ble_indicate_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *attr,
			  uint8_t err)
{
	os_sem_give(&ble_ind_sem);
}

static int ble_indicate_data(struct bt_gatt_attr *attr, uint8_t *data, uint16_t len)
{
	int ret;

	if (os_sem_take(&ble_ind_sem, OS_NO_WAIT) < 0) {
		return -EBUSY;
	}

	ble_ind_params.attr = attr;
	ble_ind_params.func = ble_indicate_cb;
	ble_ind_params.len = len;
	ble_ind_params.data = data;

	ret = hostif_bt_gatt_indicate(ble_info.ble_conn, &ble_ind_params);
	if (ret < 0) {
		return ret;
	} else {
		return (int)len;
	}
}

#ifndef CONFIG_BT_BLE_APP_UPDATE_PARAM
int bt_manager_set_ble_connection_param(uint8_t speed_mode)
{
    uint8_t phone_type = ble_info.ios_phone;

	if (!ble_info.ble_conn) {
		return -EIO;
	}

    if(speed_mode >= BLE_CONN_SPEED_MAX){
        return -EINVAL;
    }

	hostif_bt_conn_le_param_update(ble_info.ble_conn, &le_conn_param[phone_type][speed_mode]);

	SYS_LOG_INF("phone:%d speed %d",phone_type,speed_mode);
    return 0;
}
#endif

uint16_t bt_manager_get_ble_mtu(void)
{
	return (ble_info.ble_conn) ? hostif_bt_gatt_get_mtu(ble_info.ble_conn) : 0;
}

int bt_manager_get_ble_mac(bt_addr_le_t *le_addr)
{
	return hostif_bt_le_get_mac(le_addr);
}

int bt_manager_get_ble_state(void)
{
    return ble_info.ble_state;
}

bool bt_manager_is_app_enable_adv(void)
{
	return (ble_info.ble_app_enable_adv) ? true : false;
}

uint8_t bt_manager_get_ble_connected_dev_num(void)
{
    return (ble_info.ble_conn)? 1 : 0;
}

struct ble_mgr_info * bt_manager_get_ble_info(void)
{
    return &ble_info;
}

int bt_manager_get_ble_dev_rssi(int8_t *rssi)
{
	if (!ble_info.ble_conn) {
		return -EIO;
	}

	return hostif_bt_conn_read_rssi(ble_info.ble_conn,rssi);
}

int bt_manager_ble_send_data(struct bt_gatt_attr *chrc_attr,
					struct bt_gatt_attr *des_attr, uint8_t *data, uint16_t len)
{
	struct bt_gatt_chrc *chrc = (struct bt_gatt_chrc *)(chrc_attr->user_data);

	if (!ble_info.ble_conn) {
		return -EIO;
	}

#ifdef CONFIG_GATT_OVER_BREDR
	if (!ble_info.mtu)
	{
		// ble_info.mtu = CONFIG_BT_L2CAP_TX_MTU;
		ble_info.mtu = bt_manager_get_ble_mtu();
	}
#endif

	if (len > (ble_info.mtu - 3)) {
		SYS_LOG_ERR("len %d mtu %d", len, ble_info.mtu);
		return -EFBIG;
	}

#ifdef CONFIG_BT_BLE_NOTIFY_PENDING
	if (ble_info.ble_send_pending) {
		return -EBUSY;
	}

	if (0 == bt_manager_ble_ready_send_data()) {
		ble_info.ble_send_pending = 1;
		return -EBUSY;
	}
#else
	if (0 == bt_manager_ble_ready_send_data()) {
		return -EBUSY;
	}
#endif

	ble_send_data_check_interval();

	if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
		return ble_notify_data(des_attr, data, len);
	} else if (chrc->properties & BT_GATT_CHRC_INDICATE) {
		return ble_indicate_data(des_attr, data, len);
	}

	/* Wait TODO */
	/* return ble_write_data(attr, data, len) */
	SYS_LOG_WRN("Wait todo");
	return -EIO;
}

void bt_manager_ble_disconnect(void)
{
	int err;

	os_mutex_lock(&ble_mgr_lock, OS_FOREVER);

	if (!ble_info.ble_conn) {
		os_mutex_unlock(&ble_mgr_lock);
		return;
	}

#ifdef CONFIG_BT_BLE_NOTIFY_PENDING
	ble_info.ble_send_pending = 0;
#endif

#ifdef CONFIG_GATT_OVER_BREDR
	if (is_le_conn(ble_info.ble_conn))
#endif
	{
		hostif_bt_conn_ref(ble_info.ble_conn);
		err = hostif_bt_conn_disconnect(ble_info.ble_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		if (err) {
			SYS_LOG_INF("Disconnection failed (err %d)", err);
		}
		hostif_bt_conn_unref(ble_info.ble_conn);
		os_mutex_unlock(&ble_mgr_lock);
		return;
	}

	os_mutex_unlock(&ble_mgr_lock);
	SYS_LOG_INF("Wait todo!");
}

void bt_manager_ble_service_reg(struct ble_reg_manager *le_mgr)
{
	os_mutex_lock(&ble_mgr_lock, OS_FOREVER);

	sys_slist_append(&ble_list, &le_mgr->node);
	hostif_bt_gatt_service_register(&le_mgr->gatt_svc);

	os_mutex_unlock(&ble_mgr_lock);
}

int bt_manager_ble_ready_send_data(void)
{
	if (!ble_info.ble_conn) {
		return 0;
	}

	return hostif_bt_le_conn_ready_send_data(ble_info.ble_conn);
}


uint16_t bt_manager_ble_tx_pending_cnt(void)
{
	if (!ble_info.ble_conn) {
		return 0;
	}

	return hostif_bt_le_tx_pending_cnt(ble_info.ble_conn);
}

static void ble_wake_lock_check_work(struct k_work *work)
{
	static uint8_t check_idle_cnt = 0;
	static uint8_t get_wake_lock = 0;

	if (get_wake_lock) {
		if (!bt_manager_get_ble_wake_lock()) {
			check_idle_cnt++;
			if (check_idle_cnt > BLE_WAKE_LOCK_CHECK_TIMES) {
				bt_wake_unlock();
				get_wake_lock = 0;
				check_idle_cnt = 0;
				SYS_LOG_INF("Ble wake lock %d", get_wake_lock);
			}
		} else {
			check_idle_cnt = 0;
		}
	} else {
		if (bt_manager_get_ble_wake_lock()) {
			bt_wake_lock();
			get_wake_lock = 1;
			check_idle_cnt = 0;
			SYS_LOG_INF("Ble wake lock %d", get_wake_lock);
		}
	}

	os_delayed_work_submit(&ble_info.ble_wake_lock_work, BLE_WAKE_LOCK_CHECK_INTERVAL);
}

static void ble_connected_notify_work_callback(struct k_work *work)
{
    int8_t rssi;
    int ret;

    if(ble_info.ble_conn){
        notify_ble_connected(ble_info.device_mac, true);
        if (exchange_params.func) {
#ifdef CONFIG_GATT_OVER_BREDR
            if (is_le_conn(ble_info.ble_conn)) 
#endif
            {
                hostif_bt_gatt_exchange_mtu(ble_info.ble_conn, &exchange_params);
            }
        }

        ret = bt_manager_get_ble_dev_rssi(&rssi);
        if(!ret){
            ble_info.rssi = rssi;
        }
        SYS_LOG_INF("ret %d rssi %d",ret,rssi);
    }
    ble_set_clear_wake_lock(BLE_WAKE_LOCK_CONNECTED_NOTIFY,0);
}

static int bt_manager_ble_htimer_enable(bool enable,uint32_t time)
{
	SYS_LOG_INF("Ble htimer en %d t %d", enable, time);
#ifdef CONFIG_BT_BLE_IPS
	SYS_LOG_INF("avoid ips adv stop.");
	return 0;
#endif

    if (enable) {
        hrtimer_start(&ble_info.timer, 1000*time, 1000*time);
        sys_s3_wksrc_set(SLEEP_WK_SRC_T1);
    } else {
	    hrtimer_stop(&ble_info.timer);
    }

    return 0;
}

static void  ble_advertise_manager_work_callback(struct k_work *work)
{
	uint8_t adv_status = 0;
	uint8_t master_status = 0;
    int ret = -1;

	ble_set_clear_wake_lock(BLE_WAKE_LOCK_ADV_MANANGER, 0);

#ifdef CONFIG_BLE_MASTER
	master_status = le_master_get_scan_status();
#endif

	if (ble_info.ble_app_enable_adv == 0) {
		/* User not start ble advertise */
		return;
	}

	if (master_status) {
		if (ble_info.ble_state == BT_STATUS_BLE_NONE || ble_info.ble_state == BT_STATUS_BLE_DISCONNECTED) {
			/* Ble master busy now, delay check and start advertise. */
			bt_manager_ble_htimer_enable(true, BLE_ADV_DELAY_CHECK);
			return;
		} else if (ble_info.ble_state == BT_STATUS_BLE_ADV) {
			SYS_LOG_ERR("Ble adv in master busy state!");
		}
	}

    if ((ble_info.ble_state == BT_STATUS_BLE_NONE) || (ble_info.ble_state == BT_STATUS_BLE_DISCONNECTED)) {
        ble_info.ble_adv_speed = BLE_ADV_SPEED_FAST;
        ble_advertise();
		ble_info.ble_advertise_speed_change = false;
        bt_manager_ble_htimer_enable(true, BLE_ADV_FAST_MAX_TIME);
    } else if (ble_info.ble_state == BT_STATUS_BLE_ADV) {
        if (ble_info.ble_advertise_speed_change == true) {
            if (ble_info.ble_adv_speed != BLE_ADV_SPEED_SLOW) {
                hostif_bt_le_adv_stop();
                ble_info.ble_advertise_active = false;
                ble_info.ble_adv_speed = BLE_ADV_SPEED_SLOW;
                ble_set_clear_wake_lock(BLE_WAKE_LOCK_ADV_MANANGER,1);
                os_delayed_work_submit(&ble_info.ble_advertise_manager_work, 10);
            } else {
                ble_advertise();
                ble_info.ble_advertise_speed_change = false;
				ret = hrtimer_is_running(&ble_info.timer);
		        if (ret) {
                   bt_manager_ble_htimer_enable(false, 0);
                }
            }
        }
    } else {
        /* Wait TODO: BLE slave in connected state, if need adv after cnnected, wait to modify. */
        if ((ble_info.ble_advertise_active == true) || nconn_flag) {
            hostif_bt_le_adv_stop();
            ble_info.ble_advertise_active = false;

			/* The application determines whether to enable unreachable broadcasting */
			if(nconn_flag)
			{
				ble_advertise_v2();
			}
			
			ret = hrtimer_is_running(&ble_info.timer);
		    if (ret) {
               bt_manager_ble_htimer_enable(false,0);
			}
        }
    }

    hostif_bt_le_get_adv_status(&adv_status);
    SYS_LOG_INF("ble_state %d speed %d adv_status %d",ble_info.ble_state,
        ble_info.ble_adv_speed, adv_status);
}

static void bt_manager_ble_timer_callback(struct hrtimer *timer, void *expiry_fn_arg)
{
    ble_info.ble_advertise_speed_change = true;
    ble_set_clear_wake_lock(BLE_WAKE_LOCK_ADV_MANANGER, 1);
    os_delayed_work_submit(&ble_info.ble_advertise_manager_work, 0);
}

#ifdef CONFIG_ACTIVE_REQ_BLE_ENC
static void ble_req_enc_work_handler(struct k_work *work)
{
	if (ble_info.ble_conn) {
		if ((hostif_bt_conn_get_security(ble_info.ble_conn) < BT_SECURITY_L2) &&
			(!hostif_bt_conn_security_is_start(ble_info.ble_conn))) {
			hostif_bt_conn_set_security(ble_info.ble_conn, BT_SECURITY_L2);
		}
	}
}
#endif

//#define LE_AUTH_PASSKEY_DISPLAY		1

//#ifdef LE_AUTH_PASSKEY_DISPLAY
static void le_auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	struct bt_conn_info info;
	char addr[13];

	if ((hostif_bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_LE)){
		return;
	}

	memset(addr, 0, 13);
	bin2hex(info.le.dst->a.val, 6, addr, 12);
	SYS_LOG_INF("le addr: %s le passkey:%d", addr, passkey);
}

static void bt_mgr_ble_pairing_event(uint8_t event, void *param)
{
	struct app_msg  msg = {0};

	msg.type = MSG_BT_PAIRING_EVENT;
	msg.cmd = event;
	msg.value = (int)((uint32_t)param);

	send_async_msg("main", &msg);
	SYS_LOG_INF("param: %d",msg.value);
}

static void le_auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	struct bt_conn_info info;
	char addr[13];

	if ((hostif_bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_LE)){
		return;
	}

	hostif_bt_conn_auth_passkey_confirm(conn);

	memset(addr, 0, 13);
	bin2hex(info.le.dst->a.val, 6, addr, 12);
	SYS_LOG_INF("le addr: %s key:%d", addr, passkey);

	bt_mgr_ble_pairing_event(BTSRV_PAIR_PASSKEY_DISPLAY, (void *)passkey);
}
//#endif

static void le_auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	SYS_LOG_INF("le addr:%s",addr);
}

static void le_pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	struct bt_conn_info info;
	char addr[13];

	if ((hostif_bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_LE)){
		return;
	}

	memset(addr, 0, 13);
	bin2hex(info.le.dst->a.val, 6, addr, 12);
	SYS_LOG_INF("le addr %s reason %d", addr,reason);
}

static void le_pairing_complete(struct bt_conn *conn, bool bonded)
{
	struct bt_conn_info info;
	char addr[13];

	if ((hostif_bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_LE)){
		return;
	}

	memset(addr, 0, 13);
	bin2hex(info.le.dst->a.val, 6, addr, 12);
	SYS_LOG_INF("le addr %s bonded %d", addr, bonded);
}

/* IO_NO_INPUT_OUTPUT for le */
static struct bt_conn_auth_cb le_auth_cb_confirm = {
#ifdef LE_AUTH_PASSKEY_DISPLAY
	.passkey_display = le_auth_passkey_display,
	.passkey_confirm = le_auth_passkey_confirm,
#else
	.passkey_display = NULL,
	.passkey_confirm = NULL,
#endif
	.cancel = le_auth_cancel,
	.pairing_failed = le_pairing_failed,
	.pairing_complete = le_pairing_complete,
};

void bt_manager_ble_passkey_display(bool mode)
{
	// only for test.
	os_mutex_lock(&ble_mgr_lock, OS_FOREVER);
	if (mode) {
// #ifdef LE_AUTH_PASSKEY_DISPLAY
		le_auth_cb_confirm.passkey_display = le_auth_passkey_display;
		le_auth_cb_confirm.passkey_confirm = le_auth_passkey_confirm;
// #endif
	} else {
		le_auth_cb_confirm.passkey_display = NULL;
		le_auth_cb_confirm.passkey_confirm = NULL;
	}
	hostif_bt_conn_le_auth_cb_register(&le_auth_cb_confirm);
	os_mutex_unlock(&ble_mgr_lock);
}

void bt_manager_ble_init(uint8_t mode)
{
	memset(&ble_info, 0, sizeof(ble_info));
	ble_info.ble_state = BT_STATUS_BLE_NONE;
	ble_info.rssi = 0x7F;
#ifdef CONFIG_BT_BLE_NOTIFY_PENDING
	ble_info.ble_send_pending = 0;
	os_delayed_work_init(&ble_info.ble_pending_notify_work, bt_manager_ble_notify_complete);
#endif
	if (!mode) {
		sys_slist_init(&ble_list);
	}
	os_sem_init(&ble_ind_sem, 1, 1);
#ifndef CONFIG_BT_BLE_APP_UPDATE_PARAM
	os_delayed_work_init(&ble_info.param_update_work, param_update_work_callback);
	hrtimer_init(&ble_info.s3_timer, param_update_hrtimer_callback, NULL);
#endif
	os_delayed_work_init(&ble_info.ble_wake_lock_work, ble_wake_lock_check_work);
	os_delayed_work_init(&ble_info.ble_connected_notify_work, ble_connected_notify_work_callback);
	os_delayed_work_init(&ble_info.ble_advertise_manager_work, ble_advertise_manager_work_callback);
	hrtimer_init(&ble_info.timer, bt_manager_ble_timer_callback, NULL);
	os_delayed_work_submit(&ble_info.ble_wake_lock_work, BLE_WAKE_LOCK_CHECK_INTERVAL);

#ifdef CONFIG_OTA_BLE_MASTER_SUPPORT
	(void)conn_callbacks;
#else
	hostif_bt_conn_cb_register(&conn_callbacks);
#endif
#ifdef CONFIG_GATT_OVER_BREDR
	SYS_LOG_INF("gobr_reg");
	hostif_bt_gobr_reg_connected_cb(bt_gobr_connected_cb, bt_gatt_connect_status_cb);
#endif
	hostif_bt_conn_le_auth_cb_register(&le_auth_cb_confirm);
	ble_info.ble_conn_speed = BLE_CONN_NOINIT_INTERVAL;

	if (mode) {
		hostif_bt_le_adv_stop();
		bt_manager_reset_le_adv();
	}

#ifdef CONFIG_BT_ACTIONS_SUPER_SERVICE
	if (!mode) {
		ble_manager_super_register();
	}
#endif

#ifdef CONFIG_BT_ANCS_AMS
	ble_ancs_ams_init();
#endif

#ifdef CONFIG_ACTIVE_REQ_BLE_ENC
	os_delayed_work_init(&ble_info.ble_req_enc_work, ble_req_enc_work_handler);
#endif

#ifdef CONFIG_BLE_MASTER
#ifndef	CONFIG_OTA_BLE_MASTER_SUPPORT
	ble_master_init();
#endif
#endif
}


#ifdef CONFIG_BT_ANCS_AMS
void bt_manager_ble_ancs_ams_handle(uint8_t code, void* data)
{
    ble_ancs_ams_event_handle(code,data);
}
#endif

void bt_manager_ble_deinit(void)
{
	uint16_t time_out = 0;

	ble_info.ble_deinit = 1;

	if (bt_manager_ble_is_connected()) {
		bt_manager_ble_disconnect();
		while (bt_manager_ble_is_connected() && time_out++ < 200) {
			os_sleep(10);
		}

		os_sleep(50);
	}

#ifdef CONFIG_BT_MANAGER_DEINIT_SERVICE
	struct ble_reg_manager *le_mgr;
	sys_snode_t *prev_node = NULL;
	SYS_SLIST_FOR_EACH_CONTAINER(&ble_list, le_mgr, node) {
		hostif_bt_gatt_service_unregister(&le_mgr->gatt_svc);
        sys_slist_remove(&ble_list,prev_node,&le_mgr->node);
	}
#endif
	bt_manager_ble_adv_stop();

    bt_manager_ble_htimer_enable(false,0);

    ble_info.ble_delaywork_wake_lock = 0;

#ifndef CONFIG_BT_BLE_APP_UPDATE_PARAM
	os_delayed_work_cancel(&ble_info.param_update_work);
#endif
	os_delayed_work_cancel(&ble_info.ble_wake_lock_work);
	os_delayed_work_cancel(&ble_info.ble_connected_notify_work);
	os_delayed_work_cancel(&ble_info.ble_advertise_manager_work);

#ifdef CONFIG_ACTIVE_REQ_BLE_ENC
	os_delayed_work_cancel(&ble_info.ble_req_enc_work);
#endif
}

void bt_manager_ble_a2dp_play_notify(bool play)
{
	ble_info.br_a2dp_runing = (play) ? 1 : 0;
	ble_check_update_param();
}

void bt_manager_ble_hfp_play_notify(bool play)
{
	ble_info.br_hfp_runing = (play) ? 1 : 0;
	ble_check_update_param();
}

void bt_manager_ble_adv_stop(void)
{
	bool result = false;
	uint8_t adv_status = 0;

	if (ble_info.ble_app_enable_adv == 0) {
		/* Ble advertise not start */
		return;
	}
	os_mutex_lock(&ble_adv_mutex, OS_FOREVER);

	ble_info.ble_app_enable_adv = 0;
	hostif_bt_le_get_adv_status(&adv_status);
	SYS_LOG_INF("Ble stop adv, state %d %d", adv_status, ble_info.ble_state);

	if (ble_info.ble_state == BT_STATUS_BLE_ADV) {
		result = hrtimer_is_running(&ble_info.timer);
		if (result) {
			bt_manager_ble_htimer_enable(false, 0);
		}

		ble_set_clear_wake_lock(BLE_WAKE_LOCK_ADV_MANANGER, 0);
		os_delayed_work_cancel(&ble_info.ble_advertise_manager_work);
	}

	if (adv_status) {
		hostif_bt_le_adv_stop();
	}
	os_mutex_unlock(&ble_adv_mutex);

	return;
}

int bt_manager_ble_adv_start(void)
{
	os_mutex_lock(&ble_adv_mutex, OS_FOREVER);
#ifdef CONFIG_BLE_MASTER
	if (le_master_get_scan_status()) {
		SYS_LOG_INF("ble master busy");
		os_mutex_unlock(&ble_adv_mutex);
		return -EBUSY;		/* App need stop ble master scan before start advertise */
	}
#endif

	if (ble_info.ble_app_enable_adv) {
		SYS_LOG_INF("Ble adv already start");
		os_mutex_unlock(&ble_adv_mutex);
		return 0;
	} else {
		SYS_LOG_INF("Ble start adv");
	}

	ble_info.ble_app_enable_adv = 1;
	ble_advertise_init();

	os_mutex_unlock(&ble_adv_mutex);
	return 0;
}

void bt_manager_reset_le_adv(void)
{
    hostif_bt_le_adv_reset();
}

int bt_manager_get_ble_wake_lock(void)
{
#if 0
#ifndef CONFIG_BT_BLE_APP_UPDATE_PARAM
	if (ble_info.ble_conn && (ble_info.ble_conn_speed != BLE_CONN_SPEED_IDLE)) {
		return 1;
	}
#endif
#endif
    if (ble_info.ble_delaywork_wake_lock) {
        return 1;
    } else {
		return 0;
	}
}

int bt_manager_ble_find_linkkey(bt_addr_le_t *le_addr)
{
    return hostif_bt_le_find_linkkey(le_addr);
}

void bt_manager_ble_clear_linkkey(bt_addr_le_t *le_addr)
{
    return hostif_bt_le_clear_linkkey(le_addr);
}

uint8_t bt_manager_ble_get_link_key_num(void)
{
    return hostif_bt_le_get_linkkey_mum();
}


void bt_manager_ble_hid_report(uint16_t button)
{
#ifdef CONFIG_BT_HIDS
	uint8_t data[3];

	data[0] = button & 0xff;
	data[1] = (button >> 8) & 0xff;
	data[2] = 0;

    hid_send_input_report(ble_info.ble_conn, 1, 3, data);
	memset(data, 0, 3);
	hid_send_input_report(ble_info.ble_conn, 1, 3, data);
#endif
}

void bt_manager_ble_set_phone(bool ios)
{
    ble_info.ios_phone = ios ? 1 : 0;
}

bool bt_manager_ble_is_connected(void)
{
#ifdef CONFIG_GATT_OVER_BREDR
	if ((ble_info.ble_conn) && (is_le_conn(ble_info.ble_conn))) {
		return true;
	} else {
		return false;
	}
#endif
	return (ble_info.ble_conn) ? true : false;
}

void bt_manager_ble_dump_info(void)
{
	uint8_t adv_status;
	char addr[13];

	printk("Ble info wake_lock 0x%x\n", ble_info.ble_delaywork_wake_lock);
	printk("\t Ble %s\n", ble_info.ble_conn ? "connected" : "disconnected");

#ifdef CONFIG_BT_BLE_APP_UPDATE_PARAM
	printk("\t App update param\n");
#else
	printk("\t Ble manager update param\n");
#endif

#ifdef CONFIG_BT_BLE_NOTIFY_PENDING
	printk("\t Open ble notify pending\n");
#else
	printk("\t Close ble notify pending\n");
#endif

	printk("\t App enable adv %d\n", ble_info.ble_app_enable_adv);

	if (ble_info.ble_conn) {
		memset(addr, 0, 13);
		bin2hex(ble_info.device_mac, 6, addr, 12);
		printk("\t Ble connected MAC: %s sec %d\n", addr, hostif_bt_conn_get_security(ble_info.ble_conn));
		printk("\t inv %d lat %d to %d\n", ble_info.le_conn_param.interval_min,
				ble_info.le_conn_param.latency, ble_info.le_conn_param.timeout);
	} else {
		hostif_bt_le_get_adv_status(&adv_status);
		if (adv_status) {
			printk("\t Advertising speed %d interval %d\n", ble_info.ble_adv_speed, ble_adv_speed[ble_info.ble_adv_speed]);
		} else {
			printk("\t Advertising disable\n");
		}
	}

#ifdef CONFIG_BLE_MASTER
	ble_master_dump_conn_info();
#endif
}

void bt_manager_set_security(void)
{
	if (ble_info.ble_conn) {
		printk("bt_manager_set_security\n");
		if (hostif_bt_conn_get_security(ble_info.ble_conn) < BT_SECURITY_L2) {
			if (hostif_bt_conn_set_security(ble_info.ble_conn, BT_SECURITY_L2)) {
				printk("Failed to set security\n");
			}
		}
	}
}

