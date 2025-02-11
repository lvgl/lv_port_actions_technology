/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*********************
 *      INCLUDES
 *********************/

#include <app_ui.h>
#include "system_app.h"

#ifdef CONFIG_LVGL
#  include <lvgl/lvgl_view.h>
#endif

#ifdef CONFIG_GLYPHIX
#include "gx_lvgx.h"
#endif

#ifdef CONFIG_UI_SERVICE
#  include <ui_service.h>
#  include <msgbox_cache.h>
#endif

#include <view_stack.h>

/**********************
 *  GLOBAL PROTOTYPES
 **********************/

#ifdef CONFIG_ALARM_APP
extern int alarm_msgbox_popup_cb(uint16_t msgbox_id, uint8_t msg_id, void * msg_data, void * hwnd);
#endif
#ifdef CONFIG_OTA_BACKGROUND
extern int ota_msgbox_popup_cb(uint16_t msgbox_id, uint8_t msg_id, void * msg_data, void * hwnd);
#endif
#ifdef CONFIG_BT_CALL_APP
extern int btcall_msgbox_popup_cb(uint16_t msgbox_id, uint8_t msg_id, void * msg_data, void * hwnd);
#endif

extern int lpower_msgbox_popup_cb(uint16_t msgbox_id, uint8_t msg_id, void * msg_data, void * hwnd);

/**********************
 *  STATIC VARIABLES
 **********************/

static const msgbox_dsc_t msgbox_dsc[] = {
#ifdef CONFIG_ALARM_APP
	{ .id = ALARM_MSGBOX_ID, .order = 0, .handler = alarm_msgbox_popup_cb, },
#endif
#ifdef CONFIG_OTA_BACKGROUND
	{ .id = OTA_MSGBOX_ID, .order = 0, .handler = ota_msgbox_popup_cb, },
#endif
#ifdef CONFIG_BT_CALL_APP
	{ .id = BTCALL_MSGBOX_ID, .order = 0, .handler = btcall_msgbox_popup_cb, },
#endif

	{ .id = LPOWER_MSGBOX_ID, .order = 0, .handler = lpower_msgbox_popup_cb, },
};

#ifdef CONFIG_BITMAP_FONT
static const bitmap_font_cache_preset_t font_cache_preset[] = {
	{DEF_FONT28_FILE, 64*1024},
	{DEF_FONT32_FILE, 84*1024},
	{DEF_VF_50, 5*1024},
	{DEF_VF_30, 3*1024},
	{DEF_VF_26, 5*1024},
#ifndef CONFIG_EMOJI_FONT_USE_MMAP
#if CONFIG_PSRAM_SIZE > 6144
	{DEF_FONT28_EMOJI, 36*1024},
#else
	{DEF_FONT28_EMOJI, 24*1024},
#endif
#else
	{DEF_FONT28_EMOJI, 4*1024},
#endif
};
#endif /* CONFIG_BITMAP_FONT */

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

int system_app_ui_init(void)
{
#ifdef CONFIG_LVGL
	lvgl_view_system_init();

	ui_service_register_gesture_default_callback();
	msgbox_cache_init(msgbox_dsc, ARRAY_SIZE(msgbox_dsc));

#ifdef CONFIG_LVGL_USE_RES_MANAGER
	lvgl_res_loader_init(DEF_UI_WIDTH, DEF_UI_HEIGHT);
#endif

#ifdef CONFIG_LVGL_USE_BITMAP_FONT
	lvgl_bitmap_font_init(NULL);
	lvgl_bitmap_font_cache_preset((void *)font_cache_preset, ARRAY_SIZE(font_cache_preset));
#endif

#ifdef CONFIG_LVGL_USE_FREETYPE_FONT
	lvgl_freetype_font_init();
//	lvgl_freetype_font_cache_preset((void *)ft_font_cache_preset, ARRAY_SIZE(ft_font_cache_preset));
#endif
#endif

  view_stack_init();
  return 0;
}

int system_app_ui_deinit(void)
{
#ifdef CONFIG_LVGL
#if CONFIG_GLYPHIX
  lvgx_view_system_deinit();
#endif
#endif
  return 0;
}
