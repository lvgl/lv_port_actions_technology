/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file lvgl_inner.h
 *
 */

#ifndef PORTING_ZEPHYR_LVGL_INNER_H
#define PORTING_ZEPHYR_LVGL_INNER_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../../lvgl_ext.h"
#include "../../src/lvgl_private.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * @brief Initialize LVGL display porting
 *
 * @retval 0 on success else negative code.
 */
int lv_port_display_init(void);

/**
 * @brief Initialize LVGL indev pointer porting
 *
 * @retval 0 on success else negative code.
 */
int lv_port_indev_pointer_init(void);

/**
 * @brief Period callback to put pointer to indev pointer buffer queue
 *
 * @retval N/A.
 */
void lv_port_indev_pointer_scan(void);

/**
 * @brief Get LVGL tick
 *
 * @retval N/A.
 */
uint32_t lv_port_tick_get(void);

/**
 * @brief Initialize LVGL zephyr file system
 *
 * @retval N/A.
 */
void lv_port_z_fs_init(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*PORTING_ZEPHYR_LVGL_INNER_H*/
