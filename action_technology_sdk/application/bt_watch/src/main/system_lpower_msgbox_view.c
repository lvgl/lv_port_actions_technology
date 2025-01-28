/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief system low power view
 */

#include <app_ui.h>
#include <widgets/img_number.h>
#include <power_manager.h>
#include <msgbox_cache.h>
#ifdef CONFIG_SYS_WAKELOCK
#include <sys_wakelock.h>
#endif

/* alarm bg view */
enum {
	//BMP_LPOWER_BG = 0,
	BMP_TXT_CHARG = 0,
	BMP_TIP_ICON,
	BMP_TXT_LPOWER,
	BMP_TXT_ENERGY,
	BMP_TXT_PERCENT,
	BMP_CHARG_ICON,
	BMP_TXT_ENERGY_USE,

	NUM_LPOWER_IMGS,
};

enum {
	BTN_TXT_CONFIRM = 0,

	NUM_LPOWER_BTNS,
};

static const uint32_t lpower_bg_bmp_ids[] = {
	//PIC_LPOWER_BG,
	PIC_TXT_CHARG,
	PIC_TIP_ICON,
	PIC_TXT_LPOWER,
	PIC_TXT_ENERGY,
	PIC_TXT_PERCENT,
	PIC_CHARG_ICON,
	PIC_TXT_ENERGY_USE,
};

static const uint32_t lpower_btn_bmp_ids[] = {
	PIC_TXT_CONFIRM,
};

static const uint32_t res_grp_energy_num_id[] = {
	RES_LPOWER_NUM,//res_lpower_num
};

static const uint32_t lpower_num_ids[] = {
	PIC_NUM_0,
	PIC_NUM_1,
	PIC_NUM_2,
	PIC_NUM_3,
	PIC_NUM_4,
	PIC_NUM_5,
	PIC_NUM_6,
	PIC_NUM_7,
	PIC_NUM_8,
	PIC_NUM_9,
};

typedef struct lpower_view_data {
	lv_obj_t *obj_bg;
	lv_obj_t *obj_lpower_bg[NUM_LPOWER_IMGS];
	lv_obj_t *obj_lpower_btn[NUM_LPOWER_BTNS];
	lv_obj_t *obj_bat_val;

	/* lvgl resource */
	lv_image_dsc_t img_dsc_lpower_bg[NUM_LPOWER_IMGS];
	lv_image_dsc_t img_dsc_lpower_btn[NUM_LPOWER_BTNS];
	lv_image_dsc_t img_dsc_lpower_nums[10];

	lv_point_t pt_lpower_bg[NUM_LPOWER_IMGS];
	lv_point_t pt_lpower_btn[NUM_LPOWER_BTNS];

	lvgl_res_scene_t res_scene;
	lvgl_res_group_t resource;
	/* user data */
	uint8_t bat_val;

	lv_timer_t *timer;
} lpower_view_data_t;

static void _lpower_btn_evt_handler(lv_event_t *e)
{
	lv_obj_t *obj = lv_event_get_current_target(e);
	lpower_view_data_t *data = lv_event_get_user_data(e);

	if (data->obj_lpower_btn[BTN_TXT_CONFIRM] == obj)
		msgbox_cache_close(LPOWER_MSGBOX_ID, false);
}

static void _lpower_create_btn(lv_obj_t *par, lv_obj_t **pobj, lv_point_t *pt,
					lv_image_dsc_t *def, lv_image_dsc_t *sel, void *user_data)
{
	*pobj = lv_imagebutton_create(par);
	lv_obj_set_pos(*pobj, pt->x, pt->y);
	lv_obj_set_size(*pobj, def->header.w, def->header.h);
	lv_obj_add_event_cb(*pobj, _lpower_btn_evt_handler, LV_EVENT_ALL, NULL);

	lv_imagebutton_set_src(*pobj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, def, NULL);
	lv_imagebutton_set_src(*pobj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, sel, NULL);
	lv_imagebutton_set_src(*pobj, LV_IMAGEBUTTON_STATE_CHECKED_RELEASED, NULL, sel, NULL);
	lv_imagebutton_set_src(*pobj, LV_IMAGEBUTTON_STATE_CHECKED_PRESSED, NULL, def, NULL);

	lv_obj_set_ext_click_area(*pobj, lv_obj_get_style_width(*pobj, LV_PART_MAIN));
}

static void _lpower_create_img_array(lv_obj_t *par, lv_obj_t **pobj, lv_point_t *pt,
										lv_image_dsc_t *img, uint32_t num)
{
	for (int i = 0; i < num; i++) {
		pobj[i] = lv_image_create(par);
		lv_image_set_src(pobj[i], &img[i]);
		lv_obj_set_pos(pobj[i], pt[i].x, pt[i].y);
	}
}

static void _lpower_unload_resource(lpower_view_data_t *data)
{
	lvgl_res_unload_pictures(data->img_dsc_lpower_bg, NUM_LPOWER_IMGS);
	lvgl_res_unload_pictures(data->img_dsc_lpower_btn, NUM_LPOWER_BTNS);
	lvgl_res_unload_pictures(data->img_dsc_lpower_nums, ARRAY_SIZE(data->img_dsc_lpower_nums));

	lvgl_res_unload_scene(&data->res_scene);
}

static int _lpower_load_resource(lpower_view_data_t *data)
{
	int ret;

	/* scene */
	ret = lvgl_res_load_scene(SCENE_LOW_POWER_VIEW, &data->res_scene,
			DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (ret < 0) {
		SYS_LOG_ERR("SCENE_LOW_POWER_VIEW not found");
		return -ENOENT;
	}

	/* background picture */
	ret = lvgl_res_load_pictures_from_scene(&data->res_scene, lpower_bg_bmp_ids, data->img_dsc_lpower_bg, data->pt_lpower_bg, NUM_LPOWER_IMGS);
	if (ret < 0) {
		goto out_exit;
	}
	/* btn picture */
	ret = lvgl_res_load_pictures_from_scene(&data->res_scene, lpower_btn_bmp_ids, data->img_dsc_lpower_btn, data->pt_lpower_btn, NUM_LPOWER_BTNS);
	if (ret < 0) {
		goto out_exit;
	}
	ret = lvgl_res_load_group_from_scene(&data->res_scene, res_grp_energy_num_id[0], &data->resource);
	if (ret < 0) {
		goto out_exit;
	}
	ret = lvgl_res_load_pictures_from_group( &data->resource, lpower_num_ids,
			data->img_dsc_lpower_nums, NULL, ARRAY_SIZE(lpower_num_ids));

	lvgl_res_unload_group(&data->resource);

	return 0;

out_exit:
	if (ret < 0) {
		_lpower_unload_resource(data);
	}

	return ret;
}

static void _timer_cb(lv_timer_t * timer)
{
	lpower_view_data_t *data = timer->user_data;

#ifdef CONFIG_POWER
	data->bat_val = power_manager_get_battery_capacity();
#endif

	img_number_set_value(data->obj_bat_val, data->bat_val, 2);
}

static int _lpower_popup(lv_obj_t * parent)
{
	lpower_view_data_t *data = NULL;

	data = app_mem_malloc(sizeof(*data));
	if (!data)
		return -ENOMEM;

	memset(data, 0, sizeof(*data));

	/* create lpower bg image */
	if (_lpower_load_resource(data)) {
		SYS_LOG_ERR("res load failed");
		app_mem_free(data);
		return -ENOMEM;
	}

	lv_obj_set_style_bg_color(parent, lv_color_make(0x3b, 0x3b, 0x3b), 0);
	lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

	/* create bg image */
	data->obj_bg = lv_obj_create(parent);
	lv_obj_set_size(data->obj_bg, data->res_scene.width, data->res_scene.height);
	lv_obj_center(data->obj_bg);

	_lpower_create_img_array(data->obj_bg, data->obj_lpower_bg, data->pt_lpower_bg,
			data->img_dsc_lpower_bg, NUM_LPOWER_IMGS);

	_lpower_create_btn(data->obj_bg, &data->obj_lpower_btn[BTN_TXT_CONFIRM],
			&data->pt_lpower_btn[BTN_TXT_CONFIRM], data->img_dsc_lpower_btn,
			data->img_dsc_lpower_btn, data);

	lv_obj_add_flag(data->obj_lpower_btn[BTN_TXT_CONFIRM], LV_OBJ_FLAG_CLICKABLE);

	data->obj_bat_val = img_number_create(data->obj_bg);
	lv_obj_set_pos(data->obj_bat_val, data->resource.x, data->resource.y);
	lv_obj_set_size(data->obj_bat_val, data->resource.width, data->resource.height);
	img_number_set_src(data->obj_bat_val, data->img_dsc_lpower_nums, 10);
	img_number_set_align(data->obj_bat_val, LV_ALIGN_RIGHT_MID);

	data->timer = lv_timer_create(_timer_cb, 5000, data);
	if (data->timer)
		_timer_cb(data->timer);

	lv_obj_set_user_data(parent, data);

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_lock(FULL_WAKE_LOCK);
#endif

	SYS_LOG_INF("lpower msgbox created");
	return 0;
}

static int _lpower_close(lv_obj_t * parent)
{
	lpower_view_data_t *data = lv_obj_get_user_data(parent);

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_unlock(FULL_WAKE_LOCK);
#endif

	if (data->timer)
		lv_timer_delete(data->timer);

	_lpower_unload_resource(data);
	app_mem_free(data);

	SYS_LOG_INF("lpower msgbox deleted\n");
	return 0;
}

int lpower_msgbox_popup_cb(uint16_t msgbox_id, uint8_t msg_id, void * msg_data, void * hwnd)
{
	if (msgbox_id != LPOWER_MSGBOX_ID)
		return -EINVAL;

	switch (msg_id) {
	case MSG_MSGBOX_POPUP:
		return _lpower_popup(hwnd);
	case MSG_MSGBOX_CLOSE:
		return _lpower_close(hwnd);
	case MSG_MSGBOX_KEY:
	case MSG_MSGBOX_CANCEL:
	default:
		return 0;
	}
}

void system_create_low_power_view(void)
{
	msgbox_cache_popup(LPOWER_MSGBOX_ID, NULL);
}

void system_delete_low_power_view(void)
{
	msgbox_cache_close(LPOWER_MSGBOX_ID, false);
}
