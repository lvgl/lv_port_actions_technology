/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <assert.h>
#include <app_ui.h>
#include "clock_selector/clock_selector.h"
#include "clock_view.h"
#include "widgets/simple_img.h"
#include "widgets/img_number.h"
#include "m_effect.h"
#include "ui_coder/ui_coder.h"

static const uint32_t electric_img_ids[] = {
	PIC_MIN10, PIC_MAX10
};

static const uint32_t green_num1_ids[] = {
	PIC_0, PIC_1, PIC_2, PIC_3, PIC_4, PIC_5, PIC_6, PIC_7, PIC_8, PIC_9
};

static const uint32_t green_num2_ids[] = {
	PIC_0, PIC_1, PIC_2, PIC_3, PIC_4, PIC_5, PIC_6, PIC_7, PIC_8, PIC_9
};

static const uint32_t day_num_ids[] = {
	PIC_0, PIC_1, PIC_2, PIC_3, PIC_4, PIC_5, PIC_6, PIC_7, PIC_8, PIC_9
};

static const uint32_t week_ids[] = {
	PIC_0, PIC_1, PIC_2, PIC_3, PIC_4, PIC_5, PIC_6
};

static const uint32_t white_num_ids[] = {
	PIC_0, PIC_1, PIC_2, PIC_3, PIC_4, PIC_5, PIC_6, PIC_7, PIC_8, PIC_9
};

static const uint32_t min_num_ids[] = {
	PIC_00, PIC_01, PIC_02, PIC_03, PIC_04, PIC_05, PIC_06, PIC_07, PIC_08, PIC_09,
	PIC_10, PIC_11, PIC_12, PIC_13
};

typedef struct digital1_clock_view_data {
	lv_image_dsc_t img_bg;
	lv_image_dsc_t electric_img[ARRAY_SIZE(electric_img_ids)];
	lv_image_dsc_t green_num1_img[ARRAY_SIZE(green_num1_ids)];
	lv_image_dsc_t green_num2_img[ARRAY_SIZE(green_num2_ids)];
	lv_image_dsc_t day_num_img[ARRAY_SIZE(day_num_ids)];
	lv_image_dsc_t week_img[ARRAY_SIZE(week_ids)];
	lv_image_dsc_t white_num_img[ARRAY_SIZE(white_num_ids)];
	lv_image_dsc_t min_num_img[ARRAY_SIZE(min_num_ids)];

	lv_obj_t *bg_icon;
	lv_obj_t *hour_tens;
	lv_obj_t *hour_units;
	lv_obj_t *minute_tens;
	lv_obj_t *minute_units;
	lv_obj_t *week_icon;
	lv_obj_t *day_icon;
	lv_obj_t *electric_img_icon;
	lv_obj_t *electric_num_icon;
	lv_obj_t *electric_bar_icon;
	lv_obj_t *calorie_icon;
	lv_obj_t *step_icon;
	lv_obj_t *distance_icon;
	lv_obj_t *arc_icon[3];
	lvgl_res_scene_t scene;

	/* click event */
	uint32_t pressing_start;
	bool preview_invoked;
} digital1_clock_view_data_t;

static int _digital1_clock_view_load_resource(digital1_clock_view_data_t *data)
{
	uint32_t pic_id;
	int ret;

	/* scene */
	ret = lvgl_res_load_scene(SCENE_DIGITAL1_CLOCK, &data->scene,
			DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (ret < 0) {
		SYS_LOG_ERR("scene 0x%x not found", SCENE_DIGITAL1_CLOCK);
		return -ENOENT;
	}

	pic_id = PIC_BJ;
	ret = lvgl_res_load_pictures_from_scene(&data->scene, &pic_id, &data->img_bg, NULL, 1);
	if (ret < 0) {
		goto out_exit;
	}

out_exit:
	return ret;
}

static void _digital1_clock_view_unload_resource(digital1_clock_view_data_t *data)
{
	lvgl_res_unload_pictures(&data->img_bg, 1);
	lvgl_res_unload_pictures(data->electric_img, ARRAY_SIZE(data->electric_img));
	lvgl_res_unload_pictures(data->green_num1_img, ARRAY_SIZE(data->green_num1_img));
	lvgl_res_unload_pictures(data->green_num2_img, ARRAY_SIZE(data->green_num2_img));
	lvgl_res_unload_pictures(data->day_num_img, ARRAY_SIZE(data->day_num_img));
	lvgl_res_unload_pictures(data->week_img, ARRAY_SIZE(data->week_img));
	lvgl_res_unload_pictures(data->white_num_img, ARRAY_SIZE(data->white_num_img));
	lvgl_res_unload_pictures(data->min_num_img, ARRAY_SIZE(data->min_num_img));
	lvgl_res_unload_scene(&data->scene);
}

static int _digital1_clock_view_preload(view_data_t *view_data)
{
	int ret;
	uint32_t bj_id = PIC_BJ;
	ret = lvgl_res_preload_scene_compact(SCENE_DIGITAL1_CLOCK, &bj_id, 1,
			lvgl_res_scene_preload_default_cb_for_view, (void *)CLOCK_VIEW,
			DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (ret) {
		return ret;
	}
	return 0;
}

static int _dynamic_load_pictures(lvgl_res_scene_t *scene,uint32_t id, const uint32_t *pic_ids,
		int images_num, lv_image_dsc_t *images,uint8_t *buf,int buf_cnt)
{
	int ret;
	for(int i = 0 ; i < images_num ; i++)
	{
		uint8_t sign = 0;
		for(int j = 0 ; j < buf_cnt ; j++)
		{
			if(i == buf[j])
			{
				sign = 1;
				break;
			}
		}
		if(sign)
		{
			if(images[i].data == NULL)
			{
				lvgl_res_group_t picture_group;
				ret = lvgl_res_load_group_from_scene(scene,id, &picture_group);
				if (ret < 0) {
					goto out_exit;
				}
				ret = lvgl_res_load_pictures_from_group(&picture_group, &pic_ids[i] ,&images[i], NULL, 1);
				if (ret < 0) {
					goto out_exit;
				}
				out_exit:
				lvgl_res_unload_group(&picture_group);
				if (ret < 0)
					return -ENOENT;
			}
		}
		else
		{
			if(images[i].data != NULL)
				lvgl_res_unload_pictures(&images[i], 1);
		}
	}
	
	return 0;
}

static int _digital1_clock_view_paint(view_data_t *view_data)
{
	digital1_clock_view_data_t *data = view_data->user_data;
	struct rtc_time time;

	if (data == NULL || data->bg_icon == NULL) {
		return 0;
	}

	if(data->scene.scene_data == NULL)
	{
		int ret;
		/* scene */
		ret = lvgl_res_load_scene(SCENE_DIGITAL1_CLOCK, &data->scene,
				DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
		if (ret < 0) {
			SYS_LOG_ERR("scene 0x%x not found", SCENE_DIGITAL1_CLOCK);
			return -ENOENT;
		}
	}

	/* update data via presenter*/
	clock_presenter->get_time(&time);

	uint8_t buf[30] = {0};
	uint32_t buf_cnt = 0;
	int32_t value;
	int32_t arc_num[ARRAY_SIZE(data->arc_icon)];
	int32_t arc_max_num[ARRAY_SIZE(data->arc_icon)] = {1000,5000,50000};
	time.tm_hour = 10;
	time.tm_min = 28;
	buf[0] = time.tm_hour/10;
	_dynamic_load_pictures(&data->scene,GREEN_NUM1,green_num1_ids,ARRAY_SIZE(data->green_num1_img),data->green_num1_img,buf,1);
	simple_img_set_src(data->hour_tens, &data->green_num1_img[buf[0]]);

	buf[0] = time.tm_hour%10;
	_dynamic_load_pictures(&data->scene,GREEN_NUM2,green_num2_ids,ARRAY_SIZE(data->green_num2_img),data->green_num2_img,buf,1);
	simple_img_set_src(data->hour_units, &data->green_num2_img[buf[0]]);

	buf_cnt += split_decimal(time.tm_min, &buf[buf_cnt], sizeof(buf)-buf_cnt, 2);
	_dynamic_load_pictures(&data->scene,WHITE_NUM,white_num_ids,ARRAY_SIZE(data->white_num_img),data->white_num_img,buf,buf_cnt);
	simple_img_set_src(data->minute_tens, &data->white_num_img[buf[0]]);
	simple_img_set_src(data->minute_units, &data->white_num_img[buf[1]]);

	buf[0] = time.tm_wday;
	_dynamic_load_pictures(&data->scene,WEEK,week_ids,ARRAY_SIZE(data->week_img),data->week_img,buf,1);
	simple_img_set_src(data->week_icon, &data->week_img[buf[0]]);

	buf_cnt = 0;
	buf_cnt += split_decimal(time.tm_mday, &buf[buf_cnt], sizeof(buf)-buf_cnt, 0);
	_dynamic_load_pictures(&data->scene,DAY_NUM,day_num_ids,ARRAY_SIZE(data->day_num_img),data->day_num_img,buf,buf_cnt);
	img_label_set_text(data->day_icon, buf ,buf_cnt);

	value = clock_presenter->get_battery_percent();
	if(value > 10)
	{
		lv_obj_set_width(data->electric_bar_icon,value * 29 / 100);
		buf[0] = 1;
	}
	else
	{
		lv_obj_set_width(data->electric_bar_icon,0);
		buf[0] = 0;
	}
	_dynamic_load_pictures(&data->scene,ELECTRIC,electric_img_ids,ARRAY_SIZE(data->electric_img),data->electric_img,buf,1);
	simple_img_set_src(data->electric_img_icon, &data->electric_img[buf[0]]);

	buf_cnt = 0;
	uint8_t *electric_buf = &buf[buf_cnt];
	uint32_t electric_cnt = 0;
	electric_cnt += split_decimal(value, &buf[buf_cnt], sizeof(buf)-buf_cnt, 0);
	buf_cnt += electric_cnt;
	buf[buf_cnt++] = 13;
	electric_cnt++;

	value = clock_presenter->get_calories();
	arc_num[0] = value;
	uint8_t *calorie_buf = &buf[buf_cnt];
	uint32_t calorie_cnt = 0;
	calorie_cnt += split_decimal(value, &buf[buf_cnt], sizeof(buf) - buf_cnt, 0);
	buf_cnt += calorie_cnt;

	value = clock_presenter->get_step_count();
	arc_num[1] = value;
	uint8_t *step_buf = &buf[buf_cnt];
	uint32_t step_cnt = buf_cnt;
	buf_cnt += split_decimal(value, &buf[buf_cnt], sizeof(buf)-buf_cnt, 0);
	step_cnt = buf_cnt - step_cnt;

	arc_num[2] = clock_presenter->get_distance();
	value = arc_num[2]/10;
	uint8_t *distance_buf = &buf[buf_cnt];
	uint32_t distance_cnt = buf_cnt;
	buf_cnt += split_decimal(value / 100, &buf[buf_cnt], sizeof(buf) - buf_cnt, 0);
	if(value % 100)
	{
		buf[buf_cnt++] = 10;
		buf_cnt += split_decimal(value % 100, &buf[buf_cnt], sizeof(buf) - buf_cnt, 2);
	}
	buf[buf_cnt++] = 11;
	distance_cnt = buf_cnt - distance_cnt;

	_dynamic_load_pictures(&data->scene,MIN_NUM,min_num_ids,ARRAY_SIZE(data->min_num_img),data->min_num_img,buf,buf_cnt);
	img_label_set_text(data->electric_num_icon, electric_buf, electric_cnt);
	lv_obj_align_to(data->electric_num_icon,data->electric_img_icon,LV_ALIGN_OUT_RIGHT_MID,8,0);
	img_label_set_text(data->calorie_icon, calorie_buf, calorie_cnt);
	lv_obj_update_layout(data->calorie_icon);
	lv_obj_set_pos(data->calorie_icon,158 - (lv_obj_get_width(data->calorie_icon)>>1),400);
	img_label_set_text(data->step_icon, step_buf, step_cnt);
	lv_obj_update_layout(data->step_icon);
	lv_obj_set_pos(data->step_icon,234 - (lv_obj_get_width(data->step_icon)>>1),400);
	img_label_set_text(data->distance_icon, distance_buf, distance_cnt);
	lv_obj_update_layout(data->distance_icon);
	lv_obj_set_pos(data->distance_icon,310 - (lv_obj_get_width(data->distance_icon)>>1),400);
	for(uint8_t i = 0 ; i < ARRAY_SIZE(data->arc_icon) ; i++)
	{
		if(arc_num[i] > arc_max_num[i])
			arc_num[i] = arc_max_num[i];
		lv_arc_set_range(data->arc_icon[i],0,1000);
		lv_arc_set_value(data->arc_icon[i], arc_num[i] * 1000 / arc_max_num[i]);
	}

	//lv_refr_now(view_data->display);
	return 0;
}

static void _digital1_clock_event_handler(lv_event_t * e)
{
	lv_event_code_t event = lv_event_get_code(e);
	view_data_t *view_data = lv_event_get_user_data(e);
	digital1_clock_view_data_t *data = view_data->user_data;
	uint32_t pressing_duration = 0;
	if (data->preview_invoked)
		return;

	if (event == LV_EVENT_PRESSED) {
		data->pressing_start = os_uptime_get_32();
	} else if (event == LV_EVENT_RELEASED || event == LV_EVENT_PRESSING) {
		pressing_duration = os_uptime_get_32() - data->pressing_start;
	}

	if (pressing_duration > CLOCK_SWITCH_PRESSING_TIME) {
		if (clock_presenter->invoke_preview) {
			data->preview_invoked = clock_presenter->invoke_preview() ? false : true;
		}
	}
}

static void _digital1_clock_btn_event_handler(lv_event_t * e)
{
	if(lvgl_click_decision())
	{
		go_to_m_effect();
		lv_obj_remove_event_cb(lv_event_get_user_data(e),_digital1_clock_btn_event_handler);
	}
}

static int _digital1_clock_view_layout(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	digital1_clock_view_data_t *data = view_data->user_data;
	if(data == NULL)
	{
		data = app_mem_malloc(sizeof(*data));
		if (data == NULL) {
			return -ENOMEM;
		}
		memset(data, 0, sizeof(*data));
		view_data->user_data = data;
	}

	if (_digital1_clock_view_load_resource(data)) {
		return -ENOENT;
	}
	coder_simulation_tp_register(scr, CLOCK_VIEW, LV_DIR_HOR, 50, NULL);

	data->bg_icon = simple_img_create(scr);
	lv_obj_set_pos(data->bg_icon,0,0);
	simple_img_set_src(data->bg_icon, &data->img_bg);
	lv_obj_add_flag(data->bg_icon,LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_event_cb(data->bg_icon, _digital1_clock_event_handler, LV_EVENT_ALL, view_data);
	lv_obj_add_event_cb(data->bg_icon, _digital1_clock_btn_event_handler, LV_EVENT_SHORT_CLICKED, data->bg_icon);


	data->hour_tens = simple_img_create(data->bg_icon);
	lv_obj_set_pos(data->hour_tens,182,94);

	data->hour_units = simple_img_create(data->bg_icon);
	lv_obj_set_pos(data->hour_units,282,94);

	data->minute_tens = simple_img_create(data->bg_icon);
	lv_obj_set_pos(data->minute_tens,142,190);

	data->minute_units = simple_img_create(data->bg_icon);
	lv_obj_set_pos(data->minute_units,242,190);

	data->week_icon = simple_img_create(data->bg_icon);
	lv_obj_set_pos(data->week_icon,90,90);

	data->day_icon = img_label_create(data->bg_icon);
	lv_obj_set_pos(data->day_icon, 160, 90);
	img_label_set_align(data->day_icon, LV_ALIGN_RIGHT_MID);
	img_label_set_src(data->day_icon, data->day_num_img, ARRAY_SIZE(data->day_num_img));

	data->electric_img_icon = simple_img_create(data->bg_icon);
	lv_obj_set_pos(data->electric_img_icon,270,56);

	data->electric_bar_icon = lv_obj_create(data->electric_img_icon);

	lv_obj_set_pos(data->electric_bar_icon,4,4);
	lv_obj_set_size(data->electric_bar_icon,0,14);
	lv_obj_set_style_bg_color(data->electric_bar_icon,lv_color_white(),LV_PART_MAIN);
	lv_obj_set_style_bg_opa(data->electric_bar_icon,LV_OPA_100,LV_PART_MAIN);

	data->electric_num_icon = img_label_create(data->bg_icon);
	img_label_set_align(data->electric_num_icon, LV_ALIGN_LEFT_MID);
	img_label_set_src(data->electric_num_icon, data->min_num_img, ARRAY_SIZE(data->min_num_img));

	data->calorie_icon = img_label_create(data->bg_icon);
	img_label_set_align(data->calorie_icon, LV_ALIGN_CENTER);
	img_label_set_src(data->calorie_icon, data->min_num_img, ARRAY_SIZE(data->min_num_img));

	data->step_icon = img_label_create(data->bg_icon);
	img_label_set_align(data->step_icon, LV_ALIGN_CENTER);
	img_label_set_src(data->step_icon, data->min_num_img, ARRAY_SIZE(data->min_num_img));

	data->distance_icon = img_label_create(data->bg_icon);
	img_label_set_align(data->distance_icon, LV_ALIGN_CENTER);
	img_label_set_src(data->distance_icon, data->min_num_img, ARRAY_SIZE(data->min_num_img));

	for(uint8_t i = 0 ; i < ARRAY_SIZE(data->arc_icon) ; i++)
	{
		data->arc_icon[i] = lv_arc_create(data->bg_icon);
		lv_obj_set_pos(data->arc_icon[i], 135 + i * 76, 346);
		lv_obj_set_size(data->arc_icon[i], 44, 44);
		lv_arc_set_rotation(data->arc_icon[i], 270);
		lv_arc_set_bg_angles(data->arc_icon[i], 0, 360);
		lv_obj_set_style_arc_width(data->arc_icon[i], 2, LV_PART_INDICATOR);
		lv_obj_set_style_arc_color(data->arc_icon[i], lv_color_hex(0xe2ff3d), LV_PART_INDICATOR);
		lv_obj_set_style_opa(data->arc_icon[i], LV_OPA_100, LV_PART_INDICATOR);
	}

	_digital1_clock_view_paint(view_data);

	SYS_LOG_INF("digital1 view inflated");
	return 0;
}

static int _digital1_clock_view_focus_changed(view_data_t *view_data, bool focused)
{
	/* data is allocate in preload() */
	digital1_clock_view_data_t *data = view_data->user_data;

	if(focused)
	{
		const clock_dsc_t * clock_dsc = clocksel_get_clock_dsc(clock_presenter->get_clock_id());
		if (clock_dsc) {
			clock_presenter->set_rtc(focused, clock_dsc->period);
		}
	}

	if (focused) {
		if (data) {
			_digital1_clock_view_preload(view_data);
			_digital1_clock_view_load_resource(data);
			_digital1_clock_view_paint(view_data);
		}
	} else {
		if(data)
		{
			_digital1_clock_view_unload_resource(data);
		}
		lvgl_res_preload_cancel_scene(SCENE_DIGITAL1_CLOCK);
		lvgl_res_unload_scene_compact(SCENE_DIGITAL1_CLOCK);
	}

	return 0;
}

static int _digital1_clock_view_delete(view_data_t *view_data)
{
	digital1_clock_view_data_t *data = view_data->user_data;
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	lv_obj_clean(scr);

	if(data)
	{
		_digital1_clock_view_unload_resource(data);
	}
	lvgl_res_preload_cancel_scene(SCENE_DIGITAL1_CLOCK);
	lvgl_res_unload_scene_compact(SCENE_DIGITAL1_CLOCK);

	if (data) {
		app_mem_free(data);
	}

	return 0;
}

int _digital1_clock_view_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void *msg_data)
{
	SYS_LOG_ERR("_digital1_clock_view_handler %d",msg_id);

	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return _digital1_clock_view_preload(view_data);
	case MSG_VIEW_LAYOUT:
		return _digital1_clock_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _digital1_clock_view_delete(view_data);
	case MSG_VIEW_PAINT:
	case MSG_VIEW_RESUME_DISPLAY:
		return _digital1_clock_view_paint(view_data);
	case MSG_VIEW_FOCUS:
		return _digital1_clock_view_focus_changed(view_data, true);
	case MSG_VIEW_DEFOCUS:
		return _digital1_clock_view_focus_changed(view_data, false);
	case MSG_VIEW_UPDATE:
		return _digital1_clock_view_paint(view_data);
	default:
		return 0;
	}
}

