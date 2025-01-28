/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file app ui define
 */

#ifndef _APP_UI_H_
#define _APP_UI_H_

/*********************
 *      INCLUDES
 *********************/
#include <sys_event.h>
#include <msg_manager.h>
#include <ui_manager.h>

#ifdef CONFIG_LVGL
#  include <lvgl/lvgl.h>
#  include <lvgl/lvgl_widgets.h>
#ifdef CONFIG_LVGL_USE_RES_MANAGER
#  include <lvgl/lvgl_res_loader.h>
#endif
#ifdef CONFIG_LVGL_USE_BITMAP_FONT
#  include <lvgl/lvgl_bitmap_font.h>
#endif
#ifdef CONFIG_LVGL_USE_FREETYPE_FONT
#  include <lvgl/lvgl_freetype_font.h>
#endif
#endif /*CONFIG_LVGL*/

#ifdef CONFIG_VG_LITE
#  include <vg_lite/vg_lite.h>
#endif

#ifndef CONFIG_SIMULATOR
#  include <board_cfg.h>
#endif

#if CONFIG_PANEL_HOR_RES >= 466
#  include "../../resource/466x466/bt_watch_sty.h"

#define DEF_UI_WIDTH   466
#define DEF_UI_HEIGHT  466
#define DEF_FONT_SIZE_SMALL  28
#define DEF_FONT_SIZE_NORMAL 32
#define DEF_FONT_SIZE_LARGE  36
#define DEF_FONT_SIZE_XLARGE 36

#elif CONFIG_PANEL_HOR_RES >= 454
#  include "../../resource/454x454/bt_watch_sty.h"

#define DEF_UI_WIDTH   454
#define DEF_UI_HEIGHT  454
#define DEF_FONT_SIZE_SMALL  28
#define DEF_FONT_SIZE_NORMAL 32
#define DEF_FONT_SIZE_LARGE  32
#define DEF_FONT_SIZE_XLARGE 32

#elif CONFIG_PANEL_HOR_RES >= 360
#  include "../../resource/360x360/bt_watch_sty.h"

#define DEF_UI_WIDTH   360
#define DEF_UI_HEIGHT  360
#define DEF_FONT_SIZE_SMALL  22
#define DEF_FONT_SIZE_NORMAL 24
#define DEF_FONT_SIZE_LARGE  24
#define DEF_FONT_SIZE_XLARGE 24

#else
#  include "../../resource/240x280/bt_watch_sty.h"

#define DEF_UI_WIDTH   240
#define DEF_UI_HEIGHT  280
#define DEF_FONT_SIZE_SMALL  22
#define DEF_FONT_SIZE_NORMAL 22
#define DEF_FONT_SIZE_LARGE  24
#define DEF_FONT_SIZE_XLARGE 24

#endif /* CONFIG_PANEL_HOR_RES == 454 */

/*********************
 *      DEFINES
 *********************/
#ifndef _WIN32
#  define sd_funmap(ptr)
#endif

/* default ui view resolution */
#define DEF_UI_VIEW_WIDTH   CONFIG_PANEL_HOR_RES
#define DEF_UI_VIEW_HEIGHT  CONFIG_PANEL_VER_RES

// default ui file
#define DEF_UI_DISK         CONFIG_APP_UI_DISK
#define DEF_FONT_DISK       CONFIG_APP_FONT_DISK
#define DEF_STY_FILE        DEF_UI_DISK"/bt_watch.sty"
#define DEF_RES_FILE        DEF_UI_DISK"/bt_watch.res"
#define DEF_STR_FILE        DEF_UI_DISK"/bt_watch.zhC"
#define DEF_FONT22_FILE     DEF_FONT_DISK"/sans22.font"
#define DEF_FONT24_FILE     DEF_FONT_DISK"/sans24.font"
#define DEF_FONT28_FILE     DEF_FONT_DISK"/sans28.font"
#define DEF_FONT32_FILE     DEF_FONT_DISK"/sans32.font"
#define DEF_FONT28_EMOJI    DEF_FONT_DISK"/emoji28.font"
#define DEF_VFONT_FILE      DEF_FONT_DISK"/vfont.ttf"

#define DEF_FONT_FILE(size)  DEF_FONT_DISK"/sans"#size".font"
#define _DEF_FONT_FILE(size) DEF_FONT_FILE(size)
#define DEF_FONT_FILE_SMALL  _DEF_FONT_FILE(DEF_FONT_SIZE_SMALL)
#define DEF_FONT_FILE_NORMAL _DEF_FONT_FILE(DEF_FONT_SIZE_NORMAL)
#define DEF_FONT_FILE_LARGE  _DEF_FONT_FILE(DEF_FONT_SIZE_LARGE)
#define DEF_FONT_FILE_XLARGE _DEF_FONT_FILE(DEF_FONT_SIZE_XLARGE)

#define DEF_UI_FPS                    CONFIG_PANEL_TIMING_REFRESH_RATE_HZ
#define DEF_UI_LOAD_ANIM_DURATION_MS  500
#define DEF_UI_LOAD_ANIM_TYPE         APP_UI_LOAD_ANIM_NONE
//#define DEF_UI_LOAD_ANIM_TYPE         APP_UI_LOAD_ANIM_ZOOM_LEFT

#ifdef CONFIG_LVGL
#  define APP_UI_LOAD_ANIM_NONE       LV_SCR_LOAD_ANIM_NONE
#  define APP_UI_LOAD_ANIM_LEFT       LV_SCR_LOAD_ANIM_MOVE_LEFT
#  define APP_UI_LOAD_ANIM_RIGHT      LV_SCR_LOAD_ANIM_MOVE_RIGHT
#  define APP_UI_LOAD_ANIM_ZOOM_LEFT  LV_SCR_LOAD_ANIM_ZOOM_LEFT
#  define APP_UI_LOAD_ANIM_ZOOM_RIGHT LV_SCR_LOAD_ANIM_ZOOM_RIGHT
#else
#  define APP_UI_LOAD_ANIM_NONE       0
#  define APP_UI_LOAD_ANIM_LEFT       1
#  define APP_UI_LOAD_ANIM_RIGHT      2
#  define APP_UI_LOAD_ANIM_ZOOM_LEFT  3
#  define APP_UI_LOAD_ANIM_ZOOM_RIGHT 4
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**********************
 *      TYPEDEFS
 **********************/

enum APP_UI_VIEW_USER_MSG {
	MSG_VIEW_LOAD_UI = MSG_VIEW_USER_OFFSET,
	MSG_VIEW_FOCUS_SCENE,
	MSG_VIEW_CHANGE_SCROLLABLE, /* control scene scrollable */
};

enum APP_UI_VIEW_ID {
	APP_MAIN_VIEW = VIEW_ID_USER_OFFSET /* 1 */,
};

enum APP_UI_ID {
	APP_UI_INVALID = 0,

	MAIN_UI, /* 1 */
	SVG_UI,
	CUBE_UI,
	FONT_UI,
};

enum APP_UI_SCENE_ID {
	APP_SCENE_INVALID = 0,

	MAIN_SCENE, /* 1 */
	SVG_SCENE,
	CUBE_SCENE,
	FONT_SCENE,
};

/**
 * @typedef app_ui_scene_proc_t
 * @brief UI proc callback of APP_UI_SCENE_ID
 *
 * @param parent pointer to the parent object, usually the screen object.
 *               For LVGL, it is the pointer to structure lv_obj_t.
 * @param msg_id message id, see enum UI_MSG_ID
 * @param msg_data message specific data
 *
 * @retval 0 on success else negative code.
 */
typedef int (*app_ui_scene_proc_t)(void * parent, uint8_t msg_id, void * msg_data);

typedef struct {
	/** enum APP_UI_SCENE_ID */
	uint16_t id;
	/** Scene target fps. If left 0, it will fallback to DEF_UI_FPS */
	uint8_t fps;
	/** Scene proc */
	app_ui_scene_proc_t proc;
	/** Scene presenter */
	const void *presenter;
} app_ui_scene_info_t;

typedef struct {
	/** enum APP_UI_ID */
	uint16_t id;
	/** main direction. 0: horizontal, 1: vertical */
	uint16_t vertical : 1;
	/** main scroll behaviour (require 3 main scenes at least). 0: non-rotated, 1: rotated */
	uint16_t main_scrl_rotated : 1;
	/** cross scroll behaviour. 0: push-pull, 1: overlapped */
	uint16_t cross_scrl_overlapped : 1;
	/** go back to previous UI invoked by gesture or not. 0: enabled, 1: disabled */
	uint16_t back_gesture_disabled : 1;
	/**
	 * number of scenes in left, right, top and bottom sides repectively
	 * relative to the center scene along the main direction.
	 */
	uint8_t n_scenes[4];
	/**
	 * scene entries arranged in th orders left -> center -> right, then
	 * top -> bottom along the main direction.
	 */
	const app_ui_scene_info_t *scenes;
} app_ui_info_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

#ifdef CONFIG_LVGL

/**
 * Helper macros for default font management.
 */
#ifdef CONFIG_LVGL_USE_FREETYPE_FONT
#  define LVGL_FONT_OPEN_DEFAULT(font, size) lvgl_freetype_font_open(font, DEF_VFONT_FILE, size)
#  define LVGL_FONT_OPEN(font, path, size)   lvgl_freetype_font_open(font, path, size)
#  define LVGL_FONT_CLOSE(font)              lvgl_freetype_font_close(font)
#  define LVGL_FONT_SET_EMOJI(font, emoji_path) \
				lvgl_freetype_font_set_emoji_font(font, emoji_path)
#  define LVGL_FONT_SET_DEFAULT_CODE(font, code, emoji_code) \
				lvgl_freetype_font_set_default_code(font, code, emoji_code)
#elif defined(CONFIG_LVGL_USE_BITMAP_FONT)
#  define LVGL_FONT_OPEN_DEFAULT(font, size) lvgl_bitmap_font_open(font, DEF_FONT_FILE(size))
#  define LVGL_FONT_OPEN(font, path, size)   lvgl_bitmap_font_open(font, path)
#  define LVGL_FONT_CLOSE(font)              lvgl_bitmap_font_close(font)
#  define LVGL_FONT_SET_EMOJI(font, emoji_path) \
				lvgl_bitmap_font_set_emoji_font(font, emoji_path)
#  define LVGL_FONT_SET_DEFAULT_CODE(font, code, emoji_code) \
				lvgl_bitmap_font_set_default_code(font, code, emoji_code)
#else
#  define LVGL_FONT_OPEN_DEFAULT(font, size)                 (-ENOSYS)
#  define LVGL_FONT_CLOSE(font)
#  define LVGL_FONT_SET_EMOJI(font, emoji_path)              (-ENOSYS)
#  define LVGL_FONT_SET_DEFAULT_CODE(font, code, emoji_code) (-ENOSYS)
#endif /* CONFIG_LVGL_USE_FREETYPE_FONT */

static inline void lvgl_style_array_reset(lv_style_t * styles, int num)
{
	for (int i = 0; i < num; i++)
		lv_style_reset(&styles[i]);
}

static inline void lvgl_obj_set_hidden(lv_obj_t * obj, bool hidden)
{
	if (hidden != lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN)) {
		if (hidden) {
			lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
		} else {
			lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
		}
	}
}

#endif /* CONFIG_LVGL */

/**
 * UI Stack API
 */

/**
 * @brief Initialize UI system
 *
 * @revtal 0 on success else negative code.
 */
int app_ui_init(void);

/**
 * @brief Initialize UI stack
 *
 * @param main_id main UI id
 *
 * @revtal 0 on success else negative code.
 */
int app_ui_stack_init(uint16_t main_id);

/**
 * @brief Uninitialize the UI stack
 *
 * @revtal 0 on success else negative code.
 */
int app_ui_stack_deinit(void);

/**
 * @brief Cleanup the UI stack
 *
 * @param blanking indicate blanking the display or not.
 *                 If true, the whole display turns black.
 *
 * @revtal 0 on success else negative code.
 */
int app_ui_stack_clean(bool blanking);

/**
 * @brief Get the current UI
 *
 * @param p_scene_id pointer to store current scene id
 *
 * @revtal the top UI id.
 */
uint16_t app_ui_stack_current(uint16_t * p_scene_id);

/**
 * @brief Get the previous UI
 *
 * @param p_scene_id pointer to store previous scene id
 *
 * @revtal the previous UI id.
 */
uint16_t app_ui_stack_prev(uint16_t * p_scene_id);

/**
 * @brief Forward next UI
 *
 * @param id UI id to push
 * @param anim_type Transition animation type
 *
 * @revtal 0 on success else negative code.
 */
int app_ui_stack_forward(uint16_t id, uint8_t anim_type);

/**
 * @brief Back to the previous UI
 *
 * @param anim_type Transition animation type
 *
 * @revtal 0 on success else negative code.
 */
int app_ui_stack_back(uint8_t anim_type);

/**
 * @brief Back to MAIN_UI directly
 *
 * @param anim_type Transition animation type
 *
 * @revtal 0 on success else negative code.
 */
int app_ui_stack_back_home(uint8_t anim_type);

/**
 * @brief Back to the previous UI invoked by gesture
 *
 * @param anim_type Transition animation type
 *
 * @revtal 0 on success else negative code.
 */
int app_ui_stack_back_gestured(uint8_t anim_type);

/**
 * @brief Forward next UI without recording current UI
 *
 * @param id UI id to be replaced with
 * @param anim_type Transition animation type
 *
 * @revtal 0 on success else negative code.
 */
int app_ui_stack_update(uint16_t id, uint8_t anim_type);

/**
 * @brief Update current scene of the current UI
 *
 * Just record current scene ID for query by app_ui_stack_current(),
 * and no scene transition invoked.
 *
 * @param scene_id Scene ID
 *
 * @revtal 0 on success else negative code.
 */
int app_ui_stack_update_scene(uint16_t scene_id);

/**
 * @brief Dump UI stack
 *
 * @revtal N/A.
 */
void app_ui_stack_dump(void);

/**
 * UI misc API
 */

/**
 * @brief Query the UI info
 *
 * @param id UI id
 *
 * @revtal pointer to structure app_ui_info_t.
 */
const app_ui_info_t * app_ui_get_info(uint16_t id);

/**
 * @brief Query the scene info
 *
 * @param info pointer to structure app_ui_info_t.
 * @param scene_id Scene ID
 *
 * @revtal pointer to structure app_ui_scene_info_t.
 */
const app_ui_scene_info_t * app_ui_get_scene_info(const app_ui_info_t * info, uint16_t scene_id);

/**
 * @brief Get scene count of specific UI
 *
 * @param info pointer to structure app_ui_info_t.
 *
 * @revtal scene count.
 */
uint16_t app_ui_get_scene_count(const app_ui_info_t * info);

/**
 * @brief Get scene target fps
 *
 * The scroll fps is the minimum value among all the scenes.
 *
 * @param scene_info pointer to structure app_ui_scene_info_t.
 *
 * @revtal target fps.
 */
static inline uint8_t app_ui_get_scene_fps(const app_ui_scene_info_t *scene_info)
{
	return (scene_info && scene_info->fps > 0) ? scene_info->fps : DEF_UI_FPS;
}

/**
 * @brief Get scroll fps of specific UI
 *
 * The scroll fps is the minimum value among all the scenes.
 *
 * @param info pointer to structure app_ui_info_t.
 * @param vertical scroll dir is vertical or not.
 *
 * @revtal scroll fps.
 */
uint8_t app_ui_get_scroll_fps(const app_ui_info_t * info, bool vertical);

/**
 * @brief Get transition anim fps between UI
 *
 * The transition fps is the minimum value among all the scenes.
 *
 * @param to pointer to next UI info.
 * @param from pointer to previous UI info.
 * @param anim_type transition anim type.
 *
 * @revtal scroll fps.
 */
uint8_t app_ui_get_transition_fps(const app_ui_info_t * to, const app_ui_info_t * from, uint8_t anim_type);

/**
 * @brief Set the pain message to the specific scene
 *
 * Set scene_id to APP_SCENE_INVALID for current scene.
 *
 * @param scene_id Scene id
 * @param scene_data Scene data
 *
 * @revtal 0 on success else negative code.
 */
static inline int app_ui_paint(uint16_t scene_id, void * scene_data)
{
	return ui_view_paint2(APP_MAIN_VIEW, scene_id, scene_data);
}

/**
 * @brief Focus on the specific scene
 *
 * @param scene_id Scene id
 *
 * @revtal 0 on success else negative code.
 */
static inline int app_ui_focus_scene(uint16_t scene_id)
{
	return ui_view_send_user_msg2(APP_MAIN_VIEW, MSG_VIEW_FOCUS_SCENE, scene_id, NULL);
}

/**
 * @brief Set target refresh rate
 *
 * @param fps target refresh rate in Hz
 *
 * @revtal 0 on success else negative code.
 */
static inline int app_ui_set_fps(uint8_t fps)
{
	return view_set_refresh_rate(APP_MAIN_VIEW, fps);
}

/**
 * @brief Make current scene scrollable
 *
 * Only available for multiple scenes in one UI.
 * Must called in UI thread context.
 * Will reset scrollable to be true after UI transition.
 *
 * @param scrollable indicate scrollable or not
 *
 * @revtal 0 on success else negative code.
 */
int app_ui_set_scene_scrollable(bool scrollable);

/**
 * @brief UI proc for multiple scenes in one UI
 *
 * @param scr Screen object
 * @param msg_id message id
 * @param msg_data message data
 *
 * @revtal 0 on success else negative code.
 */
int app_ui_scene_tiles_proc(void * scr, uint8_t msg_id, void * msg_data);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* _APP_UI_H_ */
