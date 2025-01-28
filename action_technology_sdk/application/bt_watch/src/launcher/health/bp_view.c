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
#include "widgets/anim_img.h"
#include "widgets/img_number.h"
#include "bp_view.h"

//LOG_MODULE_DECLARE(clock);

typedef struct bp_view_tmp_res {
	lvgl_res_group_t grp_bp_low;
	lvgl_res_group_t grp_bp_high;
} bp_view_tmp_res_t;

typedef struct bp_view_data {
	lv_obj_t *obj_bg;
	lv_obj_t *obj_anim;
	lv_obj_t *obj_bp_low;
	lv_obj_t *obj_bp_high;

	/* lvgl resource */
	lv_image_dsc_t img_bg;
	lv_image_dsc_t img_numbers[10];

	lvgl_res_picregion_t prg_anim;
	lvgl_res_scene_t scene;
} bp_view_data_t;

static void _bp_view_unload_resource(bp_view_data_t *data);
static int _bp_view_paint(view_data_t *view_data);

static const uint32_t preload_ids[] = {
	PIC_BACKGROUND, RES_BP_LOW, RES_BP_HIGH,
};

static const uint32_t pic_bp_ids[] = {
	PIC_0, PIC_1, PIC_2, PIC_3, PIC_4, PIC_5, PIC_6, PIC_7, PIC_8, PIC_9,
};

static int32_t bp_preload_inited = 0;

static int _bp_view_load_resource(bp_view_data_t *data, bp_view_tmp_res_t *tmp_res, bool first_layout)
{
	uint32_t pic_id;
	int ret;

	/* scene */
	if(first_layout)
	{
		ret = lvgl_res_load_scene(SCENE_BP_VIEW, &data->scene,
				DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
		if (ret < 0) {
			SYS_LOG_ERR("SCENE_BP_VIEW not found");
			return -ENOENT;
		}
	}

	/* background picture */
	pic_id = PIC_BACKGROUND;
	ret = lvgl_res_load_pictures_from_scene(&data->scene, &pic_id, &data->img_bg, NULL, 1);
	if (ret < 0) {
		goto out_exit;
	}

	/* animation */
	ret = lvgl_res_load_picregion_from_scene(&data->scene, RES_ANIMATION, &data->prg_anim);
	if (ret < 0) {
		goto out_exit;
	}

	/* bp low */
	ret = lvgl_res_load_group_from_scene(&data->scene, RES_BP_LOW, &tmp_res->grp_bp_low);
	if (ret < 0) {
		goto out_exit;
	}

	lvgl_res_unload_group(&tmp_res->grp_bp_low);

	/* bp high */
	ret = lvgl_res_load_group_from_scene(&data->scene, RES_BP_HIGH, &tmp_res->grp_bp_high);
	if (ret < 0) {
		goto out_exit;
	}

	ret = lvgl_res_load_pictures_from_group(&tmp_res->grp_bp_high, pic_bp_ids,
			data->img_numbers, NULL, ARRAY_SIZE(pic_bp_ids));

	lvgl_res_unload_group(&tmp_res->grp_bp_high);

out_exit:
	if (ret < 0) {
		_bp_view_unload_resource(data);
	}

	return ret;
}

static void _bp_view_unload_pic_resource(bp_view_data_t *data)
{
	lvgl_res_unload_pictures(&data->img_bg, 1);
	lvgl_res_unload_pictures(data->img_numbers, ARRAY_SIZE(data->img_numbers));
	lvgl_res_unload_picregion(&data->prg_anim);
}

static void _bp_view_unload_resource(bp_view_data_t *data)
{
	lvgl_res_unload_pictures(&data->img_bg, 1);
	lvgl_res_unload_pictures(data->img_numbers, ARRAY_SIZE(data->img_numbers));
	lvgl_res_unload_picregion(&data->prg_anim);
	lvgl_res_unload_scene(&data->scene);
}

static int _bp_view_preload(view_data_t *view_data, bool update)
{
	if (bp_preload_inited == 0) {
		lvgl_res_preload_scene_compact_default_init(SCENE_BP_VIEW, preload_ids, ARRAY_SIZE(preload_ids),
			NULL, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
		bp_preload_inited = 1;
	}
	
	return lvgl_res_preload_scene_compact_default(SCENE_BP_VIEW, HEALTH_BP_VIEW, update, 0);
}

static int _bp_view_layout_update(view_data_t *view_data, bool first_layout)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	bp_view_data_t *data = view_data->user_data;
	bp_view_tmp_res_t resource;

	if(first_layout)
	{
		data = app_mem_malloc(sizeof(*data));
		if (data == NULL) {
			return -ENOMEM;
		}

		memset(data, 0, sizeof(*data));
		view_data->user_data = data;
	}

	if (_bp_view_load_resource(data, &resource, first_layout)) {
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
		data->obj_anim = anim_img_create(data->obj_bg);
		data->obj_bp_low = img_number_create(data->obj_bg);
		data->obj_bp_high = img_number_create(data->obj_bg);
		if (!data->obj_anim || !data->obj_bp_low || !data->obj_bp_high) {
			goto fail_exit;
		}
	}

	lv_obj_set_pos(data->obj_anim, data->prg_anim.x, data->prg_anim.y);
	lv_obj_set_size(data->obj_anim, data->prg_anim.width, data->prg_anim.height);
	anim_img_set_src_picregion(data->obj_anim, SCENE_BP_VIEW, &data->prg_anim);
	if(first_layout)
	{
		anim_img_set_duration(data->obj_anim, 16, 128 * data->prg_anim.frames);
		anim_img_set_repeat(data->obj_anim, 0, LV_ANIM_REPEAT_INFINITE);
	}

	lv_obj_set_pos(data->obj_bp_low, resource.grp_bp_low.x, resource.grp_bp_low.y);
	lv_obj_set_size(data->obj_bp_low, resource.grp_bp_low.width, resource.grp_bp_low.height);
	img_number_set_src(data->obj_bp_low, data->img_numbers, 10);
	img_number_set_align(data->obj_bp_low, LV_ALIGN_LEFT_MID);

	lv_obj_set_pos(data->obj_bp_high, resource.grp_bp_high.x, resource.grp_bp_high.y);
	lv_obj_set_size(data->obj_bp_high, resource.grp_bp_high.width, resource.grp_bp_high.height);
	img_number_set_src(data->obj_bp_high, data->img_numbers, 10);
	img_number_set_align(data->obj_bp_high, LV_ALIGN_LEFT_MID);


	return 0;
fail_exit:
	_bp_view_unload_resource(data);
	app_mem_free(data);
	return -ENOMEM;
}

static int _bp_view_layout(view_data_t *view_data)
{
	bp_view_data_t *data;
	int ret;
	ret = _bp_view_layout_update(view_data, true);
	if(ret < 0)
	{
		return ret;
	}

	data = view_data->user_data;
	/* initialize values */
	_bp_view_paint(view_data);

	if (data->obj_anim && view_is_focused(HEALTH_BP_VIEW)) {
		anim_img_start(data->obj_anim, false);
	}

//	lv_refr_now(view_data->display);
	SYS_LOG_INF("bp view inflated");

	return 0;
}

static int _bp_view_delete(view_data_t *view_data)
{
	bp_view_data_t *data = view_data->user_data;

	if (data) {
		_bp_view_unload_resource(data);
		app_mem_free(data);
	} else {
		lvgl_res_preload_cancel_scene(SCENE_BP_VIEW);
	}

	lvgl_res_unload_scene_compact(SCENE_BP_VIEW);
	return 0;
}

static int _bp_view_paint(view_data_t *view_data)
{
	const bp_view_presenter_t *presenter = view_get_presenter(view_data);
	bp_view_data_t *data = view_data->user_data;
	uint8_t low_bp = 75, high_bp = 123;

	if (data == NULL) {
		return 0;
	}

	presenter->get_bp_range(&low_bp, &high_bp);

	img_number_set_value(data->obj_bp_low, low_bp, 0);
	img_number_set_value(data->obj_bp_high, high_bp, 0);

	return 0;
}

static int _bp_view_updated(view_data_t *view_data)
{
	bp_view_data_t *data = view_data->user_data;

	_bp_view_layout_update(view_data, false);
	_bp_view_paint(view_data);

	anim_img_start(data->obj_anim, true);

	return 0;
}

static int _bp_view_focus_changed(view_data_t *view_data, bool focused)
{
	bp_view_data_t *data = view_data->user_data;

	if (focused) {
		if(!lvgl_res_scene_is_loaded(SCENE_BP_VIEW))
		{
			_bp_view_preload(view_data, true);
		}
		else
		{
		    if(data)
		    {
			    anim_img_start(data->obj_anim, true);
			}
		}

	} else {
        if(data)
        {
    		if(data->obj_anim)
    		{
    			anim_img_stop(data->obj_anim);
    			anim_img_clean(data->obj_anim);
    		}
    		_bp_view_unload_pic_resource(data);
    	}
		lvgl_res_preload_cancel_scene(SCENE_BP_VIEW);
		lvgl_res_unload_scene_compact(SCENE_BP_VIEW);

	}

	return 0;

}

int _bp_view_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void *msg_data)
{
	assert(view_id == HEALTH_BP_VIEW);

	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return _bp_view_preload(view_data, false);
	case MSG_VIEW_LAYOUT:
		return _bp_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _bp_view_delete(view_data);
	case MSG_VIEW_PAINT:
		return _bp_view_paint(view_data);
	case MSG_VIEW_FOCUS:
		return _bp_view_focus_changed(view_data, true);
	case MSG_VIEW_DEFOCUS:
		return _bp_view_focus_changed(view_data, false);
	case MSG_VIEW_UPDATE:
		return _bp_view_updated(view_data);
	default:
		return 0;
	}
}

VIEW_DEFINE2(health_bp, _bp_view_handler, NULL,
		NULL, HEALTH_BP_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);
