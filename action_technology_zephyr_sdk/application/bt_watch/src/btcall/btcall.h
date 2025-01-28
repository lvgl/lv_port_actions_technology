/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt call
 */

#ifndef _BT_CALL_APP_H_
#define _BT_CALL_APP_H_

#ifdef CONFIG_SYS_LOG
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "btcall"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>
#include <app_manager.h>
#include <srv_manager.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <volume_manager.h>
#include <media_player.h>
#include <audio_system.h>
#include <thread_timer.h>

#include "app_defines.h"
#include "sys_manager.h"
#include "app_ui.h"

#ifdef CONFIG_BLUETOOTH
#include <bt_manager.h>
#include "btservice_api.h"
#endif
#ifdef CONFIG_LAUNCHER_APP
#include "../launcher/launcher_app.h"
#endif

enum {
	// bt call key message
	MSG_BT_CALL_VOLUP = MSG_BT_APP_MESSAGE_START, /* 208 */
	MSG_BT_CALL_VOLDOWN,
	MSG_BT_CALL_SWITCH_CALLOUT,
	MSG_BT_CALL_SWITCH_MICMUTE,
	MSG_BT_HOLD_CURR_ANSWER_ANOTHER, /* 212 */
	MSG_BT_HANGUP_ANOTHER,
	MSG_BT_HANGUP_CURR_ANSER_ANOTHER,
	MSG_BT_HANGUP_CALL,
	MSG_BT_ACCEPT_CALL,
	MSG_BT_REJECT_CALL,
};

struct btcall_app_t {
	media_player_t *player;
	uint8_t playing  : 1;
	uint8_t mic_mute : 1;
	uint8_t need_resume_play:1;
	uint8_t phonenum_played : 1;
	uint8_t sco_established : 1;
	uint8_t siri_mode : 1;
	uint8_t hfp_ongoing : 1;
	uint8_t upload_stream_outer : 1;
	uint8_t switch_sound_source : 1;
	uint8_t hfp_3way_status;
	io_stream_t bt_stream;
	io_stream_t upload_stream;
	struct thread_timer timeout_wait_esco;
};

void btcall_bt_event_proc(struct app_msg *msg);
void btcall_input_event_proc(struct app_msg *msg);
bool btcall_key_event_proc(uint32_t event);
void btcall_tts_event_proc(struct app_msg *msg);
bool bt_call_key_event_handle(int key_event, int event_stage);
void bt_call_start_play(void);
void bt_call_stop_play(void);

struct btcall_app_t *btcall_get_app(void);

int btcall_ring_start(uint8_t *phone_num, uint16_t phone_num_cnt);
void btcall_ring_stop(void);
int btcall_ring_manager_init(void);
int btcall_ring_manager_deinit(void);
void btcall_ring_check_next_play(void);

void btcall_view_init(void);
void btcall_view_deinit(void);
void btcall_accept_call(void);
void btcall_reject_call(void);
void btcall_handup_call(void);
void btcall_vol_adjust(bool is_up);
void btcall_switch_micmute(void);
void btcall_set_phone_num(const char *phone_num, bool in_3way_view);
void btcall_set_3way_phone_num(const char *phone_num);
void btcall_sync_state_to_view(bool siri_mode);
void btcall_set_incoming_view(void);
void btcall_set_outgoing_view(void);
void btcall_switch_voice_source(bool switch_to_phone);
void btcall_set_ongoing_start_time(void);
void btcall_exit_view(void);

void btcall_proc_start(void);
void btcall_proc_exit(void);
void btcall_hangupcur_answer_another(void);
void btcall_holdcur_answer_another(void);
void btcall_hangup_another(void);

void btcall_play_next_tts(void);

#endif  // _BT_CALL_APP_H_
