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
#define APP_ID_LCMUSIC			"lcmusic"

#define APP_ID_DEFAULT			APP_ID_LCMUSIC

/*
 * app id switch list
 */

enum {
	/* user common message */
	MSG_LCMUSIC_EVENT = MSG_APP_MESSAGE_START, /* 200 */
};




extern char share_stack_area[CONFIG_APP_STACKSIZE];


#endif  /* _APP_DEFINES_H */

