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
#include <sys_event.h>
#include <bt_manager.h>
#include "bt_manager_inner.h"
#include "btservice_api.h"

#define A2DP_ENDPOINT_MAX		CONFIG_BT_MAX_BR_CONN

static const uint8_t a2dp_sbc_codec[] = {
	0x00,	/* BT_A2DP_AUDIO << 4 */
	0x00,	/* BT_A2DP_SBC */
	0xFF,	/* (SNK optional) 16000, 32000, (SNK mandatory)44100, 48000, mono, dual channel, stereo, join stereo */
	0xFF,	/* (SNK mandatory) Block length: 4/8/12/16, subbands:4/8, Allocation Method: SNR, Londness */
	0x02,	/* min bitpool */
#ifdef CONFIG_BT_A2DP_MAX_BITPOOL
	CONFIG_BT_A2DP_MAX_BITPOOL,	/* max bitpool */
#else
	0x35,
#endif
};

static const uint8_t a2dp_aac_codec[] = {
	0x00,	/* BT_A2DP_AUDIO << 4 */
	0x02,	/* BT_A2DP_MPEG2 */
	0xF0,	/* MPEG2 AAC LC, MPEG4 AAC LC, MPEG AAC LTP, MPEG4 AAC Scalable */
	0x01,	/* Sampling Frequecy 44100 */
	0x8F,	/* Sampling Frequecy 48000, channels 1, channels 2 */
	0xFF,	/* VBR, bit rate */
	0xFF,	/* bit rate */
	0xFF	/* bit rate */
};

#ifdef CONFIG_BT_A2DP_TRS
static const uint8_t a2dp_trs_sbc_codec_user[] = {
	0x00, /* BT_A2DP_AUDIO << 4 */
	0x00, /* BT_A2DP_SBC */
	0x21, /* (((BT_A2DP_SBC_48000 | BT_A2DP_SBC_44100) << 4) | (BT_A2DP_SBC_JOINT_STEREO)) */
	0xFF, /* (SNK mandatory) Block length: 4/8/12/16, subbands:4/8, Allocation Method: SNR, Londness */
	0x02, /* min bitpool */
#if defined(CONFIG_BT_A2DP_MAX_BITPOOL) && (CONFIG_BT_A2DP_MAX_BITPOOL < 48)
	CONFIG_BT_A2DP_MAX_BITPOOL, /* max bitpool */
#else
	48, /* max bitpool */
#endif
};

static const uint8_t a2dp_trs_aac_codec_user[] = {
	0x00,	/* BT_A2DP_AUDIO << 4 */
	0x02,	/* BT_A2DP_MPEG2 */
	//0xF0,	/* MPEG2 AAC LC, MPEG4 AAC LC, MPEG AAC LTP, MPEG4 AAC Scalable */
	0x80,	/* MPEG2 AAC LC*/
	0x01,	/* Sampling Frequecy 44100 */
	0x8F,	/* Sampling Frequecy 48000, channels 1, channels 2 */
	//0x84,	/* VBR, bit rate <= 320kbps*/
	//0xE2,	/* bit rate */
	//0x00	/* bit rate */
	0x81,	/* VBR, bit rate <= 128kbps*/
	0xF4,	/* bit rate */
	0x00	/* bit rate */
};
#endif


struct a2dp_codec_info {
	uint8_t codec_id;
	uint8_t sample_rate;
	uint8_t max_bitpool;
};

enum {
	A2DP_CODEC_INFO_PHONE = 0,
	A2DP_CODEC_INFO_TRS   = 1,
	A2DP_CODEC_INFO_MAX,
};

static struct a2dp_codec_info s_codec_info[A2DP_CODEC_INFO_MAX];

static void _bt_manager_a2dp_callback(uint16_t hdl, btsrv_a2dp_event_e event, void *packet, int size)
{
	switch (event) {
	case BTSRV_A2DP_STREAM_OPENED:
	{
		SYS_LOG_INF("stream opened\n");
	}
	break;
	case BTSRV_A2DP_STREAM_CLOSED:
	{
		SYS_LOG_INF("stream closed\n");
		bt_manager_event_notify(BT_A2DP_STREAM_SUSPEND_EVENT, NULL, 0);
	}
	break;
	case BTSRV_A2DP_STREAM_STARED:
	{
		SYS_LOG_INF("stream started\n");
		bt_manager_event_notify(BT_A2DP_STREAM_START_EVENT, NULL, 0);
	}
	break;
	case BTSRV_A2DP_STREAM_SUSPEND:
	{
		SYS_LOG_INF("stream suspend\n");
		bt_manager_event_notify(BT_A2DP_STREAM_SUSPEND_EVENT, NULL, 0);
	}
	break;
	case BTSRV_A2DP_DATA_INDICATED:
	{
		static uint8_t print_cnt;
		int ret = 0;

		bt_manager_stream_pool_lock();
		io_stream_t bt_stream = bt_manager_get_stream(STREAM_TYPE_A2DP);

		if (!bt_stream) {
			bt_manager_stream_pool_unlock();
			if (print_cnt == 0) {
				SYS_LOG_INF("stream is null\n");
			}
			print_cnt++;
			break;
		}
		if (stream_get_space(bt_stream) < size) {
			bt_manager_stream_pool_unlock();
			if (print_cnt == 0) {
				SYS_LOG_WRN(" stream is full\n");
			}
			print_cnt++;
			break;
		}

		ret = stream_write(bt_stream, packet, size);
		if (ret != size) {
			if (print_cnt == 0) {
				SYS_LOG_WRN("write %d error %d\n", size, ret);
			}
			print_cnt++;
			bt_manager_stream_pool_unlock();
			break;
		}
		bt_manager_stream_pool_unlock();
		print_cnt = 0;
		break;
	}
	case BTSRV_A2DP_CODEC_INFO:
	{
		uint8_t *codec_info = (uint8_t *)packet;

		if (bt_mgr_check_dev_type(BTSRV_DEVICE_PHONE, hdl)) {
			s_codec_info[A2DP_CODEC_INFO_PHONE].codec_id = codec_info[0];
			s_codec_info[A2DP_CODEC_INFO_PHONE].sample_rate = codec_info[1];
			s_codec_info[A2DP_CODEC_INFO_PHONE].max_bitpool = codec_info[2];
		} else if (bt_mgr_check_dev_type(BTSRV_DEVICE_PLAYER, hdl)) {
			s_codec_info[A2DP_CODEC_INFO_TRS].codec_id = codec_info[0];
			s_codec_info[A2DP_CODEC_INFO_TRS].sample_rate = codec_info[1];
			s_codec_info[A2DP_CODEC_INFO_TRS].max_bitpool = codec_info[2];
		}
		break;
	}
	case BTSRV_A2DP_CONNECTED:
		break;
	case BTSRV_A2DP_DISCONNECTED:
		if (bt_mgr_check_dev_type(BTSRV_DEVICE_PHONE, hdl)) {
			s_codec_info[A2DP_CODEC_INFO_PHONE].codec_id = 0;
			s_codec_info[A2DP_CODEC_INFO_PHONE].sample_rate = 44;
			s_codec_info[A2DP_CODEC_INFO_PHONE].max_bitpool = 48;
		} else if (bt_mgr_check_dev_type(BTSRV_DEVICE_PLAYER, hdl)) {
			s_codec_info[A2DP_CODEC_INFO_TRS].codec_id = 0;
			s_codec_info[A2DP_CODEC_INFO_TRS].sample_rate = 44;
			s_codec_info[A2DP_CODEC_INFO_TRS].max_bitpool = 48;
		}
		break;
	case BTSRV_A2DP_GET_INIT_DELAY_REPORT:
	{
		uint16_t *deplay_report = (uint16_t *)packet;

		/* initialize delay report, unit(1/10ms), can't block thread */
		*deplay_report = system_check_low_latencey_mode() ? 700 : 2000;
		break;
	}
	default:
		break;
	}
}

int bt_manager_a2dp_profile_start(void)
{
	struct btsrv_a2dp_start_param param;

	memset(&param, 0, sizeof(param));
	param.cb = &_bt_manager_a2dp_callback;
	param.sbc_codec = (uint8_t *)a2dp_sbc_codec;
	param.sbc_endpoint_num = A2DP_ENDPOINT_MAX;
	if (bt_manager_config_support_a2dp_aac()) {
		param.aac_codec = (uint8_t *)a2dp_aac_codec;
		param.aac_endpoint_num = A2DP_ENDPOINT_MAX;
	}
	param.a2dp_cp_scms_t = 1;
	param.a2dp_delay_report = 1;

#ifdef CONFIG_BT_A2DP_TRS
	param.a2dp_cp_scms_t = 0;		/* Is better just set for transmit */
	param.a2dp_delay_report = 0;	/* Is better just set for transmit */
	bt_manager_trs_a2dp_profile_start(&param);
	param.trs_sbc_codec = (uint8_t *)a2dp_trs_sbc_codec_user;

	if (bt_manager_config_support_a2dp_trs_aac()) {
		param.trs_aac_codec = (uint8_t *)a2dp_trs_aac_codec_user;
	} else {
		param.trs_aac_endpoint_num = 0;
	}
#endif

	return btif_a2dp_start((struct btsrv_a2dp_start_param *)&param);
}

int bt_manager_a2dp_profile_stop(void)
{
	return btif_a2dp_stop();
}

int bt_manager_a2dp_check_state(void)
{
	return btif_a2dp_check_state();
}

int bt_manager_a2dp_send_delay_report(uint16_t delay_time)
{
	return btif_a2dp_send_delay_report(delay_time);
}

int bt_manager_a2dp_disable(void)
{
	return btif_a2dp_disable();
}

int bt_manager_a2dp_enable(void)
{
	return btif_a2dp_enable();
}

int bt_manager_a2dp_get_codecid(uint8_t type)
{
	uint8_t codec_id = 0;

	if (type == BTSRV_DEVICE_PHONE) {
		codec_id = s_codec_info[A2DP_CODEC_INFO_PHONE].codec_id;
	} else if (type == BTSRV_DEVICE_PLAYER) {
		codec_id = s_codec_info[A2DP_CODEC_INFO_TRS].codec_id;
	}

	SYS_LOG_INF("codec_id %d\n", codec_id);
	return codec_id;
}

int bt_manager_a2dp_get_sample_rate(uint8_t type)
{
	uint8_t sample_rate = 44;

	if (type == BTSRV_DEVICE_PHONE) {
		sample_rate = s_codec_info[A2DP_CODEC_INFO_PHONE].sample_rate;
	} else if (type == BTSRV_DEVICE_PLAYER) {
		sample_rate = s_codec_info[A2DP_CODEC_INFO_TRS].sample_rate;
	}

	SYS_LOG_INF("sample_rate %d\n", sample_rate);
	return sample_rate;
}

int bt_manager_a2dp_get_max_bitpool(uint8_t type)
{
	uint8_t max_bitpool = 48;

	if (type == BTSRV_DEVICE_PHONE) {
		max_bitpool = s_codec_info[A2DP_CODEC_INFO_PHONE].max_bitpool;
	} else if (type == BTSRV_DEVICE_PLAYER) {
		max_bitpool = s_codec_info[A2DP_CODEC_INFO_TRS].max_bitpool;
	}

	SYS_LOG_INF("max_bitpool %d\n", max_bitpool);
	return max_bitpool;
}
