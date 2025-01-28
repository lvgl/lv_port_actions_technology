/*
 * Copyright (c) 2021 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief 
*/
#ifndef _AP_RECORD_PRIVATE_H
#define _AP_RECORD_PRIVATE_H


#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "ap_develop"
#include <mem_manager.h>
#include <app_manager.h>
#include <srv_manager.h>
#include <volume_manager.h>
#include <msg_manager.h>
#include <thread_timer.h>
#include <media_player.h>
#include <audio_system.h>
#include <audio_policy.h>
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>

#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif
#include <thread_timer.h>


#include "app_defines.h"
#include "sys_manager.h"
#include "app_ui.h"

#ifdef CONFIG_PLAYTTS
#include "tts_manager.h"
#endif
#include <user_comm/ap_record.h>

enum
{
	MSG_USOUND_EVENT = MSG_APP_MESSAGE_START,
	MSG_AP_RECORD_EVENT,
};

enum
{
	MSG_USOUND_PLAY_PAUSE_RESUME = 1,
	MSG_AP_RECORD_START,
	MSG_AP_RECORD_STOP,
};

enum
{
	MSG_USOUND_STREAM_START = 1,
    MSG_AP_RECORD_UPDATE_STATUS,
};

struct record_app_t {
	media_player_t *player;
	u32_t playing : 1;
	u32_t media_opened : 1;
	u32_t sppble_connect : 1;
	u32_t sppble_stream_opened : 1;
	io_stream_t record_upload_stream;
	record_stream_init_param user_param;
};

io_stream_t record_upload_stream_create(void *param);
struct record_app_t *record_get_app(void);
void record_input_event_proc(struct app_msg *msg);
void record_tts_event_proc(struct app_msg *msg);
void record_event_proc(struct app_msg *msg);


#endif  // _AP_RECORD_PRIVATE_H

