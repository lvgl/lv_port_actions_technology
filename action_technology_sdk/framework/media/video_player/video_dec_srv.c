/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file video_dec_srv.c
 *
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <msg_manager.h>
#include <thread_timer.h>
#include "srv_manager.h"
#include <os_common_api.h>
#include "video_player.h"
#include "video_player_defs.h"
#include "audio_system.h"
#include "audio_track.h"
#include "vp_engine.h"
#include "dvfs.h"
#include <sys_wakelock.h>
#include <soc_dsp.h>
#include <file_stream.h>
#include <video_mem.h>
#include <ui_mem.h>
#ifdef CONFIG_DVFS
#include "dvfs.h"
#endif
#include "acts_ringbuf.h"

#define AUDIO_WR_THRESHOLD		(4*1024)
#define AUDIO_TRACK_BUG_SIZE	(1024 + ADPCM_MAX_FRAME_SIZE *2)

#define AUD_TRACK_BUF_PSRAM

#ifdef AUD_TRACK_BUF_PSRAM
static char __aligned(4) audio_track_buf[AUDIO_TRACK_BUG_SIZE] __in_section_unique(VIDEO_PSRAM_REGION);
#endif

video_player_data_t *video_player_data = NULL;
extern unsigned int Samples_per_sec;

static uint32_t t_begin = 0;
static int frame_num = 0;
static int video_dec_stop(video_player_data_t * data, bool mem_release);
static void calculate_fps_check(void);

#define VIDEO_DEC_WORKQ_STACKSIZE	(1024*2)
struct k_work_q video_dec_workq;
static uint8_t video_dec_workq_stack[VIDEO_DEC_WORKQ_STACKSIZE] __aligned(4);
static OS_SEM_DEFINE(video_sem, 0, 1);
static char video_frame_buf[sizeof(frame_info_t) * 2];
static void video_dec_work_handle(struct k_work *work);
static K_WORK_DEFINE(video_dec_work, video_dec_work_handle);
static int video_dec_parse_frame(frame_info_t *frame_info);
//load next frame when decoding
static void video_dec_work_handle(struct k_work *work)
{
	video_player_data_t *data = video_player_data;
	frame_info_t *frame_info;
	uint32_t size;
	packet_type_t packet_type;

	while (acts_ringbuf_space(data->frame_ringbuf) >= sizeof(frame_info_t)) {
		frame_info = video_mem_malloc(sizeof(frame_info_t));
		if (frame_info == NULL) {
			SYS_LOG_ERR("alloc failed");
			break;
		}
		video_dec_parse_frame(frame_info);
		packet_type = frame_info->packet_type;
		size = acts_ringbuf_put(data->frame_ringbuf, frame_info, sizeof(frame_info_t));
		video_mem_free(frame_info);
		if (packet_type == VIDEO_ES)
			break;
	}
	os_sem_give(&video_sem);
}

static int video_dec_parse_frame(frame_info_t *frame_info)
{
	video_player_data_t *data = video_player_data;
	int ret = EN_NORMAL;

	av_buf_t *av_dec_buf = &frame_info->av_dec_buf;
	demuxer_packet_t *packet_buf = &data->packet_buf;

	if (!packet_buf->data) {
		return EN_MEMERR;
	}

	memset(packet_buf->data, 0, MAX_PACKET_LEN);
	ret = data->dem_plugin->getframe(data->dem_handle, packet_buf);
	frame_info->pts = packet_buf->pts;
	frame_info->packet_type = packet_buf->packet_type;
	memset(av_dec_buf, 0, sizeof(av_buf_t));

	if (ret != EN_NORMAL)
	{
		if(ret == EN_FILEISEND)
		{
			SYS_LOG_INF("### FILE END.");
			frame_info->status = FRAME_END;
			return ret;
		}
		else if(ret == EN_FILESTARTPOS)
		{
			SYS_LOG_INF("### FILE STARTPOS.");
			frame_info->status = FRAME_ERROR;
			return ret;
		}
		else
		{
			SYS_LOG_ERR("get frame failed! ret %d", ret);
			frame_info->status = FRAME_ERROR;
			return ret;
		}
	}

	av_dec_buf->data_len = packet_buf->data_len;
	av_dec_buf->src_para.w = data->video_info.width;
	av_dec_buf->src_para.h = data->video_info.height;
	av_dec_buf->out_para.w = data->video_info.width;
	av_dec_buf->out_para.h = data->video_info.height;
	av_dec_buf->file_stream = data->file_stream;

	// video
	if(packet_buf->packet_type == VIDEO_ES)
	{
		av_dec_buf->data = data->init_param.alloc(av_dec_buf->data_len);
		if (!av_dec_buf->data) {
			SYS_LOG_WRN("buf alloc %d fail", av_dec_buf->data_len);
			return EN_MEMERR;
		}

		stream_read(av_dec_buf->file_stream, av_dec_buf->data, av_dec_buf->data_len);
	}
	// audio
	else if(packet_buf->packet_type == AUDIO_ES)
	{
		av_dec_buf->data = data->init_param.alloc(av_dec_buf->data_len);
		if (!av_dec_buf->data) {
			SYS_LOG_WRN("buf alloc %d fail", av_dec_buf->data_len);
			return EN_MEMERR;
		}
		memcpy(av_dec_buf->data, packet_buf->data, av_dec_buf->data_len);
	}

	frame_info->status = FRAME_NORMAL;
	return EN_NORMAL;
}

static void calculate_fps_begin(void)
{
	if(frame_num == 0)
		t_begin = k_cycle_get_32();
}

static void calculate_fps_check(void)
{
	uint32_t total_time = 0;

	frame_num ++;
	total_time = k_cyc_to_us_floor32(k_cycle_get_32() - t_begin);
	if(total_time >= 1000000)
	{
		SYS_LOG_INF("time: %dus, frame: %d", total_time, frame_num);
		frame_num = 0;
		t_begin = 0;
	}
}

static int video_audio_open(video_player_data_t *data)
{
	int audio_mode = AUDIO_MODE_MONO;	//AUDIO_MODE_STEREO;

	dsp_powergate_enable();
#ifdef AUD_TRACK_BUF_PSRAM
	data->aud_ringbuf = video_mem_malloc(sizeof(struct acts_ringbuf));
	if (data->aud_ringbuf == NULL)
		goto err_out;

	acts_ringbuf_init(data->aud_ringbuf, audio_track_buf, AUDIO_TRACK_BUG_SIZE);
#else
	data->aud_ringbuf = acts_ringbuf_alloc(AUDIO_TRACK_BUG_SIZE);
	if (data->aud_ringbuf == NULL)
	{
		goto err_out;
	}
#endif
    SYS_LOG_INF("sample rate: %d\n", Samples_per_sec);
	data->aud_track = audio_track_create(AUDIO_STREAM_LOCAL_MUSIC,
    	Samples_per_sec/1000, AUDIO_FORMAT_PCM_16_BIT, audio_mode, data->aud_ringbuf, NULL, NULL);
	if (data->aud_track == NULL)
	{
		goto err_out;
	}

	data->audio_stream = audio_track_get_stream(data->aud_track);

	audio_track_start(data->aud_track);
	SYS_LOG_INF("### data->mute %d", data->mute);
	audio_track_mute(data->aud_track, data->mute);
	audio_track_set_fade_in(data->aud_track, 10);
    //bt_transmit_catpure_start(audio_track_get_stream(data->aud_track),
    //   Samples_per_sec/1000, 1, AUDIO_STREAM_VIDEO);
    return 0;
err_out:
	if(data->aud_ringbuf)
	{
#ifdef AUD_TRACK_BUF_PSRAM
		video_mem_free(data->aud_ringbuf);
#else
		acts_ringbuf_free(data->aud_ringbuf);
#endif
		data->aud_ringbuf = NULL;
	}

	if(data->aud_track)
	{
		audio_track_destory(data->aud_track);
		data->aud_track = NULL;
		dsp_powergate_disable();
	}

	SYS_LOG_INF("error --");
	return -1;
}

static void video_audio_close(video_player_data_t *data)
{
	if (data->aud_track) {
		audio_track_set_fade_out(data->aud_track, 10);
		os_sleep(10);
		//bt_transmit_catpure_stop(audio_track_get_stream(data->aud_track));
		//audio_track_pause(data->aud_track);

		audio_track_stop(data->aud_track);
		audio_track_destory(data->aud_track);
		data->aud_track = NULL;
		dsp_powergate_disable();
	}

	if(data->aud_ringbuf) {
#ifdef AUD_TRACK_BUF_PSRAM
		video_mem_free(data->aud_ringbuf);
#else
		acts_ringbuf_free(data->aud_ringbuf);
#endif
		data->aud_ringbuf= NULL;
	}
}


static int video_dec_init(video_player_data_t **vp_data)
{
	video_player_data_t *data;

	if(*vp_data)		// already open
	{
		SYS_LOG_ERR("error: video player already open!");
		return -1;
	}

	data = video_mem_malloc(sizeof(video_player_data_t));
	if(data == NULL)
	{
		SYS_LOG_ERR("malloc failed!");
		return -1;
	}

	memset(data, 0, sizeof(video_player_data_t));

	data->status = VP_STATUS_IDLE;

	data->packet_buf.data = video_mem_malloc(MAX_PACKET_LEN);

	if(data == NULL)
	{
		SYS_LOG_ERR("malloc failed!");
		goto err_out;
	}

	*vp_data = data;

	SYS_LOG_INF("ok");
	return 0;

err_out:
	video_mem_free(data);

	return -1;
}

static void video_dec_deinit(video_player_data_t *data)
{
	if (data == NULL)
		return;

	video_dec_stop(data, true);

	if(data->decode_buf) {
		data->init_param.free(data->decode_buf);
		data->decode_buf = NULL;
		data->decode_buf_size = 0;
	}

	if (data->file_stream) {
		stream_close(data->file_stream);
		stream_destroy(data->file_stream);
		data->file_stream = NULL;
	}

	if (data->packet_buf.data)
		video_mem_free(data->packet_buf.data);

	if (data)
		video_mem_free(data);

	SYS_LOG_INF("ok");
}

static int video_dec_set_file(video_player_data_t *data, char *url)
{
	unsigned char header_buf[512];
	video_data_t video_data = {0};
	int ret;
	io_stream_t file_stream = NULL;

	if (data->file_stream) {
		SYS_LOG_INF("file stream has esixt, please close file!");
		return -1;
	}

	file_stream = file_stream_create(url);
	if (!file_stream) {
		goto err_out;
	}

	ret = stream_open(file_stream, MODE_IN);
	if (ret < 0) {
		SYS_LOG_INF("stream_open error !");
		goto err_out;
	}

	ret = stream_read(file_stream, header_buf, 512);
	if (ret < 0) {
		SYS_LOG_INF("stream_read error !");
		goto err_out;
	}

	if ((header_buf[8] == 'A')&&(header_buf[9] == 'V')&&(header_buf[10] == 'I')&&(header_buf[11] == ' '))
	{
		SYS_LOG_INF("video format avi");
	}
	else
	{
		SYS_LOG_ERR("video format invalid!");
		goto err_out;
	}
	// seek to head
	stream_seek(file_stream, 0, SEEK_DIR_BEG);

	data->dem_plugin = avi_api();
	data->dec_plugin = mjpeg_api();
	data->wav_plugin = adpcm_api();
	if(data->dem_plugin==NULL || data->dec_plugin==NULL || data->wav_plugin==NULL)
	{
		goto err_out;
	}

	ve_media_info_t media_info;
	data->dem_handle = data->dem_plugin->open(file_stream, &media_info);
	if (data->dem_handle == NULL)
	{
		SYS_LOG_ERR("dem_plugin->open failed!");
		goto err_out;
	}
	SYS_LOG_INF("index_flag: %d", media_info.index_flag);
	SYS_LOG_INF("v_width: %d", media_info.video_info.width);
	SYS_LOG_INF("v_height: %d", media_info.video_info.height);
	SYS_LOG_INF("vtotal_time: %d", media_info.video_info.vtotal_time);
	SYS_LOG_INF("frame_rate: %d", media_info.video_info.frame_rate);

	if (media_info.video_info.width>VIDEO_PIXEL_WIDTH || media_info.video_info.height>VIDEO_PIXEL_HEIGHT)
	{
		SYS_LOG_ERR("video %dx%d not support!", media_info.video_info.width, media_info.video_info.height);
		goto err_out;
	}

	memcpy(&data->video_info, &media_info.video_info, sizeof(ve_video_info_t));

	data->dec_handle = data->dec_plugin->open(&data->video_info);
	if (data->dec_handle == NULL)
	{
		SYS_LOG_ERR("dec_plugin->open failed!");
		goto err_out;
	}

	data->wav_handle = data->wav_plugin->open(NULL);
	if (data->wav_handle == NULL)
	{
		SYS_LOG_ERR("wav_plugin->open failed!");
		goto err_out;
	}

	data->frame_ringbuf = acts_ringbuf_init_ext(video_frame_buf, sizeof(video_frame_buf));
	if (data->frame_ringbuf == NULL)
	{
		SYS_LOG_ERR("ringbuf init failed!");
		goto err_out;
	}

	if (Samples_per_sec == 0) {
		SYS_LOG_WRN("Invalid audio stream info");
		data->mute = 1;
	}

	SYS_LOG_INF("set file ok");
	data->file_stream = file_stream;
	video_data.height = media_info.video_info.height;
	video_data.width = media_info.video_info.width;
	return 0;

err_out:
	if (data->wav_handle) {
		data->wav_plugin->dispose(data->wav_handle);
		data->wav_handle = NULL;
	}
	if (data->dec_handle) {
		data->dec_plugin->dispose(data->dec_handle);
		data->dec_handle = NULL;
	}
	if (data->dem_handle) {
		data->dem_plugin->dispose(data->dem_handle);
		data->dem_handle = NULL;
	}
	if (file_stream) {
		stream_close(file_stream);
		stream_destroy(file_stream);
	}

	data->init_param.cb(VP_STATUS_ERROR, data->init_param.cb_data, NULL);

	return -1;
}

static int video_dec_play(video_player_data_t *data)
{
	if (data->dem_handle==NULL || data->dec_handle==NULL || data->wav_handle==NULL
		|| data->file_stream == NULL)
	{
		return -1;
	}

	if (data->status == VP_STATUS_PLAYING)
		return 0;

	if (!data->mute) {
		if(video_audio_open(data) != 0)
			return -1;
	}

	if(data->decode_buf == NULL) {
		data->decode_buf_size = data->video_info.height * data->video_info.width * 2;
		data->decode_buf = data->init_param.alloc(data->decode_buf_size);
	}

	data->start_time = os_uptime_get_32();
	data->status = VP_STATUS_PLAYING;

	return 0;
}

static int video_dec_stop(video_player_data_t *data, bool mem_release)
{
	if(data->status == VP_STATUS_STOPED)
		return 0;

	if (!data->mute) {
		video_audio_close(data);
	}

	if (data->dem_handle) {
		data->dem_plugin->dispose(data->dem_handle);
		data->dem_handle = NULL;
	}
	if (data->dec_handle) {
		data->dec_plugin->dispose(data->dec_handle);
		data->dec_handle = NULL;
	}
	if (data->wav_handle) {
		data->wav_plugin->dispose(data->wav_handle);
		data->wav_handle = NULL;
	}

	if (data->file_stream) {
		stream_close(data->file_stream);
		stream_destroy(data->file_stream);
		data->file_stream = NULL;
	}

	if(mem_release) {
		if(data->decode_buf) {
			data->init_param.free(data->decode_buf);
			data->decode_buf = NULL;
			data->decode_buf_size = 0;
		}
	}

	if (data->frame_ringbuf)
	{
		while (acts_ringbuf_length(data->frame_ringbuf) >= sizeof(frame_info_t)) {
			frame_info_t frame_info;
			acts_ringbuf_get(data->frame_ringbuf, &frame_info, sizeof(frame_info_t));
			if (frame_info.av_dec_buf.data) {
				data->init_param.free(frame_info.av_dec_buf.data);
			}
		}
		acts_ringbuf_destroy_ext(data->frame_ringbuf);
	}

	data->start_time = 0;
	data->cur_pts = 0;
	data->status = VP_STATUS_STOPED;

	if(data->freq_boot) {
		data->freq_boot = 0;
#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
		dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, VIDEODEC_SRV_NAME);
#endif
	}

	return 0;
}

static int video_dec_pause(video_player_data_t *data, bool mem_release)
{
	data->status = VP_STATUS_PAUSED;

	if (!data->mute) {
		video_audio_close(data);
	}

	data->init_param.cb(VP_STATUS_PAUSED, data->init_param.cb_data, NULL);
	if(mem_release && data->decode_buf) {
		data->init_param.free(data->decode_buf);
		data->decode_buf = NULL;
		data->decode_buf_size = 0;
	}

	if(data->freq_boot) {
		data->freq_boot = 0;
#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
		dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, VIDEODEC_SRV_NAME);
#endif
	}
	return 0;
}

static int video_dec_resume(video_player_data_t *data)
{
	if (!data->mute) {
		if(video_audio_open(data) != 0)
			return -1;
	}

	if(data->decode_buf == NULL) {
		data->decode_buf_size = data->video_info.height * data->video_info.width * 2;
		data->decode_buf = data->init_param.alloc(data->decode_buf_size);
	}

	data->start_time = os_uptime_get_32() - data->cur_pts;
	data->status = VP_STATUS_PLAYING;

	return 0;
}

static int video_dec_seek(video_player_data_t *data, uint32_t pos_ms)
{
	seek_info_t seek_info = {0};

	//bt_transmit_on_seek();
	seek_info.seek_cmd = SEEK_TIME;
	seek_info.curtime = pos_ms;
	data->dem_plugin->seek(data->dem_handle, &seek_info);

	return 0;
}

static int video_playing_handle(video_player_data_t *data)
{
	frame_info_t frame_info;
	av_buf_t *av_dec_buf;
	int ret = EN_NORMAL;
	int audio_len;
	video_data_t video_data;

	calculate_fps_begin();

	if(!data->freq_boot) {
		data->freq_boot = 1;
#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
		dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, VIDEODEC_SRV_NAME);
#endif
	}

	uint32_t size = acts_ringbuf_get(data->frame_ringbuf, &frame_info, sizeof(frame_info_t));
	if (size == 0) {
		video_dec_parse_frame(&frame_info);
	}

	data->cur_pts = frame_info.pts;

	if (frame_info.status == FRAME_END) {
		video_dec_stop(data, false);
		data->init_param.cb(VP_STATUS_STOPED, data->init_param.cb_data, NULL);
		if (data->init_param.repeat == true && data->user_stop == false) {
			video_dec_set_file(data, data->init_param.url);
			video_dec_play(data);
		}
		return EN_FILEISEND;
	} else if (frame_info.status == FRAME_ERROR) {
		video_dec_stop(data, false);
		return EN_DECERR;
	}

	av_dec_buf = &frame_info.av_dec_buf;
	// video
	if(frame_info.packet_type == VIDEO_ES)
	{
		av_dec_buf->outbuf = data->decode_buf;
		if(data->init_param.need_decode) {
			os_work_submit_to_queue(&video_dec_workq, &video_dec_work);
			ret = data->dec_plugin->decode(data->dec_handle, av_dec_buf);
			os_sem_take(&video_sem, OS_FOREVER);
			video_data.decode_buf_size = data->decode_buf_size;
		} else {
			memcpy(av_dec_buf->outbuf, av_dec_buf->data, av_dec_buf->data_len);
			video_data.decode_buf_size = av_dec_buf->data_len;
		}

		video_data.decode_buf = (char *)av_dec_buf->outbuf;
		video_data.width = data->video_info.width;
		video_data.height = data->video_info.height;
		video_data.format = 0;
		if(ret != EN_NORMAL)
		{
			SYS_LOG_ERR("dec_plugin.decode failed!");
			data->init_param.cb(VP_STATUS_ERROR, data->init_param.cb_data, &video_data);
		}
		else
		{
			if (data->mute && data->start_time) {
				uint32_t actual_pts = os_uptime_get_32() - data->start_time;
				SYS_LOG_DBG("cur_pts: %u, actual_pts: %u", data->cur_pts, actual_pts);
				if (data->cur_pts > actual_pts) {
					SYS_LOG_DBG("wait time: %u", data->cur_pts - actual_pts);
					os_sleep(data->cur_pts - actual_pts);
				}
			}
			data->init_param.cb(VP_STATUS_PLAYING, data->init_param.cb_data, &video_data);
		}

		calculate_fps_check();
	}
	// audio
	else if(frame_info.packet_type == AUDIO_ES)
	{
		if (!data->mute && data->aud_track) {
			int len;
	        av_dec_buf->outbuf = data->adpcm_outbuf;
			len = data->wav_plugin->decode(data->wav_handle, av_dec_buf);
			if (len < EN_NORMAL) {
				SYS_LOG_ERR("wav_plugin->decode failed!");
			} else {
				audio_len = stream_get_length(data->audio_stream);
				SYS_LOG_DBG("audio_len : %d", audio_len);
				if (audio_len >= AUDIO_WR_THRESHOLD)
				{
					int wait_time = (audio_len -AUDIO_WR_THRESHOLD)*1000/2/Samples_per_sec;
					if (wait_time>0 && wait_time<100) {
						SYS_LOG_DBG("wait_time : %d", wait_time);
						os_sleep(wait_time);
					} else {
						if (wait_time)
							SYS_LOG_WRN("### wait time: %d ms !!!", wait_time);
					}
				}

	            audio_track_write(data->aud_track, (unsigned char *)data->adpcm_outbuf, len);
			}
		}
	}

	if(av_dec_buf->data) {
		data->init_param.free(av_dec_buf->data);
	}

	return ret;
}

static int video_error_handle(void)
{
	return 0;
}


static void video_dec_handle(video_player_data_t *data)
{

	switch(data->status)
	{
		case VP_STATUS_PLAYING:
			video_playing_handle(data);
			break;

		case VP_STATUS_ERROR:
			video_error_handle();
			break;

		default:
			break;
	}
}

static void video_cmd_handle(video_player_data_t *data, struct app_msg *msg)
{
	int ret;

	if (data == NULL) {
		goto cmd_handle_finished;
	}

	switch(msg->cmd)
	{
		case CMD_VP_OPEN:
			memset(&data->init_param, 0, sizeof(video_init_param_t));
			memcpy(&data->init_param, msg->ptr, sizeof(video_init_param_t));
			SYS_LOG_INF("### CMD_VP_OPEN, url: %s", data->init_param.url);
			data->mute = data->init_param.mute;
			data->status = VP_STATUS_STOPED;
			data->user_stop = false;
			if (data->init_param.preload == true) {
				ret = video_dec_set_file(data, data->init_param.url);
				if (ret)
					break;

				if (data->decode_buf == NULL) {
					data->decode_buf_size = data->video_info.height * data->video_info.width * 2;
					data->decode_buf = data->init_param.alloc(data->decode_buf_size);
				}
				video_playing_handle(data);
				data->status = VP_STATUS_PRELOAD;
			}
			break;

		case CMD_VP_CLOSE:
			SYS_LOG_INF("### CMD_VP_CLOSE");

			if(data->freq_boot) {
				data->freq_boot = 0;
			#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
				dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, VIDEODEC_SRV_NAME);
			#endif
			}

			break;

		case CMD_VP_PLAY:
			SYS_LOG_INF("### CMD_VP_PLAY");
			data->user_stop = false;
			if (data->status == VP_STATUS_IDLE) {
				SYS_LOG_WRN("### video not open");
			} else if (data->status == VP_STATUS_PRELOAD) {
				video_dec_play(data);
			} else if (data->status == VP_STATUS_PAUSED) {
				video_dec_resume(data);
			} else if (data->status == VP_STATUS_STOPED) {
				video_dec_set_file(data, data->init_param.url);
				video_dec_play(data);
			}
			break;

		case CMD_VP_PAUSE:
			SYS_LOG_INF("### CMD_VP_PAUSE");
			data->user_stop = true;
			if(data->status == VP_STATUS_PLAYING)
			{
				video_dec_pause(data, (bool)msg->ptr);
			}
			break;
		case CMD_VP_STOP:
			SYS_LOG_INF("### CMD_VP_STOP");
			data->user_stop = true;
			if(data->status == VP_STATUS_PLAYING)
			{
				video_dec_stop(data, (bool)msg->ptr);
			}
			break;
		case CMD_VP_SEEK:
			SYS_LOG_INF("### CMD_VP_SEEK");
			if(data->status != VP_STATUS_STOPED && data->status != VP_STATUS_ERROR)
			{
				video_dec_seek(data, (uint32_t)msg->ptr);
			}
			break;
		case CMD_VP_MUTE:
			SYS_LOG_INF("### CMD_VP_MUTE");
			data->mute = 1;
			break;
		case CMD_VP_DEMUTE:
			SYS_LOG_INF("### CMD_VP_DEMUTE");
			data->mute = 0;
			break;
		default:
			SYS_LOG_ERR("no cmd found!");
			break;
	}

cmd_handle_finished:
	if(msg->sync_sem)
		os_sem_give(msg->sync_sem);
}

static void videodec_srv_main_loop(void *parama1, void *parama2, void *parama3)
{
    struct app_msg msg = { 0 };
	bool terminated = false;
	int msg_timeout;

	k_work_queue_start(&video_dec_workq, (os_thread_stack_t *)video_dec_workq_stack,
		VIDEO_DEC_WORKQ_STACKSIZE, 1, NULL);
	k_thread_name_set(&video_dec_workq.thread, "video_dec_workq");

	while (!terminated) {
		if(video_player_data && video_player_data->status == VP_STATUS_PLAYING)
			msg_timeout = 0;
		else
			msg_timeout = 10;

		if (receive_msg(&msg, msg_timeout)) {
			SYS_LOG_INF("type %d, value 0x%x, cmd %d\n", msg.type, msg.value, msg.cmd);
			switch (msg.type) {
			case MSG_VIDEO_PLAYER:
				if (msg.cmd == CMD_VP_OPEN) {
					if (!video_player_data) {
						video_dec_init(&video_player_data);
					}
				}

				video_cmd_handle(video_player_data, &msg);

				if  (msg.cmd == CMD_VP_CLOSE) {
					if (video_player_data) {
						video_dec_deinit(video_player_data);
						video_player_data = NULL;
					}
					SYS_LOG_INF("exit --");
				}
				break;

			case MSG_EXIT_APP:
				SYS_LOG_INF("### MSG_EXIT_APP");
				srv_manager_thread_exit(VIDEODEC_SRV_NAME);
				terminated = true;
				break;

			default:
				break;
			}

			if(msg.callback)
				msg.callback(&msg, 0, NULL);
		}

		if(video_player_data)
		{
			video_dec_handle(video_player_data);
		}

	}
}

#define CONFIG_VIDEO_PLAYER_STACKSIZE 4096

static char  __aligned(ARCH_STACK_PTR_ALIGN) videodec_srv_stack_area[CONFIG_VIDEO_PLAYER_STACKSIZE];

SERVICE_DEFINE(videodec_srv, videodec_srv_stack_area, sizeof(videodec_srv_stack_area),
	   1, BACKGROUND_APP, NULL, NULL, NULL,
	   videodec_srv_main_loop);

