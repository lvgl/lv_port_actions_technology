/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief audio track.
*/

#include <os_common_api.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <audio_track.h>
#include <audio_device.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/util.h>
#include <media_mem.h>
#include <assert.h>
#include <ringbuff_stream.h>
#include <arithmetic.h>

#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "audio track"


enum {
    FADE_MODE_NONE,
    FADE_MODE_IN,
    FADE_MODE_OUT,
};


static uint8_t reload_pcm_buff[2048] __aligned(4) __in_section_unique(audio.bss.ouput_pcm);

extern int stream_read_pcm(asin_pcm_t *aspcm, io_stream_t stream, int max_samples, int debug_space);
extern uint32_t get_sample_rate_hz(uint8_t fs_khz);

#ifdef CONFIG_MEDIA_EFFECT
extern void *media_resample_open(uint8_t channels, uint8_t samplerate_in,
		uint8_t samplerate_out, int *samples_in, int *samples_out, uint8_t stream_type);
extern int media_resample_process(void *handle, uint8_t channels, void *output_buf[2],
		void *input_buf[2], int input_samples);
extern void media_resample_close(void *handle);

extern void *media_fade_open(uint8_t sample_rate, uint8_t channels, uint8_t sample_bits, uint8_t is_interweaved);
extern void media_fade_close(void *handle);
extern int media_fade_process(void *handle, void **inout_buf, int samples);
extern int media_fade_in_set(void *handle, int fade_time_ms);
extern int media_fade_out_set(void *handle, int fade_time_ms);
extern int media_fade_out_is_finished(void *handle);

extern void *media_mix_open(uint8_t sample_rate, uint8_t channels, uint8_t is_interweaved);
extern void media_mix_close(void *handle);
extern int media_mix_process(void *handle, void **inout_buf, void *mix_buf,
			     int samples);
#endif /* CONFIG_MEDIA_EFFECT */

#ifdef CONFIG_MEDIA_EFFECT
static int _audio_track_data_mix(struct audio_track_t *handle, s16_t *src_buff, uint16_t samples, s16_t *dest_buff)
{
	int ret = 0;
    int max_retry_cnt = 5;
	uint16_t mix_num = 0;
	asin_pcm_t mix_pcm = {
		.channels = handle->mix_channels,
		.sample_bits = 16,
		.pcm = { handle->res_in_buf[0], handle->res_in_buf[1], },
	};

	if (stream_get_length(handle->mix_stream) <= 0 && handle->res_remain_samples <= 0)
		return 0;

	while (1) {
		uint16_t mix_samples = MIN(samples, handle->res_remain_samples);
		s16_t *mix_buff[2] = {
			handle->res_out_buf[0] + handle->res_out_samples - handle->res_remain_samples,
			handle->res_out_buf[1] + handle->res_out_samples - handle->res_remain_samples,
		};

		/* 1) consume remain samples */
		if (handle->mix_handle && dest_buff == src_buff) {
#ifdef CONFIG_AUDIO_MIX
			media_mix_process(handle->mix_handle, (void **)&dest_buff, mix_buff[0], mix_samples);
			dest_buff += handle->channels * mix_samples;
			src_buff += handle->channels * mix_samples;
#endif //CONFIG_AUDIO_MIX
		} else {
            if(handle->audio_format == AUDIO_FORMAT_PCM_32_BIT) {
                int32_t *pdst = (int32_t*)dest_buff;
                int32_t *psrc = (int32_t*)src_buff;

                if (handle->channels > 1) {
    				for (int i = 0; i < mix_samples; i++) {
    					*pdst++ = (*psrc++) / 2 + (((int32_t)mix_buff[0][i] / 2) << 16);
    					*pdst++ = (*psrc++) / 2 + (((int32_t)mix_buff[1][i] / 2) << 16);
    				}
    			} else {
    				for (int i = 0; i < mix_samples; i++) {
    					*pdst++ = (*psrc++) / 2 + (((int32_t)mix_buff[0][i] / 2) << 16);
    				}
                }
            }else{
                if (handle->channels > 1) {
    				for (int i = 0; i < mix_samples; i++) {
    					*dest_buff++ = (*src_buff++) / 2 + mix_buff[0][i] / 2;
    					*dest_buff++ = (*src_buff++) / 2 + mix_buff[1][i] / 2;
    				}
    			} else {
    				for (int i = 0; i < mix_samples; i++) {
    					*dest_buff++ = (*src_buff++) / 2 + mix_buff[0][i] / 2;
    				}
    			}
            }
		}

		handle->res_remain_samples -= mix_samples;
		samples -= mix_samples;
		mix_num += mix_samples;
		if (samples <= 0)
			break;

		/* 2) read mix stream and do resample as required */
		mix_pcm.samples = 0;
		ret = stream_read_pcm(&mix_pcm, handle->mix_stream, handle->res_in_samples, INT32_MAX);
        //printk("mix read: %d, need: %d, have: %d\n", ret, handle->res_in_samples, stream_get_length(handle->mix_stream));
		if (ret <= 0)
		{
            if(stream_check_finished(handle->mix_stream))
                break;

            max_retry_cnt--;
            if(max_retry_cnt <= 0)
            {
                SYS_LOG_ERR("wait tip tone timeout\n");
    			break;
            }

            os_sleep(2);
            continue;
		}

		if (handle->res_handle) {
#ifdef CONFIG_RESAMPLE
			uint8_t res_channels = MIN(handle->mix_channels, handle->channels);

			if (ret < handle->res_in_samples) {
				memset((uint16_t *)mix_pcm.pcm[0] + ret, 0, (handle->res_in_samples - ret) * 2);
				if (res_channels > 1)
					memset((uint16_t *)mix_pcm.pcm[1] + ret, 0, (handle->res_in_samples - ret) * 2);
			}

			/* do resample */
			handle->res_out_samples = media_resample_process(handle->res_handle, res_channels,
					(void **)handle->res_out_buf, (void **)mix_pcm.pcm, handle->res_in_samples);
			handle->res_remain_samples = handle->res_out_samples;
#endif //CONFIG_RESAMPLE
		} else {
			handle->res_out_samples = ret;
			handle->res_remain_samples = handle->res_out_samples;
		}
	}

	return mix_num;
}
#endif /* CONFIG_MEDIA_EFFECT */

static int _audio_track_request_more_data(void *handle, uint32_t reason)
{
	static uint8_t printk_cnt = 0;
	struct audio_track_t *audio_track = (struct audio_track_t *)handle;
	int read_len = audio_track->pcm_frame_size / 2;
	int stream_length = stream_get_length(audio_track->audio_stream);
	int ret = 0;
	bool reload_mode = ((audio_track->channel_mode & AUDIO_DMA_RELOAD_MODE) == AUDIO_DMA_RELOAD_MODE);
	uint8_t *buf = NULL;

    if(!audio_track->stared) {
        return 0;
    }

	if (reload_mode) {
		if (reason == AOUT_DMA_IRQ_HF) {
			buf = audio_track->pcm_frame_buff;
		} else if (reason == AOUT_DMA_IRQ_TC) {
			buf = audio_track->pcm_frame_buff + read_len;
		}
	} else {
		buf = audio_track->pcm_frame_buff;
		if (stream_length > audio_track->pcm_frame_size) {
			read_len = audio_track->pcm_frame_size;
		} else {
			read_len = audio_track->pcm_frame_size / 2;
		}
		if (audio_track->channel_id == AOUT_FIFO_DAC0) {
			if (reason == AOUT_DMA_IRQ_TC) {
				os_printk("pcm empty\n");
			}
		}
	}


	if (audio_track->compensate_samples > 0) {
		/* insert data */
		if (audio_track->compensate_samples < read_len) {
			read_len = audio_track->compensate_samples;
		}
		memset(buf, 0, read_len);
		audio_track->fill_cnt += read_len;
		audio_track->compensate_samples -= read_len;
		goto exit;
	} else if (audio_track->compensate_samples < 0) {
		/* drop data */
		if (read_len > -audio_track->compensate_samples) {
			read_len = -audio_track->compensate_samples;
		}
		if (stream_get_length(audio_track->audio_stream) >= read_len * 2) {
			stream_read(audio_track->audio_stream, buf, read_len);
			audio_track->fill_cnt -= read_len;
			audio_track->compensate_samples += read_len;
		}
	}

	if (read_len > stream_get_length(audio_track->audio_stream)) {
		if (!audio_track->flushed) {
			if (!printk_cnt++) {
				printk("F\n");
                //_playback_decoder_dump_buffer(true);
			}
			int data_len = stream_get_length(audio_track->audio_stream);
			stream_read(audio_track->audio_stream, buf, data_len);
			memset(buf + data_len, 0, read_len - data_len);
			audio_track->fill_cnt += read_len;
			goto exit;
		} else {
			read_len = stream_get_length(audio_track->audio_stream);
            if(read_len < 0)
                read_len = 0;
		}
	} else {
	    printk_cnt = 0;
	}

	ret = stream_read(audio_track->audio_stream, buf, read_len);
	if (ret != read_len) {
		if (!printk_cnt++) {
			printk("F\n");
            //_playback_decoder_dump_buffer(true);
		}
		memset(buf, 0, read_len);
		audio_track->fill_cnt += read_len;
	}

	if (audio_track->muted) {
		memset(buf, 0, read_len);
		//audio_track->fill_cnt += read_len;
		goto exit;
	}

    ret = audio_track->pcm_frame_size / 2;
    if (reload_mode && (read_len != ret)) {
        if(audio_track->flushed && (audio_track->flushed < 10) && (0 == read_len)) {
            audio_track->flushed++;
        }

        memset(buf + read_len, 0, ret - read_len);
        read_len = ret;
    }

exit:
#ifdef CONFIG_MUSIC_DAE_FADE
    if (audio_track->fade_handle && (audio_track->fade_mode == FADE_MODE_OUT) && (read_len > 0)) {
        int samples = read_len / audio_track->frame_size;
        media_fade_process(audio_track->fade_handle, (void **)&buf, samples);
    }
#endif

	if (!reload_mode && read_len > 0) {
		hal_aout_channel_write_data(audio_track->audio_handle, buf, read_len);
	}

	if (audio_track->channel_id == AOUT_FIFO_DAC0 && !reload_mode) {
#if 0
		/**local music max send 3K pcm data */
		if (audio_track->stream_type == AUDIO_STREAM_LOCAL_MUSIC) {
			if (stream_get_length(audio_track->audio_stream) > audio_track->pcm_frame_size) {
				stream_read(audio_track->audio_stream, buf, audio_track->pcm_frame_size);

#ifdef CONFIG_MUSIC_DAE_FADE
                if (audio_track->fade_handle && (audio_track->fade_mode == FADE_MODE_OUT)) {
                    int samples = audio_track->pcm_frame_size / audio_track->frame_size;
                    media_fade_process(audio_track->fade_handle, (void **)&buf, samples);
                }
#endif
				hal_aout_channel_write_data(audio_track->audio_handle, buf, audio_track->pcm_frame_size);
			}

			if (stream_get_length(audio_track->audio_stream) > audio_track->pcm_frame_size) {
				stream_read(audio_track->audio_stream, buf, audio_track->pcm_frame_size);

#ifdef CONFIG_MUSIC_DAE_FADE
                if (audio_track->fade_handle && (audio_track->fade_mode == FADE_MODE_OUT)) {
                    int samples = audio_track->pcm_frame_size / audio_track->frame_size;
                    media_fade_process(audio_track->fade_handle, (void **)&buf, samples);
                }
#endif
				hal_aout_channel_write_data(audio_track->audio_handle, buf, audio_track->pcm_frame_size);
			}
		}
#endif

		/**last frame send more 2 samples */
		if (audio_track->flushed && stream_get_length(audio_track->audio_stream) == 0) {
			memset(buf, 0, audio_track->frame_size * 2);
			hal_aout_channel_write_data(audio_track->audio_handle, buf, audio_track->frame_size * 2);
		}
	}

	return 0;
}

static void *_audio_track_init(struct audio_track_t *handle)
{
	audio_out_init_param_t aout_param = {0};

#ifdef AOUT_CHANNEL_AA
	if (handle->channel_type == AOUT_CHANNEL_AA) {
		aout_param.aa_mode = 1;
	}
#endif

	aout_param.sample_rate =  handle->output_sample_rate_hz/1000;
	aout_param.channel_type = handle->channel_type;
	aout_param.channel_id =  handle->channel_id;
	aout_param.data_width = 16;
	aout_param.channel_mode = (handle->channels == 1) ? MONO_MODE : STEREO_MODE;
	aout_param.left_volume = audio_system_get_current_pa_volume(handle->stream_type);
	aout_param.right_volume = aout_param.left_volume;

	aout_param.sample_cnt_enable = 1;

	aout_param.callback = _audio_track_request_more_data;
	aout_param.callback_data = handle;

	if ((handle->channel_mode & AUDIO_DMA_RELOAD_MODE) == AUDIO_DMA_RELOAD_MODE) {
		aout_param.dma_reload = 1;
		aout_param.reload_len = handle->pcm_frame_size;
		aout_param.reload_addr = handle->pcm_frame_buff;
	}

	return hal_aout_channel_open(&aout_param);
}

static void _audio_track_stream_observer_notify(void *observer, int readoff, int writeoff, int total_size,
										unsigned char *buf, int num, stream_notify_type type)
{
	struct audio_track_t *handle = (struct audio_track_t *)observer;

#ifdef CONFIG_SNOOP_LINK_TWS
    handle->total_samples_filled += num / handle->frame_size;
#endif

#ifdef CONFIG_MEDIA_EFFECT
    if(!os_is_in_isr()) {
    	audio_system_mutex_lock();
    	if (handle->mix_stream && (type == STREAM_NOTIFY_PRE_WRITE)) {
    		_audio_track_data_mix(handle, (s16_t *)buf, num / handle->frame_size, (s16_t *)buf);
    	}
    	audio_system_mutex_unlock();
    }
#endif

	if (!handle->stared
		&& (type == STREAM_NOTIFY_WRITE)
		&& !audio_track_is_waitto_start(handle)
		&& stream_get_length(handle->audio_stream) >= handle->pcm_frame_size) {

        if(!os_is_in_isr()) {
    		os_sched_lock();
        }

		if (handle->event_cb)
			handle->event_cb(1, handle->user_data);

		handle->stared = 1;
        hal_aout_channel_reset_sample_cnt(handle->audio_handle);

		stream_read(handle->audio_stream, handle->pcm_frame_buff, handle->pcm_frame_size);
        handle->total_samples_filled = handle->pcm_frame_size / handle->frame_size;
        handle->sample_fix = 0;

#ifdef CONFIG_MEDIA_EFFECT
        if (handle->fade_handle  && (handle->fade_mode == FADE_MODE_IN)) {
            int samples = handle->pcm_frame_size / handle->frame_size;
            media_fade_process(handle->fade_handle, (void **)&handle->pcm_frame_buff, samples);
        }
#endif

		if ((handle->channel_mode & AUDIO_DMA_RELOAD_MODE) == AUDIO_DMA_RELOAD_MODE) {
			hal_aout_channel_start(handle->audio_handle);
		} else {
			hal_aout_channel_write_data(handle->audio_handle, handle->pcm_frame_buff, handle->pcm_frame_size);
		}

        if(!os_is_in_isr()) {
    		os_sched_unlock();
        }
	}
}

struct audio_track_t *audio_track_create(uint8_t stream_type, int sample_rate,
									uint8_t format, uint8_t audio_mode, void *outer_stream,
									void (*event_cb)(uint8_t event, void *user_data), void *user_data)
{
	struct audio_track_t *audio_track = NULL;

	audio_system_mutex_lock();

	audio_track = mem_malloc(sizeof(struct audio_track_t));
	if (!audio_track) {
		SYS_LOG_ERR("audio track malloc failed");
		goto err_exit;
	}

	memset(audio_track, 0, sizeof(struct audio_track_t));
	audio_track->stream_type = stream_type;
	audio_track->audio_format = format;
	audio_track->sample_rate = sample_rate;
	audio_track->compensate_samples = 0;

	audio_track->channel_type = audio_policy_get_out_channel_type(stream_type);
	audio_track->channel_id = audio_policy_get_out_channel_id(stream_type);
	audio_track->channel_mode = audio_policy_get_out_channel_mode(stream_type);
	audio_track->volume = audio_system_get_stream_volume(stream_type);

	audio_track->output_sample_rate_hz = audio_system_get_output_sample_rate();
	if (!audio_track->output_sample_rate_hz) {
		audio_track->output_sample_rate_hz = audio_track->sample_rate;
	}
    audio_track->output_sample_rate_hz = get_sample_rate_hz(audio_track->output_sample_rate_hz);

	if (audio_mode == AUDIO_MODE_DEFAULT)
		audio_mode = audio_policy_get_out_audio_mode(stream_type);

	if (audio_mode == AUDIO_MODE_MONO) {
		audio_track->frame_size = 2;
        audio_track->channels = 1;
	} else if (audio_mode == AUDIO_MODE_STEREO) {
		audio_track->frame_size = 4;
        audio_track->channels = 2;
	}

	/* dma reload buff */
	if (system_check_low_latencey_mode()) {
		if (audio_track->sample_rate <= 16) {
			audio_track->pcm_frame_size = 256;
		} else {
			audio_track->pcm_frame_size = 512;
		}
	} else {
		audio_track->pcm_frame_size = (sample_rate <= 16) ? (sample_rate * audio_track->frame_size * 8) : 1024;
	}

	audio_track->pcm_frame_buff = reload_pcm_buff;
	if (!audio_track->pcm_frame_buff) {
		SYS_LOG_ERR("pcm_frame_buff init failed");
		goto err_exit;
	}

	audio_track->audio_handle = _audio_track_init(audio_track);
	if (!audio_track->audio_handle) {
		SYS_LOG_ERR("audio track init failed");
		goto err_exit;
	}

    hal_aout_channel_enable_sample_cnt(audio_track->audio_handle, true);
	if (audio_system_get_current_volume(audio_track->stream_type) == 0) {
		hal_aout_channel_mute_ctl(audio_track->audio_handle, 1);
	} else {
		hal_aout_channel_mute_ctl(audio_track->audio_handle, 0);
	}

	if (outer_stream) {
		audio_track->audio_stream = ringbuff_stream_create((struct acts_ringbuf *)outer_stream);
	} else {
		audio_track->audio_stream = ringbuff_stream_create_ext(
									media_mem_get_cache_pool(OUTPUT_PCM, stream_type),
									media_mem_get_cache_pool_size(OUTPUT_PCM, stream_type));
	}

	if (!audio_track->audio_stream) {
		SYS_LOG_ERR("stream create failed");
		goto err_exit;
	}

	if (stream_open(audio_track->audio_stream, MODE_IN_OUT | MODE_WRITE_BLOCK)) {
		stream_destroy(audio_track->audio_stream);
		audio_track->audio_stream = NULL;
		SYS_LOG_ERR(" stream open failed ");
		goto err_exit;
	}

    if (stream_get_space(audio_track->audio_stream)+ stream_get_length(audio_track->audio_stream) < audio_track->pcm_frame_size) {
		audio_track->pcm_frame_size = stream_get_space(audio_track->audio_stream)+ stream_get_length(audio_track->audio_stream);
		if (audio_track->pcm_frame_size / 2 < CONFIG_AUDIO_DAC_0_PCMBUF_HF_THRES) {
			hal_aout_set_pcm_threshold(audio_track->audio_handle, CONFIG_AUDIO_DAC_0_PCMBUF_HE_THRES / 2, CONFIG_AUDIO_DAC_0_PCMBUF_HF_THRES / 2);
            SYS_LOG_INF(" new pcm buf he threshold:%d,  hf:%d\n", CONFIG_AUDIO_DAC_0_PCMBUF_HE_THRES / 2, CONFIG_AUDIO_DAC_0_PCMBUF_HF_THRES / 2);
        }
	}

	stream_set_observer(audio_track->audio_stream, audio_track,
		_audio_track_stream_observer_notify, STREAM_NOTIFY_WRITE | STREAM_NOTIFY_PRE_WRITE);

	if (audio_system_register_track(audio_track)) {
		SYS_LOG_ERR(" registy track failed ");
		goto err_exit;
	}

#if defined(CONFIG_MEDIA_EFFECT)
    audio_track->fade_handle = media_fade_open(
        audio_track->sample_rate,
        audio_track->channels,
        (audio_track->audio_format != AUDIO_FORMAT_PCM_16_BIT) ? 32 : 16,
        1);
#endif

	audio_track->event_cb = event_cb;
	audio_track->user_data = user_data;

	if (audio_track->sample_rate <= 16) {
        int he_thres = audio_track->sample_rate * audio_track->channels * 4;
        hal_aout_set_pcm_threshold(audio_track->audio_handle, he_thres, he_thres + 4);
	}

	SYS_LOG_INF("stream_type : %d", audio_track->stream_type);
	SYS_LOG_INF("audio_format : %d", audio_track->audio_format);
	SYS_LOG_INF("audio_mode : %d", audio_track->channels);
	SYS_LOG_INF("channel_type : %d", audio_track->channel_type);
	SYS_LOG_INF("channel_id : %d", audio_track->channel_id);
	SYS_LOG_INF("channel_mode : %d", audio_track->channel_mode);
	SYS_LOG_INF("input_sr : %d ", audio_track->sample_rate);
	SYS_LOG_INF("output_sr : %d", audio_track->output_sample_rate_hz);
	SYS_LOG_INF("volume : %d", audio_track->volume);
	SYS_LOG_INF("audio_stream : %p", audio_track->audio_stream);
	audio_system_mutex_unlock();
	return audio_track;

err_exit:
	if (audio_track)
		mem_free(audio_track);
	audio_system_mutex_unlock();
	return NULL;
}

int audio_track_destory(struct audio_track_t *handle)
{
	assert(handle);

	SYS_LOG_INF("destory %p begin", handle);
	audio_system_mutex_lock();

	if (audio_system_unregister_track(handle)) {
		SYS_LOG_ERR(" failed\n");
		return -ESRCH;
	}

	if (handle->audio_handle) {
		hal_aout_channel_close(handle->audio_handle);
	}

	if (handle->audio_stream) {
        stream_close(handle->audio_stream);
		stream_destroy(handle->audio_stream);
	}

#ifdef CONFIG_MEDIA_EFFECT
	if (handle->fade_handle)
		media_fade_close(handle->fade_handle);
#endif

	mem_free(handle);

	audio_system_mutex_unlock();
	SYS_LOG_INF("destory %p ok", handle);
	return 0;
}

int audio_track_start(struct audio_track_t *handle)
{
	assert(handle);

	audio_track_set_waitto_start(handle, false);

	if (!handle->stared && stream_get_length(handle->audio_stream) >= handle->pcm_frame_size) {

		os_sched_lock();

		if (handle->event_cb) {
			handle->event_cb(1, handle->user_data);
		}

		handle->stared = 1;
        hal_aout_channel_reset_sample_cnt(handle->audio_handle);

		stream_read(handle->audio_stream, handle->pcm_frame_buff, handle->pcm_frame_size);
        handle->total_samples_filled = handle->pcm_frame_size / handle->frame_size;
        handle->sample_fix = 0;

#ifdef CONFIG_MEDIA_EFFECT
        if (handle->fade_handle  && (handle->fade_mode == FADE_MODE_IN)) {
            int samples = handle->pcm_frame_size / handle->frame_size;
            media_fade_process(handle->fade_handle, (void **)&handle->pcm_frame_buff, samples);
        }
#endif

		if ((handle->channel_mode & AUDIO_DMA_RELOAD_MODE) == AUDIO_DMA_RELOAD_MODE) {
			hal_aout_channel_start(handle->audio_handle);
		} else {
			hal_aout_channel_write_data(handle->audio_handle, handle->pcm_frame_buff, handle->pcm_frame_size);
        }

		os_sched_unlock();
	}

	return 0;
}

int audio_track_stop(struct audio_track_t *handle)
{
	assert(handle);

	SYS_LOG_INF("stop %p begin ", handle);

    handle->stared = 0;
	if (handle->audio_handle)
		hal_aout_channel_stop(handle->audio_handle);

	SYS_LOG_INF("stop %p ok ", handle);
	return 0;
}

int audio_track_pause(struct audio_track_t *handle)
{
	assert(handle);

	handle->muted = 1;

	stream_flush(handle->audio_stream);

	return 0;
}

int audio_track_resume(struct audio_track_t *handle)
{
	assert(handle);

	handle->muted = 0;

	return 0;
}

int audio_track_mute(struct audio_track_t *handle, int mute)
{
	assert(handle);

    handle->muted = mute;

	return 0;
}

int audio_track_is_muted(struct audio_track_t *handle)
{
	assert(handle);

    return handle->muted;
}

int audio_track_write(struct audio_track_t *handle, unsigned char *buf, int num)
{
	int ret = 0;

	assert(handle && handle->audio_stream);

	ret = stream_write(handle->audio_stream, buf, num);
	if (ret != num) {
		SYS_LOG_WRN(" %d %d\n", ret, num);
	}

	return ret;
}

int audio_track_flush(struct audio_track_t *handle)
{
	int try_cnt = 0;
	uint32_t flags;
    bool reload_mode = ((handle->channel_mode & AUDIO_DMA_RELOAD_MODE) == AUDIO_DMA_RELOAD_MODE);

	assert(handle);

	flags = irq_lock();
	if (handle->flushed) {
		irq_unlock(flags);
		return 0;
	}
	/**wait trace stream read empty*/
	handle->flushed = 1;
	irq_unlock(flags);

    if(reload_mode) {
    	while (stream_get_length(handle->audio_stream) > 0
    			&& (try_cnt++ < 100) && handle->stared) {
    		if (try_cnt % 10 == 0) {
    			SYS_LOG_INF("try_cnt %d stream %d\n", try_cnt,
    					stream_get_length(handle->audio_stream));
    		}
    		os_sleep(2);
    	}

    	SYS_LOG_INF("try_cnt %d, left_len %d\n", try_cnt,
    			stream_get_length(handle->audio_stream));
    } else {
        int pcm_buffer_size = hal_aout_channel_get_buffer_size(handle->audio_handle) - 1;

        while ((hal_aout_channel_get_buffer_space(handle->audio_handle) < pcm_buffer_size)
    			&& (try_cnt++ < 100) && handle->stared) {
    		if (try_cnt % 10 == 0) {
    			SYS_LOG_INF("try_cnt %d space %d\n", try_cnt,
    					hal_aout_channel_get_buffer_space(handle->audio_handle));
    		}
    		os_sleep(2);
    	}
#ifdef CONFIG_MEDIA_EFFECT
    	SYS_LOG_INF("try_cnt %d, left_len %d + %d, fadeout: %d\n", try_cnt,
    			pcm_buffer_size + 1 - hal_aout_channel_get_buffer_space(handle->audio_handle),
    			stream_get_length(handle->audio_stream),
    			media_fade_out_is_finished(handle->fade_handle));
#endif
    }

	audio_track_set_mix_stream(handle, NULL, 0, 1, AUDIO_STREAM_TTS);

	return 0;
}

int audio_track_set_fade_in(struct audio_track_t *handle, int fade_time)
{
	assert(handle);

#if defined(CONFIG_MEDIA_EFFECT)
    if (handle->fade_handle && (handle->fade_mode != FADE_MODE_IN)) {
        SYS_LOG_INF("fade in %d\n", fade_time);
        media_fade_in_set(handle->fade_handle, fade_time);
        handle->fade_mode = FADE_MODE_IN;
    }
#endif
	return 0;
}

int audio_track_set_fade_out(struct audio_track_t *handle, int fade_time)
{
	assert(handle);

#if defined(CONFIG_MEDIA_EFFECT)
    if (handle->fade_handle && (handle->fade_mode != FADE_MODE_OUT)) {
        SYS_LOG_INF("fade out %d\n", fade_time);
        media_fade_out_set(handle->fade_handle, fade_time);
        handle->fade_mode = FADE_MODE_OUT;
    }
#endif
	return 0;
}

int audio_track_set_waitto_start(struct audio_track_t *handle, bool wait)
{
	assert(handle);

	handle->waitto_start = wait ? 1 : 0;
	return 0;
}

int audio_track_is_waitto_start(struct audio_track_t *handle)
{
	if (!handle)
		return 0;

	return handle->waitto_start;
}

int audio_track_set_volume(struct audio_track_t *handle, int volume)
{
	uint32_t pa_volume = 0;

	assert(handle);

	SYS_LOG_INF(" volume %d\n", volume);
	if (volume) {
		if (!handle->muted) {
			hal_aout_channel_mute_ctl(handle->audio_handle, 0);
		}
		pa_volume = audio_policy_get_pa_volume(handle->stream_type, volume);
		hal_aout_channel_set_pa_vol_level(handle->audio_handle, pa_volume);
	} else {
		hal_aout_channel_mute_ctl(handle->audio_handle, 1);
		if (audio_policy_get_out_channel_type(handle->stream_type) == AUDIO_CHANNEL_I2STX) {
			/* i2s not support mute, so we set lowest volume -71625*/
			pa_volume = -71625;
			hal_aout_channel_set_pa_vol_level(handle->audio_handle, pa_volume);
		}
	}

	handle->volume = volume;
	return 0;
}

int audio_track_set_pa_volume(struct audio_track_t *handle, int pa_volume)
{
	assert(handle);

	SYS_LOG_INF("pa_volume %d\n", pa_volume);

	hal_aout_channel_mute_ctl(handle->audio_handle, 0);
	hal_aout_channel_set_pa_vol_level(handle->audio_handle, pa_volume);

	/* sync back to volume level, though meaningless for AUDIO_STREAM_VOICE at present. */
	handle->volume = audio_policy_get_volume_level_by_db(handle->stream_type, pa_volume / 100);
	return 0;
}

int audio_track_set_mute(struct audio_track_t *handle, bool mute)
{
	assert(handle);

	SYS_LOG_INF(" mute %d\n", mute);

	handle->muted = mute;

	hal_aout_channel_mute_ctl(handle->audio_handle, mute);

	return 0;
}

int audio_track_get_volume(struct audio_track_t *handle)
{
	assert(handle);

	return handle->volume;
}

int audio_track_set_samplerate(struct audio_track_t *handle, int sample_rate)
{
	assert(handle);

	handle->sample_rate = sample_rate;
	return 0;
}

int audio_track_get_samplerate(struct audio_track_t *handle)
{
	assert(handle);

	return handle->sample_rate;
}

io_stream_t audio_track_get_stream(struct audio_track_t *handle)
{
	assert(handle);

	return handle->audio_stream;
}

int audio_track_get_fill_samples(struct audio_track_t *handle)
{
	assert(handle);
	if (handle->fill_cnt) {
		SYS_LOG_INF("fill_cnt %d\n", handle->fill_cnt);
	}
	return handle->fill_cnt;
}

int audio_track_compensate_samples(struct audio_track_t *handle, int samples_cnt)
{
#ifdef CONFIG_SYS_IRQ_LOCK
	SYS_IRQ_FLAGS flags;
#endif
	assert(handle);
#ifdef CONFIG_SYS_IRQ_LOCK
	sys_irq_lock(&flags);
#endif
	handle->compensate_samples = samples_cnt;
#ifdef CONFIG_SYS_IRQ_LOCK
	sys_irq_unlock(&flags);
#endif
	return 0;
}

uint32_t audio_track_get_play_time(struct audio_track_t *handle)
{
    uint32_t play_time;
    uint32_t sample_count;
    uint64_t sample_play;
    int32_t sample_fix;

    assert(handle);
    if(!handle->stared)
        return 0;

    uint32_t flags;

    flags = irq_lock();
    sample_play = handle->total_samples_filled + handle->sample_fix;
    sample_fix = handle->sample_fix;
    sample_count = hal_aout_channel_get_sample_cnt(handle->audio_handle) / handle->channels;
    irq_unlock(flags);

    if((sample_play & 0x3FFFll) >= (sample_count & 0x3FFF)) {
        sample_play = (sample_play & ~0x3FFFll) | (sample_count & 0x3FFF);
    } else {
        sample_play = ((sample_play & ~0x3FFFll) | (sample_count & 0x3FFF)) - 0x4000;
    }

    play_time = (sample_play - sample_fix) * 1000 / handle->output_sample_rate_hz;
	return play_time;
}

uint32_t audio_track_get_play_sample_cnt(struct audio_track_t *handle)
{
    return hal_aout_channel_get_sample_cnt(handle->audio_handle) / handle->channels;
}

int audio_track_set_lr_channel_enable(struct audio_track_t *handle, bool l_enable, bool r_enable)
{
	assert(handle);

	SYS_LOG_INF("lr enable: %d, %d\n", l_enable, r_enable);
	return hal_aout_lr_channel_enable(handle->audio_handle, l_enable, r_enable);
}

int audio_track_set_fifo_src(struct audio_track_t *handle, bool from_dsp, void *dsp_audio_set_param)
{
	int ret = 0;
	assert(handle);

	SYS_LOG_INF("from dsp: %d\n", from_dsp);

	if (from_dsp) {
        int he_thres;
        int hf_thres;

        switch(handle->stream_type) {
        case AUDIO_STREAM_MUSIC:
            //he_thres = 0x780;
            he_thres = handle->output_sample_rate_hz / 1000 * handle->channels * 12;
            hf_thres = he_thres + 0x4;
            break;
        case AUDIO_STREAM_VOICE:
            he_thres = handle->output_sample_rate_hz / 1000 * handle->channels * 4;
            hf_thres = he_thres + 0x4;
            break;
        case AUDIO_STREAM_TTS:
            he_thres = handle->output_sample_rate_hz / 1000 * handle->channels * 16;
            hf_thres = he_thres + 0x4;
            break;

        default:
            he_thres = 0xF0;
            hf_thres = he_thres + 0x4;
            break;
        }

        handle->dsp_fifo_src = 1;
		hal_aout_set_pcm_threshold(handle->audio_handle, he_thres, hf_thres);
		ret = hal_aout_set_fifo_src(handle->audio_handle, handle->channel_id, true, dsp_audio_set_param);

		hal_aout_channel_start(handle->audio_handle);
		handle->stared = 1;
	} else {
		ret = hal_aout_set_fifo_src(handle->audio_handle, handle->channel_id, false, NULL);
	}


	return ret;
}

int audio_track_set_mix_stream(struct audio_track_t *handle, io_stream_t mix_stream,
		uint8_t sample_rate, uint8_t channels, uint8_t stream_type)
{
#ifdef CONFIG_MEDIA_EFFECT
	int res = 0;

	assert(handle);

	audio_system_mutex_lock();

	if (audio_system_get_track() != handle) {
		goto exit;
	}

	if (mix_stream) {
		uint16_t *frame_buf = media_mem_get_cache_pool(RESAMPLE_FRAME_DATA, stream_type);
		int frame_buf_size = media_mem_get_cache_pool_size(RESAMPLE_FRAME_DATA, stream_type);
		uint8_t res_channels = MIN(channels, handle->channels);

		if (sample_rate != handle->sample_rate) {
			int frame_size;

			handle->res_handle = media_resample_open(
					res_channels, sample_rate, handle->sample_rate,
					&handle->res_in_samples, &handle->res_out_samples, stream_type);
			if (!handle->res_handle) {
				SYS_LOG_ERR("media_resample_open failed");
				res = -ENOMEM;
				goto exit;
			}

			frame_size = channels * (ROUND_UP(handle->res_in_samples, 2) + ROUND_UP(handle->res_out_samples, 2));

			if (frame_buf_size / 2 < frame_size) {
				SYS_LOG_ERR("frame mem not enough");
				media_resample_close(handle->res_handle);
				handle->res_handle = NULL;
				res = -ENOMEM;
				goto exit;
			}

			handle->res_in_buf[0] = frame_buf;
			handle->res_in_buf[1] = (channels > 1) ?
					handle->res_in_buf[0] + ROUND_UP(handle->res_in_samples, 2) : handle->res_in_buf[0];

			handle->res_out_buf[0] = handle->res_in_buf[1] + ROUND_UP(handle->res_in_samples, 2);
			handle->res_out_buf[1] = (res_channels > 1) ?
					handle->res_out_buf[0] + ROUND_UP(handle->res_out_samples, 2) : handle->res_out_buf[0];
		} else {
			handle->res_in_samples = frame_buf_size / 2 / channels;
			handle->res_in_buf[0] = frame_buf;
			handle->res_in_buf[1] = (channels > 1) ?
					handle->res_in_buf[0] + handle->res_in_samples : handle->res_in_buf[0];

			handle->res_out_buf[0] = handle->res_in_buf[0];
			handle->res_out_buf[1] = handle->res_in_buf[1];
		}

		handle->res_out_samples = 0;
		handle->res_remain_samples = 0;

		/* open mix: only support 1 mix channels */
		if (handle->mix_channels == 1) {
			handle->mix_handle = media_mix_open(handle->sample_rate, handle->channels, 1);
		}
	}

	handle->mix_stream = mix_stream;
	handle->mix_sample_rate = sample_rate;
	handle->mix_channels = channels;

	if (!handle->mix_stream) {
		if (handle->res_handle) {
			media_resample_close(handle->res_handle);
			handle->res_handle = NULL;
		}

		if (handle->mix_handle) {
			media_mix_close(handle->mix_handle);
			handle->mix_handle = NULL;
		}
	}

	SYS_LOG_INF("mix_stream %p, sample_rate %d->%d\n",
			mix_stream, sample_rate, handle->sample_rate);

exit:
	audio_system_mutex_unlock();
	return res;
#else
	return -ENOSYS;
#endif /* CONFIG_MEDIA_EFFECT */
}

io_stream_t audio_track_get_mix_stream(struct audio_track_t *handle)
{
	assert(handle);

	return handle->mix_stream;
}
