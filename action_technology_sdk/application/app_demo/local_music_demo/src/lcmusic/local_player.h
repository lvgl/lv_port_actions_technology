/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief local player.h
 */


#ifndef _MPLAYER_H
#define _MPLAYER_H

#ifdef CONFIG_SYS_LOG
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "mplayer"
#ifdef SYS_LOG_LEVEL
#undef SYS_LOG_LEVEL
#endif
#define SYS_LOG_LEVEL 4		/* 4, debug */
#endif

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>
#include <file_stream.h>
#include <loop_fstream.h>
#include <app_manager.h>
#include <srv_manager.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <volume_manager.h>
#include <media_player.h>
#include <audio_system.h>
#include <audio_policy.h>
#include <thread_timer.h>
#include "app_defines.h"
#include "sys_manager.h"
#include <fs/fs.h>


/* used by file_selector of lcmusic, record, alarm, loop_player app */
#define MAX_DIR_LEVEL		9

struct local_player_t {
	media_player_t *player;
	io_stream_t file_stream;
};

struct lcplay_param {
	const char *url;
	media_breakpoint_info_t bp;
	int seek_time;
	u8_t play_mode;	/* 0-normal, 1-loopplay, others-reserved */
#ifdef CONFIG_SOUNDBAR_SAMPLE_RATE_FILTER
	int min_sample_rate_khz; /* <=0--disable, others--minius sample rate in kHz */
#endif

	void (*play_event_callback)(u32_t event, void *data, u32_t len, void *user_data);
};

struct local_player_t *mplayer_start_play(struct lcplay_param *lcparam);

void mplayer_stop_play(struct local_player_t *lcplayer);

int mplayer_pause(struct local_player_t *lcplayer);

int mplayer_resume(struct local_player_t *lcplayer);

int mplayer_seek(struct local_player_t *lcplayer, int seek_time, media_breakpoint_info_t *bp_info);

int mplayer_seek_relative(struct local_player_t *lcplayer, int seek_time, media_breakpoint_info_t *bp_info);

u8_t get_music_file_type(const char *url);

int mplayer_update_breakpoint(struct local_player_t *lcplayer, media_breakpoint_info_t *bp_info);

int mplayer_update_media_info(struct local_player_t *lcplayer, media_info_t *info);

#endif  /* _MPLAYER_H */
