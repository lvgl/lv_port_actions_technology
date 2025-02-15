/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*********************
 *      INCLUDES
 *********************/
#include <assert.h>
#include <os_common_api.h>
#include <widgets/text_canvas.h>
#include "applist_view_inner.h"

/**********************
 *      DEFINES
 **********************/
#define LIST_SCENE_ID SCENE_APPLIST8888_VIEW

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *   STATIC PROTOTYPES
 **********************/
static void _applist_unload_resource(applist_ui_data_t *data);

/**********************
 *  STATIC VARIABLES
 **********************/
static const uint32_t pic_grp_ids[] = {
	RES_STOPWATCH, RES_ALARMCLOCK, RES_TIMER, RES_PHONE_ON,
	RES_VIBRATION_ON, RES_AOD_ON, RES_RECOVERY, RES_POWEROFF,
	RES_ALIPAY, RES_WXPAY, RES_GPS, RES_TEST_LONGVIEW, RES_COMPASS,
	RES_VIEW_MODE, RES_CUBE_BOX, RES_SVG_MAP, RES_VIB,
#ifdef CONFIG_THIRD_PARTY_APP
	RES_THIRD_PARTY_APP,
#endif
#ifdef CONFIG_AWK_LIB
	RES_AWK_MAP,
#endif
	RES_PHONE_OFF, RES_VIBRATION_OFF, RES_AOD_OFF,
};

static const applist_view_cb_t * const sp_view_callbacks[] = {
	[APPLIST_MODE_LIST] = &g_applist_list_view_cb,
	[APPLIST_MODE_GRID] = &g_applist_grid_view_cb,
	[APPLIST_MODE_WATERWHEEL] = &g_applist_waterwheel_view_cb,
	[APPLIST_MODE_CELLULAR] = &g_applist_cellular_view_cb,
	[APPLIST_MODE_TURNTABLE] = &g_applist_turntable_view_cb,
	[APPLIST_MODE_WATERFALL] = &g_applist_waterfall_view_cb,
	[APPLIST_MODE_WONHOT] = &g_applist_wonhot_view_cb,
	[APPLIST_MODE_CUBE_TURNTABLE] = &g_applist_cube_turntable_view_cb,
	[APPLIST_MODE_CIRCLE] = &g_applist_circle_view_cb,
#if defined(CONFIG_VG_LITE)
	[APPLIST_MODE_3D_WHEEL] = &g_applist_roller3D_view_cb,
#endif
};

/**********************
 *   STATIC FUNCTIONS
 **********************/
static int _applist_load_resource(applist_ui_data_t *data)
{
	lvgl_res_group_t res_grp;
	int ret;

	/* scene */
	ret = lvgl_res_load_scene(LIST_SCENE_ID, &data->res_scene, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (ret < 0) {
		SYS_LOG_ERR("scene not found");
		return -ENOENT;
	}

	ret = lvgl_res_load_group_from_scene(&data->res_scene, RES_DRAG_TRACK, &res_grp);
	if (ret < 0) {
		data->track_radius = 0;
	} else {
		data->track_center.x = res_grp.x;
		data->track_center.y = res_grp.y;
		data->track_radius = res_grp.width;
		lvgl_res_unload_group(&res_grp);
		SYS_LOG_INF("drag circle center (%d, %d), radius %d", data->track_center.x,
				data->track_center.y, data->track_radius);
	}

	ret = lvgl_res_load_group_from_scene(&data->res_scene, RES_PADTOP, &res_grp);
	if (ret < 0) {
		data->pad_top = 0;
	} else {
		data->pad_top = res_grp.height;
		lvgl_res_unload_group(&res_grp);
	}

	ret = lvgl_res_load_group_from_scene(&data->res_scene, RES_PADBOTTOM, &res_grp);
	if (ret < 0) {
		data->pad_bottom = 0;
	} else {
		data->pad_bottom = res_grp.height;
		lvgl_res_unload_group(&res_grp);
		SYS_LOG_INF("drag pad top %d, bottom %d", data->pad_top, data->pad_bottom);
	}

	for (int i = 0; i < ARRAY_SIZE(pic_grp_ids); i++) {
		uint32_t res_id;
		lv_point_t pic_pos;

		ret = lvgl_res_load_group_from_scene(&data->res_scene, pic_grp_ids[i], &res_grp);
		if (ret < 0) {
			goto fail_exit;
		}

		res_id = PIC_ICON;
		ret = lvgl_res_load_pictures_from_group(&res_grp, &res_id, &data->icon[i], &pic_pos, 1);
		if (ret < 0) {
			lvgl_res_unload_group(&res_grp);
			goto fail_exit;
		}

		res_id = STR_TEXT;
		ret = lvgl_res_load_strings_from_group(&res_grp, &res_id, &data->text[i], 1);
		if (ret < 0) {
			lvgl_res_unload_group(&res_grp);
			goto fail_exit;
		}

		data->item_height = res_grp.height;
		data->item_space = data->text[i].x - (pic_pos.x + data->icon[i].header.w);
		lvgl_res_unload_group(&res_grp);
	}

#if LV_COLOR_DEPTH == 16
	data->item_space &= ~0x1;
#endif

	/* open font */
	if (LVGL_FONT_OPEN_DEFAULT(&data->font, DEF_FONT_SIZE_NORMAL) < 0) {
		goto fail_exit;
	}

	lvgl_res_unload_scene(&data->res_scene);
	return 0;
fail_exit:
	_applist_unload_resource(data);
	lvgl_res_unload_scene(&data->res_scene);
	return -ENOENT;
}

static void _applist_unload_resource(applist_ui_data_t *data)
{
	LVGL_FONT_CLOSE(&data->font);

	lvgl_res_unload_pictures(data->icon, ARRAY_SIZE(data->icon));
	lvgl_res_unload_strings(data->text, ARRAY_SIZE(data->text));
}

static const applist_view_cb_t * _get_view_cb(applist_ui_data_t *data)
{
	uint8_t mode = data->presenter->get_view_mode();
	if (mode >= ARRAY_SIZE(sp_view_callbacks))
		mode = 0;
	SYS_LOG_INF("applist mode %d",mode);
	return sp_view_callbacks[mode];
}

static int _applist_view_preload(void)
{
	return lvgl_res_preload_scene_compact(LIST_SCENE_ID, NULL, 0,
			lvgl_res_scene_preload_default_cb_for_view, (void *)APPLIST_VIEW,
			DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
}

static int _applist_view_layout(lv_obj_t * scr, const applist_view_presenter_t * presenter)
{
	applist_ui_data_t *data;
	int res;

	data = app_mem_malloc(sizeof(*data));
	if (data == NULL)
		return -ENOMEM;

	memset(data, 0, sizeof(*data));
	data->presenter = presenter;
	data->view_cb = _get_view_cb(data);

	res = _applist_load_resource(data);
	if (res) {
		app_mem_free(data);
		return res;
	}

	lv_obj_set_user_data(scr, data);

	res = data->view_cb->create(scr);
	if (res) {
		_applist_unload_resource(data);
		app_mem_free(data);
		lv_obj_set_user_data(scr, NULL);
		return res;
	}

	lv_obj_set_style_bg_color(scr, data->res_scene.background, 0);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

	/* set system gesture */
	ui_gesture_lock_scroll();

	return 0;
}

static int _applist_view_delete(lv_obj_t * scr)
{
	applist_ui_data_t *data = lv_obj_get_user_data(scr);

	if (data) {
		data->view_cb->delete(scr);
		lv_obj_clean(scr);
		_applist_unload_resource(data);
		app_mem_free(data);

		/* restore system gesture */
		ui_gesture_unlock_scroll();
	}

	lvgl_res_preload_cancel_scene(LIST_SCENE_ID);
	lvgl_res_unload_scene_compact(LIST_SCENE_ID);

	return 0;
}

static int _applist_view_defocus(lv_obj_t * scr)
{
	applist_ui_data_t *data = lv_obj_get_user_data(scr);
	if (data == NULL)
		return 0;

	if (data->view_cb->defocus)
		data->view_cb->defocus(scr);

	return 0;
}

static int _applist_view_paint(lv_obj_t * scr, view_user_msg_data_t * msg_data)
{
	applist_ui_data_t *data = lv_obj_get_user_data(scr);
	if (data == NULL)
		return 0;

	if (msg_data == NULL || msg_data->id != VIEWMODE_IDX)
		return 0;

	/* exit old view */
	data->view_cb->delete(scr);
	lv_obj_clean(scr);

	/* update view mode */
	uint8_t mode = data->presenter->get_view_mode();
	if (++mode == NUM_APPLIST_MODES)
		mode = APPLIST_MODE_LIST;

	/* enter new view */
	data->presenter->set_view_mode(mode);
	data->view_cb = _get_view_cb(data);
	data->view_cb->create(scr);

	return 0;
}

static int _applist_proc_key(lv_obj_t * scr, ui_key_msg_data_t * key_data)
{
	applist_ui_data_t *data = lv_obj_get_user_data(scr);
	if (data == NULL)
		return 0;

	if (KEY_VALUE(key_data->event) == KEY_GESTURE_RIGHT) {
		uint8_t mode = data->presenter->get_view_mode();
		if (mode == APPLIST_MODE_CELLULAR || mode == APPLIST_MODE_TURNTABLE
			|| mode == APPLIST_MODE_WONHOT || mode == APPLIST_MODE_CUBE_TURNTABLE) {
			key_data->done = true; /* ignore gesture */
		}
	}

	return 0;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

const lv_image_dsc_t * applist_get_icon(applist_ui_data_t *data, uint8_t idx)
{
	const applist_view_presenter_t *presenter = data->presenter;

	switch (idx) {
	case PHONE_IDX:
		return presenter->phone_is_on() ?
				&data->icon[PHONE_ON_IDX] : &data->icon[PHONE_OFF_IDX];
	case VIBRATOR_IDX:
		return presenter->vibrator_is_on() ?
				&data->icon[VIBRATOR_ON_IDX]: &data->icon[VIBRATOR_OFF_IDX];
	case AOD_IDX:
		return presenter->aod_mode_is_on() ?
				&data->icon[AOD_ON_IDX]: &data->icon[AOD_OFF_IDX];
	default:
		{
			while(idx >= NUM_ICONS)
				idx -= NUM_ICONS;
			return &data->icon[idx];
		}
	}
}

const char * applist_get_text(applist_ui_data_t *data, uint8_t idx)
{
	const applist_view_presenter_t *presenter = data->presenter;

	switch (idx) {
	case PHONE_IDX:
		return presenter->phone_is_on() ?
				data->text[PHONE_ON_IDX].txt : data->text[PHONE_OFF_IDX].txt;
	case VIBRATOR_IDX:
		return presenter->vibrator_is_on() ?
				data->text[VIBRATOR_ON_IDX].txt: data->text[VIBRATOR_OFF_IDX].txt;
	case AOD_IDX:
		return presenter->aod_mode_is_on() ?
				data->text[AOD_ON_IDX].txt: data->text[AOD_OFF_IDX].txt;
	default:
		return data->text[idx].txt;
	}
}

void applist_btn_event_def_handler(lv_event_t * e)
{
	if(!lvgl_click_decision())
		return;
	lv_obj_t *btn = lv_event_get_current_target(e);
	applist_ui_data_t *data = lv_event_get_user_data(e);
	const applist_view_presenter_t *presenter = data->presenter;
	bool update_img = false;

	int idx = (int)lv_obj_get_user_data(btn);

	SYS_LOG_INF("click btn %d", idx);

	switch (idx) {
	case PHONE_IDX:
		presenter->toggle_phone();
		update_img = true;
		break;
	case VIBRATOR_IDX:
		presenter->toggle_vibrator();
		update_img = true;
		break;
	case AOD_IDX:
		presenter->toggle_aod_mode();
		update_img = true;
		break;
	case STOPWATCH_IDX:
		presenter->open_stopwatch();
		break;
	case ALARMCLOCK_IDX:
		presenter->open_alarmclock();
		break;
	case COMPASS_IDX:
		presenter->open_compass();
		break;
	case LONGVIEW_IDX:
		/* TEST: open test long view */
		presenter->open_longview();
		break;
	case ALIPAY_IDX:
		presenter->open_alipay();
		break;
	case WXPAY_IDX:
		presenter->open_wxpay();
		break;
	case GPS_IDX:
		presenter->open_gps();
		break;
	case VIEWMODE_IDX:
		//ui_view_paint2(APPLIST_VIEW, VIEWMODE_IDX, NULL); /* just a flag */
		presenter->open_setting();
		break;
	case CUBEBOX_IDX:
		presenter->open_three_dimensional();
		break;
	case SVGMAP_IDX:
		presenter->open_svgmap();
		break;
	case VIB_IDX:
		presenter->open_vib();
		break;
#ifdef CONFIG_THIRD_PARTY_APP
	case THIRD_PARTY_APP_IDX:
		presenter->open_third_party_app();
		break;
#endif
#ifdef CONFIG_AWK_LIB
	case AWK_MAP_IDX:
		presenter->open_awk_map();
		break;
#endif
	case RECOVERY_IDX:
	case POWEROFF_IDX:
	case TIMER_IDX:
	default:
		break;
	}

	if (update_img) {
		if (data->view_cb->update_icon) {
			data->view_cb->update_icon(data, idx, applist_get_icon(data, idx));
		}

		if (data->view_cb->update_text) {
			data->view_cb->update_text(data, idx, applist_get_text(data, idx));
		}
	}
}

int applist_view_handler(uint16_t view_id, view_data_t * view_data, uint8_t msg_id, void * msg_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);

	assert(view_id == APPLIST_VIEW);

	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return _applist_view_preload();
	case MSG_VIEW_LAYOUT:
		return _applist_view_layout(scr, view_data->presenter);
	case MSG_VIEW_DELETE:
		return _applist_view_delete(scr);
	case MSG_VIEW_DEFOCUS:
		return _applist_view_defocus(scr);
	case MSG_VIEW_PAINT:
		return _applist_view_paint(scr, msg_data);
	case MSG_VIEW_KEY:
		return _applist_proc_key(scr, msg_data);
	default:
		return 0;
	}
}

VIEW_DEFINE2(applist, applist_view_handler, NULL, NULL, APPLIST_VIEW,
		NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);
