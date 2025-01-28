/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager a2dp profile.
 */
#define SYS_LOG_NO_NEWLINE
#define SYS_LOG_DOMAIN "bt manager"

#include <os_common_api.h>
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>
#include <bt_manager.h>
#include <power_manager.h>
#include <app_manager.h>
#include <sys_event.h>
#include <mem_manager.h>
#include "bt_manager_inner.h"
#include <assert.h>
#include "btservice_api.h"

#ifdef CONFIG_MEDIA
#include <media_type.h>
#endif

struct bt_manager_hfp_sco_info_t {
	uint8_t hfp_codec_id;
	uint8_t hfp_sample_rate;
};

static struct bt_manager_hfp_sco_info_t hfp_sco_manager;

static void _bt_manager_sco_callback(btsrv_hfp_event_e event, uint8_t *param, int param_size)
{
	switch (event) {
	case BTSRV_SCO_DATA_INDICATED:
	{
		int ret = 0;
		static uint8_t print_cnt = 0;

		bt_manager_stream_pool_lock();
		io_stream_t bt_stream = bt_manager_get_stream(STREAM_TYPE_SCO);

		if (!bt_stream) {
			bt_manager_stream_pool_unlock();
			if (print_cnt == 0) {
				SYS_LOG_WRN("stream is null\n");
			}
			print_cnt++;
			break;
		}

		if (stream_get_space(bt_stream) < param_size) {
			bt_manager_stream_pool_unlock();
			if (print_cnt == 0) {
				SYS_LOG_WRN("stream is full\n");
			}
			print_cnt++;
			break;
		}

		ret = stream_write(bt_stream, param, param_size);
		if (ret != param_size) {
			SYS_LOG_WRN("write %d error %d\n", param_size, ret);
		}
		bt_manager_stream_pool_unlock();
		print_cnt = 0;
		break;
	}
	default:
	break;
	}
}

int bt_manager_hfp_sco_start(void)
{
	return btif_sco_start(&_bt_manager_sco_callback);
}

int bt_manager_hfp_sco_stop(void)
{
	return btif_sco_stop();
}

int bt_manager_sco_get_codecid(void)
{
#ifdef CONFIG_MEDIA
	if (!hfp_sco_manager.hfp_codec_id) {
		hfp_sco_manager.hfp_codec_id = CVSD_TYPE;
	}
#endif
	SYS_LOG_INF("codec_id %d\n", hfp_sco_manager.hfp_codec_id);
	return hfp_sco_manager.hfp_codec_id;
}

int bt_manager_sco_get_sample_rate(void)
{
	if (!hfp_sco_manager.hfp_sample_rate) {
		hfp_sco_manager.hfp_sample_rate = 8;
	}
	SYS_LOG_INF("sample rate %d\n", hfp_sco_manager.hfp_sample_rate);
	return hfp_sco_manager.hfp_sample_rate;
}

/**add for asqt btcall simulator*/
void bt_manager_sco_set_codec_info(uint8_t codec_id, uint8_t sample_rate)
{
#ifdef CONFIG_MEDIA
	assert((codec_id == CVSD_TYPE && sample_rate == 8)
			|| (codec_id == MSBC_TYPE && sample_rate == 16));
#endif
	hfp_sco_manager.hfp_codec_id = codec_id;
	hfp_sco_manager.hfp_sample_rate = sample_rate;
}

int bt_manager_hfp_sco_init(void)
{
	memset(&hfp_sco_manager, 0, sizeof(struct bt_manager_hfp_sco_info_t));
	return 0;
}

