/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ota view
 */
#include "ota_app.h"
#include <string.h>
#include <msgbox_cache.h>
#include <stdio.h>
#include <app_ui.h>
#include <app_defines.h>
#include <widgets/img_number.h>
#ifdef CONFIG_SYS_WAKELOCK
#include <sys_wakelock.h>
#endif
#ifdef CONFIG_PROPERTY
#include <property_manager.h>
#endif
#ifdef CONFIG_RTC_ACTS
#include <drivers/rtc.h>
#endif

/* ota bg view */
enum {
	//BMP_OTA_BG = 0,
	BMP_OTA_ICON = 0,
	BMP_TXT_SYSTEM_OTA,
#if DEF_UI_WIDTH >= 454
	BMP_TXT_TOD,
#endif
	BMP_TM_COLON,
	BMP_TXT_VER_TIP,

	NUM_OTA_IMGS,
};

enum {
	BTN_INSTALL = 0,

	NUM_OTA_BTNS,
};

static const uint32_t ota_bg_bmp_ids[] = {
	//PIC_OTA_BG,
	PIC_OTA_ICON,
	PIC_TXT_SYSTEM_OTA,
#if DEF_UI_WIDTH >= 454
	PIC_TXT_TOD,
#endif
	PIC_TM_COLON,
	PIC_TXT_VER_TIP,
};
static const uint32_t ota_btn_bmp_ids[] = {
	PIC_BTN_INSTALL,
};
static const uint32_t res_grp_tm_hour_id[] = {
	RES_OTA_HOUR,
};
static const uint32_t res_grp_tm_min_id[] = {
	RES_OTA_MIN,
};

static const uint32_t ota_num_ids[] = {
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

typedef struct ota_view_data {
	lv_obj_t *obj_ota_bg;
	lv_obj_t *obj_ota_bmp[NUM_OTA_IMGS];
	lv_obj_t *obj_ota_btn[NUM_OTA_BTNS];
	lv_obj_t *obj_tm_h;
	lv_obj_t *obj_tm_m;

	/* lvgl resource */
	lv_image_dsc_t img_dsc_ota_bmp[NUM_OTA_IMGS];
	lv_image_dsc_t img_dsc_ota_btn[NUM_OTA_BTNS];
	lv_image_dsc_t img_dsc_ota_nums[10];

	lv_point_t pt_ota_bmp[NUM_OTA_IMGS];
	lv_point_t pt_ota_btn[NUM_OTA_BTNS];

	lvgl_res_scene_t res_scene;
	lvgl_res_group_t resource_h;
	lvgl_res_group_t resource_m;
	/* user data */
	uint16_t tm_min;
} ota_view_data_t;

void ota_view_show_upgrade_result(uint8_t *string, bool is_faill)
{
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_display_string(SLED_NUMBER1, string, true);
	seg_led_manager_set_timeout_event_start();
	seg_led_manager_set_timeout_event(1000, NULL);
#endif

#ifdef CONFIG_LED_MANAGER
	if (is_faill) {
		led_manager_set_display(0, LED_OFF, OS_FOREVER, NULL);
		led_manager_set_display(1, LED_OFF, OS_FOREVER, NULL);
	} else {
		led_manager_set_display(0, LED_ON, OS_FOREVER, NULL);
		led_manager_set_display(1, LED_ON, OS_FOREVER, NULL);
	}
#endif
}

void ota_view_show_upgrade_progress(uint8_t progress)
{
	uint8_t progress_string[5];
	snprintf(progress_string, sizeof(progress_string), "U%u", progress);

#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_manager_clear_screen(LED_CLEAR_ALL);
	seg_led_display_string(SLED_NUMBER1, progress_string, true);
#endif
}

static void _ota_bmp_event_handler(lv_event_t * e)
{
	lv_obj_t *obj = lv_event_get_current_target(e);
	ota_view_data_t * data = lv_event_get_user_data(e);

	if (data->obj_ota_bg == obj) {
		ota_view_exit();
	}
}

static void _ota_btn_evt_handler(lv_event_t *e)
{
	lv_obj_t *obj = lv_event_get_current_target(e);
	ota_view_data_t * data = lv_event_get_user_data(e);

	if (data->obj_ota_btn[BTN_INSTALL] == obj)
		ota_install_start();
}

static void _ota_create_btn(lv_obj_t *par, lv_obj_t **pobj, lv_point_t *pt,
							lv_image_dsc_t *def, lv_image_dsc_t *sel, void * user_data)
{
	*pobj = lv_imagebutton_create(par);
	lv_obj_set_pos(*pobj, pt->x, pt->y);
	lv_obj_set_size(*pobj, def->header.w, def->header.h);
	lv_obj_add_event_cb(*pobj, _ota_btn_evt_handler, LV_EVENT_CLICKED, user_data);

	lv_imagebutton_set_src(*pobj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, def, NULL);
	lv_imagebutton_set_src(*pobj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, sel, NULL);
	lv_imagebutton_set_src(*pobj, LV_IMAGEBUTTON_STATE_CHECKED_RELEASED, NULL, sel, NULL);
	lv_imagebutton_set_src(*pobj, LV_IMAGEBUTTON_STATE_CHECKED_PRESSED, NULL, def, NULL);

	SYS_LOG_INF("x=%d y=%d w=%d h=%d\n", pt->x, pt->y, def->header.w, def->header.h);
}

static void _ota_unload_resource(ota_view_data_t *data)
{
	lvgl_res_unload_pictures(data->img_dsc_ota_bmp, NUM_OTA_IMGS);
	lvgl_res_unload_pictures(data->img_dsc_ota_btn, NUM_OTA_BTNS);
	lvgl_res_unload_pictures(data->img_dsc_ota_nums, ARRAY_SIZE(data->img_dsc_ota_nums));

	lvgl_res_unload_scene(&data->res_scene);
}

static int _ota_load_resource(ota_view_data_t *data)
{
	int ret;

	/* scene */
	ret = lvgl_res_load_scene(SCENE_OTA_VIEW, &data->res_scene,
			DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (ret < 0) {
		SYS_LOG_ERR("SCENE_OTA_VIEW not found");
		return -ENOENT;
	}

	/* background picture */
	ret = lvgl_res_load_pictures_from_scene(&data->res_scene, ota_bg_bmp_ids, data->img_dsc_ota_bmp, data->pt_ota_bmp, NUM_OTA_IMGS);
	if (ret < 0) {
		goto out_exit;
	}
	/* btn picture */
	ret = lvgl_res_load_pictures_from_scene(&data->res_scene, ota_btn_bmp_ids, data->img_dsc_ota_btn, data->pt_ota_btn, NUM_OTA_BTNS);
	if (ret < 0) {
		goto out_exit;
	}
	ret = lvgl_res_load_group_from_scene(&data->res_scene, res_grp_tm_hour_id[0], &data->resource_h);
	if (ret < 0) {
		goto out_exit;
	}
	ret = lvgl_res_load_pictures_from_group( &data->resource_h, ota_num_ids,
			data->img_dsc_ota_nums, NULL, ARRAY_SIZE(ota_num_ids));

	lvgl_res_unload_group(&data->resource_h);

	ret = lvgl_res_load_group_from_scene(&data->res_scene, res_grp_tm_min_id[0], &data->resource_m);
	if (ret < 0) {
		goto out_exit;
	}
	lvgl_res_unload_group(&data->resource_m);

	return 0;

out_exit:
	if (ret < 0) {
		_ota_unload_resource(data);
	}
	SYS_LOG_ERR("load resource fail \n");
	return ret;
}

static void _ota_create_img_array(lv_obj_t *par, lv_obj_t **pobj, lv_point_t *pt,
										lv_image_dsc_t *img, uint32_t num)
{
	for (int i = 0; i < num; i++) {
		pobj[i] = lv_image_create(par);
		lv_image_set_src(pobj[i], &img[i]);
		lv_obj_set_pos(pobj[i], pt[i].x, pt[i].y);
		SYS_LOG_INF("x=%d y=%d w=%d h=%d\n", pt[i].x, pt[i].y, img[i].header.w, img[i].header.h);
	}
}

static int _ota_paint(lv_obj_t * parent)
{
	ota_view_data_t * data = lv_obj_get_user_data(parent);

#ifdef CONFIG_RTC_ACTS
	struct rtc_time rtc_time;
	const struct device *rtc = device_get_binding(CONFIG_RTC_0_NAME);
	if (rtc) {
		rtc_get_time(rtc, &rtc_time);
		print_rtc_time(&rtc_time);
		data->tm_min = rtc_time.tm_hour * 60 + rtc_time.tm_min;
	}
#endif

	img_number_set_value(data->obj_tm_h, data->tm_min / 60, 2);
	img_number_set_value(data->obj_tm_m, data->tm_min % 60, 2);
	return 0;
}

static int _ota_popup(lv_obj_t * parent)
{
	ota_view_data_t * data = app_mem_malloc(sizeof(*data));
	if (data == NULL)
		return -ENOMEM;

	memset(data, 0, sizeof(*data));

	if (_ota_load_resource(data)) {
		app_mem_free(data);
		return -ENOMEM;
	}

	lv_obj_set_style_bg_color(parent, lv_color_make(0x3b, 0x3b, 0x3b), 0);
	lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

	/* create ota bg image */
	data->obj_ota_bg = lv_obj_create(parent);
	lv_obj_set_size(data->obj_ota_bg, data->res_scene.width, data->res_scene.height);
	lv_obj_center(data->obj_ota_bg);

	lv_obj_add_event_cb(data->obj_ota_bg, _ota_bmp_event_handler, LV_EVENT_GESTURE, data);
	/*set current background clickable to cut out click event send to front bg*/
	lv_obj_add_flag(data->obj_ota_bg, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_remove_flag(data->obj_ota_bg, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_GESTURE_BUBBLE);

	/* create ota bg image */
	_ota_create_img_array(data->obj_ota_bg, data->obj_ota_bmp,
			data->pt_ota_bmp, data->img_dsc_ota_bmp, NUM_OTA_IMGS);
	/* create ota btn */
	_ota_create_btn(data->obj_ota_bg, &data->obj_ota_btn[BTN_INSTALL], &data->pt_ota_btn[BTN_INSTALL],
			data->img_dsc_ota_btn, data->img_dsc_ota_btn, data);
	lv_obj_add_flag(data->obj_ota_btn[BTN_INSTALL], LV_OBJ_FLAG_CLICKABLE);

	data->obj_tm_h = img_number_create(data->obj_ota_bg);
	lv_obj_set_pos(data->obj_tm_h, data->resource_h.x, data->resource_h.y);
	lv_obj_set_size(data->obj_tm_h, data->resource_h.width, data->resource_h.height);
	img_number_set_src(data->obj_tm_h, data->img_dsc_ota_nums, 10);
	img_number_set_align(data->obj_tm_h, LV_ALIGN_RIGHT_MID);

	data->obj_tm_m = img_number_create(data->obj_ota_bg);
	lv_obj_set_pos(data->obj_tm_m, data->resource_m.x, data->resource_m.y);
	lv_obj_set_size(data->obj_tm_m, data->resource_m.width, data->resource_m.height);
	img_number_set_src(data->obj_tm_m, data->img_dsc_ota_nums, 10);
	img_number_set_align(data->obj_tm_m, LV_ALIGN_RIGHT_MID);

	lv_obj_set_user_data(parent, data);
	_ota_paint(parent);

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_lock(FULL_WAKE_LOCK);
#endif

	return 0;
}

static int _ota_close(lv_obj_t * parent)
{
	ota_view_data_t * data = lv_obj_get_user_data(parent);

	_ota_unload_resource(data);

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_unlock(FULL_WAKE_LOCK);
#endif

#ifdef CONFIG_PROPERTY
	property_flush(NULL);
#endif

	app_mem_free(data);
	return 0;
}

int ota_msgbox_popup_cb(uint16_t msgbox_id, uint8_t msg_id, void * msg_data, void * hwnd)
{
	if (msgbox_id != OTA_MSGBOX_ID)
		return -EINVAL;

	switch (msg_id) {
	case MSG_MSGBOX_POPUP:
		return _ota_popup(hwnd);
	case MSG_MSGBOX_PAINT:
		return _ota_paint(hwnd);
	case MSG_MSGBOX_CLOSE:
		return _ota_close(hwnd);
	case MSG_MSGBOX_CANCEL:
	default:
		return 0;
	}
}

void ota_view_init(void)
{
	msgbox_cache_popup(OTA_MSGBOX_ID, NULL);
}

void ota_view_deinit(void)
{
	msgbox_cache_close(OTA_MSGBOX_ID, false);
}
