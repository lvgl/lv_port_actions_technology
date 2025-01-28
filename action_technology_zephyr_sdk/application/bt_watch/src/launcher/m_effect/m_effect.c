/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "m_effect.h"
#include <view_stack.h>
#include <app_ui.h>
#include "launcher_app.h"

/*********************
 *      INCLUDES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *  FUNCTIONS
 **********************/
void go_to_m_effect(void)
{
#if defined(CONFIG_VG_LITE)
    launcher_app_t *app = launcher_app_get();
    ui_switch_effect_set_type(UI_SWITCH_EFFECT_NONE);
    view_stack_push_view(EFFECT_WHEEL_VIEW, NULL);
    ui_switch_effect_set_type(app->switch_effect_mode);
#endif
}

uint8_t m_effect_get_clock_id(void)
{
	launcher_app_t *app = launcher_app_get();
	return app->clock_id;
}