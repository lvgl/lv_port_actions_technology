/*
 * Copyright (c) 2022 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file gps view
 */

#include <assert.h>
#include <view_stack.h>
#ifdef CONFIG_SYS_WAKELOCK
#include <sys_wakelock.h>
#endif
#include "app_ui.h"
#include "app_defines.h"
#include "system_app.h"
#include "gps_view.h"


enum {
	TXT_TITLE = 0,
	TXT_LATITUDE,
	TXT_TATITUDE,
	TXT_ALTITUDE,
	TXT_UTC,
	NUM_TXTS,
};

const static uint32_t _txt_ids[] = {
	STR_GPS_TITLE,
	STR_LATITUDE,
	STR_TATITUDE,
	STR_ALTITUDE,
	STR_UTC,
};


static int32_t gps_preload_inited = 0;

typedef struct gps_view_data {
	/* lvgl object */
	lv_obj_t *lbl[NUM_TXTS];

	/* ui-editor resource */
	lvgl_res_scene_t res_scene;
	lv_point_t pt_txt[NUM_TXTS];
	lvgl_res_string_t res_txt[NUM_TXTS];
	
	/* lvgl resource */
	lv_style_t style_txt[NUM_TXTS];
	lv_font_t font;
	char gps_buf[50];
} gps_view_data_t;

static void _cvt_txt_array(lv_point_t *pt, lv_style_t *sty, lv_font_t *font, lvgl_res_string_t *txt, uint32_t num)
{
	int i;

	for (i = 0; i < num; i++) {
		pt[i].x = txt[i].x;
		pt[i].y = txt[i].y;

		lv_style_init(&sty[i]);
		lv_style_set_text_font(&sty[i], font);
		lv_style_set_text_color(&sty[i], txt[i].color);
	}
}

static int _load_resource(gps_view_data_t *data, bool first_layout)
{
	int32_t ret;

	if(first_layout)
	{
		/* load scene */
		ret = lvgl_res_load_scene(SCENE_GPS_VIEW, &data->res_scene, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
		if (ret < 0) {
			SYS_LOG_ERR("SCENE_GPS_VIEW not found");
			return -ENOENT;
		}
	}

	if(first_layout)
	{
		/* open font */
		if (LVGL_FONT_OPEN_DEFAULT(&data->font, DEF_FONT_SIZE_NORMAL) < 0) {
			SYS_LOG_ERR("font not found");
			return -ENOENT;
		}

		/* load string */
		lvgl_res_load_strings_from_scene(&data->res_scene, _txt_ids, data->res_txt, NUM_TXTS);
		_cvt_txt_array(data->pt_txt, data->style_txt, &data->font, data->res_txt, NUM_TXTS);
	}

	SYS_LOG_INF("load resource succeed");

	return 0;
}

static void _unload_pic_resource(gps_view_data_t *data)
{
	lvgl_res_unload_strings(data->res_txt, NUM_TXTS);
}

static void _unload_resource(gps_view_data_t *data)
{
	lvgl_res_unload_strings(data->res_txt, NUM_TXTS);
	lvgl_res_unload_scene(&data->res_scene);

	LVGL_FONT_CLOSE(&data->font);
}

static int _gps_view_preload(view_data_t *view_data)
{
	if (gps_preload_inited == 0) {
		lvgl_res_preload_scene_compact_default_init(SCENE_GPS_VIEW, NULL, 0,
			NULL, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
		gps_preload_inited = 1;
	}
	
	return lvgl_res_preload_scene_compact_default(SCENE_GPS_VIEW, GPS_VIEW, 0, 0);
}


static int _gps_view_layout_update(view_data_t *view_data, bool first_layout)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	gps_view_data_t *data = view_data->user_data;
	int i;

	if(first_layout)
	{
		data = app_mem_malloc(sizeof(*data));
		if (!data) {
			return -ENOMEM;
		}
		memset(data, 0, sizeof(*data));

		view_data->user_data = data;
	}

	if (_load_resource(data, first_layout)) {
		app_mem_free(data);
		view_data->user_data = NULL;
		return -ENOENT;
	}
	

	// create label
	if(first_layout)
	{
		for(i=0; i<NUM_TXTS; i++)
		{
			data->lbl[i] = lv_label_create(scr);
			lv_obj_set_pos(data->lbl[i], data->pt_txt[i].x, data->pt_txt[i].y);
			lv_obj_add_style(data->lbl[i], &data->style_txt[i], LV_PART_MAIN);
		}
	}

	for(i=0; i<NUM_TXTS; i++)
		lv_label_set_text(data->lbl[i], data->res_txt[i].txt);
	
	return 0;
}

static int _gps_view_layout(view_data_t *view_data)
{
	int ret;

	ret = _gps_view_layout_update(view_data, 1);
	if(ret < 0)
	{
		return ret;
	}

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_lock(FULL_WAKE_LOCK);
#endif

	lv_refr_now(view_data->display);
	SYS_LOG_INF("_gps_view_layout");

	return 0;
}

static int _gps_view_paint(view_data_t *view_data)
{
	gps_view_data_t *data = view_data->user_data;
#ifdef CONFIG_GPS_MANAGER
	const gps_view_presenter_t *presenter = view_get_presenter(view_data);
	gps_res_t* gps_data = presenter->get_gps_data();
#endif
	if (data) {
		SYS_LOG_INF("_gps_view_paint");
#ifdef CONFIG_GPS_MANAGER
#ifdef CONFIG_GPS_PARSE_GGA_ENABLE
		snprintf(data->gps_buf, sizeof(data->gps_buf), "纬度: %f", 
			minmea_tofloat(&gps_data->gga_data.latitude));
		lv_label_set_text(data->lbl[TXT_LATITUDE], data->gps_buf);

		snprintf(data->gps_buf, sizeof(data->gps_buf), "经度: %f", 
			minmea_tofloat(&gps_data->gga_data.longitude));
		lv_label_set_text(data->lbl[TXT_TATITUDE], data->gps_buf);

		snprintf(data->gps_buf, sizeof(data->gps_buf), "海拔高度: %f", 
			minmea_tofloat(&gps_data->gga_data.altitude));
		lv_label_set_text(data->lbl[TXT_ALTITUDE], data->gps_buf);

		snprintf(data->gps_buf, sizeof(data->gps_buf), "GPS时间: %02d:%02d:%02d", 
			gps_data->gga_data.time.hours,gps_data->gga_data.time.minutes,
			gps_data->gga_data.time.seconds);
		lv_label_set_text(data->lbl[TXT_UTC], data->gps_buf);
#endif
	ARG_UNUSED(gps_data);
#endif
	}

	return 0;
}

static int _gps_view_delete(view_data_t *view_data)
{
	gps_view_data_t *data = view_data->user_data;
	int i;

	if (data) {
		for(i=0;i<NUM_TXTS;i++)
		{
			lv_obj_delete(data->lbl[i]);
			lv_style_reset(&data->style_txt[i]);
		}

		_unload_resource(data);
		app_mem_free(data);
		view_data->user_data = NULL;

#ifdef CONFIG_SYS_WAKELOCK
		sys_wake_unlock(FULL_WAKE_LOCK);
#endif
	} else {
		lvgl_res_preload_cancel();
	}

	lvgl_res_unload_scene_compact(SCENE_GPS_VIEW);
	return 0;
}

static int _gps_view_updated(view_data_t* view_data)
{
	int ret;

	ret = _gps_view_layout_update(view_data, 0);	
	return ret;
	
}

static int _gps_view_focus_changed(view_data_t *view_data, bool focused)
{
	gps_view_data_t *data = view_data->user_data;

	if (focused) 
	{
		if(!lvgl_res_scene_is_loaded(SCENE_GPS_VIEW))
		{
			lvgl_res_preload_scene_compact_default(SCENE_GPS_VIEW, GPS_VIEW, 1, 0);
		}
#ifdef CONFIG_GPS_MANAGER
		gps_manager_enable(0,0);
#endif 
	}
	else
	{
		if(data)
		{
			_unload_pic_resource(data);
		}
		lvgl_res_preload_cancel_scene(SCENE_GPS_VIEW);
		lvgl_res_unload_scene_compact(SCENE_GPS_VIEW);
#ifdef CONFIG_GPS_MANAGER
		gps_manager_disable(0, 0);
#endif
	}

	return 0;
}

int _gps_view_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void * msg_data)
{
	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return _gps_view_preload(view_data);
	case MSG_VIEW_LAYOUT:
		return _gps_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _gps_view_delete(view_data);
	case MSG_VIEW_FOCUS:
		return _gps_view_focus_changed(view_data, 1);
	case MSG_VIEW_DEFOCUS:
		return _gps_view_focus_changed(view_data, 0);
	case MSG_VIEW_UPDATE:
		return _gps_view_updated(view_data);		
	case MSG_VIEW_PAINT:
		return _gps_view_paint(view_data);
	default:
		return 0;
	}
	return 0;
}

VIEW_DEFINE2(gps_view, _gps_view_handler, NULL, \
		NULL, GPS_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_WIDTH, DEF_UI_HEIGHT);

