/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <assert.h>
#include <app_ui.h>
#include <view_stack.h>
#include <sys_manager.h>
#include <display/display_composer.h>
#include "clock_selector/clock_selector.h"
#include "widgets/watch_clock.h"
#include "clock_view.h"
#include "main_view.h"

typedef struct aod_clock_view_tmp_res {
	lvgl_res_group_t grp_hour;
	lvgl_res_group_t grp_minute;

	lv_point_t pt_colon;
} aod_clock_view_tmp_res_t;

typedef struct aod_clock_view_data {
	lv_obj_t *obj_clock;

	bool is_digital;
	/* last updated time */
	uint8_t tm_hour;
	uint8_t tm_min;
	uint32_t period;

	/* lvgl resource */
	lv_image_dsc_t img_bg;
	lv_image_dsc_t img_tm_colon; /* : between hour and minute */
	lv_image_dsc_t img_tm_hour[10]; /* 0~9 */
	lv_image_dsc_t img_tm_min[10];  /* 0~9 */

	const clock_dsc_t * clock_dsc;
	lvgl_res_scene_t scene;
} aod_clock_view_data_t;

extern const main_view_presenter_t main_view_presenter;
static int _aod_clock_view_paint(view_data_t *view_data, bool first);

static const uint32_t pic_common_ids[] = {
	PIC_0, PIC_1, PIC_2, PIC_3, PIC_4, PIC_5, PIC_6, PIC_7, PIC_8, PIC_9,
};

static bool g_aod_inflated = false;

static int _load_pictures(lvgl_res_scene_t *scene, uint32_t group_id,
		lvgl_res_group_t *group, const uint32_t *pic_ids,
		int num_ids, lv_image_dsc_t *images)
{
	int res;

	if (lvgl_res_load_group_from_scene(scene, group_id, group)) {
		return -ENOENT;
	}

	res = lvgl_res_load_pictures_from_group(group, pic_ids, images, NULL, num_ids);
	lvgl_res_unload_group(group);

	return res;
}

static int _aod_clock_view_load_resource(aod_clock_view_data_t *data, aod_clock_view_tmp_res_t *tmp_res)
{
	uint32_t pic_id;
	int ret;

	/* scene */
	ret = lvgl_res_load_scene(data->clock_dsc->scene, &data->scene,
			DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (ret < 0) {
		SYS_LOG_ERR("scene 0x%x not found", data->clock_dsc->scene);
		return -ENOENT;
	}

	/* test scene is digital or not */
	ret = lvgl_res_load_group_from_scene(&data->scene, RES_DIGITAL_ID, &tmp_res->grp_hour);
	if (ret == 0) {
		data->is_digital = true;
		lvgl_res_unload_group(&tmp_res->grp_hour);
	}

	/* background picture */
	pic_id = PIC_BACKGROUND;
	ret = lvgl_res_load_pictures_from_scene(&data->scene, &pic_id, &data->img_bg, NULL, 1);

	/* hour */
	ret = _load_pictures(&data->scene, RES_TM_HOUR, &tmp_res->grp_hour,
			pic_common_ids, data->is_digital ? 10 : 1, data->img_tm_hour);
	if (ret < 0) {
		goto out_exit;
	}

	/* minute */
	ret = _load_pictures(&data->scene, RES_TM_MINUTE, &tmp_res->grp_minute,
			pic_common_ids, data->is_digital ? 10 : 1, data->img_tm_min);
	if (ret < 0) {
		goto out_exit;
	}

	/* time colon */
	pic_id = PIC_TM_COLON;
	lvgl_res_load_pictures_from_scene(&data->scene, &pic_id,
			&data->img_tm_colon, &tmp_res->pt_colon, 1);

out_exit:
	lvgl_res_unload_scene(&data->scene);
	return ret;
}

static void _aod_clock_view_unload_resource(aod_clock_view_data_t *data)
{
	lvgl_res_unload_pictures(data->img_tm_hour, data->is_digital ? 10 : 1);
	lvgl_res_unload_pictures(data->img_tm_min, data->is_digital ? 10 : 1);
	lvgl_res_unload_pictures(&data->img_bg, 1);
	lvgl_res_unload_pictures(&data->img_tm_colon, 1);
}

static int _aod_clock_view_preload(view_data_t *view_data)
{
	aod_clock_view_data_t *data;
	int ret;

	data = app_mem_malloc(sizeof(*data));
	if (data == NULL) {
		return -ENOMEM;
	}

	memset(data, 0, sizeof(*data));
	data->tm_hour = UINT8_MAX;
	data->tm_min = UINT8_MAX;

	data->clock_dsc = clocksel_get_aod_clock_dsc();
	if (data->clock_dsc == NULL) {
		SYS_LOG_ERR("no aod clock");
		app_mem_free(data);
		return -ENOENT;
	}

	ret = lvgl_res_preload_scene_compact(data->clock_dsc->scene, NULL, 0,
			lvgl_res_scene_preload_default_cb_for_view, (void *)AOD_CLOCK_VIEW,
			DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (ret) {
		app_mem_free(data);
		return ret;
	}

	view_data->user_data = data;
	return 0;
}

static int _aod_clock_view_layout(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	aod_clock_view_data_t *data = view_data->user_data;
	aod_clock_view_tmp_res_t *resource = NULL;

	resource = app_mem_malloc(sizeof(*resource));
	if (!resource) {
		SYS_LOG_ERR("clock_view malloc tmp resource failed\n");
		return -ENOMEM;
	}

	if (_aod_clock_view_load_resource(data, resource)) {
		app_mem_free(resource);
		return -ENOENT;
	}

	data->obj_clock = watch_clock_create(scr);
	if (!data->obj_clock) {
		app_mem_free(resource);
		return -ENOMEM;
	}

	watch_clock_set_type(data->obj_clock,
			data->is_digital ? DIGITAL_CLOCK : ANALOG_CLOCK, false);
	watch_clock_set_24hour(data->obj_clock, data->is_digital);

	lv_obj_set_size(data->obj_clock, data->scene.width, data->scene.height);
	lv_obj_center(data->obj_clock);

	if (data->img_bg.data) {
		lv_obj_set_style_bg_img_src(data->obj_clock, &data->img_bg, LV_PART_MAIN);
		lv_obj_set_style_bg_image_opa(data->obj_clock, LV_OPA_COVER, LV_PART_MAIN);
	} else {
		lv_obj_set_style_bg_color(data->obj_clock, data->scene.background, LV_PART_MAIN);
		lv_obj_set_style_bg_opa(data->obj_clock, LV_OPA_COVER, LV_PART_MAIN);
	}

	if (data->is_digital) {
		lv_area_t area;

		lv_area_set(&area, resource->grp_hour.x, resource->grp_hour.y,
				resource->grp_hour.x + resource->grp_hour.width - 1,
				resource->grp_hour.y + resource->grp_hour.height - 1);
		watch_clock_set_digit_images(data->obj_clock, CLOCK_HOUR, 10, data->img_tm_hour, &area);

		lv_area_set(&area, resource->grp_minute.x, resource->grp_minute.y,
				resource->grp_minute.x + resource->grp_minute.width - 1,
				resource->grp_minute.y + resource->grp_minute.height - 1);
		watch_clock_set_digit_images(data->obj_clock, CLOCK_MIN, 10, data->img_tm_min, &area);

		if (data->img_tm_colon.data) {
			watch_clock_set_separator_image(data->obj_clock,
					&data->img_tm_colon, resource->pt_colon.x, resource->pt_colon.y);
		}
	} else {
		lv_coord_t pivot_x, pivot_y;

		pivot_x = (data->scene.width / 2) - resource->grp_hour.x;
		pivot_y = (data->scene.height / 2) - resource->grp_hour.y;
		watch_clock_set_pointer_images(data->obj_clock, CLOCK_HOUR, 1, data->img_tm_hour, pivot_x, pivot_y);

		pivot_x = (data->scene.width / 2) - resource->grp_minute.x;
		pivot_y = (data->scene.height / 2) - resource->grp_minute.y;
		watch_clock_set_pointer_images(data->obj_clock, CLOCK_MIN, 1, data->img_tm_min, pivot_x, pivot_y);
	}

	app_mem_free(resource);

	ui_gesture_lock_scroll();

	/* initialize values */
	_aod_clock_view_paint(view_data, true);

	SYS_LOG_INF("AOD view inflated");

	g_aod_inflated = true;
	return 0;
}

static int _aod_clock_view_delete(view_data_t *view_data)
{
	aod_clock_view_data_t *data = view_data->user_data;
	const clock_view_presenter_t *presenter = view_get_presenter(view_data);

	if (data->obj_clock) {
		/* disable rtc */
		presenter->set_rtc(false, 0);

		lv_obj_delete(data->obj_clock);
		_aod_clock_view_unload_resource(data);
		ui_gesture_unlock_scroll();
	} else {
		lvgl_res_preload_cancel_scene(data->clock_dsc->scene);
	}

	lvgl_res_unload_scene_compact(data->clock_dsc->scene);

	app_mem_free(data);
	return 0;
}

static int _aod_clock_view_paint(view_data_t *view_data, bool first)
{
#ifdef CONFIG_RTC_ACTS
	const clock_dsc_t * clock_dsc = clocksel_get_aod_clock_dsc();
	const clock_view_presenter_t *presenter = view_get_presenter(view_data);
	aod_clock_view_data_t *data = view_data->user_data;
	struct rtc_time time;

	if (data->obj_clock == NULL) {
		goto out_exit;
	}

	/* update data via presenter*/
	presenter->get_time(&time);

	/* adjust rtc period if time diff greater than 1s */
	if (first || time.tm_sec > 1) {
		data->period = clock_dsc->period - time.tm_sec * 1000;
		presenter->set_rtc(true, data->period);
	} else if (data->period != clock_dsc->period) {
		data->period = clock_dsc->period;
		presenter->set_rtc(true, data->period);
	}

	SYS_LOG_INF("time %02u:%02u:%02u\n", time.tm_hour, time.tm_min, time.tm_sec);

	if (time.tm_hour == data->tm_hour && time.tm_min == data->tm_min) {
		goto out_exit;
	}

	data->tm_hour = time.tm_hour;
	data->tm_min = time.tm_min;

	watch_clock_set_time(data->obj_clock, time.tm_hour, time.tm_min, 0);

	/* refresh immediately */
	view_refresh(AOD_CLOCK_VIEW);
	view_wait_for_refresh(AOD_CLOCK_VIEW, -1);

out_exit:
	system_request_fast_standby();
#endif /* CONFIG_RTC_ACTS */

	return 0;
}

int _aod_clock_view_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void *msg_data)
{
	assert(view_id == AOD_CLOCK_VIEW);

	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return _aod_clock_view_preload(view_data);
	case MSG_VIEW_LAYOUT:
		return _aod_clock_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _aod_clock_view_delete(view_data);
	case MSG_VIEW_PAINT:
		return _aod_clock_view_paint(view_data, false);
	default:
		return 0;
	}
}

VIEW_DEFINE2(aod_clock, _aod_clock_view_handler, NULL, NULL, AOD_CLOCK_VIEW,
		HIGHEST_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);

int aod_clock_view_enter(void)
{
	extern const clock_view_presenter_t clock_view_presenter;
	const clock_dsc_t * clock_dsc = clocksel_get_aod_clock_dsc();
	int wait_ms = 2000;

	if (clock_dsc) {
		ui_switch_effect_set_type(UI_SWITCH_EFFECT_NONE);

		g_aod_inflated = false;
		view_stack_push_view(AOD_CLOCK_VIEW, &clock_view_presenter);

		while (g_aod_inflated == false && wait_ms > 0) {
			os_sleep(1);
			wait_ms--;
		}

		return 0;
	}

	return -ENOSYS;
}

int aod_clock_view_exit(void)
{
	int res = 0;

	if (g_aod_inflated) {
		res = view_stack_pop();
		ui_switch_effect_set_type(main_view_presenter.get_switch_mode());
	}

	return res;
}
