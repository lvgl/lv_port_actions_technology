/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os_common_api.h>
#include <ui_manager.h>
#include <view_stack.h>
#include <app_ui.h>

#include "health_ui.h"
#include "bp_view.h"
#include "spo2_view.h"

static void _bp_view_get_bp_range(uint8_t *low, uint8_t *hight);
static uint8_t _spo2_view_get_sop2(void);

static const bp_view_presenter_t bp_view_presenter = {
	.get_bp_range = _bp_view_get_bp_range,
};

static const spo2_view_presenter_t spo2_view_presenter = {
	.get_spo2 = _spo2_view_get_sop2,
};

static const uint16_t health_view_list[] = {
	SPORT_VIEW,
	HEALTH_BP_VIEW, HEALTH_SPO2_VIEW,
};

static const void * health_view_presenter_list[] = {
	NULL,
	&bp_view_presenter, &spo2_view_presenter,
};

static const view_cache_dsc_t health_view_cache_dsc = {
	.type = PORTRAIT,
	.serial_load = 1,
	.num = ARRAY_SIZE(health_view_list),
	.vlist = health_view_list,
	.plist = health_view_presenter_list,
	.cross_vlist = { VIEW_INVALID_ID, VIEW_INVALID_ID },
	.cross_plist = { NULL, NULL },
};

static void _bp_view_get_bp_range(uint8_t *low, uint8_t *high)
{
	*low = 70;
	*high = 113;
}

static uint8_t _spo2_view_get_sop2(void)
{
	return 99;
}

int health_ui_enter(void)
{
	view_stack_push_cache(&health_view_cache_dsc, HEALTH_BP_VIEW);
	return 0;
}
