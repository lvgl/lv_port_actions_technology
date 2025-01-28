/** @file
 *  @brief actions seg led manager APIs.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SEG_LED_MANAGER_H
#define __SEG_LED_MANAGER_H

/**
 * @brief actions seg led manager APIs
 * @defgroup actions seg led manager APIs
 * @{
 */

#include <stdio.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <toolchain.h>
#ifdef CONFIG_SEG_LED_DISPLAY
#include <display/seg_led_display.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum seg_led_display_item {
	SLED_PLAY = 0,
	SLED_PAUSE,
	SLED_USB,
	SLED_SD,
	SLED_COL,
	SLED_AUX,
	SLED_FM,
	SLED_P1,
	SLED_NUMBER1,
	SLED_NUMBER2,
	SLED_NUMBER3,
	SLED_NUMBER4,
};

#define FLASH_ITEM(x)  	(1 << x)
#define FLASH_2ITEM(x, y)  	((1 << x) | (1 << y))
#define FLASH_3ITEM(x, y, z)  	((1 << x) | (1 << y) | (1 << z))
#define FLASH_ALL_NUMER	(0xf << SLED_NUMBER1)

#define FLASH_FOREVER 0xFF

enum seg_led_clear_mode {
	SLED_CLEAR_ALL = 0,
	SLED_CLEAR_NUM
};

typedef int (*seg_led_flash_callback)(void);
typedef int (*seg_led_timeout_callback)(void);

/**
 * @brief seg led display a number or letter
 *
 * This routine seg led display number or letter in num_add,0`9,A`Z
 *
 * @param num_addr addr of seg led
 * @param c number or letter
 * @param display display or not
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int seg_led_display_number(uint8_t num_addr, uint8_t c, bool display);

/**
 * @brief seg led display string
 *
 * This routine seg led display string,start from start_pos
 *
 * @param start_pos start position
 * @param str string
 * @param display display or not
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int seg_led_display_string(uint8_t start_pos, const uint8_t *str, bool display);

/**
 * @brief seg led display icon
 *
 * @param icon_idx addr of icon
 * @param display display or not
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int seg_led_display_icon(uint8_t icon_idx, bool display);

/**
 * @brief set seg led icon or numbers flashing
 *
 * @param flash_map flashing object of icon or numbers
 * @param flash_period flash period
 * @param flash_cnt flash count
 * @param cb callback when flash cnt overflow occur.
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int seg_led_display_set_flash(uint16_t flash_map, uint16_t flash_period, uint16_t flash_cnt, seg_led_flash_callback cb);

/**
 * @brief set timeout event lock
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int seg_led_manager_set_timeout_event_start(void);

/**
 * @brief set timeout event
 * @param timeout of current state
 * @param cb callback when timeout overflow occur.
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int seg_led_manager_set_timeout_event(int timeout, seg_led_flash_callback cb);

/**
 * @brief save current seg led state
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int seg_led_manager_store(void);

/**
 * @brief restore seg led state from history
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int seg_led_manager_restore(void);

/**
 * @brief timeout restore seg led state from history
 * @param timeout of current state
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int seg_led_manager_set_timeout_restore(int timeout);

/**
 * @brief set seg led mamager to wakeup mode,all seg led will restore history state before sleep
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int seg_led_manager_wake_up(void);

/**
 * @brief set seg led mamager to sleep mode, when sleep mode all led will off
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int seg_led_manager_sleep(void);

/**
 * @brief clear seg led state mode, when clear mode all or 4 numbers seg led will off
 * @param clr_mode clear all or 4 numbers
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int seg_led_manager_clear_screen(uint8_t clr_mode);

/**
 * @brief seg led manager init funcion
 *
 * This routine calls init seg led manager ,called by main
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int seg_led_manager_init(void);

/**
 * @brief seg led manager deinit funcion
 *
 * This routine calls deinit seg led manager ,called by main
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int seg_led_manager_deinit(void);
#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __SEG_LED_MANAGER_H */
