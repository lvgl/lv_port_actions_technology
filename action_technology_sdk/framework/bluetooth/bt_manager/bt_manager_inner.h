/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager.
*/

#ifndef __BT_MANAGER_INNER_H__
#define __BT_MANAGER_INNER_H__
#include <btservice_api.h>
#include <stream.h>

struct bt_mgr_dev_info {
	bd_address_t addr;
	uint16_t hdl;
	uint16_t used:1;
	uint16_t notify_connected:1;
	uint16_t is_tws:1;
	uint16_t is_trs:1;
	uint16_t hf_connected:1;
	uint16_t a2dp_connected:1;
	uint16_t avrcp_connected:1;
	uint16_t spp_connected:3;
	uint16_t hid_connected:1;
	uint16_t hid_wake_lock:1;
	uint8_t *name;

	uint8_t  hid_state;
	uint8_t  hid_report_id;
	os_delayed_work hid_delay_work;
};

struct bt_mgr_dev_info *bt_mgr_find_dev_info_by_hdl(uint16_t hdl);

bool bt_mgr_check_dev_type(uint8_t type, uint16_t hdl);

void bt_manager_record_halt_phone(void);

void *bt_manager_get_halt_phone(uint8_t *halt_cnt);

void bt_manager_startup_reconnect(void);

void bt_manager_disconnected_reason(void *param);

int bt_manager_a2dp_profile_start(void);

int bt_manager_a2dp_profile_stop(void);

int bt_manager_avrcp_profile_start(void);

int bt_manager_avrcp_profile_stop(void);

int bt_manager_hfp_profile_start(void);

int bt_manager_hfp_profile_stop(void);

int bt_manager_hfp_ag_profile_start(void);

int bt_manager_hfp_ag_profile_stop(void);

int bt_manager_hfp_sco_start(void);

int bt_manager_hfp_sco_stop(void);

int bt_manager_spp_profile_start(void);

int bt_manager_spp_profile_stop(void);

void bt_manager_hid_delay_work(os_work *work);

void bt_manager_hid_connected_check_work(uint16_t hdl);

int bt_manager_hid_profile_start(uint16_t disconnect_delay, uint16_t opt_delay);

int bt_manager_hid_profile_stop(void);

int bt_manager_hid_register_sdp();

int bt_manager_did_register_sdp();

void bt_manager_tws_init(void);

int bt_manager_hfp_init(void);

void bt_manager_hfp_dump_info(void);

int bt_manager_hfp_ag_init(void);

int bt_manager_hfp_sco_init(void);

uint8_t bt_manager_config_get_tws_limit_inquiry(void);
uint8_t bt_manager_config_get_tws_compare_high_mac(void);
uint8_t bt_manager_config_get_tws_compare_device_id(void);
uint8_t bt_manager_config_get_idle_extend_windown(void);
void bt_manager_config_set_pre_bt_mac(uint8_t *mac);
void bt_manager_updata_pre_bt_mac(uint8_t *mac);
bool bt_manager_config_enable_tws_sync_event(void);
uint8_t bt_manager_config_connect_phone_num(void);
bool bt_manager_config_support_a2dp_aac(void);
bool bt_manager_config_support_a2dp_trs_aac(void);
bool bt_manager_config_pts_test(void);
uint16_t bt_manager_config_volume_sync_delay_ms(void);
uint32_t bt_manager_config_bt_class(void);
uint16_t *bt_manager_config_get_device_id(void);
int bt_manager_config_expect_tws_connect_role(void);

int btmgr_map_time_client_connect(bd_address_t *bd);

#ifdef CONFIG_BT_AVRCP_GET_ID3
void bt_manager_avrcp_notify_playback_pos(uint32_t pos);
#endif

#ifdef CONFIG_BT_ANCS_AMS
void ble_ancs_ams_init(void);
void ble_ancs_ams_event_handle(uint8_t event_code, void *event_data);
int ble_ancs_ams_get_playback_pos(void);
#endif

#ifdef CONFIG_BT_A2DP_TRS
int bt_manager_trs_a2dp_profile_start(struct btsrv_a2dp_start_param *param);
void bt_manager_trs_avrcp_pass_ctrl_callback(void *param, uint8_t cmd, uint8_t state);
#endif

#ifdef CONFIG_BT_PNP_INFO_SEARCH
int bt_manager_pnp_info_search_init(void);
int bt_manager_pnp_info_search_deinit(void);
#endif

#endif
