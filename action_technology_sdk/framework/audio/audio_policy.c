/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief audio policy.
*/

#include <os_common_api.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <audio_system.h>
#include <audio_policy.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <audio_device.h>
#include <media_type.h>


#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "audio policy"

static const struct audio_policy_t *user_policy;

int audio_policy_get_out_channel_type(uint8_t stream_type)
{
	int out_channel = AUDIO_CHANNEL_DAC;

	if (user_policy)
		out_channel = user_policy->audio_out_channel;

	return out_channel;
}

int audio_policy_get_out_channel_id(uint8_t stream_type)
{
	int out_channel_id = AOUT_FIFO_DAC0;

	return out_channel_id;
}

int audio_policy_get_out_channel_mode(uint8_t stream_type)
{
	int out_channel_mode = AUDIO_DMA_MODE;

	switch (stream_type) {
		case AUDIO_STREAM_TTS:
        case AUDIO_STREAM_TIP:
		case AUDIO_STREAM_LINEIN:
		case AUDIO_STREAM_SPDIF_IN:
		case AUDIO_STREAM_FM:
		case AUDIO_STREAM_I2SRX_IN:
		case AUDIO_STREAM_MIC_IN:
		case AUDIO_STREAM_MUSIC:
		case AUDIO_STREAM_LOCAL_MUSIC:
		case AUDIO_STREAM_USOUND:
		case AUDIO_STREAM_VOICE:
		case AUDIO_STREAM_LE_AUDIO:
			out_channel_mode = AUDIO_DMA_MODE;
			break;
	}
	return out_channel_mode;
}

int audio_policy_get_out_pcm_channel_num(uint8_t stream_type)
{
	int channel_num = 0;

	switch (stream_type) {
	case AUDIO_STREAM_TTS:
    case AUDIO_STREAM_TIP:
		break;
	case AUDIO_STREAM_VOICE:
	case AUDIO_STREAM_LE_AUDIO:
		break;

	case AUDIO_STREAM_MIC_IN:
		break;
	default: /* decoder decided */
		channel_num = 1;
		break;
	}

	if ((user_policy && (user_policy->audio_out_channel & AUDIO_CHANNEL_I2STX))
	|| (user_policy && (user_policy->audio_out_channel & AUDIO_CHANNEL_SPDIFTX))) {
		channel_num = 2;
	}

	return channel_num;
}

int audio_policy_get_out_pcm_frame_size(uint8_t stream_type)
{
	int frame_size = 512;

	switch (stream_type) {
		case AUDIO_STREAM_TTS:
        case AUDIO_STREAM_TIP:
			frame_size = 4 * 960;
			break;
		case AUDIO_STREAM_USOUND:
		case AUDIO_STREAM_LINEIN:
		case AUDIO_STREAM_FM:
		case AUDIO_STREAM_I2SRX_IN:
		case AUDIO_STREAM_MIC_IN:
		case AUDIO_STREAM_SPDIF_IN:
		case AUDIO_STREAM_MUSIC:
		case AUDIO_STREAM_LOCAL_MUSIC:
			frame_size = 2048;
			break;
		case AUDIO_STREAM_VOICE:
			frame_size = 512;
			break;
		default:
			break;
	}

	return frame_size;
}

int audio_policy_get_out_input_start_threshold(uint8_t stream_type, uint8_t exf_stream_type, uint8_t sample_rate, uint8_t channels, uint8_t tws_mode, uint8_t format)
{
	switch (stream_type) {
	case AUDIO_STREAM_MUSIC:
		if (system_check_low_latencey_mode())
			return 40;
		else
			return 150;
	case AUDIO_STREAM_VOICE:
		if (system_check_low_latencey_mode()) {
			return 0;
		} else {
			return 60;
		}
	case AUDIO_STREAM_USOUND:
	case AUDIO_STREAM_LINEIN:
	case AUDIO_STREAM_I2SRX_IN:
	case AUDIO_STREAM_SPDIF_IN:
	case AUDIO_STREAM_FM:
		return 0;
	case AUDIO_STREAM_TTS:
    case AUDIO_STREAM_TIP:
	default:
		return 0;
	}
}

int audio_policy_get_out_input_stop_threshold(uint8_t stream_type, uint8_t exf_stream_type, uint8_t sample_rate, uint8_t channels, uint8_t tws_mode)
{
	switch (stream_type) {
	case AUDIO_STREAM_MUSIC:
		return 2;
	case AUDIO_STREAM_VOICE:
	case AUDIO_STREAM_LE_AUDIO:
	case AUDIO_STREAM_TTS:
    case AUDIO_STREAM_TIP:
	default:
		return 0;
	}
}

int audio_policy_get_out_audio_mode(uint8_t stream_type)
{
	int audio_mode = AUDIO_MODE_STEREO;

	switch (stream_type) {
		/**283D tts is mono ,but zs285A tts is stereo*/
		/* case AUDIO_STREAM_TTS: */
		case AUDIO_STREAM_VOICE:
		case AUDIO_STREAM_LOCAL_RECORD:
		case AUDIO_STREAM_GMA_RECORD:
		case AUDIO_STREAM_MIC_IN:
		case AUDIO_STREAM_LE_AUDIO:
		audio_mode = AUDIO_MODE_MONO;
		break;
	}

	return audio_mode;
}

uint8_t audio_policy_get_out_effect_type(uint8_t stream_type,
			uint8_t efx_stream_type, bool is_tws)
{
	switch (stream_type) {
	case AUDIO_STREAM_LOCAL_MUSIC:
		if (is_tws)
			efx_stream_type = AUDIO_STREAM_TWS;
		break;
	case AUDIO_STREAM_MUSIC:
		if (is_tws && efx_stream_type == AUDIO_STREAM_LINEIN) {
			efx_stream_type = AUDIO_STREAM_LINEIN;
		}
		break;
	default:
		break;
	}

	return efx_stream_type;
}

uint8_t audio_policy_get_record_effect_type(uint8_t stream_type,
			uint8_t efx_stream_type)
{
	return efx_stream_type;
}

int audio_policy_get_record_audio_mode(uint8_t stream_type)
{
	int audio_mode = AUDIO_MODE_STEREO;

	switch (stream_type) {
		case AUDIO_STREAM_VOICE:
		case AUDIO_STREAM_AI:
		case AUDIO_STREAM_LOCAL_RECORD:
		case AUDIO_STREAM_GMA_RECORD:
		case AUDIO_STREAM_MIC_IN:
		audio_mode = AUDIO_MODE_MONO;
		break;
	}

	return audio_mode;
}

int audio_policy_get_record_channel_id(uint8_t stream_type)
{
	int channel_id = AUDIO_ANALOG_MIC0;

	switch (stream_type) {
	case AUDIO_STREAM_USOUND:
	case AUDIO_STREAM_VOICE:
	case AUDIO_STREAM_LOCAL_RECORD:
	case AUDIO_STREAM_GMA_RECORD:
	case AUDIO_STREAM_MIC_IN:
	case AUDIO_STREAM_MUSIC:
	case AUDIO_STREAM_LOCAL_MUSIC:
	case AUDIO_STREAM_AI:
	case AUDIO_STREAM_LE_AUDIO:
		channel_id = AUDIO_ANALOG_MIC0;
		break;
	case AUDIO_STREAM_LINEIN:
		channel_id = AUDIO_LINE_IN0;
		break;
	case AUDIO_STREAM_FM:
		channel_id = AUDIO_ANALOG_FM0;
		break;
	}
	return channel_id;
}

int audio_policy_is_master_mix_channel(uint8_t stream_type)
{
    if(stream_type == AUDIO_STREAM_LINEIN_MIX
		|| stream_type == AUDIO_STREAM_USOUND_MIX){
        return true;
    }else{
        return false;
    }
}

int audio_policy_get_record_aec_block_size(uint8_t format_type)
{
	int block_size = 0;
/*	if (format_type == MSBC_TYPE) {
		block_size = 256 * 2 - 192;
	}else if (format_type == CVSD_TYPE){
		block_size = 256 * 2 - 128;
	}*/

	return block_size;
}

int audio_policy_get_record_channel_mix_channel(uint8_t stream_type)
{
    int mix_channel = false;

    switch (stream_type){
        case AUDIO_STREAM_LINEIN_MIX:
            mix_channel = true;
            break;
        case AUDIO_STREAM_USOUND_MIX:
            mix_channel = true;
            break;
    }

    return mix_channel;
}

int audio_policy_get_record_adc_gain(uint8_t stream_type)
{
	int gain = 0;

	return gain;
}

int audio_policy_get_record_input_gain(uint8_t stream_type)
{
	int gain = 0;/* odb */

	switch (stream_type) {
	case AUDIO_STREAM_LINEIN:
			if (user_policy)
				gain = user_policy->audio_in_linein_gain;
	break;
		case AUDIO_STREAM_FM:
			if (user_policy)
				gain = user_policy->audio_in_fm_gain;
	break;
			break;
		case AUDIO_STREAM_USOUND:
	case AUDIO_STREAM_VOICE:
		case AUDIO_STREAM_LOCAL_RECORD:
		case AUDIO_STREAM_GMA_RECORD:
		case AUDIO_STREAM_MIC_IN:
			if (user_policy)
				gain = user_policy->audio_in_mic_gain;
	break;
	case AUDIO_STREAM_AI:
			if (user_policy)
				gain = user_policy->audio_in_mic_gain;
	break;
	case AUDIO_STREAM_LE_AUDIO:
			if (user_policy)
				gain = user_policy->audio_in_mic_gain;
	break;
	}
	printk("audio_policy_get_record_input_gain --------%d \n",gain);
	return gain;
}

int audio_policy_get_record_channel_mode(uint8_t stream_type)
{
	int channel_mode =  AUDIO_DMA_MODE | AUDIO_DMA_RELOAD_MODE;

	switch (stream_type) {

	}

	return channel_mode;
}

int audio_policy_get_record_channel_support_aec(uint8_t stream_type)
{
	int support_aec = false;

	switch (stream_type) {
		case AUDIO_STREAM_VOICE:
		support_aec = true;
		break;
	}

    return support_aec;
}

int audio_policy_get_record_channel_aec_tail_length(uint8_t stream_type, uint8_t sample_rate, bool in_debug)
{
	switch (stream_type) {
	case AUDIO_STREAM_VOICE:
		if (in_debug) {
			return (sample_rate > 8) ? 32 : 64;
		} else if (user_policy) {
			return (sample_rate > 8) ?
				user_policy->voice_aec_tail_length_16k :
				user_policy->voice_aec_tail_length_8k;
		} else {
			return (sample_rate > 8) ? 48 : 96;
		}
	default:
		return 0;
	}
}

int audio_policy_is_out_channel_aec_reference(uint8_t stream_type)
{
    if(stream_type == AUDIO_STREAM_VOICE){
        return true;
    }else{
        return false;
    }
}

int audio_policy_get_out_channel_aec_reference_stream_type(uint8_t stream_type)
{
    if(stream_type == AUDIO_STREAM_VOICE){
        return AUDIO_MODE_MONO;
    }else{
        return 0;
    }
}

int audio_policy_get_channel_resample(uint8_t stream_type)
{
    int resample = false;

    switch (stream_type) {
	case AUDIO_STREAM_MUSIC:
	    /* resample = true; */
	break;
    }

    return resample;
}

int audio_policy_get_output_support_multi_track(uint8_t stream_type)
{
	int support_multi_track = false;

#ifdef CONFIG_AUDIO_SUBWOOFER
	switch (stream_type) {
		case AUDIO_STREAM_MUSIC:
		case AUDIO_STREAM_LINEIN:
		case AUDIO_STREAM_MIC_IN:
		case AUDIO_STREAM_USOUND:
			support_multi_track = true;
			break;
	}
#endif

	return support_multi_track;
}

int audio_policy_get_record_channel_type(uint8_t stream_type)
{
	int channel_type = AUDIO_CHANNEL_ADC;

	switch (stream_type) {
	case AUDIO_STREAM_SPDIF_IN:
		channel_type = AUDIO_CHANNEL_SPDIFRX;
		break;

	case AUDIO_STREAM_I2SRX_IN:
		channel_type = AUDIO_CHANNEL_I2SRX;
		break;
	}
	return channel_type;
}

int audio_policy_get_pa_volume(uint8_t stream_type, uint8_t volume_level)
{
	int pa_volume = volume_level;

	if (!user_policy)
		goto exit;

	if (volume_level > user_policy->audio_out_volume_level) {
		volume_level = user_policy->audio_out_volume_level;
	}

	switch (stream_type) {
		case AUDIO_STREAM_VOICE:
			pa_volume = user_policy->voice_pa_table[volume_level];
			break;
		case AUDIO_STREAM_LINEIN:
        case AUDIO_STREAM_FM:
        case AUDIO_STREAM_I2SRX_IN:
		case AUDIO_STREAM_MIC_IN:
		case AUDIO_STREAM_MUSIC:
		case AUDIO_STREAM_LOCAL_MUSIC:
		case AUDIO_STREAM_SPDIF_IN:
		case AUDIO_STREAM_LE_AUDIO:
			pa_volume = user_policy->music_pa_table[volume_level];
			break;
		case AUDIO_STREAM_TTS:
        case AUDIO_STREAM_TIP:
			pa_volume = user_policy->tts_pa_table[volume_level];
			break;
	}

exit:
	return pa_volume;
}

int audio_policy_get_da_volume(uint8_t stream_type, uint8_t volume_level)
{
	int da_volume = volume_level;

	if (!user_policy)
		goto exit;

	if (volume_level > user_policy->audio_out_volume_level) {
		volume_level = user_policy->audio_out_volume_level;
	}

	switch (stream_type) {
	case AUDIO_STREAM_VOICE:
		da_volume = user_policy->voice_da_table[volume_level];
		break;
	case AUDIO_STREAM_LINEIN:
	case AUDIO_STREAM_FM:
	case AUDIO_STREAM_I2SRX_IN:
	case AUDIO_STREAM_MIC_IN:
	case AUDIO_STREAM_SPDIF_IN:
	case AUDIO_STREAM_MUSIC:
	case AUDIO_STREAM_LOCAL_MUSIC:
		da_volume = user_policy->music_da_table[volume_level];
		break;
	case AUDIO_STREAM_TTS:
    case AUDIO_STREAM_TIP:
		da_volume = user_policy->tts_da_table[volume_level];
		break;
	}

exit:
	return da_volume;
}

int audio_policy_get_volume_level_by_db(uint8_t stream_type, int volume_db)
{
	int volume_level = 0;
	int max_level = 0;
	const int *volume_table = NULL;

	if (!user_policy)
		goto exit;

	max_level = user_policy->audio_out_volume_level;
	volume_table = user_policy->music_pa_table;

	switch (stream_type) {
		case AUDIO_STREAM_VOICE:
			volume_table = user_policy->voice_pa_table;
			break;
		case AUDIO_STREAM_LINEIN:
		case AUDIO_STREAM_FM:
		case AUDIO_STREAM_I2SRX_IN:
		case AUDIO_STREAM_MIC_IN:
		case AUDIO_STREAM_SPDIF_IN:
		case AUDIO_STREAM_MUSIC:
		case AUDIO_STREAM_LOCAL_MUSIC:
			volume_table = user_policy->music_pa_table;
			break;
		case AUDIO_STREAM_TTS:
        case AUDIO_STREAM_TIP:
			volume_table = user_policy->tts_pa_table;
			break;
	}

	/* to 0.001 dB */
	volume_db *= 100;

	if (volume_db == volume_table[max_level - 1]) {
		volume_level = max_level;
	} else {
		for (int i = 0; i < max_level; i++) {
			if (volume_db < volume_table[i]) {
				volume_level = i;
				break;
			}
		}
	}

exit:
	return volume_level;
}
int audio_policy_get_aec_reference_type(void)
{
#ifdef CONFIG_AUDIO_VOICE_HARDWARE_REFERENCE
    return 2;
#else
    return 1;
#endif
}

int audio_policy_get_volume_level(void)
{
	if (user_policy)
		return user_policy->audio_out_volume_level;

	return 16;
}

int audio_policy_check_tts_fixed_volume(void)
{
	if (user_policy)
		return user_policy->tts_fixed_volume;

	return 1;
}

aset_volume_table_v2_t *audio_policy_get_aset_volume_table(void)
{
	if (user_policy)
		return user_policy->aset_volume_table;

	return NULL;
}

int audio_policy_check_save_volume_to_nvram(void)
{
	if (user_policy)
		return user_policy->volume_saveto_nvram;

	return 1;
}

int audio_policy_get_reduce_threshold(int format)
{
	if (system_check_low_latencey_mode()) {
		if (format == MSBC_TYPE || format == CVSD_TYPE) {
			return 5;
		} else {
			return 15;
		}
	} else {
		if (format == MSBC_TYPE || format == CVSD_TYPE) {
			return 30;
		} else if (format == PCM_TYPE) {
			return 16;
		} else {
			return 100;
		}
	}
}

int audio_policy_get_increase_threshold(int format)
{
	if (system_check_low_latencey_mode()) {
		if (format == MSBC_TYPE || format == CVSD_TYPE) {
			return 15;
		} else {
			return 50;
		}
	} else {
		if (format == MSBC_TYPE || format == CVSD_TYPE) {
			return 34;
		} else if (format == PCM_TYPE) {
			return 32;
		} else {
			return 200;
		}
	}
}

int audio_policy_check_snoop_tws_support(void)
{
#ifdef CONFIG_SNOOP_LINK_TWS
    return 1;
#else
    return 0;
#endif
}

int audio_policy_register(const struct audio_policy_t *policy)
{
	user_policy = policy;
	return 0;
}
