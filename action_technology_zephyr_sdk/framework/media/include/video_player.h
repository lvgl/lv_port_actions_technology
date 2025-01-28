/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file video_player.h
 */
#ifndef _VIDEO_PLAYER_H_
#define _VIDEO_PLAYER_H_

#include <stream.h>

#define VIDEO_PIXEL_WIDTH		CONFIG_PANEL_TIMING_VACTIVE
#define VIDEO_PIXEL_HEIGHT		CONFIG_PANEL_TIMING_HACTIVE

typedef enum
{
	LOCAT_TYPE_DIR,
	LOCAT_TYPE_PLIST,
} location_type_e;


typedef enum
{
	VP_STATUS_IDLE = 0,
	VP_STATUS_PRELOAD,
    VP_STATUS_PLAYING,
    VP_STATUS_PAUSED,
    VP_STATUS_STOPED,
    VP_STATUS_ERROR,
} vp_status_e;

typedef enum
{
	VP_PLAYMODE_NORMAL,
    VP_PLAYMODE_LOOPBACK,
} vp_playmode_e;

typedef struct
{
	void *decode_buf;
	int decode_buf_size;
	int width;
	int height;
	int format;
} video_data_t;

typedef void (*vp_callback)(vp_status_e status, void *user_data, video_data_t *data);
typedef void *(*vp_mem_alloc)(int size);
typedef void (*vp_mem_free)(void *ptr);

typedef struct {
	vp_callback cb;
	void *cb_data;
	bool need_decode;
	bool mute;
	char *url;
	vp_mem_alloc alloc;
	vp_mem_free free;
	bool preload;
	bool repeat;
} video_init_param_t;

/******************************************************************************/
/*! \ingroup video_player_interface
* \par  Description:
*     打开video播放器
* \return  成功返回0, 失败返回-1
*******************************************************************************/
int video_player_open(video_init_param_t *init_param);

/******************************************************************************/
/*! \ingroup video_player_interface
* \par  Description:
*     关闭video播放器
* \return  成功返回0, 失败返回-1
*******************************************************************************/
int video_player_close(void);

/******************************************************************************/
/*! \ingroup video_player_interface
* \par  Description:
*     开始播放
* \return  成功返回0, 失败返回-1
*******************************************************************************/
int video_player_play(void);
/******************************************************************************/
/*! \ingroup video_player_interface
* \par  Description:
*     停止播放，释放文件和音频资源
* \param [in] sync  0: 发送停止命令后立即返回；1: 等待播放器停止后才返回
* \return  成功返回0, 失败返回-1
*******************************************************************************/
int video_player_stop(bool sync, bool mem_release);
/******************************************************************************/
/*! \ingroup video_player_interface
* \par  Description:
*     暂停播放
* \param [in] sync  0: 发送暂停命令后立即返回；1: 等待播放器暂停后才返回
* \return  成功返回0, 失败返回-1
*******************************************************************************/
int video_player_pause(bool sync, bool mem_release);

/******************************************************************************/
/*! \ingroup video_player_interface
* \par  Description:
*     暂停后恢复播放
* \return  成功返回0, 失败返回-1
*******************************************************************************/
int video_player_resume(void);

/******************************************************************************/
/*! \ingroup video_player_interface
* \par  Description:
*     seek到某个时间点，setfile后即可调用，seek后播放器进入播放状态
* \param [in] ms  seek到的时间点，单位毫秒
* \return  成功返回0, 失败返回-1
*******************************************************************************/
int video_player_seek(int ms);

/******************************************************************************/
/*! \ingroup video_player_interface
* \par  Description:
*     获取当前播放器状态
* \param [out] st  返回当前播放器状态
* \return  成功返回对应状态, 失败返回-1
*******************************************************************************/
vp_status_e video_player_get_status(void);

/******************************************************************************/
/*! \ingroup video_player_interface
* \par  Description:
*     获取视频总时间
* \return  成功返回视频总时间, 失败返回-1
*******************************************************************************/
int video_player_get_total_time(void);

/******************************************************************************/
/*! \ingroup video_player_interface
* \par  Description:
*     获取当前播放时间
* \return  成功返回当前播放时间, 失败返回-1
*******************************************************************************/
int video_player_get_cur_time(void);
void *  video_player_get_buffer(void);
int video_player_mute(void);
int video_player_demute(void);

#endif	// _VIDEO_PLAYER_H_
