/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file media player interface
 */

#define SYS_LOG_DOMAIN "media"
#include <os_common_api.h>
#include <sys/byteorder.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <srv_manager.h>
#include <sys_wakelock.h>
#include <string.h>
#include "audio_policy.h"
#include "audio_system.h"
#include "media_player.h"
#include "media_service.h"
#include "media_mem.h"
#include <file_stream.h>

#include "acts_ringbuf.h"
#include "stream.h"
#include <audio_track.h>
#include <sdfs.h>
#include <buffer_stream.h>



struct mix_pcm_manager_t {
	io_stream_t mix_pcm_stream;
	uint8_t mix_pcm_cnt;
	uint8_t mix_track_finish;
	struct audio_track_t *mix_pcm_track;
	io_stream_t mix_track_stream;
	os_delayed_work mix_track_work;
    char mix_pcm_name[20];
    media_player_t *play_handle;
    mix_service_param_t param;
};

static struct mix_pcm_manager_t mix_pcm_context = {0};

static void _mix_pcm_track_callback(uint8_t event, void *user_data)
{

}

static void _media_service_mix_callback(io_stream_t mix_stream, int result, void *reply)
{
    struct mix_pcm_manager_t *ctx = &mix_pcm_context;
    if (mix_stream != ctx->mix_pcm_stream) {
        SYS_LOG_ERR("mix_stream %p %p err \n", mix_stream, ctx->mix_pcm_stream);
        return;
    }

    stream_close(ctx->mix_pcm_stream);
    stream_destroy(ctx->mix_pcm_stream);
    ctx->mix_pcm_stream = NULL;

    ctx->mix_pcm_cnt--;
	SYS_LOG_INF("%s %d close\n", __FUNCTION__, __LINE__);
}

static void mix_instream_read_notify(void *observer, int readoff, int writeoff,
			int total_size, unsigned char *buf, int num, stream_notify_type type)
{
	struct mix_pcm_manager_t *ctx = &mix_pcm_context;

	if (stream_get_length(ctx->mix_pcm_stream) <= 0) {
		//media_mix_pcm_stream_close();

		ctx->mix_track_finish = 1;

        SYS_LOG_INF("%s %d %d mix_track_finish:%d\n", __FUNCTION__, __LINE__, stream_get_length(ctx->mix_pcm_stream), ctx->mix_track_finish);
	}
}

static io_stream_t open_file_stream(const char *url, int mode)
{
	io_stream_t stream = file_stream_create((void*)url);

	if (!stream)
		return NULL;

	if (stream_open(stream, mode)) {
		stream_destroy(stream);
		return NULL;
	}

	return stream;
}

static char mix_tmp_data[512];
static void _media_mix_pcm_put_data_work(os_work *work)
{
	struct mix_pcm_manager_t *ctx = &mix_pcm_context;
	char channel = audio_policy_get_out_channel_type(AUDIO_STREAM_TTS) == AUDIO_CHANNEL_I2STX? 2:1;

	if (!ctx->mix_track_finish) {
	    int mix_len = stream_get_space(ctx->mix_track_stream);
	    if (mix_len > 512 / channel)
	        mix_len = 512 / channel;

	    if (mix_len > stream_get_length(ctx->mix_pcm_stream))
	        mix_len = stream_get_length(ctx->mix_pcm_stream);

    	if (mix_len > 0) {
    	    if (channel == 1) { //likely
                stream_read(ctx->mix_pcm_stream, mix_tmp_data, mix_len);
                stream_write(ctx->mix_track_stream, mix_tmp_data, mix_len);
            } else {
                short *src = (short *)(&mix_tmp_data[mix_len]);
                short *dest = (short *)mix_tmp_data;
                stream_read(ctx->mix_pcm_stream, src, mix_len);
                for (int i = 0; i < mix_len / 2; i++) {
                    *dest++ = *src;
                    *dest++ = *src++;
                }
                stream_write(ctx->mix_track_stream, mix_tmp_data, mix_len * channel);
            }
    	}

        //512 byte data case 16K 16ms
        os_delayed_work_submit(&ctx->mix_track_work, 4);
    } else {

	SYS_LOG_INF("%s %d close\n", __FUNCTION__, __LINE__);
        media_mix_pcm_stream_close();
    }

}

int media_mix_pcm_stream_open(const char *url, int inrate, int outrate)
{
	struct mix_pcm_manager_t *ctx = &mix_pcm_context;
	int ret = 0;

    if (ctx->mix_pcm_cnt) {
        SYS_LOG_ERR("mix pcm is running %d\n", ctx->mix_pcm_cnt);
        return -1;
    }

    ctx->mix_pcm_cnt++;

	strcpy(ctx->mix_pcm_name, url);

    ctx->mix_pcm_stream = open_file_stream(ctx->mix_pcm_name, MODE_IN);
    if (!ctx->mix_pcm_stream) {
        SYS_LOG_ERR("stream open failed (%s)\n", ctx->mix_pcm_name);
        goto err_exit;
    }

    media_player_t *player = media_player_get_current_dumpable_player();
    if (player) {
        mix_service_param_t param;
        memset(&param, 0, sizeof(mix_service_param_t));

    	param.in_rate = inrate;
    	param.out_rate = outrate;
    	param.master_db = -6;
    	param.mix_db = -1;
    	param.channels = 1;
    	param.format = PCM_TYPE;
    	param.input_stream = ctx->mix_pcm_stream;
    	param.callback = _media_service_mix_callback;

    	ret = media_player_set_mix_stream(player, &param);
    	if (ret) {
    		SYS_LOG_ERR("set mix stream err:%d\n", ret);
    		goto err_exit;
    	}

        ctx->play_handle = player;
    } else {
        audio_system_mutex_lock();

		struct audio_track_t * mix_pcm_track = audio_track_create(AUDIO_STREAM_TTS, inrate,
									 AUDIO_FORMAT_PCM_16_BIT, AUDIO_MODE_MONO,
									 NULL,
									_mix_pcm_track_callback, ctx);

		if (!mix_pcm_track) {
			goto err_exit;
		}

        ctx->mix_pcm_track = mix_pcm_track;
        ctx->mix_track_stream = audio_track_get_stream(ctx->mix_pcm_track);

        stream_set_observer(ctx->mix_pcm_stream, ctx, mix_instream_read_notify, STREAM_NOTIFY_READ);
        //create workq put data to audio track
        os_delayed_work_init(&ctx->mix_track_work, _media_mix_pcm_put_data_work);
        os_delayed_work_submit(&ctx->mix_track_work, OS_NO_WAIT);

        os_sleep(10);
		audio_track_start(mix_pcm_track);

        audio_system_mutex_unlock();

    }

	return 0;

err_exit:
	if (ctx->mix_pcm_stream) {
		stream_close(ctx->mix_pcm_stream);
		stream_destroy(ctx->mix_pcm_stream);
		ctx->mix_pcm_stream = NULL;
	}
	ctx->mix_pcm_cnt--;
	return 0;

}


int media_mix_pcm_stream_close(void)
{
	struct mix_pcm_manager_t *ctx = &mix_pcm_context;

    if (!ctx->mix_pcm_cnt) {
        SYS_LOG_ERR("mix pcm no running %d\n", ctx->mix_pcm_cnt);
        return -1;
    }

	if (ctx->mix_pcm_track) {
    	ctx->mix_track_finish = 0;
        os_delayed_work_cancel(&ctx->mix_track_work);

	    audio_system_mutex_lock();
		audio_track_flush(ctx->mix_pcm_track);
		audio_track_stop(ctx->mix_pcm_track);
		audio_track_destory(ctx->mix_pcm_track);
		ctx->mix_pcm_track = NULL;

    	ctx->mix_pcm_track = NULL;

    	ctx->mix_pcm_cnt--;
    	stream_close(ctx->mix_pcm_stream);
        stream_destroy(ctx->mix_pcm_stream);
        audio_system_mutex_unlock();
	} else {
	    media_player_t *player = media_player_get_current_dumpable_player();
	    if (player != ctx->play_handle) {
            SYS_LOG_ERR("mix_stream %p %p err \n", player, ctx->play_handle);
            return -1;
	    }

	    int ret = media_player_set_mix_stream(ctx->play_handle, NULL);
    	if (ret) {
    		SYS_LOG_ERR("set mix stream err:%d\n", ret);
            return 0;
        }
    }

    return 0;

}


