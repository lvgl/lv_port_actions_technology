/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Media player interface
 */

#ifndef __PCM_PLAYER_H__
#define __PCM_PLAYER_H__
#include <audio_track.h>
#include <ringbuff_stream.h>
#include <stream.h>

/** media player structure */
typedef struct {
	struct audio_track_t *audio_track;
	struct acts_ringbuf *pcm_ringbuffer;
	io_stream_t audio_stream;
} pcm_player_t;

pcm_player_t *pcm_player_open(char *record_buffer, int record_buffer_size);
int pcm_player_set_volume(pcm_player_t *player, uint32_t volume);
int pcm_player_start(pcm_player_t *player);
int pcm_player_stop(pcm_player_t *player);
int pcm_player_close(pcm_player_t *player);


#endif  /* __PCM_PLAYER_H__ */
