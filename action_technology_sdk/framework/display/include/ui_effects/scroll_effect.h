/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRAMEWORK_DISPLAY_INCLUDE_EFFECTS_SCROLL_EFFECT_H_
#define FRAMEWORK_DISPLAY_INCLUDE_EFFECTS_SCROLL_EFFECT_H_

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**********************
 *      TYPEDEFS
 **********************/
typedef enum {
	UI_SCROLL_EFFECT_NONE,
	UI_SCROLL_EFFECT_CUBE,
	UI_SCROLL_EFFECT_FAN,
	UI_SCROLL_EFFECT_PAGE,
	UI_SCROLL_EFFECT_SCALE,
	UI_SCROLL_EFFECT_BOOK,

	NUM_UI_SCROLL_EFFECTS,
} ui_scroll_effect_e;

typedef enum {
	UI_VSCROLL_EFFECT_NONE,
	UI_VSCROLL_EFFECT_CUBE,
	UI_VSCROLL_EFFECT_FAN,
	UI_VSCROLL_EFFECT_PAGE,
	UI_VSCROLL_EFFECT_SCALE,
	UI_VSCROLL_EFFECT_ALPHA,

	NUM_UI_VSCROLL_EFFECTS,
} ui_vscroll_effect_e;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
#ifdef CONFIG_UI_SCROLL_EFFECT

/**
 * @brief Set UI scroll effect type
 *
 * @param type scroll type, see ui_scroll_effect_e.
 *
 * @retval 0 on success else negative errno code.
 */
int ui_scroll_effect_set_type(uint8_t type);

/**
 * @brief Set UI vertical scroll effect type
 *
 * @param type vertical scroll type, see ui_scroll_effect_e.
 *
 * @retval 0 on success else negative errno code.
 */
int ui_vscroll_effect_set_type(uint8_t type);

#else
static inline int ui_scroll_effect_set_type(uint8_t type) { return 0; }
static inline int ui_vscroll_effect_set_type(uint8_t type) { return 0; }
#endif /* CONFIG_UI_SCROLL_EFFECT */

#ifdef __cplusplus
}
#endif
#endif /*FRAMEWORK_DISPLAY_INCLUDE_EFFECTS_SCROLL_EFFECT_H_*/
