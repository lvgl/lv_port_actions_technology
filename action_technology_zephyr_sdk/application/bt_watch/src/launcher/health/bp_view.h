/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BT_WATCH_SRC_LAUNCHER_HEALTH_BP_VIEW_H_
#define BT_WATCH_SRC_LAUNCHER_HEALTH_BP_VIEW_H_

#include "app_ui.h"

typedef struct bp_view_presenter {
	void (*get_bp_range)(uint8_t *low, uint8_t *high);
} bp_view_presenter_t;

#endif /* BT_WATCH_SRC_LAUNCHER_HEALTH_BP_VIEW_H_ */
