/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file app define
 */

#ifndef _APP_DEFINES_H
#define _APP_DEFINES_H

#define APP_ID_MAIN				"main"
#define APP_ID_BTCALL			"btcall"


/*
 * app id switch list
 */

enum {
	/* bt common message */
	MSG_BT_APP_MESSAGE_START = MSG_APP_MESSAGE_START, /* 200 */
};




extern char share_stack_area[CONFIG_APP_STACKSIZE];


#endif  /* _APP_DEFINES_H */

