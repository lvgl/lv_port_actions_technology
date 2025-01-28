/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bluetooth manager interface
*/

#ifndef __BT_MANAGER_H__
#define __BT_MANAGER_H__

#include <stream.h>
#include "btservice_api.h"
#include "bt_manager_ble.h"
#include <msg_manager.h>
#include <acts_bluetooth/parse_vcard.h>

#ifdef __cplusplus
	extern "C" {
#endif
/**
 * @defgroup bt_manager_apis Bt Manager APIs
 * @ingroup bluetooth_system_apis
 * @{
 */

#ifdef CONFIG_BT_MAX_BR_CONN
#define BT_MANAGER_MAX_BR_NUM	CONFIG_BT_MAX_BR_CONN
#else
#define BT_MANAGER_MAX_BR_NUM 1
#endif

#ifdef CONFIG_BT_A2DP_TRS
#define BT_A2DP_TRS_DEV_MAX    4
#endif

/** bt stream type */
typedef enum {
    STREAM_TYPE_A2DP,
	STREAM_TYPE_LOCAL,
	STREAM_TYPE_SCO,
    STREAM_TYPE_SPP,
    STREAM_TYPE_MAX_NUM,
} bt_stream_type_e;

/** bt event type */
typedef enum {
	/** param null */
    BT_CONNECTION_EVENT = 2,
	/** param null */
    BT_DISCONNECTION_EVENT,
	/** param null */
    BT_A2DP_CONNECTION_EVENT = 4,
	/** param null */
    BT_A2DP_DISCONNECTION_EVENT,
	/** param null */
    BT_A2DP_STREAM_START_EVENT,
	/** param null */
    BT_A2DP_STREAM_SUSPEND_EVENT,
	/** param null */
    BT_A2DP_STREAM_DATA_IND_EVENT,
	/** param null */
    BT_HFP_CONNECTION_EVENT = 10,
	/** param null */
    BT_HFP_DISCONNECTION_EVENT,
	/** param null */
    BT_HFP_ESCO_ESTABLISHED_EVENT = 13,
	/** param null */
    BT_HFP_ESCO_RELEASED_EVENT,
	/** param null */
    BT_HFP_ACTIVEDEV_CHANGE_EVENT,
	/** param:NULL */
	BT_HFP_CALL_RING_STATR_EVENT,
    /* param null */
    BT_HFP_CALL_CCWA_EVENT,
	/** param:NULL */
	BT_HFP_CALL_RING_STOP_EVENT,
	/** param:NULL */
	BT_HFP_CALL_OUTGOING,
	/** param:NULL */
	BT_HFP_CALL_INCOMING,
	/** param:NULL */
	BT_HFP_CALL_ONGOING,
	/** param:NULL */
	BT_HFP_CALL_SIRI_MODE,
	/** param:NULL */
	BT_HFP_CALL_HUNGUP,
	/** param:NULL */
	BT_HFP_SIRI_START,
	/** param:NULL */
	BT_HFP_SIRI_STOP,
	/** param:NULL */
	BT_HFP_CALL_STATE_START,
	/** param:NULL */
	BT_HFP_CALL_STATE_EXIT,
	/** param:struct btsrv_hfp_clcc_info */
    BT_HFP_CALL_CLCC_INFO, 
	/** param null */
    BT_AVRCP_CONNECTION_EVENT,
	/** param null */
    BT_AVRCP_DISCONNECTION_EVENT,
	/** param :avrcp_ui.h 6.7 Notification PDUs */
    BT_AVRCP_PLAYBACK_STATUS_CHANGED_EVENT,
    /** param :null */
    BT_AVRCP_TRACK_CHANGED_EVENT,
    /** param : struct id3_info avrcp.h */
    BT_AVRCP_UPDATE_ID3_INFO_EVENT,
    /** param : positon */
    BT_AVRCP_UPDATE_PLAYBACK_POS,
	/** param null */
    BT_HID_CONNECTION_EVENT,
	/** param null */
    BT_HID_DISCONNECTION_EVENT,
	/** param hid_active_id */
    BT_HID_ACTIVEDEV_CHANGE_EVENT,
    /** param struct btmgr_map_time */
    BT_MAP_SET_TIME_EVENT,
	/** param null */
	BT_RMT_VOL_SYNC_EVENT,
	/** param null */
    BT_TWS_CONNECTION_EVENT,
	/** param null */
    BT_TWS_DISCONNECTION_EVENT,
	/** param null */
	BT_TWS_CHANNEL_MODE_SWITCH,
	/** param null */
    BT_REQ_RESTART_PLAY,
	/** apple ancs incoming call info */
    BT_ANCS_INCOMMING_CALL_EVENT,
	/** This value will transter to different tws version, must used fixed value */
    /** param null */
	BT_TWS_START_PLAY = 0xE0,
	/** param null */
	BT_TWS_STOP_PLAY = 0xE1,
#ifdef CONFIG_BT_A2DP_TRS		/** Must place in end */
    /** param null */
    BT_TRS_A2DP_STREAM_READY_EVENT,
    /** param null */
    BT_TRS_A2DP_STREAM_START_EVENT,
    /** param null */
    BT_TRS_A2DP_STREAM_SUSPEND_EVENT,
    /** param null */
    BT_TRS_INQUIRY_START_EVENT,
    /** param null */
    BT_TRS_INQUIRY_RESTART_EVENT,
    /** param null */
    BT_TRS_INQUIRY_STOP_EVENT,
	/** param null */
	BT_TRS_AVRCP_PLAY_EVENT,
	/** param null */
	BT_TRS_AVRCP_PAUSE_EVENT,
	/** param null */
	BT_TRS_AVRCP_FORWARD_EVENT,
	/** param null */
	BT_TRS_AVRCP_BACKWARD_EVENT,
	/* param null */
	BT_TRS_A2DP_STREAM_CLOSE_EVENT,
#endif
} bt_event_type_e;

/** bt link state */
enum BT_LINK_STATUS {
    BT_STATUS_NONE,
    BT_STATUS_WAIT_PAIR,
    BT_STATUS_CONNECTED,
    BT_STATUS_DISCONNECTED,
    BT_STATUS_TWS_WAIT_PAIR,
    BT_STATUS_TWS_PAIRED,
    BT_STATUS_TWS_UNPAIRED,
    BT_STATUS_MASTER_WAIT_PAIR,
};

/** bt play state */
enum BT_PLAY_STATUS {
    BT_STATUS_PAUSED				= 0x0001,
    BT_STATUS_PLAYING				= 0x0002,
};

enum BT_HFP_STATUS {
    BT_STATUS_HFP_NONE              = 0x0000,
    BT_STATUS_INCOMING              = 0x0001,
    BT_STATUS_OUTGOING              = 0x0002,
    BT_STATUS_ONGOING               = 0x0004,
    BT_STATUS_MULTIPARTY            = 0x0008,
    BT_STATUS_SIRI                  = 0x0010,
    BT_STATUS_3WAYIN                = 0x0020,
};

/** bt link state */
enum BT_BLE_STATUS {
    BT_STATUS_BLE_NONE,
    BT_STATUS_BLE_ADV,
    BT_STATUS_BLE_CONNECTED,
    BT_STATUS_BLE_DISCONNECTED,
};

/** bt manager battery report mode */
enum BT_BATTERY_REPORT_MODE_E {
	/** bt manager battery report mode init */
    BT_BATTERY_REPORT_INIT = 1,
	/** bt manager battery report mode report data */
    BT_BATTERY_REPORT_VAL,
};


/** bt application connect type */
enum {
	/** no link type */
	NONE_CONNECT_TYPE,
	/** spp link */
	SPP_CONNECT_TYPE,
	/** ble link */
	BLE_CONNECT_TYPE,
};

#define BT_STATUS_A2DP_ALL  (BT_STATUS_NONE \
			    | BT_STATUS_WAIT_PAIR \
			    | BT_STATUS_MASTER_WAIT_PAIR \
			    | BT_STATUS_PAUSED \
			    | BT_STATUS_PLAYING)

#define BT_STATUS_HFP_ALL   (BT_STATUS_INCOMING | BT_STATUS_OUTGOING \
				| BT_STATUS_ONGOING | BT_STATUS_MULTIPARTY | BT_STATUS_SIRI | BT_STATUS_3WAYIN)

typedef enum
{
	DISABLE_TEST = 0,	/* 0: Bluetooth normal run, not enter BQB */
	DUT_TEST = 1,		/* 1: BR BQB test  */
	LE_TEST = 2,		/* 2: BLE only BQB test */
	DUT_LE_TEST =3, 	/* 3: BR/BLE dual BQB test */
} bt_test_mode_e;

/** bt manager spp call back */
struct btmgr_spp_cb {
	void (*connect_failed)(uint8_t channel);
	void (*connected)(uint8_t channel, uint8_t *uuid);
	void (*disconnected)(uint8_t channel);
	void (*receive_data)(uint8_t channel, uint8_t *data, uint32_t len);
};

/** bt manager spp ble strea init param */
struct sppble_stream_init_param {
	uint8_t *spp_uuid;
	void *gatt_attr;
	uint8_t attr_size;
	void *tx_chrc_attr;
	void *tx_attr;
	void *tx_ccc_attr;
	void *rx_attr;
	void *connect_cb;
#if defined(CONFIG_OTA_PRODUCT_SUPPORT) || defined(CONFIG_OTA_BLE_MASTER_SUPPORT)
	void *rxdata_cb;
#endif
	int32_t read_timeout;
	int32_t write_timeout;
	int32_t read_buf_size;
};

/** bt manager pbap vcard filter bit */
enum {
	BT_PBAP_FILTER_VERSION              = (0x1 << VCARD_TYPE_VERSION),
	BT_PBAP_FILTER_FN                   = (0x1 << VCARD_TYPE_FN),
	BT_PBAP_FILTER_N                    = (0x1 << VCARD_TYPE_N),
	BT_PBAP_FILTER_PHOTO                = (0x1 << VCARD_TYPE_PHOTO),
	BT_PBAP_FILTER_BDAY                 = (0x1 << VCARD_TYPE_BDAY),
	BT_PBAP_FILTER_ADR                  = (0x1 << VCARD_TYPE_ADR),

	BT_PBAP_FILTER_LABEL                = (0x1 << VCARD_TYPE_LABEL),
	BT_PBAP_FILTER_TEL                  = (0x1 << VCARD_TYPE_TEL),
	BT_PBAP_FILTER_EMAIL                = (0x1 << VCARD_TYPE_EMAIL),
	BT_PBAP_FILTER_MAILER               = (0x1 << VCARD_TYPE_MAILER),
	BT_PBAP_FILTER_TZ                   = (0x1 << VCARD_TYPE_TZ),
	BT_PBAP_FILTER_GEO                  = (0x1 << VCARD_TYPE_GEO),

	BT_PBAP_FILTER_TITLE                = (0x1 << VCARD_TYPE_TITLE),
	BT_PBAP_FILTER_ROLE                 = (0x1 << VCARD_TYPE_ROLE),
	BT_PBAP_FILTER_LOGO                 = (0x1 << VCARD_TYPE_LOGO),
	BT_PBAP_FILTER_AGENT                = (0x1 << VCARD_TYPE_AGENT),
	BT_PBAP_FILTER_ORG                  = (0x1 << VCARD_TYPE_ORG),
	BT_PBAP_FILTER_NOTE                 = (0x1 << VCARD_TYPE_NOTE),

	BT_PBAP_FILTER_REV                  = (0x1 << VCARD_TYPE_REV),
	BT_PBAP_FILTER_SOUND                = (0x1 << VCARD_TYPE_SOUND),
	BT_PBAP_FILTER_URL                  = (0x1 << VCARD_TYPE_URL),
	BT_PBAP_FILTER_UID                  = (0x1 << VCARD_TYPE_UID),
	BT_PBAP_FILTER_KEY                  = (0x1 << VCARD_TYPE_KEY),
	BT_PBAP_FILTER_NICKNAME             = (0x1 << VCARD_TYPE_NICKNAME),

	BT_PBAP_FILTER_CATEGORIES           = (0x1 << VCARD_TYPE_CATEGORIES),
	BT_PBAP_FILTER_PROID                = (0x1 << VCARD_TYPE_PROID),
	BT_PBAP_FILTER_CLASS                = (0x1 << VCARD_TYPE_CLASS),
	BT_PBAP_FILTER_SORT_STRING          = (0x1 << VCARD_TYPE_SORT_STRING),
	BT_PBAP_FILTER_X_IRMC_CALL_DATETIME = (0x1 << VCARD_TYPE_X_IRMC_CALL_DATETIME),
	BT_PBAP_FILTER_X_BT_SPEEDDIALKEY    = (0x1 << VCARD_TYPE_X_BT_SPEEDDIALKEY),

	BT_PBAP_FILTER_X_BT_UCI             = (0x1 << VCARD_TYPE_X_BT_UCI),
	BT_PBAP_FILTER_TYPE_X_BT_UID        = (0x1 << VCARD_TYPE_X_BT_UID),
};

/** bt manager pbap callback result */
struct mgr_pbap_result {
	uint8_t type;
	uint16_t len;
	uint8_t *data;
};

/** bt manager map callback result */
struct mgr_map_result {
	uint8_t type;
	uint16_t len;
	uint8_t *data;
};

/** bt manager pbap callback functions */
struct btmgr_pbap_cb {
	void (*connect_failed)(uint8_t app_id);
	void (*connected)(uint8_t app_id);
	void (*disconnected)(uint8_t app_id);
	void (*max_size)(uint8_t app_id, uint16_t max_size);
	void (*result)(uint8_t app_id, struct mgr_pbap_result *result, uint8_t size);
	void (*setpath_finish)(uint8_t app_id);
	void (*search_result)(uint8_t app_id, struct mgr_pbap_result *result, uint8_t size);
	void (*get_vcard_finish)(uint8_t app_id);
	void (*end_of_body)(uint8_t app_id);
	void (*abort)(uint8_t app_id);
};

/** bt manager map callback functions */
struct btmgr_map_cb {
	void (*connect_failed)(uint8_t app_id);
	void (*connected)(uint8_t app_id);
	void (*disconnected)(uint8_t app_id);
	void (*set_path_finished)(uint8_t user_id);
	void (*result)(uint8_t app_id, struct mgr_map_result *result, uint8_t size);
};

#define BTMGR_BT_NAME_LEN	32

/** bt manager connect device information */
struct btmgr_connect_dev_info {
	bd_address_t addr;
	uint16_t is_trs:1;
	uint8_t name[BTMGR_BT_NAME_LEN + 1];
};

struct btmgr_map_time{
    uint8_t tm_sec;
	uint8_t tm_min;
	uint8_t tm_hour;
	uint8_t tm_mday;
	uint8_t tm_mon;
	uint8_t tm_wday;
	uint16_t tm_year;
	uint16_t tm_ms;
};

/**
 * @brief bt manager init
 *
 * This routine init bt manager
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_init(void);

/**
 * @brief bt manager deinit
 *
 * This routine deinit bt manager
 *
 * @return None
 */
void bt_manager_deinit(void);

/**
 * @brief bt manager check inited
 *
 * This routine check bt manager is inited
 *
 * @return true if inited others false
 */
bool bt_manager_is_inited(void);

/**
 * @brief bt manager disconnect device
 *
 * This routine disconnect device
 *
 * @param mode : BTSRV_DISCONNECT_ALL_MODE/BTSRV_DISCONNECT_PHONE_MODE/BTSRV_DISCONNECT_TWS_MODE
 *
 * @return None.
 */
void bt_manager_disconnect_device(uint8_t mode);

/**
 * @brief bt manager get connected device info
 *
 * This routine get connected device info
 *
 * @param info For save connected device info.
 * @param info_num Get max device info.
 *
 * @return int Number of connected device.
 */
int bt_manager_get_connected_dev_info(struct btmgr_connect_dev_info *info, uint8_t info_num);

/**
 * @brief dump bt manager info
 *
 * This routine dump bt manager info
 *
 * @return None
 */
void bt_manager_dump_info(void);

/**
 * @brief bt manager get bt device state
 *
 * This routine provides to get bt device state.
 *
 * @return state of bt device state
 */
int bt_manager_get_status(void);

/**
 * @brief bt manager allow sco connect
 *
 * This routine provides to allow sco connect or not
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_allow_sco_connect(bool allowed);

/**
 * @brief get profile a2dp codec id
 *
 * This routine provides to get profile a2dp codec id .
 *
 * @param type Device type
 *
 * @return codec id of a2dp profile
 */
int bt_manager_a2dp_get_codecid(uint8_t type);

/**
 * @brief get profile a2dp sample rate
 *
 * This routine provides to get profile a2dp sample rate.
 *
 * @param type Device type
 *
 * @return sample rate of profile a2dp
 */
int bt_manager_a2dp_get_sample_rate(uint8_t type);

/**
 * @brief get profile a2dp max bitpool
 *
 * This routine provides to get profile a2dp max bitpool
 *
 * @param type Device type
 *
 * @return max bitpool of profile a2dp(Only sbc has max bitpool)
 */
int bt_manager_a2dp_get_max_bitpool(uint8_t type);

/**
 * @brief check a2dp state
 *
 * This routine use by app to check a2dp playback
 * state,if state is playback ,trigger start event
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_a2dp_check_state(void);

/**
 * @brief Send delay report to a2dp active phone
 *
 * This routine Send delay report to a2dp active phone
 *
 * @param delay_time delay time (unit: 1/10 milliseconds)
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_a2dp_send_delay_report(uint16_t delay_time);

/**
 * @brief disable bt music
 *
 * This routine Send delay report to a2dp active phone
 *
 * @param delay_time delay time (unit: 1/10 milliseconds)
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_a2dp_disable(void);

/**
 * @brief enable bt music
 *
 * This routine open a2dp/avrcp profile
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_a2dp_enable(void);

/**
 * @brief Control Bluetooth to start playing through AVRCP profile
 *
 * This routine provides to Control Bluetooth to start playing through AVRCP profile.
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_avrcp_play(void);

/**
 * @brief Control Bluetooth to stop playing through AVRCP profile
 *
 * This routine provides to Control Bluetooth to stop playing through AVRCP profile.
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_avrcp_stopplay(void);

/**
 * @brief Control Bluetooth to pause playing through AVRCP profile
 *
 * This routine provides to Control Bluetooth to pause playing through AVRCP profile.
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_avrcp_pause(void);

/**
 * @brief Control Bluetooth to play next through AVRCP profile
 *
 * This routine provides to Control Bluetooth to play next through AVRCP profile.
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_avrcp_play_next(void);

/**
 * @brief Control Bluetooth to play previous through AVRCP profile
 *
 * This routine provides to Control Bluetooth to play previous through AVRCP profile.
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_avrcp_play_previous(void);

/**
 * @brief Control Bluetooth to play fast forward through AVRCP profile
 *
 * This routine provides to Control Bluetooth to play fast forward through AVRCP profile.
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_avrcp_fast_forward(bool start);

/**
 * @brief Control Bluetooth to play fast backward through AVRCP profile
 *
 * This routine provides to Control Bluetooth to play fast backward through AVRCP profile.
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_avrcp_fast_backward(bool start);

/**
 * @brief Control Bluetooth to sync vol to remote through AVRCP profile
 *
 * This routine provides to Control Bluetooth to sync vol to remote through AVRCP profile.
 *
 * @param vol volume want to sync
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_avrcp_sync_vol_to_remote(uint32_t vol);

/**
 * @brief Control Bluetooth to get current playback track's id3 info through AVRCP profile
 *
 * This routine provides to Control Bluetooth to get id3 info of cur track.
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_avrcp_get_id3_info(void);

/**
 * @brief Control Bluetooth to get current playback position.
 *
 * This routine provides to Control Bluetooth to get current playback position.
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_avrcp_get_playback_pos(void);

/**
 * @brief Avrcp set absolute volume
 *
 * @param dev_type: device type (BTSRV_DEVICE_PHONE or BTSRV_DEVICE_PLAYER)
 * @param volume: absolute volume data (1~2byte, default 1byte,  vendor 2byte)
 *                 1byte volume: Value 0x0 corresponds to 0%. Value 0x7F corresponds to 100%.
 * @param len: 1~3byte
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_avrcp_set_absolute_volume(uint8_t dev_type, uint8_t *data, uint8_t len);

/**
 * @brief get hfp status
 *
 * This routine provides to get hfp status .
 *
 * @return state of hfp status.
 */
int bt_manager_hfp_get_status(void);

/**
 * @brief Control Bluetooth to dial target phone number through HFP profile
 *
 * This routine provides to Control Bluetooth to Control Bluetooth to dial target phone number through HFP profile .
 *
 * @param number number of target phone
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_dial_number(uint8_t *number);

/**
 * @brief Control Bluetooth to dial last phone number through HFP profile
 *
 * This routine provides to Control Bluetooth to Control Bluetooth to dial last phone number through HFP profile .
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_dial_last_number(void);

/**
 * @brief Control Bluetooth to dial local memory phone number through HFP profile
 *
 * This routine provides to Control Bluetooth to Control Bluetooth to dial local memory phone number through HFP profile .
 *
 * @param location index of local memory phone number
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_dial_memory(int location);

/**
 * @brief Control Bluetooth to volume control through HFP profile
 *
 * This routine provides to Control Bluetooth to Control Bluetooth to volume control through HFP profile .
 *
 * @param type type of opertation
 * @param volume level of volume
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_volume_control(uint8_t type, uint8_t volume);

/**
 * @brief Control Bluetooth to report battery state through HFP profile
 *
 * This routine provides to Control Bluetooth to Control Bluetooth to report battery state through HFP profile .
 *
 * @param mode mode of operation, 0 init mode , 1 report mode
 * @param bat_val battery level value
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_battery_report(uint8_t mode, uint8_t bat_val);

/**
 * @brief Control Bluetooth to report battery state through HFP profile
 *
 * This routine provides to Control Bluetooth to Control Bluetooth to report battery state through HFP profile .
 *
 * @param bat_val battery level value,  1% precision
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_battery_hprec_report(uint8_t bat_val);

/**
 * @brief Control Bluetooth to accept call through HFP profile
 *
 * This routine provides to Control Bluetooth to Control Bluetooth to accept call through HFP profile .
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_accept_call(void);

/**
 * @brief Control Bluetooth to reject call through HFP profile
 *
 * This routine provides to Control Bluetooth to Control Bluetooth to reject call through HFP profile .
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_reject_call(void);

/**
 * @brief Control Bluetooth to hangup current call through HFP profile
 *
 * This routine provides to Control Bluetooth to Control Bluetooth to hangup current call through HFP profile .
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_hangup_call(void);

/**
 * @brief Control Bluetooth to hangup another call through HFP profile
 *
 * This routine provides to Control Bluetooth to Control Bluetooth to hangup another call through HFP profile .
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_hangup_another_call(void);

/**
 * @brief Control Bluetooth to hold current and answer another call through HFP profile
 *
 * This routine provides to Control Bluetooth to Control Bluetooth to hold current and answer another call through HFP profile .
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_holdcur_answer_call(void);

/**
 * @brief Control Bluetooth to hangup current and answer another call through HFP profile
 *
 * This routine provides to Control Bluetooth to Control Bluetooth to hangup current and answer another call through HFP profile .
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_hangupcur_answer_call(void);

/**
 * @brief Control Bluetooth to start siri through HFP profile
 *
 * This routine provides to Control Bluetooth to Control Bluetooth to start siri through HFP profile .
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_start_siri(void);

/**
 * @brief Control Bluetooth to stop siri through HFP profile
 *
 * This routine provides to Control Bluetooth to Control Bluetooth to stop siri through HFP profile .
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_stop_siri(void);

/**
 * @brief Control Bluetooth to sycn time from phone through HFP profile
 *
 * This routine provides to sycn time from phone through HFP profile .
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_get_time(void);

/**
 * @brief send user define at command through HFP profile
 *
 * This routine provides to send user define at command through HFP profile .
 *
 * @param command user define at command
 * @param send cmd to active_call or inactive call
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_send_at_command(uint8_t *command,uint8_t active_call);

/**
 * @brief sync volume through HFP profile
 *
 * This routine provides to sync volume through HFP profile .
 *
 * @param vol volume want to sync
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_sync_vol_to_remote(uint32_t vol);

/**
 * @brief switch sound source through HFP profile
 *
 * This routine provides to sswitch sound source HFP profile .

 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_switch_sound_source(void);

/**
 * @brief set hfp status
 *
 * This routine provides to set hfp status .
 *
 * @param state state of hfp profile
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_set_status(uint32_t state);

/**
 * @brief get profile hfp codec id
 *
 * This routine provides to get profile hfp codec id .
 *
 * @return codec id of hfp profile
 */
int bt_manager_sco_get_codecid(void);

/**
 * @brief get profile hfp sample rate
 *
 * This routine provides to get profile hfp sample rate.
 *
 * @return sample rate of profile hfp
 */
int bt_manager_sco_get_sample_rate(void);

/**
 * @brief get profile hfp call state
 *
 * This routine provides to get profile hfp call state.
 * @param active_call to get active call or inactive call
 *
 * @return call state of call
 */
int bt_manager_hfp_get_call_state(uint8_t active_call,uint8_t *call_state);

/**
 * @brief set bt call indicator by app
 *
 * This routine provides to set bt call indicator by app through HFP profile .
 * @param index  call status index to set (enum btsrv_hfp_hf_ag_indicators)
 * @param value  call status
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_ag_update_indicator(enum btsrv_hfp_hf_ag_indicators index, uint8_t value);

/**
 * @brief send call event
 *
 * This routine provides to send call event through HFP profile .
 * @param event  call event
 * @param len  event len
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_hfp_ag_send_event(uint8_t *event, uint16_t len);

/**
 * @brief spp reg uuid
 *
 * This routine provides to spp reg uuid, return channel id.
 *
 * @param uuid uuid of spp link
 * @param c call back of spp link
 *
 * @return channel of spp connected
 */
int bt_manager_spp_reg_uuid(uint8_t *uuid, struct btmgr_spp_cb *c);

/**
 * @brief spp send data
 *
 * This routine provides to send data througth target spp link
 *
 * @param chl channel of spp link
 * @param data pointer of send data
 * @param len length of send data
 *
 * @return > 0 : Send data length; <= 0 : Send failed;
 */
int bt_manager_spp_send_data(uint8_t chl, uint8_t *data, uint32_t len);

/**
 * @brief spp connect
 *
 * This routine provides to connect spp channel
 *
 * @param uuid uuid of spp connect
 *
 * @return 0 excute successed , others failed
 */
uint8_t bt_manager_spp_connect(bd_address_t *bd, uint8_t *uuid, struct btmgr_spp_cb *spp_cb);
uint8_t bt_manager_spp_connect_by_uuid(bd_address_t *bd, uint8_t *uuid);

/**
 * @brief spp disconnect
 *
 * This routine provides to disconnect spp channel
 *
 * @param chl channel of spp connect
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_spp_disconnect(uint8_t chl);

/**
 * @brief PBAP connect
 *
 * This routine connect PBAP
 *
 * @param bd device bluetooth address
 * @param cb callback function
 *
 * @return 0 failed, others success with app_id
 */
uint8_t btmgr_pbap_connect(bd_address_t *bd, struct btmgr_pbap_cb *cb);

/**
 * @brief disconnect pbap
 *
 * This routine provides to disconnect pbap
 *
 * @param app_id app_id
 *
 * @return 0 excute successed , others failed
 */
int btmgr_pbap_disconnect(uint8_t app_id);

/**
 * @brief pbap get size
 *
 * This routine provides to get pbap size
 *
 * @param app_id app_id
 * @param path phonebook path
 *
 * @return 0 excute successed , others failed
 */
int btmgr_pbap_get_size(uint8_t app_id, char *path);

/**
 * @brief pbap get phonebook
 *
 * This routine provides to get pbap phonebook
 *
 * @param app_id app_id
 * @param path phonebook path
 * @param filter Vcard filter
 *
 * @return 0 excute successed , others failed
 */
int btmgr_pbap_get_pb(uint8_t app_id, char *path, uint32_t filter);

/**
 * @brief pbap set phonebook path
 *
 * This routine provides to set phonebook path
 *
 * @param app_id app_id
 * @param path phonebook path
 *
 * @return 0 excute successed , others failed
 */
int btmgr_pbap_setpath(uint8_t app_id, char *path);

/**
 * @brief pbap get vcard
 *
 * This routine provides to get pbap vcard
 *
 * @param app_id app_id
 * @param name Vcard name
 * @param len Vcard name length
 * @param filter Vcard filter
 *
 * @return 0 excute successed , others failed
 */
int btmgr_pbap_get_vcard(uint8_t app_id, char *name, uint8_t len, uint32_t filter);

/**
 * @brief pbap get vcard continue
 *
 * This routine provides to get pbap vcard continue
 *
 * @param app_id app_id
 *
 * @return 0 excute successed , others failed
 */
int btmgr_pbap_get_vcard_continue(uint8_t app_id);

/**
 * @brief pbap search vcard by value
 *
 * This routine provides to list vcard by value
 *
 * @param app_id app_id
 * @param order list order, 0:indexed, 1:alphanumeric, 2:phonetic
 * @param attr 0: name, 1: number, 2: sound
 * @param value  Search value
 * @param len  Search value length
 *
 * @return 0 excute successed , others failed
 */
int btmgr_pbap_listing(uint8_t app_id, uint8_t order, uint8_t attr, char *value, uint8_t len);

/**
 * @brief abort phonebook
 *
 * This routine provides to abort phonebook
 *
 * @param app_id app_id
 *
 * @return 0 excute successed , others failed
 */
int btmgr_pbap_abort(uint8_t app_id);

/**
 * @brief hid send data on intr channel
 *
 * This routine provides to send data throug intr channel
 *
 * @param report_type input or output
 * @param data pointer of send data
 * @param len length of send data
 *
 * @return 0 excute successed , others failed
 */
#ifdef CONFIG_BT_HID
int bt_manager_hid_send_intr_data(uint16_t hdl, uint8_t report_type, uint8_t *data, uint32_t len);
#else
static inline int bt_manager_hid_send_intr_data(uint16_t hdl, uint8_t report_type, uint8_t *data, uint32_t len)
{
	return -1;
}
#endif

/**
 * @brief hid send data on ctrl channel
 *
 * This routine provides to send data throug ctrl channel
 *
 * @param report_type input or output
 * @param data pointer of send data
 * @param len length of send data
 *
 * @return 0 excute successed , others failed
 */
#ifdef CONFIG_BT_HID
int bt_manager_hid_send_ctrl_data(uint16_t hdl, uint8_t report_type, uint8_t *data, uint32_t len);
#else
static inline int bt_manager_hid_send_ctrl_data(uint16_t hdl, uint8_t report_type, uint8_t *data, uint32_t len)
{
	return -1;
}
#endif

/**
 * @brief hid send take photo key msg on intr channel
 *
 * This routine provides to hid send take photo key msg on intr channel
 *
 * @return 0 excute successed , others failed
 */
#ifdef CONFIG_BT_HID
int bt_manager_hid_take_photo(void);
int bt_manager_hid_send_key(uint8_t report_id);
int bt_manager_hid_reconnect(void);
int bt_manager_hid_disconnect(void);
#else
static inline int bt_manager_hid_take_photo(void)
{
	return -1;
}
static inline int bt_manager_hid_send_key(uint8_t report_id)
{
	return -1;
}
#endif

/**
 * @brief bt manager get connected phone device num
 *
 * This routine provides to get connected device num.
 *
 * @return number of connected device
 */
int bt_manager_get_connected_dev_num(void);

/**
 * @brief bt manager clear connected list
 *
 * This routine provides to clear connected list
 * if have bt device connected , disconnect it before clear connected list.
 *
 * @return None
 */
void bt_manager_clear_list(int mode);

/**
 * @brief bt manager set stream type
 *
 * This routine set stream type to bt manager
 *
 * @return None
 */
void bt_manager_set_stream_type(uint8_t stream_type);

/**
 * @brief bt manager set codec
 *
 * This routine set codec to tws, only need by set a2dp stream
 *
 * @return None
 */
void bt_manager_set_codec(uint8_t codec);

/**
 * @brief Start br disover
 *
 * This routine Start br disover.
 *
 * @param disover parameter
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_br_start_discover(struct btsrv_discover_param *param);

/**
 * @brief Stop br disover
 *
 * This routine Stop br disover.
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_br_stop_discover(void);

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
int bt_manager_br_connect(bd_address_t *bd);

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
int bt_manager_br_disconnect(bd_address_t *bd);

/**
 * @brief bt manager lock stream pool
 *
 * This routine provides to unlock stream pool
 *
 * @return None
 */
void bt_manager_stream_pool_lock(void);

/**
 * @brief bt manager unlock stream pool
 *
 * This routine provides to unlock stream pool
 *
 * @return None
 */
void bt_manager_stream_pool_unlock(void);

/**
 * @brief bt manager set target type stream enable or disable
 *
 * This routine provides to set target type stream enable or disable
 * @param type type of stream
 * @param enable true :set stream enable false: set steam disable
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_stream_enable(int type, bool enable);

/**
 * @brief bt manager check target type stream enable
 *
 * This routine provides to check target type stream enable
 * @param type type of stream
 *
 * @return ture stream is enable
 * @return false stream is not enable
 */
bool bt_manager_stream_is_enable(int type);

/**
 * @brief bt manager set target type stream
 *
 * This routine provides to set target type stream
 * @param type type of stream
 * @param stream handle of stream
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_set_stream(int type, io_stream_t stream);

/**
 * @brief bt manager get target type stream
 *
 * This routine provides to get target type stream
 * @param type type of stream
 *
 * @return handle of stream
 */
io_stream_t bt_manager_get_stream(int type);

/**
 * @cond INTERNAL_HIDDEN
 */

/**
 * @brief tws wait pair
 *
 * This routine provides to make device enter to tws pair modes
 *
 * @return None
 */
void bt_manager_tws_wait_pair(void);

/**
 * @brief tws get device role
 *
 * This routine provides to get tws device role
 *
 * @return device role of tws
 */
int bt_manager_tws_get_dev_role(void);

/**
 * @brief tws get peer version
 *
 * This routine provides to get tws peer version
 *
 * @return tws peer version
 */
uint8_t bt_manager_tws_get_peer_version(void);

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @brief Create a new spp ble stream
 *
 * This routine Create a new spp ble stream
 * @param init param for spp ble stream
 *
 * @return  handle of stream
 */
io_stream_t sppble_stream_create(void *param);

/**
 * @brief bt manager state notify
 *
 * This routine provides to notify bt state .
 *
 * @param state  state of bt device.
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_state_notify(int state);

/**
 * @brief bt manager event notify
 *
 * This routine provides to notify bt event .
 *
 * @param event_id  id of bt event
 * @param event_data param of bt event
 * @param event_data_size param size of bt event
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_event_notify(int event_id, void *event_data, int event_data_size);

/**
 * @brief bt manager event notify
 *
 * This routine provides to notify bt event .
 *
 * @param event_id  id of bt event
 * @param event_data param of bt event
 * @param event_data_size param size of bt event
 * @param call_cb param callback function to free mem 
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_event_notify_ext(int event_id, void *event_data, int event_data_size , MSG_CALLBAK call_cb);

/**
 * @brief Get bluetooth wake lock
 *
 * This routine get bluetooth wake lock.
 *
 * @return 0: idle can sleep,  other: busy can't sleep.
 */
int bt_manager_get_wake_lock(void);

/**
 * @brief bt manager set bt device state
 *
 * This routine provides to set bt device state.
 * @param state
 *
 * @return 0 excute successed , others failed
 */
int bt_manager_set_status(int state);

/**
 * @brief bt manager check device mac and name
 *
 * This routine  check device mac and name.
 *
 * @return None
 */
void bt_manager_check_mac_name(void);

/**
 * @cond INTERNAL_HIDDEN
 */

/**
 * @brief bt manager TWS sync volume to slave
 *
 * This routine for tws sync volume to slave.
 * @param media_type Media type
 * @param music_vol Volume
 *
 * @return None
 */
void bt_manager_tws_sync_volume_to_slave(uint32_t media_type, uint32_t music_vol);

/**
 * @brief bt manager TWS send command to peer
 *
 * This routine for tws send command to peer.
 * @param command Command buffer
 * @param command_len Command length
 *
 * @return None
 */
void bt_manager_tws_send_command(uint8_t *command, int command_len);

/**
 * @brief bt manager TWS send sync command to peer
 *
 * This routine for tws send sync command to peer.
 * @param command Command buffer
 * @param command_len Command length
 *
 * @return None
 */
void bt_manager_tws_send_sync_command(uint8_t *command, int command_len);

/**
 * @brief bt manager TWS send event to peer
 *
 * This routine for tws send event to peer.
 * @param event Event to send
 * @param event_param Event parameter
 *
 * @return None
 */
void bt_manager_tws_send_event(uint8_t event, uint32_t event_param);

/**
 * @brief bt manager TWS send sync event to peer
 *
 * This routine for tws send sync event to peer.
 * @param event Event to send
 * @param event_param Event parameter
 *
 * @return None
 */
void bt_manager_tws_send_event_sync(uint8_t event, uint32_t event_param);

/**
 * @brief bt manager TWS notify peer start play
 *
 * This routine for tws notify peer start play.
 * @param media_type Media type to play
 * @param codec_id Codec to play
 * @param sample_rate Sample rate
 *
 * @return None
 */
void bt_manager_tws_notify_start_play(uint8_t media_type, uint8_t codec_id, uint8_t sample_rate);

/**
 * @brief bt manager TWS notify peer stop play
 *
 * This routine for tws notify peer stop play.
 *
 * @return None
 */
void bt_manager_tws_notify_stop_play(void);

/**
 * @brief bt manager get tws runtime observer function struct
 *
 * This routine get tws runtime observer function struct.
 *
 * @return tws_runtime_observer_t Observer function struct
 */
tws_runtime_observer_t *bt_manager_tws_get_runtime_observer(void);

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @brief bt manager set sco codec information
 *
 * This routine set sco codec information.
 * @param codec_id Codec to play
 * @param sample_rate Sample rate
 *
 * @return None
 */
void bt_manager_sco_set_codec_info(uint8_t codec_id, uint8_t sample_rate);

/**
 * @brief bt manager set allow sco connect by phone
 *
 * This routine set allow sco connect by phone.
 * @param allowed True allow, false disallow
 *
 * @return int 0 Success, other failed
 */
int bt_manager_allow_sco_connect(bool allowed);

/**
 * @brief bt manager halt phone
 *
 * This routine halt phone.
 *
 * @return None
 */
void bt_manager_halt_phone(void);

/**
 * @brief bt manager resume phone
 *
 * This routine resume phone.
 *
 * @return None
 */
void bt_manager_resume_phone(void);

/**
 * @brief bt manager set BR visible
 *
 * This routine set BR visible.
 * @param connectable True visible, False Disable visible.
 *
 * @return None
 */
void bt_manager_set_visible(bool visible);

/**
 * @brief bt manager set BR connectable
 *
 * This routine set BR connectable.
 * @param connectable True Connectable, False Disable connectable.
 *
 * @return None
 */
void bt_manager_set_connectable(bool connectable);

/**
 * @brief bt manager stop auto reconnect
 *
 * @param None
 *
 * @return int 0 Success, other failed
 */
int bt_manager_stop_auto_reconnect(void);

/**
 * @brief bt manager check is auto reconnect in runing
 *
 * This routine check is auto reconnect in runing.
 *
 * @return True In runing. False Not in runing.
 */
bool bt_manager_is_auto_reconnect_runing(void);

/**
 * @brief bt manager get address linkkey
 *
 * This routine get address linkkey.
 * @param addr Address.
 * @param linkkey NULL or uint8_t buf[16].
 *
 * @return 0, get, other not find.
 */
int bt_manager_get_addr_linkkey(bd_address_t *addr, uint8_t *linkkey);

/**
 * @brief bt manager get linkkey
 *
 * This routine get linkkey.
 * @param info Output linkkey information.
 * @param cnt Max linkkey can get.
 *
 * @return int Number of linkkey returen.
 */
int bt_manager_get_linkkey(struct bt_linkkey_info *info, uint8_t cnt);

/**
 * @brief bt manager update linkkey
 *
 * This routine update linkkey.
 * @param info Linkkey information for update.
 * @param cnt Number of linkkey for update.
 *
 * @return int 0 Success, other Failed.
 */
int bt_manager_update_linkkey(struct bt_linkkey_info *info, uint8_t cnt);

/**
 * @brief bt manager write original linkkey
 *
 * This routine write original linkkey.
 * @param addr Address for write linkkey.
 * @param link_key Linkkey.
 *
 * @return int 0 Success, other Failed.
 */
int bt_manager_write_ori_linkkey(bd_address_t *addr, uint8_t *link_key);

int bt_manager_get_connected_dev_rssi(bd_address_t *addr,btsrv_get_rssi_result_cb cb);

/**
 * @brief bt manager get actived dev rssi
 *
 * This routine get actived dev rssi.
 * @param rssi for save value.
 *
 * @return int 0 Success, 0x7F Failed.
 */
int bt_manager_get_actived_dev_rssi(int8_t *rssi);

/**
 * @brief bt manager clear br linkkey
 *
 * This routine clear br linkkey.
 * @param addr for one device and NULL for all.
 *
 * @return None
 */
void bt_manager_clear_linkkey(bd_address_t *addr);

/**
 * @brief bt manager connnect br resolve address
 *
 * This routine connnect br resolve address.
 * @param param Resolve parameter.
 *
 * @return None
 */
void bt_manager_br_resolve_connect(void *param);

/**
 * @cond INTERNAL_HIDDEN
 */

/**
 * @brief bt manager tws check both is woodpecker device
 *
 * This routine for tws check both is woodpecker device.
 *
 * @return bool True Both are woodpecker device, False peer not woodpecker device.
 */
bool bt_manager_tws_check_is_woodpecker(void);

/**
 * @brief bt manager tws check both is support select feature
 *
 * This routine for tws check both is support select feature
 * @param feature Select feature.
 *
 * @return bool True Both are support, False at least one device not support.
 */
bool bt_manager_tws_check_support_feature(uint32_t feature);

/**
 * @brief bt manager tws check both is support select feature
 *
 * This routine for tws check both is support select feature
 * @param feature Select feature.
 *
 * @return bool True Both are support, False at least one device not support.
 */
void bt_manager_tws_set_stream_type(uint8_t stream_type);

/**
 * @brief bt manager tws set codec
 *
 * This routine for tws set codec
 * @param codec Set codec.
 *
 * @return None.
 */
void bt_manager_tws_set_codec(uint8_t codec);

/**
 * @brief bt manager disconnect all device and enter pair mode
 *
 * This routine disconnect all device and enter pair mode
 *
 * @return None.
 */
void bt_manager_tws_disconnect_and_wait_pair(void);

/**
 * @brief bt manager notify tws channle mode switch
 *
 * This routine notify tws channle mode switch
 *
 * @return None.
 */
void bt_manager_tws_channel_mode_switch(void);

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @brief bt manager map(Message access profile) as client connect server
 *
 * This routine map as client connect server
 *
 * @param bd Server address.
 * @param path Map access path.
 * @param cb Map callback function. 
 *
 * @return uint8_t 0 Failed, other app_id for later used.
 */
uint8_t btmgr_map_client_connect(bd_address_t *bd, char *path, struct btmgr_map_cb *cb);

/**
 * @brief bt manager map set folder
 *
 * This routine for map set folder
 *
 * @param app_id Map app id return by connect.
 * @param path Folder path.
 * @param flags Reserve for future.
 *
 * @return uint8_t 0 Success, other Failed.
 */
uint8_t btmgr_map_client_set_folder(uint8_t app_id, char *path, uint8_t flags);

/**
 * @brief bt manager map get folder listing
 *
 * This routine for map get folder listing
 *
 * @param app_id Map app id return by connect.
 *
 * @return int 0 Success, other Failed.
 */
int btmgr_map_get_folder_listing(uint8_t app_id);

/**
 * @brief bt manager map get message listing
 *
 * This routine for map get message listing
 *
 * @param app_id Map app id return by connect.
 * @param max_cn Max messages listing.
 * @param mask
 *                MAP_PARAMETER_MASK_DATETIME 	=  (0x1 << 1)
 *                MAP_PARAMETER_MASK_SENDER_ADDRESS 	=  (0x1 << 3)
 *
 * @return int 0 Success, other Failed.
 */
int btmgr_map_get_messages_listing(uint8_t app_id, uint16_t max_cn, uint32_t mask);

/**
 * @brief bt manager map abort get message
 *
 * This routine for map abort get message
 *
 * @param app_id Map app id return by connect.
 *
 * @return int 0 Success, other Failed.
 */
int btmgr_map_abort_get(uint8_t app_id);

/**
 * @brief bt manager map disconnect
 *
 * This routine for map disconnect
 *
 * @param app_id Map app id return by connect.
 *
 * @return int 0 Success, other Failed.
 */
int btmgr_map_client_disconnect(uint8_t app_id);

/**
 * @cond INTERNAL_HIDDEN
 */

/**
 * @brief bt manager ble handle ancs_ams event
 *
 * This routine for ancs_ams event handle
 *
 * @param code for event data for param.
 *
 * @return N/A.
 */
void bt_manager_ble_ancs_ams_handle(uint8_t code, void* date);

/**
 * @brief bt manager reset btdev
 *
 * This routine for bt_manager reset btdev
 *
 * @param N/A.
 *
 * @return 0 or success , other for error.
 */
int bt_manager_reset_btdev(void);

#ifdef CONFIG_BT_A2DP_TRS
/**
 * @brief bt manager transfer get a2dp codec ID
 *
 * This routine for transfer get a2dp codec ID
 *
 * @return int Codec ID.
 */
int bt_manager_trs_a2dp_get_codecid(void);

/**
 * @brief bt manager transfer get a2dp sample rate
 *
 * This routine for transfer get a2dp sample rate
 *
 * @return int Sample rate.
 */
int bt_manager_trs_a2dp_get_sample_rate(void);

/**
 * @brief bt manager transfer start connect player
 *
 * This routine for transfer start connect player
 *
 * @param mac Player address.
 *
 * @return None.
 */
void bt_manager_trs_start_connect(uint8_t *mac);

/**
 * @brief bt manager transfer enable stream
 *
 * This routine for transfer enable stream
 *
 * @param enable True enable, False Disable.
 *
 * @return int 0 Success, other Failed.
 */
int bt_manager_trs_stream_enable(bool enable);

/**
 * @brief bt manager transfer check stream is enable
 *
 * This routine for transfer check stream is enable
 *
 * @return bool True Stream is enable, False Stream disable.
 */
bool bt_manager_trs_stream_is_enable(void);

/**
 * @brief bt manager transfer set stream
 *
 * This routine for transfer set stream
 *
 * @param stream Stream to set.
 *
 * @return int 0 Success, other Failed.
 */
int bt_manager_trs_set_stream(io_stream_t stream);

/**
 * @brief bt manager transfer get play stream
 *
 * This routine for transfer get play stream
 *
 * @return io_stream_t The play stream.
 */
io_stream_t bt_manager_trs_get_stream(void);

/**
 * @brief bt manager transfer get connected device number
 *
 * This routine for transfer get play stream
 *
 * @return int The number of connected device.
 */
int bt_manager_trs_get_connected_dev_num(void);

/**
 * @brief bt manager transfer get paired device info
 *
 * This routine for transfer get paired device
 *
 * @return paired dev num.
 */
int8_t bt_manager_get_trs_dev_info(struct bt_trs_list_dev_t *device_list,uint8_t max);

/**
 * @brief bt manager transfer clear paired device info
 *
 * This routine for transfer clear paired device
 *
 * @param bt addr.
 *
 * @return null.
 */
int bt_manager_clear_trs_dev_info(bd_address_t *bd);

/**
 * @brief spp connect bt uuid
 *
 * This routine provides to connect spp channel
 *
 * @param uuid uuid of spp connect
 *
 * @return 0 excute successed , others failed
 */
uint8_t bt_manager_spp_connect_by_uuid(bd_address_t *bd, uint8_t *uuid);

#endif


#ifdef __cplusplus
	}
#endif

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @} end defgroup bt_manager_apis
 */
#endif

