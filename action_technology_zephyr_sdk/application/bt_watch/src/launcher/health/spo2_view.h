/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BT_WATCH_SRC_LAUNCHER_HEALTH_SPO2_VIEW_H_
#define BT_WATCH_SRC_LAUNCHER_HEALTH_SPO2_VIEW_H_

#include "app_ui.h"

typedef struct spo2_view_presenter {
	uint8_t (*get_spo2)(void);
} spo2_view_presenter_t;

#endif /* BT_WATCH_SRC_LAUNCHER_HEALTH_SPO2_VIEW_H_ */
