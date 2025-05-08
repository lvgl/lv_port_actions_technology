/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file view stack interface
 */

#ifndef FRAMEWORK_DISPLAY_INCLUDE_VIEW_STACK_H_
#define FRAMEWORK_DISPLAY_INCLUDE_VIEW_STACK_H_

#include <stdint.h>
#include <stdbool.h>
#include "view_cache.h"
#include "ui_effects/switch_effect.h"

/**
 * @defgroup view_stack_apis View Stack APIs
 * @ingroup system_apis
 * @{
 */

/**
 * @brief View stadk back modes
 */
enum view_stack_back_mode {
	VIEW_STACK_BACK_HOME = 0,     /* back to bottom level directly */
	VIEW_STACK_BACK_PREV,         /* back to previous level without preserving the bottom level */
	VIEW_STACK_BACK_PREV_OR_HOME, /* back to previous level if not at bottom level */

	NUM_VIEW_STACK_BACK_MODES,
};

/**
 * @typedef view_group_scroll_cb_t
 * @brief Callback to notify view scroll result
 */
typedef void (*view_group_scroll_cb_t)(uint16_t view_id);

/**
 * @typedef view_group_focus_cb_t
 * @brief Callback to notify view focus changed
 */
typedef void (*view_group_focus_cb_t)(uint16_t view_id, bool focus);

/**
 * @struct view_group_dsc
 * @brief Structure to describe view group layout
 */
typedef struct view_group_dsc {
	const uint16_t * vlist; /* view id list */
	const void ** plist; /* view presenter list */
	uint8_t num; /* number of view */
	uint8_t idx; /* index of focused view */

	/* view scroll result callback */
	view_group_scroll_cb_t scroll_cb;
	/* view focus changed callback */
	view_group_focus_cb_t focus_cb;
} view_group_dsc_t;

/**
 * @brief Initialize the view stack
 *
 * @retval 0 on success else negative code.
 */
int view_stack_init(void);

/**
 * @brief Get the number of elements of the view stack
 *
 * @retval view id on success else negative code.
 */
uint8_t view_stack_get_num(void);

/**
 * @brief Query whether view stack is empty
 *
 * @retval query result.
 */
static inline bool view_stack_is_empty(void)
{
	return view_stack_get_num() == 0;
}

/**
 * @brief Get the top (focused) view id of the view stack
 *
 * @retval view id on success else negative code.
 */
uint16_t view_stack_get_top(void);

/**
 * @brief Get the top view cache descriptor of the view stack
 *
 * @retval the top view cache descriptor if exists else NULL.
 */
const view_cache_dsc_t * view_stack_get_top_cache(void);

/**
 * @brief on the basis of level_id get view_id
 * @retval view id on success else negative code.
 */
uint16_t view_stack_level_get_view_id(uint16_t level_id);

/**
 * @brief Clean the view stack.
 *
 * @retval 0 on success else negative code.
 */
void view_stack_clean(void);

/**
 * @brief Go back to previous level of the stack
 *
 * @param mode back mode, see @enum view_stack_back_mode
 *
 * @retval 0 on success else negative code.
 */
int view_stack_back(int mode);

/**
 * @brief Got back directly to the bottom(home) level of the stack
 *
 * @retval 0 on success else negative code.
 */
static inline int view_stack_back_home(void)
{
	return view_stack_back(VIEW_STACK_BACK_HOME);
}

/**
 * @brief Got back to the previous level of the stack
 *
 * If at bottom level, this will take action like view_stack_clean()
 *
 * @retval 0 on success else negative code.
 */
static inline int view_stack_back_prev(void)
{
	return view_stack_back(VIEW_STACK_BACK_PREV);
}

/**
 * @brief Got back to the previous level if exist of the stack
 *
 * If already at bottom level, this will do nothing
 *
 * @retval 0 on success else negative code.
 */
static inline int view_stack_back_prev_or_home(void)
{
	return view_stack_back(VIEW_STACK_BACK_PREV_OR_HOME);
}

#define view_stack_pop_home view_stack_back_home
#define view_stack_pop      view_stack_back_prev_or_home

/**
 * @brief Push view cache to the view stack
 *
 * @param dsc pointer to an initialized structure view_cache_dsc
 * @param view_id initial focused view id, must in the vlist of dsc
 *
 * @retval 0 on success else negative code.
 */
int view_stack_push_cache(const view_cache_dsc_t *dsc, uint16_t view_id);

/**
 * @brief Push view cache to the view stack
 *
 * @param dsc pointer to an initialized structure view_cache_dsc
 * @param focus_view_id initial focused view id, must in the vlist of dsc
 * @param main_view_id initial central main view. used only if init_view is cross view
 *
 * @retval 0 on success else negative code.
 */
int view_stack_push_cache2(const view_cache_dsc_t *dsc,
		uint16_t focus_view_id, uint16_t main_view_id);

/**
 * @brief Push view group to the view stack
 *
 * @param dsc pointer to an initialized structure view_group_dsc
 *
 * @retval 0 on success else negative code.
 */
int view_stack_push_group(const view_group_dsc_t *dsc);

/**
 * @brief Push view to the view stack
 *
 * @param view_id id of view
 * @param presenter presenter of view
 *
 * @retval 0 on success else negative code.
 */
int view_stack_push_view(uint16_t view_id, const void *presenter);

/**
 * @brief Jump to the new view cache (do not push)
 *
 * @param dsc pointer to an initialized structure view_cache_dsc
 * @param view_id initial focused view id, must in the vlist of dsc
 *
 * @retval 0 on success else negative code.
 */
int view_stack_jump_cache(const view_cache_dsc_t *dsc, uint16_t view_id);

/**
 * @brief Jump to the new view cache (do not push)
 *
 * @param dsc pointer to an initialized structure view_cache_dsc
 * @param focus_view_id initial focused view id, must in the vlist of dsc
 * @param main_view_id initial central main view. used only if init_view is cross view
 *
 * @retval 0 on success else negative code.
 */
int view_stack_jump_cache2(const view_cache_dsc_t *dsc,
		uint16_t focus_view_id, uint16_t main_view_id);

/**
 * @brief Jump to the new view group (do not push)
 *
 * @param dsc pointer to an initialized structure view_group_dsc
 *
 * @retval 0 on success else negative code.
 */
int view_stack_jump_group(const view_group_dsc_t *dsc);

/**
 * @brief Jump to the new view (do not push)
 *
 * @param view_id id of view
 * @param presenter presenter of view
 *
 * @retval 0 on success else negative code.
 */
int view_stack_jump_view(uint16_t view_id, const void *presenter);

/**
 * @brief Dump the view stack
 *
 * @retval N/A.
 */
void view_stack_dump(void);

/**
 * @brief Get switch effect type
 *
 * @retval switch effect type.
 */
uint8_t view_stack_get_effect_type(void);

/**
 * @brief Set switch effect type
 *
 * @param type type of switch effect
 *
 * @retval N/A.
 */
void view_stack_set_effect_type(uint8_t type);

/**
 * @brief Set switch effect touch tracking enabled
 *
 * @param enabled enable touch tracking or not
 *
 * @retval N/A.
 */
void view_stack_set_effect_touch_tracking(bool enabled);

/**
 * @brief View group change focused view index
 *
 * @retval N/A.
 */
static inline void view_group_set_focus_idx(view_group_dsc_t *dsc, uint8_t idx)
{
	dsc->idx = idx;
}

/**
 * @} end defgroup system_apis
 */
#endif /* FRAMEWORK_DISPLAY_INCLUDE_VIEW_STACK_H_ */
