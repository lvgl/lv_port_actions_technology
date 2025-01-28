/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "ap_record_private.h"
#include "tts_manager.h"

void record_tts_event_proc(struct app_msg *msg)
{
	struct record_app_t *record_app = record_get_app();

	if (!record_app)
		return;

	switch (msg->value) {
	case TTS_EVENT_START_PLAY:
		if (record_app->player) {
			record_stop_record();
		}
		break;
	case TTS_EVENT_STOP_PLAY:
		if (record_app->player) {
			record_start_record(&record_app->user_param);
		}
		break;
	default:
		break;
	}
}

void record_input_event_proc(struct app_msg *msg)
{
	struct record_app_t *record_app = record_get_app();

	if (!record_app)
		return;

	switch (msg->cmd) {
	case MSG_AP_RECORD_START:
		record_start_record(&record_app->user_param);
		break;
	case MSG_AP_RECORD_STOP:
		record_stop_record();
		break;
	default:
		break;
	}
}

void record_event_proc(struct app_msg *msg)
{
	struct record_app_t *record_app = record_get_app();

	if (!record_app)
		return;

	switch (msg->cmd) {
	case MSG_AP_RECORD_UPDATE_STATUS:
	if (msg->value == 0) {
		SYS_LOG_INF("record sppble stream disconnect\n");
		if (record_app->player) {
			record_stop_record();
		}

	} else {
		SYS_LOG_INF("record sppble stream connect\n");
	}
		break;
	default:
		break;
	}

}



