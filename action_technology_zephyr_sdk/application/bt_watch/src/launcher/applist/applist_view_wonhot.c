/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os_common_api.h>
#include <widgets/simple_img.h>
#include "applist_view_inner.h"
//#include <mem_manager.h>


#define D_ICON_NUM	8	//icon num
#define D_ICON_TIER 4	//row num
#define D_ICON_MIN_ZOOM (LV_SCALE_NONE>>1)
#define D_ICON_MAX_ZOOM	(LV_SCALE_NONE + 100)
#define D_ICON_ZOOM_SIDE  (((82*D_ICON_MAX_ZOOM)/LV_SCALE_NONE)>>1)
#define D_ICON_RADIUS ((DEF_UI_WIDTH>>1) - D_ICON_ZOOM_SIDE)	//max radius
#define D_ICON_MOVE_RATIO 12 	//move coord ratio 10 = 1
#define D_ICON_ACCELERATED_SPEED 2 //auto roll rate of decay
#define D_ICON_SPEED  100  //auto roll time

typedef struct {
	lv_obj_t *obj_icon;
	int32_t angle;
	int32_t radius;
} data_wonhot_t;

static int _wonhot_view_create(lv_obj_t * scr);
static void _wonhot_view_delete(lv_obj_t * scr);
static void _wonhot_view_update_icon(applist_ui_data_t * data, uint8_t idx, const lv_image_dsc_t * src);

const applist_view_cb_t g_applist_wonhot_view_cb = {
	.create = _wonhot_view_create,
	.delete = _wonhot_view_delete,
	.update_icon = _wonhot_view_update_icon,
};

static int32_t _wonhot_radius_correct(applist_ui_data_t *data,int32_t radius)
{
	while(radius >= data->wonhot_view.max_radius)
		radius -= data->wonhot_view.max_radius;
	while(radius < 0)
		radius += data->wonhot_view.max_radius;
	return radius;
}

static void _wonhot_icon_x_y(applist_ui_data_t *data,int32_t radius)
{
	data_wonhot_t *icon_buf = (data_wonhot_t *)data->wonhot_view.user_data;
	int32_t icon_radius = (3 * (D_ICON_RADIUS>>1));
	for(uint32_t i=0;i<data->wonhot_view.icon_num;i++)
	{
		data_wonhot_t *icon_data = icon_buf+i;
		int32_t at_radius = icon_data->radius + radius;
		if(at_radius >= data->wonhot_view.max_radius)
			at_radius -= data->wonhot_view.max_radius;
		else if(at_radius < 0)
			at_radius += data->wonhot_view.max_radius;
		if(at_radius == 0 || at_radius >= icon_radius)
		{
			lv_obj_add_flag(icon_data->obj_icon,LV_OBJ_FLAG_HIDDEN);
			//simple_img_set_scale(icon_data->obj_icon,LV_SCALE_NONE);
		}
		else
		{
			int32_t x = (lv_trigo_sin(icon_data->angle) * at_radius) >> LV_TRIGO_SHIFT;
			x += data->icon[0].header.w>>1;
			x = (DEF_UI_WIDTH>>1) - x;
			int32_t y = (lv_trigo_cos(icon_data->angle) * at_radius) >> LV_TRIGO_SHIFT;
			y += data->icon[0].header.h>>1;
			y = (DEF_UI_HEIGHT>>1) - y;
			lv_obj_set_pos(icon_data->obj_icon, x , y);
			int32_t icon_zoom =  (D_ICON_MAX_ZOOM - D_ICON_MIN_ZOOM) * at_radius / icon_radius + D_ICON_MIN_ZOOM;
			simple_img_set_scale(icon_data->obj_icon,icon_zoom);
			lv_obj_remove_flag(icon_data->obj_icon,LV_OBJ_FLAG_HIDDEN);
		}
	}
}

static int32_t _wonhot_icon_radius_calibration(int32_t radius)
{
	int32_t ret_radius = LV_ABS(radius);
	ret_radius = ret_radius % (D_ICON_RADIUS >> 1);

	if(ret_radius > (D_ICON_RADIUS >> 2))
	{
		if(radius < 0)
			ret_radius = ((radius/(D_ICON_RADIUS >> 1))-1) * (D_ICON_RADIUS >> 1);
		else
			ret_radius = ((radius/(D_ICON_RADIUS >> 1))+1) * (D_ICON_RADIUS >> 1);
	}
	else
	{
		ret_radius = (radius/(D_ICON_RADIUS >> 1)) * (D_ICON_RADIUS >> 1);
	}
	return ret_radius;
}

static void _wonhot_scroll_anim(void * var, int32_t v)
{
	applist_ui_data_t *data = (applist_ui_data_t *)var;
	int32_t radius = _wonhot_radius_correct(data,v);
	_wonhot_icon_x_y(data,radius);
}

static void _wonhot_deleted_anim(lv_anim_t * a)
{
	applist_ui_data_t *data = a->var;
	int32_t radius = _wonhot_radius_correct(data,a->current_value);
	_wonhot_icon_x_y(data,radius);
	data->wonhot_view.move_radius = radius;
	int16_t scrl_y = radius;
	data->presenter->save_scroll_value(0,scrl_y);
}

static void _wonhot_create_anim(applist_ui_data_t *data, uint32_t time, uint32_t anim_st, uint32_t anim_end)
{
	lv_anim_t a;
	lv_anim_init(&a);
	lv_anim_set_var(&a, data);
	lv_anim_set_duration(&a, time);
	lv_anim_set_values(&a, anim_st, anim_end);
	lv_anim_set_exec_cb(&a, _wonhot_scroll_anim);
	lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
	lv_anim_set_deleted_cb(&a, _wonhot_deleted_anim);
	lv_anim_start(&a);
}

static void _wonhot_scroll_event_cb(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	applist_ui_data_t *data = lv_event_get_user_data(e);
	lv_indev_t *indev = lv_indev_get_act();
	lv_point_t point = {0};
	lv_indev_get_point(indev,&point);
	switch (code)
	{
		case LV_EVENT_PRESSED:
			memcpy(&data->wonhot_view.first_point,&point,sizeof(lv_point_t));
			data->wonhot_view.move_time = lv_tick_get();
			data->wonhot_view.b_move = false;
			lv_anim_delete(data,_wonhot_scroll_anim);
		break;
		case LV_EVENT_PRESSING:
		case LV_EVENT_RELEASED:
			{
				int32_t x_move = (point.x - data->wonhot_view.first_point.x) * D_ICON_RADIUS * 10 / (DEF_UI_WIDTH * D_ICON_MOVE_RATIO);
				int32_t y_move = (point.y - data->wonhot_view.first_point.y) * D_ICON_RADIUS * 10 / (DEF_UI_HEIGHT * D_ICON_MOVE_RATIO);
				int32_t at_radius = x_move + y_move + data->wonhot_view.move_radius;
				int32_t radius = _wonhot_radius_correct(data,at_radius);
				if(radius != data->wonhot_view.move_radius)
				{
					data->wonhot_view.b_move = true;
					_wonhot_icon_x_y(data,radius);
				}
				if(code == LV_EVENT_RELEASED)
				{
					bool ret = true;
					int32_t diff = at_radius - data->wonhot_view.move_radius;
					if(radius != data->wonhot_view.move_radius && diff != 0)
					{
						int32_t r_time = lv_tick_get() - data->wonhot_view.move_time;
						if(r_time)
						{
							int32_t auto_time = (LV_ABS(diff) * 1000)/(r_time * D_ICON_ACCELERATED_SPEED);
							int32_t dx = 0;
							if(diff > 0)
								dx = (diff * auto_time / r_time) + ((D_ICON_ACCELERATED_SPEED * auto_time * auto_time/ 1000)>>1);
							else
								dx = (diff * auto_time / r_time) - ((D_ICON_ACCELERATED_SPEED * auto_time * auto_time/ 1000)>>1);
							//SYS_LOG_ERR("_wonhot_scroll_event_cb %d,%d,%d,%d",dx,auto_time,data->wonhot_view.angle,r_time);
							auto_time = auto_time*2;
							if(auto_time > 1000)
								auto_time = 1000;
							dx = _wonhot_icon_radius_calibration(dx + at_radius);
							if(auto_time != 0 && dx != at_radius)
							{
								ret = false;
								_wonhot_create_anim(data, auto_time, at_radius, dx);
							}
						}
					}
					if(ret)
					{
						int32_t dx = _wonhot_icon_radius_calibration(at_radius);
						if(at_radius != data->wonhot_view.move_radius || dx != at_radius)
							_wonhot_create_anim(data, D_ICON_SPEED , at_radius, dx);
					}
				}
			}
		break;
	default:
		break;
	}
}

static void _wonhot_event_icon_handler(lv_event_t * e)
{
	applist_ui_data_t *data = lv_event_get_user_data(e);
	if(!data->wonhot_view.b_move)
		applist_btn_event_def_handler(e);
}

static int _wonhot_view_create(lv_obj_t * scr)
{
	applist_ui_data_t *data = lv_obj_get_user_data(scr);
	data->cont = lv_obj_create(scr);
	lv_obj_set_pos(data->cont,0,0);
	lv_obj_set_size(data->cont,DEF_UI_WIDTH,DEF_UI_HEIGHT);
	lv_obj_set_style_pad_all(data->cont,0,LV_PART_MAIN);
	lv_obj_set_style_border_width(data->cont,0,LV_PART_MAIN);
	lv_obj_set_scroll_dir(data->cont, LV_DIR_NONE);
	lv_obj_set_scrollbar_mode(data->cont, LV_SCROLLBAR_MODE_OFF); /* default no scrollbar */
	lv_obj_add_event_cb(data->cont, _wonhot_scroll_event_cb, LV_EVENT_ALL, data);
	coder_simulation_tp_register(data->cont, APPLIST_VIEW, LV_DIR_VER, 50, NULL);

	uint32_t a_bed_num = D_ICON_NUM;//(D_ICON_NUM>>1)<<1;
	uint32_t bed_num = D_ICON_TIER < 4 ? 4 : D_ICON_TIER;
	uint32_t icon_num = a_bed_num * bed_num;
	data->wonhot_view.icon_num = icon_num;
	data->wonhot_view.max_radius = bed_num * (D_ICON_RADIUS>>1);
	data_wonhot_t *icon_buf = app_mem_malloc(sizeof(data_wonhot_t)*icon_num);
	if(icon_buf == NULL)
		return -ENOMEM;
	memset(icon_buf, 0, sizeof(data_wonhot_t)*icon_num);
	for(uint32_t i=0;i<icon_num;i++)
	{
		data_wonhot_t *icon_data = icon_buf+i;
		icon_data->obj_icon = simple_img_create(data->cont);
		int32_t at_bed = i / a_bed_num;
		icon_data->radius = at_bed * (D_ICON_RADIUS>>1);
		if(at_bed % 2)
			icon_data->angle = (360 * (i % a_bed_num) + 180) / a_bed_num;
		else
			icon_data->angle = 360 * (i % a_bed_num) / a_bed_num;
		int32_t img_id = i % NUM_ITEMS;
		simple_img_set_src(icon_data->obj_icon,applist_get_icon(data, img_id));
		simple_img_set_pivot(icon_data->obj_icon,data->icon[0].header.w>>1,data->icon[0].header.h>>1);
		icon_data->obj_icon->user_data = (void *)img_id;
		lv_obj_add_event_cb(icon_data->obj_icon, _wonhot_event_icon_handler, LV_EVENT_SHORT_CLICKED, data);
		lv_obj_add_flag(icon_data->obj_icon,LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_flag(icon_data->obj_icon,LV_OBJ_FLAG_EVENT_BUBBLE);
	}
	data->wonhot_view.user_data = icon_buf;

	int16_t scrl_y = 0;
	data->presenter->load_scroll_value(0,&scrl_y);
	int32_t dx = _wonhot_icon_radius_calibration(scrl_y);
	_wonhot_icon_x_y(data,dx);
	data->wonhot_view.move_radius = dx;
	return 0;
}

static void _wonhot_view_delete(lv_obj_t * scr)
{
	applist_ui_data_t *data = lv_obj_get_user_data(scr);
	lv_anim_delete(data,_wonhot_scroll_anim);
	app_mem_free(data->wonhot_view.user_data);
	//data->presenter->save_scroll_value(0, lv_obj_get_scroll_y(data->cont));
}

static void _wonhot_view_update_icon(applist_ui_data_t * data, uint8_t idx, const lv_image_dsc_t * src)
{
	data_wonhot_t *icon_buf = (data_wonhot_t *)data->wonhot_view.user_data;
	for(uint32_t i=0;i<data->wonhot_view.icon_num;i++)
	{
		data_wonhot_t *icon_data = icon_buf+i;
		if((int)(icon_data->obj_icon->user_data) == idx)
			simple_img_set_src(icon_data->obj_icon, applist_get_icon(data, idx));
	}
}


