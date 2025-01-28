/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file led manager interface 
 */

#ifndef __LED_MANGER_H__
#define __LED_MANGER_H__
#include <drivers/pwm.h>

/**
 * @defgroup led_manager_apis App Led Manager APIs
 * @ingroup system_apis
 * @{
 */
/**
 * @cond INTERNAL_HIDDEN
 */

#define DISPLAY_FOREVER 0xFFFF

/**
 * INTERNAL_HIDDEN @endcond
 */
/** led status */
enum
{
	/** led status on */
	LED_ON = 1,
	/** led status of */
	LED_OFF,
	/** led status blink */
	LED_BLINK,
	/** led status breath*/
	LED_BREATH,
	/** led status do nothing */
	LED_NONE,
};

enum {
	/**start state off */
	LED_START_STATE_OFF = 0,
	/**start state on */
	LED_START_STATE_ON,
};

typedef void (*led_display_callback)(void);
typedef int (*led_timeout_callback)(void);

/**
 * @brief led manager init funcion
 *
 * This routine calls init led manager ,called by main
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int led_manager_init(void);

/**
 * @brief led manager deinit funcion
 *
 * This routine calls deinit led manager ,called by main
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */
int led_manager_deinit(void);

/**
 * @brief set led index display state 
 *
 * This routine set led index display state , on or off
 *
 * @param led_index index of led
 * @param onoff onoff state of led
 * @param timeout timeout of current state
 * @param cb callback when timeout occur.
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */
int led_manager_set_display(uint16_t led_index, uint8_t onoff, uint32_t timeout, led_display_callback cb);

/**
 * @brief set led breath mode 
 *
 * @param led_index index of led
 * @param breath param
 * @param timeout timeout of current state
 * @param cb callback when timeout occur.
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */
int led_manager_set_breath(uint16_t led_index, pwm_breath_ctrl_t *ctrl, uint32_t timeout, led_display_callback cb);

/**
 * @brief set led blink mode 
 *
 * @param led_index index of led
 * @param blink_period period of blink
 * @param blink_pulse pulse of blink
 * @param timeout timeout of current state
 * @param start_state set high or low voltage level active
 * @param cb callback when blink cnt overflow occur.
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int led_manager_set_blink(uint8_t led_index, uint16_t blink_period, uint16_t blink_pulse, uint32_t timeout, uint8_t start_state, led_display_callback cb);

/**
 * @brief set all led on or off mode 
 *
 * @param onoff state of all led
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int led_manager_set_all(uint8_t onoff);

/**
 * @brief set led mamager to sleep mode, when sleep mode all led will off
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int led_manager_sleep(void);

/**
 * @brief set led mamager to wakeup mode, when sleep mode all led will restore history state before sleep
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */
int led_manager_wake_up(void);

/**
 * @brief save current led state 
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */
int led_manager_store(void);

/**
 * @brief restore led state from history
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int led_manager_restore(void);

/**
 * @brief set timeout event lock
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */
int led_manager_set_timeout_event_start(void);

/**
 * @brief set timeout event
 * @param timeout of current state
 * @param cb callback when timeout overflow occur.
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int led_manager_set_timeout_event(int timeout, led_timeout_callback cb);

/**
 * @} end defgroup led_manager_apis
 */
#endif
