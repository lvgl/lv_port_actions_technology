/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BT_WATCH_SRC_LAUNCHER_COMPASS_VIEW_H_
#define BT_WATCH_SRC_LAUNCHER_COMPASS_VIEW_H_

#include "app_ui.h"

typedef struct compass_view_presenter {
	uint16_t (*get_bearing)(void); /* in degree */
} compass_view_presenter_t;

#endif /* BT_WATCH_SRC_LAUNCHER_COMPASS_VIEW_H_ */
