/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file main view
 */
#include <stdio.h>

#include <assert.h>
#include <lvgl.h>
#include <lvgl/lvgl_res_loader.h>
#include <lvgl/lvgl_bitmap_font.h>
#include <view_manager.h>
#include <ui_manager.h>
#ifdef CONFIG_SYS_WAKELOCK
#include <sys_wakelock.h>
#endif
#ifdef CONFIG_PROPERTY
#include <property_manager.h>
#endif
#ifdef CONFIG_BT_MANAGER
#include <bt_manager.h>
#endif
#include <view_stack.h>
#include "app_ui.h"
#include "app_defines.h"
#include "main_view.h"
#ifdef CONFIG_CAT1_APP
#include "ui_sim.h"
#endif

#define CFG_BTN_STA			"MV_BTN_STA"

enum {
	BTN_WEATHER = 0,
	BTN_BRIGHTNESS,
	BTN_BLUETOOTH,
	BTN_VIBRATOR,
	BTN_GLASS,
	BTN_SETTING,
	NUM_BTNS,
};

enum {
	IMG_BAT = 0,
	NUM_IMGS,
};

enum {
	TXT_DATE = 0,
	TXT_WEEK,
	TXT_BAT,
	NUM_TXTS,
};

enum {
	BMP_BAT1 = 0,
	BMP_BAT2,
	BMP_BAT3,
	BMP_BAT4,
	BMP_BAT5,
	NUM_BATS,
};

const static uint32_t _btn_def_ids[] = {
	PIC_WEATHER, PIC_BRIGHTNESS, PIC_BLUETOOTH,
	PIC_VIBRATOR, PIC_GLASS, PIC_SETTING,
};

const static uint32_t _btn_sel_ids[] = {
	PIC_WEATHER_H, PIC_BRIGHTNESS_H, PIC_BLUETOOTH_H,
	PIC_VIBRATOR_H, PIC_GLASS_H, PIC_SETTING_H,
};

const static uint32_t _txt_ids[] = {
	STR_DATE,	STR_WEEK,	STR_BAT,
};

const static uint32_t _bat_ids[] = {
	PIC_BAT1, PIC_BAT2, PIC_BAT3, PIC_BAT4, PIC_BAT5,
};

const static char *_txt_week[7] = {
	"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
};

static int32_t main_preload_inited = 0;
typedef struct sys_view_data {
	/* lvgl object */
	lv_obj_t *btn[NUM_BTNS];
	lv_obj_t *img[NUM_IMGS];
	lv_obj_t *lbl[NUM_TXTS];

	/* ui-editor resource */
	lvgl_res_scene_t res_scene;
	lvgl_res_string_t res_txt[NUM_TXTS];

	/* lvgl resource */
	lv_point_t pt_def[NUM_BTNS];
	lv_point_t pt_sel[NUM_BTNS];
	lv_point_t pt_bat[NUM_BATS];
	lv_point_t pt_txt[NUM_TXTS];
	lv_image_dsc_t img_dsc_def[NUM_BTNS];
	lv_image_dsc_t img_dsc_sel[NUM_BTNS];
	lv_image_dsc_t img_dsc_bat[NUM_BATS];
	lv_style_t style_bg;
	lv_style_t style_txt[NUM_TXTS];
	lv_font_t font;

	/* user data */
	uint8_t bat_val;
	uint8_t btn_sta[NUM_BTNS];
} sys_view_data_t;

sys_view_data_t *p_sysview_data = NULL;

static int _main_view_paint(view_data_t *view_data);

static void _btn_evt_handler(lv_event_t * e)
{
	lv_event_code_t event = lv_event_get_code(e);
	int idx = (int)lv_event_get_user_data(e);

	if (event == LV_EVENT_CLICKED) {
		SYS_LOG_INF("Btn[%d] Clicked\n", idx);
	#ifdef CONFIG_CAT1_APP
		if(idx == BTN_SETTING)
			view_stack_push_view(SIM_MENU_VIEW, NULL);
	#endif
	} else if (event == LV_EVENT_VALUE_CHANGED) {
		SYS_LOG_INF("Btn[%d] Toggled\n", idx);
		if (idx == BTN_BRIGHTNESS) {
			// screen always on
			p_sysview_data->btn_sta[idx] = !p_sysview_data->btn_sta[idx];
#ifdef CONFIG_SYS_WAKELOCK
			if (p_sysview_data->btn_sta[BTN_BRIGHTNESS]) {
				sys_wake_lock(FULL_WAKE_LOCK);
			} else {
				sys_wake_unlock(FULL_WAKE_LOCK);
			}
#endif
		}
	}
}

static void _cvt_txt_array(lv_point_t *pt, lv_style_t *sty, lv_font_t *font, lvgl_res_string_t* txt, uint32_t num)
{
	int i;

	for (i = 0; i < num; i++) {
		pt[i].x = txt[i].x;
		pt[i].y = txt[i].y;

		lv_style_init(&sty[i]);
		lv_style_set_text_font(&sty[i], font);
		lv_style_set_text_color(&sty[i], txt[i].color);
		lv_style_set_text_align(&sty[i], LV_TEXT_ALIGN_CENTER);
	}
}

static void _create_btn_array(lv_obj_t *par, lv_obj_t **pobj, lv_point_t *pt,
										lv_image_dsc_t *def, lv_image_dsc_t *sel, uint32_t num)
{
	int i;

	for (i = 0; i < num; i++) {
		pobj[i] = lv_imagebutton_create(par);
		lv_obj_set_pos(pobj[i], pt[i].x, pt[i].y);
		lv_obj_set_size(pobj[i], def[i].header.w, def[i].header.h);
		lv_obj_set_user_data(pobj[i], (void*)i);
		lv_obj_add_flag(pobj[i], LV_OBJ_FLAG_CHECKABLE);
		lv_obj_add_event_cb(pobj[i], _btn_evt_handler, LV_EVENT_ALL, (void*)i);

		lv_imagebutton_set_src(pobj[i], LV_IMAGEBUTTON_STATE_RELEASED, NULL, &def[i], NULL);
		lv_imagebutton_set_src(pobj[i], LV_IMAGEBUTTON_STATE_PRESSED, NULL, &sel[i], NULL);
		lv_imagebutton_set_src(pobj[i], LV_IMAGEBUTTON_STATE_CHECKED_RELEASED, NULL, &sel[i], NULL);
		lv_imagebutton_set_src(pobj[i], LV_IMAGEBUTTON_STATE_CHECKED_PRESSED, NULL, &def[i], NULL);
	}
}

static void _create_img_array(lv_obj_t *par, lv_obj_t **pobj, lv_point_t *pt,
										lv_image_dsc_t *img, uint32_t num)
{
	int i;

	for (i = 0; i < num; i++) {
		pobj[i] = lv_image_create(par);
		lv_image_set_src(pobj[i], &img[i]);
		lv_obj_set_pos(pobj[i], pt[i].x, pt[i].y);
	}
}

static void _create_lbl_array(lv_obj_t *par, lv_obj_t **pobj, lv_point_t *pt,
										lv_style_t *sty, uint32_t num)
{
	int i;

	for (i = 0; i < num; i++) {
		pobj[i] = lv_label_create(par);
		lv_obj_set_pos(pobj[i], pt[i].x, pt[i].y);
		lv_obj_add_style(pobj[i], &sty[i], LV_PART_MAIN);
		lv_label_set_text(pobj[i], "");
	}
}

static void _delete_obj_array(lv_obj_t **pobj, uint32_t num)
{
	int i;

	for (i = 0; i < num; i++) {
		if (pobj[i]) {
			lv_obj_delete(pobj[i]);
			pobj[i] = NULL;
		}
	}
}

static int _load_resource(sys_view_data_t *data, bool first_layout)
{
	/* open font */
	if (first_layout)
	{
		if (LVGL_FONT_OPEN_DEFAULT(&data->font, DEF_FONT_SIZE_NORMAL) < 0) {
			SYS_LOG_ERR("font not found");
			return -ENOENT;
		}
		lvgl_res_load_scene(SCENE_SYSTEM_VIEW, &data->res_scene, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	}

	/* load resource */
	lvgl_res_load_pictures_from_scene(&data->res_scene, _btn_def_ids, data->img_dsc_def, data->pt_def, NUM_BTNS);
	lvgl_res_load_pictures_from_scene(&data->res_scene, _btn_sel_ids, data->img_dsc_sel, data->pt_sel, NUM_BTNS);
	lvgl_res_load_pictures_from_scene(&data->res_scene, _bat_ids, data->img_dsc_bat, data->pt_bat, NUM_BATS);

	if(first_layout)
	{
		lvgl_res_load_strings_from_scene(&data->res_scene, _txt_ids, data->res_txt, NUM_TXTS);

		/* convert resource */
		_cvt_txt_array(data->pt_txt, data->style_txt, &data->font, data->res_txt, NUM_TXTS);
	}
	SYS_LOG_INF("load resource succeed");

	return 0;
}

static void _unload_pic_resource(sys_view_data_t *data)
{
	lvgl_res_unload_pictures(data->img_dsc_def, NUM_BTNS);
	lvgl_res_unload_pictures(data->img_dsc_sel, NUM_BTNS);
	lvgl_res_unload_pictures(data->img_dsc_bat, NUM_BATS);
}

static void _unload_resource(sys_view_data_t *data)
{
	lvgl_res_unload_pictures(data->img_dsc_def, NUM_BTNS);
	lvgl_res_unload_pictures(data->img_dsc_sel, NUM_BTNS);
	lvgl_res_unload_pictures(data->img_dsc_bat, NUM_BATS);
	lvgl_res_unload_strings(data->res_txt, NUM_TXTS);

	LVGL_FONT_CLOSE(&data->font);
	lvgl_res_unload_scene(&data->res_scene);
}

static void _update_btn_status(sys_view_data_t *data)
{
#ifdef CONFIG_BT_MANAGER
#if CONFIG_BT_BR_ACTS
	data->btn_sta[BTN_BLUETOOTH] = (bt_manager_get_connected_dev_num() > 0);
#else
	data->btn_sta[BTN_BLUETOOTH] = 0;
#endif
#else
	data->btn_sta[BTN_BLUETOOTH] = 0;
#endif
	if (data->btn_sta[BTN_BLUETOOTH]) {
		lv_imagebutton_set_state(data->btn[BTN_BLUETOOTH], LV_IMAGEBUTTON_STATE_CHECKED_RELEASED);
	} else {
		lv_imagebutton_set_state(data->btn[BTN_BLUETOOTH], LV_IMAGEBUTTON_STATE_RELEASED);
	}
}

static void _load_btn_status(sys_view_data_t *data)
{
	int i;

#ifdef CONFIG_PROPERTY
	property_get(CFG_BTN_STA, data->btn_sta, sizeof(data->btn_sta));
#endif

	for (i = 0; i < NUM_BTNS; i++) {
		if (data->btn_sta[i]) {
			lv_imagebutton_set_state(data->btn[i], LV_IMAGEBUTTON_STATE_CHECKED_RELEASED);
		} else {
			lv_imagebutton_set_state(data->btn[i], LV_IMAGEBUTTON_STATE_RELEASED);
		}
	}
}

static void _store_btn_status(sys_view_data_t *data)
{
#ifdef CONFIG_PROPERTY
	property_set(CFG_BTN_STA, data->btn_sta, sizeof(data->btn_sta));
#endif
}

static void _init_wakelock_status(void)
{
#ifdef CONFIG_PROPERTY
	uint8_t btn_sta[NUM_BTNS] = {0};
	property_get(CFG_BTN_STA, btn_sta, sizeof(btn_sta));
#endif

#ifdef CONFIG_SYS_WAKELOCK
	if (btn_sta[BTN_BRIGHTNESS]) {
		sys_wake_lock(FULL_WAKE_LOCK);
	}
#endif
}

static int _main_view_preload(view_data_t *view_data, bool update)
{
	if (main_preload_inited == 0) {
		lvgl_res_preload_scene_compact_default_init(SCENE_SYSTEM_VIEW, NULL, 0,
			NULL, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
		main_preload_inited = 1;

		// check screen lock
		_init_wakelock_status();
	}

	return lvgl_res_preload_scene_compact_default(SCENE_SYSTEM_VIEW, MAIN_VIEW, update, 0);
}

static int _main_view_layout_update(view_data_t *view_data, bool first_layout)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	sys_view_data_t *data = view_data->user_data;

	if(first_layout)
	{
		data = app_mem_malloc(sizeof(*data));
		if (!data) {
			return -ENOMEM;
		}

		view_data->user_data = data;
		memset(data, 0, sizeof(*data));
		p_sysview_data = data;
	}

	if (_load_resource(data, first_layout)) {
		app_mem_free(data);
		view_data->user_data = NULL;
		return -ENOENT;
	}

	if(first_layout)
	{
		// init bg
		lv_style_init(&data->style_bg);
		lv_style_set_bg_opa(&data->style_bg, LV_OPA_COVER);
		lv_style_set_bg_color(&data->style_bg, data->res_scene.background);
		lv_obj_add_style(scr, &data->style_bg, LV_PART_MAIN);

		// create button
		_create_btn_array(scr, data->btn, data->pt_def, data->img_dsc_def, data->img_dsc_sel, NUM_BTNS);
		lv_obj_remove_flag(data->btn[BTN_BLUETOOTH], LV_OBJ_FLAG_CLICKABLE);
		lv_obj_remove_flag(data->btn[BTN_SETTING], LV_OBJ_FLAG_CHECKABLE);

		// create image
		_create_img_array(scr, data->img, data->pt_bat, data->img_dsc_bat, NUM_IMGS);

		// create label
		_create_lbl_array(scr, data->lbl, data->pt_txt, data->style_txt, NUM_TXTS);

	}
	return 0;
}

static int _main_view_layout(view_data_t *view_data)
{
	int ret;
	sys_view_data_t *data;
	ret = _main_view_layout_update(view_data, true);
	if(ret < 0)
	{
		return ret;
	}

	data = view_data->user_data;
	// status update
	_load_btn_status(data);
	_update_btn_status(data);

	// paint view
	_main_view_paint(view_data);
	lv_refr_now(view_data->display);

	return 0;
}


static int _main_view_paint(view_data_t *view_data)
{
	const main_view_presenter_t *presenter = view_get_presenter(view_data);
	sys_view_data_t *data = view_data->user_data;
#ifdef CONFIG_RTC_ACTS
	struct rtc_time time;
	char buf[16];

	if (data) {
		// update battery
		data->bat_val = presenter->get_battery_percent();
		sprintf(buf, "%d%%", data->bat_val);
		lv_label_set_text(data->lbl[TXT_BAT], buf);
		lv_image_set_src(data->img[IMG_BAT], &data->img_dsc_bat[data->bat_val / (100/4)]);

		// update date
		presenter->get_time(&time);
		sprintf(buf, "%04d-%02d-%02d", time.tm_year+1900, time.tm_mon+1, time.tm_mday);
		lv_label_set_text(data->lbl[TXT_DATE], buf);
		lv_label_set_text(data->lbl[TXT_WEEK], _txt_week[time.tm_wday]);
	}
#endif
	return 0;
}

static int _main_view_delete(view_data_t *view_data)
{
	sys_view_data_t *data = view_data->user_data;
	int i;

	if (data) {
		_store_btn_status(data);
		_delete_obj_array(data->btn, NUM_BTNS);
		_delete_obj_array(data->img, NUM_IMGS);
		_delete_obj_array(data->lbl, NUM_TXTS);
		lv_style_reset(&data->style_bg);
		for (i = 0; i < ARRAY_SIZE(data->style_txt); i++) {
			lv_style_reset(&data->style_txt[i]);
		}

		_unload_resource(data);


		app_mem_free(data);
		view_data->user_data = NULL;
		p_sysview_data = NULL;
	} else {
		lvgl_res_preload_cancel_scene(SCENE_SYSTEM_VIEW);
	}

	lvgl_res_unload_scene_compact(SCENE_SYSTEM_VIEW);
	return 0;
}

static int _main_view_updated(view_data_t* view_data)
{
	_main_view_layout_update(view_data, false);

	ui_view_paint(MAIN_VIEW);
	return 0;
}

static int _main_view_focus_changed(view_data_t *view_data, bool focused)
{
	sys_view_data_t *data = view_data->user_data;


	if (focused)
	{
    	if(!data)
    	{
    		return 0;
    	}

		if(!lvgl_res_scene_is_loaded(SCENE_SYSTEM_VIEW))
		{
			_main_view_preload(view_data, true);
		}
	}
	else
	{
	    if(data)
	    {
	        _unload_pic_resource(data);
	    }
		lvgl_res_preload_cancel_scene(SCENE_SYSTEM_VIEW);
		lvgl_res_unload_scene_compact(SCENE_SYSTEM_VIEW);

	}

	if(data)
	{
		_update_btn_status(data);
	}
//	ui_view_paint(MAIN_VIEW);
	return 0;
}

int _main_view_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void * msg_data)
{
	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return _main_view_preload(view_data, false);
	case MSG_VIEW_LAYOUT:
		return _main_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _main_view_delete(view_data);
	case MSG_VIEW_PAINT:
		return _main_view_paint(view_data);
	case MSG_VIEW_FOCUS:
		return _main_view_focus_changed(view_data, true);
	case MSG_VIEW_DEFOCUS:
		return _main_view_focus_changed(view_data, false);
	case MSG_VIEW_UPDATE:
		return _main_view_updated(view_data);
	default:
		return 0;
	}
}

VIEW_DEFINE2(main, _main_view_handler, NULL, \
		NULL, MAIN_VIEW, HIGH_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);
