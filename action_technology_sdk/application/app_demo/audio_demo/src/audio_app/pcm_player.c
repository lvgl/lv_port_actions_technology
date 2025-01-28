/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file key tone
 */
#include <os_common_api.h>
#include <audio_system.h>
#include <media_player.h>
#include <buffer_stream.h>
#include <file_stream.h>
#include <app_manager.h>
#include <mem_manager.h>
#include <tts_manager.h>
#include <audio_track.h>
#include <ringbuff_stream.h>
#include <pcm_player.h>
#include <soc_dsp.h>

pcm_player_t *pcm_player_open(char *play_buffer, int play_buffer_size)
{
	pcm_player_t *handle = mem_malloc(sizeof(pcm_player_t));

	if (!handle) {
		return NULL;
	}
	
	dsp_powergate_enable();
	
	struct acts_ringbuf *pcm_buffer = acts_ringbuf_init_ext(play_buffer, play_buffer_size);

	handle->audio_track = audio_track_create(AUDIO_STREAM_TTS, 16, AUDIO_FORMAT_PCM_16_BIT, AUDIO_MODE_MONO,
									 pcm_buffer, NULL, NULL);

	if (!handle->audio_track) {
		goto err_exit;
	}

	handle->pcm_ringbuffer = pcm_buffer;

	handle->audio_stream = audio_track_get_stream(handle->audio_track);

	return handle;

err_exit:
	if (handle->audio_track)
		audio_track_destory(handle->audio_track);

	if (handle)
		mem_free(handle);

	return NULL;
}

int pcm_player_set_volume(pcm_player_t *player, uint32_t volume)
{
	int ret = 0;

	if (player->audio_track) {
		ret = audio_track_set_volume(player->audio_track, volume);
	}

	return ret;
}

int pcm_player_start(pcm_player_t *player)
{
	int ret = 0;

	if (player->audio_track) {
		ret = audio_track_start(player->audio_track);
	}
	return ret;
}

int pcm_player_stop(pcm_player_t *player)
{
	int ret = 0;

	if (player->audio_track) {
		ret = audio_track_flush(player->audio_track);
		ret |= audio_track_stop(player->audio_track);
	}
	return ret;
}

int pcm_player_close(pcm_player_t *player)
{
	int ret = 0;

	if (player->audio_track) {
		ret = audio_track_destory(player->audio_track);
	}

	if (player->pcm_ringbuffer) {
		acts_ringbuf_destroy_ext(player->pcm_ringbuffer);
	}

	mem_free(player);

	return ret;
}

