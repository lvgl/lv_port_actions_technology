/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <app_ui.h>
#include "clock_selector/clock_selector.h"
#include "widgets/watch_clock.h"
#include "widgets/simple_img.h"
#include "widgets/anim_img.h"
#include "widgets/img_number.h"
#include "clock_view.h"
#include "m_effect.h"
#include "ui_coder/ui_coder.h"

//LOG_MODULE_DECLARE(clock);

enum {
	INFO_STEPCOUNT = 0,
	INFO_HEARTRATE,
	INFO_CALORIES,
	INFO_DISTANCE,
	INFO_SLEEPTIME,

	INFO_BATTERY,

	INFO_TEMPERATURE,
	INFO_WEATHER,

	INFO_ANIM,
	INFO_DATE,
	INFO_LUNAR,

	NUM_INFO,
};

typedef struct clock_view_tmp_res {
	lvgl_res_group_t grp_hour;
	lvgl_res_group_t grp_minute;
	lvgl_res_group_t grp_second;
	lvgl_res_group_t grp_stepcount;
	lvgl_res_group_t grp_heartrate;
	lvgl_res_group_t grp_calories;
	lvgl_res_group_t grp_distance;
	lvgl_res_group_t grp_sleeptime;
	lvgl_res_group_t grp_battery;
	lvgl_res_group_t grp_temperature;

	lv_point_t pt_colon;
} clock_view_tmp_res_t;

typedef struct clock_view_data {
	bool is_digital;
	uint16_t period;

	/* previous value */
	uint8_t weather;
	uint8_t tm_mday;
	uint8_t tm_mon;
	uint16_t tm_year;
	uint32_t distance;
	uint32_t sleep_time;

	lv_obj_t *obj_clock;
	lv_obj_t *obj_info[NUM_INFO];

	lv_font_t font;
	lv_style_t sty_text[2];

	/* lvgl resource */
	lv_image_dsc_t img_bg;
	lv_image_dsc_t img_tm_colon; /* : between hour and minute */
	lv_image_dsc_t *img_tm_hour; /* 0~9 */
	lv_image_dsc_t *img_tm_min;  /* 0~9 */
	lv_image_dsc_t *img_tm_sec;  /* 0~9 */

	lv_image_dsc_t *img_stepcount; /* 0~9 */
	lv_image_dsc_t *img_heartrate; /* 0~9 */
	lv_image_dsc_t *img_calories;  /* 0~9 */
	lv_image_dsc_t *img_distance;  /* 0~9 . k m */
	lv_image_dsc_t *img_sleeptime; /* 0~9 h m */
	lv_image_dsc_t *img_battery;   /* 0~9 */
	lv_image_dsc_t *img_temperature; /* 0~9 */
	lv_image_dsc_t img_weather;

	lvgl_res_string_t res_str[2];
	lvgl_res_picregion_t prg_anim;
	lvgl_res_group_t grp_weather;
	lvgl_res_scene_t scene;

	/* click event */
	uint32_t pressing_start;
	bool preview_invoked;
} clock_view_data_t;

static int _clock_view_paint(view_data_t *view_data, bool forced);

static const uint32_t preload_ids[] = {
	PIC_BACKGROUND, PIC_TM_COLON, RES_TM_HOUR, RES_TM_MINUTE, RES_TM_SECOND,
	RES_STEP_COUNT, RES_HEART_RATE, RES_CALORIES, RES_BATTERY, RES_TEMPERATURE,
	RES_DISTANCE, RES_SLEEP_TIME,
};

static const uint32_t pic_common_ids[] = {
	PIC_0, PIC_1, PIC_2, PIC_3, PIC_4, PIC_5, PIC_6, PIC_7, PIC_8, PIC_9,
};

static const uint32_t pic_weather_ids[] = {
	PIC_SUNNY, PIC_CLOUDY, PIC_HAIL, PIC_SANDSTORM, PIC_FOG, PIC_HEAVYRAIN,
	PIC_HEAVYSNOW, PIC_THUNDERSTORM, PIC_CLEAR_TO_OVERCAST,
};

static const uint32_t pic_distance_ids[] = {
	PIC_0, PIC_1, PIC_2, PIC_3, PIC_4, PIC_5, PIC_6, PIC_7, PIC_8, PIC_9, PIC_DOT, PIC_K, PIC_M,
};

static const uint32_t pic_sleeptime_ids[] = {
	PIC_0, PIC_1, PIC_2, PIC_3, PIC_4, PIC_5, PIC_6, PIC_7, PIC_8, PIC_9, PIC_H, PIC_M,
};

static const uint32_t str_ids[] = { STR_DATE, STR_LUNAR };

static const char * txt_wday[7] = {
	//"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
	"周日", "周一", "周二", "周三", "周四", "周五", "周六"
};

static uint8_t clock_preload_inited = 0;
static const clock_dsc_t * sp_clock_dsc = NULL;
const clock_view_presenter_t *clock_presenter = NULL;

static int _load_pictures(lvgl_res_scene_t *scene, uint32_t group_id,
		lvgl_res_group_t *group, const uint32_t *pic_ids,
		int num_ids, lv_image_dsc_t **images, bool first_layout)
{
	if (first_layout) {
		*images = app_mem_malloc(num_ids * sizeof(lv_image_dsc_t));
		if (*images == NULL) {
			return -ENOMEM;
		}
	}

	if (lvgl_res_load_group_from_scene(scene, group_id, group)) {
		goto fail_free_image;
	}

	if (lvgl_res_load_pictures_from_group(group, pic_ids, *images, NULL, num_ids)) {
		lvgl_res_unload_group(group);
		goto fail_free_image;
	}

	lvgl_res_unload_group(group);
	return 0;
fail_free_image:
	if (first_layout) {
		app_mem_free(*images);
		*images = NULL;
	}

	return -ENOENT;
}

static void _unload_pictures(lv_image_dsc_t **images, int num_images, bool deleting)
{
	if (*images) {
		lvgl_res_unload_pictures(*images, num_images);
		if (deleting) {
			app_mem_free(*images);
			*images = NULL;
		}
	}
}

static int _clock_view_load_resource(clock_view_data_t *data, clock_view_tmp_res_t *tmp_res, bool first_layout)
{
	uint32_t pic_id;
	int ret;

	/* scene */
	ret = lvgl_res_load_scene(sp_clock_dsc->scene, &data->scene,
			DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (ret < 0) {
		SYS_LOG_ERR("scene 0x%x not found", sp_clock_dsc->scene);
		return -ENOENT;
	}

	if (first_layout) {
		//test scene is digital or not
		ret = lvgl_res_load_group_from_scene(&data->scene, RES_DIGITAL_ID, &tmp_res->grp_hour);
		if (ret == 0) {
			data->is_digital = true;
			lvgl_res_unload_group(&tmp_res->grp_hour);
		}
	}

	/* background picture */
	pic_id = PIC_BACKGROUND;
	ret = lvgl_res_load_pictures_from_scene(&data->scene, &pic_id, &data->img_bg, NULL, 1);

	/* hour */
	ret = _load_pictures(&data->scene, RES_TM_HOUR, &tmp_res->grp_hour,
			pic_common_ids, data->is_digital ? 10 : 1, &data->img_tm_hour, first_layout);
	if (ret < 0) {
		return -ENOENT;
	}

	/* minute */
	ret = _load_pictures(&data->scene, RES_TM_MINUTE, &tmp_res->grp_minute,
			pic_common_ids, data->is_digital ? 10 : 1, &data->img_tm_min, first_layout);
	if (ret < 0) {
		return -ENOENT;
	}

	/* second */
	ret = _load_pictures(&data->scene, RES_TM_SECOND, &tmp_res->grp_second,
			pic_common_ids, data->is_digital ? 10 : 1, &data->img_tm_sec, first_layout);

	/* time colon */
	pic_id = PIC_TM_COLON;
	lvgl_res_load_pictures_from_scene(&data->scene, &pic_id,
			&data->img_tm_colon, &tmp_res->pt_colon, 1);

	/* step count */
	_load_pictures(&data->scene, RES_STEP_COUNT, &tmp_res->grp_stepcount,
			pic_common_ids, 10, &data->img_stepcount, first_layout);
	/* heart rate */
	_load_pictures(&data->scene, RES_HEART_RATE, &tmp_res->grp_heartrate,
			pic_common_ids, 10, &data->img_heartrate, first_layout);
	/* calories */
	_load_pictures(&data->scene, RES_CALORIES, &tmp_res->grp_calories,
			pic_common_ids, 10, &data->img_calories, first_layout);
	/* distance */
	_load_pictures(&data->scene, RES_DISTANCE, &tmp_res->grp_distance,
			pic_distance_ids, ARRAY_SIZE(pic_distance_ids), &data->img_distance, first_layout);
	/* sleep time */
	_load_pictures(&data->scene, RES_SLEEP_TIME, &tmp_res->grp_sleeptime,
			pic_sleeptime_ids, ARRAY_SIZE(pic_sleeptime_ids), &data->img_sleeptime, first_layout);
	/* battery */
	_load_pictures(&data->scene, RES_BATTERY, &tmp_res->grp_battery,
			pic_common_ids, 10, &data->img_battery, first_layout);
	/* temperature */
	_load_pictures(&data->scene, RES_TEMPERATURE, &tmp_res->grp_temperature,
			pic_common_ids, 10, &data->img_temperature, first_layout);

	/* weather */
	lvgl_res_load_group_from_scene(&data->scene, RES_WEATHER, &data->grp_weather);
	/* animation */
	lvgl_res_load_picregion_from_scene(&data->scene, RES_ANIMATION, &data->prg_anim);
	/* date, weak and lunar */
	lvgl_res_load_strings_from_scene(&data->scene, str_ids, data->res_str, ARRAY_SIZE(str_ids));

	if (first_layout) {
		if (data->res_str[0].width > 0) {
			if (LVGL_FONT_OPEN_DEFAULT(&data->font, DEF_FONT_SIZE_SMALL) < 0) {
				return -ENOENT;
			}
		}
	}
	return 0;
}

static void _clock_view_unload_resource(clock_view_data_t *data, bool deleting)
{
	lvgl_res_preload_cancel_scene(sp_clock_dsc->scene);
	lvgl_res_unload_scene_compact(sp_clock_dsc->scene);

	if (data) {
		if (deleting) {
			if (data->res_str[0].width > 0) {
				LVGL_FONT_CLOSE(&data->font);
			}
		}
		_unload_pictures(&data->img_tm_hour, data->is_digital ? 10 : 1, deleting);
		_unload_pictures(&data->img_tm_min, data->is_digital ? 10 : 1, deleting);
		_unload_pictures(&data->img_tm_sec, data->is_digital ? 10 : 1, deleting);
		_unload_pictures(&data->img_stepcount, 10, deleting);
		_unload_pictures(&data->img_heartrate, 10, deleting);
		_unload_pictures(&data->img_calories, 10, deleting);
		_unload_pictures(&data->img_distance, ARRAY_SIZE(pic_distance_ids), deleting);
		_unload_pictures(&data->img_sleeptime, ARRAY_SIZE(pic_sleeptime_ids), deleting);
		_unload_pictures(&data->img_battery, 10, deleting);
		_unload_pictures(&data->img_temperature, 10, deleting);

		lvgl_res_unload_pictures(&data->img_bg, 1);
		lvgl_res_unload_pictures(&data->img_tm_colon, 1);
		lvgl_res_unload_pictures(&data->img_weather, 1);

		lvgl_res_unload_group(&data->grp_weather);
		lvgl_res_unload_picregion(&data->prg_anim);
		lvgl_res_unload_strings(data->res_str, ARRAY_SIZE(data->res_str));
		lvgl_res_unload_scene(&data->scene);
	}
}

static int _clock_view_preload(view_data_t *view_data, bool update)
{
	const clock_view_presenter_t *presenter = view_get_presenter(view_data);
	//uint32_t regular_id[2] = {PIC_BACKGROUND, PIC_TM_COLON};

	if (clock_preload_inited == 0) {
		lvgl_res_preload_scene_compact_default_init(SCENE_CLOCK_VIEW, preload_ids, ARRAY_SIZE(preload_ids),
			NULL, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
		lvgl_res_preload_scene_compact_default_init(SCENE_DIGITAL_CLOCK_VIEW, preload_ids, ARRAY_SIZE(preload_ids),
			NULL, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
#ifdef SCENE_AOD_CLOCK_VIEW
		lvgl_res_preload_scene_compact_default_init(SCENE_AOD_CLOCK_VIEW, preload_ids, ARRAY_SIZE(preload_ids),
			NULL, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
#endif
		clock_preload_inited = 1;
	}

	sp_clock_dsc = clocksel_get_clock_dsc(presenter->get_clock_id());
	if(!sp_clock_dsc)
		return -1;
	return lvgl_res_preload_scene_compact_default(sp_clock_dsc->scene, CLOCK_VIEW, update, 0);
}

static void _clock_event_handler(lv_event_t * e)
{
	lv_event_code_t event = lv_event_get_code(e);
	view_data_t *view_data = lv_event_get_user_data(e);
	clock_view_data_t *data = view_data->user_data;
	const clock_view_presenter_t *presenter = view_get_presenter(view_data);
	uint32_t pressing_duration = 0;

	if (data->preview_invoked)
		return;

	if (event == LV_EVENT_PRESSED) {
		data->pressing_start = os_uptime_get_32();
	} else if (event == LV_EVENT_RELEASED || event == LV_EVENT_PRESSING) {
		pressing_duration = os_uptime_get_32() - data->pressing_start;
	}

	if (pressing_duration > CLOCK_SWITCH_PRESSING_TIME) {
		if (presenter->invoke_preview) {
			data->preview_invoked = presenter->invoke_preview() ? false : true;
		}
	}
}

static void _clock_btn_event_handler(lv_event_t * e)
{
	if(lvgl_click_decision())
	{
		go_to_m_effect();
		lv_obj_remove_event_cb(lv_event_get_user_data(e),_clock_btn_event_handler);
	}
}

static int _clock_view_layout_update(view_data_t *view_data, bool first_layout)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	clock_view_data_t *data = view_data->user_data;
	clock_view_tmp_res_t *resource;
	int i;

	if (first_layout) {
		data = app_mem_malloc(sizeof(*data));
		if (data == NULL) {
			return -ENOMEM;
		}

		memset(data, 0, sizeof(*data));
		data->period = sp_clock_dsc->period;
		view_data->user_data = data;
	}

	resource = app_mem_malloc(sizeof(*resource));
	if (!resource) {
		SYS_LOG_ERR("clock_view malloc tmp resource failed\n");
		if (first_layout) app_mem_free(data);
		view_data->user_data = NULL;
		return -ENOMEM;
	}

	if (_clock_view_load_resource(data, resource, first_layout)) {
		SYS_LOG_ERR("load resource failed\n");
		if (first_layout) app_mem_free(data);
		app_mem_free(resource);
		view_data->user_data = NULL;
		return -ENOENT;
	}

	if (first_layout == false) {
		lv_image_cache_drop(NULL);
		goto out_exit;
	}
	coder_simulation_tp_register(scr, CLOCK_VIEW, LV_DIR_HOR, 50, NULL);

	data->obj_clock = watch_clock_create(scr);
	watch_clock_set_type(data->obj_clock,
			data->is_digital ? DIGITAL_CLOCK : ANALOG_CLOCK,
			data->img_tm_sec ? true : false);
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
	lv_obj_set_user_data(data->obj_clock, view_data);
	lv_obj_add_event_cb(data->obj_clock, _clock_event_handler, LV_EVENT_ALL, view_data);
	lv_obj_add_event_cb(data->obj_clock, _clock_btn_event_handler, LV_EVENT_SHORT_CLICKED, data->obj_clock);

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

		if (data->img_tm_sec) {
			lv_area_set(&area, resource->grp_second.x, resource->grp_second.y,
					resource->grp_second.x + resource->grp_second.width - 1,
					resource->grp_second.y + resource->grp_second.height - 1);
			watch_clock_set_digit_images(data->obj_clock, CLOCK_SEC, 10, data->img_tm_sec, &area);
		}

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

		if (data->img_tm_sec) {
			pivot_x = (data->scene.width / 2) - resource->grp_second.x;
			pivot_y = (data->scene.height / 2) - resource->grp_second.y;
			watch_clock_set_pointer_images(data->obj_clock, CLOCK_SEC, 1, data->img_tm_sec, pivot_x, pivot_y);
		}
	}

	if (data->img_stepcount) {
		data->obj_info[INFO_STEPCOUNT] = img_number_create(data->obj_clock);
		if (data->obj_info[INFO_STEPCOUNT]) {
			lv_obj_set_pos(data->obj_info[INFO_STEPCOUNT], resource->grp_stepcount.x, resource->grp_stepcount.y);
			lv_obj_set_size(data->obj_info[INFO_STEPCOUNT], resource->grp_stepcount.width, resource->grp_stepcount.height);
			img_number_set_src(data->obj_info[INFO_STEPCOUNT], data->img_stepcount, 10);
		}
	}

	if (data->img_heartrate) {
		data->obj_info[INFO_HEARTRATE] = img_number_create(data->obj_clock);
		if (data->obj_info[INFO_HEARTRATE]) {
			lv_obj_set_pos(data->obj_info[INFO_HEARTRATE], resource->grp_heartrate.x, resource->grp_heartrate.y);
			lv_obj_set_size(data->obj_info[INFO_HEARTRATE], resource->grp_heartrate.width, resource->grp_heartrate.height);
			img_number_set_src(data->obj_info[INFO_HEARTRATE], data->img_heartrate, 10);
		}
	}

	if (data->img_calories) {
		data->obj_info[INFO_CALORIES] = img_number_create(data->obj_clock);
		if (data->obj_info[INFO_CALORIES]) {
			lv_obj_set_pos(data->obj_info[INFO_CALORIES], resource->grp_calories.x, resource->grp_calories.y);
			lv_obj_set_size(data->obj_info[INFO_CALORIES], resource->grp_calories.width, resource->grp_calories.height);
			img_number_set_src(data->obj_info[INFO_CALORIES], data->img_calories, 10);
		}
	}

	if (data->img_battery) {
		data->obj_info[INFO_BATTERY] = img_number_create(data->obj_clock);
		if (data->obj_info[INFO_BATTERY]) {
			lv_obj_set_pos(data->obj_info[INFO_BATTERY], resource->grp_battery.x, resource->grp_battery.y);
			lv_obj_set_size(data->obj_info[INFO_BATTERY], resource->grp_battery.width, resource->grp_battery.height);
			img_number_set_src(data->obj_info[INFO_BATTERY], data->img_battery, 10);
		}
	}

	if (data->img_temperature) {
		data->obj_info[INFO_TEMPERATURE] = img_number_create(data->obj_clock);
		if (data->obj_info[INFO_TEMPERATURE]) {
			lv_obj_set_pos(data->obj_info[INFO_TEMPERATURE], resource->grp_temperature.x, resource->grp_temperature.y);
			lv_obj_set_size(data->obj_info[INFO_TEMPERATURE], resource->grp_temperature.width, resource->grp_temperature.height);
			img_number_set_src(data->obj_info[INFO_TEMPERATURE], data->img_temperature, 10);
		}
	}

	if (data->img_distance) {
		data->obj_info[INFO_DISTANCE] = img_label_create(data->obj_clock);
		if (data->obj_info[INFO_DISTANCE]) {
			lv_obj_set_pos(data->obj_info[INFO_DISTANCE], resource->grp_distance.x, resource->grp_distance.y);
			lv_obj_set_size(data->obj_info[INFO_DISTANCE], resource->grp_distance.width, resource->grp_distance.height);
			img_label_set_align(data->obj_info[INFO_DISTANCE], LV_ALIGN_RIGHT_MID);
			img_label_set_src(data->obj_info[INFO_DISTANCE], data->img_distance, ARRAY_SIZE(pic_distance_ids));
		}
	}

	if (data->img_sleeptime) {
		data->obj_info[INFO_SLEEPTIME] = img_label_create(data->obj_clock);
		if (data->obj_info[INFO_SLEEPTIME]) {
			lv_obj_set_pos(data->obj_info[INFO_SLEEPTIME], resource->grp_sleeptime.x, resource->grp_sleeptime.y);
			lv_obj_set_size(data->obj_info[INFO_SLEEPTIME], resource->grp_sleeptime.width, resource->grp_sleeptime.height);
			img_label_set_align(data->obj_info[INFO_SLEEPTIME], LV_ALIGN_LEFT_MID);
			img_label_set_src(data->obj_info[INFO_SLEEPTIME], data->img_sleeptime, ARRAY_SIZE(pic_sleeptime_ids));
		}
	}

	if (data->grp_weather.width > 0) {
		data->obj_info[INFO_WEATHER] = anim_img_create(data->obj_clock);
		if (data->obj_info[INFO_WEATHER]) {
			lv_obj_set_pos(data->obj_info[INFO_WEATHER], data->grp_weather.x, data->grp_weather.y);
			lv_obj_set_size(data->obj_info[INFO_WEATHER], data->grp_weather.width, data->grp_weather.height);
			anim_img_set_preload(data->obj_info[INFO_WEATHER], false);
			anim_img_set_src_picgroup(data->obj_info[INFO_WEATHER], sp_clock_dsc->scene, &data->grp_weather,
					pic_weather_ids, ARRAY_SIZE(pic_weather_ids));
		}
	}

	if (data->prg_anim.width > 0) {
		data->obj_info[INFO_ANIM] = anim_img_create(data->obj_clock);
		if (data->obj_info[INFO_ANIM]) {
			lv_obj_set_pos(data->obj_info[INFO_ANIM], data->prg_anim.x, data->prg_anim.y);
			lv_obj_set_size(data->obj_info[INFO_ANIM], data->prg_anim.width, data->prg_anim.height);
			anim_img_set_src_picregion(data->obj_info[INFO_ANIM], sp_clock_dsc->scene, &data->prg_anim);
			anim_img_set_duration(data->obj_info[INFO_ANIM], 16, 64 * data->prg_anim.frames);
			anim_img_set_repeat(data->obj_info[INFO_ANIM], 0, LV_ANIM_REPEAT_INFINITE);
		}
	}

	for (i = 0; i < ARRAY_SIZE(data->sty_text); i++) {
		lv_style_init(&data->sty_text[i]);
		lv_style_set_text_font(&data->sty_text[i], &data->font);
		lv_style_set_text_color(&data->sty_text[i], data->res_str[i].color);
		lv_style_set_text_align(&data->sty_text[i], LV_TEXT_ALIGN_LEFT);
		lv_style_set_bg_opa(&data->sty_text[i], LV_OPA_TRANSP);
	}

	if (data->res_str[0].width > 0) {
		data->obj_info[INFO_DATE] = lv_label_create(data->obj_clock);
		if (data->obj_info[INFO_DATE]) {
			lv_obj_set_pos(data->obj_info[INFO_DATE], data->res_str[0].x, data->res_str[0].y);
			lv_obj_set_height(data->obj_info[INFO_DATE], data->res_str[0].height);
			lv_obj_add_style(data->obj_info[INFO_DATE], &data->sty_text[0], LV_PART_MAIN);
			lv_label_set_text(data->obj_info[INFO_DATE], "周五 05-12");
		}
	}

	if (data->res_str[1].width > 0) {
		data->obj_info[INFO_LUNAR] = lv_label_create(data->obj_clock);
		if (data->obj_info[INFO_LUNAR]) {
			lv_obj_set_pos(data->obj_info[INFO_LUNAR], data->res_str[1].x, data->res_str[1].y);
			lv_obj_set_height(data->obj_info[INFO_LUNAR], data->res_str[1].height);
			lv_obj_add_style(data->obj_info[INFO_LUNAR], &data->sty_text[1], LV_PART_MAIN);
			lv_label_set_text(data->obj_info[INFO_LUNAR], "正月初五");
		}
	}

	for (i = 0; i < ARRAY_SIZE(data->obj_info); i++) {
		if (data->obj_info[i])
			watch_clock_add_info(data->obj_clock, data->obj_info[i]);
	}

out_exit:
	app_mem_free(resource);

	/* update values */
	_clock_view_paint(view_data, true);

	if (data->obj_info[INFO_ANIM] && view_is_focused(CLOCK_VIEW)) {
		anim_img_start(data->obj_info[INFO_ANIM], !first_layout);
	}

	return 0;
}

static int _clock_view_delete(view_data_t *view_data)
{
	clock_view_data_t *data = view_data->user_data;
	int i;

	if (data) {
		lv_obj_delete(data->obj_clock);
		for (i = 0; i < ARRAY_SIZE(data->sty_text); i++) {
			lv_style_reset(&data->sty_text[i]);
		}
	}

	_clock_view_unload_resource(data, true);

	if (data) {
		app_mem_free(data);
	}

	return 0;
}

static int _clock_view_paint(view_data_t *view_data, bool forced)
{
	const clock_view_presenter_t *presenter = view_get_presenter(view_data);
	clock_view_data_t *data = view_data->user_data;
	struct rtc_time time;
	uint32_t value;
	uint8_t buf[16];
	int count;

	if (data == NULL || data->obj_clock == NULL) {
		return 0;
	}

	/* update data via presenter*/
	presenter->get_time(&time);

	/* adjust rtc period */
	if (sp_clock_dsc->period > 1000) {
		uint16_t period = sp_clock_dsc->period / 1000;
		uint16_t sec_remain = (time.tm_min * 60 + time.tm_sec) % period;

		if (sec_remain > 1) {
			data->period = sp_clock_dsc->period - sec_remain * 1000;
			presenter->set_rtc(true, data->period);
		} else if (data->period != sp_clock_dsc->period) {
			data->period = sp_clock_dsc->period;
			presenter->set_rtc(true, data->period);
		}
	}

	SYS_LOG_DBG("time %02u:%02u:%02u.%03u\n", time.tm_hour, time.tm_min, time.tm_sec, time.tm_ms);

	watch_clock_set_time(data->obj_clock, time.tm_hour, time.tm_min, time.tm_sec * 1000 + time.tm_ms);

	if (data->obj_info[INFO_DATE]) {
		if (forced || (data->tm_mday != time.tm_mday) ||
			(data->tm_mon != time.tm_mon) || (data->tm_year != time.tm_year)) {
			lv_label_set_text_fmt(data->obj_info[INFO_DATE], "%s %02d-%02d",
					txt_wday[time.tm_wday], time.tm_mon + 1, time.tm_mday);

			data->tm_mday = time.tm_mday;
			data->tm_mon = time.tm_mon;
			data->tm_year = time.tm_year;
		}
	}

	if (data->obj_info[INFO_STEPCOUNT])
		img_number_set_value(data->obj_info[INFO_STEPCOUNT], presenter->get_step_count(), 0);
	if (data->obj_info[INFO_HEARTRATE])
		img_number_set_value(data->obj_info[INFO_HEARTRATE], presenter->get_heart_rate(), 0);
	if (data->obj_info[INFO_CALORIES])
		img_number_set_value(data->obj_info[INFO_CALORIES], presenter->get_calories(), 0);
	if (data->obj_info[INFO_BATTERY])
		img_number_set_value(data->obj_info[INFO_BATTERY], presenter->get_battery_percent(), 0);
	if (data->obj_info[INFO_TEMPERATURE])
		img_number_set_value(data->obj_info[INFO_TEMPERATURE], presenter->get_temperature(), 0);

	if (data->obj_info[INFO_WEATHER]) /* update weather */
		anim_img_play_static(data->obj_info[INFO_WEATHER], presenter->get_weather());

	/* update distance */
	if (data->obj_info[INFO_DISTANCE]) {
		value = presenter->get_distance() / 10;
		if (forced || data->distance != value) {
			data->distance = value;

			count = split_decimal(value / 100, buf, sizeof(buf), 0);
			buf[count++] = 10;
			count += split_decimal(value % 100, &buf[count], sizeof(buf) - count, 2);
			buf[count++] = 11;
			buf[count++] = 12;
			img_label_set_text(data->obj_info[INFO_DISTANCE], buf, count);
		}
	}

	/* update sleep time */
	if (data->obj_info[INFO_SLEEPTIME]) {
		value = presenter->get_sleep_time();
		if (forced || data->sleep_time != value) {
			data->sleep_time = value;

			count = split_decimal(value / 60, buf, sizeof(buf), 0);
			buf[count++] = 10;
			count += split_decimal(value % 60, &buf[count], sizeof(buf) - count, 2);
			buf[count++] = 11;
			img_label_set_text(data->obj_info[INFO_SLEEPTIME], buf, count);
		}
	}

	return 0;
}

static int _clock_view_focus_changed(view_data_t *view_data, bool focused)
{
	/* data is allocate in preload() */
	clock_view_data_t *data = view_data->user_data;
	const clock_view_presenter_t *presenter = view_get_presenter(view_data);

	if (presenter)
		presenter->set_rtc(focused, sp_clock_dsc->period);

	if (focused) {
		if (data) {
			if (!lvgl_res_scene_is_loaded(sp_clock_dsc->scene)) {
				SYS_LOG_INF("preload clock_view\n");
				_clock_view_preload(view_data, true);
			} else {
				if (data->obj_info[INFO_ANIM]) {
					SYS_LOG_INF("_clock_view anim_img_start no need to preload\n");
					anim_img_start(data->obj_info[INFO_ANIM], true);
				}
			}
		}
	} else {
		if (data) {
			if (data->obj_info[INFO_ANIM]) {
				anim_img_stop(data->obj_info[INFO_ANIM]);
				anim_img_clean(data->obj_info[INFO_ANIM]);
			}
		}

		_clock_view_unload_resource(data, false);
	}

	return 0;
}

int _clock_view_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void *msg_data)
{
	assert(view_id == CLOCK_VIEW);

	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return _clock_view_preload(view_data, false);
	case MSG_VIEW_LAYOUT:
		return _clock_view_layout_update(view_data, true);
	case MSG_VIEW_DELETE:
		return _clock_view_delete(view_data);
	case MSG_VIEW_PAINT:
	case MSG_VIEW_RESUME_DISPLAY:
		return _clock_view_paint(view_data, false);
	case MSG_VIEW_FOCUS:
		return _clock_view_focus_changed(view_data, true);
	case MSG_VIEW_DEFOCUS:
		return _clock_view_focus_changed(view_data, false);
	case MSG_VIEW_UPDATE:
		return _clock_view_layout_update(view_data, false);
	default:
		return 0;
	}
}

int _clock_view_select(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void *msg_data)
{
	assert(view_id == CLOCK_VIEW);

	if(MSG_VIEW_PRELOAD == msg_id || clock_presenter == NULL)
		clock_presenter = view_get_presenter(view_data);
	uint8_t clock_id = clock_presenter->get_clock_id();
	switch (clock_id)
	{
	case MAIN_CLOCK_ID:
	case DIGITAL_CLOCK_ID:
	#ifdef SCENE_AOD_CLOCK_VIEW
	case AOD_CLOCK_ID:
	#endif
		return _clock_view_handler(view_id, view_data, msg_id, msg_data);
	case DIGITAL1_CLOCK_ID:
		return _digital1_clock_view_handler(view_id, view_data, msg_id, msg_data);
	#ifdef CONFIG_VIDEO_APP
	case VIDEO_CLOCK_ID:
		return _video_clock_view_handler(view_id, view_data, msg_id, msg_data);
	#endif
	default:
		return -ENOMEM;
	}
}

VIEW_DEFINE2(clock, _clock_view_select, NULL, NULL, CLOCK_VIEW,
		NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);
