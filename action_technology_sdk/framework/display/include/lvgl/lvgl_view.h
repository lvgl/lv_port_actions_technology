/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief lvgl view
 */

#ifndef FRAMEWORK_DISPLAY_INCLUDE_LVGL_VIEW_H
#define FRAMEWORK_DISPLAY_INCLUDE_LVGL_VIEW_H

/**
 * @defgroup lvgl_view_apis LVGL View APIs
 * @ingroup lvgl_apis
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
* @cond INTERNAL_HIDDEN
*/

/**
 * @brief Initialize the lvgl view system.
 *
 * It will register lvgl view implementation to ui_service.
 *
 * @retval 0 on success else negative code.
 */
int lvgl_view_system_init(void);

/**
* INTERNAL_HIDDEN @endcond
*/

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* FRAMEWORK_DISPLAY_INCLUDE_LVGL_VIEW_H */
