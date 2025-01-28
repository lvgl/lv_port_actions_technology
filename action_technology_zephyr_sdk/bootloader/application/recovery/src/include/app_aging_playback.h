/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file app_aging_playback.h
 */

#ifndef _APP_AGING_PLAYBACK_H_
#define _APP_AGING_PLAYBACK_H_

#include <kernel.h>

#define AGING_PLAYBACK_STA_DISABLE  0
#define AGING_PLAYBACK_STA_DOING    1
#define AGING_PLAYBACK_STA_PASSED   2
#define AGING_PLAYBACK_STA_ERROR    3

#define AGING_PLAYBACK_STA_ERROR_VNRAM  4

struct aging_playback_info {
	u8_t aging_playback_status; /* 0-disable, 1-aging doing, 2-aging passed, >= 3 -aging error */
	u8_t aging_playback_source; /* 0-pink noise, 1-white noise, 2-sine wave */
	u8_t aging_playback_volume; /* volume level 0 ~ 31 */
	u16_t aging_playback_duration; /* unit 1 minute */
	u16_t aging_playback_consume; /* unit 1 minute */
};

void aging_playback_app_try(void);

#endif
