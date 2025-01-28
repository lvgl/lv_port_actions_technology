/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief local player.h
 */


#ifndef _BT_TRANSMIT_H
#define _BT_TRANSMIT_H

#ifdef CONFIG_FILE_STREAM
#include <file_stream.h>
#endif

#ifdef CONFIG_BLUETOOTH
#include "btservice_api.h"
#endif
#ifdef CONFIG_BT_MANAGER
#include "bt_manager.h"
#endif

#if CONFIG_AUDIO
#include <audio_system.h>
#include <audio_policy.h>
#endif

#if CONFIG_MEDIA
#include <media_player.h>
#include <media_mem.h>
#endif

#include "app_manager.h"

struct bt_device_info_t {
	uint8_t *name;
	bd_address_t addr;
	uint8_t connected : 1;
	uint8_t paired : 1;
	int8_t rssi;
};

typedef void (*bt_transmit_inquiry_handler_t)(struct bt_device_info_t *bt_info, uint8_t count);

void bt_transmit_inquiry_start_notify(bt_transmit_inquiry_handler_t handler);
void bt_transmit_inquiry_restart_notify(bt_transmit_inquiry_handler_t handler);
void bt_transmit_inquiry_stop_notify(void);

void bt_transmit_inquiry_start(bt_transmit_inquiry_handler_t handler);
void bt_transmit_inquiry_restart(bt_transmit_inquiry_handler_t handler);
void bt_transmit_inquiry_stop(void);

void bt_transmit_catpure_start(io_stream_t input_stream, uint16_t sample_rate, uint8_t channels);
void bt_transmit_catpure_stop(void);
void bt_transmit_catpure_pre_start(void);

void bt_transmit_capture_start_inner(void);
void bt_transmit_capture_stop_inner(void);

void bt_transmit_capture_set_ready(bool ready);

bool bt_transmit_check_dev_connected(uint8_t *addr);

int bt_transmit_sync_vol_to_remote(void);

#endif  /* _MPLAYER_H */
