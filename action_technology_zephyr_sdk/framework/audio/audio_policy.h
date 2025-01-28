/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief audio policy.
*/

#ifndef __AUDIO_POLICY_H__
#define __AUDIO_POLICY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <audio_hal.h>
#include <media_effect_param.h>

/**
 * @defgroup audio_policy_apis Auido policy APIs
 * @{
 */

struct audio_policy_t {
	uint8_t audio_out_channel;
	uint8_t tts_fixed_volume:1;
	uint8_t volume_saveto_nvram:1;
	uint8_t audio_out_volume_level;
    uint8_t tts_max_volume;
    uint8_t tts_min_volume;

	s16_t audio_in_linein_gain;
	s16_t audio_in_fm_gain;
	s16_t audio_in_mic_gain;

	uint8_t voice_aec_tail_length_8k;
	uint8_t voice_aec_tail_length_16k;

	const short *music_da_table; /* 0.1 dB */
	const int   *music_pa_table; /* 0.001 dB */
	const short *voice_da_table; /* 0.1 dB */
	const int   *voice_pa_table; /* 0.001 dB */
	const short *usound_da_table; /* 0.1 dB */
	const int   *usound_pa_table; /* 0.001 dB */
	const short *tts_da_table; /* 0.1 dB */
	const int   *tts_pa_table; /* 0.001 dB */
	aset_volume_table_v2_t *aset_volume_table;
};

typedef enum {
	AEC_REF_TYPE_SF = 1,
	AEC_REF_TYPE_HW = 2,
} aec_type;

/**
 * @cond INTERNAL_HIDDEN
 */
int audio_policy_get_out_channel_type(uint8_t stream_type);

int audio_policy_get_out_channel_id(uint8_t stream_type);

int audio_policy_get_out_channel_mode(uint8_t stream_type);

/* return 0 for decoder decided */
int audio_policy_get_out_pcm_channel_num(uint8_t stream_type);

int audio_policy_get_out_pcm_frame_size(uint8_t stream_type);

int audio_policy_get_out_channel_asrc_alloc_method(uint8_t stream_type);

int audio_policy_get_out_input_start_threshold(uint8_t stream_type,
			uint8_t exf_stream_type, uint8_t sample_rate, uint8_t channels, uint8_t tws_mode, uint8_t format);

int audio_policy_get_out_input_stop_threshold(uint8_t stream_type,
			uint8_t exf_stream_type, uint8_t sample_rate, uint8_t channels, uint8_t tws_mode);

int audio_policy_get_out_audio_mode(uint8_t stream_type);

uint8_t audio_policy_get_out_effect_type(uint8_t stream_type,
			uint8_t efx_stream_type, bool is_tws);

int audio_policy_is_out_channel_aec_reference(uint8_t stream_type);

int audio_policy_is_out_channel_aec_reference(uint8_t stream_type);

uint8_t audio_policy_get_record_effect_type(uint8_t stream_type,
			uint8_t efx_stream_type);

int audio_policy_get_record_audio_mode(uint8_t stream_type);

int audio_policy_get_record_channel_id(uint8_t stream_type);

int audio_policy_get_record_adc_gain(uint8_t stream_type);

int audio_policy_get_record_input_gain(uint8_t stream_type);

int audio_policy_get_record_channel_mode(uint8_t stream_type);

int audio_policy_get_record_channel_type(uint8_t stream_type);

int audio_policy_get_volume_level(void);

/* return volume in 0.001 dB */
int audio_policy_get_pa_volume(uint8_t stream_type, uint8_t volume_level);
int audio_policy_get_pa_class(uint8_t stream_type);

/* return volume in 0.1 dB */
int audio_policy_get_da_volume(uint8_t stream_type, uint8_t volume_level);

int audio_policy_get_record_channel_support_aec(uint8_t stream_type);

int audio_policy_get_record_channel_aec_tail_length(uint8_t stream_type, uint8_t sample_rate, bool in_debug);

int audio_policy_get_channel_resample(uint8_t stream_type);

int audio_policy_get_output_support_multi_track(uint8_t stream_type);

int audio_policy_check_tts_fixed_volume();

int audio_policy_check_save_volume_to_nvram(void);

int audio_policy_get_increase_threshold(int format);

int audio_policy_get_reduce_threshold(int format);

/* @volume_db in 0.1 dB */
int audio_policy_get_volume_level_by_db(uint8_t stream_type, int volume_db);

aset_volume_table_v2_t *audio_policy_get_aset_volume_table(void);

int audio_policy_is_master_mix_channel(uint8_t stream_type);

int audio_policy_get_record_aec_block_size(uint8_t format_type);

int audio_policy_get_record_channel_mix_channel(uint8_t stream_type);

int audio_policy_get_aec_reference_type(void);

int audio_policy_get_out_channel_aec_reference_stream_type(uint8_t stream_type);

int audio_policy_check_snoop_tws_support(void);

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @brief Register user audio policy to audio system
 *
 * This routine Register user audio policy to audio system, must be called when system
 * init. if not set user audio policy ,system may used default policy for audio system.
 *
 * @param user_policy user sudio policy
 *
 * @return 0 excute successed , others failed
 */

int audio_policy_register(const struct audio_policy_t *user_policy);

/**
 * @} end defgroup audio_policy_apis
 */
#ifdef __cplusplus
}
#endif
#endif