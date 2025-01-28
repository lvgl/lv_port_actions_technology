/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file key tone
 */
#include <os_common_api.h>
#include <audio_system.h>
#include <media_player.h>
#include <buffer_stream.h>
#include <file_stream.h>
#include <app_manager.h>
#include <mem_manager.h>
#include <tts_manager.h>
#include <audio_track.h>
#include <fs/fs.h>

#include <ringbuff_stream.h>
#include <stream.h>

#include <srv_manager.h>
#include <app_defines.h>
#include <lcmusic.h>

#include "lcmusic.h"
#ifdef CONFIG_PROPERTY
#include "property_manager.h"
#endif

static struct lcmusic_app_t *p_local_music;
static u8_t sd_music_init_state = LCMUSIC_STATUS_STOP;
static u8_t uhost_music_init_state = LCMUSIC_STATUS_STOP;
static u8_t nor_music_init_state = LCMUSIC_STATUS_STOP;
static u8_t nand_music_init_state = LCMUSIC_STATUS_STOP;
static u8_t lcmusic_play_mode = MPLAYER_REPEAT_ALL_MODE;

int lcmusic_get_status(void)
{
	if (!p_local_music)
		return LCMUSIC_STATUS_NULL;
	return p_local_music->music_state;
}
u8_t lcmusic_get_play_state(const char *dir)
{
	if (memcmp(dir, "/SD:", strlen("/SD:")) == 0)
		return sd_music_init_state;
	else if (memcmp(dir, "/USB:", strlen("/USB:")) == 0)
		return uhost_music_init_state;
	else if (memcmp(dir, "/NOR:", strlen("/NOR:")) == 0)
		return nor_music_init_state;
	else if (memcmp(dir, "/NAND:", strlen("/NAND:")) == 0)
		return nand_music_init_state;
	else {
		SYS_LOG_INF("dir :%s error\n", dir);
		return LCMUSIC_STATUS_PLAYING;
	}
}
void lcmusic_store_play_state(const char *dir)
{
	if (memcmp(dir, "/SD:", strlen("/SD:")) == 0)
		sd_music_init_state = p_local_music->music_state;
	else if (memcmp(dir, "/USB:", strlen("/USB:")) == 0)
		uhost_music_init_state = p_local_music->music_state;
	else if (memcmp(dir, "/NOR:", strlen("/NOR:")) == 0)
		nor_music_init_state = p_local_music->music_state;
	else if (memcmp(dir, "/NAND:", strlen("/NAND:")) == 0)
		nand_music_init_state = p_local_music->music_state;
	else {
		SYS_LOG_INF("dir :%s error\n", dir);
	}
#ifdef CONFIG_ESD_MANAGER
	u8_t playing = p_local_music->music_state;

	esd_manager_save_scene(TAG_PLAY_STATE, &playing, 1);
#endif

}
void lcmusic_store_play_mode(u8_t play_mode)
{
	lcmusic_play_mode = play_mode;
}

void _lcmusic_restore_play_state(void)
{
	p_local_music->is_init = 1;
#ifdef CONFIG_THREAD_TIMER
	thread_timer_start(&p_local_music->monitor_timer, 10, 0);
#endif
}
static int _lcmusic_init(const char *dir)
{
	if (p_local_music)
		return 0;

	p_local_music = app_mem_malloc(sizeof(struct lcmusic_app_t));
	if (!p_local_music) {
		SYS_LOG_ERR("malloc failed!\n");
		return -ENOMEM;
	}

	memset(p_local_music, 0, sizeof(struct lcmusic_app_t));

	snprintf(p_local_music->cur_dir, sizeof(p_local_music->cur_dir), "%s", dir);

	/* resume track_no */
	_lcmusic_bpinfo_resume(p_local_music->cur_dir, &p_local_music->music_bp_info);


	lcmusic_thread_timer_init(p_local_music);

	SYS_LOG_INF("init ok,p_local_music:%p,music_state:%d\n", p_local_music, p_local_music->music_state);
	return 0;
}
void _lcmusic_exit(void)
{
	if (!p_local_music)
		goto exit;

	lcmusic_thread_timer_exit(p_local_music);

	lcmusic_bp_info_save(p_local_music);

	lcmusic_stop_play(p_local_music, false);

	lcmusic_exit_iterator();

	/* lcmusic_view_deinit(); */

	app_mem_free(p_local_music);

	p_local_music = NULL;

#ifdef CONFIG_PROPERTY
	property_flush_req(NULL);
#endif

exit:
	/* app_manager_thread_exit(app_manager_get_current_app()); */

	SYS_LOG_INF("exit ok\n");
}
void lcmusic_exit(void)
{
	_lcmusic_exit();
}
void lcmusic_init(const char *dir)
{
	/* int res = 0; */

	if (_lcmusic_init(dir)) {
		SYS_LOG_ERR("init failed");
		return;
	}

	_lcmusic_restore_play_state();
}


/*/SD://cluster:2/864/12992420/aaaa.mp3*/
void lcmusic_get_song_name(char **song_name, uint16_t *str_len)
{
	uint16_t temp_len = 0;
	uint16_t len = 0;

	if (!p_local_music)
		return;

	len = strlen(p_local_music->cur_url);
	while (len) {
		if (p_local_music->cur_url[--len] == '/') {
			*song_name = p_local_music->cur_url + len + 1;
			break;
		}
	}
	/*remove file format*/
	if (len) {
		temp_len = strlen(*song_name);
		while (--temp_len) {
			if (*(*song_name + temp_len) == '.') {
				*str_len = temp_len;
				break;
			}
		}
	}

	SYS_LOG_INF("cur_url:%s, file_name:%s, strlen:%d\n", p_local_music->cur_url, *song_name, *str_len);
}

void lcmusic_start_player(void)
{
	struct app_msg msg = {
		.type = MSG_INPUT_EVENT,
		.cmd = MSG_LCMPLAYER_PLAY_PLAYING,
	};
	SYS_LOG_INF("music_state=%d\n", p_local_music->music_state);

	if (!p_local_music || p_local_music->music_state == LCMUSIC_STATUS_PLAYING)
		return;

	send_async_msg(app_manager_get_current_app(), &msg);
}

void lcmusic_stop(void)
{
	struct app_msg msg = {
		.type = MSG_INPUT_EVENT,
		.cmd = MSG_LCMPLAYER_PLAY_PAUSE,
	};
	SYS_LOG_INF("music_state=%d\n", p_local_music->music_state);

	if (!p_local_music)
		return;
	send_async_msg(app_manager_get_current_app(), &msg);
}

void lcmusic_play_or_pause(void)
{
	struct app_msg msg = {
		.type = MSG_INPUT_EVENT,
	};

	if (!p_local_music)
		return;

	SYS_LOG_INF("music_state=%d\n", p_local_music->music_state);

	if (p_local_music->music_state == LCMUSIC_STATUS_STOP) {
		msg.cmd = MSG_LCMPLAYER_PLAY_PLAYING;
	} else if (p_local_music->music_state == LCMUSIC_STATUS_PLAYING) {
		msg.cmd = MSG_LCMPLAYER_PLAY_PAUSE;
	} else
		return;

	send_async_msg(app_manager_get_current_app(), &msg);
}
void lcmusic_restart_play(void)
{
	struct app_msg msg = {
		.type = MSG_INPUT_EVENT,
		.cmd = MSG_LCMPLAYER_PLAY_PLAYING,
	};

	if (!p_local_music || p_local_music->music_state == LCMUSIC_STATUS_STOP)
		return;

	lcmusic_stop();
	send_async_msg(app_manager_get_current_app(), &msg);
	SYS_LOG_INF("\n");
}

void lcmusic_play_next(void)
{
	struct app_msg msg = {
		.type = MSG_INPUT_EVENT,
		.cmd = MSG_LCMPLAYER_PLAY_NEXT,
	};

	if (!p_local_music)
		return;

	send_async_msg(app_manager_get_current_app(), &msg);

	SYS_LOG_INF("play next\n");
}
void lcmusic_play_prev(void)
{
	struct app_msg msg = {
		.type = MSG_INPUT_EVENT,
		.cmd = MSG_LCMPLAYER_PLAY_PREV,
	};

	if (!p_local_music)
		return;

	send_async_msg(app_manager_get_current_app(), &msg);

	SYS_LOG_INF("play prev\n");
}
void lcmusic_set_play_mode(void)
{
	if (!p_local_music)
		return;
	struct app_msg msg = {
		.type = MSG_INPUT_EVENT,
		.cmd = MSG_LCMPLAYER_SET_PLAY_MODE,
	};

	send_async_msg(app_manager_get_current_app(), &msg);

	SYS_LOG_INF("mode:%d\n", p_local_music->mplayer_mode);

}
uint8_t lcmusic_get_play_mode(void)
{
	SYS_LOG_INF("play mode:%d\n", lcmusic_play_mode);
	return lcmusic_play_mode;
}

void lcmusic_vol_adjust(bool is_add)
{
	if (!p_local_music)
		return;
	struct app_msg msg = {
		.type = MSG_INPUT_EVENT,
		.cmd = MSG_LCMPLAYER_PLAY_VOLDOWN,
	};

	if (is_add)
		msg.cmd = MSG_LCMPLAYER_PLAY_VOLUP;

	send_async_msg(app_manager_get_current_app(), &msg);

	SYS_LOG_INF("mode:%d\n", is_add);

}
void lcmusic_vol_sync(int music_vol)
{
	if (!p_local_music)
		return;

	struct app_msg msg = {
		.type = MSG_INPUT_EVENT,
		.cmd = MSG_LCMPLAYER_VOL_SYNC_EVENT,
		.value = music_vol,
	};
	send_async_msg(app_manager_get_current_app(), &msg);
}

void lcmusic_seek(int seek_time, int cur_time, bool relative_seek)
{
	if (!p_local_music)
		return;

	struct app_msg msg = {
		.type = MSG_INPUT_EVENT,
	};

	msg.cmd = MSG_LCMPLAYER_SEEK_TIME;
	msg.value = seek_time;

	send_async_msg(app_manager_get_current_app(), &msg);
}

void lcmusic_get_song_bp(uint32_t *total_time, int *cur_time)
{
	if (!p_local_music)
		return;
	*total_time = p_local_music->music_bp_info.total_time;
	*cur_time = p_local_music->music_bp_info.bp_info.time_offset;

}
bool lcmusic_is_playing(void)
{
	if (p_local_music && p_local_music->music_state == LCMUSIC_STATUS_PLAYING)
		return true;
	return false;
}

struct lcmusic_app_t *lcmusic_get_app(void)
{
	return p_local_music;
}


static void lcmusic_app_loop(void *p1, void *p2, void *p3)
{
	struct app_msg msg = {0};
	bool terminated = false;

	SYS_LOG_INF(APP_ID_LCMUSIC " enter");

	lcmusic_init(CONFIG_APP_FAT_DISK"/");
	while (!terminated) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			switch (msg.type) {
			case MSG_EXIT_APP:
				app_manager_thread_exit(APP_ID_LCMUSIC);
				terminated = true;
				break;
			case MSG_INPUT_EVENT:
				lcmusic_input_event_proc(&msg);
				break;
			case MSG_LCMUSIC_EVENT:
				lcmusic_event_proc(&msg);
				break;
			default:
				break;
			}

			if (msg.callback != NULL)
				msg.callback(&msg, 0, NULL);
		}

		thread_timer_handle_expired();
	}

	SYS_LOG_INF(APP_ID_LCMUSIC " exit");
}


APP_DEFINE(lcmusic, share_stack_area, sizeof(share_stack_area),
		CONFIG_APP_PRIORITY, DEFAULT_APP | FOREGROUND_APP, NULL, NULL, NULL,
	   lcmusic_app_loop, NULL);


