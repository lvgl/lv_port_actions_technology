/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*********************
 *      INCLUDES
 *********************/
#include <assert.h>
#include <app_ui.h>
#include <widgets/svg_img.h>
#include <widgets/draw_util.h>
#ifdef CONFIG_SYS_WAKELOCK
#  include <sys_wakelock.h>
#endif
#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
#  include <dvfs.h>
#endif

#if defined(CONFIG_SVG_RENDER)

/*********************
 *      DEFINES
 *********************/
#define SVG_PATH DEF_UI_DISK"/hero.svg"
#define ZOOM_MIN (LV_SCALE_NONE / 2)

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
	lv_obj_t *svg_obj;
	lv_timer_t * timer;
	uint16_t angle; /* measure in 0.1 degree */
	uint16_t zoom_base;
} svgmap_scene_data_t;

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *  STATIC FUNCTIONS
 **********************/

static void _timer_cb(lv_timer_t *timer)
{
	svgmap_scene_data_t * data = timer->user_data;
	uint16_t zoom;

	data->angle += 100;
	if (data->angle >= 3600)
		data->angle -= 3600;

	svg_img_set_rotation(data->svg_obj, data->angle);

	zoom = ZOOM_MIN + data->angle * (LV_SCALE_NONE - ZOOM_MIN) / 3600;
	zoom = data->zoom_base * zoom / LV_SCALE_NONE;

	svg_img_set_scale(data->svg_obj, zoom);
}

static int _svgmap_view_layout(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);

	svgmap_scene_data_t * data = app_mem_malloc(sizeof(* data));
	if (data == NULL)
		return -ENOMEM;

	memset(data, 0, sizeof(* data));

	data->svg_obj = svg_img_create(scr);
	if (LV_RESULT_OK != svg_img_parse_file(data->svg_obj, SVG_PATH)) {
		SYS_LOG_ERR("fail to parse %s", SVG_PATH);
		app_mem_free(data);
		return -EINVAL;
	}

	lv_point_t size;
	svg_img_get_size(data->svg_obj, &size);
	SYS_LOG_INF("SVG size %d x %d", size.x, size.y);

	lv_coord_t img_size = LV_MAX(size.x, size.y);
	lv_coord_t scr_size = LV_MIN(lv_obj_get_width(scr), lv_obj_get_height(scr));
	data->zoom_base = (img_size <= scr_size) ? LV_SCALE_NONE :
			(LV_SCALE_NONE * scr_size / img_size);

	svg_img_set_font(data->svg_obj, DEF_VFONT_FILE);
	svg_img_set_scale(data->svg_obj, data->zoom_base);
	svg_img_set_rotation(data->svg_obj, data->angle);
	lv_obj_center(data->svg_obj);

	/* FIXME: the background is white when showed in web browser. */
	lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

	data->timer = lv_timer_create(_timer_cb, 16, data);
	if (data->timer)
		lv_timer_ready(data->timer);

	view_data->user_data = data;

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_lock(FULL_WAKE_LOCK);
#endif

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "svg");
#endif

	return 0;
}

static int _svgmap_view_delete(view_data_t *view_data)
{
	svgmap_scene_data_t * data = view_data->user_data;

	if (data) {
		if (data->timer)
			lv_timer_delete(data->timer);

		app_mem_free(data);

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
		dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "svg");
#endif

#ifdef CONFIG_SYS_WAKELOCK
		sys_wake_unlock(FULL_WAKE_LOCK);
#endif
	}

	return 0;
}

static int _svgmap_view_proc_key(view_data_t *view_data, ui_key_msg_data_t * key_data)
{
	return 0;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

int svgmap_view_handler(uint16_t view_id, view_data_t * view_data, uint8_t msg_id, void * msg_data)
{
	assert(view_id == SVGMAP_VIEW);

	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return ui_view_layout(SVGMAP_VIEW);
	case MSG_VIEW_LAYOUT:
		return _svgmap_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _svgmap_view_delete(view_data);
	case MSG_VIEW_KEY:
		return _svgmap_view_proc_key(view_data, msg_data);
	case MSG_VIEW_FOCUS:
	case MSG_VIEW_DEFOCUS:
	default:
		return 0;
	}
}

VIEW_DEFINE2(svgmap, svgmap_view_handler, NULL, NULL, SVGMAP_VIEW,
		NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);

#endif /* defined(CONFIG_SVG_RENDER) */
