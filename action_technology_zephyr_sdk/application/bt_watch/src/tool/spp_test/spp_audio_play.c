/**
 *  ***************************************************************************
 *  Copyright (c) 2003-2021 Actions Semiconductor. All rights reserved.
 *
 *  \file       spp_audio_play.c
 *  \brief      play audio data that read through spp
 *  \author     zhouxl
 *  \version    1.00
 *  \date       2021-1-6
 *  ***************************************************************************
 *
 *  \History:
 *  Version 1.00
 *       Initial release
 */

#include "abtp_inner.h"
#include <audio_track.h>

#define AUDIO_PLAY_THRESHHOLD_SIZE      (256)
// audio data fill timer, MUST less than speaker consume time
#define AUDIO_PLAY_TIMER_MS             (AUDIO_PLAY_THRESHHOLD_SIZE / 8 / 4)

typedef struct
{
    s16_t *pcm_data;
    u32_t size;
    u32_t offset;

    u32_t sample_rate;

    // struct hrtimer timer;
    struct thread_timer ttimer;
    struct audio_track_t *audio_track;
    io_stream_t audio_stream;
} audio_output_context_t;

audio_output_context_t* audio_play_test;

int32_t player_convert_stero(int16_t *dest, int16_t *src, int32_t sample_cnt)
{
    int32_t i;

    for (i = sample_cnt - 1; i >= 0; i--)
    {
        *(dest + i * 2) = *(src + i);
        *(dest + i * 2 + 1) = *(src + i);
    }

    return sample_cnt;
}

// void audio_data_play_test_callback(struct hrtimer *timer, void *expiry_fn_arg)
void audio_data_play_test_callback(struct thread_timer *ttimer, void *expiry_fn_arg)
{
    int32_t write_bytes, free_bytes, offset, time_ms;

    free_bytes = stream_get_space(audio_play_test->audio_stream);
        offset = audio_play_test->offset;
    if ((offset + free_bytes) >= audio_play_test->size)
        {
            write_bytes = audio_play_test->size - offset;
            audio_play_test->offset = 0;
        time_ms = 1;
        }
        else
        {
        audio_play_test->offset += free_bytes;
        write_bytes = free_bytes;
        time_ms = AUDIO_PLAY_TIMER_MS;
        }

        // SYS_LOG_DBG("free %d, write %d", free_bytes, write_bytes);
        audio_track_write(audio_play_test->audio_track, (u8_t*)audio_play_test->pcm_data + offset, write_bytes);
    // hrtimer_start(timer, time_ms, 0);
}

void audio_data_play_test_start(s16_t *pcm_data, u32_t size, u32_t sample_rate)
{
	struct audio_track_t *audio_track;

    SYS_LOG_INF("");

    audio_play_test = (audio_output_context_t*)app_mem_malloc(sizeof(audio_output_context_t));
	if (!audio_play_test) return;
    memset(audio_play_test, 0, sizeof(audio_output_context_t));

    audio_play_test->pcm_data = pcm_data;
    audio_play_test->size = size;
    audio_play_test->sample_rate = sample_rate;

    audio_track = audio_track_create(AUDIO_STREAM_VOICE, sample_rate / 1000,
        AUDIO_FORMAT_PCM_16_BIT, AUDIO_MODE_MONO, NULL, NULL, NULL);

    if (!audio_track)
    {
        SYS_LOG_ERR("failed");
    }
    else
    {
        audio_play_test->audio_track = audio_track;
        audio_play_test->audio_stream = audio_track_get_stream(audio_track);
        SYS_LOG_INF("free %d", stream_get_space(audio_play_test->audio_stream));
        audio_track_start(audio_track);

        thread_timer_init(&audio_play_test->ttimer, audio_data_play_test_callback, NULL);
        thread_timer_start(&audio_play_test->ttimer, 2, 2);
    }
}

void audio_data_play_test_stop(void)
{
    SYS_LOG_INF("");
    if (audio_play_test)
    {
        if (audio_play_test->audio_track)
        {
            thread_timer_stop(&audio_play_test->ttimer);
            audio_track_stop(audio_play_test->audio_track);
            audio_track_destory(audio_play_test->audio_track);
        }

        app_mem_free(audio_play_test);
        audio_play_test = NULL;
    }
}

