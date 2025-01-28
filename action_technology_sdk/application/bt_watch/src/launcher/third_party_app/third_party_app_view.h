/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef BT_WATCH_SRC_LAUNCHER_THIRD_PARTY_APP_VIEW_H_
#define BT_WATCH_SRC_LAUNCHER_THIRD_PARTY_APP_VIEW_H_

#include <msg_manager.h>

typedef struct third_party_app_view_presenter {
	void (*reserve)(void);
} third_party_app_view_presenter_t;

extern const third_party_app_view_presenter_t third_party_app_view_presenter;

#endif
