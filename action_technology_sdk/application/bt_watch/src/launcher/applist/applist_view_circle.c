/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os_common_api.h>
#include <widgets/simple_img.h>
#include "applist_view_inner.h"

#define D_ICON_NUM		42
#define ICON_CIRCLE_RAD	160
#define ICON_INTERVAL_Y	15
#define ICON_MAX_ZOOM	(LV_SCALE_NONE + 0xff)
#define ICON_LOOP_ZOOM	(LV_SCALE_NONE + 0x1f)
#define ICON_MIN_ZOOM	0x7f
#define D_ICON_ANIM_SPEED  100
#define D_ICON_SCROLL_SPEED  100

typedef struct {
	lv_obj_t *obj_icon;
} data_circle_t;

static int _circle_view_create(lv_obj_t * scr);
static void _circle_view_delete(lv_obj_t * scr);
static void _circle_view_update_icon(applist_ui_data_t * data, uint8_t idx, const lv_img_dsc_t * src);

const applist_view_cb_t g_applist_circle_view_cb = {
	.create = _circle_view_create,
	.delete = _circle_view_delete,
	.update_icon = _circle_view_update_icon,
};

static void _circle_icon_set_pos(applist_circle_view_t *circle_view)
{
	int32_t obj_row = 1 + (-circle_view->move_y / ICON_CIRCLE_RAD);
	int32_t offset_y = circle_view->move_y % ICON_CIRCLE_RAD;
	data_circle_t *icon_buf = (data_circle_t *)circle_view->user_data;
	int32_t middle_id = obj_row * 3;
	int32_t skip_id = middle_id;
	int32_t skip_y = offset_y;
	const lv_img_dsc_t *img_dsc = simple_img_get_src(icon_buf->obj_icon);
	int32_t offset_x = img_dsc->header.w / 2;
	int32_t target_y = lv_trigo_sin(45) * ICON_CIRCLE_RAD / 32767;
	int32_t target_x = lv_trigo_cos(45) * ICON_CIRCLE_RAD / 32767;
	bool b_calculate = true;
	for (uint16_t i = 0; i < D_ICON_NUM/ 3; i++) {
		if(skip_id < 0 || skip_id >= D_ICON_NUM)
			break;
		int32_t distance = skip_y;
		data_circle_t *icon_data = icon_buf + skip_id;
		if(LV_ABS(distance) < ICON_CIRCLE_RAD) {
			int32_t angle = ui_map(distance, -ICON_CIRCLE_RAD, ICON_CIRCLE_RAD, -45, 45);
			int32_t target_yc = lv_trigo_sin(angle) * ICON_CIRCLE_RAD / 32767;
			int32_t target_xc = lv_trigo_cos(angle) * ICON_CIRCLE_RAD / 32767;
			target_xc = LV_ABS(target_xc);
			lv_obj_set_pos(icon_data[0].obj_icon, DEF_UI_WIDTH/2 - target_xc - offset_x, DEF_UI_HEIGHT/2 + target_yc - offset_x);
			simple_img_set_scale(icon_data[0].obj_icon, ICON_LOOP_ZOOM);
			lv_obj_remove_flag(icon_data[0].obj_icon, LV_OBJ_FLAG_HIDDEN);
			if(skip_id + 2 < D_ICON_NUM) {
				lv_obj_set_pos(icon_data[2].obj_icon, DEF_UI_WIDTH/2 + target_xc - offset_x, DEF_UI_HEIGHT/2 + target_yc - offset_x);
				simple_img_set_scale(icon_data[2].obj_icon, ICON_LOOP_ZOOM);
				lv_obj_remove_flag(icon_data[2].obj_icon, LV_OBJ_FLAG_HIDDEN);
			}
			if(skip_id + 1 < D_ICON_NUM) {
				int32_t zoom = ui_map(LV_ABS(distance), 0, ICON_CIRCLE_RAD, ICON_MAX_ZOOM, ICON_LOOP_ZOOM);
				lv_obj_set_pos(icon_data[1].obj_icon, DEF_UI_WIDTH/2 - offset_x, DEF_UI_HEIGHT/2 + distance - offset_x);
				simple_img_set_scale(icon_data[1].obj_icon, zoom);
				lv_obj_remove_flag(icon_data[1].obj_icon, LV_OBJ_FLAG_HIDDEN);
			}
		} else if (LV_ABS(distance) == ICON_CIRCLE_RAD) {
			for (uint16_t j = 0; j < 3; j++) {
				if(skip_id + j >= D_ICON_NUM)
					break;
				simple_img_set_scale(icon_data[j].obj_icon, ICON_LOOP_ZOOM);
				lv_obj_remove_flag(icon_data[j].obj_icon, LV_OBJ_FLAG_HIDDEN);
				if(j == 1) {
					lv_obj_set_pos(icon_data[j].obj_icon, DEF_UI_WIDTH/2 - offset_x, DEF_UI_HEIGHT/2 + ICON_CIRCLE_RAD - offset_x);
				} else {
					int32_t target_xc = target_x;
					if(j == 2)
						target_xc = -target_x;
					lv_obj_set_pos(icon_data[j].obj_icon, DEF_UI_WIDTH/2 - target_xc - offset_x, DEF_UI_HEIGHT/2 + target_y - offset_x);
				}
			}
		} else {
			int32_t offset_y = 0;
			int32_t zoom = 0;
			if(b_calculate) {
				distance = distance > 0 ? distance - ICON_CIRCLE_RAD : distance + ICON_CIRCLE_RAD;
				int32_t surplus_value = distance % ICON_CIRCLE_RAD;
				surplus_value = ui_map(surplus_value, -ICON_CIRCLE_RAD, ICON_CIRCLE_RAD, -ICON_INTERVAL_Y - img_dsc->header.w, ICON_INTERVAL_Y + img_dsc->header.w);
				offset_y = (distance / ICON_CIRCLE_RAD) * (ICON_INTERVAL_Y + img_dsc->header.w) + surplus_value;
				if(distance / ICON_CIRCLE_RAD)
					zoom = ICON_MIN_ZOOM;
				else
					zoom = ui_map(LV_ABS(surplus_value), 0, ICON_INTERVAL_Y + img_dsc->header.w, ICON_LOOP_ZOOM, ICON_MIN_ZOOM);
				int32_t offset_zoom = offset_x * zoom / LV_SCALE_NONE;
				if(LV_ABS(offset_y + target_y - offset_zoom) > DEF_UI_HEIGHT / 2)
					b_calculate = false;
			}
			if(b_calculate) {
				for (uint16_t j = 0; j < 3; j++) {
					if(skip_id + j >= D_ICON_NUM)
						break;
					simple_img_set_scale(icon_data[j].obj_icon, zoom);
					if (j == 1) {
						lv_obj_set_pos(icon_data[j].obj_icon, DEF_UI_WIDTH/2 - offset_x, DEF_UI_HEIGHT/2  + offset_y + ICON_CIRCLE_RAD - offset_x);
					} else {
						int32_t target_xc = target_x;
						if(j == 2)
							target_xc = -target_x;
						lv_obj_set_pos(icon_data[j].obj_icon, DEF_UI_WIDTH/2 - target_xc - offset_x, DEF_UI_HEIGHT/2 + offset_y + target_y - offset_x);
					}
					lv_obj_remove_flag(icon_data[j].obj_icon, LV_OBJ_FLAG_HIDDEN);
				}
			} else {
				for (uint16_t j = 0; j < 3; j++) {
					if(skip_id + j >= D_ICON_NUM)
						break;
					lv_obj_add_flag(icon_data[j].obj_icon, LV_OBJ_FLAG_HIDDEN);
				}
			}
		}
		skip_y += ICON_CIRCLE_RAD;
		skip_id += 3;
	}
	skip_id = middle_id - 3;
	skip_y = offset_y - ICON_CIRCLE_RAD;
	b_calculate = true;
	for (uint16_t i = 0; i < D_ICON_NUM/ 3; i++) {
		if(skip_id < 0 || skip_id >= D_ICON_NUM)
			break;
		int32_t distance = skip_y;
		data_circle_t *icon_data = icon_buf + skip_id;
		//inexistence LV_ABS(distance) < ICON_CIRCLE_RAD
		if (LV_ABS(distance) == ICON_CIRCLE_RAD) {
			for (uint16_t j = 0; j < 3; j++) {
				if(skip_id + j >= D_ICON_NUM)
					continue;
				simple_img_set_scale(icon_data[j].obj_icon, ICON_LOOP_ZOOM);
				lv_obj_remove_flag(icon_data[j].obj_icon, LV_OBJ_FLAG_HIDDEN);
				if(j == 1) {
					lv_obj_set_pos(icon_data[j].obj_icon, DEF_UI_WIDTH/2 - offset_x, DEF_UI_HEIGHT/2 - ICON_CIRCLE_RAD - offset_x);
				} else {
					int32_t target_xc = target_x;
					if(j == 2)
						target_xc = -target_x;
					lv_obj_set_pos(icon_data[j].obj_icon, DEF_UI_WIDTH/2 - target_xc - offset_x, DEF_UI_HEIGHT/2 - target_y - offset_x);
				}
			}
		} else {
			int32_t offset_y = 0;
			int32_t zoom = 0;
			if(b_calculate) {
				distance = distance > 0 ? distance - ICON_CIRCLE_RAD : distance + ICON_CIRCLE_RAD;
				int32_t surplus_value = distance % ICON_CIRCLE_RAD;
				surplus_value = ui_map(surplus_value, -ICON_CIRCLE_RAD, ICON_CIRCLE_RAD, -ICON_INTERVAL_Y - img_dsc->header.w, ICON_INTERVAL_Y + img_dsc->header.w);
				offset_y = (distance / ICON_CIRCLE_RAD) * (ICON_INTERVAL_Y + img_dsc->header.w) + surplus_value;
				if(distance / ICON_CIRCLE_RAD)
					zoom = ICON_MIN_ZOOM;
				else
					zoom = ui_map(LV_ABS(surplus_value), 0, ICON_INTERVAL_Y + img_dsc->header.w, ICON_LOOP_ZOOM, ICON_MIN_ZOOM);
				int32_t offset_zoom = offset_x * zoom / LV_SCALE_NONE;
				if(LV_ABS(offset_y + target_y - offset_zoom) > DEF_UI_HEIGHT / 2)
					b_calculate = false;
			}
			if(b_calculate) {
				for (uint16_t j = 0; j < 3; j++) {
					if(skip_id + j >= D_ICON_NUM)
						continue;
					simple_img_set_scale(icon_data[j].obj_icon, zoom);
					if (j == 1) {
						lv_obj_set_pos(icon_data[j].obj_icon, DEF_UI_WIDTH/2 - offset_x, DEF_UI_HEIGHT/2  + offset_y - ICON_CIRCLE_RAD - offset_x);
					} else {
						int32_t target_xc = target_x;
						if(j == 2)
							target_xc = -target_x;
						lv_obj_set_pos(icon_data[j].obj_icon, DEF_UI_WIDTH/2 - target_xc - offset_x, DEF_UI_HEIGHT/2 + offset_y - target_y - offset_x);
					}
					lv_obj_remove_flag(icon_data[j].obj_icon, LV_OBJ_FLAG_HIDDEN);
				}
			} else {
				for (uint16_t j = 0; j < 3; j++) {
					if(skip_id + j >= D_ICON_NUM)
						continue;
					lv_obj_add_flag(icon_data[j].obj_icon, LV_OBJ_FLAG_HIDDEN);
				}
			}
		}
		skip_y -= ICON_CIRCLE_RAD;
		skip_id -= 3;
	}
}

static int32_t _circle_icon_calibration(applist_circle_view_t *circle_view, int32_t move_y)
{
	int32_t all_y = circle_view->move_y + move_y;
	int32_t margin_y = all_y % ICON_CIRCLE_RAD;
	if(margin_y) {
		if(LV_ABS(margin_y) >= ICON_CIRCLE_RAD / 2) {
			if(margin_y > 0)
				margin_y = (all_y / ICON_CIRCLE_RAD + 1) * ICON_CIRCLE_RAD;
			else
				margin_y = (all_y / ICON_CIRCLE_RAD - 1) * ICON_CIRCLE_RAD;
		} else {
			margin_y = all_y / ICON_CIRCLE_RAD * ICON_CIRCLE_RAD;
		}
	} else {
		margin_y = all_y;
	}
	return margin_y - circle_view->move_y;
}

static void _circle_icon_anim(void * var, int32_t v)
{
	applist_circle_view_t *circle_view = var;
	circle_view->move_y = v;
	_circle_icon_set_pos(circle_view);
}

static void _circle_icon_anim_create(applist_circle_view_t *circle_view, lv_coord_t st_y, lv_coord_t end_y)
{
	uint32_t time = LV_ABS(st_y - end_y) * 100 / D_ICON_ANIM_SPEED;
	if(time > 500)
		time = 500;
	if(time)
	{
		lv_anim_t a;
		lv_anim_init(&a);
		lv_anim_set_var(&a, circle_view);
		lv_anim_set_duration(&a, time);
		lv_anim_set_values(&a, st_y, end_y);
		lv_anim_set_exec_cb(&a, _circle_icon_anim);
		lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
		lv_anim_start(&a);
	}
}

static void _circle_icon_anim_del(applist_circle_view_t *circle_view)
{
	lv_anim_delete(circle_view,_circle_icon_anim);
}

static void _circle_scroll_event_cb(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	applist_ui_data_t *data = lv_event_get_user_data(e);
	applist_circle_view_t *circle_view = &data->circle_view;
	lv_indev_t *indev = lv_indev_get_act();
	lv_point_t point = {0};
	lv_indev_get_vect(indev,&point);
	switch (code)
	{
	case LV_EVENT_PRESSED:
		_circle_icon_anim_del(circle_view);
		circle_view->y_all = 0;
		circle_view->y_all_tick = lv_tick_get();
		break;
	case LV_EVENT_PRESSING:
	case LV_EVENT_RELEASED:
		circle_view->y_all += point.y;
		circle_view->move_y += point.y;
		if(circle_view->move_y > 0)
			circle_view->move_y = 0;
		if(circle_view->move_y < circle_view->min_y)
			circle_view->move_y = circle_view->min_y;
		if(code == LV_EVENT_RELEASED) {
			lv_coord_t tick = lv_tick_elaps(circle_view->y_all_tick);
			lv_coord_t d_angle = 0;
			lv_coord_t test = 0;
			if(tick) {
				lv_coord_t add_distance = circle_view->y_all * D_ICON_SCROLL_SPEED / tick;
				d_angle = _circle_icon_calibration(circle_view, add_distance);
				test = add_distance;
			}
			else
				d_angle = _circle_icon_calibration(circle_view, 0);
			lv_coord_t st_y = circle_view->move_y;
			circle_view->move_y += d_angle;
			if(circle_view->move_y > 0)
				circle_view->move_y = 0;
			if(circle_view->move_y < circle_view->min_y)
				circle_view->move_y = circle_view->min_y;
			_circle_icon_anim_create(circle_view, st_y, circle_view->move_y);
		} else {
			_circle_icon_set_pos(circle_view);
		}
		break;
	default:
		break;
	}
}

static int _circle_view_create(lv_obj_t * scr)
{
	applist_ui_data_t *data = lv_obj_get_user_data(scr);
	data->cont = lv_obj_create(scr);
	lv_obj_set_pos(data->cont,0,0);
	lv_obj_set_size(data->cont,DEF_UI_WIDTH,DEF_UI_HEIGHT);
	lv_obj_set_style_pad_all(data->cont,0,LV_PART_MAIN);
	lv_obj_set_style_border_width(data->cont,0,LV_PART_MAIN);
	lv_obj_set_scroll_dir(data->cont, LV_DIR_NONE);
	lv_obj_set_scrollbar_mode(data->cont, LV_SCROLLBAR_MODE_OFF); /* default no scrollbar */
	lv_obj_add_event_cb(data->cont, _circle_scroll_event_cb, LV_EVENT_ALL, data);

	data_circle_t *icon_buf = app_mem_malloc(sizeof(data_circle_t)*D_ICON_NUM);
	if(icon_buf == NULL)
		return -ENOMEM;
	data->circle_view.user_data = icon_buf;
	uint16_t align_num = D_ICON_NUM;
	if(D_ICON_NUM % 3)
		align_num = D_ICON_NUM / 3 + 1;
	else
		align_num = D_ICON_NUM / 3;
	data->circle_view.min_y = -(align_num - 3) * ICON_CIRCLE_RAD;
	for(uint16_t i = 0; i < D_ICON_NUM; i++) {
		data_circle_t *icon_data = icon_buf + i;
		icon_data->obj_icon = simple_img_create(data->cont);
		int32_t img_id = i % NUM_ITEMS;
		const lv_img_dsc_t * img_src = applist_get_icon(data, img_id);
		simple_img_set_src(icon_data->obj_icon, img_src);
		simple_img_set_pivot(icon_data->obj_icon, img_src->header.w / 2, img_src->header.h / 2);
		lv_obj_add_flag(icon_data->obj_icon, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_event_cb(icon_data->obj_icon, applist_btn_event_def_handler, LV_EVENT_SHORT_CLICKED, data);
		lv_obj_add_flag(icon_data->obj_icon, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_flag(icon_data->obj_icon, LV_OBJ_FLAG_EVENT_BUBBLE);
		lv_obj_set_user_data(icon_data->obj_icon,(void *)img_id);
	}
	int16_t scrl_y = 0;
	data->presenter->load_scroll_value(NULL, &scrl_y);
	data->circle_view.move_y = scrl_y;
	lv_coord_t d_angle = _circle_icon_calibration(&data->circle_view, 0);
	data->circle_view.move_y += d_angle;
	_circle_icon_set_pos(&data->circle_view);
	return 0;
}

static void _circle_view_delete(lv_obj_t * scr)
{
	applist_ui_data_t *data = lv_obj_get_user_data(scr);
	applist_circle_view_t *circle_view = &data->circle_view;
	if(circle_view) {
		data->presenter->save_scroll_value(0, circle_view->move_y);
		_circle_icon_anim_del(circle_view);
		app_mem_free(circle_view->user_data);
		circle_view->user_data = NULL;
	}
}

static void _circle_view_update_icon(applist_ui_data_t * data, uint8_t idx, const lv_img_dsc_t * src)
{
	applist_circle_view_t *circle_view = &data->circle_view;
	data_circle_t *icon_buf = (data_circle_t *)circle_view->user_data;
	if(simple_img_get_src(icon_buf[idx].obj_icon) != src) {
		simple_img_set_src(icon_buf[idx].obj_icon, src);
		for(uint16_t i = 0; i < D_ICON_NUM; i++) {
			idx += NUM_ITEMS;
			if(idx >= D_ICON_NUM)
				break;
			simple_img_set_src(icon_buf[idx].obj_icon, src);
		}
	}
}
