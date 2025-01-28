/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt csb media
 */
#define SYS_LOG_DOMAIN "btsrv_adapter"
#include "btsrv_os_common.h"
#include "btsrv_inner.h"

#define CSB_SEND_DATA_BY_ACL	1

#define TEST_CODEC_ID		SBC_TYPE
#define TEST_SAMPLE_RATE	16
#define CSB_WORK_INTERVAL	5
#define CSB_DATA_LEN		128
#define CSB_VOLUME			6
#define CSB_LINK_KEY_SEED	"1234567890ABCDEF"
#define CSB_LT_ADDR			2
#define CSB_TRANSFER_RESERVE_SLOTS		(2*6)
#define CSB_INTERVAL		32
#define CSB_TRANSFER_INTERVAL	(CSB_TRANSFER_RESERVE_SLOTS + CSB_INTERVAL)
#define CSB_TIMEOUT			0x2710 /* CSB_TIMEOUT*0.625ms 6.25s */

static uint8_t csb_broadcase_runing;
static uint8_t csb_receive_runing;
static io_stream_t capture_stream;
static io_stream_t playback_stream;
static media_player_t *capture_player;
static media_player_t *playback_player;
static os_delayed_work csb_delaywork;
static uint8_t send_buf[CSB_DATA_LEN];
static uint8_t broadcast_addr[6] = {0x85, 0xBD, 0xEE, 0xFC, 0x4E, 0xF4};		/* Broadcast device bt address */
static uint8_t csb_receive_train;
static uint8_t csb_slave_timeout;
static struct bt_hci_evt_sync_train_receive csb_rx_train;

extern int con_get_csb_broadcast_pkt_num(void);

static void csb_delaywork_proc(os_work *work)
{
	int ret;

	if (!csb_broadcase_runing || !capture_stream) {
		return;
	}

	if (con_get_csb_broadcast_pkt_num() >= 4) {
		goto next_time;
	}

	ret = stream_get_length(capture_stream);
	if (ret >= CSB_DATA_LEN) {
		ret = stream_read(capture_stream, send_buf, CSB_DATA_LEN);
		if (ret > 0) {
#if CSB_SEND_DATA_BY_ACL
			printk(".");
			hostif_bt_csb_broadcase_by_acl(0, send_buf, CSB_DATA_LEN);
#else
			hostif_bt_csb_set_CSB_date(CSB_LT_ADDR, send_buf, CSB_DATA_LEN);
#endif
		}
	}

next_time:
	os_delayed_work_submit(&csb_delaywork, CSB_WORK_INTERVAL);
}

static void media_data_cb(uint8_t *data, uint16_t len)
{
	int ret;

	if (!csb_receive_runing || !playback_stream) {
		return;
	}

	printk(".");
	ret = stream_get_space(playback_stream);
	if (ret >= len) {
		stream_write(playback_stream, data, len);
	}
}

void csb_slave_receive_train_cb(struct bt_hci_evt_sync_train_receive *evt)
{
	SYS_LOG_INF(" ");
	memcpy(&csb_rx_train, evt, sizeof(csb_rx_train));
	csb_receive_train = 1;
}

void csb_slave_receive_data_cb(uint8_t *data, uint16_t len)
{
	media_data_cb(data, len);
}

void csb_slave_timeout_cb(uint8_t *mac, uint8_t lt_addr)
{
	csb_slave_timeout = 1;
	SYS_LOG_INF("lt_addr %d", lt_addr);
}

static io_stream_t leaudio_create_stream(bool capture)
{
	int ret = 0;
	io_stream_t ret_stream = NULL;

	struct buffer_t capture_buffer = {
		.length = media_mem_get_cache_pool_size(OUTPUT_DECODER, AUDIO_STREAM_MUSIC),
		.base = media_mem_get_cache_pool(OUTPUT_DECODER, AUDIO_STREAM_MUSIC),
		.cache_size = 8 * 68,
	};

	struct buffer_t play_buffer = {
		.length = media_mem_get_cache_pool_size(INPUT_PLAYBACK, AUDIO_STREAM_MUSIC),
		.base = media_mem_get_cache_pool(INPUT_PLAYBACK, AUDIO_STREAM_MUSIC),
		.cache_size = 8 * 68,
	};

	if (capture) {
		ret_stream = buffer_stream_create(&capture_buffer);
	} else {
		ret_stream = buffer_stream_create(&play_buffer);
	}

	if (!ret_stream) {
		SYS_LOG_INF("stream create failed");
		goto exit;
	}

	if (capture) {
		ret = stream_open(ret_stream, MODE_IN_OUT);
	} else {
		ret = stream_open(ret_stream, MODE_IN_OUT | MODE_READ_BLOCK);
	}

	if (ret) {
		stream_destroy(ret_stream);
		ret_stream = NULL;
		SYS_LOG_INF("stream open failed");
		goto exit;
	}

exit:
	SYS_LOG_INF("%p", ret_stream);
	return ret_stream;
}

static int csb_start_broadcast(void)
{
	hostif_bt_csb_set_broadcast_link_key_seed(CSB_LINK_KEY_SEED, strlen(CSB_LINK_KEY_SEED));
	os_sleep(10);
	hostif_bt_csb_set_reserved_lt_addr(CSB_LT_ADDR);
	os_sleep(10);
	hostif_bt_csb_set_CSB_broadcast(CSB_TRANSFER_RESERVE_SLOTS, CSB_LT_ADDR, CSB_TRANSFER_INTERVAL, CSB_TRANSFER_INTERVAL);
	os_sleep(10);
	hostif_bt_csb_write_sync_train_param(CSB_TRANSFER_INTERVAL, CSB_TRANSFER_INTERVAL, 0, CSB_TRANSFER_RESERVE_SLOTS);
	os_sleep(10);
	hostif_bt_csb_start_sync_train();
	os_sleep(10);
	return 0;
}

static int csb_stop_broadcast(void)
{
	hostif_bt_csb_set_CSB_broadcast(0, CSB_LT_ADDR, CSB_TRANSFER_INTERVAL, CSB_TRANSFER_INTERVAL);
	return 0;
}

static int csb_start_receive(void)
{
	hostif_bt_csb_set_broadcast_link_key_seed(CSB_LINK_KEY_SEED, strlen(CSB_LINK_KEY_SEED));
	os_sleep(10);
	hostif_bt_csb_set_reserved_lt_addr(CSB_LT_ADDR);
	os_sleep(10);

	csb_receive_train = 0;
	hostif_bt_csb_receive_sync_train(broadcast_addr, 0, CSB_TRANSFER_INTERVAL, CSB_TRANSFER_INTERVAL);

	while (!csb_receive_train) {
		os_sleep(10);
	}

	csb_slave_timeout = 0;
	hostif_bt_csb_set_CSB_receive(csb_rx_train.service_data, &csb_rx_train, 1600);
	os_sleep(10);

	return 0;
}

static int csb_stop_receive(void)
{
	if (!csb_slave_timeout) {
		hostif_bt_csb_set_CSB_receive(0, &csb_rx_train, CSB_TIMEOUT);
	}
	return 0;
}

void csb_broadcase(void)
{
	if (csb_receive_runing) {
		SYS_LOG_INF("csb receive is runing");
		return;
	}

	if (!csb_broadcase_runing) {
		csb_broadcase_runing = 1;
		csb_start_broadcast();
		os_delayed_work_init(&csb_delaywork, csb_delaywork_proc);
		os_delayed_work_submit(&csb_delaywork, CSB_WORK_INTERVAL);
	} else {
		csb_broadcase_runing = 0;
		csb_stop_broadcast();
		os_delayed_work_cancel(&csb_delaywork);
	}
}

void csb_receive(void)
{
	if (csb_broadcase_runing) {
		SYS_LOG_INF("csb broadcase is runing");
		return;
	}

	if (!csb_receive_runing) {
		/*TODO:  Used discovery to find broadcase device mac */
		csb_receive_runing = 1;
		csb_start_receive();
		/* Need set discoverable/connectable false */
	} else {
		csb_receive_runing = 0;
		csb_stop_receive();
	}
}
