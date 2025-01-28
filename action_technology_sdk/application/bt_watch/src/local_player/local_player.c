/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief lccal player
 */

#include "local_player.h"
#include "tts_manager.h"

#ifdef CONFIG_BT_TRANSMIT
#include "../bt_transmit/bt_transmit.h"
#endif
#define ENABLE_LCPLAYER_FADEIN_FADEOUT 1
#define toupper(a)  \
	(((a) >= 'a' && (a) <= 'z') ? (a)-('a'-'A'):(a))

int parse_match_extension_type(const char *url, uint8_t *match_str)
{
	int i;
	uint32_t str_len, cmp_len;
	uint8_t *src_str = (uint8_t *)url;

	if (*src_str  == '\0') {
		return false;
	}

	str_len = strlen(url);
	cmp_len = strlen(match_str);

	if (str_len < cmp_len) {
		return false;
	}

	for (i = 0; i < strlen(match_str); i++) {
		if (toupper(match_str[i]) != toupper(src_str[str_len - cmp_len + i])) {
			break;
		}
	}

	if (i == strlen(match_str)) {
		return true;
	}

	return false;
}

uint8_t get_music_file_type(const char *url)
{
#ifdef CONFIG_DECODER_M4A
	if (parse_match_extension_type(url, ".aac") || 
		parse_match_extension_type(url, ".m4a"))
		return M4A_TYPE;
#endif

#ifdef CONFIG_DECODER_WAV
	if (parse_match_extension_type(url, ".wav"))
		return WAV_TYPE;
#endif

#ifdef CONFIG_DECODER_MP3
	if (parse_match_extension_type(url, ".mp3") ||
		parse_match_extension_type(url, ".mp2") ||
		parse_match_extension_type(url, ".mp1"))
		return MP3_TYPE;
#endif

#ifdef CONFIG_DECODER_APE
	if (parse_match_extension_type(url, ".ape"))
		return APE_TYPE;
#endif

#ifdef CONFIG_DECODER_WMA
	if (parse_match_extension_type(url, ".wma"))
		return WMA_TYPE;
#endif

#ifdef CONFIG_DECODER_FLAC
	if (parse_match_extension_type(url, ".flac") || parse_match_extension_type(url, ".fla"))
		return FLA_TYPE;
#endif

	return UNSUPPORT_TYPE;
}

static void _mplayer_cleanup(struct local_player_t *lcplayer)
{
	if (!lcplayer)
		return;

	if (lcplayer->player) {
		media_player_stop(lcplayer->player);
		media_player_close(lcplayer->player);
		lcplayer->player = NULL;
	}

	if (lcplayer->file_stream) {
		stream_close(lcplayer->file_stream);
		stream_destroy(lcplayer->file_stream);
		lcplayer->file_stream = NULL;
	}

	if (lcplayer) {
		app_mem_free(lcplayer);
		lcplayer = NULL;
	}
}

int mplayer_update_breakpoint(struct local_player_t *lcplayer, media_breakpoint_info_t *bp_info)
{

	if (!lcplayer || !lcplayer->player) {
		SYS_LOG_ERR("player is null\n");
		return -EFAULT;
	}
	if (media_player_get_breakpoint(lcplayer->player, bp_info)) {
		SYS_LOG_ERR("bp failed\n");
		return -EINVAL;
	}

	if (bp_info->time_offset < 0) {
		SYS_LOG_ERR("bp invalid\n");
	}

	SYS_LOG_DBG("bp time_offset=%d, file_offset=%d\n",
			bp_info->time_offset, bp_info->file_offset);
	return 0;
}

int mplayer_update_media_info(struct local_player_t *lcplayer, media_info_t *info)
{
	if (!lcplayer || !lcplayer->player) {
		SYS_LOG_ERR("player is null\n");
		return -EFAULT;
	}
	if (media_player_get_mediainfo(lcplayer->player, info)) {
		SYS_LOG_ERR("get media info failed\n");
		return -EINVAL;
	}

	if (info->total_time < 0) {
		SYS_LOG_ERR("media info invalid\n");
	}

	SYS_LOG_INF("total_time=%d, sample_rate=%d,\
				channels=%d, avg_bitrate=%d\n",
				info->total_time, info->sample_rate,
				info->channels, info->avg_bitrate);
	return 0;
}

struct local_player_t *mplayer_start_play(struct lcplay_param *lcparam)
{
	media_init_param_t init_param;

	if (!lcparam->url || lcparam->url[0] == 0)
		return NULL;

	SYS_LOG_INF("url:%s\n", lcparam->url);

	struct local_player_t *lcplayer = app_mem_malloc(sizeof(struct local_player_t));
	if (!lcplayer) {
		SYS_LOG_ERR("malloc faild\n");
		return NULL;
	}

#ifdef CONFIG_PLAYTTS
	tts_manager_wait_finished(false);
#endif

#ifdef CONFIG_LOOP_FSTREAM
	if (lcparam->play_mode == 1)
		lcplayer->file_stream = loop_fstream_create((void *)lcparam->url);
	else
#endif
	lcplayer->file_stream = file_stream_create((void *)lcparam->url);
	if (!lcplayer->file_stream) {
		SYS_LOG_ERR("stream create failed (%s)\n", lcparam->url);
		goto err_exit;
	}

	if (stream_open(lcplayer->file_stream, MODE_IN)) {
		SYS_LOG_ERR("stream open failed (%s)\n", lcparam->url);
		stream_destroy(lcplayer->file_stream);
		lcplayer->file_stream = NULL;
		goto err_exit;
	}

	memset(&init_param, 0, sizeof(media_init_param_t));

	init_param.type = MEDIA_SRV_TYPE_PARSER_AND_PLAYBACK;
	init_param.stream_type = AUDIO_STREAM_LOCAL_MUSIC;
	init_param.efx_stream_type = AUDIO_STREAM_LOCAL_MUSIC;
	init_param.format = get_music_file_type(lcparam->url);
	init_param.input_stream = lcplayer->file_stream;
	init_param.output_stream = NULL;
	init_param.event_notify_handle = lcparam->play_event_callback;
	init_param.support_tws = 1;
	init_param.dumpable = 1;
	init_param.dsp_output = 1;

	init_param.bp_file_offset = lcparam->bp.file_offset;
	init_param.bp_time_offset = lcparam->bp.time_offset;
	if(lcparam->seek_time != 0)
	{
		init_param.bp_file_offset = -1;
		init_param.bp_time_offset = lcparam->seek_time;
	}

#ifdef CONFIG_BT_TRANSMIT
	bt_transmit_catpure_pre_start();
#endif
	/* open service */
	lcplayer->player = media_player_open(&init_param);
	if (!lcplayer->player) {
		SYS_LOG_ERR("open failed\n");
		goto err_exit;
	}

#ifdef CONFIG_BT_TRANSMIT
	/*makesure bt transmit STEREO mode*/
	media_player_set_effect_output_mode(lcplayer->player, MEDIA_EFFECT_OUTPUT_DEFAULT);
	bt_transmit_catpure_start(init_param.output_stream, init_param.sample_rate, init_param.channels);
#endif

	/**seek when necessary**/
/*
	if (lcparam->seek_time != 0) {
		mplayer_seek(lcplayer, lcparam->seek_time, &lcparam->bp);
	}
*/

#if ENABLE_LCPLAYER_FADEIN_FADEOUT
	media_player_fade_in(lcplayer->player, 30);
#endif

	/* start play */
	media_player_play(lcplayer->player);

	SYS_LOG_INF("%p ok\n", lcplayer->player);

	return lcplayer;

err_exit:
	_mplayer_cleanup(lcplayer);
	return NULL;
}

void mplayer_stop_play(struct local_player_t *lcplayer)
{

	if (!lcplayer)
		return;

#if ENABLE_LCPLAYER_FADEIN_FADEOUT
	media_player_fade_out(lcplayer->player, 30);
	/** reserve time to fade out*/
	os_sleep(60);
#endif

#ifdef CONFIG_BT_TRANSMIT
	bt_transmit_catpure_stop();
#endif

	_mplayer_cleanup(lcplayer);

	SYS_LOG_INF("stopped\n");
}

int mplayer_seek(struct local_player_t *lcplayer, int seek_time, media_breakpoint_info_t *bp_info)
{
	media_seek_info_t info = { .file_offset = -1, };

	if (!lcplayer || !lcplayer->player) {
		SYS_LOG_ERR("player is null\n");
		return -EFAULT;
	}
	info.whence = AP_SEEK_SET;
	info.time_offset = seek_time;

	if (media_player_seek(lcplayer->player, &info)) {
		SYS_LOG_ERR("(%d) failed\n", seek_time);
		return -EINVAL;
	}
	SYS_LOG_INF("seek_time=%d,chunk_time_offset=%d, chunk_file_offset=%d\n",
			seek_time, info.chunk_time_offset, info.chunk_file_offset);

	/* update breakpoint from seeking feedback */
	if (info.chunk_time_offset >= 0) {
		bp_info->time_offset = info.chunk_time_offset;
		bp_info->file_offset = info.chunk_file_offset;
	} else {
		bp_info->time_offset += seek_time;
		bp_info->file_offset = -1;
		if (bp_info->time_offset < 0)
			bp_info->time_offset = 0;
	}
	return 0;
}

int mplayer_seek_relative(struct local_player_t *lcplayer, int seek_time, media_breakpoint_info_t *bp_info)
{
	media_seek_info_t info = { .file_offset = -1, };

	if (!lcplayer || !lcplayer->player) {
		SYS_LOG_ERR("player is null\n");
		return -EFAULT;
	}
	info.whence = AP_SEEK_CUR;
	info.time_offset = seek_time;

	SYS_LOG_INF("start seek relative %d\n", seek_time);
	if (media_player_seek(lcplayer->player, &info)) {
		SYS_LOG_ERR("(%d) failed\n", seek_time);
		return -EINVAL;
	}
	SYS_LOG_INF("end seek_time=%d,chunk_time_offset=%d, chunk_file_offset=%d\n",
			seek_time, info.chunk_time_offset, info.chunk_file_offset);

	/* update breakpoint from seeking feedback */
	if (info.chunk_time_offset >= 0) {
		bp_info->time_offset = info.chunk_time_offset;
		bp_info->file_offset = info.chunk_file_offset;
	} else {
		bp_info->time_offset += seek_time;
		bp_info->file_offset = -1;
		if (bp_info->time_offset < 0)
			bp_info->time_offset = 0;
	}
	return 0;
}

int mplayer_pause(struct local_player_t *lcplayer)
{
	if (!lcplayer || !lcplayer->player) {
		SYS_LOG_ERR("player is null\n");
		return -EFAULT;
	}
	return media_player_pause(lcplayer->player);
}

int mplayer_resume(struct local_player_t *lcplayer)
{
	if (!lcplayer || !lcplayer->player) {
		SYS_LOG_ERR("player is null\n");
		return -EFAULT;
	}
	return media_player_resume(lcplayer->player);
}

