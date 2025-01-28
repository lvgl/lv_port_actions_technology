/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt service inner head file
 */

#ifndef _BTSRV_INNER_H_
#define _BTSRV_INNER_H_

#include <btservice_api.h>
#include "../btsrv_config.h"
#include <acts_bluetooth/host_interface.h>
#include <hci_core.h>

#ifdef CONFIG_SUPPORT_TWS
#include <btsrv_tws.h>
#endif

/* Two phone device, one tws device or one trs device */
#define BTSRV_SAVE_AUTOCONN_NUM					(3)

enum {
	LINK_ADJUST_IDLE,
	LINK_ADJUST_RUNNING,
	LINK_ADJUST_STOP,
	LINK_ADJUST_SINK_BLOCK,
	LINK_ADJUST_SINK_CLEAR_BLOCK,
};

typedef enum {
	/** first time reconnect */
	RECONN_STARTUP,
	/** reconnect when timeout */
	RECONN_TIMEOUT,
} btstack_reconnect_mode_e;

struct rdm_sniff_info {
	uint8_t sniff_mode:4;
	uint8_t sniff_entering:1;
	uint8_t sniff_exiting:1;
	uint16_t sniff_interval;
	uint16_t conn_rxtx_cnt;
	uint32_t idle_start_time;
	uint32_t sniff_entering_time;
};

typedef void (*rdm_connected_dev_cb)(struct bt_conn *base_conn, uint8_t tws_dev, void *cb_param);

#define MEDIA_RTP_HEAD_LEN		12
#define AVDTP_SBC_HEADER_LEN	13
#define AVDTP_AAC_HEADER_LEN	16

enum {
	MEDIA_RTP_HEAD	   = 0x80,
	MEDIA_RTP_TYPE_SBC = 0x60,
	MEDIA_RTP_TYPE_AAC = 0x80,		/* 0x80 or 0x62 ?? */
};

/** avrcp device state */
typedef enum {
	BTSRV_AVRCP_PLAYSTATUS_STOPPED,
	BTSRV_AVRCP_PLAYSTATUS_PAUSEED,
	BTSRV_AVRCP_PLAYSTATUS_PLAYING,
} btsrv_avrcp_state_e;

enum {
	BTSRV_LINK_BASE_CONNECTED,
	BTSRV_LINK_BASE_CONNECTED_FAILED,
	BTSRV_LINK_BASE_CONNECTED_TIMEOUT,
	BTSRV_LINK_BASE_DISCONNECTED,
	BTSRV_LINK_BASE_GET_NAME,
	BTSRV_LINK_HFP_CONNECTED,
	BTSRV_LINK_HFP_DISCONNECTED,
	BTSRV_LINK_A2DP_CONNECTED,
	BTSRV_LINK_A2DP_DISCONNECTED,
	BTSRV_LINK_AVRCP_CONNECTED,
	BTSRV_LINK_AVRCP_DISCONNECTED,
	BTSRV_LINK_SPP_CONNECTED,
	BTSRV_LINK_SPP_DISCONNECTED,
	BTSRV_LINK_PBAP_CONNECTED,
	BTSRV_LINK_PBAP_DISCONNECTED,
	BTSRV_LINK_HID_CONNECTED,
	BTSRV_LINK_HID_DISCONNECTED,
	BTSRV_LINK_MAP_CONNECTED,
	BTSRV_LINK_MAP_DISCONNECTED,
};

enum {
	BTSRV_WAKE_LOCK_WAIT_CONNECT = (0x1 << 0),
	BTSRV_WAKE_LOCK_PAIR_MODE = (0x1 << 1),
	BTSRV_WAKE_LOCK_SEARCH = (0x1 << 2),
	BTSRV_WAKE_LOCK_RECONNECT = (0x1 << 3),

	BTSRV_WAKE_LOCK_CONN_BUSY = (0x1 << 8),
	BTSRV_WAKE_LOCK_CALL_BUSY = (0x1 << 9),
	BTSRV_WAKE_LOCK_ACTIVE_MODE = (0x1 << 10),
	BTSRV_WAKE_LOCK_BTSRV_RUNING = (0x1 << 11),
	BTSRV_WAKE_LOCK_TWS_IRQ_BUSY = (0x1 << 12),
};

struct btsrv_info_t {
	uint8_t running:1;
	uint8_t allow_sco_connect:1;
	uint16_t bt_wake_lock;
	btsrv_callback callback;
	uint8_t device_name[CONFIG_MAX_BT_NAME_LEN + 1];
	uint8_t device_addr[6];
	struct btsrv_config_info cfg;
	struct thread_timer wait_disconnect_timer;
	struct thread_timer wake_lock_timer;
};

struct btsrv_addr_name {
	bd_address_t mac;
	uint8_t name[CONFIG_MAX_BT_NAME_LEN + 1];
};

struct btsrv_mode_change_param {
	struct bt_conn *conn;
	uint8_t mode;
	uint16_t interval;
};

#ifdef CONFIG_BT_A2DP_TRS
struct bt_paired_list_dev_t {
	bd_address_t addr;
    uint8_t name[CONFIG_MAX_BT_NAME_LEN + 1];
    uint8_t valid;
};
#endif


extern struct btsrv_info_t *btsrv_info;
#define btsrv_max_conn_num()                    btsrv_info->cfg.max_conn_num
#define btsrv_max_phone_num()                   btsrv_info->cfg.max_phone_num
#define btsrv_is_pts_test()                     btsrv_info->cfg.pts_test_mode
#define btsrv_is_preemption_mode()              btsrv_info->cfg.double_preemption_mode
#define btsrv_volume_sync_delay_ms()            btsrv_info->cfg.volume_sync_delay_ms
#define btsrv_get_tws_version()                 btsrv_info->cfg.tws_version
#define btsrv_get_tws_feature()                 btsrv_info->cfg.tws_feature
#define btsrv_allow_sco_connect()               btsrv_info->allow_sco_connect

struct btsrv_info_t *btsrv_adapter_init(btsrv_callback cb);
int btsrv_adapter_process(struct app_msg *msg);
int btsrv_adapter_callback(btsrv_event_e event, void *param);
void btsrv_adapter_run(void);
int btsrv_adapter_stop(void);
void btsrv_adapter_allow_sco_connect(bool allow);
int btsrv_adapter_set_config_info(void *param);
void btsrv_adapter_set_clear_wake_lock(uint16_t wake_lock, uint8_t set);
int btsrv_adapter_get_wake_lock(void);
void btsrv_adapter_srv_get_wake_lock(void);
int btsrv_adapter_start_discover(struct btsrv_discover_param *param);
int btsrv_adapter_stop_discover(void);
int btsrv_adapter_remote_name_request(bd_address_t *addr, bt_br_remote_name_cb_t cb);
int btsrv_adapter_connect(bd_address_t *addr, const struct bt_br_conn_param *param);
int btsrv_adapter_check_cancal_connect(bd_address_t *addr);
int btsrv_adapter_disconnect(struct bt_conn *conn);
int btsrv_adapter_set_discoverable(bool enable);
int btsrv_adapter_set_connnectable(bool enable);
void btsrv_adapter_dump_info(void);
void btsrv_dump_info_proc(void);

int btsrv_a2dp_process(struct app_msg *msg);
int btsrv_a2dp_init(struct btsrv_a2dp_start_param *param);
int btsrv_a2dp_deinit(void);
int btsrv_a2dp_disconnect(struct bt_conn *conn);
int btsrv_a2dp_connect(struct bt_conn *conn, uint8_t role);
void btsrv_a2dp_halt_aac_endpoint(bool halt);

int btsrv_a2dp_media_state_change(struct bt_conn *conn, uint8_t state);
int btsrv_a2dp_media_parser_frame_info(uint8_t codec_id, uint8_t *data, uint32_t data_len, uint16_t *frame_cnt, uint16_t *frame_len);
uint32_t btsrv_a2dp_media_cal_frame_time_us(uint8_t codec_id, uint8_t *data);
uint16_t btsrv_a2dp_media_cal_frame_samples(uint8_t codec_id, uint8_t *data);
uint8_t btsrv_a2dp_media_get_samples_rate(uint8_t codec_id, uint8_t *data);
uint8_t *btsrv_a2dp_media_get_zero_frame(uint8_t codec_id, uint16_t *len, uint8_t sample_rate);
int btsrv_a2dp_trs_conn_get(void);

struct btsrv_rdm_avrcp_pass_info {
	uint8_t pass_state;
	uint8_t op_id;
	uint32_t op_time;
};

int btsrv_avrcp_process(struct app_msg *msg);
int btsrv_avrcp_init(btsrv_avrcp_callback_t *cb);
int btsrv_avrcp_deinit(void);
int btsrv_avrcp_disconnect(struct bt_conn *conn);
int btsrv_avrcp_connect(struct bt_conn *conn);
bool btsrv_avrcp_is_support_get_playback_pos(void);

typedef enum {
	BTSRV_SCO_STATE_INIT,
	BTSRV_SCO_STATE_PHONE,
	BTSRV_SCO_STATE_HFP,
	BTSRV_SCO_STATE_DISCONNECT,
} btsrv_sco_state;

typedef enum {
	BTSRV_HFP_ROLE_HF =0,
	BTSRV_HFP_ROLE_AG,
} btsrv_hfp_role;

int btsrv_hfp_process(struct app_msg *msg);
int btsrv_hfp_init(btsrv_hfp_callback cb);
int btsrv_hfp_deinit(void);
int btsrv_hfp_disconnect(struct bt_conn *conn);
int btsrv_hfp_connect(struct bt_conn *conn);
int btsrv_hfp_rejected(struct bt_conn *conn);
int btsrv_hfp_set_status(struct bt_conn *conn, int state);
uint8_t btsrv_hfp_get_codec_id(struct bt_conn *conn);
int btsrv_hfp_get_call_state(uint8_t active_call, uint8_t *call_state);
int btsrv_sco_process(struct app_msg *msg);
struct bt_conn *btsrv_sco_get_conn(void);
void btsrv_sco_disconnect(struct bt_conn *sco_conn);

int btsrv_hfp_ag_process(struct app_msg *msg);
int btsrv_hfp_ag_connect(struct bt_conn *conn);
int btsrv_hfp_ag_set_status(struct bt_conn *conn, int state);

int btsrv_spp_send_data(uint8_t app_id, uint8_t *data, uint32_t len);
int btsrv_spp_process(struct app_msg *msg);

int btsrv_pbap_process(struct app_msg *msg);
int btsrv_map_process(struct app_msg *msg);

void btsrv_hid_connect(struct bt_conn *conn);
int btsrv_hid_process(struct app_msg *msg);

bool btsrv_rdm_need_high_performance(void);
struct bt_conn *btsrv_rdm_find_conn_by_addr(bd_address_t *addr);
struct bt_conn *btsrv_rdm_find_conn_by_hdl(uint16_t hdl);
int btsrv_rdm_get_connected_dev(rdm_connected_dev_cb cb, void *cb_param);
int btsrv_rdm_get_dev_state(struct bt_dev_rdm_state *state);
int btsrv_rdm_get_connected_dev_cnt_by_type(int type);
int btsrv_rdm_get_autoconn_dev(struct autoconn_info *info, int max_dev);
int btsrv_rdm_base_disconnected(struct bt_conn *base_conn);
int btsrv_rdm_add_dev(struct bt_conn *base_conn);
int btsrv_rdm_remove_dev(uint8_t *mac);
void btsrv_rdm_set_security_changed(struct bt_conn *base_conn);
bool btsrv_rdm_is_security_changed(struct bt_conn *base_conn);
bool btsrv_rdm_is_connected(struct bt_conn *base_conn);
bool btsrv_rdm_is_a2dp_connected(struct bt_conn *base_conn);
bool btsrv_rdm_is_avrcp_connected(struct bt_conn *base_conn);
bool btsrv_rdm_is_hfp_connected(struct bt_conn *base_conn);
bool btsrv_rdm_is_spp_connected(struct bt_conn *base_conn);
bool btsrv_rdm_is_hid_connected(struct bt_conn *base_conn);
int btsrv_rdm_set_a2dp_connected(struct bt_conn *base_conn, bool connected);
int btsrv_rdm_a2dp_actived_switch_lock(struct bt_conn *base_conn, uint8_t lock);
int btsrv_rdm_a2dp_actived(struct bt_conn *base_conn, uint8_t actived);
struct bt_conn *btsrv_rdm_a2dp_get_actived(void);
struct bt_conn *btsrv_rdm_avrcp_get_actived(void);
struct bt_conn *btsrv_rdm_a2dp_get_second_dev(void);
int btsrv_rdm_is_actived_a2dp_stream_open(void);
int btsrv_rdm_is_a2dp_stream_open(struct bt_conn *base_conn);
int btsrv_rdm_a2dp_set_codec_info(struct bt_conn *base_conn, uint8_t format, uint8_t sample_rate, uint8_t cp_type);
int btsrv_rdm_a2dp_get_codec_info(struct bt_conn *base_conn, uint8_t *format, uint8_t *sample_rate, uint8_t *cp_type);
int btsrv_rdm_a2dp_set_bitpool(struct bt_conn *base_conn, uint8_t bitpool);
uint8_t btsrv_rdm_a2dp_get_bitpool(struct bt_conn *base_conn);
int btsrv_rdm_get_a2dp_start_time(struct bt_conn *base_conn, uint32_t *start_time);
void btsrv_rdm_get_a2dp_acitve_mac(bd_address_t *addr);
int btsrv_rdm_set_a2dp_pending_ahead_start(struct bt_conn *base_conn, uint8_t start);
uint8_t btsrv_rdm_get_a2dp_pending_ahead_start(struct bt_conn *base_conn);
int btsrv_rdm_set_avrcp_connected(struct bt_conn *base_conn, bool connected);
int btsrv_rdm_avrcp_update_play_status(struct bt_conn *base_conn, uint8_t status);
struct bt_conn *btsrv_rdm_acl_get_connected_dev(void);
struct bt_conn *btsrv_rdm_avrcp_get_connected_dev(void);
struct bt_conn *btsrv_rdm_a2dp_get_connected_dev(void);
void btsrv_rdm_avrcp_set_getting_pos_time(struct bt_conn *base_conn, uint8_t getting);
int btsrv_rdm_avrcp_get_getting_pos_time(struct bt_conn *base_conn);
void *btsrv_rdm_avrcp_get_pass_info(struct bt_conn *base_conn);
int btsrv_rdm_set_hfp_connected(struct bt_conn *base_conn, bool connected);
int btsrv_rdm_set_hfp_role(struct bt_conn *base_conn, uint8_t role);
int btsrv_rdm_get_hfp_role(struct bt_conn *base_conn);
int btsrv_rdm_hfp_actived(struct bt_conn *base_conn, uint8_t actived, uint8_t force);
struct bt_conn *btsrv_rdm_hfp_get_actived_sco(void);
bool btsrv_rdm_hfp_in_call_state(struct bt_conn *base_conn);
int btsrv_rdm_hfp_set_codec_info(struct bt_conn *base_conn, uint8_t format, uint8_t sample_rate);
int btsrv_rdm_hfp_set_state(struct bt_conn *base_conn, uint8_t state);
int btsrv_rdm_hfp_get_state(struct bt_conn *base_conn);
int btsrv_rdm_hfp_set_sco_state(struct bt_conn *base_conn, uint8_t state);
int btsrv_rdm_hfp_get_sco_state(struct bt_conn *base_conn);
int btsrv_rdm_hfp_set_call_state(struct bt_conn *base_conn, uint8_t active, uint8_t held, uint8_t in, uint8_t out);
int btsrv_rdm_hfp_get_call_state(struct bt_conn *base_conn, uint8_t *active, uint8_t *held, uint8_t *in, uint8_t *out);
int btsrv_rdm_hfp_get_codec_info(struct bt_conn *base_conn, uint8_t *format, uint8_t *sample_rate);
int btsrv_rdm_hfp_set_notify_phone_num_state(struct bt_conn *base_conn, uint8_t state);
int btsrv_rdm_hfp_get_notify_phone_num_state(struct bt_conn *base_conn);
struct bt_conn *btsrv_rdm_hfp_get_actived(void);
struct bt_conn *btsrv_rdm_hfp_get_second_dev(void);
void btsrv_rdm_get_hfp_acitve_mac(bd_address_t *addr);
void btsrv_rdm_set_hfp_service(struct bt_conn *base_conn, uint8_t service);
uint8_t btsrv_rdm_get_hfp_service(void);
struct bt_conn *btsrv_rdm_get_base_conn_by_sco(struct bt_conn *sco_conn);
int btsrv_rdm_sco_connected(struct bt_conn *base_conn, struct bt_conn *sco_conn);
int btsrv_rdm_sco_disconnected(struct bt_conn *sco_conn);
bool btsrv_rdm_is_sco_connected(struct bt_conn *acl_conn);
void btsrv_rdm_set_sco_connected_reject(struct bt_conn *acl_conn, uint8_t reject);
bool btsrv_rdm_is_sco_connected_reject(struct bt_conn *acl_conn);
int btsrv_rdm_set_spp_connected(struct bt_conn *base_conn, bool connected);
int btsrv_rdm_set_pbap_connected(struct bt_conn *base_conn, bool connected);
int btsrv_rdm_set_map_connected(struct bt_conn *base_conn, bool connected);
int btsrv_rdm_set_hid_connected(struct bt_conn *base_conn, bool connected);
struct bt_conn *btsrv_rdm_hid_get_actived(void);
int btsrv_rdm_set_tws_role(struct bt_conn *base_conn, uint8_t role);
struct bt_conn *btsrv_rdm_get_tws_by_role(uint8_t role);
int btsrv_rdm_get_conn_role(struct bt_conn *base_conn);
int btsrv_rdm_get_dev_role(void);
uint16_t btsrv_rdm_get_actived_phone_hdl(void);
int btsrv_rdm_set_controler_role(struct bt_conn *base_conn, uint8_t role);
int btsrv_rdm_get_controler_role(struct bt_conn *base_conn, uint8_t *role);
int btsrv_rdm_set_link_time(struct bt_conn *base_conn, uint16_t link_time);
uint16_t btsrv_rdm_get_link_time(struct bt_conn *base_conn);
void btsrv_rdm_set_dev_name(struct bt_conn *base_conn, uint8_t *name);
uint8_t *btsrv_rdm_get_dev_name(struct bt_conn *base_conn);
void btsrv_rdm_set_wait_to_diconnect(struct bt_conn *base_conn, bool set);
bool btsrv_rdm_is_wait_to_diconnect(struct bt_conn *base_conn);
void btsrv_rdm_set_switch_sbc_state(struct bt_conn *base_conn, uint8_t state);
uint8_t btsrv_rdm_get_switch_sbc_state(struct bt_conn *base_conn);
void *btsrv_rdm_get_sniff_info(struct bt_conn *base_conn);
int btsrv_rdm_get_active_dev_rssi(void);
int btsrv_rdm_init(void);
void btsrv_rdm_deinit(void);
void btsrv_rdm_dump_info(void);

struct thread_timer * btsrv_rdm_get_sco_disconnect_timer(struct bt_conn *base_conn);
int btsrv_rdm_get_sco_creat_time(struct bt_conn *base_conn, uint32_t *creat_time);

void btsrv_autoconn_info_update(void);
int btsrv_connect_get_auto_reconnect_info(struct autoconn_info *info, uint8_t max_cnt);
void btsrv_connect_set_auto_reconnect_info(struct autoconn_info *info, uint8_t max_cnt);
void btsrv_connect_set_phone_controler_role(bd_address_t *bd, uint8_t role);
int btsrv_connect_process(struct app_msg *msg);
bool btsrv_autoconn_is_reconnecting(void);
bool btsrv_autoconn_is_runing(void);
void btsrv_connect_tws_role_confirm(void);
int btsrv_connect_init(void);
void btsrv_connect_deinit(void);
void btsrv_connect_dump_info(void);

void btsrv_scan_set_param(struct bt_scan_parameter *param, bool enhance_param);
void btsrv_scan_set_user_discoverable(bool enable, bool immediate);
void btsrv_scan_set_user_connectable(bool enable, bool immediate);
void btsrv_inner_set_scan_enable(bool discoverable, bool connectable);
void btsrv_scan_update_mode(bool immediate);
uint8_t btsrv_scan_get_inquiry_mode(void);
int btsrv_scan_init(void);
void btsrv_scan_deinit(void);
void btsrv_scan_dump_info(void);

int btsrv_link_adjust_set_tws_state(uint8_t adjust_state, uint16_t buff_size, uint16_t source_cache, uint16_t cache_sink);
int btsrv_link_adjust_tws_set_bt_play(bool bt_play);
int btsrv_link_adjust_init(void);
void btsrv_link_adjust_deinit(void);

/* btsrv_sniff.c */
void btsrv_sniff_update_idle_time(struct bt_conn *conn);
void btsrv_sniff_mode_change(void *param);
void btsrv_sniff_set_check_enable(bool enable);
bool btsrv_sniff_in_sniff_mode(struct bt_conn *conn);
void btsrv_sniff_init(void);
void btsrv_sniff_deinit(void);

/* btsrv_pnp.c */
int btsrv_pnp_info_search(struct bt_conn *conn);
int btsrv_pnp_info_process(struct app_msg *msg);

bd_address_t *GET_CONN_BT_ADDR(struct bt_conn *conn);
int btsrv_set_negative_prio(void);
void btsrv_revert_prio(int prio);
int btsrv_property_set(const char *key, char *value, int value_len);
int btsrv_property_get(const char *key, char *value, int value_len);

void ctrl_adjust_link_time(struct bt_conn *base_conn, int16_t adjust_val);

#define BTSTAK_READY 0

enum {
	MSG_BTSRV_BASE = MSG_APP_MESSAGE_START,
	MSG_BTSRV_CONNECT,
	MSG_BTSRV_A2DP,
	MSG_BTSRV_AVRCP,
	MSG_BTSRV_HFP,
	MSG_BTSRV_HFP_AG,
	MSG_BTSRV_SCO,
	MSG_BTSRV_SPP,
	MSG_BTSRV_PBAP,
	MSG_BTSRV_HID,
	MSG_BTSRV_TWS,
	MSG_BTSRV_MAP,
	MSG_BTSRV_PNP,
	MSG_BTSRV_MAX,
};

enum {
	MSG_BTSRV_SET_DEFAULT_SCAN_PARAM,
	MSG_BTSRV_SET_ENHANCE_SCAN_PARAM,
	MSG_BTSRV_SET_DISCOVERABLE,
	MSG_BTSRV_SET_CONNECTABLE,
	MSG_BTSRV_AUTO_RECONNECT,
	MSG_BTSRV_AUTO_RECONNECT_STOP,
	MSG_BTSRV_CONNECT_TO,
	MSG_BTSRV_DISCONNECT,
	MSG_BTSRV_READY,
	MSG_BTSRV_REQ_FLUSH_NVRAM,
	MSG_BTSRV_CONNECTED,
	MSG_BTSRV_CONNECTED_FAILED,
	MSG_BTSRV_SECURITY_CHANGED,
	MSG_BTSRV_ROLE_CHANGE,
	//MSG_BTSRV_MODE_CHANGE,		/* Avoid build bt_trans lib, put in end */
	MSG_BTSRV_DISCONNECTED,
	MSG_BTSRV_DISCONNECTED_REASON,
	MSG_BTSRV_GET_NAME_FINISH,
	MSG_BTSRV_CLEAR_LIST_CMD,
	MGS_BTSRV_CLEAR_AUTO_INFO,
	MSG_BTSRV_DISCONNECT_DEVICE,
	MSG_BTSRV_ALLOW_SCO_CONNECT,		/* Not used, but can't remove, avoid build bt_trans lib */

	MSG_BTSRV_A2DP_START,
	MSG_BTSRV_A2DP_STOP,
	MSG_BTSRV_A2DP_CHECK_STATE,
	MSG_BTSRV_A2DP_CONNECT_TO,
	MSG_BTSRV_A2DP_DISCONNECT,
	MSG_BTSRV_A2DP_CONNECTED,
	MSG_BTSRV_A2DP_DISCONNECTED,
	//MSG_BTSRV_A2DP_MEDIA_STATE_CB,		/* Avoid build bt_trans lib, put in end */
	//MSG_BTSRV_A2DP_SET_CODEC_CB,			/* Avoid build bt_trans lib, put in end */
	MSG_BTSRV_A2DP_MEDIA_STATE_OPEN,
	MSG_BTSRV_A2DP_MEDIA_STATE_START,
	MSG_BTSRV_A2DP_MEDIA_STATE_CLOSE,
	MSG_BTSRV_A2DP_MEDIA_STATE_SUSPEND,
	MSG_BTSRV_A2DP_ACTIVED_DEV_CHANGED,
	MSG_BTSRV_A2DP_SEND_DELAY_REPORT,
	MSG_BTSRV_A2DP_CHECK_SWITCH_SBC,

	MSG_BTSRV_AVRCP_START,
	MSG_BTSRV_AVRCP_STOP,
	MSG_BTSRV_AVRCP_CONNECT_TO,
	MSG_BTSRV_AVRCP_DISCONNECT,
	MSG_BTSRV_AVRCP_CONNECTED,
	MSG_BTSRV_AVRCP_DISCONNECTED,
	MSG_BTSRV_AVRCP_NOTIFY_CB,
	MSG_BTSRV_AVRCP_PASS_CTRL_CB,
	MSG_BTSRV_AVRCP_PLAYBACK_POS_CB,
	MSG_BTSRV_AVRCP_SEND_CMD,
	MSG_BTSRV_AVRCP_SYNC_VOLUME,
	MSG_BTSRV_AVRCP_GET_ID3_INFO,
	MSG_BTSRV_AVRCP_GET_PLAYBACK_POS,
	MSG_BTSRV_AVRCP_SET_ABSOLUTE_VOLUME,

	MSG_BTSRV_HFP_START,
	MSG_BTSRV_HFP_STOP,
	MSG_BTSRV_HFP_CONNECTED,
	MSG_BTSRV_HFP_DISCONNECTED,
	MSG_BTSRV_HFP_SET_STATE,
	MSG_BTSRV_HFP_VOLUME_CHANGE,
	MSG_BTSRV_HFP_PHONE_NUM,
	MSG_BTSRV_HFP_CODEC_INFO,
	MSG_BTSRV_HFP_SIRI_STATE,
	MSG_BTSRV_SCO_START,
	MSG_BTSRV_SCO_STOP,
	MSG_BTSRV_SCO_CONNECTED,
	MSG_BTSRV_SCO_DISCONNECTED,

	MSG_BTSRV_HFP_SWITCH_SOUND_SOURCE,
	MSG_BTSRV_HFP_HF_DIAL_NUM,
	MSG_BTSRV_HFP_HF_DIAL_LAST_NUM,
	MSG_BTSRV_HFP_HF_DIAL_MEMORY,
	MSG_BTSRV_HFP_HF_VOLUME_CONTROL,
	MSG_BTSRV_HFP_HF_ACCEPT_CALL,
	MSG_BTSRV_HFP_HF_BATTERY_REPORT,
	MSG_BTSRV_HFP_HF_REJECT_CALL,
	MSG_BTSRV_HFP_HF_HANGUP_CALL,
	MSG_BTSRV_HFP_HF_HANGUP_ANOTHER_CALL,
	MSG_BTSRV_HFP_HF_HOLDCUR_ANSWER_CALL,
	MSG_BTSRV_HFP_HF_HANGUPCUR_ANSWER_CALL,
	MSG_BTSRV_HFP_HF_VOICE_RECOGNITION_START,
	MSG_BTSRV_HFP_HF_VOICE_RECOGNITION_STOP,
	MSG_BTSRV_HFP_HF_VOICE_SEND_AT_COMMAND,
    MSG_BTSRV_HFP_HF_CCWA_PHONE_NUM,
	MSG_BTSRV_HFP_ACTIVED_DEV_CHANGED,
	MSG_BTSRV_HFP_GET_TIME,
	MSG_BTSRV_HFP_TIME_UPDATE,
    MSG_BTSRV_HFP_CLCC_INFO,

	MSG_BTSRV_HFP_AG_START,
	MSG_BTSRV_HFP_AG_STOP,
	MSG_BTSRV_HFP_AG_CONNECTED,
	MSG_BTSRV_HFP_AG_DISCONNECTED,
	MSG_BTSRV_HFP_AG_UPDATE_INDICATOR,
	MSG_BTSRV_HFP_AG_SEND_EVENT,

	MSG_BTSRV_SPP_START,
	MSG_BTSRV_SPP_STOP,
	MSG_BTSRV_SPP_REGISTER,
	MSG_BTSRV_SPP_CONNECT,
	MSG_BTSRV_SPP_DISCONNECT,
	MSG_BTSRV_SPP_CONNECT_FAILED,
	MSG_BTSRV_SPP_CONNECTED,
	MSG_BTSRV_SPP_DISCONNECTED,

	MSG_BTSRV_PBAP_CONNECT_FAILED,
	MSG_BTSRV_PBAP_CONNECTED,
	MSG_BTSRV_PBAP_DISCONNECTED,
	MSG_BTSRV_PBAP_CMD_OP,

	MSG_BTSRV_HID_START,
	MSG_BTSRV_HID_STOP,
	MSG_BTSRV_HID_CONNECTED,
	MSG_BTSRV_HID_DISCONNECTED,
	MSG_BTSRV_HID_REGISTER,
	MSG_BTSRV_HID_CONNECT,
	MSG_BTSRV_HID_DISCONNECT,
	MSG_BTSRV_HID_SEND_CTRL_DATA,
	MSG_BTSRV_HID_SEND_INTR_DATA,
	MSG_BTSRV_HID_SEND_RSP,
	MSG_BTSRV_HID_UNPLUG,
	MSG_BTSRV_HID_EVENT_CB,
	MSG_BTSRV_DID_REGISTER,

	MSG_BTSRV_TWS_INIT,
	MSG_BTSRV_TWS_DEINIT,
	MSG_BTSRV_TWS_VERSION_NEGOTIATE,
	MSG_BTSRV_TWS_ROLE_NEGOTIATE,
	MSG_BTSRV_TWS_NEGOTIATE_SET_ROLE,
	MSG_BTSRV_TWS_CONNECTED,
	MSG_BTSRV_TWS_DISCONNECTED,
	MSG_BTSRV_TWS_DISCONNECTED_ADDR,
	MSG_BTSRV_TWS_WAIT_PAIR,
	MSG_BTSRV_TWS_CANCEL_WAIT_PAIR,
	MSG_BTSRV_TWS_DISCOVERY_RESULT,
	MSG_BTSRV_TWS_DISCONNECT,
	MSG_BTSRV_TWS_RESTART,
	MSG_BTSRV_TWS_PROTOCOL_DATA,
	MSG_BTSRV_TWS_EVENT_SYNC,
	MSG_BTSRV_TWS_SCO_DATA,

	MSG_BTSRV_MAP_CONNECT,
	MSG_BTSRV_MAP_DISCONNECT,
	MSG_BTSRV_MAP_SET_FOLDER,
	MSG_BTSRV_MAP_GET_FOLDERLISTING,	
	MSG_BTSRV_MAP_GET_MESSAGESLISTING,		
	MSG_BTSRV_MAP_GET_MESSAGE,	
	MSG_BTSRV_MAP_ABORT_GET,	
	MSG_BTSRV_MAP_CONNECT_FAILED,
	MSG_BTSRV_MAP_CONNECTED,
	MSG_BTSRV_MAP_DISCONNECTED,

	MSG_BTSRV_MODE_CHANGE,				/* Avoid build bt_trans lib, put in end */
	MSG_BTSRV_A2DP_MEDIA_STATE_CB,		/* Avoid build bt_trans lib, put in end */
	MSG_BTSRV_A2DP_SET_CODEC_CB,		    /* Avoid build bt_trans lib, put in end */
	MSG_BTSRV_AVRCP_GET_PLAY_STATE,
	MSG_BTSRV_AVRCP_PLAY_STATE_CB,
	MSG_BTSRV_CLEAR_PAIRED_INFO,
	MSG_BTSRV_GET_CONNECTED_DEV_RSSI,/* Avoid build bt_trans lib, put in end */
	MSG_BTSRV_GET_ACTIVED_DEV_RSSI,  /* Avoid build bt_trans lib, put in end */
	MSG_BTSRV_PAIR_PASSKEY_DISPLAY,
	MSG_BTSRV_PAIRING_COMPLETE,
	MSG_BTSRV_PAIRING_FAIL,
	MSG_BTSRV_GET_WAKE_LOCK,		/* Just for get wake lock */
	MSG_BTSRV_HFP_MANUFACTURE_INFO,
	MSG_BTSRV_HFP_PHONE_NAME,
	MSG_BTSRV_HFP_SERVICE,
	MSG_BTSRV_BR_RESOLVE_ADDR,

	MSG_BTSRV_PNP_INFO_START,
	MSG_BTSRV_PNP_INFO_STOP,

	MSG_BTSRV_HFP_HPREC_BATTERY,
	MSG_BTSRV_HFP_HF_BATTERY_HPREC_REPORT,

	MSG_BTSRV_A2DP_DISABLE,
	MSG_BTSRV_A2DP_ENABLE,

	/** restore spp sdp service, only use to btreset */
	MSG_BTSRV_SPP_SDP_SERVICE_RESTORE,
};

static inline btsrv_callback _btsrv_get_msg_param_callback(struct app_msg *msg)
{
	return (btsrv_callback)msg->ptr;
}

static inline int _btsrv_get_msg_param_type(struct app_msg *msg)
{
	return msg->type;
}

static inline int _btsrv_get_msg_param_cmd(struct app_msg *msg)
{
	return msg->cmd;
}

static inline int _btsrv_get_msg_param_reserve(struct app_msg *msg)
{
	return msg->reserve;
}

static inline void *_btsrv_get_msg_param_ptr(struct app_msg *msg)
{
	return msg->ptr;
}

static inline int _btsrv_get_msg_param_value(struct app_msg *msg)
{
	return msg->value;
}

int btsrv_event_notify(int event_type, int cmd, void *param);
int btsrv_event_notify_value(int event_type, int cmd, int value);
int btsrv_event_notify_ext(int event_type, int cmd, void *param, uint8_t code);
int btsrv_event_notify_malloc(int event_type, int cmd, uint8_t *data, uint16_t len, uint8_t code);
#define btsrv_function_call            btsrv_event_notify
#define btsrv_function_call_ext        btsrv_event_notify_ext
#define btsrv_function_call_value      btsrv_event_notify_value
#define btsrv_function_call_malloc     btsrv_event_notify_malloc

typedef int (*btsrv_msg_process)(struct app_msg *msg);

void bt_service_set_bt_ready(void);
uint8_t bt_service_ready_status(void);
int btsrv_register_msg_processer(uint8_t msg_type, btsrv_msg_process processer);

int btsrv_storage_init(void);
int btsrv_storage_get_addr_linkkey(bd_address_t *addr, uint8_t *linkkey);
int btsrv_storage_get_linkkey(struct bt_linkkey_info *info, uint8_t cnt);
int btsrv_storage_update_linkkey(struct bt_linkkey_info *info, uint8_t cnt);
int btsrv_storage_write_ori_linkkey(bd_address_t *addr, uint8_t *link_key);
void btsrv_storage_clean_linkkey(bd_address_t *addr);

int btsrv_pts_send_hfp_cmd(char *cmd);
int btsrv_pts_hfp_active_connect_sco(void);
int btsrv_pts_avrcp_pass_through_cmd(uint8_t opid);
int btsrv_pts_avrcp_notify_volume_change(uint8_t volume);
int btsrv_pts_avrcp_reg_notify_volume_change(void);
int btsrv_pts_register_auth_cb(bool reg_auth);

#ifdef CONFIG_BT_A2DP_TRS
struct bt_conn *btsrv_rdm_trs_avrcp_get_actived(void);
struct bt_conn *btsrv_rdm_trs_a2dp_get_actived(void);
int btsrv_rdm_get_dev_trs_mode(void);
int btsrv_rdm_set_trs_mode(bd_address_t *addr, uint8_t trs_mode);
int btsrv_rdm_get_trs_mode(struct bt_conn *base_conn);
int btsrv_rdm_get_direction(bd_address_t *addr);

int btsrv_rdm_trs_a2dp_set_codec_info(struct bt_conn *base_conn, struct bt_a2dp_media_codec *codec);
int btsrv_rdm_trs_a2dp_set_mtu(struct bt_conn *base_conn, uint16_t mtu);
int btsrv_rdm_trs_a2dp_get_codec_info(struct bt_conn *base_conn, struct bt_a2dp_media_codec *codec, uint16_t *media_mtu);
int btsrv_rdm_trs_a2dp_stream_open(struct bt_conn *base_conn, bool open);
struct bt_conn *btsrv_rdm_trs_get_conn(void);
struct thread_timer * btsrv_rdm_get_trs_discover_timer(struct bt_conn *base_conn);

void btsrv_trs_a2dp_ready(struct bt_conn *conn);
void btsrv_trs_a2dp_close(struct bt_conn *conn);
void btsrv_trs_a2dp_start(struct bt_conn *conn);
void btsrv_trs_a2dp_suspend(struct bt_conn *conn);

bool btsrv_trs_a2dp_media_state_req_cb(struct bt_conn *conn, uint8_t state);
bool btsrv_trs_a2dp_seted_codec_cb(struct bt_conn *conn, struct bt_a2dp_media_codec *codec);
int btsrv_trs_a2dp_init(struct btsrv_a2dp_start_param *param);
int btsrv_trs_a2dp_deinit(void);
bool btsrv_trs_a2dp_process(struct app_msg *msg);

bool btsrv_trs_avrcp_ctrl_pass_ctrl_cb(struct bt_conn *conn, uint8_t op_id, uint8_t state);
int btsrv_trs_avrcp_init(btsrv_avrcp_callback_t *cb);
int btsrv_trs_avrcp_deinit(void);

uint8_t btsrv_get_trs_dev_record(struct bt_paired_list_dev_t *record, uint8_t max);
#endif

#endif
