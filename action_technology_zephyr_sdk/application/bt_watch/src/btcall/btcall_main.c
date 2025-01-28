/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt call main.
 */

#include "btcall.h"
#include "tts_manager.h"
#ifdef CONFIG_PROPERTY
#include "property_manager.h"
#endif

static struct btcall_app_t *p_btcall_app;

static void _btcall_ongoing_wait_esco_timeout_hander(struct thread_timer *ttimer, void *arg)
{
#ifdef CONFIG_UI_MANAGER
	btcall_exit_view();
#endif
}

static int _btcall_init(void)
{
	if (p_btcall_app)
		return 0;

	p_btcall_app = app_mem_malloc(sizeof(struct btcall_app_t));
	if (!p_btcall_app) {
		SYS_LOG_ERR("malloc fail!\n");
		return -ENOMEM;
	}

	thread_timer_init(&p_btcall_app->timeout_wait_esco,
			_btcall_ongoing_wait_esco_timeout_hander, p_btcall_app);

	bt_manager_set_stream_type(AUDIO_STREAM_VOICE);
#ifdef CONFIG_UI_MANAGER
	btcall_view_init();
#endif
	btcall_ring_manager_init();

	SYS_LOG_INF("finished\n");
	return 0;
}

static void _btcall_exit(void)
{
	if (!p_btcall_app)
		goto exit;

	thread_timer_stop(&p_btcall_app->timeout_wait_esco);

	bt_call_stop_play();

	btcall_ring_manager_deinit();
#ifdef CONFIG_UI_MANAGER
	btcall_view_deinit();
#endif
	app_mem_free(p_btcall_app);
	p_btcall_app = NULL;

#ifdef CONFIG_PROPERTY
	property_flush_req(NULL);
#endif

exit:
	SYS_LOG_INF("finished\n");

#ifdef CONFIG_LAUNCHER_APP
	launcher_restore_last_player();
#else
	if (memcmp(app_manager_get_current_app(), APP_ID_BTCALL, strlen(APP_ID_BTCALL)) == 0) {
		app_manager_thread_exit(APP_ID_BTCALL);
	}
#endif
}

struct btcall_app_t *btcall_get_app(void)
{
	return p_btcall_app;
}

#ifdef CONFIG_LAUNCHER_APP
void btcall_proc_start(void)
{
	_btcall_init();
}

void btcall_proc_exit(void)
{
	_btcall_exit();
}

#else /* CONFIG_LAUNCHER_APP */

static void btcall_main_loop(void *parama1, void *parama2, void *parama3)
{
	struct app_msg msg = {0};
	int result = 0;
	bool terminaltion = false;

	if (_btcall_init()) {
		SYS_LOG_ERR(" init failed");
		_btcall_exit();
		return;
	}

	while (!terminaltion) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			SYS_LOG_INF("type %d, value %d\n",msg.type, msg.value);
			switch (msg.type) {
				case MSG_EXIT_APP:
					_btcall_exit();
					terminaltion = true;
					break;
				case MSG_BT_EVENT:
					btcall_bt_event_proc(&msg);
					break;
				case MSG_INPUT_EVENT:
					btcall_input_event_proc(&msg);
					break;
				case MSG_KEY_INPUT:
					btcall_key_event_proc(msg.event);
					break;
				case MSG_TTS_EVENT:
					btcall_tts_event_proc(&msg);
					break;
				default:
					SYS_LOG_ERR("error: 0x%x \n", msg.type);
					break;
			}
			if (msg.callback != NULL)
				msg.callback(&msg, result, NULL);
		}
		thread_timer_handle_expired();
	}
}

APP_DEFINE(btcall, share_stack_area, sizeof(share_stack_area), CONFIG_APP_PRIORITY,
	   FOREGROUND_APP, NULL, NULL, NULL,
	   btcall_main_loop, NULL);

#endif /* CONFIG_LAUNCHER_APP */
