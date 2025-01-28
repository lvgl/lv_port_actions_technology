/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt service tws
 */

#ifndef _BTSRV_TWS_H_
#define _BTSRV_TWS_H_

typedef enum {
	TWS_STATE_INIT,
	TWS_STATE_WAIT_PAIR,
	TWS_STATE_CONNECTING,
	TWS_STATE_DISCONNECTING,
	TWS_STATE_DETECT_ROLE,
	TWS_STATE_CONNECTED,
	TWS_STATE_READY_PLAY,
	TWS_STATE_RESTART_PLAY,
} btsrv_tws_state_e;

void btsrv_tws_set_input_stream(io_stream_t stream, bool user_set);
void btsrv_tws_set_sco_input_stream(io_stream_t stream);
int btsrv_tws_process(struct app_msg *msg);
int btsrv_tws_get_state(void);
void btsrv_tws_get_pairing_connecting_state(bool *pairing, bool *connecting);
int btsrv_tws_set_bt_local_play(bool bt_play, bool local_play);
void btsrv_tws_updata_tws_mode(uint8_t tws_mode, uint8_t drc_mode);
void btsrv_tws_set_codec(uint8_t codec);
void btsrv_tws_dump_info(void);

int btsrv_tws_connect_to(bt_addr_t *addr, uint8_t try_times, uint8_t req_role);
void btsrv_tws_cancal_auto_connect(void);
int btsrv_tws_can_do_pair(void);
int btsrv_tws_is_in_connecting(void);

tws_runtime_observer_t *btsrv_tws_monitor_get_observer(void);
int bsrv_tws_send_user_command(uint8_t *data, int len);
int bsrv_tws_send_user_command_sync(uint8_t *data, int len);

int btsrv_tws_protocol_data_cb(struct bt_conn *conn, uint8_t *data, uint16_t len);

uint32_t btsrv_tws_get_bt_clock(void);

bool btsrv_tws_check_support_feature(uint32_t feature);
bool btsrv_tws_check_is_woodpecker(void);

void btsrv_tws_hfp_start_callout(uint8_t codec);
void btsrv_tws_hfp_stop_call();

#endif
