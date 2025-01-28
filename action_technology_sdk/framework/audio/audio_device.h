/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 
/**
 * @file audio device interface
 */

#ifndef __AUDIO_DEVICE_H__
#define __AUDIO_DEVICE_H__
#include <audio_system.h>
/**
 * @cond INTERNAL_HIDDEN
 */

typedef enum 
{
	AUDIO_TYPE_IN,
	AUDIO_TYPE_OUT,
} audio_type_e;

typedef enum 
{
	AUDIO_DMA_MODE = 1,
	AUDIO_DSP_MODE = 2,
	AUDIO_ASRC_MODE = 4,
	AUDIO_DMA_RELOAD_MODE = 8,
} audio_channel_mode_e;

struct audio_device_t *audio_device_create(uint8_t stream_type, int sample_rate,
										uint8_t format, uint8_t audio_mode);

int audio_device_attach_track(struct audio_device_t *device, struct audio_track_t *track);

int audio_device_dettach_track(struct audio_device_t *device, struct audio_track_t *track);

int audio_device_mix_data(struct audio_device_t *device);

int audio_device_mix_data(struct audio_device_t *device);

int audio_device_mix_data(struct audio_device_t *device);

int audio_device_set_volume(struct audio_device_t *handle, int volume);

int audio_device_start(struct audio_device_t *handle);

int audio_device_pause(struct audio_device_t *handle);

int audio_device_resume(struct audio_device_t *handle);

int audio_device_stop(struct audio_device_t *handle);
/**
 * INTERNAL_HIDDEN @endcond
 */
#endif



