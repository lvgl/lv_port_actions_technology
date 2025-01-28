/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief lcmusic app main.
 */

#include "lcmusic.h"
#include "tts_manager.h"
#include "fs_manager.h"
#include <bt_manager.h>
#ifdef CONFIG_PROPERTY
#include "property_manager.h"
#endif
#ifdef CONFIG_SYS_WAKELOCK
#include <sys_wakelock.h>
#endif

static struct lcmusic_app_t *p_local_music = NULL;
static u8_t sd_music_init_state = LCMUSIC_STATUS_STOP;
static u8_t uhost_music_init_state = LCMUSIC_STATUS_STOP;
static u8_t nor_music_init_state = LCMUSIC_STATUS_STOP;
static u8_t nand_music_init_state = LCMUSIC_STATUS_STOP;
static u8_t lcmusic_play_mode = MPLAYER_REPEAT_ALL_MODE;
static lcmusic_callback_t *lcmusic_cbk = NULL;

void lcmusic_wake_lock_set(uint8_t lock)
{
	static uint8_t locked = 0;

	if (lock) {
		if (!locked) {
		#ifdef CONFIG_SYS_WAKELOCK
			sys_wake_lock(PARTIAL_WAKE_LOCK);
		#endif
			SYS_LOG_INF("lock\n");
			locked = 1;
		}
	} else {
		if (locked) {
		#ifdef CONFIG_SYS_WAKELOCK
			sys_wake_unlock(PARTIAL_WAKE_LOCK);
		#endif
			SYS_LOG_INF("unlock\n");
			locked = 0;
		}
	}
}

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
	if (p_local_music->is_init == 0) {
		p_local_music->is_init = 1;
	}
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
#ifdef CONFIG_TWS
#ifndef CONFIG_TWS_BACKGROUND_BT
	bt_manager_halt_phone();
#else
	if (system_check_low_latencey_mode()) {
		bt_manager_halt_phone();
	}
#endif
#endif

	memset(p_local_music, 0, sizeof(struct lcmusic_app_t));
#ifdef CONFIG_BT_MANAGER
#if CONFIG_BT_BR_ACTS
	bt_manager_set_stream_type(AUDIO_STREAM_LOCAL_MUSIC);
#endif
#endif
	snprintf(p_local_music->cur_dir, sizeof(p_local_music->cur_dir), "%s", dir);

	/* resume track_no */
	_lcmusic_bpinfo_resume(p_local_music->cur_dir, &p_local_music->music_bp_info);

	p_local_music->mplayer_mode = lcmusic_play_mode;
	//lcmusic_view_init(p_local_music);

	lcmusic_thread_timer_init(p_local_music);

	SYS_LOG_INF("init ok\n");
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

	//lcmusic_view_deinit();

	app_mem_free(p_local_music);

	p_local_music = NULL;

#ifdef CONFIG_PROPERTY
	property_flush_req(NULL);
#endif

exit:
	//app_manager_thread_exit(app_manager_get_current_app());

	SYS_LOG_INF("exit ok\n");
}
void lcmusic_exit(void)
{
	_lcmusic_exit();
}
void lcmusic_init(const char *dir)
{
	//int res = 0;

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
		.reserve = LCMUSIC_PLAYER,
	};

	if (!p_local_music)
		return;

	SYS_LOG_INF("music_state=%d\n", p_local_music->music_state);

	if (p_local_music->music_state == LCMUSIC_STATUS_PLAYING)
		return;

	send_async_msg(app_manager_get_current_app(), &msg);
}

void lcmusic_stop(void)
{
	struct app_msg msg = {
		.type = MSG_INPUT_EVENT,
		.cmd = MSG_LCMPLAYER_PLAY_PAUSE,
		.reserve = LCMUSIC_PLAYER,
	};

	if (!p_local_music)
		return;

	SYS_LOG_INF("music_state=%d\n", p_local_music->music_state);

	send_async_msg(app_manager_get_current_app(), &msg);
}

void lcmusic_play_or_pause(void)
{
	struct app_msg msg = {
		.type = MSG_INPUT_EVENT,
		.reserve = LCMUSIC_PLAYER,
	};

	if (!p_local_music)
		return;

	SYS_LOG_INF("music_state=%d\n", p_local_music->music_state);

	if (p_local_music->music_state == LCMUSIC_STATUS_STOP) {
		msg.cmd = MSG_LCMPLAYER_PLAY_PLAYING;
	}
	else if (p_local_music->music_state == LCMUSIC_STATUS_PLAYING) {
		msg.cmd = MSG_LCMPLAYER_PLAY_PAUSE;
	}
	else
		return;

	send_async_msg(app_manager_get_current_app(), &msg);
}
void lcmusic_restart_play(void)
{
	struct app_msg msg = {
		.type = MSG_INPUT_EVENT,
		.cmd = MSG_LCMPLAYER_PLAY_PLAYING,
		.reserve = LCMUSIC_PLAYER,
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
		.reserve = LCMUSIC_PLAYER,
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
		.reserve = LCMUSIC_PLAYER,
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
		.reserve = LCMUSIC_PLAYER,
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
		.reserve = LCMUSIC_PLAYER,
	};

	if (is_add)
		msg.cmd = MSG_LCMPLAYER_PLAY_VOLUP;

	send_async_msg(app_manager_get_current_app(), &msg);

	SYS_LOG_INF("mode:%d\n", is_add);

}
void lcmusic_vol_sync(int music_vol, bool forced)
{
	if (!p_local_music)
		return;

	struct app_msg msg = {
		.type = MSG_INPUT_EVENT,
		.cmd = MSG_LCMPLAYER_VOL_SYNC_EVENT,
		.reserve = LCMUSIC_PLAYER,
		.value = music_vol,
	};

	if(forced) {
		send_async_msg(app_manager_get_current_app(), &msg);
	}
	else {
		msg_manager_send_async_msg_discardable(app_manager_get_current_app(), &msg);
	}
}

void lcmusic_seek(int seek_time, int cur_time, bool relative_seek)
{
	if(!p_local_music)
		return;

	struct app_msg msg = {
		.type = MSG_INPUT_EVENT,
		.reserve = LCMUSIC_PLAYER,
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
static void _lcmusic_main_prc(const char *dir)
{
	struct app_msg msg = { 0 };

	bool terminated = false;

	if (_lcmusic_init(dir)) {
		SYS_LOG_ERR("init failed");
		_lcmusic_exit();
		return;
	}
	_lcmusic_restore_play_state();
	while (!terminated) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			SYS_LOG_INF("type %d, value %d\n", msg.type, msg.value);
			switch (msg.type) {

			case MSG_INPUT_EVENT:
				lcmusic_input_event_proc(&msg);
				break;
			case MSG_LCMUSIC_EVENT:
				lcmusic_event_proc(&msg);
				break;
			case MSG_TTS_EVENT:
				lcmusic_tts_event_proc(&msg);
				break;
			case MSG_BT_EVENT:
				lcmusic_bt_event_proc(&msg);
				break;
			case MSG_EXIT_APP:
				_lcmusic_exit();
				terminated = true;
				break;
			default:
				break;
			}
			if (msg.callback)
				msg.callback(&msg, 0, NULL);
		}
		if (!terminated)
			thread_timer_handle_expired();
	}
}
struct lcmusic_app_t *lcmusic_get_app(void)
{
	return p_local_music;
}

void lcmusic_set_callback(lcmusic_callback_t *cbk)
{
	lcmusic_cbk = cbk;
}

void lcmusic_set_state(uint8_t new_state)
{
	uint8_t pre_state;

	if(!p_local_music)
		return;

	pre_state = p_local_music->music_state;
	p_local_music->music_state = new_state;

	if(lcmusic_cbk && lcmusic_cbk->state_notify)
		lcmusic_cbk->state_notify(pre_state, new_state);
}

void lcmusic_set_volume(int volume)
{
	audio_system_set_stream_volume(AUDIO_STREAM_LOCAL_MUSIC, volume);

	if (p_local_music && p_local_music->lcplayer && p_local_music->lcplayer->player) {
		media_player_set_volume(p_local_music->lcplayer->player, volume, volume);
	}

	if(lcmusic_cbk && lcmusic_cbk->set_volume) {
		lcmusic_cbk->set_volume(volume);
	}
}

static void sd_mplayer_main_loop(void *parama1, void *parama2, void *parama3)
{
	SYS_LOG_INF("sd Enter\n");

#ifdef CONFIG_FS_MANAGER
	fs_manager_sdcard_enter_high_speed();
#endif

	sys_event_notify(SYS_EVENT_ENTER_SDMPLAYER);

	_lcmusic_main_prc("/SD:/");

#ifdef CONFIG_FS_MANAGER
	fs_manager_sdcard_exit_high_speed();
#endif

	SYS_LOG_INF("sd Exit\n");
}

APP_DEFINE(sd_mplayer, share_stack_area, sizeof(share_stack_area),
	   CONFIG_APP_PRIORITY, FOREGROUND_APP, NULL, NULL, NULL,
	   sd_mplayer_main_loop, NULL);


static void uhost_mplayer_main_loop(void *parama1, void *parama2, void *parama3)
{
	SYS_LOG_INF("uhost Enter\n");

	sys_event_notify(SYS_EVENT_ENTER_UMPLAYER);

	_lcmusic_main_prc("/USB:/");

	SYS_LOG_INF("uhost Exit\n");


}

APP_DEFINE(uhost_mplayer, share_stack_area, sizeof(share_stack_area),
			  CONFIG_APP_PRIORITY, FOREGROUND_APP, NULL, NULL, NULL,
			  uhost_mplayer_main_loop, NULL);

#ifdef CONFIG_DISK_ACCESS_NOR
static void nor_mplayer_main_loop(void *parama1, void *parama2, void *parama3)
{
	SYS_LOG_INF("nor Enter\n");

	sys_event_notify(SYS_EVENT_ENTER_NORMPLAYER);

	_lcmusic_main_prc("/NOR:/");

	SYS_LOG_INF("nor Exit\n");
}
APP_DEFINE(nor_mplayer, share_stack_area, sizeof(share_stack_area),
			CONFIG_APP_PRIORITY, FOREGROUND_APP, NULL, NULL, NULL,
			nor_mplayer_main_loop, NULL);
#endif

