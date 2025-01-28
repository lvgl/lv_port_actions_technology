/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system app
 */

#ifndef _SYSTEM_APP_H_
#define _SYSTEM_APP_H_

#include <stdint.h>

enum {
	SYS_INIT_NORMAL_MODE,
	SYS_INIT_ATT_TEST_MODE,
	SYS_INIT_ALARM_MODE,
};

/** system app input handle*/
void system_input_handle_init(void);
void system_key_event_handle(uint32_t key_event);
void system_sr_input_event_handle(void *value);
int system_app_launch(uint8_t mode);
int system_app_launch_init(void);

#endif
