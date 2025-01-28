/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt call ring
 */

#define LOG_MODULE_CUSTOMER

#include "btcall.h"
#include "tts_manager.h"

LOG_MODULE_DECLARE(btcall, LOG_LEVEL_INF);

enum {
	STATE_RING_INIT,
	STATE_RING_READY,
	STATE_RING_PLAYING,
	STATE_RING_DONE,
};

typedef struct {
	uint8_t current_index;
	uint8_t phone_num_cnt;
	uint8_t phone_num[32];
} btcall_ring_info_t;

typedef struct {
	uint8_t state;
	btcall_ring_info_t *ring_info;
	struct thread_timer call_ring_timer;
} btcall_ring_context_t;

static btcall_ring_context_t *ring_manager_context;

static btcall_ring_context_t *_btcall_ring_get_manager(void)
{
	return ring_manager_context;
}

static int _btcall_ring_get_file_name(uint8_t num, uint8_t *file_name, uint8_t file_name_len)
{
	if ((num >= '0') && (num <= '9')) {
		snprintf(file_name, file_name_len, "%c.act", num);
	} else if (num == 0xff) {
		snprintf(file_name, file_name_len, "%s.act", "callring");
	}

	return 0;
}

#ifdef CONFIG_PLAYTTS
static void _btcall_ring_tts_event_nodify(uint8_t *tts_id, uint32_t event)
{
	uint8_t file_name[13];
	btcall_ring_context_t *ring_manager = _btcall_ring_get_manager();
	btcall_ring_info_t *ring_info = ring_manager->ring_info;
	struct btcall_app_t *btcall = btcall_get_app();

	switch (event) {
	case TTS_EVENT_START_PLAY:
		break;
	case TTS_EVENT_STOP_PLAY:

		if (ring_info && ring_manager->state != STATE_RING_DONE) {
		#ifdef CONFIG_BT_CALL_FORCE_PLAY_PHONE_NUM
			if (ring_info->current_index == ring_info->phone_num_cnt - 1) {
				_btcall_ring_get_file_name(ring_info->phone_num[ring_info->current_index], file_name, sizeof(file_name));
			#ifdef CONFIG_PLAYTTS
				tts_manager_play(file_name, 0);
			#endif
				break;
			}
		#endif
			ring_info->current_index %= ring_info->phone_num_cnt;
			_btcall_ring_get_file_name(ring_info->phone_num[ring_info->current_index++], file_name, sizeof(file_name));
		#ifdef CONFIG_PLAYTTS
			tts_manager_play(file_name, 0);
		#endif
		} else {

			if (btcall->sco_established) {
				bt_manager_event_notify(BT_HFP_CALL_RING_STOP_EVENT, NULL, 0);
				ring_manager->state = STATE_RING_DONE;
			}
		}

		break;
	}
}
#endif

static void _btcall_ring_start(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	btcall_ring_context_t *ring_manager = _btcall_ring_get_manager();
	uint8_t file_name[13];

	_btcall_ring_get_file_name(
		ring_manager->ring_info->phone_num[ring_manager->ring_info->current_index++],
		file_name, sizeof(file_name));

#ifdef CONFIG_PLAYTTS
	tts_manager_add_event_lisener(_btcall_ring_tts_event_nodify);

	tts_manager_play(file_name, 0);
#endif

	ring_manager->state = STATE_RING_PLAYING;
}

int btcall_ring_start(uint8_t *phone_num, uint16_t phone_num_cnt)
{
	btcall_ring_info_t *ring_info = NULL;
	btcall_ring_context_t *ring_manager = _btcall_ring_get_manager();

	SYS_LOG_INF(" %s cnt %d\n", phone_num, phone_num_cnt);
	if (ring_manager->ring_info)
		return -EBUSY;

	ring_info = app_mem_malloc(sizeof(btcall_ring_info_t));
	if (!ring_info)
		return -ENOMEM;

	memset(ring_info, 0, sizeof(btcall_ring_info_t));

	if (phone_num_cnt > sizeof(ring_info->phone_num))
		phone_num_cnt = sizeof(ring_info->phone_num);

	ring_info->phone_num_cnt = phone_num_cnt;
	ring_info->current_index = 0;
	memcpy(ring_info->phone_num, phone_num, phone_num_cnt);
	ring_info->phone_num[ring_info->phone_num_cnt++] = 0xff;

	ring_manager->state = STATE_RING_READY;
	ring_manager->ring_info = ring_info;
#ifdef CONFIG_THREAD_TIMER
	thread_timer_start(&ring_manager->call_ring_timer, 800, 0);
#endif
	return 0;
}

void btcall_ring_stop(void)
{
	btcall_ring_context_t *ring_manager = _btcall_ring_get_manager();

	ring_manager->state = STATE_RING_DONE;
#ifdef CONFIG_THREAD_TIMER
	if (thread_timer_is_running(&ring_manager->call_ring_timer))
		thread_timer_stop(&ring_manager->call_ring_timer);
#endif
	if (ring_manager->ring_info) {
	#ifdef CONFIG_PLAYTTS
		tts_manager_stop(NULL);
		tts_manager_wait_finished(true);
	#endif
		app_mem_free(ring_manager->ring_info);
		ring_manager->ring_info = NULL;
	}
#ifdef CONFIG_PLAYTTS
	tts_manager_remove_event_lisener(_btcall_ring_tts_event_nodify);
#endif
	SYS_LOG_INF(" ok\n");
}

int btcall_ring_manager_init(void)
{
	if (ring_manager_context)
		return -EEXIST;

	ring_manager_context = app_mem_malloc(sizeof(btcall_ring_context_t));
	if (!ring_manager_context)
		return -ENOMEM;

	ring_manager_context->state = STATE_RING_INIT;
#ifdef CONFIG_THREAD_TIMER
	thread_timer_init(&ring_manager_context->call_ring_timer, _btcall_ring_start, NULL);
#endif
	return 0;
}

int btcall_ring_manager_deinit(void)
{
	btcall_ring_stop();
	if (ring_manager_context) {
		if (ring_manager_context->ring_info) {
			app_mem_free(ring_manager_context->ring_info);
			ring_manager_context->ring_info = NULL;
		}
		app_mem_free(ring_manager_context);
		ring_manager_context = NULL;
	}

	return 0;
}

