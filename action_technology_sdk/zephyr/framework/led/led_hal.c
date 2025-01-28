/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file led hal interface
 */
#include <os_common_api.h>
#include <led_manager.h>
#include <mem_manager.h>
#include <string.h>
#include <drivers/gpio.h>
#include <ui_manager.h>
#include "led_hal.h"
#include <display/led_display.h>

void led_on(u8_t led_index)
{
	u32_t color = LED_COLOR_ON;
	led_draw_pixel(led_index, color, NULL);
}

void led_off(u8_t led_index)
{
	u32_t color = LED_COLOR_OFF;
	led_draw_pixel(led_index, color, NULL);
}

void led_breath(u8_t led_index, pwm_breath_ctrl_t *ctrl)
{
	u32_t color = LED_COLOR_BREATH;

	led_draw_pixel(led_index, color, ctrl);
}
void led_blink(u8_t led_index, u16_t period, u16_t pulse, u8_t start_state)
{
	u32_t color = LED_COLOR_FLASH(period, pulse, start_state);
	led_draw_pixel(led_index, color, NULL);
}


