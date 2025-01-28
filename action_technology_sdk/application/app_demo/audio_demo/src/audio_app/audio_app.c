/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file key tone
 */

#include <app_defines.h>
#include "audio_app.h"

#define RECORD_TIMER_PERIOD 2
#define PLAY_TIMER_PERIOD 2
#define MICBYPASS_TIMER_PERIOD 2
#define RECORD_MAX_TIME 30*1000 /* 30 s */

t_audio_app audio_app;
static char cache_buffer[1024 * 4];
static char record_pcm_buffer[1024 * 8];
static char play_pcm_buffer[1024 * 8];

static void record_timer_handle(struct thread_timer *ttimer, void *expiry_fn_arg);
static void play_timer_handle(struct thread_timer *ttimer, void *expiry_fn_arg);
static void mic_bypass_timer_handle(struct thread_timer *ttimer, void *expiry_fn_arg);

/* Initializes the related variables. */
static void audio_app_init(t_audio_app *handle)
{
	handle->cache_buffer = cache_buffer;
	handle->cache_buffer_size = sizeof(cache_buffer);
	handle->cur_vol = 8;
	handle->player_state = PLAYER_IDLE;
	thread_timer_init(&handle->record_timer, record_timer_handle, handle);
	thread_timer_init(&handle->play_timer, play_timer_handle, handle);
	thread_timer_init(&handle->mic_bypass_timer, mic_bypass_timer_handle, handle);
}

/* Record timer handle function. */
static void record_timer_handle(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	t_audio_app *p_audio_app = (t_audio_app *)expiry_fn_arg;
	io_stream_t audio_stream;
	int ret;

	if (!p_audio_app || p_audio_app->player_state != PLAYER_RECORDING) {
		return;
	}

	audio_stream = p_audio_app->mic_recoder->audio_stream;
	/* Read data from audio stream,and write to file. */
	if (stream_get_length(audio_stream) >= p_audio_app->cache_buffer_size) {
		ret = stream_read(audio_stream, p_audio_app->cache_buffer, p_audio_app->cache_buffer_size);
		if (ret > 0) {
			ret = fs_write(&p_audio_app->pcm_file, (void *)p_audio_app->cache_buffer, ret);
		}
	}
	/* stop record if timeout. */
	if (os_uptime_get_32() - p_audio_app->start_record_time > RECORD_MAX_TIME) {
		SYS_LOG_INF("The recording time has exceeded the maximum value,stop record...");
		record_stop();
	}
}

/* Record start function. */
static void app_record_start(t_audio_app *p_audio_app, const char *url)
{
	int ret;

	if (!p_audio_app || p_audio_app->player_state != PLAYER_IDLE) {
		return;
	}

	ret = fs_open(&p_audio_app->pcm_file, url, FS_O_CREATE | FS_O_RDWR);
	if (ret) {
		SYS_LOG_ERR("file open failed %s !!", url);
		return;
	}
	
	p_audio_app->mic_recoder = mic_recorder_open(record_pcm_buffer, sizeof(record_pcm_buffer));
	if (!p_audio_app->mic_recoder) {
		goto err_exit;
	}

	ret = mic_recorder_start(p_audio_app->mic_recoder);
	if (ret) {
		goto err_exit;
	}

	thread_timer_start(&p_audio_app->record_timer, RECORD_TIMER_PERIOD, RECORD_TIMER_PERIOD);
	p_audio_app->player_state = PLAYER_RECORDING;
	p_audio_app->start_record_time = os_uptime_get_32();
	return;

err_exit:
	fs_close(&p_audio_app->pcm_file);
}

/* Record stop function. */
static void app_record_stop(t_audio_app *p_audio_app)
{
	int file_offset;
	int file_size;

	thread_timer_stop(&p_audio_app->record_timer);
	mic_recorder_stop(p_audio_app->mic_recoder);
	mic_recorder_close(p_audio_app->mic_recoder);
	/* Save file's data.Truncate the file if it is smaller than before. */
	file_offset =  fs_tell(&p_audio_app->pcm_file);
	SYS_LOG_INF("file offset:%d", file_offset);
	fs_seek(&p_audio_app->pcm_file, 0, FS_SEEK_END);
	file_size =  fs_tell(&p_audio_app->pcm_file);
	SYS_LOG_INF("file size:%d", file_size);
	if (file_offset < file_size)
		fs_truncate(&p_audio_app->pcm_file, file_offset);
	fs_close(&p_audio_app->pcm_file);

	SYS_LOG_INF("record finished ----------\n");
	p_audio_app->player_state = PLAYER_IDLE;
}

/* Play timer handle function. */
static void play_timer_handle(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	t_audio_app *p_audio_app = (t_audio_app *)expiry_fn_arg;
	io_stream_t audio_stream;
	int ret;

	if (!p_audio_app || p_audio_app->player_state != PLAYER_PLAYING) {
		return;
	}

	audio_stream = p_audio_app->player->audio_stream;
	/* Read data from file,and write to audio stream.
	 * If read to the end of file or exception occurs,stop playback.
	 */
	if (stream_get_space(audio_stream) >= p_audio_app->cache_buffer_size) {
		ret = fs_read(&p_audio_app->pcm_file, (void *)p_audio_app->cache_buffer, p_audio_app->cache_buffer_size);
		if (ret > 0) {
			stream_write(audio_stream, p_audio_app->cache_buffer, ret);
		} else {
			play_stop();
		}
	}
}

/* Play start function. */
static void app_play_start(t_audio_app *p_audio_app, const char *url)
{
	int ret;

	if (!p_audio_app || p_audio_app->player_state != PLAYER_IDLE) {
		return;
	}

	ret = fs_open(&p_audio_app->pcm_file, url, FS_O_READ);
	if (ret) {
		SYS_LOG_ERR("file open failed %s !!\n", url);
		return;
	}

	p_audio_app->player = pcm_player_open(play_pcm_buffer, sizeof(play_pcm_buffer));
	if (!p_audio_app->player) {
		goto err_exit;
	}

	pcm_player_set_volume(p_audio_app->player,  p_audio_app->cur_vol);
	ret = pcm_player_start(p_audio_app->player);
	if (ret) {
		goto err_exit;
	}

	thread_timer_start(&p_audio_app->play_timer, PLAY_TIMER_PERIOD, PLAY_TIMER_PERIOD);
	p_audio_app->player_state = PLAYER_PLAYING;
	return;

err_exit:
	fs_close(&p_audio_app->pcm_file);
}

/* Play stop function. */
static void app_play_stop(t_audio_app *p_audio_app)
{
	thread_timer_stop(&p_audio_app->play_timer);
	pcm_player_stop(p_audio_app->player);
	pcm_player_close(p_audio_app->player);
	fs_close(&p_audio_app->pcm_file);
	p_audio_app->player_state = PLAYER_IDLE;
}

/* mic_bypass handle function. */
static void mic_bypass_timer_handle(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	t_audio_app *p_audio_app = (t_audio_app *)expiry_fn_arg;
	io_stream_t in_stream;
	io_stream_t out_stream;
	int ret;

	if (!p_audio_app || p_audio_app->player_state != PLAYER_MIC_BYPASS) {
		return;
	}

	in_stream = p_audio_app->mic_recoder->audio_stream;
	out_stream = p_audio_app->player->audio_stream;
	/* Read data from input audio stream,and write to ouput audio stream. */
	if (stream_get_space(out_stream) >= p_audio_app->cache_buffer_size) {
		if (stream_get_length(in_stream) >= p_audio_app->cache_buffer_size) {
			ret = stream_read(in_stream, p_audio_app->cache_buffer, p_audio_app->cache_buffer_size);
			if (ret > 0) {
				stream_write(out_stream, p_audio_app->cache_buffer, ret);
			}
		}
	}
}

/* mic_bypass start function. */
static void app_mic_bypass_start(t_audio_app *p_audio_app)
{
	if (!p_audio_app || p_audio_app->player_state != PLAYER_IDLE) {
		return;
	}

	p_audio_app->mic_recoder = mic_recorder_open(record_pcm_buffer, sizeof(record_pcm_buffer));
	if (!p_audio_app->mic_recoder) {
		return;
	}

	p_audio_app->player = pcm_player_open(play_pcm_buffer, sizeof(play_pcm_buffer));
	if (!p_audio_app->player) {
		return;
	}

	mic_recorder_start(p_audio_app->mic_recoder);
	pcm_player_set_volume(p_audio_app->player,  p_audio_app->cur_vol);
	pcm_player_start(p_audio_app->player);
	thread_timer_start(&p_audio_app->mic_bypass_timer, MICBYPASS_TIMER_PERIOD, MICBYPASS_TIMER_PERIOD);
	p_audio_app->player_state = PLAYER_MIC_BYPASS;
}

/* mic_bypass stop function. */
static void app_mic_bypass_stop(t_audio_app *p_audio_app)
{
	thread_timer_stop(&p_audio_app->mic_bypass_timer);
	pcm_player_stop(p_audio_app->player);
	pcm_player_close(p_audio_app->player);
	mic_recorder_stop(p_audio_app->mic_recoder);
	mic_recorder_close(p_audio_app->mic_recoder);
	p_audio_app->player_state = PLAYER_IDLE;
}

void record_start(char *url)
{
	struct app_msg msg = {0};

	msg.cmd = RECORD_START;
	msg.ptr = url;
	send_async_msg(APP_ID_AUDIO_APP, &msg);
}

void record_stop(void)
{
	struct app_msg msg = {0};

	msg.cmd = RECORD_STOP;
	send_async_msg(APP_ID_AUDIO_APP, &msg);
}

void play_start(char *url)
{
	struct app_msg msg = {0};

	msg.cmd = PLAY_START;
	msg.ptr = url;
	send_async_msg(APP_ID_AUDIO_APP, &msg);
}

void play_stop(void)
{
	struct app_msg msg = {0};

	msg.cmd = PLAY_STOP;
	send_async_msg(APP_ID_AUDIO_APP, &msg);
}


void mic_bypass_start(void)
{
	struct app_msg msg = {0};

	msg.cmd = MIC_BYPASS_START;
	send_async_msg(APP_ID_AUDIO_APP, &msg);
}

void mic_bypass_stop(void)
{
	struct app_msg msg = {0};

	msg.cmd = MIC_BYPASS_STOP;
	send_async_msg(APP_ID_AUDIO_APP, &msg);
}

static void audio_app_task(void *p1, void *p2, void *p3)
{
	t_audio_app *p_audio_app = &audio_app;
	struct app_msg msg = {0};
	const char *url = NULL;

	audio_app_init(p_audio_app);

	while (1) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			switch (msg.cmd) {
			case RECORD_START:
				url = (const char *)msg.ptr;
				app_record_start(p_audio_app, url);
				break;


			case RECORD_STOP:
				app_record_stop(p_audio_app);
				break;

			case PLAY_START:
				url = (const char *)msg.ptr;
				app_play_start(p_audio_app, url);
				break;


			case PLAY_STOP:
				app_play_stop(p_audio_app);
				break;

			case MIC_BYPASS_START:
				app_mic_bypass_start(p_audio_app);
				break;


			case MIC_BYPASS_STOP:
				app_mic_bypass_stop(p_audio_app);
				break;

			default:
				break;
			}

			if (msg.callback)
				msg.callback(&msg, 0, NULL);

		}

		thread_timer_handle_expired();
	}

}
APP_DEFINE(audio_app, share_stack_area, sizeof(share_stack_area),
		CONFIG_APP_PRIORITY, DEFAULT_APP | FOREGROUND_APP, NULL, NULL, NULL,
	   audio_app_task, NULL);



