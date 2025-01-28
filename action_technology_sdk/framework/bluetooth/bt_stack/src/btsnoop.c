/** @file
 * @brief Bluetooth snoop log capture
 *
 * Copyright (c) 2019 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME bt_snoop
#include <logging/log.h>
LOG_MODULE_DECLARE(os, LOG_LEVEL_INF);

#include <zephyr.h>
#include <sys/printk.h>
#include <stdlib.h>
#include <string.h>

#define BTSNOOP_WRITE_SDCARD		0

#if BTSNOOP_WRITE_SDCARD
#include <stream.h>
#include <file_stream.h>
#include <fs_manager.h>
#include "media_mem.h"
#include "audio_system.h"

#define BTSNOOP_FILE_NAME			"SD:/sp.log"
#define SNOOP_WRITE_STACKSIZE		(1024*2)

static io_stream_t file_stream;
static uint16_t write_pos;
static uint16_t read_pos;
static uint16_t snoop_buff_len;
static uint8_t wait_write_sd;

static struct k_work_q snoop_write_q;
static struct k_delayed_work snoop_delaywork;
static uint8_t snoop_write_stack[SNOOP_WRITE_STACKSIZE] __aligned(4);
#endif

enum {
	kCommandPacket = 1,
	kAclPacket = 2,
	kScoPacket = 3,
	kEventPacket = 4
};

#define reverse_32(x) ((uint32_t) ((((x) >> 24) & 0xff) | \
					(((x) >> 8) & 0xff00) | \
					(((x) & 0xff00) << 8) | \
					(((x) & 0xff) << 24)))

#define SNOOP_SHELL_MODULE		"snoop"
#if BTSNOOP_WRITE_SDCARD
static uint8_t *snoop_buf;
#else
#define SNOOP_MAX_SIZE			100 //(1024*100)
static uint8_t snoop_buf[SNOOP_MAX_SIZE];
#endif
#define SNOOP_HEAD				"btsnoop\0\0\0\0\1\0\0\x3\xea"
#define SNOOP_CAP_PACKET_LEN	0	/* 0: Capture whole packet, other: Capture len */

static K_MUTEX_DEFINE(snoop_lock);
static uint8_t snoop_init_flag;
static uint32_t cap_len;

extern void printf(const char *fmt, ...);
static void btsnoop_write(const void *data, uint32_t length);

#if BTSNOOP_WRITE_SDCARD
static void snoop_write_delaywork(os_work *work)
{
	int wd_ret;
	uint16_t data_len, wd_len;

try_again:
	if (!file_stream) {
		return;
	}

	k_mutex_lock(&snoop_lock, K_FOREVER);
	data_len = cap_len;
	k_mutex_unlock(&snoop_lock);

	if ((read_pos + data_len) > snoop_buff_len) {
		wd_len = snoop_buff_len - read_pos;
		wd_ret = stream_write(file_stream, &snoop_buf[read_pos], wd_len);
		if (wd_ret != wd_len) {
			LOG_ERR("Write err %d, %d\n", wd_ret, wd_len);
			return;
		}

		wd_len = data_len - wd_len;
		wd_ret = stream_write(file_stream, &snoop_buf[0], wd_len);
		if (wd_ret != wd_len) {
			LOG_ERR("Write err %d, %d\n", wd_ret, wd_len);
			return;
		}

		read_pos = wd_len;
	} else {
		wd_ret = stream_write(file_stream, &snoop_buf[read_pos], data_len);
		if (wd_ret != data_len) {
			LOG_ERR("Write err %d, %d\n", wd_ret, data_len);
			return;
		}

		read_pos += data_len;
	}

	k_mutex_lock(&snoop_lock, K_FOREVER);
	cap_len -= data_len;
	data_len = cap_len;
	k_mutex_unlock(&snoop_lock);

	if (data_len) {
		goto try_again;
	} else {
		wait_write_sd = 0;
	}
}

static void btsnoop_init_sdcard_write(void)
{
	int ret;

	fs_unlink(BTSNOOP_FILE_NAME);

	file_stream = file_stream_create((void *)BTSNOOP_FILE_NAME);
	if (!file_stream) {
		LOG_ERR("Failed to create file!\n");
		return;
	}

	ret = stream_open(file_stream, MODE_IN_OUT | MODE_WRITE_BLOCK);
	if (ret) {
		LOG_ERR("stream open failed %d\n", ret);
		stream_destroy(file_stream);
		file_stream = NULL;
		return;
	}

	snoop_buf = media_mem_get_cache_pool(INPUT_PLAYBACK, AUDIO_STREAM_MUSIC);
	snoop_buff_len = media_mem_get_cache_pool_size(INPUT_PLAYBACK, AUDIO_STREAM_MUSIC);
	LOG_INF("Buff %p, len %d\n", snoop_buf, snoop_buff_len);

	write_pos = 0;
	read_pos = 0;
	cap_len = 0;

	k_work_q_start(&snoop_write_q, (k_thread_stack_t)snoop_write_stack, SNOOP_WRITE_STACKSIZE, 3);
	k_delayed_work_init(&snoop_delaywork, snoop_write_delaywork);
}

static void snoop_file_close(void)
{
	if (file_stream) {
		k_mutex_lock(&snoop_lock, K_FOREVER);
		stream_close(file_stream);
		stream_destroy(file_stream);
		file_stream = NULL;
		k_mutex_unlock(&snoop_lock);
	}

	LOG_INF("Close finish!\n");
}
#endif

static void btsnoop_write(const void *data, uint32_t length)
{
#if BTSNOOP_WRITE_SDCARD
	uint16_t wd_len, len;

	if (!file_stream) {
		return;
	}

	if ((write_pos + length) > snoop_buff_len) {
		wd_len = snoop_buff_len - write_pos;
		memcpy(&snoop_buf[write_pos], data, wd_len);
		len = length - wd_len;
		memcpy(&snoop_buf[0], &((uint8_t *)data)[wd_len], len);
		write_pos = len;
		cap_len += length;
	} else {
		memcpy(&snoop_buf[write_pos], data, length);
		write_pos += length;
		cap_len += length;
	}

	if (0 == wait_write_sd) {
		k_delayed_work_submit_to_queue(&snoop_write_q, &snoop_delaywork, 0);
		wait_write_sd = 1;
	}
#else
	memcpy(&snoop_buf[cap_len], data, length);
	cap_len += length;
#endif
}

int btsnoop_init(void)
{
	k_mutex_lock(&snoop_lock, K_FOREVER);
	memset(snoop_buf, 0, sizeof(snoop_buf));
	snoop_init_flag = 1;
	cap_len = 0;

#if BTSNOOP_WRITE_SDCARD
	btsnoop_init_sdcard_write();
#endif

	btsnoop_write(SNOOP_HEAD, 16);
	k_mutex_unlock(&snoop_lock);
	LOG_INF("Btsnoop init success!");

	return 0;
}

int btsnoop_write_packet(uint8_t type, const uint8_t *packet, bool is_received)
{
	uint32_t length_he = 0;
	uint32_t length;
	uint32_t flags;
	uint32_t drops = 0;
	u64_t time;

	if (snoop_init_flag == 0) {
		return 0;
	}

	switch (type) {
	case kCommandPacket:
		length_he = packet[2] + 4;
		flags = 2;
		break;
	case kAclPacket:
		length_he = (packet[3] << 8) + packet[2] + 5;
		flags = is_received;
		break;
	case kScoPacket:
		length_he = packet[2] + 4;
		flags = is_received;
		break;
	case kEventPacket:
		length_he = packet[1] + 3;
		flags = 3;
		break;
	}

	if (SNOOP_CAP_PACKET_LEN) {
		length_he = (length_he > SNOOP_CAP_PACKET_LEN) ? SNOOP_CAP_PACKET_LEN : length_he;
	}

	time = k_uptime_get_32();
	time *= 1000;
	time += 0x00E03AB44A676000;		/* January 1st 2000 AD */
	uint32_t time_hi = time >> 32;
	uint32_t time_lo = time & 0xFFFFFFFF;

	length = reverse_32(length_he);
	flags = reverse_32(flags);
	drops = reverse_32(drops);
	time_hi = reverse_32(time_hi);
	time_lo = reverse_32(time_lo);

	/* This function is called from different contexts. */
	k_mutex_lock(&snoop_lock, K_FOREVER);

#if BTSNOOP_WRITE_SDCARD
	if ((length_he + 24 + cap_len) < snoop_buff_len) {
#else
	if ((length_he + 24 + cap_len) < SNOOP_MAX_SIZE) {
#endif
		btsnoop_write(&length, 4);
		btsnoop_write(&length, 4);
		btsnoop_write(&flags, 4);
		btsnoop_write(&drops, 4);
		btsnoop_write(&time_hi, 4);
		btsnoop_write(&time_lo, 4);
		btsnoop_write(&type, 1);
		btsnoop_write(packet, length_he - 1);
	} else {
		LOG_INF("Full!");
	}

	k_mutex_unlock(&snoop_lock);

	return length_he;
}

#if BTSNOOP_WRITE_SDCARD
static void dump_file(void)
{
	uint8_t buff[16], i;
	int file_len, rd_len, ret;
	uint32_t dump_cnt = 0;

	if (!file_stream) {
		LOG_INF("Btsnoop not initialize!");
		return;
	}

	k_mutex_lock(&snoop_lock, K_FOREVER);
	file_len = stream_tell(file_stream);
	stream_seek(file_stream, 0, SEEK_DIR_BEG);
	rd_len = 0;

	printf("\nDump snoop data start len: %d\n\n", file_len);

	while (rd_len < file_len) {
		ret = stream_read(file_stream, buff, 16);
		if (ret <= 0) {
			break;
		} else {
			rd_len += ret;
		}

		for (i = 0; i < ret; i++) {
			printf("%02x ", buff[i]);
		}
		printf("\n");

		dump_cnt++;
		if ((dump_cnt%0xF) == 0) {
			k_sleep(K_MSEC(1));
		}
	}

	stream_seek(file_stream, 0, SEEK_DIR_END);
	k_mutex_unlock(&snoop_lock);
}
#else
static void dump_buffer(void)
{
	uint32_t i;

	if (snoop_init_flag == 0) {
		LOG_INF("Btsnoop not initialize!");
		return;
	}

	k_mutex_lock(&snoop_lock, K_FOREVER);

	printf("\nDump snoop data start len: %d\n\n", cap_len);
	for (i = 0; i < cap_len; ) {
		printf("%02x ", snoop_buf[i++]);
		if ((i%16) == 0) {
			printf("\n");
		}

		if ((i%0xFF) == 0) {
			k_sleep(K_MSEC(1));
		}
	}

	printf("\n\nDump snoop data end\n");
	k_mutex_unlock(&snoop_lock);
}
#endif

void hci_snoop_dump(void)
{
#if BTSNOOP_WRITE_SDCARD
	dump_file();
#else
	dump_buffer();
#endif
}

void hci_snoop_reinit(void)
{
#if BTSNOOP_WRITE_SDCARD
	snoop_file_close();
#endif
	btsnoop_init();
}

void hci_snoop_info(void)
{
#if BTSNOOP_WRITE_SDCARD
	if (!file_stream) {
		LOG_INF("Btsnoop not initialize!");
		return;
	}

	LOG_INF("SD card capture data: %d, %d\n", stream_tell(file_stream), cap_len);
#else
	if (snoop_init_flag == 0) {
		LOG_INF("Btsnoop not initialize!");
		return;
	}

	LOG_INF("Capture data: %d, remain buff: %d\n", cap_len, (SNOOP_MAX_SIZE - cap_len));
#endif
}

void hci_snoop_close(void)
{
#if BTSNOOP_WRITE_SDCARD
	snoop_file_close();
#endif
}
