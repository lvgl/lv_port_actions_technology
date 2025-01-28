/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief card reader app view
*/
#include <os_common_api.h>
#include "charger.h"
#include "hotplug_manager.h"
#include <input_manager.h>
#include <ui_manager.h>
#include <app_ui.h>

enum {
	//BMP_CHARG_BG,
	BMP_CHARG_ICON,
	NUM_CHARGER_IMGS,
};

static const uint32_t charger_bg_bmp_ids[] = {
	//PIC_CHARGER_BG,
	PIC_CHARG_ICON,
};

typedef struct charger_view_data {
	/* resource */
	lvgl_res_scene_t res_scene;
	lv_image_dsc_t img_dsc_charger_bg[NUM_CHARGER_IMGS];
	lv_point_t pt_charger_bg[NUM_CHARGER_IMGS];
	/* lvgl object */
	lv_obj_t *cont;
	lv_obj_t *img[NUM_CHARGER_IMGS];
} charger_view_data_t;

static charger_view_data_t *p_charger_view_data = NULL;


void charger_view_init(void)
{
#ifdef CONFIG_UI_MANAGER
	ui_view_create(CHARGER_VIEW, NULL, UI_CREATE_FLAG_SHOW);
#endif
	SYS_LOG_INF("charger_view_init ok\n");
}

void charger_view_deinit(void)
{
#ifdef CONFIG_UI_MANAGER
	ui_view_delete(CHARGER_VIEW);
#endif
	SYS_LOG_INF("ok\n");
}

static int _charger_load_resource(charger_view_data_t *data)
{
	int ret;
	/* scene */
	ret = lvgl_res_load_scene(SCENE_CHARGER_VIEW, &data->res_scene,
			DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (ret < 0) {
		SYS_LOG_ERR("SCENE_CHARGER_VIEW not found");
		return -ENOENT;
	}

	/* background picture */
	ret = lvgl_res_load_pictures_from_scene(&data->res_scene, charger_bg_bmp_ids, data->img_dsc_charger_bg, data->pt_charger_bg, NUM_CHARGER_IMGS);
	if (ret < 0) {
		SYS_LOG_ERR("charger scene load pictures failed");
		return ret;
	}

	return 0;
}

static void _charger_unload_resource(charger_view_data_t *data)
{
	lvgl_res_unload_pictures(data->img_dsc_charger_bg, NUM_CHARGER_IMGS);
	lvgl_res_unload_scene(&data->res_scene);
}

static void _charger_create_img_array(lv_obj_t *par, lv_obj_t **pobj, lv_point_t *pt,
										lv_image_dsc_t *img, uint32_t num)
{
	for (int i = 0; i < num; i++) {
		pobj[i] = lv_image_create(par);
		lv_image_set_src(pobj[i], &img[i]);
		lv_obj_set_pos(pobj[i], pt[i].x, pt[i].y);
	}
}

static int _charger_view_preload(view_data_t *view_data)
{
	return lvgl_res_preload_scene_compact(SCENE_CHARGER_VIEW, NULL, 0,
				lvgl_res_scene_preload_default_cb_for_view, (void *)CHARGER_VIEW,
				DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
}

static int _charger_view_layout(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);

	charger_view_data_t * data;
	data = app_mem_malloc(sizeof(*data));
	if (!data)
		return -ENOMEM;

	view_data->user_data = data;
	memset(data, 0, sizeof(*data));

	if (_charger_load_resource(data)) {
		SYS_LOG_ERR("res load failed");
		app_mem_free(data);
		return -ENOMEM;
	}

	p_charger_view_data = data;

	/* background */
	data->cont = lv_obj_create(scr);
    lv_obj_set_size(data->cont, LV_PCT(100), LV_PCT(100));
	lv_obj_set_style_bg_color(data->cont, lv_color_make(0x3b, 0x3b, 0x3b), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(data->cont, LV_OPA_COVER, LV_PART_MAIN);
	/* charger img */
	_charger_create_img_array(data->cont, data->img, data->pt_charger_bg, data->img_dsc_charger_bg, NUM_CHARGER_IMGS);
	SYS_LOG_INF("create\n");
	return 0;
}

static int _charger_view_paint(view_data_t *view_data)
{
	return 0;
}

static int _charger_view_delete(view_data_t *view_data)
{
	charger_view_data_t *data = view_data->user_data;

	if (data) {
		lv_obj_delete(data->cont);
		_charger_unload_resource(data);
		app_mem_free(data);
		view_data->user_data = NULL;
	}
	else {
		lvgl_res_preload_cancel_scene(SCENE_CHARGER_VIEW);
	}
	p_charger_view_data = NULL;
	lvgl_res_unload_scene_compact(SCENE_CHARGER_VIEW);
	lvgl_res_cache_clear(0);
	
	return 0;
}

int _charger_view_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void *msg_data)
{
	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return _charger_view_preload(view_data);
	case MSG_VIEW_LAYOUT:
		return _charger_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _charger_view_delete(view_data);
	case MSG_VIEW_PAINT:
		return _charger_view_paint(view_data);
	default:
		return 0;
	}
}

VIEW_DEFINE2(charger_view, _charger_view_handler, NULL, \
		NULL, CHARGER_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);


