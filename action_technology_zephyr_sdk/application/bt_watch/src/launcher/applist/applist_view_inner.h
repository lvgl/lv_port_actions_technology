/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SRC_LAUNCHER_APPLIST_VIEW_INNER_H_
#define SRC_LAUNCHER_APPLIST_VIEW_INNER_H_

/*********************
 *      INCLUDES
 *********************/
#include "applist_ui.h"
#include "ui_coder/ui_coder.h"

/**********************
 *      TYPEDEFS
 **********************/
enum applist_idx {
	STOPWATCH_IDX = 0,
	ALARMCLOCK_IDX,
	TIMER_IDX,
	PHONE_IDX,
	VIBRATOR_IDX,
	AOD_IDX,
	RECOVERY_IDX,
	POWEROFF_IDX,
	ALIPAY_IDX,
	WXPAY_IDX,
	GPS_IDX,
	LONGVIEW_IDX,
	COMPASS_IDX,
	VIEWMODE_IDX,
	CUBEBOX_IDX,
	SVGMAP_IDX,
	VIB_IDX,
#ifdef CONFIG_THIRD_PARTY_APP
	THIRD_PARTY_APP_IDX,
#endif
	NUM_ITEMS,

	PHONE_OFF_IDX = NUM_ITEMS,
	VIBRATOR_OFF_IDX,
	AOD_OFF_IDX,

	NUM_ICONS,

	PHONE_ON_IDX = PHONE_IDX,
	VIBRATOR_ON_IDX = VIBRATOR_IDX,
	AOD_ON_IDX = AOD_IDX,
};

typedef struct {
	lv_coord_t *col_dsc;
	lv_coord_t *row_dsc;
} applist_grid_view_t;

typedef struct {
	lv_style_t sty_btn;
} applist_list_view_t;

typedef struct {
	lv_style_t sty_btn;
	lv_coord_t btn_h;
	lv_coord_t icon_space;
} applist_waterwheel_view_t;

typedef struct {
	int16_t radius;
	lv_point_t *pivots;

	bool scrolling;
	uint16_t scrl_nearest_idx;
	lv_point_t scrl_ofs;
	lv_point_t scrl_anim_end;
	lv_point_t scrl_anim_vect;
	lv_area_t scrl_area; /* bounding box of scrl_ofs */
	lv_point_t pressed_pt;
	int16_t max_zoom;
} applist_cellular_view_t;

typedef struct {
	lv_obj_t **obj_icon;
	uint32_t icon_num;
	lv_coord_t scroll_len;
	lv_coord_t end_y;
	lv_coord_t st_x;
	lv_coord_t w_space;
	lv_coord_t centre_scroll_y;
	lv_coord_t scroll_recover;
	lv_coord_t diff_h;
} applist_waterfall_view_t;

typedef struct {
	void *user_data;
	lv_obj_t *middle_icon;
	uint32_t icon_num;
	lv_point_t qua_point;
	int32_t angle;
	uint32_t move_time;
	int32_t icon_up_angle;
	bool b_move;
} applist_turntable_view_t;

typedef struct {
	void *user_data;
	lv_point_t first_point;
	uint32_t icon_num;
	int32_t max_radius;
	uint32_t move_time;
	int32_t move_radius;
	bool b_move;
} applist_wonhot_view_t;

typedef struct {
	void *user_data;
	lv_obj_t *list_name_txt;
	int32_t middle_id;
	uint32_t icon_num;
	lv_point_t qua_point;
	int32_t angle;
	uint32_t move_time;
	int32_t icon_up_angle;
	bool b_move;
}applist_cube_turntable_view_t;

typedef struct {
	void *user_data;
	uint32_t y_all_tick;
	int32_t y_all;
	int32_t min_y;
	int32_t move_y;
}applist_circle_view_t;

struct applist_ui_data;

typedef struct {
	int ( * create)(lv_obj_t * scr);
	void ( * delete)(lv_obj_t * scr);
	void ( * defocus)(lv_obj_t * scr);
	void (* update_icon)(struct applist_ui_data * data, uint8_t idx, const lv_image_dsc_t * src);
	void (* update_text)(struct applist_ui_data * data, uint8_t idx, const char * text);
} applist_view_cb_t;

typedef struct applist_ui_data {
	/* container object */
	lv_obj_t * cont;

	/* specific data of view mode */
	union {
		applist_grid_view_t grid_view;
		applist_list_view_t list_view;
		applist_waterwheel_view_t waterwheel_view;
		applist_cellular_view_t cellular_view;
		applist_waterfall_view_t waterfall_view;
		applist_turntable_view_t turntable_view;
		applist_wonhot_view_t wonhot_view;
		applist_cube_turntable_view_t cube_turntable_view;
		applist_circle_view_t circle_view;
	};

	/* specific callback of view mode */
	const applist_view_cb_t * view_cb;
	/* shared presenter of all applist modes */
	const applist_view_presenter_t * presenter;

	/* lvgl resource */
	lv_font_t font;
	lv_image_dsc_t icon[NUM_ICONS];
	lvgl_res_string_t text[NUM_ICONS];
	lvgl_res_scene_t res_scene;

	lv_coord_t item_height;
	lv_coord_t item_space;
	lv_coord_t pad_top;
	lv_coord_t pad_bottom;
	lv_coord_t track_radius;
	lv_point_t track_center;
	void *user_data;
} applist_ui_data_t;

extern const applist_view_cb_t g_applist_grid_view_cb;
extern const applist_view_cb_t g_applist_list_view_cb;
extern const applist_view_cb_t g_applist_waterwheel_view_cb;
extern const applist_view_cb_t g_applist_cellular_view_cb;
extern const applist_view_cb_t g_applist_turntable_view_cb;
extern const applist_view_cb_t g_applist_waterfall_view_cb;
extern const applist_view_cb_t g_applist_wonhot_view_cb;
extern const applist_view_cb_t g_applist_cube_turntable_view_cb;
extern const applist_view_cb_t g_applist_circle_view_cb;
#if defined(CONFIG_VG_LITE)
extern const applist_view_cb_t g_applist_roller3D_view_cb;
#endif
/**********************
 * GLOBAL PROTOTYPES
 **********************/
/** Common functions */
const lv_image_dsc_t * applist_get_icon(applist_ui_data_t *data, uint8_t idx);
const char * applist_get_text(applist_ui_data_t *data, uint8_t idx);
void applist_btn_event_def_handler(lv_event_t * e);

#endif /* SRC_LAUNCHER_APPLIST_VIEW_INNER_H_ */
