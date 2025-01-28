/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file analog_clock.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "watch_clock.h"
#include "simple_img.h"
#include "img_number.h"
#include "draw_util.h"
#include <lvgl/src/lvgl_private.h>

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &watch_clock_class

/**********************
 *      TYPEDEFS
 **********************/
/** Data of actions rotation image */
typedef struct {
	lv_obj_t obj;

	uint8_t type;
	uint8_t en_24hour : 1;

	lv_obj_t *time[3];    /* clock time (hour/minute/second) */
	lv_obj_t *colon;      /* colon between hour and minute of digital clock */
} watch_clock_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void watch_clock_event(const lv_obj_class_t * class_p, lv_event_t * e);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t watch_clock_class = {
	.event_cb = watch_clock_event,
	.width_def = LV_PCT(100),
	.height_def = LV_PCT(100),
	.instance_size = sizeof(watch_clock_t),
	.base_class = &lv_obj_class,
	.name = "watch-clock",
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * watch_clock_create(lv_obj_t * parent)
{
	LV_LOG_INFO("begin");
	lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
	lv_obj_class_init_obj(obj);
	return obj;
}

void watch_clock_set_type(lv_obj_t * obj, clock_type_t type, bool show_sec)
{
	watch_clock_t * clock = (watch_clock_t *)obj;
	lv_obj_t * (*create_fn)(lv_obj_t *);
	int pointer_cnt = show_sec ? 3 : 2;

	clock->type = type;
	clock->en_24hour = 0;
	create_fn = (type == ANALOG_CLOCK) ? simple_img_create : img_number_create;

	for (int i = 0; i < pointer_cnt; i++) {
		clock->time[i] = create_fn(obj);
	}
}

void watch_clock_set_pointer_images(lv_obj_t * obj, uint8_t idx, uint8_t cnt,
		const lv_image_dsc_t * srcs, lv_coord_t pivot_x, lv_coord_t pivot_y)
{
	watch_clock_t * clock = (watch_clock_t *)obj;

	if (clock->type != ANALOG_CLOCK || idx >= 3)
		return;

	if (clock->time[idx]) {
		lv_coord_t obj_w = lv_obj_get_style_width(obj, LV_PART_MAIN);
		lv_coord_t obj_h = lv_obj_get_style_height(obj, LV_PART_MAIN);

		/* the pointer image pivot should be at the center of the clock */
		lv_obj_set_pos(clock->time[idx], obj_w / 2 - pivot_x, obj_h / 2 - pivot_y);

		simple_img_set_src(clock->time[idx], &srcs[0]);
		simple_img_set_pivot(clock->time[idx], pivot_x, pivot_y);
	}
}

void watch_clock_set_digit_images(lv_obj_t * obj, uint8_t idx, uint8_t cnt,
		const lv_image_dsc_t * srcs, lv_area_t *area)
{
	watch_clock_t * clock = (watch_clock_t *)obj;

	if (clock->type != DIGITAL_CLOCK || idx >= 3 || cnt != 10)
		return;

	if (clock->time[idx]) {
		lv_obj_set_pos(clock->time[idx], area->x1, area->y1);
		lv_obj_set_size(clock->time[idx], lv_area_get_width(area), lv_area_get_height(area));
		img_number_set_align(clock->time[idx], LV_ALIGN_RIGHT_MID);
		img_number_set_src(clock->time[idx], srcs, 10);
	}
}

void watch_clock_set_separator_image(lv_obj_t * obj,
		const lv_image_dsc_t * src, lv_coord_t pos_x, lv_coord_t pos_y)
{
	watch_clock_t * clock = (watch_clock_t *)obj;

	if (clock->type != DIGITAL_CLOCK)
		return;

	if (clock->colon == NULL)
		clock->colon = simple_img_create(obj);

	if (clock->colon) {
		lv_obj_set_pos(clock->colon, pos_x, pos_y);
		simple_img_set_src(clock->colon, src);
	}
}

void watch_clock_set_24hour(lv_obj_t * obj, bool en)
{
	watch_clock_t * clock = (watch_clock_t *)obj;

	clock->en_24hour = (en && clock->type == DIGITAL_CLOCK) ? 1 : 0;
}

void watch_clock_set_time(lv_obj_t * obj, uint8_t hour, uint8_t minute, uint16_t msec)
{
	watch_clock_t * clock = (watch_clock_t *)obj;

	if (hour >= 24 || minute >= 60 || msec >= 60000)
		return;

	if (!clock->en_24hour && hour >= 12)
		hour -= 12;

	if (clock->type == ANALOG_CLOCK) {
		simple_img_set_rotation(clock->time[CLOCK_HOUR], hour * 300 + minute * 5);
		if (clock->time[CLOCK_SEC]) {
			simple_img_set_rotation(clock->time[CLOCK_MIN], minute * 60 + msec / 1000);
			simple_img_set_rotation(clock->time[CLOCK_SEC], msec * 6 / 100);
		} else {
			simple_img_set_rotation(clock->time[CLOCK_MIN], minute * 60);
		}
	} else {
		img_number_set_value(clock->time[CLOCK_HOUR], hour, 2);
		img_number_set_value(clock->time[CLOCK_MIN], minute, 2);
		if (clock->time[CLOCK_SEC])
			img_number_set_value(clock->time[CLOCK_SEC], msec / 1000, 2);
	}
}

void watch_clock_add_info(lv_obj_t * obj, lv_obj_t * info_obj)
{
	lv_obj_move_to_index(info_obj, 0);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void watch_clock_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
	LV_UNUSED(class_p);

	lv_event_code_t code = lv_event_get_code(e);

	/*Ancestor events will be called during drawing*/
	if (code != LV_EVENT_DRAW_POST) {
		/*Call the ancestor's event handler*/
		lv_obj_event_base(MY_CLASS, e);
	}
}
