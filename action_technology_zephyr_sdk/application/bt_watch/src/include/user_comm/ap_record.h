/*
 * Copyright (c) 2021 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief 
*/
#ifndef _AP_RECORD_H
#define _AP_RECORD_H

#include <mem_manager.h>
#include <app_manager.h>
#include <srv_manager.h>
#include <volume_manager.h>
#include <msg_manager.h>
#include <thread_timer.h>
#include <media_player.h>
#include <audio_system.h>
#include <audio_policy.h>
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>

#include <thread_timer.h>

#include "app_defines.h"
#include "sys_manager.h"
#include "app_ui.h"

#define OPUS_FRAME_LEN (40)

typedef int (*record_send_pkg_to_stream)(u8_t *, u16_t, int);
typedef void (*record_resource_release)(void);

typedef struct
{
	record_send_pkg_to_stream stream_send_cb;
	record_resource_release release_cb;

} record_stream_init_param;

void record_start_record(record_stream_init_param *user_param);
void record_stop_record(void);

#endif  // _AP_RECORD_H

