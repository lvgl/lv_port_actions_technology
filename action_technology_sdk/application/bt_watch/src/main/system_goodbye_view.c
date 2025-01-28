/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <lvgl.h>
#include <lvgl/lvgl_res_loader.h>
#include <ui_manager.h>
#include <app_ui.h>

#define CONFIG_GOODBYE_ANIM_TIME    2000

typedef struct goodbye_view_data {
	lv_image_dsc_t img_src;
	lv_point_t img_pos;
	lvgl_res_scene_t scene;
} goodbye_view_data_t;

static int _goodbye_view_load_resource(goodbye_view_data_t *data)
{
	uint32_t pic_id;
	int ret;

	/* scene */
	ret = lvgl_res_load_scene(SCENE_LOGO, &data->scene,
			WELCOME_STY_FILE, WELCOME_RES_FILE, NULL);
	if (ret < 0) {
		SYS_LOG_ERR("SCENE_LOGO 0x%x not found", SCENE_LOGO);
		return -ENOENT;
	}

	pic_id = PIC_BYE;

	ret = lvgl_res_load_pictures_from_scene(&data->scene,
			&pic_id, &data->img_src, &data->img_pos, 1);

	lvgl_res_unload_scene(&data->scene);

	return ret;
}

static void _goodbye_view_unload_resource(goodbye_view_data_t *data)
{
	lvgl_res_unload_pictures(&data->img_src, 1);
}

static void _anim_set_img_opa(void *var, int32_t value)
{
	lv_obj_set_style_img_opa(var, 5 + value, 0);
}

static void _anim_ready_cb(lv_anim_t *anim)
{
	ui_view_delete(GOODBYE_VIEW);
}

static int _goodbye_view_layout(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	goodbye_view_data_t *data;

	data = app_mem_malloc(sizeof(*data));
	if (!data) {
		return -ENOMEM;
	}

	if (_goodbye_view_load_resource(data)) {
		SYS_LOG_ERR("res load failed");
		app_mem_free(data);
		return -ENOENT;
	}

	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(scr, data->scene.background, 0);

	lv_obj_t *img = lv_image_create(scr);
	lv_obj_set_pos(img, data->img_pos.x, data->img_pos.y);
	lv_image_set_src(img, &data->img_src);

	lv_anim_t anim;
	lv_anim_init(&anim);
	lv_anim_set_var(&anim, img);
	lv_anim_set_values(&anim, 250, 0);
	lv_anim_set_duration(&anim, CONFIG_GOODBYE_ANIM_TIME);
	lv_anim_set_exec_cb(&anim, _anim_set_img_opa);
	lv_anim_set_ready_cb(&anim, _anim_ready_cb);

	lv_anim_start(&anim);

	lv_refr_now(view_data->display);

	view_data->user_data = data;
	return 0;
}

static int _goodbye_view_delete(view_data_t *view_data)
{
	goodbye_view_data_t *data = view_data->user_data;

	if (data) {
		lv_anim_delete(NULL, _anim_set_img_opa);
		_goodbye_view_unload_resource(data);
		app_mem_free(data);
	}

	return 0;
}

int _goodbye_view_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void *msg_data)
{
	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return ui_view_layout(view_id);
	case MSG_VIEW_LAYOUT:
		return _goodbye_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _goodbye_view_delete(view_data);
	default:
		return 0;
	}
}

VIEW_DEFINE2(goodbye_view, _goodbye_view_handler, NULL, NULL, GOODBYE_VIEW,
		HIGHEST_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);
