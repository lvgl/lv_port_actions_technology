/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief audio record.
*/

#include <os_common_api.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <audio_hal.h>
#include <audio_record.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <media_mem.h>
#include <ringbuff_stream.h>
#include <audio_device.h>



#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "audio record"

#define AUDIO_RECORD_PCM_BUFF_SIZE  1024

#ifdef CONFIG_FIXED_DMA_ACCESS_PSRAM
static uint8_t record_pcm_buff[1024] __aligned(4) __in_section_unique(audio.bss.input_pcm);
#endif

static int _audio_record_request_more_data(void *priv_data, uint32_t reason)
{
	int ret = 0;
    struct audio_record_t *handle = (struct audio_record_t *)priv_data;
	int num = handle->pcm_buff_size / 2;
	uint8_t *buf = NULL;

	if (!handle->audio_stream || handle->paused)
		goto exit;

	if (reason == AOUT_DMA_IRQ_HF) {
		buf = handle->pcm_buff;
	} else if (reason == AOUT_DMA_IRQ_TC) {
		buf = handle->pcm_buff + num;
	}

	if (handle->muted) {
		memset(buf, 0, num);
	}

	/**TODO: this avoid adc noise for first block*/
	if (handle->first_frame) {
		memset(buf, 0, num);
		handle->first_frame = 0;
	}

	ret = stream_write(handle->audio_stream, buf, num);
	if (ret <= 0) {
        if(handle->printk_cnt == 0) {
    		SYS_LOG_WRN("data full ret %d ", ret);
        }

        handle->printk_cnt++;
	} else {
        handle->printk_cnt = 0;
	}

exit:
	return 0;
}

static void *_audio_record_init(struct audio_record_t *handle)
{
	audio_in_init_param_t ain_param = {0};

	memset(&ain_param, 0, sizeof(audio_in_init_param_t));

	ain_param.sample_rate = handle->sample_rate;
	ain_param.adc_gain = handle->adc_gain;
	ain_param.input_gain = handle->input_gain;
	ain_param.boost_gain = 0;
	ain_param.data_mode = handle->audio_mode;
	ain_param.data_width = 16;
	ain_param.dma_reload = 1;
	ain_param.reload_addr = handle->pcm_buff;
	ain_param.reload_len = handle->pcm_buff_size;
	ain_param.channel_type = handle->channel_type;

	ain_param.audio_device = audio_policy_get_record_channel_id(handle->stream_type);

	ain_param.callback = _audio_record_request_more_data;
	ain_param.callback_data = handle;

    return hal_ain_channel_open(&(ain_param));
}

struct audio_record_t *audio_record_create(uint8_t stream_type, int sample_rate_input, int sample_rate_output,
									uint8_t format, uint8_t audio_mode, void *outer_stream)
{
	struct audio_record_t *audio_record = NULL;

	audio_record = mem_malloc(sizeof(struct audio_record_t));
	if (!audio_record)
		return NULL;

	/* dma reload buff */
	if (system_check_low_latencey_mode()) {
		audio_record->pcm_buff_size = (sample_rate_input <= 16) ? 256 : 512;
	} else {
		audio_record->pcm_buff_size = (sample_rate_input <= 16) ? 512 : 1024;
	}

#ifdef CONFIG_FIXED_DMA_ACCESS_PSRAM
	audio_record->pcm_buff = record_pcm_buff;
	
#else
	audio_record->pcm_buff = mem_malloc(audio_record->pcm_buff_size);
#endif

	if (!audio_record->pcm_buff)
		goto err_exit;

	audio_record->stream_type = stream_type;
	audio_record->audio_format = format;
	audio_record->audio_mode = audio_mode;
	audio_record->output_sample_rate = sample_rate_output;

	audio_record->channel_type = audio_policy_get_record_channel_type(stream_type);
	audio_record->channel_mode = audio_policy_get_record_channel_mode(stream_type);
	audio_record->adc_gain = audio_policy_get_record_adc_gain(stream_type);
	audio_record->input_gain = audio_policy_get_record_input_gain(stream_type);
	audio_record->sample_rate = sample_rate_input;
	audio_record->first_frame = 1;

	if (audio_record->audio_mode == AUDIO_MODE_DEFAULT)
		audio_record->audio_mode = audio_policy_get_record_audio_mode(stream_type);

	if (audio_record->audio_mode == AUDIO_MODE_MONO) {
		audio_record->frame_size = 2;
	} else if (audio_record->audio_mode == AUDIO_MODE_STEREO) {
		audio_record->frame_size = 4;
	}

	audio_record->audio_handle = _audio_record_init(audio_record);
	if (!audio_record->audio_handle) {
		goto err_exit;
	}

	if (outer_stream) {
		audio_record->audio_stream = ringbuff_stream_create((struct acts_ringbuf *)outer_stream);
	} else {
		audio_record->audio_stream = ringbuff_stream_create_ext(
									media_mem_get_cache_pool(INPUT_PCM, stream_type),
									media_mem_get_cache_pool_size(INPUT_PCM, stream_type));
	}

	if (!audio_record->audio_stream) {
		SYS_LOG_ERR("audio_stream create failed");
		goto err_exit;
	}

	if (stream_open(audio_record->audio_stream, MODE_IN_OUT)) {
		stream_destroy(audio_record->audio_stream);
		audio_record->audio_stream = NULL;
		SYS_LOG_ERR(" audio_stream open failed ");
		goto err_exit;
	}

	if (audio_system_register_record(audio_record)) {
		SYS_LOG_ERR(" audio_system_registy_track failed ");
		stream_close(audio_record->audio_stream);
		stream_destroy(audio_record->audio_stream);
		goto err_exit;
	}

	if (system_check_low_latencey_mode()) {
		if (audio_record->stream_type == AUDIO_STREAM_VOICE) {
			if(sample_rate_input == 16) {
				memset(audio_record->pcm_buff, 0, audio_record->pcm_buff_size);
				stream_write(audio_record->audio_stream, audio_record->pcm_buff, 256);
				stream_write(audio_record->audio_stream, audio_record->pcm_buff, 256);
				stream_write(audio_record->audio_stream, audio_record->pcm_buff, 208);
			}
		}
	}

	if (audio_policy_get_aec_reference_type() == AEC_REF_TYPE_HW 
		&& audio_record->stream_type == AUDIO_STREAM_VOICE){
		hal_ain_set_aec_record_back(audio_record->audio_handle, 1);
	}

	SYS_LOG_INF("stream_type : %d", audio_record->stream_type);
	SYS_LOG_INF("audio_format : %d", audio_record->audio_format);
	SYS_LOG_INF("audio_mode : %d", audio_record->audio_mode);
	SYS_LOG_INF("channel_type : %d ", audio_record->channel_type);
	SYS_LOG_INF("channel_id : %d ", audio_record->channel_id);
	SYS_LOG_INF("channel_mode : %d ", audio_record->channel_mode);
	SYS_LOG_INF("input_sr : %d ", audio_record->sample_rate);
	SYS_LOG_INF("output_sr : %d ", audio_record->output_sample_rate);
	SYS_LOG_INF("volume : %d ", audio_record->volume);
	SYS_LOG_INF("audio_handle : %p", audio_record->audio_handle);
	SYS_LOG_INF("audio_stream : %p", audio_record->audio_stream);
	if (outer_stream) {
		SYS_LOG_INF("audio_stream ptr : %x", ((struct acts_ringbuf *)outer_stream)->cpu_ptr);
	}
	return audio_record;

err_exit:

#ifndef CONFIG_FIXED_DMA_ACCESS_PSRAM
	if (audio_record->pcm_buff)
		mem_free(audio_record->pcm_buff);
#endif

	mem_free(audio_record);

	return NULL;
}

int audio_record_destory(struct audio_record_t *handle)
{
	if (!handle)
		return -EINVAL;

	if (audio_system_unregister_record(handle)) {
		SYS_LOG_ERR(" audio_system_unregisty_record failed ");
		return -ESRCH;
	}

	if (handle->audio_handle)
		hal_ain_channel_close(handle->audio_handle);

	if (handle->audio_stream)
		stream_destroy(handle->audio_stream);

#ifndef CONFIG_FIXED_DMA_ACCESS_PSRAM
	if (handle->pcm_buff) {
		mem_free(handle->pcm_buff);
		handle->pcm_buff = NULL;
	}
#endif

	mem_free(handle);

	SYS_LOG_INF(" handle: %p ok ", handle);

	return 0;
}

int audio_record_start(struct audio_record_t *handle)
{
	if (!handle)
		return -EINVAL;

	hal_ain_channel_read_data(handle->audio_handle, handle->pcm_buff, handle->pcm_buff_size);

	hal_ain_channel_start(handle->audio_handle);

	return 0;
}

int audio_record_stop(struct audio_record_t *handle)
{
	if (!handle)
		return -EINVAL;

	if (handle->audio_stream)
		stream_close(handle->audio_stream);

	if (audio_policy_get_aec_reference_type() == AEC_REF_TYPE_HW 
		&& handle->stream_type == AUDIO_STREAM_VOICE){
		hal_ain_set_aec_record_back(handle->audio_handle, 0);
	}

	if (handle->audio_handle)
		hal_ain_channel_stop(handle->audio_handle);

	return 0;
}

int audio_record_pause(struct audio_record_t *handle)
{
	if (!handle)
		return -EINVAL;

	handle->paused = 1;

	return 0;
}

int audio_record_resume(struct audio_record_t *handle)
{
	if (!handle)
		return -EINVAL;

	handle->paused = 0;

	return 0;
}

int audio_record_read(struct audio_record_t *handle, uint8_t *buff, int num)
{
	if (!handle->audio_stream)
		return 0;

	return stream_read(handle->audio_stream, buff, num);
}

int audio_record_flush(struct audio_record_t *handle)
{
	if (!handle)
		return -EINVAL;

	/**TODO: wanghui wait record data empty*/

	return 0;
}

int audio_record_set_volume(struct audio_record_t *handle, int volume)
{
	adc_gain gain;
	uint8_t i;

	if (!handle)
		return -EINVAL;

	handle->volume = volume;

	for (i = 0; i < ADC_CH_NUM_MAX; i++)
		gain.ch_gain[i] = volume;

	hal_ain_channel_set_volume(handle->audio_handle, &gain);
	SYS_LOG_INF("volume ---%d\n", volume);


	return 0;
}

int audio_record_get_volume(struct audio_record_t *handle)
{
	if (!handle)
		return -EINVAL;

	return handle->volume;
}

int audio_record_set_sample_rate(struct audio_record_t *handle, int sample_rate)
{
	if (!handle)
		return -EINVAL;

	handle->sample_rate = sample_rate;

	return 0;
}

int audio_record_get_sample_rate(struct audio_record_t *handle)
{
	if (!handle)
		return -EINVAL;

	return handle->sample_rate;
}

io_stream_t audio_record_get_stream(struct audio_record_t *handle)
{
	if (!handle)
		return NULL;

	return handle->audio_stream;
}

int audio_record_set_play_flag(struct audio_record_t *handle, uint16_t *play_flag)
{
	if (!handle || !play_flag)
		return -EINVAL;

	handle->play_flag = play_flag;
    handle->drop_cnt = 1;
	return 0;
}
