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

static int _btcall_init(void)
{
	if (p_btcall_app)
		return 0;

	p_btcall_app = app_mem_malloc(sizeof(struct btcall_app_t));
	if (!p_btcall_app) {
		SYS_LOG_ERR("malloc fail!\n");
		return -ENOMEM;
	}

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

	app_mem_free(p_btcall_app);
	p_btcall_app = NULL;

#ifdef CONFIG_PROPERTY
	property_flush_req(NULL);
#endif

exit:
	SYS_LOG_INF("finished\n");
}

struct btcall_app_t *btcall_get_app(void)
{
	return p_btcall_app;
}


void btcall_proc_start(void)
{
	_btcall_init();
}

void btcall_proc_exit(void)
{
	_btcall_exit();
}

