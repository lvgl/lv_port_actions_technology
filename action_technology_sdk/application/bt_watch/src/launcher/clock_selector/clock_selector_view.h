/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BT_WATCH_SRC_LAUNCHER_CLOCK_SELECTOR_VIEW_H_
#define BT_WATCH_SRC_LAUNCHER_CLOCK_SELECTOR_VIEW_H_

#include "app_ui.h"

typedef struct clock_selector_view_presenter {
	uint8_t (*get_clock_id)(void);
	const char *(*get_clock_name)(uint8_t id);
	void (*set_clock_id)(uint8_t id);
} clock_selector_view_presenter_t;

#endif /* BT_WATCH_SRC_LAUNCHER_CLOCK_SELECTOR_VIEW_H_ */
