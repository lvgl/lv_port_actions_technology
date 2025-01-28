/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os_common_api.h>
#include <widgets/simple_img.h>
#include "applist_view_inner.h"
#include <math.h>
//#include <mem_manager.h>

#define D_ICON_ZOOM LV_SCALE_NONE
#define D_ICON_ROLLER_FACTOR 3
#define D_ICON_ROW_NUM 3
#define D_ICON_INTERVAL_X 10
#define D_ICON_INTERVAL_Y 8
#define D_ICON_MIN_OPA LV_OPA_0
#define D_ICON_ANIM_SPEED  50  
#define D_ICON_SCROLL_SPEED  500  
#define  PI   3.14159265358979323846

typedef struct {
	lv_obj_t *icon_obj[NUM_ICONS];
	lv_obj_t *btn_obj[NUM_ICONS];
	lv_coord_t angle_offset;
	lv_coord_t min_angle;
	lv_coord_t interval_angle;
	lv_coord_t y_change;
	lv_coord_t y_all_tick;
	lv_coord_t y_all_change;
	lv_coord_t anim_angle;
} data_roller3D_t;

static int _roller3D_view_create(lv_obj_t * scr);
static void _roller3D_view_delete(lv_obj_t * scr);
static void _roller3D_view_update_icon(applist_ui_data_t * data, uint8_t idx, const lv_image_dsc_t * src);

const applist_view_cb_t g_applist_roller3D_view_cb = {
	.create = _roller3D_view_create,
	.delete = _roller3D_view_delete,
	.update_icon = _roller3D_view_update_icon,
};

static lv_coord_t roller3D_scroll_homing(data_roller3D_t *roller3D_data,lv_coord_t d_angle)
{
	lv_coord_t angle_offset = roller3D_data->angle_offset + d_angle;
	lv_coord_t residue_offset = angle_offset / roller3D_data->interval_angle;
	if(-angle_offset % roller3D_data->interval_angle >= roller3D_data->interval_angle / 2)
		residue_offset = (residue_offset - 1) * roller3D_data->interval_angle;
	else
		residue_offset = residue_offset * roller3D_data->interval_angle;
	return (residue_offset - roller3D_data->angle_offset);
}

static void roller3D_scroll_angle(data_roller3D_t *roller3D_data,lv_coord_t angle)
{
	if(angle)
	{
		lv_coord_t angle_offset = roller3D_data->angle_offset + angle;
		if(angle_offset > 0)
			angle -= angle_offset;
		else if(angle_offset < roller3D_data->min_angle)
			angle += roller3D_data->min_angle - angle_offset;
		lv_coord_t min_opa_angle = roller3D_data->interval_angle * 3;
		roller3D_data->angle_offset += (int16_t)angle;
		for(int32_t i = 0 ; i < NUM_ICONS ; i++)
		{
			lv_coord_t opa = LV_OPA_0;
			float offset_distance = (i / D_ICON_ROW_NUM) * roller3D_data->interval_angle + roller3D_data->angle_offset;
			if(LV_ABS(offset_distance) >= min_opa_angle)
				lv_obj_set_style_opa(roller3D_data->icon_obj[i],LV_OPA_0,LV_PART_MAIN);
			else
			{
				opa = LV_OPA_100 + LV_ABS(offset_distance) * (D_ICON_MIN_OPA - LV_OPA_100) / min_opa_angle;
				lv_obj_set_style_opa(roller3D_data->icon_obj[i],opa,LV_PART_MAIN);
			}
			face_map_set_angle_vect(roller3D_data->icon_obj[i],(int16_t)angle, 0, 0,true);
			lv_area_t coords = {0};
			if(opa != LV_OPA_0)
				face_map_get_area(roller3D_data->icon_obj[i], &coords);
			lv_memcpy(&roller3D_data->btn_obj[i]->coords,&coords,sizeof(lv_area_t));
		}
	}
}

static void _roller3D_scroll_anim(void * var, int32_t v)
{
	data_roller3D_t *roller3D_data = var;
	roller3D_scroll_angle(roller3D_data,v - roller3D_data->anim_angle);
	roller3D_data->anim_angle = v;
}

static void _roller3D_scroll_anim_del(data_roller3D_t *roller3D_data)
{
	lv_anim_delete(roller3D_data,_roller3D_scroll_anim);
}

static void _roller3D_scroll_anim_create(data_roller3D_t *roller3D_data, lv_coord_t at_angle, lv_coord_t go_angle)
{
	roller3D_data->anim_angle = at_angle;
	uint32_t time = LV_ABS(go_angle - at_angle) * 100 / D_ICON_ANIM_SPEED;
	if(time > 500)
		time = 500;
	if(time)
	{
		lv_anim_t a;
		lv_anim_init(&a);
		lv_anim_set_var(&a, roller3D_data);
		lv_anim_set_duration(&a, time);
		lv_anim_set_values(&a, at_angle, go_angle);
		lv_anim_set_exec_cb(&a, _roller3D_scroll_anim);
		lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
		lv_anim_start(&a);
	}
}

static void _roller3D_scroll_event_cb(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	applist_ui_data_t *data = lv_event_get_user_data(e);
	data_roller3D_t *roller3D_data = data->user_data;
	lv_point_t point = {0};
	lv_indev_t *indev = lv_indev_get_act();
	lv_indev_get_point(indev,&point);
	switch (code)
	{
		case LV_EVENT_PRESSED:
			_roller3D_scroll_anim_del(roller3D_data);
			roller3D_data->y_change = point.y;
			roller3D_data->y_all_tick = lv_tick_get();
			roller3D_data->y_all_change = point.y;
			break;
		case LV_EVENT_PRESSING:
		case LV_EVENT_RELEASED:
			roller3D_scroll_angle(roller3D_data, (point.y - roller3D_data->y_change) * D_ICON_ROLLER_FACTOR);
			roller3D_data->y_change = point.y;
			if(code == LV_EVENT_RELEASED)
			{
				lv_coord_t tick = lv_tick_elaps(roller3D_data->y_all_tick);
				lv_coord_t distance = point.y - roller3D_data->y_all_change;
				if(LV_ABS(distance) > DEF_CLICK_SCOPE)
				{
					lv_coord_t d_angle = 0;
					if(tick)
					{
						lv_coord_t add_distance = LV_ABS(distance) * D_ICON_SCROLL_SPEED / tick;
						if(distance < 0)
							add_distance = -add_distance;
						d_angle = roller3D_scroll_homing(roller3D_data,add_distance);
					}
					else
						d_angle = roller3D_scroll_homing(roller3D_data,0);
					_roller3D_scroll_anim_create(roller3D_data, roller3D_data->angle_offset , roller3D_data->angle_offset + d_angle);
				}
			}
			break;
		default:
			break;
	}
}

static int _roller3D_view_create(lv_obj_t * scr)
{
	applist_ui_data_t *data = lv_obj_get_user_data(scr);
	data_roller3D_t *roller3D_data = lv_malloc(sizeof(data_roller3D_t));
	if(!roller3D_data)
	{
		LV_LOG_ERROR("_roller3D_view_create alloc error");
		return -ENOMEM;
	}
	lv_memzero(roller3D_data,sizeof(data_roller3D_t));
	data->user_data = roller3D_data;
	data->cont = lv_obj_create(scr);
	lv_obj_set_pos(data->cont,0,0);
	lv_obj_set_size(data->cont,DEF_UI_WIDTH,DEF_UI_HEIGHT);
	lv_obj_set_style_pad_all(data->cont,0,LV_PART_MAIN);
	lv_obj_set_style_border_width(data->cont,0,LV_PART_MAIN);
	lv_obj_set_scroll_dir(data->cont, LV_DIR_NONE);
	lv_obj_set_scrollbar_mode(data->cont, LV_SCROLLBAR_MODE_OFF); /* default no scrollbar */
	lv_obj_add_event_cb(data->cont, _roller3D_scroll_event_cb, LV_EVENT_ALL, data);
	coder_simulation_tp_register(data->cont, APPLIST_VIEW, LV_DIR_VER, 50, NULL);

	lv_coord_t w = data->icon[0].header.w,h = data->icon[0].header.h;
	lv_coord_t centre_x = w/2, centre_y = h/2, centre_z = DEF_UI_HEIGHT*4/9;
	lv_coord_t w_interval = w * D_ICON_ZOOM / LV_SCALE_NONE + D_ICON_INTERVAL_X;
	lv_coord_t h_interval = h * D_ICON_ZOOM / LV_SCALE_NONE + D_ICON_INTERVAL_Y;
	lv_coord_t st_x = (DEF_UI_WIDTH - (w_interval * D_ICON_ROW_NUM - D_ICON_INTERVAL_X) + w_interval - D_ICON_INTERVAL_X - w) / 2;
	lv_coord_t st_y = (DEF_UI_WIDTH - h) / 2;
	const vertex_t verts[4] = {
		{-centre_x		,-centre_y		,-centre_z},
		{w - centre_x	,-centre_y		,-centre_z},
		{w - centre_x	,h - centre_y	,-centre_z},
		{-centre_x		,h - centre_y	,-centre_z},
	};
	lv_coord_t angle = (lv_coord_t)(asin((float)h_interval/(float)(centre_z*2)) * 1800 * 2 / PI);
	roller3D_data->interval_angle = angle == 0 ? 1 : angle;
	roller3D_data->min_angle = -NUM_ICONS / D_ICON_ROW_NUM * angle;
	for(int32_t i = 0 ; i < NUM_ICONS ; i++)
	{
		roller3D_data->icon_obj[i] = face_map_create(data->cont);
		lv_obj_set_pos(roller3D_data->icon_obj[i],st_x + (i % D_ICON_ROW_NUM) * w_interval,st_y);
		face_map_set_src(roller3D_data->icon_obj[i], applist_get_icon(data,i));
		face_map_set_periphery_dot(roller3D_data->icon_obj[i],(vertex_t *)verts);
		face_map_set_pivot(roller3D_data->icon_obj[i],centre_x,centre_y,centre_z);
		face_map_set_angle_vect(roller3D_data->icon_obj[i],(i / D_ICON_ROW_NUM) * (int16_t)angle, 0, 0,false);
		if(i % D_ICON_ROW_NUM == 0)
			face_map_set_observe(roller3D_data->icon_obj[i],w_interval,0,centre_z * 5,true);
		else if(i % D_ICON_ROW_NUM == 1)
			face_map_set_observe(roller3D_data->icon_obj[i],0,0,centre_z * 5,true);
		else
			face_map_set_observe(roller3D_data->icon_obj[i],-w_interval,0,centre_z * 5,true);
		face_map_set_normals(roller3D_data->icon_obj[i],true);
		face_map_set_zoom(roller3D_data->icon_obj[i],D_ICON_ZOOM);
		lv_obj_add_flag(roller3D_data->icon_obj[i], LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_flag(roller3D_data->icon_obj[i], LV_OBJ_FLAG_EVENT_BUBBLE);
		lv_obj_update_layout(roller3D_data->icon_obj[i]);
		roller3D_data->btn_obj[i] = lv_button_create(data->cont);
		lv_obj_add_flag(roller3D_data->btn_obj[i], LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_flag(roller3D_data->btn_obj[i], LV_OBJ_FLAG_EVENT_BUBBLE);
		lv_obj_set_user_data(roller3D_data->btn_obj[i],(void *)i);
		lv_obj_add_event_cb(roller3D_data->btn_obj[i], applist_btn_event_def_handler, LV_EVENT_SHORT_CLICKED, data);
	}
	int16_t scrl_y = 0;
	data->presenter->load_scroll_value(NULL, &scrl_y);
	lv_coord_t d_angle = roller3D_scroll_homing(roller3D_data,scrl_y);
	if(d_angle == 0)
		d_angle = 1;
	roller3D_scroll_angle(roller3D_data,d_angle);
	return 0;
}

static void _roller3D_view_delete(lv_obj_t * scr)
{
	applist_ui_data_t *data = lv_obj_get_user_data(scr);
	data_roller3D_t *roller3D_data = data->user_data;
	if(roller3D_data)
	{
		data->presenter->save_scroll_value(0, roller3D_data->angle_offset);
		_roller3D_scroll_anim_del(roller3D_data);
		lv_free(roller3D_data);
		data->user_data = NULL;
	}
}

static void _roller3D_view_update_icon(applist_ui_data_t * data, uint8_t idx, const lv_image_dsc_t * src)
{
	data_roller3D_t *roller3D_data = data->user_data;
	if(face_map_get_src(roller3D_data->icon_obj[idx]) != src)
		face_map_set_src(roller3D_data->icon_obj[idx], src);
}
