/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief audio system.
*/
#include <os_common_api.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <audio_system.h>
#include <audio_record.h>
#include <audio_track.h>
#include <audio_device.h>
#include <audio_hal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <property_manager.h>

#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "audio system"

struct audio_system_t *audio_system;

int audio_system_set_output_sample_rate(int value)
{
	if (!audio_system)
		return -ESRCH;

	audio_system->output_sample_rate = value;

	return 0;
}

int audio_system_get_output_sample_rate(void)
{
	if (!audio_system)
		return -ESRCH;

	return audio_system->output_sample_rate;
}

int audio_system_get_output_framecount(void)
{
	if (!audio_system)
		return -ESRCH;

	return 0;
}

int audio_system_set_master_volume(int value)
{
	if (!audio_system)
		return -ESRCH;

	audio_system->master_volume = value;

	return audio_system_set_stream_volume(AUDIO_STREAM_DEFAULT, value);

}

int audio_system_get_master_volume(void)
{
	if (!audio_system)
		return -ESRCH;

	return audio_system->master_volume;
}

int audio_system_set_master_mute(int value)
{
	if (!audio_system)
		return -ESRCH;

	audio_system->master_muted = value;
	/**TODO set master mute */

	return 0;
}

int audio_system_get_master_mute(void)
{
	if (!audio_system)
		return -ESRCH;

	return audio_system->master_muted;
}

int audio_system_set_stream_mute(int stream_type, int mute)
{
	struct audio_track_t *audio_track = NULL;
	int ret = -ESRCH;

	if (!audio_system)
		return -ESRCH;

	for (int i = 0; i < MAX_AUDIO_TRACK_NUM; i++) {
		audio_track = audio_system->audio_track_pool[i];
		if (audio_track && audio_track->stream_type == stream_type) {
			ret = audio_track_set_mute(audio_track, mute);
		}
	}

	return ret;
}

int audio_system_get_stream_mute(int stream_type)
{
	struct audio_track_t *audio_track = NULL;
	int ret = -ESRCH;

	if (!audio_system)
		return -ESRCH;

	for (int i = 0; i < MAX_AUDIO_TRACK_NUM; i++) {
		audio_track = audio_system->audio_track_pool[i];
		if (audio_track && audio_track->stream_type == stream_type) {
			ret = audio_track_get_volume(audio_track);
			break;
		}
	}

	return (ret == 0) ? 1 : 0;
}

int audio_system_mute_microphone(int value)
{
	struct audio_record_t *audio_record = NULL;

	if (!audio_system)
		return -ESRCH;

	audio_system->microphone_muted = value;

	for (int i = 0; i < MAX_AUDIO_RECORD_NUM; i++) {
		audio_record = audio_system->audio_record_pool[i];
		if (audio_record) {
			audio_record->muted = audio_system->microphone_muted;
		}
	}

	return 0;
}

int audio_system_get_microphone_muted(void)
{
	if (!audio_system)
		return -ESRCH;

	return audio_system->microphone_muted ? 1 : 0;
}

int audio_system_register_track(struct audio_track_t *audio_track)
{
	if (!audio_system)
		return -ESRCH;

	for (int i = 0; i < MAX_AUDIO_TRACK_NUM; i++) {
		if (!audio_system->audio_track_pool[i]) {
			audio_system->audio_track_pool[i] = audio_track;
			audio_system->audio_track_num++;
			SYS_LOG_INF(" %p index %d\n", audio_track, i);
			break;
		}
	}

	return 0;
}

int audio_system_mutex_lock(void)
{
	if (!audio_system)
		return -ESRCH;

	return os_mutex_lock(&audio_system->audio_system_mutex, OS_FOREVER);
}

int audio_system_mutex_unlock(void)
{
	if (!audio_system)
		return -ESRCH;

	os_mutex_unlock(&audio_system->audio_system_mutex);
	return 0;
}

struct audio_track_t *audio_system_get_track(void)
{
	if (!audio_system)
		return NULL;

	for (int i = 0; i < MAX_AUDIO_TRACK_NUM; i++) {
		if (audio_system->audio_track_pool[i]) {
			return audio_system->audio_track_pool[i];
		}
	}

	return NULL;
}

int audio_system_unregister_track(struct audio_track_t *audio_track)
{
	int ret = -ENXIO;

	if (!audio_system)
		return -ESRCH;

	for (int i = 0; i < MAX_AUDIO_TRACK_NUM; i++) {
		if (audio_system->audio_track_pool[i] == audio_track) {
			audio_system->audio_track_pool[i] = NULL;
			audio_system->audio_track_num--;
			SYS_LOG_INF(" %p index %d\n", audio_track, i);
			ret = 0;
			break;
		}
	}

	for (int i = 0; i < MAX_AUDIO_TRACK_NUM; i++) {
		struct audio_track_t *tmp_track = NULL;

		tmp_track = audio_system->audio_track_pool[i];
		if (tmp_track)
			audio_system_set_stream_volume(tmp_track->stream_type, tmp_track->volume);
	}

	return 0;
}

int audio_system_register_record(struct audio_record_t *audio_record)
{
	int ret = -EAGAIN;

	if (!audio_system)
		return -ESRCH;

	for (int i = 0; i < MAX_AUDIO_RECORD_NUM; i++) {
		if (!audio_system->audio_record_pool[i]) {
			audio_system->audio_record_pool[i] = audio_record;
			audio_system->audio_record_num++;
			SYS_LOG_INF(" %p index %d\n", audio_record, i);
			ret = 0;
			break;
		}
	}

	return ret;
}

int audio_system_unregister_record(struct audio_record_t *audio_record)
{
	int ret = -ENXIO;

	if (!audio_system)
		return -ESRCH;

	for (int i = 0; i < MAX_AUDIO_RECORD_NUM; i++) {
		if (audio_system->audio_record_pool[i] == audio_record) {
			audio_system->audio_record_pool[i] = NULL;
			SYS_LOG_INF(" %p index %d\n", audio_record, i);
			audio_system->audio_record_num--;
			ret = 0;
			break;
		}
	}
	return ret;
}

static void _audio_system_get_volume_config(const char *name, uint8_t *volume)
{
#ifdef CONFIG_PROPERTY
	char temp[8];

	memset(temp, 0, sizeof(temp));

	int ret = property_get(name, temp, sizeof(temp));

	if (ret >= 0) {
		*volume = atoi(temp);
	} else {
	    *volume = DEFAULT_VOLUME;
	}

	SYS_LOG_INF(" %s  %d\n", name, *volume);
#endif

}

static int _audio_system_save_volume(const char *name, uint8_t volume)
{
	char temp[5];

	if (!audio_policy_check_save_volume_to_nvram()) {
		return 0;
	}

	snprintf(temp, sizeof(temp), "%d", volume);

#ifdef CONFIG_PROPERTY
	return property_set(name, temp, strlen(temp) + 1);
#else
	return 0;
#endif
}

int audio_system_set_stream_volume(int stream_type, int volume)
{
	struct audio_track_t *audio_track = NULL;
	int ret = 0;

	if (!audio_system)
		return -ESRCH;

	if (volume >= audio_policy_get_volume_level()) {
		volume = audio_policy_get_volume_level();
		ret = MAX_VOLUME_VALUE;
	}

	/**usound default used 16 level */
	if (stream_type == AUDIO_STREAM_USOUND) {
		if (volume >= 16) {
			volume = 16;
			ret = MAX_VOLUME_VALUE;
		}
	}

	if (volume <= 0) {
		ret = MIN_VOLUME_VALUE;
		volume = 0;
	}
	os_mutex_lock(&audio_system->audio_system_mutex, OS_FOREVER);
	for (int i = 0; i < MAX_AUDIO_TRACK_NUM; i++) {
		audio_track = audio_system->audio_track_pool[i];
		if (audio_track &&
					 (audio_track->stream_type == stream_type
						|| stream_type == AUDIO_STREAM_DEFAULT)) {
			audio_track_set_volume(audio_track, volume);
		}
	}
	os_mutex_unlock(&audio_system->audio_system_mutex);

	if (stream_type == AUDIO_STREAM_VOICE) {
	    audio_system->voice_volume = volume;
	    _audio_system_save_volume(CFG_BTCALL_VOLUME, volume);
	}

	if (stream_type == AUDIO_STREAM_MUSIC || stream_type == AUDIO_STREAM_DEFAULT) {
	    audio_system->music_volume = volume;
	    _audio_system_save_volume(CFG_BTPLAY_VOLUME, volume);
	}

	if (stream_type == AUDIO_STREAM_LINEIN || stream_type == AUDIO_STREAM_DEFAULT) {
	    audio_system->linein_volume = volume;
	    _audio_system_save_volume(CFG_LINEIN_VOLUME, volume);
	}

	if (stream_type == AUDIO_STREAM_USOUND || stream_type == AUDIO_STREAM_DEFAULT) {
	    audio_system->usound_volume = volume;
	    _audio_system_save_volume(CFG_USOUND_VOLUME, volume);
	}

	if (stream_type == AUDIO_STREAM_LOCAL_MUSIC || stream_type == AUDIO_STREAM_DEFAULT) {
	    audio_system->lcmusic_volume = volume;
	    _audio_system_save_volume(CFG_LOCALPLAY_VOLUME, volume);
	}

	if (stream_type == AUDIO_STREAM_FM || stream_type == AUDIO_STREAM_DEFAULT) {
	    audio_system->fm_volume = volume;
	    _audio_system_save_volume(CFG_FM_VOLUME, volume);
	}

	if (stream_type == AUDIO_STREAM_I2SRX_IN || stream_type == AUDIO_STREAM_DEFAULT) {
	    audio_system->i2srx_in_volume = volume;
	    _audio_system_save_volume(CFG_I2SRX_IN_VOLUME, volume);
	}

	if (stream_type == AUDIO_STREAM_MIC_IN || stream_type == AUDIO_STREAM_DEFAULT) {
	    audio_system->mic_in_volume = volume;
	    _audio_system_save_volume(CFG_MIC_IN_VOLUME, volume);
	}

	if (stream_type == AUDIO_STREAM_SPDIF_IN || stream_type == AUDIO_STREAM_DEFAULT) {
	    audio_system->spidf_in_volume = volume;
	    _audio_system_save_volume(CFG_SPDIF_IN_VOLUME, volume);
	}
	if (stream_type != AUDIO_STREAM_VOICE) {
	    audio_system->master_volume = volume;

	if (volume < audio_system->min_volume)
		    volume = audio_system->min_volume;
	    if(volume > audio_system->max_volume)
		    volume = audio_system->max_volume;

		if (!audio_policy_check_tts_fixed_volume()) {
			audio_system->tts_volume = volume;
			_audio_system_save_volume(CFG_TONE_VOLUME, volume);
		}
	}

	SYS_LOG_INF("volume: %d %d ret %d ok\n", volume, stream_type, ret);

	return ret;
}

int audio_system_get_current_volume(int stream_type)
{
	int volume = 0;
	struct audio_track_t *audio_track = NULL;

	if (!audio_system)
		return -ESRCH;

	volume = audio_system_get_stream_volume(stream_type);

	/**make sure tts have volume*/
	for (int i = 0; i < MAX_AUDIO_TRACK_NUM; i++) {
		audio_track = audio_system->audio_track_pool[i];
		if ((audio_track->stream_type == AUDIO_STREAM_TTS)
            || (audio_track->stream_type == AUDIO_STREAM_TIP)) {
			if (volume != audio_system->tts_volume) {
				volume = audio_system->tts_volume;
			}
		}
	}
	return volume;
}

int audio_system_get_current_pa_volume(int stream_type)
{
	int volume = 0;
	int pa_volume = 0;
	struct audio_track_t *audio_track = NULL;

	if (!audio_system)
		return -ESRCH;

	volume = audio_system_get_current_volume(stream_type);
	pa_volume = audio_policy_get_pa_volume(stream_type, volume);


	/**make sure tts have volume*/
	for (int i = 0; i < MAX_AUDIO_TRACK_NUM; i++) {
		audio_track = audio_system->audio_track_pool[i];
		if ((audio_track->stream_type == AUDIO_STREAM_TTS)
            || (audio_track->stream_type == AUDIO_STREAM_TIP)) {
			pa_volume = audio_policy_get_pa_volume(AUDIO_STREAM_TTS, volume);
		}
	}
	return pa_volume;
}

int audio_system_get_stream_volume(int stream_type)
{
	int volume = 0;

	if (!audio_system)
		return -ESRCH;

	switch (stream_type) {
	case AUDIO_STREAM_MUSIC:
		volume = audio_system->music_volume;
		break;
	case AUDIO_STREAM_TTS:
    case AUDIO_STREAM_TIP:
		volume = audio_system->tts_volume;
		break;
	case AUDIO_STREAM_VOICE:
		volume = audio_system->voice_volume;
		break;
	case AUDIO_STREAM_LINEIN:
	    volume = audio_system->linein_volume;
	break;
	case AUDIO_STREAM_USOUND:
	    volume = audio_system->usound_volume;
	break;
	case AUDIO_STREAM_LOCAL_MUSIC:
	    volume = audio_system->lcmusic_volume;
	break;
	case AUDIO_STREAM_FM:
	    volume = audio_system->fm_volume;
	break;
	case AUDIO_STREAM_I2SRX_IN:
		volume = audio_system->i2srx_in_volume;
		break;
	case AUDIO_STREAM_SPDIF_IN:
		volume = audio_system->spidf_in_volume;
		break;
	default:
		volume = audio_system->master_volume;
		break;
	}

	return volume;
}

int audio_system_set_stream_pa_volume(int stream_type, int volume)
{
	struct audio_track_t *audio_track = NULL;

	if (!audio_system)
		return -ESRCH;

	os_mutex_lock(&audio_system->audio_system_mutex, OS_FOREVER);
	for (int i = 0; i < MAX_AUDIO_TRACK_NUM; i++) {
		audio_track = audio_system->audio_track_pool[i];
		if (audio_track &&
					 (audio_track->stream_type == stream_type
						|| stream_type == AUDIO_STREAM_DEFAULT)) {
			audio_track_set_pa_volume(audio_track, volume);
		}
	}
	os_mutex_unlock(&audio_system->audio_system_mutex);

	return 0;
}

int audio_system_set_microphone_volume(int stream_type, int volume)
{
	struct audio_record_t *audio_record = NULL;
	int ret = 0;

	if (!audio_system)
		return -ESRCH;

	for (int i = 0; i < MAX_AUDIO_RECORD_NUM; i++) {
		audio_record = audio_system->audio_record_pool[i];
		if (audio_record) {
			audio_record_set_volume(audio_record, volume);
		}
	}

	SYS_LOG_INF("volume: %d  ret %d ok\n", volume, ret);

	return ret;
}

int audio_system_get_max_volume(void)
{
	return audio_system->max_volume;
}

int audio_system_get_min_volume(void)
{
	return audio_system->min_volume;
}

static struct audio_system_t global_audio_system;

int aduio_system_init(void)
{
	audio_system = &global_audio_system;

	memset(audio_system, 0, sizeof(struct audio_system_t));

	os_mutex_init(&audio_system->audio_system_mutex);

	_audio_system_get_volume_config(CFG_BTPLAY_VOLUME, &audio_system->music_volume);

	audio_system->master_volume = audio_system->music_volume;

	_audio_system_get_volume_config(CFG_TONE_VOLUME, &audio_system->tts_volume);

	_audio_system_get_volume_config(CFG_BTCALL_VOLUME, &audio_system->voice_volume);

	_audio_system_get_volume_config(CFG_LINEIN_VOLUME, &audio_system->linein_volume);

    _audio_system_get_volume_config(CFG_USOUND_VOLUME, &audio_system->usound_volume);

    _audio_system_get_volume_config(CFG_LOCALPLAY_VOLUME, &audio_system->lcmusic_volume);

	_audio_system_get_volume_config(CFG_FM_VOLUME, &audio_system->fm_volume);

	_audio_system_get_volume_config(CFG_I2SRX_IN_VOLUME, &audio_system->i2srx_in_volume);

	_audio_system_get_volume_config(CFG_SPDIF_IN_VOLUME, &audio_system->spidf_in_volume);

    _audio_system_get_volume_config(CFG_TTS_MIN_VOL, &audio_system->min_volume);

    _audio_system_get_volume_config(CFG_TTS_MAX_VOL, &audio_system->max_volume);

	/* audio_system_set_output_sample_rate(48); */
#if CONFIG_AUDIO_ACTS
    /* init audio hal */
    hal_audio_out_init();

	hal_audio_in_init();
#endif
	/* audio_flinger_init(); */

	return 0;
}
