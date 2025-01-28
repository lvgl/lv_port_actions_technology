/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BT_WATCH_SRC_LAUNCHER_HEART_VIEW_H_
#define BT_WATCH_SRC_LAUNCHER_HEART_VIEW_H_

#include "app_ui.h"

typedef struct heart_view_presenter {
	uint32_t (*get_heart_rate)(void);
} heart_view_presenter_t;

#endif /* BT_WATCH_SRC_LAUNCHER_HEART_VIEW_H_ */
