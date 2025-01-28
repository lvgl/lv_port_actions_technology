/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*********************
 *      INCLUDES
 *********************/
#include <widgets/svg_img.h>
#include <svgrender/vglite_renderer.h>
#include "launcher_ui.h"

/*********************
 *      DEFINES
 *********************/
#define SVG_PATH DEF_UI_DISK"/tiger.svg"

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
	lv_obj_t *svg_obj;
	lv_timer_t * timer;
	uint16_t angle; /* measure in 0.1 degree */
} svg_scene_data_t;

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *  STATIC FUNCTIONS
 **********************/

static void _timer_cb(lv_timer_t *timer)
{
	svg_scene_data_t * data = timer->user_data;

	data->angle += 150;
	if (data->angle >= 3600)
		data->angle -= 3600;

	svg_img_set_angle(data->svg_obj, data->angle);
}

static int _svg_scene_create(lv_obj_t * scr, const void * presenter)
{
	svg_scene_data_t * data = app_mem_malloc(sizeof(* data));
	if (data == NULL)
		return -ENOMEM;

	memset(data, 0, sizeof(* data));

	data->svg_obj = svg_img_create(scr);
	if (LV_RES_OK != svg_img_parse_file(data->svg_obj, SVG_PATH)) {
		SYS_LOG_ERR("fail to parse %s", SVG_PATH);
		app_mem_free(data);
		return -EINVAL;
	}

	lv_point_t size;
	svg_img_get_size(data->svg_obj, &size);
	SYS_LOG_INF("SVG size %d x %d", size.x, size.y);

	svg_img_set_zoom(data->svg_obj, 512);
	svg_img_set_angle(data->svg_obj, data->angle);
	lv_obj_center(data->svg_obj);

	/* FIXME: the background is white when showed in web browser. */
	lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
	lv_obj_set_user_data(scr, data);

	data->timer = lv_timer_create(_timer_cb, 16, data);
	if (data->timer)
		lv_timer_ready(data->timer);

	return 0;
}

static int _svg_scene_delete(lv_obj_t * scr)
{
	svg_scene_data_t * data = lv_obj_get_user_data(scr);
	if (data == NULL)
		return 0;

	if (data->timer)
		lv_timer_del(data->timer);

	app_mem_free(data);
	return 0;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
int svg_scene_proc(void * scr, uint8_t msg_id, void * msg_data)
{
	switch (msg_id) {
	case MSG_VIEW_CREATE:
		return _svg_scene_create(scr, msg_data);
	case MSG_VIEW_DELETE:
		return _svg_scene_delete(scr);
	default:
		return 0;
	}
}
