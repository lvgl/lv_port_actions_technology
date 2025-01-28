/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*********************
 *      INCLUDES
 *********************/
#include <app_ui.h>
#include "launcher_app.h"
#include "main_view.h"
#include "clock_view.h"
#include "heart_view.h"
#include "clock_selector/clock_selector.h"

#ifdef CONFIG_PROPERTY
#include <property_manager.h>
#endif

#ifndef CONFIG_SIMULATOR
#  include <soc.h>
#  include <drivers/power_supply.h>
#  include <power_manager.h>
#else
#  include <native_window.h>
#endif

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void _get_time(struct rtc_time *time);
static uint8_t _get_battery_percent(void);
static uint8_t _get_clock_id(void);
static uint8_t _get_scroll_mode(void);
static uint8_t _get_switch_mode(void);
static uint32_t _get_calories(void);
static uint32_t _get_distance(void);
static uint32_t _get_heart_rate(void);
static uint32_t _get_sleep_time(void);
static uint32_t _get_step_count(void);
static uint8_t _get_temperature(void);
static uint8_t _get_weather(void);

static void _set_rtc(bool en, uint16_t period_ms);
static void _set_scroll_mode(uint8_t scroll_mode);
static void _set_switch_mode(uint8_t switch_mode);
static int _open_clock_preview(void);

/**********************
 *  GLOBAL VARIABLES
 **********************/
const main_view_presenter_t main_view_presenter = {
	.get_time = _get_time,
	.get_battery_percent = _get_battery_percent,
	.set_scroll_mode = _set_scroll_mode,
	.get_scroll_mode = _get_scroll_mode,
	.set_switch_mode = _set_switch_mode,
	.get_switch_mode = _get_switch_mode,
};

const heart_view_presenter_t heart_view_presenter = {
	.get_heart_rate = _get_heart_rate,
};

const clock_view_presenter_t clock_view_presenter = {
	.get_time = _get_time,
	.get_battery_percent = _get_battery_percent,
	.get_temperature = _get_temperature,
	.get_weather = _get_weather,
	.get_step_count = _get_step_count,
	.get_heart_rate = _get_heart_rate,
	.get_calories = _get_calories,
	.get_sleep_time = _get_sleep_time,
	.get_distance = _get_distance,
	.get_clock_id = _get_clock_id,

	.set_rtc = _set_rtc,
	.invoke_preview = _open_clock_preview,
};

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void _get_time(struct rtc_time *time)
{
	launcher_app_t *app = launcher_app_get();

#ifdef CONFIG_RTC_ACTS
	if (app->rtc_dev)
		rtc_get_time(app->rtc_dev, &app->rtc_time);
#elif defined(CONFIG_SIMULATOR)
	native_window_get_local_time(&app->rtc_time);
#endif /* CONFIG_RTC_ACTS */

	memcpy(time, &app->rtc_time, sizeof(*time));
}

static uint8_t _get_battery_percent(void)
{
#ifdef CONFIG_POWER
	return power_manager_get_battery_capacity();
#else
	return 100;
#endif
}

static uint32_t _get_calories(void)
{
	return 789;
}

static uint8_t _get_clock_id(void)
{
	launcher_app_t *app = launcher_app_get();

	return app->clock_id;
}

static uint8_t _get_scroll_mode(void)
{
	launcher_app_t *app = launcher_app_get();

	return app->scroll_effects_mode;
}

static void _set_scroll_mode(uint8_t scroll_mode)
{
	launcher_app_t *app = launcher_app_get();

#ifdef CONFIG_UI_SCROLL_EFFECT
	if(scroll_mode >=  NUM_UI_SCROLL_EFFECTS){
		scroll_mode = UI_SCROLL_EFFECT_NONE;
	}

	if (scroll_mode != app->scroll_effects_mode) {
#ifdef CONFIG_PROPERTY
		property_set_int(CFG_SCROLL_MODE, scroll_mode);
		property_flush(CFG_SCROLL_MODE);
#endif
		app->scroll_effects_mode = scroll_mode;
	}

	ui_scroll_effect_set_type(app->scroll_effects_mode);
#endif /* CONFIG_UI_SCROLL_EFFECT */
}

static uint8_t _get_switch_mode(void)
{
	launcher_app_t *app = launcher_app_get();

	return app->switch_effect_mode;
}

static void _set_switch_mode(uint8_t switch_mode)
{
	launcher_app_t *app = launcher_app_get();

#ifdef CONFIG_UI_SWITCH_EFFECT
	if(switch_mode >=  NUM_UI_SWITCH_EFFECTS){
		switch_mode = UI_SWITCH_EFFECT_NONE;
	}

	if (switch_mode != app->switch_effect_mode) {
#ifdef CONFIG_PROPERTY
		property_set_int(CFG_SWITCH_MODE, switch_mode);
		property_flush(CFG_SWITCH_MODE);
#endif
		app->switch_effect_mode = switch_mode;
	}

	ui_switch_effect_set_type(app->switch_effect_mode);
#endif /* CONFIG_UI_SWITCH_EFFECT */
}

static uint32_t _get_distance(void)
{
	return 12345;
}

static uint32_t _get_heart_rate(void)
{
	launcher_app_t *app = launcher_app_get();

	return app->heart_rate;
}

static uint32_t _get_sleep_time(void)
{
	return 310;
}

static uint32_t _get_step_count(void)
{
	launcher_app_t *app = launcher_app_get();

	return app->step_count;
}

static uint8_t _get_temperature(void)
{
	return 30;
}

static uint8_t _get_weather(void)
{
	return SUNNY;
}

#ifdef CONFIG_SIMULATOR
static void _nwin_rtc_timer_period_handler(void * user_data)
{
	ui_view_paint(CLOCK_VIEW);
}
#endif /* CONFIG_SIMULATOR */

static void _set_rtc(bool en, uint16_t period_ms)
{
	SYS_LOG_INF("rtc: en %d, period %u", en, period_ms);

#ifndef CONFIG_SIMULATOR
	launcher_set_rtc_period(en, period_ms);
#else
	if (en) {
		native_window_register_callback(NWIN_CB_CLOCK, _nwin_rtc_timer_period_handler, NULL);
	} else {
		native_window_unregister_callback(NWIN_CB_CLOCK);
	}
#endif /* CONFIG_SIMULATOR */
}

static int _open_clock_preview(void)
{
	return clocksel_ui_enter();
}
