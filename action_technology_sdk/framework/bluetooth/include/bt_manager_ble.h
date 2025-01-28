/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt ble manager.
*/

#ifndef __BT_MANAGER_BLE_H__
#define __BT_MANAGER_BLE_H__
#include <acts_bluetooth/conn.h>
#include <acts_bluetooth/gatt.h>
#ifdef CONFIG_BLE_MASTER
#include "../bt_manager/ble_master/ble_master.h"
#endif

#ifdef __cplusplus
	extern "C" {
#endif
/**
 * @defgroup bt_manager_ble_apis Bt Manager Ble APIs
 * @ingroup bluetooth_system_apis
 * @{
 */

 enum {
	BLE_WAKE_LOCK_ADV_MANANGER         =  (0x1 << 0),
	BLE_WAKE_LOCK_CONNECTED_NOTIFY     =  (0x1 << 1),
	BLE_WAKE_LOCK_ANCS_DISCOVER        =  (0x1 << 2),
};

/** ble register manager structure */
struct ble_reg_manager {
	/* ble acl connected/disconnected callback */
	void (*link_cb)(uint8_t *mac, uint8_t connected);
	/* bt gatt service to register */
	struct bt_gatt_service gatt_svc;
	sys_snode_t node;
};

/**
 * @brief get ble mtu
 *
 * This routine provides to get ble mtu
 *
 * @return ble mtu
 */
uint16_t bt_manager_get_ble_mtu(void);

/**
 * @brief bt manager send ble data
 *
 * This routine provides to bt manager send ble data
 *
 * @param chrc_attr pointer of characteristic attribute to send data
 * @param des_attr pointer of descriptor attribute to send data
 * @param data pointer of send data
 * @param len length of data
 *
 * @return 0 pending or data len for successed , others failed
 */
int bt_manager_ble_send_data(struct bt_gatt_attr *chrc_attr,
					struct bt_gatt_attr *des_attr, uint8_t *data, uint16_t len);

/**
 * @brief ble disconnect
 *
 * This routine do ble disconnect
 *
 * @return  N/A
 */
void bt_manager_ble_disconnect(void);

/**
 * @brief ble service register
 *
 * This routine provides ble service register
 *
 * @param le_mgr ble register info 
 *
 * @return  N/A
 */
void bt_manager_ble_service_reg(struct ble_reg_manager *le_mgr);

/**
 * @brief init btmanager ble
 *
 * This routine init btmanager ble
 *
 * @return  N/A
 */
void bt_manager_ble_init(uint8_t mode);

/**
 * @brief deinit btmanager ble
 *
 * This routine  deinit btmanager ble
 *
 * @return  N/A
 */
void bt_manager_ble_deinit(void);

/**
 * @brief notify ble a2dp play state
 *
 * This routine notify ble a2dp play state
 *
 * @param play a2dp play or stop
 *
 * @return  N/A
 */
void bt_manager_ble_a2dp_play_notify(bool play);

/**
 * @brief notify ble hfp play state
 *
 * This routine notify ble hfp play state
 *
 * @param play hfp play or stop
 *
 * @return  N/A
 */
void bt_manager_ble_hfp_play_notify(bool play);

/**
 * @brief stop ble advertise
 *
 * This routine disable ble adv
 *
 * @return  N/A
 */
void bt_manager_ble_adv_stop(void);

/**
 * @brief start ble advertise
 *
 * This routine enable ble adv
 *
 * @return  0:success;   <0:fail;
 */
int bt_manager_ble_adv_start(void);

/**
* @brief enable ble no-connect advertise
*
* This routine enable ble adv
*
* @return  0:success;   <0:fail;
*/
void bt_manager_ble_set_nconn_adv(bool enable);

/**
 * @brief reset ble advertise
 *
 * This routine reset ble adv
 *
 * @return  0:success;   <0:fail;
 */
int bt_manager_ble_set_adv_data(struct bt_data ad_data[], uint8_t ad_data_len, struct bt_data sd_data[], uint8_t sd_data_len);

/**
 * @brief get ble wake lock
 *
 * This routine get ble wake lock
 *
 * @return  0: idle can sleep; other: busy can't sleep.
 */
int bt_manager_get_ble_wake_lock(void);

/**
 * @brief get ble mac addr
 *
 * This routine get ble mac
 *
 * @return  0: success; other: error.
 */
int bt_manager_get_ble_mac(bt_addr_le_t *le_addr);

/** @brief check le ready for send data.
 *
 *  @return  0: no ready , > 0:  ready.
 */
int bt_manager_ble_ready_send_data(void);

/** @brief Get le tx pending cnt.
 *
 *  @return  Number of pendind send(include host and controler).
 */
uint16_t bt_manager_ble_tx_pending_cnt(void);

/**
 * @brief get bt ble connected dev num
 *
 * This routine get ble connected dev num
 *
 * @return  bt ble connected dev num.
 */
uint8_t bt_manager_get_ble_connected_dev_num(void);

/**
 * @brief get bt ble state
 *
 * This routine get ble state
 *
 * @return  bt ble state.
 */
int bt_manager_get_ble_state(void);

/**
 * @brief Check is app enable le advertise
 *
 * This routine Check is app enable le advertise
 *
 * @return  true: app enable ble advertise, false: app disable advertise.
 */
bool bt_manager_is_app_enable_adv(void);

/**
 * @brief get bt ble info
 *
 * This routine get ble info
 *
 * @return  bt ble info.
 */
struct ble_mgr_info * bt_manager_get_ble_info(void);

/**
 * @brief get bt ble connected dev rssi
 *
 * This routine get ble connected dev rssi
 *
 * @return  bt ble connected dev rssi.
 */
int bt_manager_get_ble_dev_rssi(int8_t *rssi);

/**
 * @brief start bt ble ancs and ams
 *
 * This routine start bt ble ancs and ams
 *
 * @return  N/A.
 */
void bt_manager_ble_ancs_ams_start(void);

/**
 * @brief enter br bqb or ble bqb
 *
 * This routine enter br/ble bqb
 *
 * @return  N/A.
 */
void bt_manager_ble_super_enter_bqb(uint8_t mode);

/**
 * @brief ble set speed(Set ble parameter)
 *
 * This routine set ble parameter
 *
 * @return  0: success, other: failed.
 */
int bt_manager_ble_super_set_speed(uint8_t expect_speed);

/**
 * @brief ble super test notify
 *
 * This routine ble super test notify
 *
 * @return  0: success, other: failed.
 */
int bt_manager_ble_super_test_notify(void);

/**
 * @brief register bt ble super service
 *
 * This routine resister super service
 *
 * @return  0 success other fail.
 */
int ble_manager_super_register(void);

/**
 * @brief bt ble set connect param
 *
 * This routine set ble connection param
 *
 * @param speed_mode
 *
 * @return  0 success other fail.
 */
int bt_manager_set_ble_connection_param(uint8_t speed_mode);

/**
 * @brief bt ble find linkkey by le_addr
 *
 * This routine ble find linkkey by le_addr
 *
 * @param le_addr for find
 *
 * @return  id (0 ~ max linkkey size) success other fail.
 */
int bt_manager_ble_find_linkkey(bt_addr_le_t *le_addr);

/**
 * @brief bt ble clear linkkey by le_addr
 *
 * This routine ble find linkkey by le_addr
 *
 * @param NULL for all and le_addr for target device
 *
 * @return  N/A
 */
void bt_manager_ble_clear_linkkey(bt_addr_le_t *le_addr);

/**
 * @brief bt ble find linkkey num
 *
 * This routine ble find linkkey number
 *
 * @return ble linkkey number or 0
 */
uint8_t bt_manager_ble_get_link_key_num(void);

void bt_manager_ble_hid_report(uint16_t button);

void bt_manager_ble_set_phone(bool ios);

bool bt_manager_ble_is_connected(void);

void bt_manager_reset_le_adv(void);

int ble_get_ams_status(void);

void ble_set_clear_wake_lock(uint16_t wake_lock, uint8_t set);

void bt_manager_ble_update_param_wait(void);

#ifdef CONFIG_BT_BLE_APP_UPDATE_PARAM
/**
 * @brief bt ble upate parameter
 *
 * This routine update ble parameter
 *
 *  @param param.interval_min  Minimum Connection Interval (N * 1.25 ms)
 *   			param.interval_max  Maximum Connection Interval (N * 1.25 ms)
 *   			param.latency     Connection Latency
 *  			param.timeout     Supervision Timeout (N * 10 ms)
 *
 * @return  0: success, other failed.
 */
int bt_manager_ble_update_param(const struct bt_le_conn_param *param);

/**
 * @brief bt get ble paramter
 *
 * This routine get ble parameter
 *
 *  @param *interval Connection Interval (N * 1.25 ms)
 *  @param *latency  Connection Latency
 *  @param *timeout  Supervision Timeout (N * 10 ms)
 *
 * @return  0: success, other failed.
 */
int bt_manager_ble_get_param(uint16_t *interval, uint16_t *latency, uint16_t *timeout);

/**
 * @brief bt get ble connect phone type
 *
 * This routine get ble connect phone type
 *
 * @return  0: Android, 1: iso, other: not connected or unknow.
 */
int bt_manager_ble_get_phone_type(void);

/**
 * @brief check br is busy(a2dp playing or hfp on call)
 *
 * This routine check br is busy
 *
 * @return  0: br idle, 1: br busy.
 */
int bt_manager_ble_is_br_busy(void);
#else
static inline int bt_manager_ble_update_param(const struct bt_le_conn_param *param)
{
	return -1;
}

static inline int bt_manager_ble_get_param(uint16_t *interval, uint16_t *latency, uint16_t *timeout)
{
	if (interval) {
		*interval = 0;
	}

	if (latency) {
		*latency = 0;
	}

	if (timeout) {
		*timeout = 0;
	}

	return -1;
}

static inline int bt_manager_ble_get_phone_type(void)
{
	return 0;
}

static inline int bt_manager_ble_is_br_busy(void)
{
	return 0;
}
#endif

void bt_manager_ble_dump_info(void);

#ifdef __cplusplus
}
#endif

#endif  // __BT_MANAGER_BLE_H__
