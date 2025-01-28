/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager genarate bt mac and name.
 */
#define SYS_LOG_DOMAIN "bt manager"

#include <os_common_api.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <msg_manager.h>
#include <mem_manager.h>
#include <bt_manager.h>
#include "bt_manager_inner.h"
#include <sys_event.h>
#include <property_manager.h>
#include "bt_porting_inner.h"

//#define CONFIG_AUDIO_RANDOM 1
#define CONFIG_HARDWARE_GET_RANDOM	1

#if defined(CONFIG_AUDIO_RANDOM) && defined(CONFIG_AUDIO)
#include <audio_record.h>
#elif defined(CONFIG_HARDWARE_GET_RANDOM)
#include <soc.h>
#endif

#define MAC_STR_LEN		(12+1)	/* 12(len) + 1(NULL) */
#define BT_NAME_LEN		(32+1)	/* 32(len) + 1(NULL) */
#define BT_PRE_NAME_LEN	(20+1)	/* 10(len) + 1(NULL) */


#if defined(CONFIG_PROPERTY) && defined(CONFIG_AUDIO) && defined(CONFIG_AUDIO_RANDOM)
#define MIC_READ_BUF_LEN	128
#define MIC_READ_SAMPLES	(128*20)

static uint32_t mic_gen_rand(void)
{
	uint32_t rand_value = os_cycle_get_32();
	int ret, i, sample_cnt, opened = 0, max_time = 500;
	struct audio_record_t *handle;
	uint8_t buff[MIC_READ_BUF_LEN];

	handle = audio_record_create(AUDIO_STREAM_VOICE, 16, 16, AUDIO_FORMAT_PCM_16_BIT, AUDIO_MODE_MONO, NULL);
	if (!handle) {
		SYS_LOG_ERR("Create err!");
		goto mic_exit;
	}

	ret = audio_record_start(handle);
	if (ret) {
		SYS_LOG_ERR("Start err!");
		goto mic_exit;
	} else {
		opened = 1;
	}

	sample_cnt = MIC_READ_SAMPLES;
	while (sample_cnt && max_time) {
		ret = audio_record_read(handle, buff, MIC_READ_BUF_LEN);
		for (i = 0; i < ret; i++) {
			rand_value = (rand_value*131) + buff[i];
		}

		if (sample_cnt > ret) {
			sample_cnt -= ret;
		} else {
			sample_cnt = 0;
		}

		max_time--;
		os_sleep(1);
	}

mic_exit:
	if (opened) {
		audio_record_stop(handle);
	}

	if (handle) {
		audio_record_destory(handle);
	}

	return rand_value;
}
#endif

#if defined(CONFIG_PROPERTY) && defined(CONFIG_HARDWARE_GET_RANDOM)
uint32_t hardware_randomizer_gen_rand(void)
{
	uint32_t trng_low, trng_high;

	se_trng_init();
	se_trng_process(&trng_low, &trng_high);
	se_trng_deinit();

	return trng_low;
}
#endif

static void bt_manager_bt_name(uint8_t *mac_str)
{
#ifdef CONFIG_PROPERTY
	uint8_t bt_name[BT_NAME_LEN];
	uint8_t bt_pre_name[BT_PRE_NAME_LEN];
	uint8_t len = 0;
	uint8_t *cfg_name = CFG_BT_NAME;
	int ret;

try_ble_name:
	memset(bt_name, 0, BT_NAME_LEN);
	ret = property_get(cfg_name, bt_name, (BT_NAME_LEN-1));
	if (ret < 0) {
		SYS_LOG_WRN("ret: %d", ret);

		memset(bt_pre_name, 0, BT_PRE_NAME_LEN);
		ret = property_get(CFG_BT_PRE_NAME, bt_pre_name, (BT_PRE_NAME_LEN-1));
		if (ret > 0) {
			len = strlen(bt_pre_name);
			memcpy(bt_name, bt_pre_name, len);
		}
		memcpy(&bt_name[len], mac_str, strlen(mac_str));
		len += strlen(mac_str);

		property_set_factory(cfg_name, bt_name, len);
	}

	SYS_LOG_INF("%s: %s", cfg_name, bt_name);

	if (!memcmp(cfg_name, CFG_BT_NAME, strlen(CFG_BT_NAME))) {
		cfg_name = CFG_BLE_NAME;
		goto try_ble_name;
	}
#endif
}


static void bt_manager_bt_mac(uint8_t *mac_str)
{
#ifdef CONFIG_PROPERTY
	uint8_t mac[6], i;
	int ret;
	uint32_t rand_val;

	ret = property_get(CFG_BT_MAC, mac_str, (MAC_STR_LEN-1));
	if (ret < (MAC_STR_LEN-1)) {
		SYS_LOG_WRN("ret: %d", ret);
#if defined(CONFIG_AUDIO) && defined(CONFIG_AUDIO_RANDOM)
		rand_val = mic_gen_rand();
#elif defined(CONFIG_HARDWARE_GET_RANDOM)
		rand_val = hardware_randomizer_gen_rand();
#else
		rand_val = os_cycle_get_32();
#endif
		bt_manager_config_set_pre_bt_mac(mac);
		memcpy(&mac[3], &rand_val, 3);

		for (i = 3; i < 6; i++) {
			if (mac[i] == 0)
				mac[i] = 0x01;
		}

		bin2hex(mac, 6, mac_str, 12);

		property_set_factory(CFG_BT_MAC, mac_str, (MAC_STR_LEN-1));
	} else {
		hex2bin(mac_str, 12, mac, 6);
		bt_manager_updata_pre_bt_mac(mac);
	}
#endif
	SYS_LOG_INF("BT MAC: %s", mac_str);
}

void bt_manager_check_mac_name(void)
{
	uint8_t mac_str[MAC_STR_LEN];
	uint32_t seed = 0, i;

	memset(mac_str, 0, MAC_STR_LEN);
	bt_manager_bt_mac(mac_str);
	bt_manager_bt_name(mac_str);

	for (i=0; i<MAC_STR_LEN; i++) {
		seed = seed*131 + mac_str[i];
	}
	bt_rand32_set_seed(seed);
}
