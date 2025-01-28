/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief audio track.
*/
#ifndef __AUDIO_TRACK_H__
#define __AUDIO_TRACK_H__

#include <audio_system.h>
#include <audio_policy.h>
#include <stream.h>
/**
 * @defgroup audio_track_apis Auido Track APIs
 * @{
 */
/**
 * @brief Create New Track Instance.
 *
 * This routine Create new Track instance.
 *
 * @param stream_type stream type of track.
 * @param sampleRate sample rate of track output 
 * @param format audio trace format 
 * @param channel_mode audio mode of track , mono or stereo
 * @param outer_stream Track output stream
 * @param event_cb event callback, event=1 track started, event=0 track stopped
 * @param user_data user_data for event_cb
 *
 * @return handle of new track
 */
struct audio_track_t *audio_track_create(uint8_t stream_type,
						 			int sampleRate, uint8_t format,
									uint8_t channel_mode, void *outer_stream,
									void (*event_cb)(uint8_t event, void *user_data), void *user_data);

/**
 * @brief Destory a Track Instance.
 *
 * This routine Destory a Track Instance 
 *
 * @param handle handle of Track
 *
 * @return 0 excute successed , others failed
 */
int audio_track_destory(struct audio_track_t *handle);
/**
 * @brief Start Audio Track Output
 *
 * This routine Start audio Track Output, enable adc and dma.
 *
 * @param handle handle of Track
 *
 * @return 0 excute successed , others failed
 */
int audio_track_start(struct audio_track_t *handle);
/**
 * @brief Stop Audio Track Output
 *
 * This routine Stop audio Track Output, disable adc and stop dma.
 *
 * @param handle handle of Track
 *
 * @return 0 excute successed , others failed
 */
int audio_track_stop(struct audio_track_t *handle);
/**
 * @brief Pause Audio Track Output
 *
 * This routine pause audio Track Output, pause dma output
 *
 * @param handle handle of Track
 *
 * @return 0 excute successed , others failed
 */
int audio_track_pause(struct audio_track_t *handle);
/**
 * @brief Resume Audio Track Output
 *
 * This routine Resume Audio Track Output, dma start and continue output
 *
 * @param handle handle of Track
 *
 * @return 0 excute successed , others failed
 */
int audio_track_resume(struct audio_track_t *handle);
/**
 * @brief Write data to audio track
 *
 * This routine Write data to audio track.
 *
 * @param handle handle of track
 * @param buff store data witch want to write ot audio track
 * @param num number of bytes want to write
 *
 * @return len of write datas
 */
int audio_track_write(struct audio_track_t *handle, unsigned char *buf, int num);
/**
 * @brief flush Audio Track data
 *
 * This routine flush Audio Track data, wait audio track data output finished.
 *
 * @param handle handle of Track
 *
 * @return 0 excute successed , others failed
 */
int audio_track_flush(struct audio_track_t *handle);
/**
 * @cond INTERNAL_HIDDEN
 */
int audio_track_set_waitto_start(struct audio_track_t *handle, bool wait);
int audio_track_is_waitto_start(struct audio_track_t *handle);
int audio_track_get_fill_samples(struct audio_track_t *handle);
int audio_track_compensate_samples(struct audio_track_t *handle, int samples_cnt);
int audio_track_set_mix_stream(struct audio_track_t *handle, io_stream_t mix_stream,
		uint8_t sample_rate, uint8_t channels, uint8_t stream_type);
uint32_t audio_track_get_play_time(struct audio_track_t *handle);
io_stream_t audio_track_get_mix_stream(struct audio_track_t *handle);
int audio_track_set_mute(struct audio_track_t *handle, bool mute);
int audio_track_set_fade_out(struct audio_track_t *handle, int fade_time);
int audio_track_set_fade_in(struct audio_track_t *handle, int fade_time);
/**
 * INTERNAL_HIDDEN @endcond
 */
/**
 * @brief set track volume
 *
 * This routine set track volume, adjust audio track output gain.
 *
 * @param handle handle of track
 * @param volume gain level for track output.
 *
 * @return 0 excute successed , others failed
 */
int audio_track_set_volume(struct audio_track_t *handle, int volume);
/**
 * @brief get track volume
 *
 * This routine get track volume, get audio track output gain.
 *
 * @param handle handle of track
 *
 * @return value of track gain
 */
int audio_track_get_volume(struct audio_track_t *handle);
/**
 * @brief set track pa volume
 *
 * This routine set track pa volume, adjust audio track output gain.
 *
 * @param handle handle of track
 * @param pa_volume gain (in 0.001 dB) for track output.
 *
 * @return 0 excute successed , others failed
 */
int audio_track_set_pa_volume(struct audio_track_t *handle, int pa_volume);
/**
 * @brief set track sample rate
 *
 * This routine set track sample rate.
 *
 * @param handle handle of record
 * @param sample_rate sample rate for track.
 *
 * @return 0 excute successed , others failed
 */
int audio_track_set_samplerate(struct audio_track_t *handle, int sample_rate);
/**
 * @brief get track sample rate
 *
 * This routine get track sample rate.
 *
 * @param handle handle of track
 *
 * @return value of track sample rate
 */
int audio_track_get_samplerate(struct audio_track_t *handle);
/**
 * @brief Get Audio Track Stream
 *
 * This routine Get Audio Track Stream
 *
 * @param handle handle of Track
 *
 * @return handle of audio Track stream
 */
io_stream_t audio_track_get_stream(struct audio_track_t *handle);

/**
 * @brief Set audio Track mute
 *
 * This routine Set audio Track mute
 *
 * @param handle handle of Track
 * @param mute 1: mute 0: unmute
 *
 * @return 0 excute successed , others failed
 */
int audio_track_mute(struct audio_track_t *handle, int mute);
/**
 * @brief check audio Track mute
 *
 * This routine check audio Track mute state
 *
 * @param handle handle of Track
 *
 * @return state of track mute 
 */
int audio_track_is_muted(struct audio_track_t *handle);

/**
 * @brief get DAC output sample count
 *
 * This routine get DAC output sample count
 *
 * @param handle handle of Track
 *
 * @return DAC output sample count
 */
uint32_t audio_track_get_play_sample_cnt(struct audio_track_t *handle);


/**
 * @brief set audio Track fifo (dacfifo) source
 *
 * This routine set audio Track fifo (dacfifo) source
 *
 * @param handle handle of Track
 * @param from_dsp fifo source from dsp or no
 * @param dsp_audio_set_param callback function of dsp audio set param to dsp core
 *
 * @return 0 excute successed , others failed
 */
int audio_track_set_fifo_src(struct audio_track_t *handle, bool from_dsp, void *dsp_audio_set_param);

/**
 * @} end defgroup audio_track_apis
 */
#endif