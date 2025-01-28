/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file video_player.c
 *
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <os_common_api.h>
#include "srv_manager.h"
#include <soc_dsp.h>
#include "video_player.h"
#include "video_player_defs.h"
#include <video_mem.h>
#include <file_stream.h>

extern video_player_data_t *video_player_data;

static int vp_send_msg(char *srv_name, int cmd, void *ptr, bool sync_flag)
{
	int ret = 0;
	os_sem sync_sem;

	struct app_msg msg = {
		.type = MSG_VIDEO_PLAYER,
		.cmd = cmd,
		.ptr = ptr,
		.callback = NULL,
		.sync_sem = NULL,
	};

	if (!srv_manager_check_service_is_actived(srv_name)) {
        return -1;
    }

	if (sync_flag) {
		msg.sync_sem = &sync_sem;
		os_sem_init(msg.sync_sem, 0, 1);
	}

	send_async_msg(srv_name, &msg);
	
	if(sync_flag)
	{
		ret = os_sem_take(msg.sync_sem, OS_FOREVER);
	}
	
	return ret;
}

int video_player_open(video_init_param_t *p_init_param)
{	
	SYS_LOG_INF("enter ++");

	if(p_init_param == NULL) {
		return -1;
	}
	
   if (!srv_manager_check_service_is_actived(VIDEODEC_SRV_NAME))
	{
        srv_manager_active_service(VIDEODEC_SRV_NAME);
    }

	vp_send_msg(VIDEODEC_SRV_NAME, CMD_VP_OPEN, p_init_param, true);

	SYS_LOG_INF("ok");
	return 0;
}

int video_player_close(void)
{	
	SYS_LOG_INF("enter ++");

	vp_send_msg(VIDEODEC_SRV_NAME, CMD_VP_CLOSE, NULL, false);
	SYS_LOG_INF("ok");
	return 0;
}

int video_player_play(void)
{
	SYS_LOG_INF("enter ++");
	
	vp_send_msg(VIDEODEC_SRV_NAME, CMD_VP_PLAY, NULL, false);
	
	return 0;
}

int video_player_pause(bool sync, bool mem_release)
{
	SYS_LOG_INF("enter ++");

	vp_send_msg(VIDEODEC_SRV_NAME, CMD_VP_PAUSE, (void *)mem_release, sync);
	
	return 0;
}

int video_player_stop(bool sync, bool mem_release)
{
	SYS_LOG_INF("enter ++");
	
	vp_send_msg(VIDEODEC_SRV_NAME, CMD_VP_STOP, (void *)mem_release, sync);
	
	return 0;
}

int video_player_seek(int ms)
{
	SYS_LOG_INF("seek at %dms", ms);

	vp_send_msg(VIDEODEC_SRV_NAME, CMD_VP_SEEK, (void *)ms, false);
	
	return 0;
}

int video_player_mute(void)
{
	SYS_LOG_INF("enter ++");

	vp_send_msg(VIDEODEC_SRV_NAME, CMD_VP_MUTE, NULL, false);
	
	return 0;
}

int video_player_demute(void)
{
	SYS_LOG_INF("enter ++");

	vp_send_msg(VIDEODEC_SRV_NAME, CMD_VP_DEMUTE, NULL, false);
	
	return 0;
}

vp_status_e video_player_get_status(void)
{
	video_player_data_t *data = video_player_data;

	if(data == NULL)
		return -1;
	
	return data->status;
}

int video_player_get_total_time(void)
{
	video_player_data_t *data = video_player_data;

	if(data == NULL)
		return -1;
	
	return data->video_info.vtotal_time;
}

int video_player_get_cur_time(void)
{
	video_player_data_t *data = video_player_data;

	if(data == NULL)
		return -1;
	
	return data->cur_pts;
}

void * video_player_get_buffer(void)
{
	video_player_data_t *data = video_player_data;

	if(data == NULL)
		return NULL;
	
	return data->decode_buf;
}

