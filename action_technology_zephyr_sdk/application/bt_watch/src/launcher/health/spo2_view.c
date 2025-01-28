/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <assert.h>
#include <lvgl.h>
#include <lvgl/lvgl_res_loader.h>
#include <ui_manager.h>
#include <view_manager.h>
#include "widgets/img_number.h"
#include "spo2_view.h"

//LOG_MODULE_DECLARE(clock);

typedef struct spo2_view_tmp_res {
	lvgl_res_scene_t scene;
	lvgl_res_group_t grp_spo2;
} spo2_view_tmp_res_t;

typedef struct spo2_view_data {
	lv_obj_t *obj_bg;
	lv_obj_t *obj_spo2;

	/* lvgl resource */
	lv_image_dsc_t img_bg;
	lv_image_dsc_t img_numbers[10];
} spo2_view_data_t;

static void _spo2_view_unload_resource(spo2_view_data_t *data);
static int _spo2_view_paint(view_data_t *view_data);

static const uint32_t pic_spo2_ids[] = {
	PIC_0, PIC_1, PIC_2, PIC_3, PIC_4, PIC_5, PIC_6, PIC_7, PIC_8, PIC_9,
};

static int32_t spo2_preload_inited = 0;

static int _spo2_view_load_resource(spo2_view_data_t *data, spo2_view_tmp_res_t *tmp_res, bool first_layout)
{
	uint32_t pic_id;
	int ret;

	/* scene */
	ret = lvgl_res_load_scene(SCENE_SPO2_VIEW, &tmp_res->scene,
			DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (ret < 0) {
		SYS_LOG_ERR("SCENE_SPO2_VIEW not found");
		return -ENOENT;
	}

	/* background picture */
	pic_id = PIC_BACKGROUND;
	ret = lvgl_res_load_pictures_from_scene(&tmp_res->scene, &pic_id, &data->img_bg, NULL, 1);
	if (ret < 0) {
		goto out_exit;
	}

	/* spo2 value */
	ret = lvgl_res_load_group_from_scene(&tmp_res->scene, RES_SPO2, &tmp_res->grp_spo2);
	if (ret < 0) {
		goto out_exit;
	}

	ret = lvgl_res_load_pictures_from_group(&tmp_res->grp_spo2, pic_spo2_ids,
			data->img_numbers, NULL, ARRAY_SIZE(pic_spo2_ids));

	lvgl_res_unload_group(&tmp_res->grp_spo2);
out_exit:
	lvgl_res_unload_scene(&tmp_res->scene);
	if (ret < 0) {
		_spo2_view_unload_resource(data);
	}

	return ret;
}

static void _spo2_view_unload_resource(spo2_view_data_t *data)
{
	lvgl_res_unload_pictures(&data->img_bg, 1);
	lvgl_res_unload_pictures(data->img_numbers, ARRAY_SIZE(data->img_numbers));
}

static int _spo2_view_preload(view_data_t *view_data, bool update)
{
	if (spo2_preload_inited == 0) {
		lvgl_res_preload_scene_compact_default_init(SCENE_SPO2_VIEW, NULL, 0,
			NULL, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
		spo2_preload_inited = 1;
	}
	
	return lvgl_res_preload_scene_compact_default(SCENE_SPO2_VIEW, HEALTH_SPO2_VIEW, update, 0);
}

static int _spo2_view_layout_update(view_data_t *view_data, bool first_layout)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	spo2_view_data_t *data = view_data->user_data;
	spo2_view_tmp_res_t resource;

	if(first_layout)
	{
		data = app_mem_malloc(sizeof(*data));
		if (data == NULL) {
			return -ENOMEM;
		}
		memset(data, 0, sizeof(*data));
		view_data->user_data = data;
	}

	if (_spo2_view_load_resource(data, &resource, first_layout)) {
		app_mem_free(data);
		return -ENOENT;
	}

	if(first_layout)
	{
		data->obj_bg = lv_image_create(scr);
		if (data->obj_bg == NULL) {
			goto fail_exit;
		}
	}

	lv_image_set_src(data->obj_bg, &data->img_bg);

	if(first_layout)
	{
		data->obj_spo2 = img_number_create(data->obj_bg);
		if (data->obj_spo2 == NULL) {
			goto fail_exit;
		}
	}

	lv_obj_set_pos(data->obj_spo2, resource.grp_spo2.x, resource.grp_spo2.y);
	lv_obj_set_size(data->obj_spo2, resource.grp_spo2.width, resource.grp_spo2.height);
	img_number_set_src(data->obj_spo2, data->img_numbers, 10);	
	img_number_set_align(data->obj_spo2, LV_ALIGN_RIGHT_MID);


	return 0;
fail_exit:
	_spo2_view_unload_resource(data);
	app_mem_free(data);
	return -ENOMEM;
}

static int _spo2_view_layout(view_data_t *view_data)
{
	int ret;
	
	ret = _spo2_view_layout_update(view_data, true);
	if(ret < 0)
	{
		return -1;
	}
	
	// paint view
	_spo2_view_paint(view_data);
//	lv_refr_now(view_data->display);
	SYS_LOG_INF("spo2 view inflated");

	return 0;
	
}

static int _spo2_view_delete(view_data_t *view_data)
{
	spo2_view_data_t *data = view_data->user_data;

	if (data) {
		_spo2_view_unload_resource(data);
		app_mem_free(data);
	} else {
		lvgl_res_preload_cancel_scene(SCENE_SPO2_VIEW);
	}

	lvgl_res_unload_scene_compact(SCENE_SPO2_VIEW);
	return 0;
}

static int _spo2_view_paint(view_data_t *view_data)
{
	const spo2_view_presenter_t *presenter = view_get_presenter(view_data);
	spo2_view_data_t *data = view_data->user_data;
	uint8_t spo2;

	if (data == NULL) {
		return 0;
	}

	spo2 = presenter->get_spo2();
	img_number_set_value(data->obj_spo2, spo2, 0);

	return 0;
}

static int _spo2_view_updated(view_data_t* view_data)
{
	int ret;

	ret = _spo2_view_layout_update(view_data, false);
	
	return ret;
	
}

static int _spo2_view_focus_changed(view_data_t *view_data, bool focused)
{
	spo2_view_data_t *data = view_data->user_data;


	if (focused) 
	{
		if(!lvgl_res_scene_is_loaded(SCENE_SPO2_VIEW))
		{
			_spo2_view_preload(view_data, true);
		}
	}
	else
	{
	    if(data)
	    {
	        _spo2_view_unload_resource(data);
	    }
		lvgl_res_preload_cancel_scene(SCENE_SPO2_VIEW);
		lvgl_res_unload_scene_compact(SCENE_SPO2_VIEW);
		
	}

	return 0;
}


int _spo2_view_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void *msg_data)
{
	assert(view_id == HEALTH_SPO2_VIEW);

	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return _spo2_view_preload(view_data, false);
	case MSG_VIEW_LAYOUT:
		return _spo2_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _spo2_view_delete(view_data);
	case MSG_VIEW_PAINT:
		return _spo2_view_paint(view_data);
	case MSG_VIEW_FOCUS:
		return _spo2_view_focus_changed(view_data, true);
	case MSG_VIEW_DEFOCUS:
		return _spo2_view_focus_changed(view_data, false);
	case MSG_VIEW_UPDATE:
		return _spo2_view_updated(view_data);
		
	default:
		return 0;
	}
}

VIEW_DEFINE2(health_spo2, _spo2_view_handler, NULL,
		NULL, HEALTH_SPO2_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);
