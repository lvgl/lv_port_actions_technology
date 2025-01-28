/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*********************
 *      INCLUDES
 *********************/
#include <os_common_api.h>
#include <app_ui.h>

#ifdef CONFIG_LVGL
#  include <lvgl/lvgl_view.h>
#endif

#ifdef CONFIG_UI_MANAGER
#  include <ui_service.h>
#  include <msgbox_cache.h>
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  GLOBAL PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
int app_ui_init(void)
{
	lvgl_view_system_init();
	ui_service_register_gesture_default_callback();

#ifdef CONFIG_LVGL_USE_RES_MANAGER
	lvgl_res_loader_init(DEF_UI_WIDTH, DEF_UI_HEIGHT);
#endif

#ifdef CONFIG_LVGL_USE_BITMAP_FONT
	lvgl_bitmap_font_init(NULL);
	//lvgl_bitmap_font_cache_preset((void *)font_cache_preset, ARRAY_SIZE(font_cache_preset));
#endif

#ifdef CONFIG_LVGL_USE_FREETYPE_FONT
	lvgl_freetype_font_init();
//	lvgl_freetype_font_cache_preset((void *)ft_font_cache_preset, ARRAY_SIZE(ft_font_cache_preset));
#endif

#if defined(CONFIG_VG_LITE) && !defined(CONFIG_LV_USE_GPU_ACTS_VG_LITE)
	vg_lite_init(CONFIG_PANEL_HOR_RES, CONFIG_PANEL_VER_RES);
#endif

	app_ui_stack_init(APP_UI_INVALID);

	return 0;
}
