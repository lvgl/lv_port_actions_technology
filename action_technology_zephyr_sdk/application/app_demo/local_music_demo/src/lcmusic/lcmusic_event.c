/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief lcmusic app event
 */

#include "lcmusic.h"
#include "tts_manager.h"
#include <fs_manager.h>


#define MONITOR_TIME_PERIOD	1000/* 1000ms */
#define DISPLAY_TIME_PERIOD	1000/* 1000ms */

#define SEEK_SPEED_LEVEL	(10)
static u32_t start_seek_time;
extern void lcmusic_display_song_info(char *song_info, bool direct_update);
extern void lcmusic_store_play_mode(u8_t play_mode);

void lcmusic_display_play_time(struct lcmusic_app_t *lcmusic, int seek_time)
{

}

void lcmusic_display_track_no(u16_t track_no, int display_times)
{

}

#ifndef CONFIG_LAUNCHER_APP
static bool _check_disk_plugout(void)
{
	return false;
}
#endif
static void _lcmusic_force_play_next(struct lcmusic_app_t *lcmusic, bool force_switch)
{
	struct app_msg msg = {
		.type = MSG_LCMUSIC_EVENT,
		.cmd = MSG_LCMPLAYER_AUTO_PLAY_NEXT,
	};

	SYS_LOG_INF("%d\n", force_switch);

	switch (lcmusic->mplayer_mode) {
	case MPLAYER_REPEAT_ONE_MODE:
		if (!force_switch)
			msg.cmd = MSG_LCMPLAYER_PLAY_CUR;
		break;
	default:
		break;
	}
	if (lcmusic->prev_music_state) {
		msg.type = MSG_INPUT_EVENT;
		msg.cmd = MSG_LCMPLAYER_PLAY_PREV;
		lcmusic->filter_prev_tts = 1;
	}
	/*fix app message lost:decoder thread sleep 20ms to reduce app message*/
	os_sleep(20);
	send_async_msg(app_manager_get_current_app(), &msg);
}

static void _lcmusic_clear_breakpoint(struct lcmusic_app_t *lcmusic)
{
	lcmusic->music_bp_info.bp_info.file_offset = 0;
	lcmusic->music_bp_info.bp_info.time_offset = 0;
}

static void _lcmusic_play_event_callback(u32_t event, void *data, u32_t len, void *user_data)
{
	struct lcmusic_app_t *lcmusic = lcmusic_get_app();

	if (!lcmusic)
		return;

	SYS_LOG_INF("lcmusic: play event %d\n", event);

	switch (event) {
	case PLAYBACK_EVENT_STOP_ERROR:
		if (lcmusic->mplayer_state != MPLAYER_STATE_NORMAL)
			lcmusic->mplayer_state = MPLAYER_STATE_ERROR;
		lcmusic->music_state = LCMUSIC_STATUS_ERROR;
		_lcmusic_force_play_next(lcmusic, true);
		break;
	case PLAYBACK_EVENT_STOP_COMPLETE:
	/* case PARSER_EVENT_STOP_COMPLETE: */
		lcmusic->prev_music_state = 0;
		lcmusic->mplayer_state = MPLAYER_STATE_NORMAL;
		lcmusic->full_cycle_times = 0;
		_lcmusic_force_play_next(lcmusic, false);
		break;
	default:

		break;
	}
}

void lcmusic_bp_info_save(struct lcmusic_app_t *lcmusic)
{
	if (lcmusic->music_state == LCMUSIC_STATUS_NULL)
		return;
	mplayer_update_breakpoint(lcmusic->lcplayer, &lcmusic->music_bp_info.bp_info);
	_lcmusic_bpinfo_save(lcmusic->cur_dir, lcmusic->music_bp_info);
}

void lcmusic_stop_play(struct lcmusic_app_t *lcmusic, bool need_updata_bp)
{
	_lcmusic_clear_breakpoint(lcmusic);

	if (!lcmusic->lcplayer)
		return;

	if (need_updata_bp) {
		mplayer_update_breakpoint(lcmusic->lcplayer, &lcmusic->music_bp_info.bp_info);
		mplayer_update_media_info(lcmusic->lcplayer, &lcmusic->media_info);
	}
#ifdef CONFIG_THREAD_TIMER
	if (thread_timer_is_running(&lcmusic->monitor_timer))
		thread_timer_stop(&lcmusic->monitor_timer);

	if (lcmusic->music_state != LCMUSIC_STATUS_SEEK) {
		if (thread_timer_is_running(&lcmusic->seek_timer))
			thread_timer_stop(&lcmusic->seek_timer);
	}
#endif
	mplayer_stop_play(lcmusic->lcplayer);

	lcmusic->lcplayer = NULL;

	if (lcmusic->music_state == LCMUSIC_STATUS_PLAYING)
		lcmusic->music_state = LCMUSIC_STATUS_STOP;
}

static int _lcmusic_esd_restore_bp_info(struct lcmusic_app_t *lcmusic, u8_t *play_state)
{
	int res = -EINVAL;

#ifdef CONFIG_ESD_MANAGER
	if (esd_manager_check_esd()) {
		res = 0;
		esd_manager_restore_scene(TAG_PLAY_STATE, play_state, 1);
		esd_manager_restore_scene(TAG_BP_INFO, (u8_t *)&lcmusic->music_bp_info, sizeof(lcmusic->music_bp_info));

		SYS_LOG_INF("music_bp_info:time_offset=%d,file_offset=%d\n",
			lcmusic->music_bp_info.bp_info.time_offset, lcmusic->music_bp_info.bp_info.file_offset);

		/* print_buffer(&lcmusic->music_bp_info, 1, sizeof(lcmusic->music_bp_info), 16, -1); */
	}
#endif
	return res;
}

static void _lcmusic_esd_save_bp_info(struct lcmusic_app_t *lcmusic, u8_t is_need_update_cluster)
{
	mplayer_update_breakpoint(lcmusic->lcplayer, &lcmusic->music_bp_info.bp_info);
	if (is_need_update_cluster) {
		if (lcmusic_get_cluster_by_url(lcmusic))
			SYS_LOG_INF("get cluster failed\n");
		/* print_buffer(&lcmusic->music_bp_info, 1, sizeof(lcmusic->music_bp_info), 16, -1); */
	}
}

static void _lcmusic_monitor_timer(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct lcmusic_app_t *lcmusic = lcmusic_get_app();
	struct app_msg msg = { 0 };

	/* SYS_LOG_INF("_lcmusic_monitor_timer,lcmusic:%p,is_init:%d,music_state:%d\n",lcmusic,lcmusic->is_init,lcmusic->music_state); */
	if (!lcmusic)
		return;
	if (lcmusic->is_init) {
		lcmusic->check_usb_plug_time = 0;
		lcmusic->is_init = 0;
		lcmusic->music_state = LCMUSIC_STATUS_STOP;
		msg.type = MSG_LCMUSIC_EVENT;
		msg.cmd = MSG_LCMPLAYER_INIT;
		send_async_msg(app_manager_get_current_app(), &msg);
		return;
	}
	/*display seek progress*/
	if (lcmusic->music_state == LCMUSIC_STATUS_SEEK) {
		if (lcmusic->seek_direction == SEEK_FORWARD) {
			lcmusic_display_play_time(lcmusic, SEEK_SPEED_LEVEL * (os_uptime_get_32() - start_seek_time));
		} else if (lcmusic->seek_direction == SEEK_BACKWARD) {
			lcmusic_display_play_time(lcmusic, -SEEK_SPEED_LEVEL * (os_uptime_get_32() - start_seek_time));
		}
	}

	if (!lcmusic->lcplayer || lcmusic->music_state != LCMUSIC_STATUS_PLAYING)
		return;

	if (lcmusic->is_play_in5s) {
		if (++lcmusic->play_time > 5000 / MONITOR_TIME_PERIOD) {
			lcmusic->is_play_in5s = 0;
			lcmusic->play_time = 0;
			SYS_LOG_INF("play 5s\n");
		}
	}
	_lcmusic_esd_save_bp_info(lcmusic, 0);
	lcmusic_display_play_time(lcmusic, 0);
}
static void _lcmusic_seek_timer(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct app_msg msg = {
		.type = MSG_LCMUSIC_EVENT,
	};
	struct lcmusic_app_t *lcmusic = lcmusic_get_app();

	if (!lcmusic || !lcmusic->seek_direction)
		return;

	/* replay current url from beginning */
	_lcmusic_clear_breakpoint(lcmusic);

	SYS_LOG_INF("%d\n", lcmusic->seek_direction);

	lcmusic->mplayer_state = MPLAYER_STATE_NORMAL;
	lcmusic->prev_music_state = 0;
	if (lcmusic->seek_direction == SEEK_FORWARD) {
		_lcmusic_force_play_next(lcmusic, false);
	} else {
		msg.cmd = MSG_LCMPLAYER_PLAY_CUR;
		send_async_msg(app_manager_get_current_app(), &msg);
	}
	lcmusic->seek_direction = SEEK_NULL;
}

void lcmusic_get_song_bp(uint32_t *total_time, int *cur_time);
bool lcmusic_is_playing(void);
static void _lcmusic_display_info_timer(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	uint32_t total_time;
	int cur_time;

	if (lcmusic_is_playing()) {
		lcmusic_get_song_bp(&total_time, &cur_time);
		SYS_LOG_INF("total_time:%u,cur_time:%d", total_time, cur_time);
	}
}

void lcmusic_thread_timer_init(struct lcmusic_app_t *lcmusic)
{
#ifdef CONFIG_THREAD_TIMER
	thread_timer_init(&lcmusic->monitor_timer, _lcmusic_monitor_timer, NULL);
	thread_timer_init(&lcmusic->seek_timer, _lcmusic_seek_timer, NULL);
	thread_timer_init(&lcmusic->display_info_timer, _lcmusic_display_info_timer, NULL);
#endif
}

void lcmusic_thread_timer_exit(struct lcmusic_app_t *lcmusic)
{
#ifdef CONFIG_THREAD_TIMER
	thread_timer_stop(&lcmusic->monitor_timer);
	thread_timer_stop(&lcmusic->seek_timer);
	thread_timer_stop(&lcmusic->display_info_timer);
#endif
}

static void _lcmusic_get_song_total_time(struct lcmusic_app_t *lcmusic)
{
	if (mplayer_update_media_info(lcmusic->lcplayer, &lcmusic->media_info)) {
		lcmusic->music_bp_info.total_time = 0;
	} else {
		lcmusic->music_bp_info.total_time = lcmusic->media_info.total_time;
	}
}
static void _lcmusic_start_play(struct lcmusic_app_t *lcmusic, const char *url, int seek_time)
{
	struct lcplay_param play_param;

	memset(&play_param, 0, sizeof(struct lcplay_param));
	play_param.url = url;
	play_param.seek_time = seek_time;
	play_param.play_event_callback = _lcmusic_play_event_callback;
	play_param.bp.file_offset = lcmusic->music_bp_info.bp_info.file_offset;
	play_param.bp.time_offset = lcmusic->music_bp_info.bp_info.time_offset;

	lcmusic->lcplayer = mplayer_start_play(&play_param);
	if (!lcmusic->lcplayer) {
		lcmusic->music_state = LCMUSIC_STATUS_ERROR;
		if (lcmusic->mplayer_state != MPLAYER_STATE_NORMAL)
			lcmusic->mplayer_state = MPLAYER_STATE_ERROR;
		_lcmusic_force_play_next(lcmusic, true);
	} else {
		lcmusic->is_play_in5s = 1;
		lcmusic->play_time = 0;
		lcmusic->music_state = LCMUSIC_STATUS_PLAYING;
		lcmusic_store_play_state(lcmusic->cur_dir);
		/*get song total time*/
		_lcmusic_get_song_total_time(lcmusic);
		if (!lcmusic->filt_track_no)
			lcmusic_display_track_no(lcmusic->music_bp_info.track_no, 3000);
		lcmusic->filt_track_no = 0;
	#ifdef CONFIG_THREAD_TIMER
		thread_timer_start(&lcmusic->monitor_timer, 0, MONITOR_TIME_PERIOD / 2);
		thread_timer_start(&lcmusic->display_info_timer, 0, DISPLAY_TIME_PERIOD);
	#endif
		_lcmusic_esd_save_bp_info(lcmusic, 1);
	}
}
static void _lcmusic_switch_app_check(struct lcmusic_app_t *lcmusic)
{
#ifndef CONFIG_SOUNDBAR_LCMUSIC
#ifndef CONFIG_LAUNCHER_APP
	struct app_msg msg = {0};

	if (lcmusic->mplayer_state == MPLAYER_STATE_ERROR) {
		msg.reserve = 0;
		msg.type = MSG_INPUT_EVENT;
		msg.cmd = MSG_SWITCH_APP;
		if (_check_disk_plugout()) {
			msg.ptr = APP_ID_DEFAULT;
			SYS_LOG_WRN("disk plug out,exit app\n");
		} else {
			msg.ptr = NULL;
			SYS_LOG_WRN("no music,exit app\n");
		}
		/* change to next app */
		send_async_msg(APP_ID_MAIN, &msg);
	} else {
		/*update play time to 00:00*/
		lcmusic_display_play_time(lcmusic, 0);
		lcmusic->music_state = LCMUSIC_STATUS_ERROR;
		SYS_LOG_INF("not switch\n");
	}
#endif
#else
	lcmusic->music_state = LCMUSIC_STATUS_ERROR;

	if (_check_disk_plugout()) {
		/* disk out */
		lcmusic->is_init = 1;
		lcmusic_exit_iterator();
		lcmusic->soundbar_check_disk_out = 0;
	} else {
		/* disk in but no file */
		lcmusic->soundbar_check_disk_out = 1;
	}
#ifdef CONFIG_THREAD_TIMER
	thread_timer_start(&lcmusic->soundbar_disk_timer, 1000, 0);
#endif
#endif
}


static void _lcmusic_tts_delay_resume(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct lcmusic_app_t *lcmusic = lcmusic_get_app();

	if ((lcmusic->music_state != LCMUSIC_STATUS_PLAYING)
		&& (lcmusic->music_state != LCMUSIC_STATUS_ERROR)) {
		lcmusic->filt_track_no = 1;
		_lcmusic_start_play(lcmusic, lcmusic->cur_url, 0);
	}
	lcmusic->need_resume_play = 0;
}

void lcmusic_tts_event_proc(struct app_msg *msg)
{
	struct lcmusic_app_t *lcmusic = lcmusic_get_app();

	if (!lcmusic)
		return;

	SYS_LOG_INF("lcmusic: msg->value=%d,music_state=%d,need_resume_play=%d\n",
		msg->value, lcmusic->music_state, lcmusic->need_resume_play);

	switch (msg->value) {
	case TTS_EVENT_START_PLAY:
		if (lcmusic->music_state == LCMUSIC_STATUS_PLAYING)
			lcmusic->need_resume_play = 1;
		if (lcmusic->lcplayer)
			lcmusic_stop_play(lcmusic, true);
		break;
	case TTS_EVENT_STOP_PLAY:
		if (lcmusic->need_resume_play) {
		#ifdef CONFIG_THREAD_TIMER
			if (thread_timer_is_running(&lcmusic->tts_resume_timer)) {
				thread_timer_stop(&lcmusic->tts_resume_timer);
			}
			thread_timer_init(&lcmusic->tts_resume_timer, _lcmusic_tts_delay_resume, NULL);
			thread_timer_start(&lcmusic->tts_resume_timer, 2000, 0);
		#endif
		}
		break;
	}
}

void lcmusic_input_event_proc(struct app_msg *msg)
{
	struct app_msg new_msg = {0};
	struct lcmusic_app_t *lcmusic = lcmusic_get_app();
	media_breakpoint_info_t bp;

	if (!lcmusic)
		return;

	SYS_LOG_INF("lcmusic: input cmd %d\n", msg->cmd);

	switch (msg->cmd) {
	case MSG_LCMPLAYER_PLAY_NEXT:
		if (lcmusic->disk_scanning || lcmusic->seek_direction)
			break;

		lcmusic->prev_music_state = 0;
		lcmusic->filt_track_no = 0;
		lcmusic_stop_play(lcmusic, false);
		sys_event_notify(SYS_EVENT_PLAY_NEXT);

		if (lcmusic_play_next_url(lcmusic, true)) {
			new_msg.type = MSG_LCMUSIC_EVENT;
			new_msg.cmd = MSG_LCMPLAYER_PLAY_URL;
			send_async_msg(app_manager_get_current_app(), &new_msg);
		} else {
			_lcmusic_switch_app_check(lcmusic);
		}
		break;
	case MSG_LCMPLAYER_PLAY_PREV:
		if (lcmusic->disk_scanning || lcmusic->seek_direction)
			break;

		lcmusic->prev_music_state = 1;
		if (lcmusic->is_play_in5s) {
			lcmusic->is_play_in5s = 0;
			lcmusic->filt_track_no = 0;
			lcmusic_stop_play(lcmusic, false);
			sys_event_notify(SYS_EVENT_PLAY_PREVIOUS);
			if (!lcmusic_play_prev_url(lcmusic)) {
				SYS_LOG_INF("prev url is null\n");
				if (lcmusic->music_state == LCMUSIC_STATUS_ERROR) {
					new_msg.cmd = MSG_LCMPLAYER_AUTO_PLAY_NEXT;
				} else {
					new_msg.cmd = MSG_LCMPLAYER_PLAY_URL;
				}
			} else {
				new_msg.cmd = MSG_LCMPLAYER_PLAY_URL;
			}
			new_msg.type = MSG_LCMUSIC_EVENT;
			send_async_msg(app_manager_get_current_app(), &new_msg);
		} else {
			if ((lcmusic->music_state == LCMUSIC_STATUS_PLAYING) || (lcmusic->music_state == LCMUSIC_STATUS_STOP)) {
				lcmusic_stop_play(lcmusic, false);
				_lcmusic_start_play(lcmusic, lcmusic->cur_url, 0);
			} else {
				lcmusic_stop_play(lcmusic, false);
				if (!lcmusic->filter_prev_tts) {
					sys_event_notify(SYS_EVENT_PLAY_PREVIOUS);
				}
				lcmusic->filter_prev_tts = 0;
				if (lcmusic_play_prev_url(lcmusic)) {
					_lcmusic_start_play(lcmusic, lcmusic->cur_url, 0);
				} else {
					SYS_LOG_INF("prev url is null\n");
					if (lcmusic->music_state == LCMUSIC_STATUS_ERROR) {
						new_msg.cmd = MSG_LCMPLAYER_AUTO_PLAY_NEXT;
					} else {
						new_msg.cmd = MSG_LCMPLAYER_PLAY_URL;
					}
					new_msg.type = MSG_LCMUSIC_EVENT;
					send_async_msg(app_manager_get_current_app(), &new_msg);
				}
			}
		}
		break;
	case MSG_LCMPLAYER_PLAY_VOLUP:
		system_volume_up(AUDIO_STREAM_LOCAL_MUSIC, 1);
		break;
	case MSG_LCMPLAYER_PLAY_VOLDOWN:
		system_volume_down(AUDIO_STREAM_LOCAL_MUSIC, 1);
		break;
	case MSG_LCMPLAYER_VOL_SYNC_EVENT:
		system_volume_set(AUDIO_STREAM_LOCAL_MUSIC, msg->value, false);
		break;
	case MSG_LCMPLAYER_PLAY_PAUSE:
		if (lcmusic->music_state == LCMUSIC_STATUS_PLAYING) {
			lcmusic->filt_track_no = 1;
			lcmusic_stop_play(lcmusic, true);
			lcmusic_store_play_state(lcmusic->cur_dir);
			sys_event_notify(SYS_EVENT_PLAY_PAUSE);
			lcmusic_bp_info_save(lcmusic);
		}
		break;
	case MSG_LCMPLAYER_PLAY_PLAYING:
		sys_event_notify(SYS_EVENT_PLAY_START);

		new_msg.type = MSG_LCMUSIC_EVENT;
		new_msg.cmd = MSG_LCMPLAYER_PLAY_URL;
		send_async_msg(app_manager_get_current_app(), &new_msg);
		break;
	case MSG_LCMPLAYER_SEEK_TIME:
		if (msg->value) {
			SYS_LOG_INF("MSG_LCMPLAYER_PLAY_SEEK_TIME %d\n", msg->value);
			if (lcmusic->music_state == LCMUSIC_STATUS_STOP) {
				_lcmusic_start_play(lcmusic, lcmusic->cur_url, msg->value);
			} else {
				mplayer_seek(lcmusic->lcplayer, msg->value, &bp);
			}
		}
		break;
	case MSG_LCMPLAYER_SEEK_TIME_RELATIVE:
		if (msg->value) {
			SYS_LOG_INF("MSG_LCMPLAYER_PLAY_SEEK_TIME_RELATIVE %d\n", msg->value);
			if (lcmusic->music_state == LCMUSIC_STATUS_STOP) {
				_lcmusic_start_play(lcmusic, lcmusic->cur_url, msg->value);
			}
			mplayer_seek_relative(lcmusic->lcplayer, msg->value, &bp);
		}
		break;
	case MSG_LCMPLAYER_PLAY_SEEK_FORWARD:
	#ifdef CONFIG_THREAD_TIMER
		if (thread_timer_is_running(&lcmusic->seek_timer))
			thread_timer_stop(&lcmusic->seek_timer);

		lcmusic->seek_direction = SEEK_NULL;
		if (lcmusic->music_state == LCMUSIC_STATUS_SEEK) {
			if (thread_timer_is_running(&lcmusic->monitor_timer))
				thread_timer_stop(&lcmusic->monitor_timer);

			lcmusic->filt_track_no = 1;
			_lcmusic_start_play(lcmusic, lcmusic->cur_url, SEEK_SPEED_LEVEL * (os_uptime_get_32() - start_seek_time));
		}
	#endif
		break;
	case MSG_LCMPLAYER_PLAY_SEEK_BACKWARD:
	#ifdef CONFIG_THREAD_TIMER
		if (thread_timer_is_running(&lcmusic->seek_timer))
			thread_timer_stop(&lcmusic->seek_timer);

		lcmusic->seek_direction = SEEK_NULL;
		if (lcmusic->music_state == LCMUSIC_STATUS_SEEK) {
			if (thread_timer_is_running(&lcmusic->monitor_timer))
				thread_timer_stop(&lcmusic->monitor_timer);

			lcmusic->filt_track_no = 1;
			_lcmusic_start_play(lcmusic, lcmusic->cur_url, -(SEEK_SPEED_LEVEL * (os_uptime_get_32() - start_seek_time)));
		}
	#endif
		break;
	case MSG_LCMPLAYER_SEEKFW_START_CTIME:
		lcmusic->music_state = LCMUSIC_STATUS_SEEK;
		lcmusic->seek_direction = SEEK_FORWARD;
		start_seek_time = os_uptime_get_32();
		lcmusic_stop_play(lcmusic, true);
	#ifdef CONFIG_THREAD_TIMER
		thread_timer_start(&lcmusic->monitor_timer, 1000 / SEEK_SPEED_LEVEL, 1000 / SEEK_SPEED_LEVEL);
		if (lcmusic->media_info.total_time > lcmusic->music_bp_info.bp_info.time_offset) {
			thread_timer_start(&lcmusic->seek_timer,
							(lcmusic->media_info.total_time - lcmusic->music_bp_info.bp_info.time_offset) / SEEK_SPEED_LEVEL, 0);
		}
	#endif
		break;
	case MSG_LCMPLAYER_SEEKBW_START_CTIME:
		lcmusic->music_state = LCMUSIC_STATUS_SEEK;
		lcmusic->seek_direction = SEEK_BACKWARD;
		start_seek_time = os_uptime_get_32();
		lcmusic_stop_play(lcmusic, true);
	#ifdef CONFIG_THREAD_TIMER
		thread_timer_start(&lcmusic->monitor_timer, 1000 / SEEK_SPEED_LEVEL, 1000 / SEEK_SPEED_LEVEL);
		thread_timer_start(&lcmusic->seek_timer,
						lcmusic->music_bp_info.bp_info.time_offset / SEEK_SPEED_LEVEL, 0);
	#endif
		break;
	case MSG_LCMPLAYER_SET_PLAY_MODE:
		if (++lcmusic->mplayer_mode >= MPLAYER_NUM_PLAY_MODES)
			lcmusic->mplayer_mode = 0;
		lcmusic_play_set_mode(lcmusic, lcmusic->mplayer_mode);
		lcmusic_store_play_mode(lcmusic->mplayer_mode);
		SYS_LOG_INF("set mplayer_mode :%d\n", lcmusic->mplayer_mode);
		break;
	case MSG_LCMPLAYER_PLAY_NEXT_DIR:
		if (lcmusic->disk_scanning || lcmusic->seek_direction)
			break;
		lcmusic->prev_music_state = 0;
		lcmusic->filt_track_no = 0;
		lcmusic_stop_play(lcmusic, false);

		if (lcmusic_play_next_folder_url(lcmusic)) {
			new_msg.type = MSG_LCMUSIC_EVENT;
			new_msg.cmd = MSG_LCMPLAYER_PLAY_URL;
			send_async_msg(app_manager_get_current_app(), &new_msg);
		} else {
			_lcmusic_switch_app_check(lcmusic);
		}
		break;
	case MSG_LCMPLAYER_PLAY_PREV_DIR:
		if (lcmusic->disk_scanning || lcmusic->seek_direction)
			break;

		lcmusic->filt_track_no = 0;
		lcmusic_stop_play(lcmusic, false);
		if (lcmusic_play_prev_folder_url(lcmusic)) {
			new_msg.type = MSG_LCMUSIC_EVENT;
			new_msg.cmd = MSG_LCMPLAYER_PLAY_URL;
			send_async_msg(app_manager_get_current_app(), &new_msg);
		} else {
			_lcmusic_switch_app_check(lcmusic);
		}
		break;

	default:
		SYS_LOG_WRN("unknown msg cmd =%d\n", msg->cmd);
		break;
	}
}
/*need to clear bp when filed*/
static int _lcmusic_get_bp_url(struct lcmusic_app_t *lcmusic)
{
	return lcmusic_get_url_by_cluster(lcmusic);
}

void lcmusic_event_proc(struct app_msg *msg)
{
	struct app_msg new_msg = {0};
	struct lcmusic_app_t *lcmusic = lcmusic_get_app();
	int res = 0;
	u8_t init_play_state = 0;

	if (!lcmusic)
		return;

	SYS_LOG_INF("lcmusic: proc cmd %d\n", msg->cmd);

	switch (msg->cmd) {
	case MSG_LCMPLAYER_INIT:
		init_play_state = lcmusic_get_play_state(lcmusic->cur_dir);

		res = _lcmusic_esd_restore_bp_info(lcmusic, &init_play_state);/*need to consider reset when geting bp*/
		if (res)
			_lcmusic_bpinfo_resume(lcmusic->cur_dir, &lcmusic->music_bp_info);

		lcmusic_scan_disk();
		res = _lcmusic_get_bp_url(lcmusic);

		if (init_play_state != LCMUSIC_STATUS_STOP) {
			if (!res) {
				new_msg.cmd = MSG_LCMPLAYER_PLAY_URL;
			} else {
				new_msg.cmd = MSG_LCMPLAYER_AUTO_PLAY_NEXT;
			}
			new_msg.type = MSG_APP_MESSAGE_START;
			send_async_msg(app_manager_get_current_app(), &new_msg);
		} else {
			lcmusic->music_state = LCMUSIC_STATUS_STOP;
			if (res)
				lcmusic_play_next_url(lcmusic, false);
			SYS_LOG_INF("init cur url %s\n", lcmusic->cur_url);
			lcmusic_display_play_time(lcmusic, 0);
		}
		break;

	case MSG_LCMPLAYER_AUTO_PLAY_NEXT:
		/* avoid wrong track_no display */
		lcmusic->filt_track_no = 0;

		lcmusic_stop_play(lcmusic, false);
		if (lcmusic_play_next_url(lcmusic, false)) {
			_lcmusic_start_play(lcmusic, lcmusic->cur_url, 0);
		} else {
			_lcmusic_switch_app_check(lcmusic);
		}
		break;
	case MSG_LCMPLAYER_PLAY_URL:
		if (lcmusic->lcplayer) {
			lcmusic_stop_play(lcmusic, false);
		}
		_lcmusic_start_play(lcmusic, lcmusic->cur_url, 0);
		break;
	case MSG_LCMPLAYER_PLAY_CUR:
		lcmusic_stop_play(lcmusic, false);
		_lcmusic_start_play(lcmusic, lcmusic->cur_url, 0);
		break;
	default:
		SYS_LOG_WRN("unknown msg cmd =%d\n", msg->cmd);
		break;
	}
}


