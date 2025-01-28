/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief alarm app view
 */

#include "alarm.h"

#ifdef CONFIG_UI_MANAGER
#include <ui_manager.h>
#endif
#include <view_manager.h>
#include <lvgl/lvgl_res_loader.h>
#include <msgbox_cache.h>
#ifdef CONFIG_SYS_WAKELOCK
#include <sys_wakelock.h>
#endif

/* alarm ring bg */
enum {
	BMP_AL_RING_ICON = 0,
	BMP_AL_RING_NAME,
	BMP_AL_RING_ICON_L,

	NUM_AL_RING_BG_IMGS,
};

/* alarm ring btn */
enum {
	BTN_AL_RING_SN = 0,
	BTN_AL_RING_STOP,

	NUM_AL_RING_BTNS,
};

static const uint32_t al_ring_bg_bmp_ids[] = {
	PIC_ALARM_RING_ICON,
	PIC_ALARM_RING_NAME,
	PIC_ALARM_RING_I,
};

static const uint32_t al_ring_btn_def_ids[] = {
	PIC_BTN_SN,
	PIC_BTN_STOP,
};

static const uint32_t al_ring_btn_sel_ids[] = {
	PIC_BTN_SN_P,
	PIC_BTN_STOP_P,
};

typedef struct al_ring_view_data {
	lv_obj_t *obj_bg;
	lv_obj_t *obj_ring_bg[NUM_AL_RING_BG_IMGS];
	lv_obj_t *obj_ring_btn[NUM_AL_RING_BTNS];

	/* lvgl resource */
	lv_image_dsc_t img_dsc_ring_bg[NUM_AL_RING_BG_IMGS];
	lv_image_dsc_t img_dsc_def_ring_btn[NUM_AL_RING_BTNS];
	lv_image_dsc_t img_dsc_sel_ring_btn[NUM_AL_RING_BTNS];

	lv_point_t pt_ring_bg[NUM_AL_RING_BG_IMGS];
	lv_point_t pt_ring_btn[NUM_AL_RING_BTNS];

	lvgl_res_scene_t res_scene;
} al_ring_view_data_t;

void alarm_calendar_year_display(struct alarm_app_t *alarm, bool is_light)
{
#ifdef CONFIG_SEG_LED_MANAGER
	char diplay_str[5] = {0};

	seg_led_manager_set_timeout_event_start();
	seg_led_display_icon(SLED_COL, false);
	snprintf(diplay_str, sizeof(diplay_str), "%04u", alarm->tm.tm_year);
	seg_led_display_string(SLED_NUMBER1, diplay_str, is_light);
	seg_led_manager_set_timeout_event(20000, NULL);
#endif
}

void alarm_calendar_mon_day_display(struct alarm_app_t *alarm, bool is_light)
{
#ifdef CONFIG_SEG_LED_MANAGER
	char diplay_str[5] = {0};

	seg_led_manager_set_timeout_event_start();
	seg_led_display_icon(SLED_COL, false);
	snprintf(diplay_str, sizeof(diplay_str), "%02u%02u", alarm->tm.tm_mon, alarm->tm.tm_mday);
	seg_led_display_string(SLED_NUMBER1, diplay_str, is_light);
	seg_led_manager_set_timeout_event(20000, NULL);
#endif
}

void alarm_calendar_mon_flash(struct alarm_app_t *alarm, bool is_light)
{
#ifdef CONFIG_SEG_LED_MANAGER
	char diplay_str[3] = {0};

	seg_led_manager_set_timeout_event_start();
	snprintf(diplay_str, sizeof(diplay_str), "%02u", alarm->tm.tm_mon);
	seg_led_display_string(SLED_NUMBER1, diplay_str, is_light);
	seg_led_manager_set_timeout_event(20000, NULL);
#endif
}

void alarm_calendar_day_flash(struct alarm_app_t *alarm, bool is_light)
{
#ifdef CONFIG_SEG_LED_MANAGER
	char diplay_str[3] = {0};

	seg_led_manager_set_timeout_event_start();
	snprintf(diplay_str, sizeof(diplay_str), "%02u", alarm->tm.tm_mday);
	seg_led_display_string(SLED_NUMBER3, diplay_str, is_light);
	seg_led_manager_set_timeout_event(20000, NULL);
#endif
}

void alarm_clock_set_display(struct alarm_app_t *alarm)
{
#ifdef CONFIG_SEG_LED_MANAGER
	char diplay_str[5] = {0};

	seg_led_manager_set_timeout_event_start();
	seg_led_display_icon(SLED_COL, true);
	snprintf(diplay_str, sizeof(diplay_str), "%02u%02u", alarm->tm.tm_hour, alarm->tm.tm_min);
	seg_led_display_string(SLED_NUMBER1, diplay_str, true);
	seg_led_manager_set_timeout_event(20000, NULL);
#endif
}

void alarm_clock_display(struct alarm_app_t *alarm, bool need_update)
{
#ifdef CONFIG_SEG_LED_MANAGER
	char diplay_str[5] = {0};

	seg_led_manager_set_timeout_event_start();
	seg_led_display_icon(SLED_P1, false);
	if (need_update)
		alarm_manager_get_time(&alarm->tm);

	snprintf(diplay_str, sizeof(diplay_str), "%02u%02u", alarm->tm.tm_hour, alarm->tm.tm_min);
	seg_led_display_string(SLED_NUMBER1, diplay_str, true);

	if (alarm->reflash_counter % 2)
		seg_led_display_icon(SLED_COL, false);
	else
		seg_led_display_icon(SLED_COL, true);
	seg_led_manager_set_timeout_event(20000, NULL);
#endif
}

void alarm_clock_hour_flash(struct alarm_app_t *alarm, bool is_light)
{
#ifdef CONFIG_SEG_LED_MANAGER
	char diplay_str[3] = {0};

	seg_led_manager_set_timeout_event_start();
	snprintf(diplay_str, sizeof(diplay_str), "%02u", alarm->tm.tm_hour);
	seg_led_display_string(SLED_NUMBER1, diplay_str, is_light);
	seg_led_manager_set_timeout_event(20000, NULL);
#endif
}

void alarm_clock_min_flash(struct alarm_app_t *alarm, bool is_light)
{
#ifdef CONFIG_SEG_LED_MANAGER
	char diplay_str[3] = {0};

	seg_led_manager_set_timeout_event_start();
	snprintf(diplay_str, sizeof(diplay_str), "%02u", alarm->tm.tm_min);
	seg_led_display_string(SLED_NUMBER3, diplay_str, is_light);
	seg_led_manager_set_timeout_event(20000, NULL);
#endif
}

void alarm_onoff_display(struct alarm_app_t *alarm, bool is_light)
{
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_manager_set_timeout_event_start();
	seg_led_display_icon(SLED_COL, false);
	if (alarm->alarm_is_on) {
		seg_led_display_string(SLED_NUMBER1, " ON ", is_light);
	} else {
		seg_led_display_string(SLED_NUMBER1, " OFF", is_light);
	}
	seg_led_manager_set_timeout_event(20000, NULL);
#endif
}

void alarm_ringing_display(struct alarm_app_t *alarm)
{
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_display_icon(SLED_PLAY, true);
	if (memcmp(alarm->dir, "SD:ALARM", strlen("SD:ALARM")) == 0) {
		seg_led_display_icon(SLED_SD, true);
		seg_led_display_icon(SLED_USB, false);
	} else if (memcmp(alarm->dir, "USB:ALARM", strlen("USB:ALARM")) == 0) {
		seg_led_display_icon(SLED_USB, true);
		seg_led_display_icon(SLED_SD, false);
	}
	alarm_clock_display(alarm, true);
	seg_led_manager_set_timeout_event(0, NULL);
	alarm->set_ok = 0;
#ifdef CONFIG_THREAD_TIMER
	thread_timer_start(&alarm->monitor_timer, REFLASH_CLOCK_PERIOD, REFLASH_CLOCK_PERIOD);
#endif
#endif
}

void alarm_ringing_clock_flash(struct alarm_app_t *alarm, bool is_light)
{
#ifdef CONFIG_SEG_LED_MANAGER
	char diplay_str[5] = {0};

	alarm_manager_get_time(&alarm->tm);
	snprintf(diplay_str, sizeof(diplay_str), "%02u%02u", alarm->tm.tm_hour, alarm->tm.tm_min);
	seg_led_display_string(SLED_NUMBER1, diplay_str, is_light);
	seg_led_display_icon(SLED_COL, is_light);
#endif
}

static void _alarm_ring_btn_evt_handler(lv_event_t *e)
{
	lv_obj_t *obj = lv_event_get_current_target(e);
	al_ring_view_data_t *data = lv_event_get_user_data(e);

	if (data->obj_ring_btn[BTN_AL_RING_SN] == obj) {
		alarm_view_snooze_event();
	} else if (data->obj_ring_btn[BTN_AL_RING_STOP] == obj) {
		alarm_view_stop_event();
	}
}

static void _alarm_ring_create_img_array(lv_obj_t *par, lv_obj_t **pobj, lv_point_t *pt,
										lv_image_dsc_t *img, uint32_t num)
{
	for (int i = 0; i < num; i++) {
		pobj[i] = lv_image_create(par);
		lv_image_set_src(pobj[i], &img[i]);
		lv_obj_set_pos(pobj[i], pt[i].x, pt[i].y);
	}
}

static void _alarm_ring_create_btn(lv_obj_t *par, lv_obj_t **pobj, lv_point_t *pt,
				    lv_image_dsc_t *def, lv_image_dsc_t *sel, void * user_data)
{
	*pobj = lv_imagebutton_create(par);
	lv_obj_set_pos(*pobj, pt->x, pt->y);
	lv_obj_set_size(*pobj, def->header.w, def->header.h);
	lv_obj_add_event_cb(*pobj, _alarm_ring_btn_evt_handler, LV_EVENT_CLICKED, user_data);

	lv_imagebutton_set_src(*pobj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, def, NULL);
	lv_imagebutton_set_src(*pobj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, sel, NULL);
	lv_imagebutton_set_src(*pobj, LV_IMAGEBUTTON_STATE_CHECKED_RELEASED, NULL, def, NULL);
	lv_imagebutton_set_src(*pobj, LV_IMAGEBUTTON_STATE_CHECKED_PRESSED, NULL, sel, NULL);
}

static void _alarm_ring_unload_resource(al_ring_view_data_t *data)
{
	lvgl_res_unload_pictures(data->img_dsc_ring_bg, NUM_AL_RING_BG_IMGS);
	lvgl_res_unload_pictures(data->img_dsc_def_ring_btn, NUM_AL_RING_BTNS);
	lvgl_res_unload_pictures(data->img_dsc_sel_ring_btn, NUM_AL_RING_BTNS);

	lvgl_res_unload_scene(&data->res_scene);
}

static int _alarm_ring_load_resource(al_ring_view_data_t *data)
{
	int ret;

	/* scene */
	ret = lvgl_res_load_scene(SCENE_ALARM_RING_VIEW, &data->res_scene,
			DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (ret < 0) {
		SYS_LOG_ERR("SCENE_ALARM_SET_VIEW not found");
		return -ENOENT;
	}

	/* bg picture */
	ret = lvgl_res_load_pictures_from_scene(&data->res_scene, al_ring_bg_bmp_ids, data->img_dsc_ring_bg, data->pt_ring_bg, NUM_AL_RING_BG_IMGS);
	if (ret < 0) {
		goto out_exit;
	}
	/* btn picture */
	ret = lvgl_res_load_pictures_from_scene(&data->res_scene, al_ring_btn_def_ids, data->img_dsc_def_ring_btn, data->pt_ring_btn, NUM_AL_RING_BTNS);
	if (ret < 0) {
		goto out_exit;
	}
	ret = lvgl_res_load_pictures_from_scene(&data->res_scene, al_ring_btn_sel_ids, data->img_dsc_sel_ring_btn, data->pt_ring_btn, NUM_AL_RING_BTNS);
	if (ret < 0) {
		goto out_exit;
	}
	return ret;
out_exit:
	if (ret < 0) {
		_alarm_ring_unload_resource(data);
	}

	return ret;
}

static int _alarm_popup(lv_obj_t * parent)
{
	al_ring_view_data_t *data = app_mem_malloc(sizeof(*data));
	if (data == NULL)
		return -ENOMEM;

	memset(data, 0, sizeof(*data));

	if (_alarm_ring_load_resource(data)) {
		app_mem_free(data);
		return -ENOMEM;
	}

	/* create bg image */
	data->obj_bg = lv_obj_create(parent);
	lv_obj_set_size(data->obj_bg, data->res_scene.width, data->res_scene.height);
	lv_obj_set_style_bg_color(data->obj_bg, lv_color_make(0x3b, 0x3b, 0x3b), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(data->obj_bg, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_center(data->obj_bg);

	_alarm_ring_create_img_array(data->obj_bg, data->obj_ring_bg,
			data->pt_ring_bg, data->img_dsc_ring_bg, NUM_AL_RING_BG_IMGS);

	lv_obj_remove_flag(data->obj_bg, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_GESTURE_BUBBLE);
	/* create alarm ring btn image */
	_alarm_ring_create_btn(data->obj_bg, &data->obj_ring_btn[BTN_AL_RING_SN],
			&data->pt_ring_btn[BTN_AL_RING_SN],&data->img_dsc_def_ring_btn[BTN_AL_RING_SN],
			&data->img_dsc_sel_ring_btn[BTN_AL_RING_SN], data);
	lv_obj_add_flag(data->obj_ring_btn[BTN_AL_RING_SN], LV_OBJ_FLAG_CHECKABLE);
	_alarm_ring_create_btn(data->obj_bg, &data->obj_ring_btn[BTN_AL_RING_STOP],
			&data->pt_ring_btn[BTN_AL_RING_STOP], &data->img_dsc_def_ring_btn[BTN_AL_RING_STOP],
			&data->img_dsc_sel_ring_btn[BTN_AL_RING_STOP], data);
	lv_obj_add_flag(data->obj_ring_btn[BTN_AL_RING_STOP], LV_OBJ_FLAG_CHECKABLE);

	lv_obj_set_user_data(parent, data);
	lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(parent, lv_color_black(), 0);

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_lock(FULL_WAKE_LOCK);
#endif

	return 0;
}

static int _alarm_close(lv_obj_t * parent)
{
	al_ring_view_data_t *data = lv_obj_get_user_data(parent);
	int i;
	
#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_unlock(FULL_WAKE_LOCK);
#endif

	for (i = 0; i < NUM_AL_RING_BTNS; i++) {
		if (data->obj_ring_btn[i])
			lv_obj_delete(data->obj_ring_btn[i]);
	}

	for (i = 0; i < NUM_AL_RING_BG_IMGS; i++) {
		if (data->obj_ring_bg[i])
			lv_obj_delete(data->obj_ring_bg[i]);
	}

	if (data->obj_bg)
		lv_obj_delete(data->obj_bg);

	_alarm_ring_unload_resource(data);
	app_mem_free(data);
	return 0;
}

int alarm_msgbox_popup_cb(uint16_t msgbox_id, uint8_t msg_id, void * msg_data, void * hwnd)
{
	if (msgbox_id != ALARM_MSGBOX_ID)
		return -EINVAL;

	switch (msg_id) {
	case MSG_MSGBOX_POPUP:
		return _alarm_popup(hwnd);
	case MSG_MSGBOX_CLOSE:
		return _alarm_close(hwnd);
	case MSG_MSGBOX_CANCEL:
	default:
		return 0;
	}
}

void alarm_view_init(void)
{
	msgbox_cache_popup(ALARM_MSGBOX_ID, NULL);
}

void alarm_view_deinit(void)
{
	msgbox_cache_close(ALARM_MSGBOX_ID, false);
}
