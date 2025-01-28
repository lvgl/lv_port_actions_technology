/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os_common_api.h>
#include <property_manager.h>
#include <ui_manager.h>
#include <view_manager.h>
#include <view_stack.h>
#include <lvgl/lvgl_res_loader.h>
#include <assert.h>
#include "launcher_app.h"
#include "clock_selector.h"
#include "clock_selector_view.h"

static const clock_dsc_t clock_dsc[NUM_CLOCK_IDS] = {
	[MAIN_CLOCK_ID] = { "analog", SCENE_CLOCK_VIEW, 100 },
	[DIGITAL_CLOCK_ID] = { "digital", SCENE_DIGITAL_CLOCK_VIEW, 1000 },
	/* AOD clock must place last */
#ifdef SCENE_AOD_CLOCK_VIEW
	[AOD_CLOCK_ID] = { "aod", SCENE_AOD_CLOCK_VIEW, 60000 },
#endif
	[DIGITAL1_CLOCK_ID] = { "digital1", SCENE_DIGITAL1_CLOCK, 1000 },
#ifdef CONFIG_VIDEO_APP
	[VIDEO_CLOCK_ID] = {"video" , SCENE_CLOCK_VIEW , 100},
#endif
};

const clock_dsc_t * clocksel_get_clock_dsc(uint8_t index)
{
	if (index < ARRAY_SIZE(clock_dsc))
		return &clock_dsc[index];

	return NULL;
}

const clock_dsc_t * clocksel_get_aod_clock_dsc(void)
{
#ifdef SCENE_AOD_CLOCK_VIEW
	return &clock_dsc[AOD_CLOCK_ID];
#else
	return NULL;
#endif
}

static uint8_t _clock_selector_view_get_clock_id(void);
static const char *_clock_selector_view_get_clock_name(uint8_t id);
static void _clock_selector_view_set_clock_id(uint8_t id);

static const clock_selector_view_presenter_t clock_selector_view_presenter = {
	.get_clock_id = _clock_selector_view_get_clock_id,
	.get_clock_name = _clock_selector_view_get_clock_name,
	.set_clock_id = _clock_selector_view_set_clock_id,
};

static uint8_t _clock_selector_view_get_clock_id(void)
{
	launcher_app_t *app = launcher_app_get();

	return app->clock_id;
}

static const char *_clock_selector_view_get_clock_name(uint8_t id)
{
	return clock_dsc[id].name;
}

static void _clock_selector_view_set_clock_id(uint8_t id)
{
	launcher_app_t *app = launcher_app_get();

	SYS_LOG_INF("select clock %d", id);

	if (id != app->clock_id && id < ARRAY_SIZE(clock_dsc)) {
#ifdef CONFIG_PROPERTY
		property_set_int(CFG_CLOCK_ID, id);
		property_flush(CFG_CLOCK_ID);
#endif

		launcher_apply_clock_id(id);
	}

	view_stack_pop();
}

int clocksel_ui_enter(void)
{
	return view_stack_push_view(CLOCK_SELECTOR_VIEW, &clock_selector_view_presenter);
}
