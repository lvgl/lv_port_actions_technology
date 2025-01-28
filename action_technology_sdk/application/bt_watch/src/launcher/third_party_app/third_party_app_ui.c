/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os_common_api.h>
#include <ui_manager.h>
#include <view_stack.h>
#include <app_ui.h>
#include "third_party_app_view.h"

const third_party_app_view_presenter_t third_party_app_view_presenter = {
	.reserve = NULL,
};


void third_party_app_ui_enter(void)
{
	view_stack_push_view(THIRD_PARTY_APP_VIEW, &third_party_app_view_presenter);
}

