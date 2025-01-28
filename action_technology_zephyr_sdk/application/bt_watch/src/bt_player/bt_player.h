/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt player.h
 */


#ifndef _BT_PLAYER_H_
#define _BT_PLAYER_H_

#include "app_ui.h"

enum {
	/* bt play key message */
	MSG_BT_PLAY_PAUSE = MSG_BT_APP_MESSAGE_START, /* 208 */
	MSG_BT_PLAY_RESUME,
	MSG_BT_PLAY_NEXT, /* 210 */
	MSG_BT_PLAY_PREVIOUS,
	MSG_BT_PLAY_VOLUP,
	MSG_BT_PLAY_VOLDOWN,
	MSG_BT_PLAY_VOLSYNC,
	MSG_BT_PLAY_SYNC_REMOTE_STATE,
	MSG_BT_PLAY_STOP_PLAYER,
};

typedef struct btmusic_callback {
	void (*set_play_info)(const char *song_name, const char *album, uint32_t total_time);
	void (*set_play_pos)(uint32_t cur_time);
	void (*set_volume)(int volume);
	void (*set_status)(int status);
} btmusic_callback_t;

void bt_player_bt_event_proc(struct app_msg *msg);

/* external control by view */
void btmusic_start(void);
void btmusic_stop(bool forced);
void btmusic_play_next(void);
void btmusic_play_prev(void);
void btmusic_vol_adjust(bool is_add);
void btmusic_vol_sync(int music_vol, bool forced);
bool btmusic_get_avrcp_play_state(void);
bool btmusic_get_a2dp_play_state(void);
bool btmusic_get_player_state(void);
void btmusic_check_remote_state(void);

void btmusic_set_play_callback(const btmusic_callback_t *callback);

#endif  /* _BT_PLAYER_H_ */
