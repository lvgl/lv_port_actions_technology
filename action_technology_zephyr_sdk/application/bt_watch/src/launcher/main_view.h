/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BT_WATCH_SRC_LAUNCHER_MAIN_VIEW_H_
#define BT_WATCH_SRC_LAUNCHER_MAIN_VIEW_H_
#ifdef CONFIG_RTC_ACTS
#include <drivers/rtc.h>
#endif
#include "app_ui.h"

typedef struct main_view_presenter {
	void (*get_time)(struct rtc_time *time);
	uint8_t (*get_battery_percent)(void);
	uint8_t (*get_scroll_mode)(void);
	void (*set_scroll_mode)(uint8_t scroll_mode);
	uint8_t (*get_switch_mode)(void);
	void (*set_switch_mode)(uint8_t switch_mode);
} main_view_presenter_t;

extern const main_view_presenter_t main_view_presenter;

#endif /* BT_WATCH_SRC_LAUNCHER_MAIN_VIEW_H_ */
