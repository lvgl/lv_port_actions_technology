/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief lcmusic app
 */

#ifndef _LCMUSIC_H
#define _LCMUSIC_H

#ifdef CONFIG_SYS_LOG
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "lcmusic"

#endif
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>
#include <file_stream.h>
#include <app_manager.h>
#include <srv_manager.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <volume_manager.h>
#include <media_player.h>
#include <audio_system.h>
#include <audio_policy.h>
#include <thread_timer.h>
#include "btservice_api.h"
#include "app_defines.h"
#include "sys_manager.h"
#include "app_ui.h"
#include <iterator/file_iterator.h>
#include <fs_manager.h>
#include "../local_player/local_player.h"
#ifdef CONFIG_UI_MANAGER
#include <ui_manager.h>
#endif
#ifdef CONFIG_ESD_MANAGER
#include "esd_manager.h"
#endif
#ifdef CONFIG_LAUNCHER_APP
#include "../launcher/launcher_app.h"
#endif

#define LCMUSIC_SPECIFY_DIR_NAME	10
#define LCMUSIC_STATUS_ALL  (LCMUSIC_STATUS_PLAYING | LCMUSIC_STATUS_SEEK | LCMUSIC_STATUS_ERROR | LCMUSIC_STATUS_STOP)

enum LCMUSIC_PLAYER_STATE {
	LCMUSIC_MPLAYER_STATE_INIT = 0,
	LCMUSIC_MPLAYER_STATE_NORMAL,
	LCMUSIC_MPLAYER_STATE_ERROR,
};

enum MPLAYER_PLAY_MODE {
	MPLAYER_REPEAT_ALL_MODE = 0,
	MPLAYER_REPEAT_ONE_MODE,
	//MPLAYER_NORMAL_ALL_MODE,
	//MPLAYER_REPEAT_FOLDER_MODE,

	MPLAYER_NUM_PLAY_MODES,
};

enum LCMUSIC_PLAY_STATUS {
	LCMUSIC_STATUS_NULL    = 0x0000,
	LCMUSIC_STATUS_PLAYING = 0x0001,
	LCMUSIC_STATUS_SEEK    = 0x0002,
	LCMUSIC_STATUS_ERROR   = 0x0004,
	LCMUSIC_STATUS_STOP   = 0x0008,
};
#ifndef CONFIG_LAUNCHER_APP
enum LCMUSIC_MSG_EVENT_TYPE {
    /* lcmusic mplayer message */
	MSG_LCMUSIC_EVENT = MSG_APP_MESSAGE_START,
};
#endif
enum LCMUSIC_MSG_EVENT_CMD {
	MSG_LCMPLAYER_INIT = 0,
	MSG_LCMPLAYER_PLAY_URL,
	MSG_LCMPLAYER_PLAY_CUR,
	MSG_LCMPLAYER_PLAY_NEXT,
	MSG_LCMPLAYER_PLAY_PREV,
	MSG_LCMPLAYER_AUTO_PLAY_NEXT,/*5*/

	MSG_LCMPLAYER_PLAY_VOLUP,
	MSG_LCMPLAYER_PLAY_VOLDOWN,
	MSG_LCMPLAYER_VOL_SYNC_EVENT,
	MSG_LCMPLAYER_PLAY_PAUSE,
	MSG_LCMPLAYER_PLAY_PLAYING,
	MSG_LCMPLAYER_PLAY_SEEK_FORWARD,/*10*/

	MSG_LCMPLAYER_PLAY_SEEK_BACKWARD,
	MSG_LCMPLAYER_SEEKFW_START_CTIME,
	MSG_LCMPLAYER_SEEKBW_START_CTIME,
	MSG_LCMPLAYER_SEEK_TIME,
	MSG_LCMPLAYER_SEEK_TIME_RELATIVE,
	MSG_LCMPLAYER_SET_PLAY_MODE,
	MSG_LCMPLAYER_PLAY_NEXT_DIR,/*15*/
	MSG_LCMPLAYER_PLAY_PREV_DIR,
#ifdef	CONFIG_INPUT_DEV_ACTS_IRKEY
	MSG_LCMPLAYER_PLAY_TRACK_NO,
	MSG_LCMPLAYER_SET_NUM0,
	MSG_LCMPLAYER_SET_NUM1,
	MSG_LCMPLAYER_SET_NUM2,/*20*/
	MSG_LCMPLAYER_SET_NUM3,
	MSG_LCMPLAYER_SET_NUM4,
	MSG_LCMPLAYER_SET_NUM5,
	MSG_LCMPLAYER_SET_NUM6,
	MSG_LCMPLAYER_SET_NUM7,/*25*/
	MSG_LCMPLAYER_SET_NUM8,
	MSG_LCMPLAYER_SET_NUM9,
#endif
};

enum LCMUSIC_SEEK_STATUS {
	SEEK_NULL = 0,
	SEEK_FORWARD,
	SEEK_BACKWARD,
};


struct file_dir_info_t {
	u32_t cluster;/*cur cluster*/
	u32_t dirent_blk_ofs;/*dir offset in the cur cluster*/
};

struct music_bp_info_t {
	media_breakpoint_info_t bp_info;
#if CONFIG_SUPPORT_FILE_FULL_NAME
	struct file_dir_info_t file_dir_info[MAX_DIR_LEVEL];
#else
	struct file_dir_info_t file_dir_info[1];
#endif
	u32_t total_time;/* total time in milliseconds */
	u32_t file_size;
	u16_t track_no;
};

typedef struct lcmusic_callback {
	void (*set_volume)(int volume);
	void (*state_notify)(uint8_t pre_state, uint8_t new_state);
} lcmusic_callback_t;

struct lcmusic_app_t {
	u16_t sum_track_no;
#ifdef	CONFIG_INPUT_DEV_ACTS_IRKEY
	u16_t track_no;
	u8_t num_play_time;
#endif
	u8_t music_state;
	u32_t full_cycle_times : 3;
	u32_t mplayer_state : 2;
	u32_t mplayer_mode : 3;
	u32_t need_resume_play : 1;
	u32_t prev_music_state : 1;
	u32_t seek_direction : 2;/*1--forward,2--backward*/
	u32_t is_init : 1;
	u32_t is_play_in5s : 1;
	u32_t play_time : 4;
	u32_t check_usb_plug_time : 3;
	u32_t filter_prev_tts : 1;
	u32_t filt_track_no : 1;/*1--don't show track_no*/
	u32_t disk_scanning : 1;
	u32_t num_play : 1;
#ifdef CONFIG_SOUNDBAR_LCMUSIC
	u32_t soundbar_check_disk_out : 1;/*1--check if disk pluged out*/
#endif
	char cur_dir[LCMUSIC_SPECIFY_DIR_NAME + 1];
	char cur_url[MAX_URL_LEN + 1];

	struct music_bp_info_t  music_bp_info;
	media_info_t media_info;
	struct local_player_t *lcplayer;

	struct thread_timer seek_timer;
	struct thread_timer monitor_timer;
	struct thread_timer tts_resume_timer;
#ifdef CONFIG_SOUNDBAR_LCMUSIC
	struct thread_timer soundbar_disk_timer;
#endif
	struct thread_timer seek_time_timer;
	int last_seek_time;
};

void lcmusic_view_init(struct lcmusic_app_t *lcmusic);

void lcmusic_view_deinit(void);

struct lcmusic_app_t *lcmusic_get_app(void);

void lcmusic_bp_info_save(struct lcmusic_app_t *lcmusic);

void _lcmusic_bpinfo_save(const char *dir, struct music_bp_info_t music_bp_info);

int _lcmusic_bpinfo_resume(const char *dir, struct music_bp_info_t *music_bp_info);

void lcmusic_input_event_proc(struct app_msg *msg);

void lcmusic_event_proc(struct app_msg *msg);

void lcmusic_tts_event_proc(struct app_msg *msg);

int lcmusic_get_status(void);

int lcmusic_init_iterator(struct lcmusic_app_t *lcmusic);

const char *lcmusic_play_next_url(struct lcmusic_app_t *lcmusic, bool force_switch);

const char *lcmusic_play_prev_url(struct lcmusic_app_t *lcmusic);

void lcmusic_exit_iterator(void);

void lcmusic_stop_play(struct lcmusic_app_t *lcmusic, bool need_updata_bp);

u8_t lcmusic_get_play_state(const char *dir);

void lcmusic_store_play_state(const char *dir);

void lcmusic_thread_timer_init(struct lcmusic_app_t *lcmusic);
void lcmusic_thread_timer_exit(struct lcmusic_app_t *lcmusic);
void lcmusic_bt_event_proc(struct app_msg *msg);
void lcmusic_volume_changed_event_proc(struct app_msg *msg);
void lcmusic_display_play_time(struct lcmusic_app_t *lcmusic, int seek_time);
void lcmusic_display_icon_play(void);
void lcmusic_display_icon_pause(void);
const char *lcmusic_play_next_folder_url(struct lcmusic_app_t *lcmusic);
const char *lcmusic_play_prev_folder_url(struct lcmusic_app_t *lcmusic);
const char *lcmusic_play_set_track_no(struct lcmusic_app_t *lcmusic, u16_t track_no);
int lcmusic_play_set_mode(struct lcmusic_app_t *lcmusic, u8_t mode);
int lcmusic_get_url_by_cluster(struct lcmusic_app_t *lcmusic);
int lcmusic_get_cluster_by_url(struct lcmusic_app_t *lcmusic);
void lcmusic_scan_disk(void);
void lcmusic_display_track_no(u16_t track_no, int display_times);
void lcmusic_start_player(void);
void lcmusic_stop(void);
void lcmusic_play_or_pause(void);
void lcmusic_play_next(void);
void lcmusic_play_prev(void);
void lcmusic_set_play_mode(void);
void lcmusic_get_song_name(char **song_name, uint16_t *strlen);
void lcmusic_vol_adjust(bool is_add);
void lcmusic_vol_sync(int music_vol, bool force);
void lcmusic_get_song_bp(uint32_t *total_time, int *cur_time);
bool lcmusic_is_playing(void);
uint8_t lcmusic_get_play_mode(void);
void lcmusic_seek(int seek_time, int cur_time, bool relative_seek);
void lcmusic_set_state(uint8_t new_state);
void lcmusic_set_volume(int volume);
void lcmusic_set_callback(lcmusic_callback_t *cbk);

#endif  /* _LCMUSIC_H */
