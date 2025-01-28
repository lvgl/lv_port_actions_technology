/**
 *  ***************************************************************************
 *  Copyright (c) 2003-2020 Actions Semiconductor. All rights reserved.
 *
 *  \file       spp_mic_upload.c
 *  \brief      upload MIC record data during spp connection
 *  \author     zhouxl
 *  \version    1.00
 *  \date       2020-8-1
 *  ***************************************************************************
 *
 *  \History:
 *  Version 1.00
 *       Initial release
 */

#include "abtp_inner.h"
#include <acts_ringbuf.h>

#define SUPPORT_FIXED_MIC_GAIN 1
mic_upload_context_t *g_mic_upload_ctx;

#ifdef CONFIG_UI_MEMORY_MANAGER
extern void ui_mem_res_free(void * ptr);
extern void * ui_mem_res_alloc(size_t size);
#endif

static char *mic_update_buf;
bool mic_data_start_capture(thread_timer_expiry_t expiry_fn, u32_t sample_rate, u32_t channel_bitmap)
{
    struct audio_record_t *audio_record;
    uint8_t stream_type;
    //audio_input_gain input_gain;
    g_mic_upload_ctx = app_mem_malloc(sizeof(mic_upload_context_t));
    if(!g_mic_upload_ctx)
        return false;
    memset(g_mic_upload_ctx, 0, sizeof(mic_upload_context_t));

    g_mic_upload_ctx->channel_bitmap = channel_bitmap;
    g_mic_upload_ctx->sample_rate = sample_rate;

    g_mic_upload_ctx->micdata_ref = app_mem_malloc(MICDATA_PACKET_SIZE * 2);
    if(!g_mic_upload_ctx->micdata_ref)
        return false;
    g_mic_upload_ctx->micdata_main = app_mem_malloc(MICDATA_PACKET_SIZE * 2);
    if(!g_mic_upload_ctx->micdata_main)
        return false;
    g_mic_upload_ctx->spp_data = app_mem_malloc(MICDATA_PACKET_SIZE + sizeof(mic_pcm_frame_info_t));
    if(!g_mic_upload_ctx->spp_data)
        return false;

    g_mic_upload_ctx->pcm_frame_head = (mic_pcm_frame_info_t*)g_mic_upload_ctx->spp_data;
    g_mic_upload_ctx->pcm_frame_head->data_flag = 0x16;
    g_mic_upload_ctx->pcm_frame_head->payload_len = MICDATA_PACKET_SIZE + 8;
    g_mic_upload_ctx->pcm_frame_head->sample_rate = sample_rate;
    g_mic_upload_ctx->pcm_frame_head->sequence = 0;
#ifdef CONFIG_UI_MEMORY_MANAGER
	mic_update_buf = ui_mem_res_alloc(32 * 30 * 1024);
#endif
	printk("mic_update_buf %p \n",mic_update_buf);
	g_mic_upload_ctx->ringbuf_audio_input = acts_ringbuf_init_ext(mic_update_buf, 32 * 30 * 1024);
    #if SUPPORT_FIXED_MIC_GAIN
    uint8_t i;

    if(channel_bitmap & 0x3) {
        stream_type = AUDIO_STREAM_VOICE;
        for(i=0; i < 4; i++){
            //input_gain.ch_again[i] = 210;  //again
            //input_gain.ch_dgain[i] = 0;    //dgain
        }
    } //else {
     //   stream_type = AUDIO_STREAM_VOICE_ANC;
     //   for(i=0; i < 4; i++){
            //input_gain.ch_again[i] = 90;   //again
            //input_gain.ch_dgain[i] = 0;    //dgain
     //   }
   // }
    #else
    CFG_Type_MIC_Gain  MIC_Gain;
    app_config_read
    (
        CFG_ID_BT_CALL_QUALITY,
        &MIC_Gain,
        offsetof(CFG_Struct_BT_Call_Quality, MIC_Gain),
        sizeof(CFG_Type_MIC_Gain)
    );
    #if CONFIG_AUDIO_IN_ADGAIN_SUPPORT

    if(channel_bitmap & 0x3) {
        stream_type = AUDIO_STREAM_VOICE;
    } else {
        stream_type = AUDIO_STREAM_VOICE_ANC;
    }
    //input_gain.ch_again[0] = MIC_Gain.ADC0_aGain;
    //input_gain.ch_again[1] = MIC_Gain.ADC1_aGain;
    //input_gain.ch_again[2] = MIC_Gain.ADC2_aGain;
    //input_gain.ch_again[3] = MIC_Gain.ADC3_aGain;
    //input_gain.ch_dgain[0] = MIC_Gain.ADC0_dGain;
    //input_gain.ch_dgain[1] = MIC_Gain.ADC1_dGain;
    //input_gain.ch_dgain[2] = MIC_Gain.ADC2_dGain;
    //input_gain.ch_dgain[3] = MIC_Gain.ADC3_dGain;
    #else
    //input_gain.ch_gain[0] = MIC_Gain.ADC0_Gain;
    //input_gain.ch_gain[1] = MIC_Gain.ADC1_Gain;
    //input_gain.ch_gain[2] = MIC_Gain.ADC2_Gain;
    //input_gain.ch_gain[3] = MIC_Gain.ADC3_Gain;
    #endif
    #endif
    //g_mic_upload_ctx->mic_exchange = audio_policy_get_record_channel_swap(AUDIO_STREAM_MIC_IN, stream_type);
    audio_record = audio_record_create(AUDIO_STREAM_VOICE, sample_rate / 1000, sample_rate / 1000,
        AUDIO_FORMAT_PCM_16_BIT, AUDIO_MODE_MONO, (void *)g_mic_upload_ctx->ringbuf_audio_input);

    if (!audio_record)
    {
        SYS_LOG_ERR("failed");
        return FALSE;
    }
    else
    {
        //audio_record_set_volume(audio_record, &input_gain);
        g_mic_upload_ctx->audio_record = audio_record;
        g_mic_upload_ctx->audio_stream = audio_record_get_stream(audio_record);
        SYS_LOG_INF("free %d", stream_get_space(g_mic_upload_ctx->audio_stream));
        audio_record_start(audio_record);

        thread_timer_init(&g_mic_upload_ctx->ttimer, expiry_fn, NULL);
        thread_timer_start(&g_mic_upload_ctx->ttimer, 5, 5);
        return TRUE;
    }
}

void mic_data_stop_capture(void)
{
    if (NULL == g_mic_upload_ctx)
    {
        return;
    }

    if (g_mic_upload_ctx->audio_record)
    {
        thread_timer_stop(&g_mic_upload_ctx->ttimer);
        audio_record_stop(g_mic_upload_ctx->audio_record);
        audio_record_destory(g_mic_upload_ctx->audio_record);
    }

    if(g_mic_upload_ctx->micdata_ref != NULL)
    {
        app_mem_free(g_mic_upload_ctx->micdata_ref);
        g_mic_upload_ctx->micdata_ref = NULL;
    }

    if(g_mic_upload_ctx->micdata_main != NULL)
    {
        app_mem_free(g_mic_upload_ctx->micdata_main);
        g_mic_upload_ctx->micdata_main = NULL;
    }

    if(g_mic_upload_ctx->spp_data != NULL)
    {
        app_mem_free(g_mic_upload_ctx->spp_data);
        g_mic_upload_ctx->spp_data = NULL;
    }

	if (g_mic_upload_ctx->ringbuf_audio_input) {
		acts_ringbuf_destroy_ext(g_mic_upload_ctx->ringbuf_audio_input);
	}

	if (mic_update_buf) {
#ifdef CONFIG_UI_MEMORY_MANAGER
		ui_mem_res_free(mic_update_buf);
#endif
		mic_update_buf = NULL;
	}
    app_mem_free(g_mic_upload_ctx);
}

int32_t player_split_stero_data(int16_t *src, int16_t *dest, uint32_t samples)
{
    uint32_t k;
    
    for (k = 0; k < samples; k++)
    {
        src[k] = src[2*k];
        dest[k] = src[1+2*k];
    }

    return k;
}

void mic_pcm_upload_handler(struct thread_timer *ttimer, void *expiry_fn_arg)
{
    uint32_t size, length, bytes;
    s16_t *main_mic_data, *vice_mic_data;

    size = MICDATA_PACKET_SIZE;

    if (g_mic_upload_ctx->mic_exchange)
    {
        main_mic_data = (s16_t *)g_mic_upload_ctx->micdata_ref;
        vice_mic_data = (s16_t *)g_mic_upload_ctx->micdata_main;
    }
    else
    {
        main_mic_data = (s16_t *)g_mic_upload_ctx->micdata_main;
        vice_mic_data = (s16_t *)g_mic_upload_ctx->micdata_ref;
    }

    bytes = stream_get_length(g_mic_upload_ctx->audio_stream);
    if ((bytes + g_mic_upload_ctx->offset) >= size * 2)
    {
        SYS_LOG_DBG("offset %d, size %d", g_mic_upload_ctx->offset, bytes);

        bytes = size * 2 - g_mic_upload_ctx->offset;
        audio_record_read(g_mic_upload_ctx->audio_record,
            (u8_t*)g_mic_upload_ctx->micdata_main + g_mic_upload_ctx->offset, bytes);
        g_mic_upload_ctx->offset = 0;

        player_split_stero_data((int16_t *)g_mic_upload_ctx->micdata_main,
            (int16_t *)g_mic_upload_ctx->micdata_ref, size / 2);

        if (g_mic_upload_ctx->channel_bitmap & 0x01)
        {
            /* send main MIC data
                */
            g_mic_upload_ctx->pcm_frame_head->channel = 0;
            memcpy(g_mic_upload_ctx->pcm_frame_head->data, main_mic_data, size);
            length = spp_test_backend_write((u8_t*)g_mic_upload_ctx->pcm_frame_head, size + sizeof(mic_pcm_frame_info_t), 2000);
            if ((size + sizeof(mic_pcm_frame_info_t)) != length)
            {
                SYS_LOG_ERR("%d Fail to upload mic0 data(%dB)", __LINE__, length);
            }
        }

        if (g_mic_upload_ctx->channel_bitmap & 0x02)
        {
            /* send vice MIC data
                */
            g_mic_upload_ctx->pcm_frame_head->channel = 1;
            memcpy(g_mic_upload_ctx->pcm_frame_head->data, vice_mic_data, size);
            length = spp_test_backend_write((u8_t*)g_mic_upload_ctx->pcm_frame_head, size + sizeof(mic_pcm_frame_info_t), 2000);
            if ((size + sizeof(mic_pcm_frame_info_t)) != length)
            {
                SYS_LOG_ERR("%d Fail to upload mic1 data(%dB)",__LINE__, length);
            }
        }

        if (g_mic_upload_ctx->channel_bitmap & 0x04)
        {
            g_mic_upload_ctx->pcm_frame_head->channel = 2;
            memcpy(g_mic_upload_ctx->pcm_frame_head->data, main_mic_data, size);
            length = spp_test_backend_write((u8_t*)g_mic_upload_ctx->pcm_frame_head, size + sizeof(mic_pcm_frame_info_t), 2000);
            if ((size + sizeof(mic_pcm_frame_info_t)) != length)
            {
                SYS_LOG_ERR("%d Fail to upload mic0 data(%dB)",__LINE__, length);
    		}
        }
        if (g_mic_upload_ctx->channel_bitmap & 0x08)
       {
            g_mic_upload_ctx->pcm_frame_head->channel = 3;
            memcpy(g_mic_upload_ctx->pcm_frame_head->data, vice_mic_data, size);
            length = spp_test_backend_write((u8_t*)g_mic_upload_ctx->pcm_frame_head, size + sizeof(mic_pcm_frame_info_t), 2000);
            if ((size + sizeof(mic_pcm_frame_info_t)) != length)
            {
                SYS_LOG_ERR("%d Fail to upload mic1 data(%dB)", __LINE__, length);
            }
        }
        g_mic_upload_ctx->pcm_frame_head->sequence++;
    } else if (bytes > 0) {
        audio_record_read(g_mic_upload_ctx->audio_record,
            (u8_t*)g_mic_upload_ctx->micdata_main + g_mic_upload_ctx->offset, bytes);
        g_mic_upload_ctx->offset += bytes;
    }
}

