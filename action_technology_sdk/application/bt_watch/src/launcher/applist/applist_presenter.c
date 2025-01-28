/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*********************
 *      INCLUDES
 *********************/
#include <os_common_api.h>
#ifndef CONFIG_SIMULATOR
#  include <soc.h>
#endif
#ifdef CONFIG_PROPERTY
#  include <property_manager.h>
#endif
#include <view_stack.h>
#include "applist_ui.h"
#include "compass_view.h"
#include "../alipay/alipay_ui.h"
#include "../gps/gps_ui.h"
#include "../vibration/vibration_ui.h"
#include "../clock_selector/clock_selector.h"
#include "launcher_app.h"
#ifdef CONFIG_THIRD_PARTY_APP
#include "../third_party_app/third_party_app_ui.h"
#endif
/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
	uint8_t view_mode;
	uint8_t phone_en : 1;
	uint8_t vibrator_en : 1;
	int16_t scrl_x;
	int16_t scrl_y;
} applist_presenter_data_t;

/**********************
 *   STATIC PROTOTYPES
 **********************/
static uint8_t _applist_get_view_mode(void);
static void _applist_set_view_mode(uint8_t mode);
static void _applist_load_scroll_value(int16_t *scrl_x, int16_t *scrl_y);
static void _applist_save_scroll_value(int16_t scrl_x, int16_t scrl_y);
static bool _applist_view_phone_is_on(void);
static void _applist_view_toggle_phone(void);
static bool _applist_view_vibrator_is_on(void);
static void _applist_view_toggle_vibrator(void);
static bool _applist_view_aod_mode_is_on(void);
static void _applist_view_toggle_aod_mode(void);
static void _applist_view_open_stopwatch(void);
static void _applist_view_open_alarm(void);
static void _applist_view_open_compass(void);
static void _applist_view_open_longview(void);
static void _applist_view_open_three_dimensional(void);
static void _applist_view_open_svgmap(void);
static void _applist_view_open_setting(void);

static uint16_t _compass_view_get_bearing(void);

/**********************
 *  STATIC VARIABLES
 **********************/
const applist_view_presenter_t applist_view_presenter = {
	.get_view_mode = _applist_get_view_mode,
	.set_view_mode = _applist_set_view_mode,
	.load_scroll_value = _applist_load_scroll_value,
	.save_scroll_value = _applist_save_scroll_value,
	.phone_is_on = _applist_view_phone_is_on,
	.toggle_phone = _applist_view_toggle_phone,
	.vibrator_is_on = _applist_view_vibrator_is_on,
	.toggle_vibrator = _applist_view_toggle_vibrator,
	.aod_mode_is_on = _applist_view_aod_mode_is_on,
	.toggle_aod_mode = _applist_view_toggle_aod_mode,
	.open_stopwatch = _applist_view_open_stopwatch,
	.open_alarmclock = _applist_view_open_alarm,
	.open_compass = _applist_view_open_compass,
	.open_longview = _applist_view_open_longview,
	.open_alipay = alipay_ui_enter,
	.open_wxpay = wxpay_ui_enter,
	.open_gps = gps_ui_enter,
	.open_three_dimensional = _applist_view_open_three_dimensional,
	.open_svgmap = _applist_view_open_svgmap,
	.open_setting = _applist_view_open_setting,
	.open_vib = vibration_ui_enter,
#ifdef CONFIG_THIRD_PARTY_APP
	.open_third_party_app = third_party_app_ui_enter,
#endif
};

static const compass_view_presenter_t compass_view_presenter = {
	.get_bearing = _compass_view_get_bearing,
};

static applist_presenter_data_t s_applist_model = {
	.view_mode = APPLIST_MODE_LIST,
	.scrl_x = INT16_MAX,
	.scrl_y = INT16_MAX,
};

/**********************
 *   STATIC FUNCTIONS
 **********************/
static uint8_t _applist_get_view_mode(void)
{
#ifdef CONFIG_PROPERTY
	s_applist_model.view_mode = property_get_int(CFG_APPLIST_MODE, APPLIST_MODE_LIST);
#endif
	return s_applist_model.view_mode;
}

static void _applist_set_view_mode(uint8_t mode)
{
	if (mode < NUM_APPLIST_MODES && mode != s_applist_model.view_mode) {
		s_applist_model.view_mode = mode;
		s_applist_model.scrl_x = INT16_MAX;
		s_applist_model.scrl_y = INT16_MAX;
#ifdef CONFIG_PROPERTY
		property_set_int(CFG_APPLIST_MODE, mode);
		property_flush(CFG_APPLIST_MODE);
#endif
	}
}

static void _applist_load_scroll_value(int16_t *scrl_x, int16_t *scrl_y)
{
	if (scrl_x && s_applist_model.scrl_x != INT16_MAX)
		*scrl_x = s_applist_model.scrl_x;

	if (scrl_y && s_applist_model.scrl_y != INT16_MAX)
		*scrl_y = s_applist_model.scrl_y;
}

static void _applist_save_scroll_value(int16_t scrl_x, int16_t scrl_y)
{
	s_applist_model.scrl_x = scrl_x;
	s_applist_model.scrl_y = scrl_y;
}

static bool _applist_view_phone_is_on(void)
{
	return s_applist_model.phone_en;
}

static void _applist_view_toggle_phone(void)
{
	s_applist_model.phone_en = !s_applist_model.phone_en;
}

static bool _applist_view_vibrator_is_on(void)
{
	return s_applist_model.vibrator_en;
}

static void _applist_view_toggle_vibrator(void)
{
	s_applist_model.vibrator_en = !s_applist_model.vibrator_en;
}

static bool _applist_view_aod_mode_is_on(void)
{
#ifndef CONFIG_SIMULATOR
	return soc_get_aod_mode();
#else
	return 0;
#endif
}

static void _applist_view_toggle_aod_mode(void)
{
#ifndef CONFIG_SIMULATOR
	/* AOD mode supported */
	if (clocksel_get_aod_clock_dsc() != NULL) {
		soc_set_aod_mode(!soc_get_aod_mode());
#ifdef CONFIG_PROPERTY
		property_set_int(CFG_AOD_MODE, soc_get_aod_mode());
		property_flush(CFG_AOD_MODE);
#endif
	}
#endif /* CONFIG_SIMULATOR */
}

static void _applist_view_open_stopwatch(void)
{
	view_stack_push_view(STOPWATCH_VIEW, NULL);
}

static void _applist_view_open_setting(void)
{
	view_stack_push_view(SETTING_VIEW, NULL);
}

static void _applist_view_open_alarm(void)
{
#ifdef CONFIG_ALARM_MANAGER
	view_stack_push_view(ALARM_SET_VIEW, NULL);
#endif
}

static void _applist_view_open_compass(void)
{
	view_stack_push_view(COMPASS_VIEW, &compass_view_presenter);
}

static void _applist_view_open_longview(void)
{
#ifndef CONFIG_VIEW_SCROLL_MEM_LOWEST
	view_stack_push_view(TEST_LONG_VIEW, NULL);
#endif
}

static void _applist_view_open_three_dimensional(void)
{
#if defined(SCENE_THREE_D_VIEW) && defined(CONFIG_VG_LITE)
	view_stack_push_view(THREE_DIMENSIONAL_VIEW, NULL);
#endif
}

static void _applist_view_open_svgmap(void)
{
#if defined(CONFIG_SVG_RENDER)
	view_stack_push_view(SVGMAP_VIEW, NULL);
#endif
}

static uint16_t _compass_view_get_bearing(void)
{
	launcher_app_t *app = launcher_app_get();

	return app->bearing;
}

int applist_ui_enter(void)
{
	view_stack_push_view(APPLIST_VIEW, &applist_view_presenter);
	return 0;
}
