/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os_common_api.h>
#include <ui_manager.h>
#include <view_stack.h>
#include <app_ui.h>
#include "vibration_view.h"
#ifndef CONFIG_SIMULATOR
#include "drivers/vibrator.h"
#endif
#define PWM_CHAN 1

static uint8_t current_level = 0;

static void vibration_start(void);
static void vibration_stop(void);
static void vibration_set_level(uint8_t level);
static uint8_t vibration_get_level(void);
static void vibration_set_freq(uint32_t freq);

static uint8_t vib_strength[LEVEL_NUM] = {
		40,
		60,
		90,
};

const vibration_view_presenter_t vibration_view_presenter = {
	.start = vibration_start,
	.stop = vibration_stop,
	.set_level = vibration_set_level,
	.get_level = vibration_get_level,
	.set_freq = vibration_set_freq
};

static void vibration_start(void)
{
#ifndef CONFIG_SIMULATOR
	const struct device *dev = device_get_binding(CONFIG_VIBRATOR_DEV_NAME);
	if(!dev) {
		LOG_INF("dev not found!");
		return;
	}
	vibrat_start(dev, PWM_CHAN, vib_strength[current_level]);
#endif
	return;
}

static void vibration_stop(void)
{
#ifndef CONFIG_SIMULATOR
	const struct device *dev = device_get_binding(CONFIG_VIBRATOR_DEV_NAME);
	if(!dev) {
		LOG_INF("dev not found!");
		return;
	}
	vibrat_stop(dev, PWM_CHAN);
	return;
#endif
}

static void vibration_set_level(uint8_t level)
{
	if(current_level == level)
		return;
	//set vibration strength
	current_level = level;
	vibration_start();
	return;
}

static uint8_t vibration_get_level(void)
{
	return current_level;
}

static void vibration_set_freq(unsigned int freq)
{
#ifndef CONFIG_SIMULATOR
	const struct device *dev = device_get_binding(CONFIG_VIBRATOR_DEV_NAME);
	if(!dev) {
		LOG_INF("dev not found!");
		return;
	}
	vibrat_set_freq_param(dev, PWM_CHAN, freq);
	return;
#endif
}

void vibration_ui_enter(void)
{
	view_stack_push_view(VIB_VIEW, &vibration_view_presenter);
}

