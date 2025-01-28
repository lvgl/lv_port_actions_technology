/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file img_number.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <sys/util.h>
#include "draw_util.h"
#include "img_number.h"
#include <lvgl/src/lvgl_private.h>

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &img_number_class

/**********************
 *      TYPEDEFS
 **********************/
/** Data of img number */
typedef struct {
	img_label_t label;
	uint32_t value;
	uint8_t min_w;
	uint8_t has_unit : 1;
} img_number_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t img_number_class = {
	.width_def = LV_SIZE_CONTENT,
	.height_def = LV_SIZE_CONTENT,
	.instance_size = sizeof(img_number_t),
	.base_class = &img_label_class,
	.name = "img-number",
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * img_number_create(lv_obj_t * parent)
{
	LV_LOG_INFO("begin");
	lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
	lv_obj_class_init_obj(obj);

	img_number_t * img = (img_number_t *)obj;
	img->value = 1;
	img_number_set_value(obj, 0, 0);

	return obj;
}

void img_number_set_align(lv_obj_t *obj, lv_align_t align)
{
	img_label_set_align(obj, align);
}

void img_number_set_src(lv_obj_t *obj, const lv_image_dsc_t * srcs, uint8_t cnt)
{
	img_number_t * img = (img_number_t *)obj;

	if (cnt >= 10) {
		img_label_set_src(obj, srcs, cnt);
		img->has_unit = (cnt > 10);
	}
}

void img_number_set_value(lv_obj_t * obj, uint32_t value, uint8_t min_w)
{
	img_number_t * img = (img_number_t *)obj;

	if (img->value != value || img->min_w != min_w) {
		uint8_t buf[12];
		int count = split_decimal(value, buf, 10, min_w);

		if (img->has_unit)
			buf[count++] = 10;

		img_label_set_text(obj, buf, count);
		img->value = value;
	}
}
