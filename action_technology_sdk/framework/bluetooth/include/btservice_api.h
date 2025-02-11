/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @defgroup bt service APIs
 * @ingroup bluetooth_system_apis
 * @brief Just for bt manager used, not for application.
 * @{
 * @}
 */

/**
 * @cond INTERNAL_HIDDEN
 */

/**
 * @file bt service interface
 */

#ifndef _BTSERVICE_API_H_
#define _BTSERVICE_API_H_
#include <stream.h>
#include <btservice_base.h>
#include <btservice_tws_api.h>
#include <acts_bluetooth/sdp.h>

enum {
	CONTROLER_ROLE_MASTER,
	CONTROLER_ROLE_SLAVE,
};

/** btsrv auto conn mode */
enum btsrv_auto_conn_mode_e {
	/** btsrv auto connet device alternate */
	BTSRV_AUTOCONN_ALTERNATE,
	/** btsrv auto connect by device order */
	BTSRV_AUTOCONN_ORDER,
};

enum btsrv_disconnect_mode_e {
	/**  Disconnect all device */
	BTSRV_DISCONNECT_ALL_MODE,
	/** Just disconnect all phone */
	BTSRV_DISCONNECT_PHONE_MODE,
	/** Just disconnect tws */
	BTSRV_DISCONNECT_TWS_MODE,
	/** Just disconnect trs */
	BTSRV_DISCONNECT_TRS_MODE,
};

enum btsrv_device_type_e {
	/** all device */
	BTSRV_DEVICE_ALL         = 0,
	/** Just phone device*/
	BTSRV_DEVICE_PHONE       = 1,
	/** Just tws device*/
	BTSRV_DEVICE_TWS         = 2,
	/** Just player device*/
	BTSRV_DEVICE_PLAYER      = 3,		/* For BT_A2DP_TRS,  value = 3, can't change */
};

/** BR inquiry/page scan parameter */
struct bt_scan_parameter {
	uint8_t tws_limit_inquiry:1;		/* 0: Normal inquiry,  1: limit inquiry */
	uint8_t idle_extend_windown:1;		/* 0: Normal inquiry/page windown in ilde, 1: extend windown */
	uint16_t inquiry_interval;
	uint16_t inquiry_windown;
	uint16_t page_interval;
	uint16_t page_windown;
};

/** device auto connect info */
struct autoconn_info {
	/** bluetooth device address */
	bd_address_t addr;
	/** bluetooth device address valid flag*/
	uint8_t addr_valid:1;
	/** bluetooth device tws role */
	uint8_t tws_role:3;
	/** bluetooth device need connect a2dp */
	uint8_t a2dp:1;
	/** bluetooth device need connect avrcp */
	uint8_t avrcp:1;
	/** bluetooth device need connect hfp */
	uint8_t hfp:1;
	/** bluetooth connect hfp first */
	uint8_t hfp_first:1;
	/** bluetooth device need connect hid */
	uint8_t hid:1;
	/** bluetooth device type */
	uint8_t trs_mode:1;
};

struct btsrv_config_info {
	/** Max br connect device number */
	uint8_t max_conn_num:4;
	/** Max br phone device number */
	uint8_t max_phone_num:4;
	/** Set pts test mode */
	uint8_t pts_test_mode:1;
	/** Set pts test mode */
	uint8_t double_preemption_mode:1;
	/** delay time for volume sync */
	uint16_t volume_sync_delay_ms;
	/** Tws version */
	uint8_t tws_version;
	/** Tws feature */
	uint32_t tws_feature;
};

/** auto connect info */
struct bt_set_autoconn {
	bd_address_t addr;
	uint8_t strategy;
	uint8_t base_try;
	uint8_t profile_try;
	uint16_t base_interval;
	uint16_t profile_interval;
	uint8_t trs_mode;

	/* For active connect device without auto connect info and not trs device */
	uint8_t force_connect:1;		/* Connect acl or profile, when acl connected, connect profile */
	uint8_t connect_hfp:1;
	uint8_t connect_a2dp:1;
	uint8_t connect_avrcp:1;
	uint8_t connect_hfp_first:1;
};

struct bt_linkkey_info {
	bd_address_t addr;
	uint8_t linkkey[16];
};


#ifdef CONFIG_BT_A2DP_TRS
struct bt_trs_list_dev_t {
	bd_address_t addr;
    uint8_t name[33];
    uint8_t valid;
};
#endif

enum {
	BT_LINK_EV_ACL_CONNECT_REQ,
	BT_LINK_EV_ACL_CONNECTED,
	BT_LINK_EV_ACL_DISCONNECTED,
	BT_LINK_EV_GET_NAME,
	BT_LINK_EV_HF_CONNECTED,
	BT_LINK_EV_HF_DISCONNECTED,
	BT_LINK_EV_A2DP_CONNECTED,
	BT_LINK_EV_A2DP_DISCONNECTED,
	BT_LINK_EV_AVRCP_CONNECTED,
	BT_LINK_EV_AVRCP_DISCONNECTED,
	BT_LINK_EV_SPP_CONNECTED,
	BT_LINK_EV_SPP_DISCONNECTED,
	BT_LINK_EV_HID_CONNECTED,
	BT_LINK_EV_HID_DISCONNECTED,
    BT_LINK_EV_PASSKEY_DISPLAY,
};

struct bt_link_cb_param {
	uint8_t link_event;
	uint8_t is_tws:1;
	uint8_t is_trs:1;
	uint8_t new_dev:1;
	uint8_t reason;
	uint16_t hdl;
	bd_address_t *addr;
	uint8_t *name;
    uint32_t passkey;
};

struct bt_connected_cb_param {
	uint8_t acl_connected:1;	/* Just BR acl connected */
	bd_address_t *addr;
	uint8_t *name;
};

struct bt_disconnected_cb_param {
	uint8_t acl_disconnected:1;	/* BR acl disconnected */
	bd_address_t addr;
	uint8_t reason;
};

/** bt device disconnect info */
struct bt_disconnect_reason {
	/** bt device addr */
	bd_address_t addr;
	/** bt device disconnected reason */
	uint8_t reason;
	/** bt device tws role */
	uint8_t tws_role;
};

/* Bt device rdm state  */
struct bt_dev_rdm_state {
	bd_address_t addr;
	uint8_t acl_connected:1;
	uint8_t hfp_connected:1;
	uint8_t a2dp_connected:1;
	uint8_t avrcp_connected:1;
	uint8_t hid_connected:1;
	uint8_t sco_connected:1;
	uint8_t a2dp_stream_started:1;
};

struct btsrv_discover_result {
	uint8_t discover_finish:1;
	bd_address_t addr;
	int8_t rssi;
	uint8_t len;
	uint8_t *name;
	uint8_t cod[3];			/** Class of Device */
	uint16_t device_id[4];
};

typedef void (*btsrv_discover_result_cb)(void *result);

struct btsrv_discover_param {
	/** Maximum length of the discovery in units of 1.28 seconds. Valid range is 0x01 - 0x30. */
	uint8_t length;

	/** 0: Unlimited number of responses, Maximum number of responses. */
	uint8_t num_responses;

	/** Result callback function */
	btsrv_discover_result_cb cb;
};

struct btsrv_check_device_role_s {
	bd_address_t addr;
	uint8_t len;
	uint8_t *name;
};

struct bt_get_rssi_result{
    int8_t status;
	int8_t rssi;
};

typedef void (*btsrv_get_rssi_result_cb)(struct bt_get_rssi_result * result);

struct bt_get_rssi_param {
	bd_address_t addr;
	btsrv_get_rssi_result_cb cb;
};

struct bt_br_resolve_param {
	uint8_t br_connected:1;
	bd_address_t addr;
};

/*=============================================================================
 *				global api
 *===========================================================================*/
/** Callbacks to report Bluetooth service's events*/
typedef enum {
	/** notifying that Bluetooth service has been started err, zero on success or (negative) error code otherwise*/
	BTSRV_READY,
	/** notifying link event */
	BTSRV_LINK_EVENT,
	/** notifying remote device disconnected mac with reason */
	BTSRV_DISCONNECTED_REASON,
	/** Request high cpu performace */
	BTSRV_REQ_HIGH_PERFORMANCE,
	/** Release high cpu performace */
	BTSRV_RELEASE_HIGH_PERFORMANCE,
	/** Request flush property to nvram */
	BTSRV_REQ_FLUSH_PROPERTY,
	/** CHECK device is phone or tws */
	BTSRV_CHECK_NEW_DEVICE_ROLE,

	BTSRV_PAIR_PASSKEY_DISPLAY,

	BTSRV_PAIRING_COMPLETE,

	BTSRV_PAIRING_FAIL,
	/** Return resolve br address */
	BTSRV_BR_RESOLVE_ADDR,
} btsrv_event_e;

/**
 * @brief btsrv callback
 *
 * This routine is btsrv callback, this callback context is in bt service
 *
 * @param event btsrv event
 * @param param param of btsrv event
 *
 * @return N/A
 */
typedef int (*btsrv_callback)(btsrv_event_e event, void *param);

/**
 * @brief Rgister base processer
 *
 * @return 0 excute successed , others failed
 */
int btif_base_register_processer(void);

/**
 * @brief start bt service
 *
 * This routine start bt service.
 *
 * @param cb handle of btsrv event callback
 * @param classOfDevice clase of device
 * @param did device ID
 *
 * @return 0 excute successed , others failed
 */
int btif_start(btsrv_callback cb, uint32_t classOfDevice, uint16_t *did);

/**
 * @brief stop bt service
 *
 * This routine stop bt service.
 *
 * @return 0 excute successed , others failed
 */
int btif_stop(void);

/**
 * @brief set base config info to bt service
 *
 * This routine set base config info to bt service.
 *
 * @return 0 excute successed , others failed
 */
int btif_base_set_config_info(void *param);

/**
 * @brief Get bluetooth wake lock
 *
 * This routine get bluetooth wake lock.
 *
 * @return 0: idle can sleep,  other: busy can't sleep.
 */
int btif_base_get_wake_lock(void);

/**
 * @brief set br scan parameter
 *
 * This routine set br scan parameter.
 *
 * @param param parameter
 * @enhance_param: true enhance param, false: default param
 *
 * @return 0 excute successed , others failed
 */
int btif_br_set_scan_param(const struct bt_scan_parameter *param, bool enhance_param);

/**
 * @brief set br discoverable
 *
 * This routine set br discoverable.
 *
 * @param enable true enable discoverable, false disable discoverable
 *
 * @return 0 excute successed , others failed
 */
int btif_br_set_discoverable(bool enable);

/**
 * @brief set br connnectable
 *
 * This routine set br connnectable.
 *
 * @param enable true enable connnectable, false disable connnectable
 *
 * @return 0 excute successed , others failed
 */
int btif_br_set_connnectable(bool enable);

/**
 * @brief allow br sco connnectable
 *
 * This routine disable br sco connect or not.
 *
 * @param enable true enable, false disable 
 *
 * @return 0 excute successed , others failed
 */
int btif_br_allow_sco_connect(bool allowed);

/**
 * @brief Start br disover
 *
 * This routine Start br disover.
 *
 * @param disover parameter
 *
 * @return 0 excute successed , others failed
 */
int btif_br_start_discover(struct btsrv_discover_param *param);

/**
 * @brief Stop br disover
 *
 * This routine Stop br disover.
 *
 * @return 0 excute successed , others failed
 */
int btif_br_stop_discover(void);

/**
 * @brief Remote name request
 *
 * This routine Request remote device name.
 *
 * @param addr Device address
 * @param cb Result callback function
 *
 * @return 0 excute successed , others failed
 */
int btif_br_remote_name_req(bd_address_t *addr, bt_br_remote_name_cb_t cb);

/**
 * @brief connect to target bluetooth device
 *
 * This routine connect to target bluetooth device with bluetooth mac addr.
 *
 * @param bd bluetooth addr of target bluetooth device
 *                 Mac low address store in low memory address
 *
 * @return 0 excute successed , others failed
 */
int btif_br_connect(bd_address_t *bd);

/**
 * @brief disconnect to target bluetooth device
 *
 * This routine disconnect to target bluetooth device with bluetooth mac addr.
 *
 * @param bd bluetooth addr of target bluetooth device
 *                 Mac low address store in low memory address
 *
 * @return 0 excute successed , others failed
 */
int btif_br_disconnect(bd_address_t *bd);

/**
 * @brief get auto reconnect info from bt service
 *
 * This routine get auto reconnect info from bt service.
 *
 * @param info pointer store the audo reconnect info
 * @param max_cnt max number of bluetooth device
 *
 * @return 0 excute successed , others failed
 */
int btif_br_get_auto_reconnect_info(struct autoconn_info *info, uint8_t max_cnt);

/**
 * @brief set auto reconnect info from bt service
 *
 * This routine set auto reconnect info from bt service.
 *
 * @param info pointer store the audo reconnect info
 * @param max_cnt max number of bluetooth device
 */
void btif_br_set_auto_reconnect_info(struct autoconn_info *info, uint8_t max_cnt);

/**
 * @brief auto reconnect to all need reconnect device
 *
 * This routine auto reconnect to all need reconnect device
 *
 * @param param auto reconnect info
 *
 * @return 0 excute successed , others failed
 */
int btif_br_auto_reconnect(struct bt_set_autoconn *param);

/**
 * @brief stop auto reconnect
 *
 * This routine stop auto reconnect
 *
 * @return 0 excute successed , others failed
 */

int btif_br_auto_reconnect_stop(void);

/**
 * @brief check auto reconnect is runing
 *
 * This routine check auto reconnect is runing
 *
 * @return ture: runing; false stop;
 */
bool btif_br_is_auto_reconnect_runing(void);

/**
 * @brief clear br connected info list
 *
 * This routine clear br connected info list, will disconnected all connnected device first.
 *
 * @return 0 excute successed , others failed
 */
int btif_br_clear_list(int mode);

/**
 * @brief disconnect all connected device
 *
 * This routine disconnect all connected device.
 * @param:  disconnect_mode
 *
 * @return 0 excute successed , others failed
 */
int btif_br_disconnect_device(int disconnect_mode);

/**
 * @brief get br connected device number
 *
 * This routine get connected device number.
 *
 * @return connected device number(include phone and tws device)
 */
int btif_br_get_connected_device_num(void);

/**
 * @brief get br device state from rdm
 *
 * This routine get device state from rdm.
 *
 * @return 0 successed, others failed.
 */
int btif_br_get_dev_rdm_state(struct bt_dev_rdm_state *state);

/**
 * @brief set phone controler role
 *
 * This routine get device state from rdm.
 */
void btif_br_set_phone_controler_role(bd_address_t *bd, uint8_t role);

/**
 * @brief bt  get address linkkey
 *
 * This routine get address linkkey.
 * @param addr Address.
 * @param linkkey NULL or uint8_t buf[16].
 *
 * @return int Number of linkkey returen.
 */
int btif_br_get_addr_linkkey(bd_address_t *addr, uint8_t *linkkey);

/**
 * @brief Get linkkey information
 *
 * This routine Get linkkey information.
 * @param info for store get linkkey information
 * @param cnt want to get linkkey number
 *
 * return int  number of linkkey getted
 */
int btif_br_get_linkkey(struct bt_linkkey_info *info, uint8_t cnt);

/**
 * @brief Update linkkey information
 *
 * This routine Update linkkey information.
 * @param info for update linkkey information
 * @param cnt update linkkey number
 *
 * return int 0: update successful, other update failed.
 */
int btif_br_update_linkkey(struct bt_linkkey_info *info, uint8_t cnt);

/**
 * @brief Writ original linkkey information
 *
 * This routine Writ original linkkey information.
 * @param addr update device mac address
 * @param link_key original linkkey
 *
 * return 0: successfull, other: failed.
 */
int btif_br_write_ori_linkkey(bd_address_t *addr, uint8_t *link_key);

/**
 * @brief clean bt service linkkey from nvram
 *
 * This routine clean bt service linkkey from nvram.
 */
void btif_br_clean_linkkey(bd_address_t *addr);

int btif_br_get_connected_device_rssi(struct bt_get_rssi_param *param);

int btif_br_get_actived_device_rssi(int8_t *rssi);

uint16_t btif_br_get_active_phone_hdl(void);

int btif_bt_set_apll_temp_comp(uint8_t enable);

int btif_bt_do_apll_temp_comp(void);

/**
 * @brief pts test send hfp command
 *
 * This routine pts test send hfp command
 *
 * @return 0 successed, others failed.
 */
int btif_pts_send_hfp_cmd(char *cmd);

/**
 * @brief pts test hfp active connect sco
 *
 * This routine pts test hfp active connect sco
 *
 * @return 0 successed, others failed.
 */
int btif_pts_hfp_active_connect_sco(void);

int btif_pts_a2dp_set_err_code(uint8_t err_code);

int btif_pts_avrcp_pass_through_cmd(uint8_t opid);
int btif_pts_avrcp_notify_volume_change(uint8_t volume);
int btif_pts_avrcp_reg_notify_volume_change(void);
int btif_pts_register_auth_cb(bool reg_auth);
int btif_pts_set_class_of_device(uint32_t classOfDevice);

/**
 * @brief dump bt srv info
 *
 * This routine dump bt srv info.
 *
 * @return 0 excute successed , others failed
 */
int btif_dump_brsrv_info(void);

/*=============================================================================
 *				a2dp service api
 *===========================================================================*/

/** Callbacks to report a2dp profile events*/
typedef enum {
	/** notifying that a2dp connected*/
	BTSRV_A2DP_CONNECTED,
	/** notifying that a2dp disconnected */
	BTSRV_A2DP_DISCONNECTED,
	/** notifying that a2dp stream opened */
	BTSRV_A2DP_STREAM_OPENED,
	/** notifying that a2dp stream closed*/
	BTSRV_A2DP_STREAM_CLOSED,
	/** notifying that a2dp stream started */
	BTSRV_A2DP_STREAM_STARED,
	/** notifying that a2dp stream suspend */
	BTSRV_A2DP_STREAM_SUSPEND,
	/** notifying that a2dp stream data indicated */
	BTSRV_A2DP_DATA_INDICATED,
	/** notifying that a2dp codec info */
	BTSRV_A2DP_CODEC_INFO,
	/** notifying that a2dp actived device change*/
	BTSRV_A2DP_ACTIVED_DEV_CHANGED,
	/** Get initialize delay report  */
	BTSRV_A2DP_GET_INIT_DELAY_REPORT,
} btsrv_a2dp_event_e;

/**
 * @brief callback of bt service a2dp profile
 *
 * This routine callback of bt service a2dp profile.
 *
 * @param event id of bt service a2dp event
 * @param packet param of btservice a2dp event
 * @param size length of param
 *
 * @return N/A
 */
typedef void (*btsrv_a2dp_callback)(btsrv_a2dp_event_e event, void *packet, int size);
typedef void (*btsrv_a2dp_hdl_callback)(uint16_t hdl, btsrv_a2dp_event_e event, void *packet, int size);

struct btsrv_a2dp_start_param {
	btsrv_a2dp_hdl_callback cb;
	uint8_t *sbc_codec;
	uint8_t *aac_codec;
	uint8_t sbc_endpoint_num;
	uint8_t aac_endpoint_num;
	uint8_t a2dp_cp_scms_t:1;		/* Support content protection SCMS-T */
	uint8_t a2dp_delay_report:1;	/* Support delay report */
#ifdef CONFIG_BT_A2DP_TRS
	uint8_t *trs_sbc_codec;
	uint8_t trs_sbc_endpoint_num;
	btsrv_a2dp_callback trs_cb;

	uint8_t *trs_aac_codec;
	uint8_t trs_aac_endpoint_num;
#endif
};

/**
 * @brief Rgister a2dp processer
 *
 * @return 0 excute successed , others failed
 */
int btif_a2dp_register_processer(void);

/**
 * @brief start bt service a2dp profile
 *
 * This routine start bt service a2dp profile.
 *
 * @param cbs handle of btsrv event callback
 *
 * @return 0 excute successed , others failed
 */
int btif_a2dp_start(struct btsrv_a2dp_start_param *param);

/**
 * @brief stop bt service a2dp profile
 *
 * This routine stop bt service a2dp profile.
 *
 * @return 0 excute successed , others failed
 */
int btif_a2dp_stop(void);

/**
 * @brief disable bt service a2dp/avrcp profile
 *
 * This routine disable bt service a2dp/avrcp profile.
 *
 * @return 0 excute successed , others failed
 */
int btif_a2dp_disable(void);

/**
 * @brief enable bt service a2dp/avrcp profile
 *
 * This routine enable bt service a2dp/avrcp profile.
 *
 * @return 0 excute successed , others failed
 */
int btif_a2dp_enable(void);

/**
 * @brief connect target device a2dp profile
 *
 * This routine connect target device a2dp profile.
 *
 * @param is_src mark as a2dp source or a2dp sink
 * @param bd bt address of target device
 *
 * @return 0 excute successed , others failed
 */
int btif_a2dp_connect(bool is_src, bd_address_t *bd);

/**
 * @brief disconnect target device a2dp profile
 *
 * This routine disconnect target device a2dp profile.
 *
 * @param bd bt address of target device
 *
 * @return 0 excute successed , others failed
 */
int btif_a2dp_disconnect(bd_address_t *bd);

/**
 * @brief trigger start bt service a2dp profile
 *
 * This routine use by app to check a2dp playback
 * state,if state is playback ,trigger start event
 *
 * @return 0 excute successed , others failed
 */
int btif_a2dp_check_state(void);

/**
 * @brief Send delay report to a2dp active phone
 *
 * This routine Send delay report to a2dp active phone
 *
 * @param delay_time delay time (unit: 1/10 milliseconds)
 *
 * @return 0 excute successed , others failed
 */
int btif_a2dp_send_delay_report(uint16_t delay_time);

/**
 * @brief Get a2d0 active mac address
 *
 * @param addr active a2dp device mac address
 */
void btif_a2dp_get_active_mac(bd_address_t *addr);

/*=============================================================================
 *				avrcp service api
 *===========================================================================*/

struct bt_attribute_info{
	uint32_t id;
	uint16_t character_id;
	uint16_t len;
	uint8_t* data;
}__packed;

#define BT_TOTAL_ATTRIBUTE_ITEM_NUM   5

struct bt_id3_info{
	struct bt_attribute_info item[BT_TOTAL_ATTRIBUTE_ITEM_NUM];
}__packed;

/** avrcp controller cmd*/
typedef enum {
	BTSRV_AVRCP_CMD_PLAY,
	BTSRV_AVRCP_CMD_PAUSE,
	BTSRV_AVRCP_CMD_STOP,
	BTSRV_AVRCP_CMD_FORWARD,
	BTSRV_AVRCP_CMD_BACKWARD,
	BTSRV_AVRCP_CMD_VOLUMEUP,
	BTSRV_AVRCP_CMD_VOLUMEDOWN,
	BTSRV_AVRCP_CMD_MUTE,
	BTSRV_AVRCP_CMD_FAST_FORWARD_START,
	BTSRV_AVRCP_CMD_FAST_FORWARD_STOP,
	BTSRV_AVRCP_CMD_FAST_BACKWARD_START,
	BTSRV_AVRCP_CMD_FAST_BACKWARD_STOP,
	BTSRV_AVRCP_CMD_REPEAT_SINGLE,
	BTSRV_AVRCP_CMD_REPEAT_ALL_TRACK,
	BTSRV_AVRCP_CMD_REPEAT_OFF,
	BTSRV_AVRCP_CMD_SHUFFLE_ON,
	BTSRV_AVRCP_CMD_SHUFFLE_OFF
} btsrv_avrcp_cmd_e;

/** Callbacks to report avrcp profile events*/
typedef enum {
	/** notifying that avrcp connected */
	BTSRV_AVRCP_CONNECTED,
	/** notifying that avrcp disconnected*/
	BTSRV_AVRCP_DISCONNECTED,
	/** notifying that avrcp stoped */
	BTSRV_AVRCP_STOPED,
	/** notifying that avrcp paused */
	BTSRV_AVRCP_PAUSED,
	/** notifying that avrcp playing */
	BTSRV_AVRCP_PLAYING,
	/** notifying cur id3 info */
	BTSRV_AVRCP_UPDATE_ID3_INFO,
	/** notifying cur track change */
	BTSRV_AVRCP_TRACK_CHANGE,
	/** notifying cur playback position */
	BTSRV_AVRCP_PLAYBACK_POS,
} btsrv_avrcp_event_e;

typedef void (*btsrv_avrcp_callback)(uint16_t hdl, btsrv_avrcp_event_e event , void* param);
typedef void (*btsrv_avrcp_get_volume_callback)(uint16_t hdl, uint8_t *volume);
typedef void (*btsrv_avrcp_set_volume_callback)(uint16_t hdl, uint8_t volume);
typedef void (*btsrv_avrcp_get_batt_callback)(uint16_t hdl, uint8_t *batt);
typedef void (*btsrv_avrcp_pass_ctrl_hdl_callback)(uint16_t hdl, uint8_t cmd, uint8_t state);
typedef void (*btsrv_avrcp_pass_ctrl_callback)(void *param, uint8_t cmd, uint8_t state);

typedef struct {
	btsrv_avrcp_callback                  event_cb;
	btsrv_avrcp_get_volume_callback       get_volume_cb;
	btsrv_avrcp_set_volume_callback       set_volume_cb;
	btsrv_avrcp_get_batt_callback         get_batt_cb;
	btsrv_avrcp_pass_ctrl_hdl_callback    pass_ctrl_cb;
#ifdef CONFIG_BT_A2DP_TRS
	btsrv_avrcp_pass_ctrl_callback        trs_pass_ctrl_cb;
	btsrv_avrcp_set_volume_callback       trs_set_volume_cb;
#endif
} btsrv_avrcp_callback_t;

/**
 * @brief Rgister avrcp processer
 *
 * @return 0 excute successed , others failed
 */
int btif_avrcp_register_processer(void);
int btif_avrcp_start(btsrv_avrcp_callback_t *cbs);
int btif_avrcp_stop(void);
int btif_avrcp_connect(bd_address_t *bd);
int btif_avrcp_disconnect(bd_address_t *bd);
int btif_avrcp_send_command(int command);
int btif_avrcp_sync_vol(uint32_t volume);
int btif_avrcp_get_id3_info();
bool btif_avrcp_is_support_get_playback_pos();
int btif_avrcp_get_playback_pos();
int btif_avrcp_set_absolute_volume(uint8_t dev_type, uint8_t *data, uint8_t len);
int btif_avrcp_get_play_status(void);

/*=============================================================================
 *				hfp hf service api
 *===========================================================================*/
/** Callbacks to report avrcp profile events*/
typedef enum {
	/** notifying that hfp connected */
	BTSRV_HFP_CONNECTED,
	/** notifying that hfp disconnected*/
	BTSRV_HFP_DISCONNECTED,
	/** notifying that hfp codec info*/
	BTSRV_HFP_CODEC_INFO,
	/** notifying that hfp phone number*/
	BTSRV_HFP_PHONE_NUM,
	/** notifying that hfp phone number stop report*/
	BTSRV_HFP_PHONE_NUM_STOP,
	/** notifying that hfp in coming call*/
	BTSRV_HFP_CALL_INCOMING,
	/** notifying that hfp call out going*/
	BTSRV_HFP_CALL_OUTGOING,
	/** notifying that hfp ccwa phone number*/
	BTSRV_HFP_CCWA_PHONE_NUM,

	BTSRV_HFP_CALL_3WAYIN,

	/** notifying that hfp call ongoing*/
	BTSRV_HFP_CALL_ONGOING,

	BTSRV_HFP_CALL_MULTIPARTY,

	BTSRV_HFP_CALL_ALERTED,

	BTSRV_HFP_CALL_EXIT,
	BTSRV_HFP_VOLUME_CHANGE,
	BTSRV_HFP_ACTIVE_DEV_CHANGE,
	BTSRV_HFP_SIRI_STATE_CHANGE,
	BTSRV_HFP_SCO,
	/** notifying that sco connected*/
	BTSRV_SCO_CONNECTED,
	/** notifying that sco disconnected*/
	BTSRV_SCO_DISCONNECTED,
	/** notifying that data indicatied*/
	BTSRV_SCO_DATA_INDICATED,
	/** update time from phone*/
	BTSRV_HFP_TIME_UPDATE,
    /** update cgmi from phone*/
    BTSRV_HFP_CLCC_INFO,
	/** at cmd from hf*/
	BTSRV_HFP_AT_CMD,
	/** update cgmi from phone*/
	BTSRV_HFP_CGMI_UPDATE,
    /** notifying that sco rejected*/
    BTSRV_HFP_SCO_REJECTED,
	/** notifying that hfp phone number*/
	BTSRV_HFP_PHONE_NAME,
	/** enable battery high precision report*/
	BTSRV_HFP_BATTERY_HPREC,
} btsrv_hfp_event_e;

typedef enum {
	BTSRV_HFP_STATE_INIT,
	BTSRV_HFP_STATE_LINKED,
	BTSRV_HFP_STATE_CALL_INCOMING,
	BTSRV_HFP_STATE_CALL_OUTCOMING,
	BTSRV_HFP_STATE_CALL_ALERTED,
	BTSRV_HFP_STATE_CALL_ONGOING,
	BTSRV_HFP_STATE_CALL_3WAYIN,
	BTSRV_HFP_STATE_CALL_MULTIPARTY,
	BTSRV_HFP_STATE_SCO_ESTABLISHED,
	BTSRV_HFP_STATE_SCO_RELEASED,
	BTSRV_HFP_STATE_CALL_EXITED,
	BTSRV_HFP_STATE_CALL_SETUP_EXITED,
	BTSRV_HFP_STATE_CALL_HELD,
	BTSRV_HFP_STATE_CALL_MULTIPARTY_HELD,
	BTSRV_HFP_STATE_CALL_HELD_EXITED,
} btsrv_hfp_state;

enum {
	BTSRV_HFP_SIRI_CHANGE_DEACTIVE,
	BTSRV_HFP_SIRI_CHANGE_ACTIVE,
	BTSRV_HFP_SIRI_CHANGE_START,
};

struct btsrv_hfp_clcc_info {
	struct bt_conn *conn;
    uint32_t idx;
    uint32_t dir;
    uint32_t status;
    uint32_t mode;
    uint32_t mpty;
    uint8_t number[24];
};

typedef void (*btsrv_hfp_callback)(btsrv_hfp_event_e event,  uint8_t *param, int param_size);

int btif_hfp_register_processer(void);
int btif_hfp_start(btsrv_hfp_callback cb);
int btif_hfp_stop(void);
int btif_hfp_hf_dial_number(uint8_t *number);
int btif_hfp_hf_dial_last_number(void);
int btif_hfp_hf_dial_memory(int location);
int btif_hfp_hf_volume_control(uint8_t type, uint8_t volume);
int btif_hfp_hf_battery_report(uint8_t mode, uint8_t bat_val);
int btif_hfp_hf_battery_hprec_report(uint8_t bat_val);
int btif_hfp_hf_accept_call(void);
int btif_hfp_hf_reject_call(void);
int btif_hfp_hf_hangup_call(void);
int btif_hfp_hf_hangup_another_call(void);
int btif_hfp_hf_holdcur_answer_call(void);
int btif_hfp_hf_hangupcur_answer_call(void);
int btif_hfp_hf_voice_recognition_start(void);
int btif_hfp_hf_voice_recognition_stop(void);
int btif_hfp_hf_send_at_command(uint8_t *command,uint8_t active_call);
int btif_hfp_hf_switch_sound_source(void);
int btif_hfp_hf_get_call_state(uint8_t active_call,uint8_t *call_state);
int btif_hfp_hf_get_time(void);
void btif_hfp_get_active_mac(bd_address_t *addr);
bool btif_hfp_active_phone_service_ready(void);

io_stream_t sco_upload_stream_create(uint8_t hfp_codec_id);

typedef void (*btsrv_sco_callback)(btsrv_hfp_event_e event,  uint8_t *param, int param_size);
int btif_sco_start(btsrv_sco_callback cb);
int btif_sco_stop(void);

/*=============================================================================
 *				hfp ag service api
 *===========================================================================*/
enum btsrv_hfp_hf_ag_indicators {
	BTSRV_HFP_AG_SERVICE_IND,
	BTSRV_HFP_AG_CALL_IND,
	BTSRV_HFP_AG_CALL_SETUP_IND,
	BTSRV_HFP_AG_CALL_HELD_IND,
	BTSRV_HFP_AG_SINGNAL_IND,
	BTSRV_HFP_AG_ROAM_IND,
	BTSRV_HFP_AG_BATTERY_IND,
	BTSRV_HFP_AG_MAX_IND,
};


typedef void (*btsrv_hfp_ag_callback)(btsrv_hfp_event_e event,  uint8_t *param, int param_size);

int btif_hfp_ag_register_processer(void);
int btif_hfp_ag_start(btsrv_hfp_ag_callback cb);
int btif_hfp_ag_stop(void);
int btif_hfp_ag_update_indicator(enum btsrv_hfp_hf_ag_indicators index, uint8_t value);
int btif_hfp_ag_send_event(uint8_t *event, uint16_t len);

/*=============================================================================
 *				spp service api
 *===========================================================================*/
#define SPP_UUID_LEN		16

/** Callbacks to report spp profile events*/
typedef enum {
	/** notifying that spp register failed */
	BTSRV_SPP_REGISTER_FAILED,
	/** notifying that spp connect failed */
	BTSRV_SPP_REGISTER_SUCCESS,
	/** notifying that spp connect failed */
	BTSRV_SPP_CONNECT_FAILED,
	/** notifying that spp connected*/
	BTSRV_SPP_CONNECTED,
	/** notifying that spp disconnected */
	BTSRV_SPP_DISCONNECTED,
	/** notifying that spp stream data indicated */
	BTSRV_SPP_DATA_INDICATED,
} btsrv_spp_event_e;

typedef void (*btsrv_spp_callback)(btsrv_spp_event_e event, uint8_t app_id, void *packet, int size);

struct bt_spp_reg_param {
	uint8_t app_id;
	uint8_t *uuid;
};

struct bt_spp_connect_param {
	uint8_t app_id;
	bd_address_t bd;
	uint8_t *uuid;
};

int btif_spp_register_processer(void);
int btif_spp_reg(struct bt_spp_reg_param *param);
int btif_spp_send_data(uint8_t app_id, uint8_t *data, uint32_t len);
int btif_spp_connect(struct bt_spp_connect_param *param);
int btif_spp_disconnect(uint8_t app_id);
int btif_spp_start(btsrv_spp_callback cb);
int btif_spp_stop(void);

/*=============================================================================
 *				PBAP service api
 *===========================================================================*/
typedef struct parse_vcard_result pbap_vcard_result_t;

typedef enum {
	/** notifying that pbap connect failed*/
	BTSRV_PBAP_CONNECT_FAILED,
	/** notifying that pbap connected*/
	BTSRV_PBAP_CONNECTED,
	/** notifying that pbap disconnected */
	BTSRV_PBAP_DISCONNECTED,
	/** notifying that pbap max size */
	BTSRV_PBAP_MAX_SIZE,
	/** notifying that pbap vcard result */
	BTSRV_PBAP_SETPATH_FINISH,
	/** notifying that pbap vcard result */
	BTSRV_PBAP_VCARD_RESULT,
	/** notifying that pbap vcard result */
	BTSRV_PBAP_SEARCH_RESULT,
	/** notifying that pbap get vcard or search finish */
	BTSRV_PBAP_GET_VCARD_FINISH,
	/** notifying that pbap vcard end */
	BTSRV_PBAP_END_OF_BODY,
	/** notifying that pbap abort finish */
	BTSRV_PBAP_ABORT_FINISH,
} btsrv_pbap_event_e;

enum {
	BTSRV_PBAP_OP_CMD_CONNECT,
	BTSRV_PBAP_OP_CMD_DISCONNECT,
	BTSRV_PBAP_OP_CMD_GET_SIZE,
	BTSRV_PBAP_OP_CMD_GET_PB,
	BTSRV_PBAP_OP_CMD_SET_PATH,
	BTSRV_PBAP_OP_CMD_GET_VCARD,
	BTSRV_PBAP_OP_CMD_GET_CONTINUE,
	BTSRV_PBAP_OP_CMD_LISTING,
	BTSRV_PBAP_OP_CMD_ABORT,
};

typedef void (*btsrv_pbap_callback)(btsrv_pbap_event_e event, uint8_t app_id, void *data, uint8_t size);

struct bt_pbap_op_param {
	uint8_t cmd;
	uint8_t app_id;
	union {
		struct {
			char *pb_path;
			uint32_t pb_filter;
		};
		struct {
			uint8_t vcard_name_len;
			uint8_t vcard_name[20];
			uint32_t vc_filter;
		};
		struct {
			bd_address_t bd;
			btsrv_pbap_callback cb;
		};
		struct {
			uint8_t order:4;
			uint8_t search_attr:4;
			uint8_t search_len;
			uint8_t search_value[20];
		};
	};
};

int btif_pbap_register_processer(void);
int btif_pbap_cmd_op(struct bt_pbap_op_param *param);

/*=============================================================================
 *				MAP service api
 *===========================================================================*/
typedef enum {
	/** notifying that pbap connect failed*/
	BTSRV_MAP_CONNECT_FAILED,
	/** notifying that pbap connected*/
	BTSRV_MAP_CONNECTED,
	/** notifying that pbap disconnected */
	BTSRV_MAP_DISCONNECTED,
	/** notifying that pbap disconnected */
	BTSRV_MAP_SET_PATH_FINISHED,

	BTSRV_MAP_MESSAGES_RESULT,
} btsrv_map_event_e;

typedef void (*btsrv_map_callback)(btsrv_map_event_e event, uint8_t app_id, void *data, uint8_t size);

struct bt_map_connect_param {
	bd_address_t bd;
	uint8_t app_id;
	char *map_path;
	btsrv_map_callback cb;
};

struct bt_map_get_param {
	bd_address_t bd;
	uint8_t app_id;
	char *map_path;
	btsrv_map_callback cb;
};

struct bt_map_set_folder_param {
	uint8_t app_id;
	uint8_t flags;
	char *map_path; 
};

struct bt_map_get_messages_listing_param {
    uint8_t app_id;
    uint8_t reserved;
    uint16_t max_list_count;
    uint32_t parameter_mask;
};

int btif_map_client_connect(struct bt_map_connect_param *param);
int btif_map_get_message(struct bt_map_get_param *param);
int btif_map_get_messages_listing(struct bt_map_get_messages_listing_param *param);
int btif_map_get_folder_listing(uint8_t app_id);
int btif_map_abort_get(uint8_t app_id);
int btif_map_client_disconnect(uint8_t app_id);
int btif_map_client_set_folder(struct bt_map_set_folder_param *param);
int btif_map_register_processer(void);


/*=============================================================================
 *				hid service api
 *===========================================================================*/

struct bt_device_id_info{
	uint16_t vendor_id;
	uint16_t product_id;
	uint16_t id_source;
};

/* Different report types in get, set, data
*/
enum {
	BTSRV_HID_REP_TYPE_OTHER=0,
	BTSRV_HID_REP_TYPE_INPUT,
	BTSRV_HID_REP_TYPE_OUTPUT,
	BTSRV_HID_REP_TYPE_FEATURE,
};

/* Parameters for Handshake
*/
enum {
	BTSRV_HID_HANDSHAKE_RSP_SUCCESS = 0,
	BTSRV_HID_HANDSHAKE_RSP_NOT_READY,
	BTSRV_HID_HANDSHAKE_RSP_ERR_INVALID_REP_ID,
	BTSRV_HID_HANDSHAKE_RSP_ERR_UNSUPPORTED_REQ,
	BTSRV_HID_HANDSHAKE_RSP_ERR_INVALID_PARAM,
	BTSRV_HID_HANDSHAKE_RSP_ERR_UNKNOWN = 14,
	BTSRV_HID_HANDSHAKE_RSP_ERR_FATAL,
};

#define BT_HID_REPORT_DATA_LEN	64

struct bt_hid_report {
	uint16_t hdl;
	uint8_t report_type;
	uint8_t has_size:1;
	uint8_t reserved:3;
	uint8_t data[BT_HID_REPORT_DATA_LEN];
	int len;
};

/** Callbacks to report hid profile events*/
typedef enum {
	/** notifying that hid connected*/
	BTSRV_HID_CONNECTED,
	/** notifying that hid disconnected */
	BTSRV_HID_DISCONNECTED,
	/** notifying hid get reprot data */
	BTSRV_HID_GET_REPORT,
	/** notifying hid set reprot data */
	BTSRV_HID_SET_REPORT,
	/** notifying hid get protocol data */
	BTSRV_HID_GET_PROTOCOL,
	/** notifying hid set protocol data */
	BTSRV_HID_SET_PROTOCOL,
	/** notifying hid intr data */
	BTSRV_HID_INTR_DATA,
	/** notifying hid unplug */
	BTSRV_HID_UNPLUG,
	/** notifying hid suspend */
	BTSRV_HID_SUSPEND,
	/** notifying hid exit suspend */
	BTSRV_HID_EXIT_SUSPEND,
} btsrv_hid_event_e;

typedef void (*btsrv_hid_callback)(uint16_t hdl, btsrv_hid_event_e event, void *packet, int size);

int btif_hid_register_processer(void);
int btif_did_register_sdp(uint8_t *data, uint32_t len);
int btif_hid_register_sdp(struct bt_sdp_attribute * hid_attrs,uint8_t attrs_size);
int btif_hid_send_ctrl_data(uint16_t hdl, uint8_t report_type,uint8_t *data, uint32_t len);
int btif_hid_send_intr_data(uint16_t hdl, uint8_t report_type,uint8_t *data, uint32_t len);
int btif_hid_send_rsp(uint16_t hdl, uint8_t status);
int btif_hid_connect(uint16_t hdl);
int btif_hid_disconnect(uint16_t hdl);
int btif_hid_start(btsrv_hid_callback cb);
int btif_hid_stop(void);

/*=============================================================================
 *				pnp info service api
 *===========================================================================*/
/** Callbacks to report pnp info events*/
typedef enum {
	/** vendor id get*/
	BTSRV_PNP_VENDOR_ID,
	/** producr id get*/
	BTSRV_PNP_PRODUCT_ID,
} btsrv_pnp_info_event_e;

typedef void (*btsrv_pnp_info_callback)(btsrv_pnp_info_event_e event,  void *param, int param_size);

int btif_pnp_info_register_processer(void);
int btif_pnp_info_search_start(btsrv_pnp_info_callback cb);
int btif_pnp_info_search_stop(void);

#ifdef CONFIG_BT_A2DP_TRS
int btif_br_get_trs_list_info(struct bt_trs_list_dev_t *info, uint8_t max_cnt);
int btif_br_clean_trs_dev_info(bd_address_t *bd);
#endif
/**
 * INTERNAL_HIDDEN @endcond
 */

#endif
