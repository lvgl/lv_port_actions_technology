/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRAMEWORK_DISPLAY_INCLUDE_EFFECTS_SWITCH_EFFECT_H_
#define FRAMEWORK_DISPLAY_INCLUDE_EFFECTS_SWITCH_EFFECT_H_

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
	UI_SWITCH_EFFECT_NONE,
	UI_SWITCH_EFFECT_ALPHA,
	UI_SWITCH_EFFECT_FAN,
	UI_SWITCH_EFFECT_PAGE,
	UI_SWITCH_EFFECT_SCALE,
	UI_SWITCH_EFFECT_ZOOM_ALPHA,
	UI_SWITCH_EFFECT_TRANSLATION,
	UI_SWITCH_EFFECT_CUBE,

	UI_SWITCH_EFFECT_BOOK,
	NUM_UI_SWITCH_EFFECTS,
} ui_switch_effect_e;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
#ifdef CONFIG_UI_SWITCH_EFFECT

/**
 * @brief Set UI switch effect type
 *
 * @param type effect type, see ui_switch_effect_e.
 *
 * @retval 0 on success else negative errno code.
 */
int ui_switch_effect_set_type(uint8_t type);

/**
 * @brief Get UI switch effect type
 *
 * @retval type effect type, see ui_switch_effect_e.
 */
uint8_t ui_switch_effect_get_type(void);

/**
 * @brief Set UI switch effect total frames
 *
 * @param frame number of effect frames.
 *
 * @retval N/A.
 */
void ui_switch_effect_set_total_frames(uint16_t frame);

/**
 * @brief Set UI switch effect old UI leave anim direction
 *
 * @param out_right indicate old UI leave direction: moving left or rotating negative directoin around Y-axis.
 *
 * @retval N/A.
 */
void ui_switch_effect_set_anim_dir(bool out_right);

/**
 * @brief Set UI switch effect tracking touch
 *
 * @param enabled enable touch tracking or not
 *
 * @retval N/A.
 */
void ui_switch_effect_set_touch_tracking(bool enabled);

#else

static inline int ui_switch_effect_set_type(uint8_t type) { return 0; }
static inline uint8_t ui_switch_effect_get_type(void) { return UI_SWITCH_EFFECT_NONE; }
static inline void ui_switch_effect_set_total_frames(uint16_t frame) { }
static inline void ui_switch_effect_set_anim_dir(bool out_right) { }
static inline void ui_switch_effect_set_touch_tracking(bool enabled) { }

#endif /* CONFIG_UI_SWITCH_EFFECT */

#ifdef __cplusplus
}
#endif
#endif /*FRAMEWORK_DISPLAY_INCLUDE_EFFECTS_SWITCH_EFFECT_H_*/
