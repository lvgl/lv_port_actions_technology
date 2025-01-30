/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief application message interface
 */

#ifndef __APP_MSG_H__
#define __APP_MSG_H__

#include <msg_manager.h>

// app name
#define APP_NAME	"main"

// app message type
enum APP_MSG_TYPE {
	MSG_UI = MSG_APP_MESSAGE_START,
	MSG_OTA,
};

// app message cmd
enum APP_MSG_CMD {
	CMD_NULL = 0,
	CMD_SCREEN_OFF,
	CMD_SCREEN_ON,
};

int app_msg_init(void);
int app_msg_send(const char* receiver, uint8_t type, uint8_t cmd);

#endif /* __APP_MSG_H__ */

