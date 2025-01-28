/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <mem_manager.h>
#include <app_manager.h>
#include <srv_manager.h>
#include <volume_manager.h>
#include <msg_manager.h>
#include <thread_timer.h>
#include <media_player.h>
#include <audio_system.h>
#include <audio_policy.h>
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>
#include "app_defines.h"
#include "sys_manager.h"
#include "app_ui.h"

#include "ap_record_private.h"
#include "tts_manager.h"
#include "buffer_stream.h"
#include "ringbuff_stream.h"
#include "media_mem.h"

#define RECORD_SAMPLE_RATE_KH 16
static io_stream_t _record_create_uploadstream(record_stream_init_param *user_param)
{
	int ret = 0;
	io_stream_t upload_stream = NULL;

	upload_stream = record_upload_stream_create(user_param);
	if (!upload_stream) {
		goto exit;
	}

	ret = stream_open(upload_stream, MODE_IN_OUT);
	if (ret) {
		stream_destroy(upload_stream);
		upload_stream = NULL;
		goto exit;
	}

exit:
	SYS_LOG_INF(" %p\n", upload_stream);
	return upload_stream;
}

void record_start_record(record_stream_init_param *rec_param)
{
	struct record_app_t *record_app = record_get_app();
	media_init_param_t init_param;
	io_stream_t upload_stream = NULL;

	if (!record_app)
		return;

	if (!rec_param)
		return;

#ifdef CONFIG_PLAYTTS
	tts_manager_wait_finished(true);
#endif

	if (record_app->player) {
		SYS_LOG_INF(" already open\n");
		return;
	}

	memset(&init_param, 0, sizeof(media_init_param_t));

	upload_stream = _record_create_uploadstream(rec_param);

	if (!upload_stream) {
		goto err_exit;
	}

	init_param.type = MEDIA_SRV_TYPE_CAPTURE;
	init_param.stream_type = AUDIO_STREAM_ASR;
	init_param.support_tws = 0;

	init_param.capture_bit_rate = 16;
#ifdef CONFIG_TUYA_APP
	init_param.capture_bit_rate = 32;
#endif

	init_param.capture_format = OPUS_TYPE;
	init_param.capture_sample_rate_input = 16;
	init_param.capture_sample_rate_output = 16;
	init_param.capture_channels_input = 1;
	init_param.capture_channels_output = 1;
	init_param.capture_input_stream = NULL;
	init_param.capture_output_stream = upload_stream;

	record_app->player = media_player_open(&init_param);
	if (!record_app->player) {
		SYS_LOG_ERR("open failed\n");
		goto err_exit;
	}

	record_app->record_upload_stream = upload_stream;

	media_player_play(record_app->player);
	record_app->playing = TRUE;
	memcpy (&record_app->user_param, rec_param, sizeof(record_stream_init_param));
	SYS_LOG_INF("sucessed %p ", record_app->player);
	return;
err_exit:

	if (upload_stream) {
		stream_close(upload_stream);
		stream_destroy(upload_stream);
	}
	SYS_LOG_INF("failed\n");
}

void record_stop_record(void)
{
	struct record_app_t *record_app = record_get_app();

	if (!record_app)
		return;

	if (!record_app->player)
		return;

	if (record_app->record_upload_stream)
		stream_close(record_app->record_upload_stream);

	media_player_stop(record_app->player);

	media_player_close(record_app->player);

	SYS_LOG_INF(" %p ok\n", record_app->player);

	record_app->player = NULL;

	if (record_app->record_upload_stream) {
		stream_destroy(record_app->record_upload_stream);
		record_app->record_upload_stream = NULL;
	}
	record_app->playing = FALSE;
}

