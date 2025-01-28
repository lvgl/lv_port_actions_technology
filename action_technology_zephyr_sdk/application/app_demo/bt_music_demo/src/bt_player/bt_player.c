/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt player
 */

#define LOG_MODULE_CUSTOMER

#include <stdlib.h>
#include <audio_system.h>
#include <audio_policy.h>
#include <volume_manager.h>
#include <media_mem.h>
#include <media_player.h>
#include <tts_manager.h>
#include <bt_manager.h>
#include <app_manager.h>
#include <ringbuff_stream.h>

#include "bt_player.h"

LOG_MODULE_REGISTER(btmusic, LOG_LEVEL_INF);

struct bt_player {
	media_player_t *player;
	io_stream_t bt_stream;

	/* user playing state */
	uint8_t user_playing;

	const btmusic_callback_t *play_cb;
};

static struct bt_player g_bt_player;

static io_stream_t _bt_player_create_inputstream(void)
{
	int ret = 0;
	io_stream_t input_stream = ringbuff_stream_create_ext(
			media_mem_get_cache_pool(INPUT_PLAYBACK, AUDIO_STREAM_MUSIC),
			media_mem_get_cache_pool_size(INPUT_PLAYBACK, AUDIO_STREAM_MUSIC));

	if (!input_stream) {
		return NULL;
	}

	ret = stream_open(input_stream, MODE_IN_OUT | MODE_READ_BLOCK | MODE_BLOCK_TIMEOUT);
	if (ret) {
		stream_destroy(input_stream);
		input_stream = NULL;
	}

	return	input_stream;
}

static int bt_player_start_play(void)
{
	struct bt_player *btplayer = &g_bt_player;
	media_init_param_t init_param;
	uint8_t codec_id = bt_manager_a2dp_get_codecid(BTSRV_DEVICE_PHONE);
	uint8_t sample_rate = bt_manager_a2dp_get_sample_rate(BTSRV_DEVICE_PHONE);

	if (btplayer->player) {
		if (btplayer->bt_stream) {
			bt_manager_set_codec(codec_id);
			bt_manager_set_stream(STREAM_TYPE_A2DP, btplayer->bt_stream);

			media_player_play(btplayer->player);
			SYS_LOG_INF("already open\n");
			return 0;
		}

		SYS_LOG_INF("reset player\n");
		media_player_stop(btplayer->player);
		media_player_close(btplayer->player);
		btplayer->player = NULL;
		os_sleep(200);
	}

#ifdef CONFIG_PLAYTTS
	tts_manager_wait_finished(false);
#endif

	SYS_LOG_INF("btmusic: codec_id: %d sample_rate %d\n", codec_id, sample_rate);

	memset(&init_param, 0, sizeof(media_init_param_t));
	/* set decoder format */
	if (codec_id == 0) {
		init_param.format = SBC_TYPE;
	} else if (codec_id == 2) {
		init_param.format = AAC_TYPE;
	} else {
		SYS_LOG_ERR("unknown code id %d", codec_id);
		return -EINVAL;
	}

	btplayer->bt_stream = _bt_player_create_inputstream();
	if (!btplayer->bt_stream) {
		SYS_LOG_ERR("stream create failed");
		return -ENOMEM;
	}

	init_param.type = MEDIA_SRV_TYPE_PLAYBACK;
	init_param.stream_type = AUDIO_STREAM_MUSIC;
	init_param.efx_stream_type = AUDIO_STREAM_MUSIC;
	init_param.sample_rate = sample_rate;
	/* input_stream will provide data to decoder */
	init_param.input_stream = btplayer->bt_stream;
	init_param.support_tws = 1;
	init_param.dumpable = 1;
	init_param.dsp_output = 0;
	if (audio_policy_get_out_audio_mode(init_param.stream_type) == AUDIO_MODE_STEREO) {
		init_param.channels = 2;
	} else {
		init_param.channels = 1;
	}

	btplayer->player = media_player_open(&init_param);
	if (!btplayer->player) {
		SYS_LOG_ERR("player open failed");
		stream_close(btplayer->bt_stream);
		stream_destroy(btplayer->bt_stream);
		btplayer->bt_stream = NULL;
		return -EINVAL;
	}

	bt_manager_set_codec(codec_id);
	bt_manager_set_stream(STREAM_TYPE_A2DP, btplayer->bt_stream);
#ifdef CONFIG_BT_AVRCP
	bt_manager_avrcp_sync_vol_to_remote(audio_system_get_stream_volume(AUDIO_STREAM_MUSIC));
#endif

	media_player_fade_in(btplayer->player, 60);
	media_player_play(btplayer->player);

	SYS_LOG_INF("btmusic: start\n");
	return 0;
}

static void bt_player_stop_play(void)
{
	struct bt_player *btplayer = &g_bt_player;

	if (!btplayer->player)
		return;

	media_player_fade_out(btplayer->player, 60);

	/** reserve time to fade out*/
	os_sleep(60);

	if (btplayer->bt_stream) {
		stream_close(btplayer->bt_stream);
	}

	bt_manager_set_stream(STREAM_TYPE_A2DP, NULL);

	media_player_stop(btplayer->player);
	media_player_close(btplayer->player);
	btplayer->player = NULL;

	if (btplayer->bt_stream) {
		stream_destroy(btplayer->bt_stream);
		btplayer->bt_stream = NULL;
	}

	SYS_LOG_INF("btmusic: stop\n");
}

void bt_player_bt_event_proc(struct app_msg *msg)
{
	SYS_LOG_INF("bt cmd %d\n", msg->cmd);

	switch (msg->cmd) {
	case BT_A2DP_STREAM_START_EVENT: /* 6 */
	{
		SYS_LOG_INF("STREAM_START\n");
		bt_player_start_play();
	#ifdef CONFIG_BT_AVRCP
		bt_manager_avrcp_get_id3_info();
	#endif
		break;
	}
	case BT_A2DP_STREAM_SUSPEND_EVENT: /* 7 */
	{
		SYS_LOG_INF("STREAM_SUSPEND\n");
		bt_player_stop_play();
		break;
	}
	case BT_AVRCP_PLAYBACK_STATUS_CHANGED_EVENT: /* 31 */
	{
		int param = *(int *)(msg->ptr);

		if (param == BT_STATUS_PAUSED) {
		} else if (param == BT_STATUS_PLAYING) {
			bt_manager_a2dp_check_state();
		}
		break;
	}
	case BT_AVRCP_TRACK_CHANGED_EVENT: /* 32 */
	{
	#ifdef CONFIG_BT_AVRCP
		bt_manager_avrcp_get_id3_info();
	#endif
		break;
	}
	case BT_AVRCP_UPDATE_PLAYBACK_POS: /* 34 */
	{
		uint32_t pos = *(uint32_t *)msg->ptr;

		if (g_bt_player.play_cb && g_bt_player.play_cb->set_play_pos) {
			g_bt_player.play_cb->set_play_pos(pos);
		}
		break;
	}
	case BT_AVRCP_UPDATE_ID3_INFO_EVENT: /* 33 */
	{
		struct bt_id3_info *pinfo = (struct bt_id3_info *)msg->ptr;

		if (g_bt_player.play_cb && g_bt_player.play_cb->set_play_info) {
			g_bt_player.play_cb->set_play_info(pinfo->item[0].data, pinfo->item[1].data, atoi(pinfo->item[4].data));
		}
		break;
	}

	case BT_RMT_VOL_SYNC_EVENT: /* 39 */
	{
		audio_system_set_stream_volume(AUDIO_STREAM_MUSIC, msg->value);

		if (g_bt_player.player) {
			media_player_set_volume(g_bt_player.player, msg->value, msg->value);
		}

		if (g_bt_player.play_cb && g_bt_player.play_cb->set_volume) {
			g_bt_player.play_cb->set_volume(msg->value);
		}
		break;
	}
	default:
		break;
	}
}

void btmusic_view_input_event_proc(struct app_msg *msg)
{
	SYS_LOG_INF("input cmd %d\n", msg->cmd);

	switch (msg->cmd) {
	case MSG_BT_PLAY_PAUSE: /* 208 */
	{
		if (g_bt_player.player) {
		#ifdef CONFIG_BT_AVRCP
			bt_manager_avrcp_pause();
		#endif

			/* FIXME: just wait until receiveing BT_A2DP_STREAM_SUSPEND_EVENT
			 * if not required stop immediately.
			 */
			if (msg->value) {
				bt_player_stop_play();
			}
		}
		break;
	}

	case MSG_BT_PLAY_RESUME: /* 209 */
	{
	#ifdef CONFIG_BT_AVRCP
		SYS_LOG_INF("bt_manager_avrcp_play\n");
		bt_manager_avrcp_play();
	#endif
		bt_manager_a2dp_check_state();
		break;
	}

	case MSG_BT_PLAY_NEXT: /* 209 */
	{
	#ifdef CONFIG_BT_AVRCP
		bt_manager_avrcp_play_next();
	#endif
		break;
	}

	case MSG_BT_PLAY_PREVIOUS: /* 210 */
	{
	#ifdef CONFIG_BT_AVRCP
		bt_manager_avrcp_play_previous();
	#endif
		break;
	}

	case MSG_BT_PLAY_VOLUP: /* 211 */
	{
		system_volume_up(AUDIO_STREAM_MUSIC, 1);
		break;
	}

	case MSG_BT_PLAY_VOLDOWN: /* 212 */
	{
		system_volume_down(AUDIO_STREAM_MUSIC, 1);
		break;
	}

	case MSG_BT_PLAY_VOLSYNC: /* 213 */
	{
		system_volume_set(AUDIO_STREAM_MUSIC, msg->value, 0);
		break;
	}

	case MSG_BT_PLAY_SYNC_REMOTE_STATE: {
	#ifdef CONFIG_BT_AVRCP
		bt_manager_avrcp_get_id3_info();
	#endif

		if (!g_bt_player.player) {
			SYS_LOG_INF("check a2dp state");
			bt_manager_a2dp_check_state();
		}
		break;
	}

	default:
		break;
	}
}

void btmusic_start(void)
{
	SYS_LOG_INF("start %p %d", g_bt_player.player, g_bt_player.user_playing);

	if (!g_bt_player.user_playing || !g_bt_player.player) {
		struct app_msg msg = {
			.type = MSG_INPUT_EVENT,
			.cmd = MSG_BT_PLAY_RESUME,
			.reserve = 1,
		};

		g_bt_player.user_playing = 1;

		send_async_msg(app_manager_get_current_app(), &msg);
	}
}

void btmusic_stop(bool forced)
{
	SYS_LOG_INF("stop %p %d, force %d", g_bt_player.player, g_bt_player.user_playing, forced);

	if (g_bt_player.user_playing || g_bt_player.player) {
		struct app_msg msg = {
			.type = MSG_INPUT_EVENT,
			.cmd = MSG_BT_PLAY_PAUSE,
			.reserve = 1,
			.value = forced,
		};

		g_bt_player.user_playing = 0;

		send_async_msg(app_manager_get_current_app(), &msg);
	}
}

void btmusic_play_next(void)
{
	struct app_msg msg = {
		.type = MSG_INPUT_EVENT,
		.cmd = MSG_BT_PLAY_NEXT,
		.reserve = 1,
	};

	SYS_LOG_INF("play next");

	send_async_msg(app_manager_get_current_app(), &msg);
}

void btmusic_play_prev(void)
{
	struct app_msg msg = {
		.type = MSG_INPUT_EVENT,
		.cmd = MSG_BT_PLAY_PREVIOUS,
		.reserve = 1,
	};

	SYS_LOG_INF("play prev");

	send_async_msg(app_manager_get_current_app(), &msg);
}

void btmusic_vol_adjust(bool is_add)
{
	struct app_msg msg = {
		.type = MSG_INPUT_EVENT,
		.cmd = is_add ? MSG_BT_PLAY_VOLUP : MSG_BT_PLAY_VOLDOWN,
		.reserve = 1,
	};

	SYS_LOG_INF("is_add %d\n", is_add);

	send_async_msg(app_manager_get_current_app(), &msg);
}

void btmusic_vol_sync(int music_vol)
{
	struct app_msg msg = {
		.type = MSG_INPUT_EVENT,
		.cmd = MSG_BT_PLAY_VOLSYNC,
		.reserve = 1,
		.value = music_vol,
	};

	SYS_LOG_INF("vol %d\n", music_vol);

	send_async_msg(app_manager_get_current_app(), &msg);
}

void btmusic_check_remote_state(void)
{
	struct app_msg msg = {
		.type = MSG_INPUT_EVENT,
		.cmd = MSG_BT_PLAY_SYNC_REMOTE_STATE,
		.reserve = 1,
	};

	send_async_msg(app_manager_get_current_app(), &msg);
}

bool btmusic_get_play_state(void)
{
	return g_bt_player.player ? true : false;
}

void btmusic_set_play_callback(const btmusic_callback_t *callback)
{
	g_bt_player.play_cb = callback;
}
