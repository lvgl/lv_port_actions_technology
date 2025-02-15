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
#ifdef CONFIG_INPUT_MANAGER
#  include <input_manager.h>
#endif

#ifdef CONFIG_UI_MANAGER
#  include <ui_manager.h>
#  include <ui_effects/scroll_effect.h>
#  include <ui_effects/switch_effect.h>
#endif

#ifdef CONFIG_LVGL
#  include <lvgl/lvgl.h>
#  include <lvgl/src/lvgl_private.h>
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

#ifndef CONFIG_UI_MANAGER
#  define VIEW_ID_USER_OFFSET		1
#  define VIEW_INVALID_ID			0
#endif

#if CONFIG_PANEL_HOR_RES >= 466
#  include "../resource/466x466/bt_watch_sty.h"
#  include "../resource/466x466/logo_sty.h"

#define DEF_UI_WIDTH   466
#define DEF_UI_HEIGHT  466
#define DEF_FONT_SIZE_SMALL  28
#define DEF_FONT_SIZE_NORMAL 32
#define DEF_FONT_SIZE_LARGE  32
#define DEF_FONT_SIZE_XLARGE 32

#elif CONFIG_PANEL_HOR_RES >= 454
#  include "../resource/454x454/bt_watch_sty.h"
#  include "../resource/454x454/logo_sty.h"

#define DEF_UI_WIDTH   454
#define DEF_UI_HEIGHT  454
#define DEF_FONT_SIZE_SMALL  28
#define DEF_FONT_SIZE_NORMAL 32
#define DEF_FONT_SIZE_LARGE  32
#define DEF_FONT_SIZE_XLARGE 32

#elif CONFIG_PANEL_HOR_RES >= 360
#  include "../resource/360x360/bt_watch_sty.h"
#  include "../resource/360x360/logo_sty.h"

#define DEF_UI_WIDTH   360
#define DEF_UI_HEIGHT  360
#define DEF_FONT_SIZE_SMALL  22
#define DEF_FONT_SIZE_NORMAL 24
#define DEF_FONT_SIZE_LARGE  24
#define DEF_FONT_SIZE_XLARGE 24

#else
#  include "../resource/240x280/bt_watch_sty.h"
#  include "../resource/240x280/logo_sty.h"

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
/* default ui view resolution */
#define DEF_UI_VIEW_WIDTH   CONFIG_PANEL_HOR_RES
#define DEF_UI_VIEW_HEIGHT  CONFIG_PANEL_VER_RES

/* click area judge */
#define DEF_CLICK_SCOPE 5

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
#define DEF_VF_50      		DEF_FONT_DISK"/vf_50.font"
#define DEF_VF_30      		DEF_FONT_DISK"/vf_30.font"
#define DEF_VF_26      		DEF_FONT_DISK"/vf_26.font"
#define DEF_VFONT_FILE      DEF_FONT_DISK"/vfont.ttf"

#define DEF_FONT_FILE(size)  DEF_FONT_DISK"/sans"#size".font"
#define _DEF_FONT_FILE(size) DEF_FONT_FILE(size)
#define DEF_FONT_FILE_SMALL  _DEF_FONT_FILE(DEF_FONT_SIZE_SMALL)
#define DEF_FONT_FILE_NORMAL _DEF_FONT_FILE(DEF_FONT_SIZE_NORMAL)
#define DEF_FONT_FILE_LARGE  _DEF_FONT_FILE(DEF_FONT_SIZE_LARGE)
#define DEF_FONT_FILE_XLARGE _DEF_FONT_FILE(DEF_FONT_SIZE_XLARGE)

#define WELCOME_UI_DISK     CONFIG_WELCOME_UI_DISK
#define WELCOME_STY_FILE    WELCOME_UI_DISK"/logo.sty"
#define WELCOME_RES_FILE    WELCOME_UI_DISK"/logo.res"
#define GOODBYE_STY_FILE    WELCOME_UI_DISK"/logo.sty"
#define GOODBYE_RES_FILE    WELCOME_UI_DISK"/logo.res"
#if CONFIG_PANEL_ROUND_SHAPE
#  define DEF_UI_VSCROLL_EFFECT UI_VSCROLL_EFFECT_ALPHA
#else
#  define DEF_UI_VSCROLL_EFFECT UI_VSCROLL_EFFECT_NONE
#endif

/* configuration name definition */
#define CFG_AOD_MODE        "AOD_MODE"
#define CFG_CLOCK_ID        "CLOCK_ID"
#define CFG_APPLIST_MODE     "APPLIST_MODE"
#define CFG_SCROLL_MODE     "SCROLL_MODE"
#define CFG_SWITCH_MODE     "SWITCH_MODE"

#ifdef __cplusplus
extern "C" {
#endif

/**********************
 *      TYPEDEFS
 **********************/
enum {
	NEED_INIT_BT,
	NO_NEED_INIT_BT,
};

enum {
	// bt common key message
	MSG_BT_PLAY_TWS_PAIR = MSG_APP_MESSAGE_START, /* 200 */
	MSG_BT_PLAY_DISCONNECT_TWS_PAIR,
	MSG_BT_PLAY_CLEAR_LIST,
	MSG_BT_SIRI_START,
	MSG_BT_SIRI_STOP, /* 204 */
	MSG_BT_CALL_LAST_NO,
	MSG_BT_HID_START,
	MSG_KEY_POWER_OFF,
	MSG_BT_APP_MESSAGE_START, /* 208 */
	MSG_BT_TWS_SWITCH_MODE,
	MSG_RECORDER_START_STOP,
	MSG_GMA_RECORDER_START,
	MSG_ALARM_ENTRY_EXIT,

	MSG_APP_MESSAGE_MAX_INDEX, /*213*/
};

enum APP_UI_VIEW_ORDER {
	HIGHEST_ORDER,
	HIGH_ORDER,
	NORMAL_ORDER,
};

enum APP_UI_VIEW_ID {
	MAIN_VIEW = VIEW_ID_USER_OFFSET /* 1 */,
	BTMUSIC_VIEW,
	BTCALL_VIEW,
	TWS_VIEW,
	LCMUSIC_VIEW,
	LOOP_PLAYER_VIEW,
	LINEIN_VIEW,
	AUDIO_INPUT_VIEW,
	RECORDER_VIEW,
	USOUND_VIEW,
	CARD_READER_VIEW,
	CHARGER_VIEW,
	ALARM_VIEW,
	FM_VIEW,
	OTA_VIEW,
	CLOCK_VIEW,				// 16
	CLOCK_SELECTOR_VIEW,	// 17
	APPLIST_VIEW,			// 18
	SPORT_VIEW,				// 19
	HEART_VIEW,				// 20
	MSG_VIEW,				// 21
	COMPASS_VIEW,			// 22
	MUSIC_VIEW,				// 23
	HEALTH_BP_VIEW,			// 24
	HEALTH_SPO2_VIEW,		// 25
	POWER_VIEW,
	STOPWATCH_VIEW,
	ALARM_SET_VIEW,
	LOW_POWER_VIEW,

	AOD_CLOCK_VIEW,
	TEST_LONG_VIEW,

	/* Test views to demo 'bar' shared with 3 views */
	TEST_TOP_VIEW1,        // 32
	TEST_TOP_VIEW2,
	TEST_TOP_VIEW3,
	TEST_BAR_VIEW,

	WELCOME_VIEW, // 36
	GOODBYE_VIEW, // 37

	ALIPAY_MAIN_VIEW,		//38
	ALIPAY_BIND_VIEW, 		//39
	ALIPAY_PAY_VIEW, 		//40
	ALIPAY_UNBIND_VIEW, 	//41

	WXPAY_MAIN_VIEW,		//42
	WXPAY_BIND_VIEW, 		//43
	WXPAY_PAY_VIEW, 		//44
	WXPAY_UNBIND_VIEW, 		//45

	GPS_VIEW,               //46
	CUBEBOX_VIEW,
	SVGMAP_VIEW,
	SETTING_VIEW,
	APPLIST_ANIM_SET_VIEW,
	SCROLL_ANIM_SET_VIEW,
	SWITCH_ANIM_SET_VIEW,
	VIB_VIEW,               //53
	THREE_DIMENSIONAL_VIEW,
	FACE_WHEEL_VIEW,
	EFFECT_WHEEL_VIEW,
	GIF_VIEW,               //57
	ALIPAY_MENU_VIEW,
	ALIPAY_CARDLIST_VIEW,
	ALIPAY_TRANSITCODE_VIEW,
	SIM_MENU_VIEW, //61
	SIM_DEVICE_INFO_VIEW,
	SIM_TELE_VIEW,
	SIM_POWER_SET_VIEW,
	SIM_LOG_SET_VIEW,
	SIM_CARD_INFORMATION_VIEW, //66
	SIM_SIGNAL_STATE_VIEW,
	SIM_HOUSING_INFORMATION_VIEW,
	SIM_NETWORK_STATE_VIEW,
	SIM_CALL_SET_VIEW, //70
	SIM_CARD_SET_VIEW,
	SIM_CALL_DIAL_VIEW,
	SIM_SMS_SET_VIEW,
	THIRD_PARTY_APP_VIEW,
	GLYPHIX_APPLET_VIEW,

	LOTTIE_VIEW, //76
	SIM_SMS_LOAD_LIST_VIEW,
	AWK_MAP_VIEW,
};

enum APP_UI_MSGBOX_ID {
	ALARM_MSGBOX_ID = MSGBOX_ID_USER_OFFSET, /* 1 */
	OTA_MSGBOX_ID,
	BTCALL_MSGBOX_ID,
	LPOWER_MSGBOX_ID,
};

enum APP_UI_EVENT_ID {
	UI_EVENT_NONE = 0,
	UI_EVENT_POWER_ON,
	UI_EVENT_POWER_OFF,
	UI_EVENT_STANDBY,
	UI_EVENT_WAKE_UP,

	UI_EVENT_LOW_POWER,
	UI_EVENT_NO_POWER,
	UI_EVENT_CHARGE,
	UI_EVENT_CHARGE_OUT,
	UI_EVENT_POWER_FULL,

	UI_EVENT_ENTER_PAIRING,
	UI_EVENT_CLEAR_PAIRED_LIST,
	UI_EVENT_WAIT_CONNECTION,
	UI_EVENT_CONNECT_SUCCESS,
	UI_EVENT_SECOND_DEVICE_CONNECT_SUCCESS,
	UI_EVENT_BT_DISCONNECT,
	UI_EVENT_TWS_START_TEAM,
	UI_EVENT_TWS_TEAM_SUCCESS,
	UI_EVENT_TWS_DISCONNECTED,
	UI_EVENT_TWS_PAIR_FAILED,

	UI_EVENT_VOLUME_CHANGED,
	UI_EVENT_MIN_VOLUME,
	UI_EVENT_MAX_VOLUME,

	UI_EVENT_PLAY_START,
	UI_EVENT_PLAY_PAUSE,
	UI_EVENT_PLAY_PREVIOUS,
	UI_EVENT_PLAY_NEXT,

	UI_EVENT_BT_INCOMING,
	UI_EVENT_BT_OUTGOING,
	UI_EVENT_BT_START_CALL,
	UI_EVENT_BT_ONGOING,
	UI_EVENT_BT_HANG_UP,
	UI_EVENT_START_SIRI,
	UI_EVENT_STOP_SIRI,
	UI_EVENT_BT_CALLRING,

	UI_EVENT_MIC_MUTE,
	UI_EVENT_MIC_MUTE_OFF,
	UI_EVENT_TAKE_PICTURES,
	UI_EVENT_FACTORY_DEFAULT,

	UI_EVENT_ENTER_BTMUSIC,
	UI_EVENT_ENTER_LINEIN,
	UI_EVENT_ENTER_USOUND,
	UI_EVENT_ENTER_SPDIF_IN,
	UI_EVENT_ENTER_I2SRX_IN,
	UI_EVENT_ENTER_SDMPLAYER,
	UI_EVENT_ENTER_UMPLAYER,
	UI_EVENT_ENTER_NORMPLAYER,
	UI_EVENT_ENTER_SDPLAYBACK,
	UI_EVENT_ENTER_UPLAYBACK,
	UI_EVENT_ENTER_RECORD,
	UI_EVENT_ENTER_MIC_IN,
	UI_EVENT_ENTER_FM,
};

struct event_strmap_s {
	int8_t event;
	const char *str;
};

struct event_ui_map {
	uint8_t  sys_event;
	uint8_t  ui_event;
};

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
			lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
		}
	}
}

static inline bool lvgl_click_decision(void)
{
	lv_indev_t *indev = lv_indev_get_act();
	if(indev) {
		if (LV_ABS(indev->pointer.scroll_sum.x) <= DEF_CLICK_SCOPE
			&& LV_ABS(indev->pointer.scroll_sum.y) <= DEF_CLICK_SCOPE)
		return true;
	}
	return false;
}

#endif /* CONFIG_LVGL */

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* _APP_UI_H_ */
