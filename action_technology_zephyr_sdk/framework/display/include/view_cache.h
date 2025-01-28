/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file view cache interface
 */

#ifndef FRAMEWORK_DISPLAY_INCLUDE_VIEW_CACHE_H_
#define FRAMEWORK_DISPLAY_INCLUDE_VIEW_CACHE_H_

#include <stdint.h>

/**
 * @defgroup view_cache_apis View Cache APIs
 * @ingroup system_apis
 * @{
 */

enum VICE_CACHE_SLIDE_TYPE {
	LANDSCAPE,
	PORTRAIT,
};

enum VICE_CACHE_EVENT_TYPE {
	/* For serial load only */
	VIEW_CACHE_EVT_LOAD_BEGIN,
	VIEW_CACHE_EVT_LOAD_END,
	VIEW_CACHE_EVT_LOAD_CANCEL,
};

/**
 * @typedef view_cache_focus_cb_t
 * @brief Callback to notify view focus changed
 */
typedef void (*view_cache_focus_cb_t)(uint16_t view_id, bool focus);

/**
 * @typedef view_cache_monitor_cb_t
 * @brief Callback to notify view msg, like scroll begin/end
 */
typedef void (*view_cache_monitor_cb_t)(uint16_t view_id, uint8_t msg_id);

/**
 * @typedef view_cache_event_cb_t
 * @brief Callback to notify view cache event
 */
typedef void (*view_cache_event_cb_t)(uint8_t event);

/**
 * @struct view_cache_dsc
 * @brief Structure to describe view cache
 */
typedef struct view_cache_dsc {
	uint8_t type; /* enum SLIDE_VIEW_TYPE */
	uint8_t rotate : 1; /* rotate sliding mode, only when 'num' >= 3 */
	uint8_t serial_load : 1; /* (deprecated) serial loading the views */
	uint8_t rebound : 1; /* allow the main views to scroll partially out of
	                      * the screen and rebound on release.
						  * this field ignored when rotate == 1
						  */

	uint8_t num;  /* number of views in vlist */
	const uint16_t *vlist; /* main view list */
	const uint8_t *vlist_create_flags; /* main view extra flags, UI_CREATE_FLAG_POST_ON_PAINT, etc. */
	const void **plist; /* main view presenter list */

	/* Limit cross view sliding only when the view in vlist focused.
	 * Set VIEW_INVALID_ID (0) to ignore the limit.
	 */
	uint16_t cross_attached_view;

	/* cross view list and their presenter list.
	 * set VIEW_INVALID_ID (0) to the corresponding index if the view does not exist.
	 *
	 * For LANDSCAPE, [0] is the UP view, [1] is the DOWN view
	 * For PORTRAIT, [0] is the LEFT view, [1] is the RIGHT view
	 */
	uint16_t cross_vlist[2];
	const void *cross_plist[2];

	/* view focus changed callback */
	view_cache_focus_cb_t focus_cb;
	/* view monitor callback */
	view_cache_monitor_cb_t monitor_cb;
	/* (deprecated) event callback */
	view_cache_event_cb_t event_cb;
} view_cache_dsc_t;

/**
 * @brief Initialize the view cache
 *
 * @param dsc pointer to an initialized structure view_cache_dsc
 * @param init_view initial focused view in vlist or cross_vlist
 *
 * @retval 0 on success else negative code.
 */
int view_cache_init(const view_cache_dsc_t *dsc, uint16_t init_view);

/**
 * @brief Initialize the view cache
 *
 * @param dsc pointer to an initialized structure view_cache_dsc
 * @param init_focus_view initial focused view in vlist or cross_vlist
 * @param init_main_view initial central main view. used only if init_view is cross view
 *
 * @retval 0 on success else negative code.
 */
int view_cache_init2(const view_cache_dsc_t *dsc,
		uint16_t init_focus_view, uint16_t init_main_view);

/**
 * @brief Deinitialize the view cache
 *
 * @retval 0 on success else negative code.
 */
int view_cache_deinit(void);

/**
 * @brief Get focused view id
 *
 * @retval id of focused view.
 */
uint16_t view_cache_get_focus_view(void);

/**
 * @brief Get focused main view id
 *
 * @retval id of focused view.
 */
uint16_t view_cache_get_focus_main_view(void);

/**
 * @brief Set focus to view
 *
 * Must not called during view sliding or when cross views are focused
 *
 * @view_id id of focus view, must in the vlist of view cache
 *
 * @retval 0 on success else negative code.
 */
int view_cache_set_focus_view(uint16_t view_id);

/**
 * @brief Shrink the view cache
 *
 * The view cache will be restored when some view in view cache get focused.
 *
 * @retval 0 on success else negative code.
 */
int view_cache_shrink(void);

/**
 * @brief Dump the view cache
 *
 * @retval 0 on success else negative code.
 */
void view_cache_dump(void);

/**
 * @} end defgroup system_apis
 */
#endif /* FRAMEWORK_DISPLAY_INCLUDE_VIEW_CACHE_H_ */
