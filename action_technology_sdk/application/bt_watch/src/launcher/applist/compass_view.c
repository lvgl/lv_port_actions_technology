/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <assert.h>
#include <lvgl.h>
#include <lvgl/lvgl_res_loader.h>
#include <view_manager.h>
#include <res_manager_api.h>
#ifdef CONFIG_SYS_WAKELOCK
#include <sys_wakelock.h>
#endif
#include "widgets/compass.h"
#include "compass_view.h"


#ifdef CONFIG_SENSOR_MANAGER
#include <sensor_manager.h>
#endif

typedef struct compass_view_tmp_res {
	lvgl_res_scene_t scene;
	lvgl_res_group_t grp_hole;
} compass_view_tmp_res_t;

typedef struct compass_view_data {
	lv_obj_t *compass;

	/* lvgl resource */
	lv_image_dsc_t img_main;
#ifdef PIC_MAIN_ROT180
	lv_image_dsc_t img_main_rot180;
#endif
	lv_image_dsc_t img_decor;
	lv_image_dsc_t img_char[NUM_COMPASS_CHARS];
} compass_view_data_t;

static const uint32_t pic_char_ids[NUM_COMPASS_CHARS] = {
	PIC_0, PIC_1, PIC_2, PIC_3, PIC_4, PIC_5, PIC_6, PIC_7, PIC_8, PIC_9,
	PIC_DEGREE, PIC_EAST, PIC_SOUTH, PIC_WEST, PIC_NORTH,
};

static void _compass_view_unload_resource(compass_view_data_t *data);

static int _load_pictures(lvgl_res_scene_t *scene, uint32_t group_id,
		const uint32_t *pic_ids, int num_ids, lv_image_dsc_t *images)
{
	lvgl_res_group_t group;
	int res = -ENOENT;

	if (lvgl_res_load_group_from_scene(scene, group_id, &group)) {
		return -ENOENT;
	}

	if (lvgl_res_load_pictures_from_group(&group, pic_ids, images, NULL, num_ids)) {
		goto out_unload_group;
	}

	res = 0;
out_unload_group:
	lvgl_res_unload_group(&group);
	return res;
}

static int _compass_view_load_resource(compass_view_data_t *data, compass_view_tmp_res_t *res)
{
	uint32_t pic_id;
	int ret;

	/* scene */
	ret = lvgl_res_load_scene(SCENE_COMPASS_VIEW, &res->scene, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (ret < 0) {
		SYS_LOG_ERR("SCENE_COMPASS_VIEW not found");
		return -ENOENT;
	}

	/* background picture */
	pic_id = PIC_MAIN;
	ret = lvgl_res_load_pictures_from_scene(&res->scene, &pic_id, &data->img_main, NULL, 1);
	if (ret < 0) {
		goto fail_exit;
	}

#ifdef PIC_MAIN_ROT180
	pic_id = PIC_MAIN_ROT180;
	ret = lvgl_res_load_pictures_from_scene(&res->scene, &pic_id, &data->img_main_rot180, NULL, 1);
	if (ret < 0) {
		goto fail_exit;
	}
#endif

	pic_id = PIC_DECOR;
	ret = lvgl_res_load_pictures_from_scene(&res->scene, &pic_id, &data->img_decor, NULL, 1);
	if (ret < 0) {
		goto fail_exit;
	}

	ret = _load_pictures(&res->scene, RES_CHAR, pic_char_ids, ARRAY_SIZE(pic_char_ids), data->img_char);
	if (ret)
		goto fail_exit;

	ret = lvgl_res_load_group_from_scene(&res->scene, RES_HOLE, &res->grp_hole);
	if (ret)
		goto fail_exit;

	lvgl_res_unload_scene(&res->scene);
	lvgl_res_unload_group(&res->grp_hole);
	return 0;
fail_exit:
	lvgl_res_unload_scene(&res->scene);
	_compass_view_unload_resource(data);
	return -ENOENT;
}

static void _compass_view_unload_resource(compass_view_data_t *data)
{
	lvgl_res_unload_pictures(&data->img_main, 1);
#ifdef PIC_MAIN_ROT180
	lvgl_res_unload_pictures(&data->img_main_rot180, 1);
#endif
	lvgl_res_unload_pictures(&data->img_decor, 1);
	lvgl_res_unload_pictures(data->img_char, ARRAY_SIZE(data->img_char));
}

static int _compass_view_preload(view_data_t *view_data)
{
	compass_view_data_t *data = app_mem_malloc(sizeof(*data));
	if (data == NULL) {
		return -ENOMEM;
	}

	int res = lvgl_res_preload_scene_compact(SCENE_COMPASS_VIEW, NULL, 0,
			lvgl_res_scene_preload_default_cb_for_view, (void *)COMPASS_VIEW,
			DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (res) {
		app_mem_free(data);
		return res;
	}

	memset(data, 0, sizeof(*data));
	view_data->user_data = data;
	return 0;
}

static int _compass_view_layout(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	compass_view_data_t *data = view_data->user_data;
	compass_view_tmp_res_t tmp_res;

	if (_compass_view_load_resource(data, &tmp_res)) {
		goto fail_exit;
	}

	data->compass = compass_create(scr);
	if (!data->compass) {
		goto fail_exit;
	}

	lv_obj_set_style_bg_color(scr, tmp_res.scene.background, LV_PART_MAIN);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_bg_color(data->compass, tmp_res.scene.background, LV_PART_MAIN);
	lv_obj_set_style_bg_opa(data->compass, LV_OPA_COVER, LV_PART_MAIN);

	compass_set_decor_image(data->compass, &data->img_decor);
	compass_set_char_image(data->compass, data->img_char, ARRAY_SIZE(data->img_char));
#ifdef PIC_MAIN_ROT180
	compass_set_main_image(data->compass, &data->img_main, &data->img_main_rot180,
			tmp_res.grp_hole.width > 1 ? tmp_res.grp_hole.width - 1 : 0);
#else
	compass_set_main_image(data->compass, &data->img_main, NULL,
			tmp_res.grp_hole.width > 1 ? tmp_res.grp_hole.width - 1 : 0);
#endif

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_lock(FULL_WAKE_LOCK);
#endif

	SYS_LOG_INF("compass view created");
	return 0;
fail_exit:
	SYS_LOG_INF("compass view failed");
	_compass_view_unload_resource(data);
	return -ENOMEM;
}

static int _compass_view_delete(view_data_t *view_data)
{
	compass_view_data_t *data = view_data->user_data;

	if (data->compass) {
		lv_obj_delete(data->compass);
		_compass_view_unload_resource(data);

#ifdef CONFIG_SYS_WAKELOCK
		sys_wake_unlock(FULL_WAKE_LOCK);
#endif
	} else {
		lvgl_res_preload_cancel_scene(SCENE_COMPASS_VIEW);
	}

	lvgl_res_unload_scene_compact(SCENE_COMPASS_VIEW);

	app_mem_free(data);
	return 0;
}

static int _compass_view_paint(view_data_t *view_data)
{
	const compass_view_presenter_t *presenter = view_get_presenter(view_data);
	compass_view_data_t *data = view_data->user_data;
	uint16_t bearing;

	if (data->compass == NULL) {
		return 0;
	}

	bearing = presenter->get_bearing();
	compass_set_bearing(data->compass, bearing);

	SYS_LOG_DBG("bearing %u\n", bearing);
	return 0;
}

int _compass_view_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void * msg_data)
{
	assert(view_id == COMPASS_VIEW);

	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return _compass_view_preload(view_data);
	case MSG_VIEW_LAYOUT:
		return _compass_view_layout(view_data);
	case MSG_VIEW_DELETE:
#ifdef CONFIG_SENSOR_MANAGER
		sensor_manager_disable(IN_MAG, 0);
#endif
		return _compass_view_delete(view_data);
	case MSG_VIEW_PAINT:
		return _compass_view_paint(view_data);
#ifdef CONFIG_SENSOR_MANAGER
	case MSG_VIEW_FOCUS:
		return sensor_manager_enable(IN_MAG, 0);
	case MSG_VIEW_DEFOCUS:
		return sensor_manager_disable(IN_MAG, 0);
#endif
	default:
		return 0;
	}
}

VIEW_DEFINE2(compass, _compass_view_handler, NULL,
		NULL, COMPASS_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);
