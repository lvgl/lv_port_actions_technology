/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file dsp_video_defs.h
 */
#ifndef _DSP_VIDEO_DEFS_H_
#define _DSP_VIDEO_DEFS_H_

#include "stdio.h"
#include <video_player.h>
#include <dsp_hal_defs.h>
#include <msg_manager.h>
#include "acts_ringbuf.h"
#include <stream.h>
#include "vp_engine.h"
#include "avi_demuxer.h"


#define VIDEODEC_DSP_BIN	"video.dsp"

#define VIDEODEC_SRV_NAME	"videodec_srv"


#define MAX_PACKET_LEN 		2048

#define MACROBLOCK_PIXEL_LINE	16

#define ADPCM_MAX_FRAME_SIZE  (3200)

enum video_player_msg {
	MSG_VIDEO_PLAYER = MSG_SRV_MESSAGE_START,
};

typedef enum{
	CMD_VP_OPEN,
	CMD_VP_CLOSE,
	CMD_VP_SET_FILE,
	CMD_VP_PLAY,
	CMD_VP_STOP,
	CMD_VP_PAUSE,
	CMD_VP_RESUME,
	CMD_VP_SEEK,
	CMD_VP_MUTE,
	CMD_VP_DEMUTE,
}vp_cmd_t;

typedef struct {
	video_init_param_t init_param;
	vp_status_e status;
	vp_playmode_e play_mode;
	void *decode_buf;
	int decode_buf_size;
	io_stream_t file_stream;
	demuxer_plugin_t *dem_plugin;
	dec_plugin_t *dec_plugin;
	dec_plugin_t *wav_plugin;
	void *dem_handle;
	void *dec_handle;
	void *wav_handle;

	ve_video_info_t video_info;
	demuxer_packet_t packet_buf;
	unsigned int cur_pts;

	struct acts_ringbuf *inbuf;
	struct acts_ringbuf *outbuf;

	struct acts_ringbuf *aud_ringbuf;
	struct audio_track_t *aud_track;
	io_stream_t audio_stream;
    short adpcm_outbuf[ADPCM_MAX_FRAME_SIZE];
	unsigned char mute;
	unsigned char freq_boot;
	bool user_stop;
	uint32_t start_time;
	struct acts_ringbuf *frame_ringbuf;
} video_player_data_t;

int char_to_short(char *in, int len, char *out, int size);

#endif	// _DSP_VIDEO_DEFS_H_

