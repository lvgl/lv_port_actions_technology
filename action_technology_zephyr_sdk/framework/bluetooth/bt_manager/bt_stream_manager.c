/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt stream manager.
 */
#define SYS_LOG_DOMAIN "bt manager"
#include <os_common_api.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <stream.h>
#include <btservice_api.h>
#include <bt_manager.h>
#include "bt_manager_inner.h"

static OS_MUTEX_DEFINE(bt_stream_pool_mutex);

static io_stream_t bt_stream_pool[STREAM_TYPE_MAX_NUM] = {NULL};

static bool bt_stream_pool_enable[STREAM_TYPE_MAX_NUM] = {0};

void bt_manager_stream_pool_lock(void)
{
	os_mutex_lock(&bt_stream_pool_mutex, OS_FOREVER);
}

void bt_manager_stream_pool_unlock(void)
{
	os_mutex_unlock(&bt_stream_pool_mutex);
}

int bt_manager_stream_enable(int type, bool enable)
{
	if (type >= STREAM_TYPE_MAX_NUM) {
		return -EINVAL;
	}

	bt_manager_stream_pool_lock();

	bt_stream_pool_enable[type] = enable;

	bt_manager_stream_pool_unlock();

	return 0;
}

bool bt_manager_stream_is_enable(int type)
{

	if (type >= STREAM_TYPE_MAX_NUM) {
		return -EINVAL;
	}

	return bt_stream_pool_enable[type];
}


int bt_manager_set_stream(int type, io_stream_t stream)
{
	SYS_LOG_INF(" stream : %p type %d\n", stream, type);
	if (type >= STREAM_TYPE_MAX_NUM) {
		return -EINVAL;
	}

	bt_manager_stream_pool_lock();

	bt_stream_pool[type] = stream;

#ifdef CONFIG_TWS_MONO_MODE
	if (type == STREAM_TYPE_LOCAL ||
		(type == STREAM_TYPE_A2DP && bt_manager_tws_get_dev_role() == BTSRV_TWS_SLAVE)) {
		btif_tws_set_input_stream(stream);
	}
#else
	if(type != STREAM_TYPE_SCO)
		btif_tws_set_input_stream(stream);
	else
		btif_tws_set_sco_input_stream(stream);
#endif

	bt_manager_stream_pool_unlock();

#ifdef CONFIG_BT_BLE
	/**notify ble adjust interval*/
	if (type == STREAM_TYPE_A2DP) {
		bt_manager_ble_a2dp_play_notify((stream ? true : false));
	} else if (type == STREAM_TYPE_SCO) {
		bt_manager_ble_hfp_play_notify((stream ? true : false));
	}
#endif

	return 0;
}

/**this function must call in stream pool lock */
io_stream_t bt_manager_get_stream(int type)
{
	if (type >= STREAM_TYPE_MAX_NUM) {
		return NULL;
	}

	return bt_stream_pool[type];
}
