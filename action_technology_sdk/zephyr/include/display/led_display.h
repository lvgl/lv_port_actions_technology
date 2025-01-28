/** @file
 *  @brief actions led display APIs.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __LED_DISPLAY_H
#define __LED_DISPLAY_H

/**
 * @brief actions led display APIs
 * @defgroup actions ledisplay APIs
 * @{
 */

#include <stdio.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <toolchain.h>
#include <drivers/pwm.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	LED_OP_OFF,
	LED_OP_ON,
	LED_OP_BREATH,
	LED_OP_BLINK,
}led_op_code;

typedef struct
{
	u32_t pulse:14;
	u32_t period:15;
	u32_t op_code:2;
	u32_t start_state:1;
}led_pixel_value;

#define LED_COLOR_ON	(LED_OP_ON << 29)
#define LED_COLOR_OFF	(LED_OP_OFF << 29)
#define LED_COLOR_BREATH	(LED_OP_BREATH << 29)
#define LED_COLOR_FLASH(period, pulse, start_state)	(start_state << 31 | (LED_OP_BLINK << 29) | (period << 14) | pulse)

int led_draw_pixel(int led_id, u32_t color, pwm_breath_ctrl_t *ctrl);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __LED_DISPLAY_H */
