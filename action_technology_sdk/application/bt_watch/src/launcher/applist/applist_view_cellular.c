/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <os_common_api.h>
#include <widgets/simple_img.h>
#include "applist_view_inner.h"

/**********************
 *   DEFINES
 **********************/
/* Maximum zoom factor of the origin image to help reduce image storage size */
#define ICON_ZOOM_MAX  (LV_SCALE_NONE * 3 / 2)

#define ICON_CODER_ZOOM_MIN (LV_SCALE_NONE/2)
#define ICON_CODER_ZOOM_MAX (LV_SCALE_NONE*4)

/* Distance zoom coefficient */
#define ICON_ZOOM_COEF        9

/* Cellular radius increment in percentage to help increase icon space */
#define ICON_RADIUS_ZOOM_PCT  10

/* Minimum scroll animation duration */
#define SCROLL_ANIM_TIME_MIN  200    /*ms*/

/* Maximum scroll animation duration */
#define SCROLL_ANIM_TIME_MAX  400    /*ms*/

/* Test options: can set this if too few icons */
#define TEST_DOUBLE_ICONS     0

/**********************
 *   STATIC PROTOTYPES
 **********************/
static int _cellular_view_create(lv_obj_t * scr);
static void _cellular_view_delete(lv_obj_t * scr);
static void _cellular_view_defocus(lv_obj_t * scr);
static void _cellular_view_update_icon(applist_ui_data_t * data, uint8_t idx, const lv_image_dsc_t * src);
static void _cellular_press_event_cb(lv_event_t * e);

/**********************
 *  GLOBAL VARIABLES
 **********************/
const applist_view_cb_t g_applist_cellular_view_cb = {
	.create = _cellular_view_create,
	.delete = _cellular_view_delete,
	.defocus = _cellular_view_defocus,
	.update_icon = _cellular_view_update_icon,
	.update_text = NULL,
};

/**********************
 *  STATIC VARIABLES
 **********************/

static void _compute_icon_step(lv_point_t *step, uint16_t hexagon_r, uint16_t hexagon_idx)
{
	/* left-top edge */
	if (hexagon_idx <= hexagon_r) {
		step->x = -hexagon_r * 2 + hexagon_idx;
		step->y = -hexagon_idx;
		return;
	}

	hexagon_idx -= hexagon_r;

	/* top edge */
	if (hexagon_idx <= hexagon_r) {
		step->x = -hexagon_r + hexagon_idx * 2;
		step->y = -hexagon_r;
		return;
	}

	hexagon_idx -= hexagon_r;

	/* right-top edge */
	if (hexagon_idx <= hexagon_r) {
		step->x = hexagon_r + hexagon_idx;
		step->y = -hexagon_r + hexagon_idx;
		return;
	}

	hexagon_idx -= hexagon_r;

	/* right-bottom edge */
	if (hexagon_idx <= hexagon_r) {
		step->x = hexagon_r * 2 - hexagon_idx;
		step->y = hexagon_idx;
		return;
	}

	hexagon_idx -= hexagon_r;

	/* bottom edge */
	if (hexagon_idx <= hexagon_r) {
		step->x = hexagon_r - hexagon_idx * 2;
		step->y = hexagon_r;
		return;
	}

	hexagon_idx -= hexagon_r;

	/* left-bottom edge */
	if (hexagon_idx <= hexagon_r) {
		step->x = -hexagon_r - hexagon_idx;
		step->y = hexagon_r - hexagon_idx;
		return;
	}
}

static void _cellular_image_press_event_cb(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_SHORT_CLICKED) {
		applist_ui_data_t *data = lv_event_get_user_data(e);
		applist_cellular_view_t *cellular = &data->cellular_view;

		if (cellular->scrolling == false)
			applist_btn_event_def_handler(e);
	} else {
		_cellular_press_event_cb(e);
	}
}

static void _celluar_view_move(applist_ui_data_t *data)
{
	applist_cellular_view_t *cellular = &data->cellular_view;
	lv_point_t bg_size = { lv_obj_get_content_width(data->cont), lv_obj_get_content_height(data->cont) };
	lv_point_t bg_pivot = { bg_size.x / 2, bg_size.y / 2 };
	uint32_t dist_max2 = UINT32_MAX;
	/* add 2 extra pixels for compute deviation */
	cellular->radius =LV_MAX(data->icon[0].header.w, data->icon[0].header.h);
	cellular->radius = cellular->radius * cellular->max_zoom / (LV_SCALE_NONE * 2);
	cellular->radius += cellular->radius * ICON_RADIUS_ZOOM_PCT / 100;
	lv_area_increase(&cellular->scrl_area, cellular->radius, cellular->radius);

	uint16_t hexagon_r = 0;
	uint16_t hexagon_icons = 1;
	uint16_t hexagon_idx = 0;
	lv_coord_t col_ofs = (lv_trigo_sin(30) * cellular->radius) >> (LV_TRIGO_SHIFT - 1);
	lv_coord_t row_ofs = (lv_trigo_sin(60) * cellular->radius) >> (LV_TRIGO_SHIFT - 1);
	for (int i = lv_obj_get_child_cnt(data->cont) - 1; i >= 0; i--, hexagon_idx++) {
		lv_obj_t *obj_icon = lv_obj_get_child(data->cont, i);
		if (obj_icon == NULL) /* make Klocwork static code check happy */
			continue;

		if (hexagon_idx >= hexagon_icons) {
			hexagon_r++; /* outer hexagon circle */
			hexagon_icons = hexagon_r * 6;
			hexagon_idx = 0;
		}

		lv_point_t step = { 0, 0 };
		_compute_icon_step(&step, hexagon_r, hexagon_idx);

		cellular->pivots[i].x = bg_pivot.x + step.x * col_ofs;
		cellular->pivots[i].y = bg_pivot.y + step.y * row_ofs;

		if (cellular->scrl_area.x1 > -cellular->pivots[i].x)
			cellular->scrl_area.x1 = -cellular->pivots[i].x;
		if (cellular->scrl_area.x2 < bg_size.x - cellular->pivots[i].x)
			cellular->scrl_area.x2 = bg_size.x - cellular->pivots[i].x;
		if (cellular->scrl_area.y1 > -cellular->pivots[i].y)
			cellular->scrl_area.y1 = -cellular->pivots[i].y;
		if (cellular->scrl_area.y2 < bg_size.y - cellular->pivots[i].y)
			cellular->scrl_area.y2 = bg_size.y - cellular->pivots[i].y;

		lv_point_t pivot = {
			cellular->pivots[i].x + cellular->scrl_ofs.x,
			cellular->pivots[i].y + cellular->scrl_ofs.y,
		};

		uint32_t dist2 = (pivot.x - bg_pivot.x) * (pivot.x - bg_pivot.x) +
				(pivot.y - bg_pivot.y) * (pivot.y - bg_pivot.y);
		if (dist2 < dist_max2) {
			dist_max2 = dist2;
			cellular->scrl_nearest_idx = i;
		}

		lv_sqrt_res_t res;
		lv_sqrt(dist2, &res, 0x8000);

		lv_coord_t dist = res.i;
		lv_coord_t dist_near = dist - cellular->radius;
		lv_coord_t dist_far = dist + cellular->radius;

		/* zoom distance */
		dist_near = dist_near * cellular->radius * (ICON_ZOOM_COEF + 1) /
				(cellular->radius * ICON_ZOOM_COEF + LV_ABS(dist_near));
		dist_far = dist_far * cellular->radius * (ICON_ZOOM_COEF + 1) /
				(cellular->radius * ICON_ZOOM_COEF + LV_ABS(dist_far));

#ifdef CONFIG_PANEL_ROUND_SHAPE
		dist_far = LV_MIN(dist_far, bg_pivot.x);
		if (dist_near >= dist_far) {
			lvgl_obj_set_hidden(obj_icon, true);
			continue;
		}

		lv_coord_t dist_pivot = (dist_near + dist_far) / 2;
		lv_coord_t radius = (dist_far - dist_near) / 2;

		if (dist > 0) {
			pivot.x = bg_pivot.x + (pivot.x - bg_pivot.x) * dist_pivot / dist;
			pivot.y = bg_pivot.y + (pivot.y - bg_pivot.y) * dist_pivot / dist;
		}
#else
		lv_coord_t dist_pivot = (dist_near + dist_far) / 2;
		lv_coord_t radius = (dist_far - dist_near) / 2;

		if (dist > 0) {
			pivot.x = bg_pivot.x + (pivot.x - bg_pivot.x) * dist_pivot / dist;
			pivot.y = bg_pivot.y + (pivot.y - bg_pivot.y) * dist_pivot / dist;
		}

		if (pivot.x + radius < 0 || pivot.x - radius > bg_size.x ||
			pivot.y + radius < 0 || pivot.y - radius > bg_size.y) {
			lvgl_obj_set_hidden(obj_icon, true);
			continue;
		}

		if (pivot.x - radius < 0) { /* left side */
			radius = (pivot.x + radius) / 2;
			pivot.x = radius;
		} else if (pivot.x + radius > bg_size.x) { /* right side */
			radius = (bg_size.x - (pivot.x - radius)) / 2;
			pivot.x = bg_size.x - radius;
		}

		if (pivot.y - radius < 0) { /* top side */
			radius = (pivot.y + radius) / 2;
			pivot.y = radius;
		} else if (pivot.y + radius > bg_size.y) { /* bottom side */
			radius = (bg_size.y - (pivot.y - radius)) / 2;
			pivot.y = bg_size.y - radius;
		}
#endif /* CONFIG_PANEL_ROUND_SHAPE */

		uint16_t zoom = cellular->max_zoom * radius / cellular->radius;
		/*if (zoom < cellular->min_zoom) {
			lvgl_obj_set_hidden(obj_icon, true);
			continue;
		}*/

		simple_img_set_scale(obj_icon, zoom);
		lvgl_obj_set_hidden(obj_icon, false);
		lv_obj_set_pos(obj_icon, pivot.x - data->icon[0].header.w / 2,
				pivot.y - data->icon[0].header.h / 2);
	}
}

static void _cellular_scroll_anim(void * var, int32_t v)
{
	applist_ui_data_t *data = var;
	applist_cellular_view_t* cellular = &data->cellular_view;

	cellular->scrl_ofs.x = cellular->scrl_anim_end.x -
			cellular->scrl_anim_vect.x * (1024 - v) / 1024;
	cellular->scrl_ofs.y = cellular->scrl_anim_end.y -
			cellular->scrl_anim_vect.y * (1024 - v) / 1024;

	_celluar_view_move(data);
}

static void _cellular_press_event_cb(lv_event_t * e)
{
	applist_ui_data_t *data = lv_event_get_user_data(e);
	applist_cellular_view_t *cellular = &data->cellular_view;
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_PRESSED) {
		lv_indev_t * indev = lv_event_get_param(e);
		lv_indev_get_point(indev, &cellular->pressed_pt);
		lv_anim_delete(data, _cellular_scroll_anim);
		cellular->scrolling = false;
		return;
	}

	if (code == LV_EVENT_PRESSING) {
		lv_indev_t * indev = lv_event_get_param(e);
		lv_point_t vect = { 0, 0 };

		if (cellular->scrolling == false) {
			lv_indev_get_point(indev, &vect);

			vect.x -= cellular->pressed_pt.x;
			vect.y -= cellular->pressed_pt.y;

			if (LV_ABS(vect.x) < indev->scroll_limit &&
				LV_ABS(vect.y) < indev->scroll_limit) {
				return;
			}

			cellular->scrolling = true;
		}

		lv_indev_get_vect(indev, &vect);

		cellular->scrl_ofs.x = LV_CLAMP(cellular->scrl_area.x1,
				cellular->scrl_ofs.x + vect.x, cellular->scrl_area.x2);
		cellular->scrl_ofs.y = LV_CLAMP(cellular->scrl_area.y1,
				cellular->scrl_ofs.y + vect.y, cellular->scrl_area.y2);

		_celluar_view_move(data);
	}

	if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
		if (cellular->scrolling) {
			lv_point_t *pivot = &cellular->pivots[cellular->scrl_nearest_idx];
			cellular->scrl_anim_end.x = lv_obj_get_content_width(data->cont) / 2 - pivot->x;
			cellular->scrl_anim_end.y = lv_obj_get_content_height(data->cont) / 2 - pivot->y;
			cellular->scrl_anim_vect.x = cellular->scrl_anim_end.x - cellular->scrl_ofs.x;
			cellular->scrl_anim_vect.y = cellular->scrl_anim_end.y - cellular->scrl_ofs.y;

			if (cellular->scrl_anim_vect.x || cellular->scrl_anim_vect.y) {
				lv_anim_t a;
				lv_anim_init(&a);
				lv_anim_set_var(&a, data);

				uint32_t range = LV_MAX(LV_ABS(cellular->scrl_anim_vect.x), LV_ABS(cellular->scrl_anim_vect.y));
				uint32_t t = lv_anim_speed_to_time((lv_obj_get_content_width(data->cont) * 2) >> 2, 0, range);
				if (t < SCROLL_ANIM_TIME_MIN) t = SCROLL_ANIM_TIME_MIN;
				if (t > SCROLL_ANIM_TIME_MAX) t = SCROLL_ANIM_TIME_MAX;

				lv_anim_set_duration(&a, t);
				lv_anim_set_values(&a, 0, 1024);
				lv_anim_set_exec_cb(&a, _cellular_scroll_anim);
				lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
				lv_anim_start(&a);
			}
		}
	}
}

static void _cellular_coder_cb(void *trigger_data)
{
	uint16_t diff = 20;
	ui_coder_data_t * coder_data = _get_coder_data();
	applist_ui_data_t *data = (applist_ui_data_t *)get_coder_user_data(trigger_data);
	applist_cellular_view_t* cellular = &data->cellular_view;
	int16_t zoom = cellular->max_zoom;

	if(coder_data == NULL)
		return;

	if(coder_data->direction == CODER_DIRECTION_FORWARD)
		zoom += diff;
	else if(coder_data->direction == CODER_DIRECTION_INVERSION)
		zoom -= diff;
	if(zoom < ICON_CODER_ZOOM_MIN)
		zoom = ICON_CODER_ZOOM_MIN;
	else if(zoom > ICON_CODER_ZOOM_MAX)
		zoom = ICON_CODER_ZOOM_MAX;

	if(cellular->max_zoom != zoom)
	{
		lv_point_t *pivot = &cellular->pivots[cellular->scrl_nearest_idx];
		cellular->scrl_ofs.x = lv_obj_get_content_width(data->cont) / 2 - pivot->x;
		cellular->scrl_ofs.y = lv_obj_get_content_height(data->cont) / 2 - pivot->y;
		cellular->max_zoom = zoom;
		_celluar_view_move(data);
	}
}

static int _cellular_view_create(lv_obj_t * scr)
{
	applist_ui_data_t *data = lv_obj_get_user_data(scr);
	applist_cellular_view_t* cellular = &data->cellular_view;

	data->cont = lv_obj_create(scr);
	lv_obj_remove_flag(data->cont, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_pos(data->cont, data->res_scene.x, data->res_scene.y);
	lv_obj_set_size(data->cont, data->res_scene.width, data->res_scene.height);
	lv_obj_set_scroll_dir(data->cont, LV_DIR_ALL);
	lv_obj_set_style_pad_all(data->cont, 10, LV_PART_MAIN);
	lv_obj_set_user_data(data->cont, data);
	lv_obj_add_event_cb(data->cont, _cellular_press_event_cb, LV_EVENT_ALL, data);
	coder_private_event_register(data->cont, APPLIST_VIEW, _cellular_coder_cb, data);
	lv_obj_update_layout(data->cont);

	cellular->max_zoom = ICON_ZOOM_MAX;

	cellular->pivots = lv_malloc(sizeof(lv_point_t) * NUM_ITEMS * (TEST_DOUBLE_ICONS + 1));
	if (cellular->pivots == NULL)
		return -ENOMEM;

	for (int i = 0; i < NUM_ITEMS * (TEST_DOUBLE_ICONS + 1); i++) {
		lv_obj_t* obj_icon = simple_img_create(data->cont);
		simple_img_set_src(obj_icon, applist_get_icon(data, i % NUM_ITEMS));
		lv_obj_add_flag(obj_icon, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_set_user_data(obj_icon, (void*)(i % NUM_ITEMS));
		lv_obj_add_event_cb(obj_icon, _cellular_image_press_event_cb, LV_EVENT_ALL, data);
	};

	int16_t scrl_x = 0;
	int16_t scrl_y = 0;
	data->presenter->load_scroll_value(&scrl_x, &scrl_y);
	cellular->scrl_ofs.x = scrl_x;
	cellular->scrl_ofs.y = scrl_y;

	/* update the buttons position manually for first*/
	_celluar_view_move(data);

	return 0;
}

static void _cellular_view_delete(lv_obj_t * scr)
{
	applist_ui_data_t *data = lv_obj_get_user_data(scr);
	applist_cellular_view_t* cellular = &data->cellular_view;

	lv_anim_delete(data, _cellular_scroll_anim);
	lv_free(cellular->pivots);

	data->presenter->save_scroll_value(
			cellular->scrl_ofs.x, cellular->scrl_ofs.y);
}

static void _cellular_view_defocus(lv_obj_t * scr)
{
	applist_ui_data_t *data = lv_obj_get_user_data(scr);
	lv_anim_delete(data, _cellular_scroll_anim);
}

static void _cellular_view_update_icon(applist_ui_data_t * data, uint8_t idx, const lv_image_dsc_t * src)
{
	lv_obj_t * obj_icon = lv_obj_get_child(data->cont, idx);
	if (obj_icon) {
		simple_img_set_src(obj_icon, src);
	}

#if TEST_DOUBLE_ICONS
	obj_icon = lv_obj_get_child(data->cont, idx + NUM_ITEMS);
	if(obj_icon) {
		simple_img_set_src(obj_icon, src);
	}
#endif
}
