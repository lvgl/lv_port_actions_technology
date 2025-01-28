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
#include <sys_manager.h>
#include <btservice_api.h>
#include <srv_manager.h>
#include <acts_bluetooth/host_interface.h>
#include <property_manager.h>
#include <drivers/bluetooth/bt_drv.h>
#include "bt_porting_inner.h"
#ifdef CONFIG_PROPERTY
#include <property_manager.h>
#endif

#ifdef CONFIG_BT_TEMP_COMPENSATION
#include <board_cfg.h>
#include <drivers/adc.h>
#include <soc_pmu.h>
#include <drivers/hrtimer.h>
#endif

#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif

#ifdef CONFIG_TASK_WDT
#include <task_wdt_manager.h>
#endif

#define MAX_MGR_DEV 	CONFIG_BT_MAX_CONN

#define BT_RESET_SUPPORT (1)

//#define RUN_BQB_IN_SYSTEM_MODE 	1

struct bt_manager_info_t {
	uint16_t connected_phone_num:2;
	uint16_t trs_devices_num:2;
	uint16_t tws_mode:1;
	uint16_t playing:1;
	uint16_t inited:1;
	uint16_t btdev_err:1;
	uint16_t bt_TO_reboot:1;
	uint16_t bt_ready:1;
	uint16_t bt_state;
	uint8_t dis_reason;
	struct bt_mgr_dev_info dev[MAX_MGR_DEV];
	bd_address_t halt_addr[MAX_MGR_DEV];
	os_sem ready_sem;
#ifdef CONFIG_BT_TEMP_COMPENSATION
	uint16_t bt_temp_comp_stage:2;
	uint16_t temp_comp_runing_flag:1;
	int16_t  bt_comp_last_temp;
	os_delayed_work bt_temp_comp_timer;
	struct hrtimer temp_comp_hrtimer;
#endif
};

static struct bt_manager_info_t bt_mgr_info;

#if CONFIG_BT_BR_ACTS
/* BR inquiry/page scan interval/window
 * Range: 0x0011 to 0x1000
 * Time = N * 0.625 ms
 * Time Range: 10.625 ms to Page Scan Interval
 * Default: inquity scan interval(0x1000) window(0x0012)
 * Default: page scan interval(0x0800) window(0x0012)
 */
static const struct bt_scan_parameter default_scan_param = {
	.tws_limit_inquiry = 1,		/* 0: Normal inquiry,  other: limit inquiry */
	.idle_extend_windown = 0,
	.inquiry_interval = 0x1000,		/* Spec default value 0x1000 */
	.inquiry_windown = 0x0024,		/* Spec default value 0x12 */
	.page_interval = 0x0800,		/* Spec default value 0x0800 */
	.page_windown = 0x0024,			/* Spec default value 0x12 */
};

static const struct bt_scan_parameter enhance_scan_param = {
	.tws_limit_inquiry = 1,		/* 0: Normal inquiry,  other: limit inquiry */
	.idle_extend_windown = 0,
	.inquiry_interval = 0x1000,		/* Spec default value 0x1000 */
	.inquiry_windown = 0x0024,		/* Spec default value 0x12 */
	.page_interval = 0x0800,		/* Spec default value 0x0800 */
	.page_windown = 0x0024,			/* Spec default value 0x12 */
};

static void bt_mgr_set_config_info(void)
{
	struct btsrv_config_info cfg;

	memset(&cfg, 0, sizeof(cfg));
	cfg.max_conn_num = CONFIG_BT_MAX_BR_CONN;
	cfg.max_phone_num = bt_manager_config_connect_phone_num();
	cfg.pts_test_mode = bt_manager_config_pts_test() ? 1 : 0;
	cfg.volume_sync_delay_ms = bt_manager_config_volume_sync_delay_ms();
	cfg.tws_version = get_tws_current_versoin();
	cfg.tws_feature = get_tws_current_feature();
#ifdef CONFIG_BT_DOUBLE_PHONE_PREEMPTION_MODE
	cfg.double_preemption_mode = 1;
#else
	cfg.double_preemption_mode = 0;
#endif
	btif_base_set_config_info(&cfg);
}

/* User can change to different scan param at any time after bt ready.
 * bt_manager_set_visible(false);
 * bt_manager_set_connectable(false);
 * btif_br_set_scan_param(&scan_param, true);		// new parameter
 * bt_manager_set_visible(true);
 * bt_manager_set_connectable(true);
 */
static void bt_mgr_set_scan_param(void)
{
	struct bt_scan_parameter param;

	/* If not set default_scan_param, bt service will used default param from spec */
	memcpy(&param, &default_scan_param, sizeof(param));
	param.tws_limit_inquiry = bt_manager_config_get_tws_limit_inquiry();
	param.idle_extend_windown = bt_manager_config_get_idle_extend_windown();
	btif_br_set_scan_param(&param, false);

	memcpy(&param, &enhance_scan_param, sizeof(param));
	param.tws_limit_inquiry = bt_manager_config_get_tws_limit_inquiry();
	param.idle_extend_windown = bt_manager_config_get_idle_extend_windown();
	btif_br_set_scan_param(&param, true);
}
#endif

static int bt_mananger_btdev_error(int err)
{
	struct app_msg  msg = {0};

	if (!err) {
		return 0;
	}

#if BT_RESET_SUPPORT
	if (0 == bt_mgr_info.bt_ready) {
		printk("bt is not ok, bt_ready %d.", bt_mgr_info.bt_ready);
		return 0;
	}

	void bt_conn_unack_tx(void);
	bt_conn_unack_tx();

	bt_mgr_info.bt_TO_reboot = 0;
	msg.type = MSG_BT_DEVICE_ERROR;		/* bt module reset */
	send_async_msg("main", &msg);
#else
	bt_mgr_info.bt_TO_reboot = 1;
	msg.type = MSG_REBOOT;
	msg.cmd = REBOOT_REASON_HCI_TIMEOUT;
	send_async_msg("main", &msg);
#endif
	return 0;
}

#define CFG_BT_CRASH_CNT	"BT_CRASH_CNT"
int bt_manager_reset_btdev(void)
{
	int time_out = 0;
	uint8_t btreset_cnt = 0;

	if (0 == bt_mgr_info.bt_ready) {
		printk("bt is not ok, bt_ready %d.\n", bt_mgr_info.bt_ready);
		return 0;
	}

	/*********record the numbers of btcore crash **********/
	if (1 == property_get(CFG_BT_CRASH_CNT, (char *)&btreset_cnt, sizeof(btreset_cnt))) {
		if(btreset_cnt < 0xFF)
			btreset_cnt ++;
	} else {
		btreset_cnt = 1;
	}
	printk("btreset_cnt %d.\n", btreset_cnt);
	property_set(CFG_BT_CRASH_CNT, (char *)&btreset_cnt, sizeof(btreset_cnt));

	bt_mgr_info.bt_ready = 0;

#ifdef CONFIG_BT_TEMP_COMPENSATION
	os_delayed_work_cancel(&bt_mgr_info.bt_temp_comp_timer);
	if (hrtimer_is_running(&bt_mgr_info.temp_comp_hrtimer)) {
		hrtimer_stop(&bt_mgr_info.temp_comp_hrtimer);
	}
#endif

#if CONFIG_BT_BR_ACTS
	bt_manager_set_visible(false);
	bt_manager_set_connectable(false);
	btif_br_auto_reconnect_stop();
	if (btif_br_get_connected_device_num()) {
		btif_br_disconnect_device(BTSRV_DISCONNECT_ALL_MODE);
		
		while (btif_br_get_connected_device_num() && time_out++ < 100) {
			os_sleep(10);
		}
	}
#endif

	void bt_conn_release_all(void);
	bt_conn_release_all();

#ifdef CONFIG_BT_SPP
		bt_manager_spp_profile_stop();
#endif

#ifdef CONFIG_BT_A2DP
		bt_manager_a2dp_profile_stop();
#endif

#ifdef CONFIG_BT_AVRCP
		bt_manager_avrcp_profile_stop();
#endif

#ifdef CONFIG_BT_HFP_HF
		bt_manager_hfp_profile_stop();
		bt_manager_hfp_sco_stop();
#endif

#ifdef CONFIG_BT_HFP_AG
		bt_manager_hfp_ag_profile_stop();
#endif

#ifdef CONFIG_BT_HID
		bt_manager_hid_profile_stop();
#endif

#ifdef CONFIG_BT_PNP_INFO_SEARCH
		bt_manager_pnp_info_search_deinit();
#endif

	btif_stop();

	time_out = 0;
	while (srv_manager_check_service_is_actived(BLUETOOTH_SERVICE_NAME) && (time_out++ < 500)) {
		os_sleep(10);
	}

#ifdef CONFIG_BT_BLE
	bt_manager_ble_deinit();
#endif

	btdrv_reset();

	bt_mgr_info.inited = 0;
    bt_mgr_info.btdev_err = 1;

    bt_manager_init();

	return 0;

}

/* Better call by app after receive MSG_BT_ENGINE_READY message */
static void bt_mgr_start_open_bt(void)
{
#if CONFIG_BT_BR_ACTS
	//uint8_t a2dp_enable;
#endif
#ifdef CONFIG_BT_BLE
#ifndef CONFIG_OTA_BLE_MASTER_SUPPORT
	bt_manager_ble_adv_start();
#endif
#endif

#if CONFIG_BT_BR_ACTS
#ifdef CONFIG_BT_BREDR_DISABLE
	bt_manager_set_visible(false);
	bt_manager_set_connectable(false);
#else
	bt_manager_set_visible(true);
	bt_manager_set_connectable(true);
#endif

	if (!bt_manager_config_pts_test()) {
		/**only for power on disable a2dp/avrcp******/
		//if(property_get(CFG_BT_AUDIO_ENABLE, (char *)&a2dp_enable, sizeof(a2dp_enable)) >= 0) {
		//	if(!a2dp_enable)
		//		bt_manager_a2dp_disable();
		//}

		bt_manager_startup_reconnect();
	}
#endif
}

static void bt_mgr_btsrv_ready(int err)
{
#ifdef CONFIG_BT_BLE
    uint8_t ble_init_mode = 0;
#endif

#ifdef CONFIG_BT_A2DP_TRS
	struct bt_trs_list_dev_t dev_list[BT_A2DP_TRS_DEV_MAX];
#endif

	SYS_LOG_INF("err %d",err);

    if(err){
		bt_mananger_btdev_error(err);
		return;
    }
	
#if CONFIG_BT_BR_ACTS
	bt_mgr_set_config_info();
	bt_mgr_set_scan_param();
#endif

#ifdef CONFIG_TWS
	bt_manager_tws_init();
#endif

#ifdef CONFIG_BT_PNP_INFO_SEARCH
	bt_manager_pnp_info_search_init();
#endif

#ifndef CONFIG_OTA_PRODUCT_SUPPORT
#ifdef CONFIG_BT_A2DP
	bt_manager_a2dp_profile_start();
#endif
#ifdef CONFIG_BT_AVRCP
	bt_manager_avrcp_profile_start();
#endif

#ifdef CONFIG_BT_HFP_HF
	bt_manager_hfp_init();
	bt_manager_hfp_sco_init();

	bt_manager_hfp_sco_start();
	bt_manager_hfp_profile_start();
#endif

#ifdef CONFIG_BT_HFP_AG
	bt_manager_hfp_ag_init();
	bt_manager_hfp_ag_profile_start();
#endif
#endif

#ifdef CONFIG_BT_SPP
	bt_manager_spp_profile_start();
#endif

#ifdef CONFIG_BT_HID
	bt_manager_hid_register_sdp();
	bt_manager_hid_profile_start(5000, 1000);
	bt_manager_did_register_sdp();
#endif

	if(bt_mgr_info.btdev_err){
#if CONFIG_BT_BR_ACTS
		int btif_spp_sdp_restore(void);
		btif_spp_sdp_restore();
#endif
#ifdef CONFIG_BT_BLE
		ble_init_mode = 1;
#endif
		os_sleep(500);
	}
#ifdef CONFIG_BT_BLE
	bt_manager_ble_init(ble_init_mode);
#endif

#ifdef CONFIG_BT_A2DP_TRS
	bt_manager_get_trs_dev_info(dev_list,BT_A2DP_TRS_DEV_MAX);
#endif

	/* TODO: fixed me, temp to send message to main */
    if (!bt_mgr_info.btdev_err) {
        struct app_msg  msg = {0};

        msg.type = MSG_BT_ENGINE_READY;
        send_async_msg("main", &msg);
		os_sem_give(&bt_mgr_info.ready_sem);
    } else {
		bt_mgr_info.btdev_err = 0;
    }

	bt_mgr_start_open_bt();
	bt_mgr_info.bt_ready = 1;
}

#if CONFIG_BT_BR_ACTS
static void bt_mgr_add_dev_info(bd_address_t *addr, uint16_t hdl)
{
	int i;

	for (i = 0; i < MAX_MGR_DEV; i++) {
		if (bt_mgr_info.dev[i].used && !memcmp(&bt_mgr_info.dev[i].addr, addr, sizeof(bd_address_t))) {
			SYS_LOG_WRN("Already exist!\n");
			return;
		}
	}

	for (i = 0; i < MAX_MGR_DEV; i++) {
		if (bt_mgr_info.dev[i].used == 0) {
			bt_mgr_info.dev[i].used = 1;
			memcpy(&bt_mgr_info.dev[i].addr, addr, sizeof(bd_address_t));
			bt_mgr_info.dev[i].hdl = hdl;
#ifdef CONFIG_BT_HID
			os_delayed_work_init(&bt_mgr_info.dev[i].hid_delay_work, bt_manager_hid_delay_work);
#endif
			return;
		}
	}

	SYS_LOG_WRN("Without new dev info!\n");
}

static void bt_mgr_free_dev_info(struct bt_mgr_dev_info *info)
{
#ifdef CONFIG_BT_HID
	os_delayed_work_cancel(&info->hid_delay_work);
	if (info->hid_wake_lock) {
		bt_wake_unlock();
		info->hid_wake_lock = 0;
	}
#endif
	memset(info, 0, sizeof(struct bt_mgr_dev_info));
}

static struct bt_mgr_dev_info *bt_mgr_find_dev_info(bd_address_t *addr)
{
	int i;

	for (i = 0; i < MAX_MGR_DEV; i++) {
		if (bt_mgr_info.dev[i].used && !memcmp(&bt_mgr_info.dev[i].addr, addr, sizeof(bd_address_t))) {
			return &bt_mgr_info.dev[i];
		}
	}

	return NULL;
}

struct bt_mgr_dev_info *bt_mgr_find_dev_info_by_hdl(uint16_t hdl)
{
	int i;

	for (i = 0; i < MAX_MGR_DEV; i++) {
		if (bt_mgr_info.dev[i].used && bt_mgr_info.dev[i].hdl == hdl) {
			return &bt_mgr_info.dev[i];
		}
	}

	return NULL;
}

bool bt_mgr_check_dev_type(uint8_t type, uint16_t hdl)
{
	struct bt_mgr_dev_info *info = bt_mgr_find_dev_info_by_hdl(hdl);

	if (!info) {
		return false;
	}

	if (type == BTSRV_DEVICE_ALL) {
		return true;
	} else if (type == BTSRV_DEVICE_PHONE &&
				info->is_tws == 0 && info->is_trs == 0) {
		return true;
	} else if (type == BTSRV_DEVICE_TWS && info->is_tws) {
		return true;
	} else if (type == BTSRV_DEVICE_PLAYER && info->is_trs) {
		return true;
	}

	return false;
}

static void bt_mgr_notify_connected(struct bt_mgr_dev_info *info)
{
	if (info->is_tws || info->is_trs || info->notify_connected) {
		/* Tws not notify in here, or already notify */
		return;
	}

	SYS_LOG_INF("btsrv connected:%s\n", (char *)info->name);
	info->notify_connected = 1;
	bt_manager_set_status(BT_STATUS_CONNECTED);

	/* Advise not to set, just let phone make dicision. */
	//btif_br_set_phone_controler_role(&info->addr, CONTROLER_ROLE_MASTER);	/* Set phone controler as master */
	//btif_br_set_phone_controler_role(&info->addr, CONTROLER_ROLE_SLAVE);		/* Set phone controler as slave */
#ifdef CONFIG_BT_MAP_CLIENT
	/**only for get datatime*/
	if (btmgr_map_time_client_connect(&info->addr)) {
		SYS_LOG_INF("bt map time connected:%s\n", (char *)info->name);
	} else {
		SYS_LOG_INF("bt map time connected:%s failed\n", (char *)info->name);
	}
#endif
}

static void bt_mgr_check_disconnect_notify(struct bt_mgr_dev_info *info, uint8_t reason)
{
	if (info->is_tws || info->is_trs) {
		/* Tws not notify in here */
		return;
	}

	if (info->notify_connected) {
		SYS_LOG_INF("btsrv disconnected reason %d\n", reason);
		info->notify_connected = 0;
		bt_mgr_info.dis_reason = reason;		/* Transfer to BT_STATUS_DISCONNECTED */
		bt_manager_set_status(BT_STATUS_DISCONNECTED);
	}
}

/* return 0: accept connect request, other: rejuect connect request
 * Direct callback from bt stack, can't do too much thing in this function.
 */
static int bt_mgr_check_connect_req(struct bt_link_cb_param *param)
{
	if (param->new_dev) {
		SYS_LOG_INF("New connect request\n");
	} else {
		SYS_LOG_INF("%s connect request\n", param->is_tws ? "Tws" : (param->is_trs ? "Device" : "Phone"));
	}

	return 0;
}
#endif

/* Sample code, just for reference */
#ifdef CONFIG_BT_DOUBLE_PHONE_EXT_MODE
#define SUPPORT_CHECK_DISCONNECT_NONACTIVE_DEV		1
#else
#define SUPPORT_CHECK_DISCONNECT_NONACTIVE_DEV		0
#endif

#if SUPPORT_CHECK_DISCONNECT_NONACTIVE_DEV
static void bt_mgr_check_disconnect_nonactive_dev(struct bt_mgr_dev_info *info)
{
	int i, phone_cnt = 0, tws_cnt = 0, trs_cnt = 0;
	bd_address_t a2dp_active_addr;
	struct bt_mgr_dev_info *exp_disconnect_info = NULL;

	btif_a2dp_get_active_mac(&a2dp_active_addr);

	for (i = 0; ((i < MAX_MGR_DEV) && bt_mgr_info.dev[i].used); i++) {
		if (bt_mgr_info.dev[i].is_tws) {
			tws_cnt++;
		} else if (bt_mgr_info.dev[i].is_trs) {
			trs_cnt++;
		} else {
			phone_cnt++;
			if (memcmp(&bt_mgr_info.dev[i].addr, &a2dp_active_addr, sizeof(bd_address_t)) &&
				memcmp(&bt_mgr_info.dev[i].addr, &info->addr, sizeof(bd_address_t))) {
				exp_disconnect_info = &bt_mgr_info.dev[i];
			}
		}
	}

	/* Tws Trs paired */
	if (tws_cnt || trs_cnt) {
		if (phone_cnt >= 2) {
			bt_manager_br_disconnect(&info->addr);
		}
		return;
	}

	if (phone_cnt >= 3) {
		if (exp_disconnect_info) {
			bt_manager_br_disconnect(&exp_disconnect_info->addr);
		}
	}
}
#endif

#ifdef CONFIG_BT_A2DP_TRS
int bt_manager_trs_get_connected_dev_num(void)
{
	return bt_mgr_info.trs_devices_num;
}

static void bt_mgr_trs_check_connect_notify(struct bt_mgr_dev_info *info)
{
	if (info->is_trs == 0) {
		return;
	}

	if(info->notify_connected == 0) {
		SYS_LOG_INF("bt trs connected\n");
		bt_mgr_info.trs_devices_num++;
		info->notify_connected = 1;
	}
}

static void bt_mgr_trs_check_disconnect_notify(struct bt_mgr_dev_info *info, uint8_t reason)
{
	if (info->is_trs == 0) {
		return;
	}

	if (info->notify_connected) {
		SYS_LOG_INF("bt trs disconnected reason %d\n", reason);
		bt_mgr_info.trs_devices_num--;
		info->notify_connected = 0;
	}
}

static void bt_mgr_trs_a2dp_disconnect_notify(struct bt_mgr_dev_info *info)
{
	if (info->a2dp_connected && info->is_trs) {
		bt_manager_event_notify(BT_TRS_A2DP_STREAM_CLOSE_EVENT, NULL, 0);
	}
}

int8_t bt_manager_get_trs_dev_info(struct bt_trs_list_dev_t *device_list,uint8_t max)
{
    uint8_t num,i;
    if(!device_list){
        return -EIO;
    }
    num = btif_br_get_trs_list_info(device_list,max);
    if(num > 0){
	    for (i = 0; i < num; i++) {
            SYS_LOG_INF("addr: %x %x %x %x %x %x name:%s",
				device_list[i].addr.val[0],
				device_list[i].addr.val[1],
				device_list[i].addr.val[2],
				device_list[i].addr.val[3],
				device_list[i].addr.val[4],
				device_list[i].addr.val[5],
				device_list[i].name);
		}
    }
    return num;
}

int bt_manager_clear_trs_dev_info(bd_address_t *bd)
{
    if(!bd){
        return -EIO;
    }

    return btif_br_clean_trs_dev_info(bd);
}
#endif

#if CONFIG_BT_BR_ACTS
static int bt_mgr_link_event(void *param)
{
	int ret = 0;
	int8_t rssi;

	struct bt_mgr_dev_info *info;
	struct bt_link_cb_param *in_param = param;

	SYS_LOG_INF("Link event(%d) %02x:%02x:%02x:%02x:%02x:%02x\n", in_param->link_event,
			in_param->addr->val[5], in_param->addr->val[4], in_param->addr->val[3],
			in_param->addr->val[2], in_param->addr->val[1], in_param->addr->val[0]);

	info = bt_mgr_find_dev_info(in_param->addr);
	if ((info == NULL) && (in_param->link_event != BT_LINK_EV_ACL_CONNECTED) &&
		(in_param->link_event != BT_LINK_EV_ACL_CONNECT_REQ)) {
		SYS_LOG_WRN("Already free %d\n", in_param->link_event);
		return ret;
	}

	switch (in_param->link_event) {
	case BT_LINK_EV_ACL_CONNECT_REQ:
		ret = bt_mgr_check_connect_req(in_param);
		break;
	case BT_LINK_EV_ACL_CONNECTED:
		bt_mgr_add_dev_info(in_param->addr, in_param->hdl);
		bt_manager_get_actived_dev_rssi(&rssi);
#ifdef CONFIG_OTA_PRODUCT_SUPPORT
		void ota_product_ota_spp_connect(void);
		ota_product_ota_spp_connect();
#endif
		break;
	case BT_LINK_EV_ACL_DISCONNECTED:
#ifdef CONFIG_BT_A2DP_TRS
		bt_mgr_trs_a2dp_disconnect_notify(info);
		bt_mgr_trs_check_disconnect_notify(info, in_param->reason);
#endif
		bt_mgr_check_disconnect_notify(info, in_param->reason);
		bt_mgr_free_dev_info(info);
		break;
	case BT_LINK_EV_GET_NAME:
		info->name = in_param->name;
		info->is_tws = in_param->is_tws;
#ifdef CONFIG_BT_A2DP_TRS
		info->is_trs = in_param->is_trs;
#endif
#if SUPPORT_CHECK_DISCONNECT_NONACTIVE_DEV
		bt_mgr_check_disconnect_nonactive_dev(info);
#endif
		break;
	case BT_LINK_EV_HF_CONNECTED:
#ifdef CONFIG_BT_A2DP_TRS
		info->is_trs = in_param->is_trs;
		bt_mgr_trs_check_connect_notify(info);
#endif
		bt_mgr_notify_connected(info);
		info->hf_connected = 1;
		break;
	case BT_LINK_EV_HF_DISCONNECTED:
		info->hf_connected = 0;
		break;
	case BT_LINK_EV_A2DP_CONNECTED:
		info->a2dp_connected = 1;
#ifdef CONFIG_BT_A2DP_TRS
		info->is_trs = in_param->is_trs;
		bt_mgr_trs_check_connect_notify(info);
#endif
		bt_mgr_notify_connected(info);
		break;
	case BT_LINK_EV_A2DP_DISCONNECTED:
#ifdef CONFIG_BT_A2DP_TRS
		bt_mgr_trs_a2dp_disconnect_notify(info);
#endif
		info->a2dp_connected = 0;
		break;
	case BT_LINK_EV_AVRCP_CONNECTED:
		info->avrcp_connected = 1;
		break;
	case BT_LINK_EV_AVRCP_DISCONNECTED:
		info->avrcp_connected = 0;
		break;
	case BT_LINK_EV_SPP_CONNECTED:
		info->spp_connected++;
		break;
	case BT_LINK_EV_SPP_DISCONNECTED:
		if (info->spp_connected) {
			info->spp_connected--;
		}
		break;
	case BT_LINK_EV_HID_CONNECTED:
		info->hid_connected = 1;
#ifdef CONFIG_BT_HID
		if (!info->hid_wake_lock) {
			bt_wake_lock();
			info->hid_wake_lock = 1;
		}
		bt_manager_hid_connected_check_work(info->hdl);
#endif
		break;
	case BT_LINK_EV_HID_DISCONNECTED:
		info->hid_connected = 0;
#ifdef CONFIG_BT_HID
		if (info->hid_wake_lock) {
			bt_wake_unlock();
			info->hid_wake_lock = 0;
		}
#endif
		break;
	default:
		break;
	}

	return ret;
}

/* Return 0: phone device; other : tws device
 * Direct callback from bt stack, can't do too much thing in this function.
 */
static int bt_mgr_check_new_device_role(void *param)
{
	struct btsrv_check_device_role_s *cb_param = param;
	uint8_t pre_mac[3];

	if (bt_manager_config_get_tws_compare_high_mac()) {
		bt_manager_config_set_pre_bt_mac(pre_mac);
		if (cb_param->addr.val[5] != pre_mac[0] ||
			cb_param->addr.val[4] != pre_mac[1] ||
			cb_param->addr.val[3] != pre_mac[2]) {
			return 0;
		}
	}

#ifdef CONFIG_PROPERTY
	uint8_t name[33];
	memset(name, 0, sizeof(name));
	property_get(CFG_BT_NAME, name, 32);
	if (strlen(name) != cb_param->len || memcmp(cb_param->name, name, cb_param->len)) {
		return 0;
	}
#endif

	return 1;
}

static void bt_mgr_btsrv_pairing_event(uint8_t event,void* param)
{
	struct app_msg  msg = {0};

	msg.type = MSG_BT_PAIRING_EVENT;
    msg.cmd = event;
    msg.value = (int)((uint32_t)param);

	send_async_msg("main", &msg);
	SYS_LOG_INF("param: %d",msg.value);
}
#endif

static int _bt_mgr_callback(btsrv_event_e event, void *param)
{
	int ret = 0;

	switch (event) {
	case BTSRV_READY:
		bt_mgr_btsrv_ready((int)param);
		break;
		
#if CONFIG_BT_BR_ACTS
	case BTSRV_LINK_EVENT:
		ret = bt_mgr_link_event(param);
		break;
	case BTSRV_DISCONNECTED_REASON:
		bt_manager_disconnected_reason(param);
		break;
	case BTSRV_REQ_HIGH_PERFORMANCE:
	#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
		dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "btmanager");
	#endif
	break;
	case BTSRV_RELEASE_HIGH_PERFORMANCE:
	#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
		dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "btmanager");
	#endif
		break;
	case BTSRV_REQ_FLUSH_PROPERTY:
		SYS_LOG_INF("Req flush %s\n", (char *)param);
#ifdef CONFIG_PROPERTY
		//property_flush_req(param);
		property_flush(param);			/* Direct flush */
#endif
		break;
	case BTSRV_CHECK_NEW_DEVICE_ROLE:
		ret = bt_mgr_check_new_device_role(param);
		break;
	case BTSRV_PAIR_PASSKEY_DISPLAY	:
		bt_mgr_btsrv_pairing_event(BTSRV_PAIR_PASSKEY_DISPLAY,param);
		break;
	case BTSRV_PAIRING_COMPLETE:
		bt_mgr_btsrv_pairing_event(BTSRV_PAIRING_COMPLETE,param);
		break;
	case BTSRV_PAIRING_FAIL:
		bt_mgr_btsrv_pairing_event(BTSRV_PAIRING_FAIL,NULL);
		break;
	case BTSRV_BR_RESOLVE_ADDR:
		bt_manager_br_resolve_connect(param);
		break;
#endif
    default:
		break;
    }

	return ret;
}

#if CONFIG_BT_BR_ACTS
void bt_manager_record_halt_phone(void)
{
	uint8_t i, record = 0;

	memset(bt_mgr_info.halt_addr, 0, sizeof(bt_mgr_info.halt_addr));
	for (i = 0; i < MAX_MGR_DEV; i++) {
		if (bt_mgr_info.dev[i].used && !bt_mgr_info.dev[i].is_tws && !bt_mgr_info.dev[i].is_trs &&
			(bt_mgr_info.dev[i].a2dp_connected || bt_mgr_info.dev[i].hf_connected)) {
			memcpy(&bt_mgr_info.halt_addr[record], &bt_mgr_info.dev[i].addr, sizeof(bd_address_t));
			record++;
		}
	}
}

void *bt_manager_get_halt_phone(uint8_t *halt_cnt)
{
	*halt_cnt = MAX_MGR_DEV;
	return bt_mgr_info.halt_addr;
}

int bt_manager_get_status(void)
{
	return bt_mgr_info.bt_state;
}

int bt_manager_get_connected_dev_num(void)
{
	return bt_mgr_info.connected_phone_num;
}

int bt_manager_set_status(int state)
{
	switch (state) {
	case BT_STATUS_CONNECTED:
	{
		bt_mgr_info.connected_phone_num++;
		if (bt_mgr_info.connected_phone_num == 1) {
			sys_event_notify(SYS_EVENT_BT_CONNECTED);
			bt_manager_event_notify(BT_CONNECTION_EVENT, NULL, 0);
		} else {
			sys_event_notify(SYS_EVENT_2ND_CONNECTED);
		}
		break;
	}
	case BT_STATUS_DISCONNECTED:
	{
		if (bt_mgr_info.connected_phone_num > 0) {
			bt_mgr_info.connected_phone_num--;
			if (bt_mgr_info.dis_reason != 0x16) {
				sys_event_notify(SYS_EVENT_BT_DISCONNECTED);
			}

			if (!bt_mgr_info.connected_phone_num) {
				sys_event_notify(SYS_EVENT_BT_UNLINKED);
				bt_manager_event_notify(BT_DISCONNECTION_EVENT, NULL, 0);
			}
		}
		break;
	}
#ifdef CONFIG_TWS
	case BT_STATUS_TWS_PAIRED:
	{
		if (!bt_mgr_info.tws_mode) {
			bt_mgr_info.tws_mode = 1;
			if (btif_tws_get_dev_role() == BTSRV_TWS_MASTER) {
				sys_event_notify(SYS_EVENT_TWS_CONNECTED);
			}
			bt_manager_event_notify(BT_TWS_CONNECTION_EVENT, NULL, 0);
		}
        break;
	}
	case BT_STATUS_TWS_UNPAIRED:
	{
		if (bt_mgr_info.tws_mode) {
			bt_mgr_info.tws_mode = 0;
			bt_manager_event_notify(BT_TWS_DISCONNECTION_EVENT, NULL, 0);
		}
        break;
	}
#endif
	case BT_STATUS_MASTER_WAIT_PAIR:
	{
		/* Check can_do_pair before call
		 * bt_manager_set_status(BT_STATUS_MASTER_WAIT_PAIR)
		 */
		sys_event_notify(SYS_EVENT_TWS_START_PAIR);
        break;
	}
	default:
		break;
	}

	bt_mgr_info.bt_state = state;
	return 0;
}

void bt_manager_clear_list(int mode)
{
	btif_br_clear_list(mode);
	sys_event_notify(SYS_EVENT_CLEAR_PAIRED_LIST);
}

void bt_manager_set_stream_type(uint8_t stream_type)
{
#ifdef CONFIG_TWS
	bt_manager_tws_set_stream_type(stream_type);
#endif
}

void bt_manager_set_codec(uint8_t codec)
{
#ifdef CONFIG_TWS
	bt_manager_tws_set_codec(codec);
#endif
}

int bt_manager_br_connect(bd_address_t *bd)
{
	return btif_br_connect(bd);
}

int bt_manager_br_disconnect(bd_address_t *bd)
{
	return btif_br_disconnect(bd);
}
#endif

static void bt_manager_set_bt_drv_param(void)
{
	btdrv_init_param_t param;

	memset(&param, 0, sizeof(param));
	param.set_hosc_cap = 1;
	param.hosc_capacity = 0x64;			/* Wait todo: get for config or nvram */
	param.set_max_rf_power = 1;
	param.bt_max_rf_tx_power = 38;		/* 8db */
	//param.set_ble_rf_power = 1;
	//param.ble_rf_tx_power = 34;

	btdrv_set_init_param(&param);
}

#ifdef CONFIG_BT_CTRL_BQB
/* Notify: Customer can use different way to check enter bqb. */
#define CFG_BT_BQB_MODE			"BT_BQB_MODE"

static uint8_t bt_manager_get_bqb_mode(void)
{
#ifdef CONFIG_PROPERTY
	int ret = -EIO;
	uint8_t value = 0;

	ret = property_get(CFG_BT_BQB_MODE, (char *)&value, 1);
	if (ret != 1) {
		return DISABLE_TEST;
	} else {
		value -= 0x30;
	}

	if (value <= DUT_LE_TEST) {
		return value;
	} else {
		return DISABLE_TEST;
	}
#else
	return DISABLE_TEST;
#endif
}

int pmuadc_mode_switch(bool always_on);
#ifndef CONFIG_SOC_NO_PSRAM
static inline void sys_writel(unsigned int val, unsigned long reg)
{
	*(volatile unsigned int *)(reg) = val;
}
#endif
__in_section_unique(ram.noinit.bqb) uint32_t reg_data;

static int bt_manager_check_enter_bqb(void)
{
	uint8_t bqb_mode = bt_manager_get_bqb_mode();
	SYS_LOG_INF("Get bqb mode %d", bqb_mode);

	if (bqb_mode != DISABLE_TEST) {
		/* BQB mode [0:BR BQB Test, 1:BLE BQB Test, 2:BR/BLE dual BQB Test] */
		extern int bqb_init(int bqb_mode);

		pmuadc_mode_switch(1);

#ifndef CONFIG_SOC_NO_PSRAM
		uint32_t flags;
		uint32_t i;

		flags = irq_lock();
		k_sched_lock();
		/* GPIO36 ~ GPIO48, Psram IO*/
		for (i = 0; i < 13; i++)
			sys_writel(0x2 | (0 <<12), 0x40068090 + (i * 4));// 0x0: Drive level1

#ifdef CONFIG_SPI_FLASH_ACTS
		/* Psram Device set drive level3*/
#ifdef CONFIG_SLEEP_FUNC_IN_SRAM
#define __sleepfunc	__attribute__((noinline))			\
					__attribute__((long_call, section(".sleepfunc")))
#else
#define __sleepfunc __ramfunc
#endif
		__sleepfunc void   __psram_reg_write(unsigned int reg_addr, unsigned int reg_data);
		__sleepfunc unsigned int __psram_reg_read(unsigned int reg_addr);


		reg_data = __psram_reg_read(0);
		//printk("psram reg addr0 data 0x%x.\n", reg_data);
		if(soc_boot_is_mini()) {
			//
			reg_data &= (~(0x7 << 12));
			reg_data |= (0x3 << 12);
			__psram_reg_write(0, reg_data);
			k_busy_wait(3000);
		} else {
			reg_data &= (~0x3);
			reg_data |= 0x3;
			__psram_reg_write(0, reg_data);
			k_busy_wait(3000);
			//printk("WRITE PSRAM 0x%x.\n", reg_data);
		}

		reg_data = __psram_reg_read(0);
		printk("adjust psram reg addr0 data 0x%x.\n", reg_data);
#endif
		k_sched_unlock();
		irq_unlock(flags);
#endif

		bt_wake_lock();
		bqb_init((bqb_mode - 1));
		SYS_LOG_INF("Enter BQB mode %d", (bqb_mode - 1));

#ifndef RUN_BQB_IN_SYSTEM_MODE
#ifdef CONFIG_TASK_WDT
		task_wdt_stop();
#endif
		while(true){
			os_sleep(1000);
		}
#endif

		return 1;
	}
	return 0;
}
#endif

#ifdef CONFIG_BT_ACTIONS_SUPER_SERVICE
uint8_t bt_manager_check_pincode_mode(void)
{
#ifdef CONFIG_PROPERTY
	int ret = -EIO;
	uint8_t value = 0;

	ret = property_get(CFG_BT_PINCODE_MODE, (char *)&value, 1);
	if (ret != 1) {
		return 0;
	} 

	if (value == 1) {
		return 1;
	} else {
		return 0;
	}
#else
	return 0;
#endif
}
#endif

#ifdef CONFIG_BT_TEMP_COMPENSATION
#define BT_TEMP_COMP_HRTIME_CHECK_TIME		(1000*1000*60*5)	/* 300s */
#define BT_TEMP_COMP_CHECK_TIMER_MS 		1000

/* soc_pmu_get_temperature used too much time,
 * tmp use individual work_q, better add one callback funciton to get temperature.
 */
static OS_THREAD_STACK_DEFINE(temp_comp_stack, 1024);
static os_work_q temp_comp_work_q;

int bt_manager_bt_set_apll_temp_comp(uint8_t enable)
{
	if (bt_mgr_info.bt_ready) {
	    return btif_bt_set_apll_temp_comp(enable);
	}
    else{
        extern int bt_bqb_vs_set_apll_temp_comp(uint8_t enable);
        return bt_bqb_vs_set_apll_temp_comp(enable);
    }
}

int bt_manager_bt_do_apll_temp_comp(void)
{
	if (bt_mgr_info.bt_ready) {
	    return btif_bt_do_apll_temp_comp();
	}
    else{
        int bt_bqb_vs_do_apll_temp_comp(void);
        return bt_bqb_vs_do_apll_temp_comp();
    }
}

#define HLT_TEST_ADD_ADJUST_BDG			1

#if (HLT_TEST_ADD_ADJUST_BDG)
#define CFG_TEST_BDG_VALUE	"EFUSE_BDG_VALUE"
static uint32_t efuse_bdg_val;
static int pre_bdg_adjust_temp = 25;

static void bdg_vol_adjust_init(void)
{
	uint32_t val;
	uint32_t bdg_valu;
	int ret;

	val = sys_read32(BDG_CTL_SVCC);
	SYS_LOG_INF("Register efuse_bdg_val: 0x%x BDG_CTL_SVCC 0x%x", (val & 0x1F), val);

	ret = property_get(CFG_TEST_BDG_VALUE, (char *)&bdg_valu, sizeof(bdg_valu));		/* Should read from efuse and set in init */
	if (ret == sizeof(bdg_valu)) {
		efuse_bdg_val = bdg_valu & 0x1F;
		val &= (~0x1F);
		val |= (efuse_bdg_val&0x1F);
		sys_write32(val, BDG_CTL_SVCC);
		k_busy_wait(300);
		SYS_LOG_INF("NVRAM efuse_bdg_val: 0x%x BDG_CTL_SVCC 0x%x", efuse_bdg_val, val);
	} else {
		efuse_bdg_val = val & 0x1F;
		SYS_LOG_INF("efuse_bdg_val: 0x%x BDG_CTL_SVCC 0x%x", efuse_bdg_val, val);

		bdg_valu = efuse_bdg_val;
		property_set_factory(CFG_TEST_BDG_VALUE, (char *)&bdg_valu, sizeof(bdg_valu));
		property_flush(CFG_TEST_BDG_VALUE);
	}
}

static void bdg_vol_adjust_temp(int temp)
{
	int adjust_flag = 0;
	int adjust_val = 0;
	uint32_t val, set_val;

	if (temp <= -30) {
		if (pre_bdg_adjust_temp > -30) {
			adjust_flag = 1;
			adjust_val = 3;
		}
	} else if (temp <= -10) {
		if ((pre_bdg_adjust_temp > -10) ||
			((pre_bdg_adjust_temp <= -30) && (temp >= -28))) {
			adjust_flag = 1;
			adjust_val = 2;
		}
	} else if (temp <= 10) {
		if ((pre_bdg_adjust_temp > 10) ||
			((pre_bdg_adjust_temp <= -10) && (temp >= -8))) {
			adjust_flag = 1;
			adjust_val = 1;
		}
	} else if (temp <= 45) {
		if ((pre_bdg_adjust_temp > 45) ||
			((pre_bdg_adjust_temp <= 10) && (temp >= 12))) {
			adjust_flag = 1;
			adjust_val = 0;
		}
	} else if (temp <= 70) {
		if ((pre_bdg_adjust_temp > 70) ||
			((pre_bdg_adjust_temp <= 45) && (temp >= 47))) {
			adjust_flag = 1;
			adjust_val = -1;
		}
	} else if (temp <= 100) {
		if ((pre_bdg_adjust_temp > 100) ||
			((pre_bdg_adjust_temp <= 70) && (temp >= 72))) {
			adjust_flag = 1;
			adjust_val = -2;
		}
	} else {
		if ((pre_bdg_adjust_temp <= 100) && (temp >= 102)) {
			adjust_flag = 1;
			adjust_val = -3;
		}
	}

	if (adjust_flag) {
		pre_bdg_adjust_temp = temp;
		val = sys_read32(BDG_CTL_SVCC);
		val &= (~0x1F);
		set_val = efuse_bdg_val;
		set_val += adjust_val;
		val |= (set_val&0x1F);
		sys_write32(val, BDG_CTL_SVCC);
		k_busy_wait(300);
		SYS_LOG_INF("adjust_bdg_val: temp %d adjust_val %d efuse_val 0x%x set_val 0x%x BDG_CTL_SVCC 0x%x",
					temp, adjust_val, efuse_bdg_val, set_val, sys_read32(BDG_CTL_SVCC));
	}
}
#endif

static void bt_temp_comp_timer_handler(struct k_work* work)
{
	static uint16_t cnt;
	int temp = 0;
	uint8_t keep_wake_lock = (bt_mgr_info.temp_comp_runing_flag == 0) ? 1 : 0;

	if (keep_wake_lock) {
		bt_wake_lock();
	}
	temp = soc_pmu_get_temperature();		/* Return -1, read error. */
	if (keep_wake_lock) {
		bt_wake_unlock();
	}

	if (temp == (-1)) {
		SYS_LOG_ERR("Temp comp err %d", temp);
		goto exit_temp_comp;
	}

#if (HLT_TEST_ADD_ADJUST_BDG)
	bdg_vol_adjust_temp((temp/10));
#endif

#ifdef CONFIG_TEMP_COMPENSATION_ACTS
	extern void cap_temp_do_comp(int temp);
	bt_wake_lock();
	cap_temp_do_comp((temp/10));
	bt_wake_unlock();
#endif

	if (bt_mgr_info.bt_temp_comp_stage == 0) {
		SYS_LOG_INF("temperature %d.%d", temp / 10, abs(temp % 10));
		bt_mgr_info.bt_temp_comp_stage = 1;
		bt_mgr_info.bt_comp_last_temp  = temp;
	} else if (abs(temp - bt_mgr_info.bt_comp_last_temp) >= 150) {
		SYS_LOG_INF("DO_TEMP_COMP %d.%d", temp / 10, abs(temp % 10));
		bt_manager_bt_set_apll_temp_comp(true);
		bt_manager_bt_do_apll_temp_comp();
		bt_mgr_info.bt_comp_last_temp = temp;
	} else {
		if (cnt++ > 600) {
			cnt = 0;
			SYS_LOG_INF("temperature %d.%d", temp / 10, abs(temp % 10));
		} else if (bt_mgr_info.temp_comp_runing_flag == 0) {
			SYS_LOG_INF("temperature %d.%d", temp / 10, abs(temp % 10));
		} else {
			//高低温时打开，确认打印温度用
			//SYS_LOG_INF("temperature %d.%d", temp / 10, abs(temp % 10));
		}
	}

exit_temp_comp:
	bt_mgr_info.temp_comp_runing_flag = 1;
	//os_delayed_work_submit(&bt_mgr_info.bt_temp_comp_timer, BT_TEMP_COMP_CHECK_TIMER_MS);
	os_delayed_work_submit_to_queue(&temp_comp_work_q, &bt_mgr_info.bt_temp_comp_timer, BT_TEMP_COMP_CHECK_TIMER_MS);
}

static void bt_temp_comp_hrtimer_hdl(struct hrtimer *timer, void *expiry_fn_arg)
{
	if (bt_mgr_info.temp_comp_runing_flag == 0) {
		//os_delayed_work_submit(&bt_mgr_info.bt_temp_comp_timer, 0);
		os_delayed_work_submit_to_queue(&temp_comp_work_q, &bt_mgr_info.bt_temp_comp_timer, 0);
	}

	bt_mgr_info.temp_comp_runing_flag = 0;
}

static int bt_temp_comp_init(bool mode)
{
#if (HLT_TEST_ADD_ADJUST_BDG)
	bdg_vol_adjust_init();
#endif

	if (!mode) {
		k_work_queue_start(&temp_comp_work_q, temp_comp_stack, K_THREAD_STACK_SIZEOF(temp_comp_stack), 12, NULL);
		os_thread_name_set(&temp_comp_work_q.thread, "tcomp_workq");
	}

	os_delayed_work_init(&bt_mgr_info.bt_temp_comp_timer, bt_temp_comp_timer_handler);
	bt_mgr_info.bt_temp_comp_stage = 0;

	//os_delayed_work_submit(&bt_mgr_info.bt_temp_comp_timer, BT_TEMP_COMP_CHECK_TIMER_MS);
	os_delayed_work_submit_to_queue(&temp_comp_work_q, &bt_mgr_info.bt_temp_comp_timer, BT_TEMP_COMP_CHECK_TIMER_MS);

	bt_mgr_info.temp_comp_runing_flag = 1;
	hrtimer_init(&bt_mgr_info.temp_comp_hrtimer, bt_temp_comp_hrtimer_hdl, NULL);
	hrtimer_start(&bt_mgr_info.temp_comp_hrtimer, BT_TEMP_COMP_HRTIME_CHECK_TIME, BT_TEMP_COMP_HRTIME_CHECK_TIME);

	return 0;
}
#endif

int bt_manager_init(void)
{
	int ret = 0;
    bool btdev_err = bt_mgr_info.btdev_err;

    if(bt_mgr_info.inited){
        return 0;
    }

	memset(&bt_mgr_info, 0, sizeof(struct bt_manager_info_t));
	bt_manager_set_bt_drv_param();

#ifdef CONFIG_BT_CTRL_BQB
	if (bt_manager_check_enter_bqb()) {
        struct app_msg  msg = {0};
        msg.type = MSG_BT_ENGINE_READY;
        send_async_msg("main", &msg);
		return 0;
	}
#endif

#ifdef CONFIG_BT_TEMP_COMPENSATION
    bt_temp_comp_init(btdev_err);
#endif

#ifdef CONFIG_BT_ACTIONS_SUPER_SERVICE
    if(bt_manager_check_pincode_mode() == 1){
        hostif_bt_br_enable_pincode(true);
    }
    else{
        hostif_bt_br_enable_pincode(false);
    }
#endif

	bt_manager_check_mac_name();
#if CONFIG_BT_BR_ACTS
	bt_manager_set_status(BT_STATUS_NONE);
#endif

	btif_base_register_processer();

#ifdef CONFIG_BT_PNP_INFO_SEARCH
	btif_pnp_info_register_processer();
#endif

#ifdef CONFIG_BT_HFP_HF
	btif_hfp_register_processer();
#endif
#ifdef CONFIG_BT_HFP_AG
	btif_hfp_ag_register_processer();
#endif
#ifdef CONFIG_BT_A2DP
	btif_a2dp_register_processer();
#endif
#ifdef CONFIG_BT_AVRCP
	btif_avrcp_register_processer();
#endif
#ifdef CONFIG_BT_SPP
	btif_spp_register_processer();
#endif
#ifdef CONFIG_BT_PBAP_CLIENT
	btif_pbap_register_processer();
#endif
#ifdef CONFIG_BT_MAP_CLIENT
	btif_map_register_processer();
#endif
#if CONFIG_BT_HID
	btif_hid_register_processer();
#endif
#ifdef CONFIG_TWS
	btif_tws_register_processer();
#endif

	if (!btdev_err) {
		os_sem_init(&bt_mgr_info.ready_sem, 0, 1);
	}

	if (btif_start(_bt_mgr_callback, bt_manager_config_bt_class(), bt_manager_config_get_device_id()) < 0) {
		SYS_LOG_ERR("btsrv start error!");
		ret = -EACCES;
		goto bt_start_err;
	}
#if CONFIG_BT_BR_ACTS
	bt_manager_set_status(BT_STATUS_WAIT_PAIR);
#endif
	bt_mgr_info.inited = 1;
    bt_mgr_info.btdev_err = btdev_err;

	if ((!btdev_err) && (!bt_mgr_info.bt_ready)) {
		os_sem_take(&bt_mgr_info.ready_sem, 2000); // wait 2s
	}
	SYS_LOG_INF("bt init ok!");
	return 0;

bt_start_err:
	return ret;
}

void bt_manager_deinit(void)
{
	#if CONFIG_BT_BR_ACTS
	int time_out = 0;
	#endif

	/* Not inited or hci timeout, can't run deinit. */
	if (!bt_mgr_info.inited || bt_mgr_info.bt_TO_reboot) {
		return;
	}

#ifdef CONFIG_BT_TEMP_COMPENSATION
	os_delayed_work_cancel(&bt_mgr_info.bt_temp_comp_timer);
	if (hrtimer_is_running(&bt_mgr_info.temp_comp_hrtimer)) {
		hrtimer_stop(&bt_mgr_info.temp_comp_hrtimer);
	}
#endif

#if CONFIG_BT_BR_ACTS
	bt_manager_set_visible(false);
	bt_manager_set_connectable(false);
	btif_br_auto_reconnect_stop();
	btif_br_disconnect_device(BTSRV_DISCONNECT_ALL_MODE);

	while (btif_br_get_connected_device_num() && time_out++ < 300) {
		os_sleep(10);
	}
#endif

#ifdef CONFIG_BT_SPP
	bt_manager_spp_profile_stop();
#endif

#ifdef CONFIG_BT_A2DP
	bt_manager_a2dp_profile_stop();
#endif

#ifdef CONFIG_BT_AVRCP
	bt_manager_avrcp_profile_stop();
#endif

#ifdef CONFIG_BT_HFP_HF
	bt_manager_hfp_profile_stop();
	bt_manager_hfp_sco_stop();
#endif

#ifdef CONFIG_BT_HFP_AG
	bt_manager_hfp_ag_profile_stop();
#endif

#ifdef CONFIG_BT_HID
	bt_manager_hid_profile_stop();
#endif

#ifdef CONFIG_BT_BLE
	bt_manager_ble_deinit();
#endif

#ifdef CONFIG_BT_PNP_INFO_SEARCH
	bt_manager_pnp_info_search_deinit();
#endif

	/**
	 *  TODO: must clean btdrv /bt stack and bt service when bt manager deinit
	 *  enable this after all is work well.
	 */
#ifdef CONFIG_BT_MANAGER_DEINIT_SERVICE
	btif_stop();

	time_out = 0;
	while (srv_manager_check_service_is_actived(BLUETOOTH_SERVICE_NAME) && time_out++ < 500) {
        os_sleep(10);
	}
#endif

	bt_mgr_info.inited = 0;
	SYS_LOG_INF("deinit ok!");
}

int bt_manager_bt_crash_test(void)
{
#if BT_RESET_SUPPORT
	if (bt_mgr_info.bt_ready) {
		void acts_reset_peripheral_assert(int reset_id);
		#define RESET_ID_BT 			56

		soc_powergate_set(POWERGATE_BT_PG_DEV, true);
		acts_reset_peripheral_assert(RESET_ID_BT);
	}
#endif
	return 0;
}

int bt_manager_bt_reset_test(void)
{
#if BT_RESET_SUPPORT
	int time_out = 0;

	if (bt_mgr_info.bt_ready) {
		if (bt_manager_ble_is_connected()) {
			bt_manager_ble_disconnect();
			while (bt_manager_ble_is_connected() && time_out++ < 200) {
				os_sleep(10);
			}

			os_sleep(50);
		}
		bt_manager_ble_adv_stop();

		soc_powergate_set(POWERGATE_BT_PG_DEV, true);
		bt_mgr_btsrv_ready(1);
	}
#endif
	return 0;
}

bool bt_manager_is_inited(void)
{
	return (bt_mgr_info.inited == 1);
}

void bt_manager_disconnect_device(uint8_t mode)
{
	btif_br_disconnect_device(mode);
}

int bt_manager_get_connected_dev_info(struct btmgr_connect_dev_info *info, uint8_t info_num)
{
	int i, name_len, dev_cnt = 0;

	if (!info_num) {
		return 0;
	}

	memset(info, 0, sizeof(struct btmgr_connect_dev_info)*info_num);

	for (i = 0; i < MAX_MGR_DEV; i++) {
		if (bt_mgr_info.dev[i].used && bt_mgr_info.dev[i].notify_connected) {
			memcpy(&info[dev_cnt].addr, &bt_mgr_info.dev[i].addr, sizeof(bd_address_t));
			info[dev_cnt].is_trs = bt_mgr_info.dev[i].is_trs;
			name_len = MIN(BTMGR_BT_NAME_LEN, strlen(bt_mgr_info.dev[i].name));
			memcpy(info[dev_cnt].name, bt_mgr_info.dev[i].name, name_len);

			dev_cnt++;
			if (dev_cnt >= info_num) {
				break;
			}
		}
	}

	return dev_cnt;
}

int bt_manager_get_wake_lock(void)
{
#ifdef CONFIG_BT_BLE
	if (btif_base_get_wake_lock() || bt_manager_get_ble_wake_lock())
#else
	if (btif_base_get_wake_lock())
#endif
	{
		return 1;
	} else {
		return 0;
	}
}

void bt_manager_dump_info(void)
{
#if CONFIG_BT_BR_ACTS
	int i;

	printk("Bt manager info\n");
	printk("\t num %d, tws_mode %d, bt_state %d, playing %d\n", bt_mgr_info.connected_phone_num,
		bt_mgr_info.tws_mode, bt_mgr_info.bt_state, bt_mgr_info.playing);
	for (i = 0; i < MAX_MGR_DEV; i++) {
		if (bt_mgr_info.dev[i].used) {
			printk("\t Dev name %s, tws %d, trs %d (%d,%d,%d,%d,%d,%d)\n", bt_mgr_info.dev[i].name, bt_mgr_info.dev[i].is_tws,
				bt_mgr_info.dev[i].is_trs,
				bt_mgr_info.dev[i].notify_connected, bt_mgr_info.dev[i].a2dp_connected, bt_mgr_info.dev[i].avrcp_connected,
				bt_mgr_info.dev[i].hf_connected, bt_mgr_info.dev[i].spp_connected, bt_mgr_info.dev[i].hid_connected);
		}
	}
#endif

#ifdef CONFIG_BT_HFP_HF
	bt_manager_hfp_dump_info();
#endif

#ifdef CONFIG_BT_BLE
	bt_manager_ble_dump_info();
#endif

#if CONFIG_BT_BR_ACTS
	btif_dump_brsrv_info();
#endif
}
