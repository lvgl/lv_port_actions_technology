/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os_common_api.h>
#include <widgets/simple_img.h>
#include "applist_view_inner.h"
//#include <mem_manager.h>

#define D_ICON_NUM	10	//icon num
#define D_ICON_SPEED  100  //homing time
#define D_CIRCLE_RADIUS 180 //radius
#define D_ICON_ACCELERATED_SPEED 5 //auto roll rate of decay
#define D_ICON_ZOOM	LV_SCALE_NONE + 128

typedef struct {
	lv_obj_t *obj_icon;
	int32_t angle;
} data_turntable_t;

static int _turntable_view_create(lv_obj_t * scr);
static void _turntable_view_delete(lv_obj_t * scr);
static void _turntable_view_update_icon(applist_ui_data_t * data, uint8_t idx, const lv_image_dsc_t * src);

const applist_view_cb_t g_applist_turntable_view_cb = {
	.create = _turntable_view_create,
	.delete = _turntable_view_delete,
	.update_icon = _turntable_view_update_icon,
};

static void _turntable_icon_x_y(applist_ui_data_t *data,lv_obj_t *icon,int32_t angle)
{
	int32_t x = lv_trigo_sin(angle) * D_CIRCLE_RADIUS / LV_TRIGO_SIN_MAX;
	x += data->icon[0].header.w>>1;
	x = (DEF_UI_WIDTH>>1) - x;
	int32_t y = lv_trigo_cos(angle) * D_CIRCLE_RADIUS / LV_TRIGO_SIN_MAX;
	y += data->icon[0].header.h>>1;
	y = (DEF_UI_HEIGHT>>1) - y;
	lv_obj_set_pos(icon, x , y);
}

static int32_t _turntable_angle_correct(int32_t angle)
{
	while(angle >= 360)
		angle -= 360;
	while(angle < 0)
		angle += 360;
	return angle;
}

static int32_t _turntable_icon_angle_calibration(applist_ui_data_t *data,int32_t angle)
{
	data_turntable_t *icon_data = (data_turntable_t *)data->turntable_view.user_data;
	int32_t middle_angle = 360 / data->turntable_view.icon_num;
	int32_t icon_angle = _turntable_angle_correct(icon_data->angle + angle);
	int32_t ret_angle = icon_angle % middle_angle;

	if(ret_angle > middle_angle >> 1)
	{
		if(angle < 0)
			ret_angle = -ret_angle;
		else
			ret_angle = middle_angle - ret_angle;
		//SYS_LOG_ERR("ret_angle > middle_angle %d,%d",ret_angle,angle);
	}
	else
	{
		if(angle < 0)
			ret_angle = middle_angle - ret_angle;
		else
			ret_angle = -ret_angle;
		//SYS_LOG_ERR("ret_angle < middle_angle %d,%d",ret_angle,angle);
	}

	return ret_angle;
}

static void _turntable_icon_change(bool save,applist_ui_data_t *data,int32_t angle)
{
	data_turntable_t *icon_buf = (data_turntable_t *)data->turntable_view.user_data;
	data_turntable_t *icon_data = icon_buf;
	int32_t icon_angle = icon_data->angle + angle;
	int32_t diff_angle = data->turntable_view.icon_up_angle - icon_angle;
	for(uint32_t i=0;i<data->turntable_view.icon_num;i++)
	{
		icon_data = icon_buf+i;
		icon_angle = _turntable_angle_correct(icon_data->angle + angle);
		_turntable_icon_x_y(data,icon_data->obj_icon,icon_angle);
		if(save)
			icon_data->angle = icon_angle;
		int32_t middle_angle = (360 / data->turntable_view.icon_num) >> 1;
		if(icon_angle >= 360 - middle_angle || icon_angle < middle_angle)
		{
			lv_image_dsc_t *icon_img = (lv_image_dsc_t *)simple_img_get_src(icon_data->obj_icon);
			lv_image_dsc_t *middle_img = (lv_image_dsc_t *)simple_img_get_src(data->turntable_view.middle_icon);
			if(icon_img != middle_img)
			{
				simple_img_set_src(data->turntable_view.middle_icon,icon_img);
				data->turntable_view.middle_icon->user_data = icon_data->obj_icon->user_data;
				if(diff_angle != 0)
				{
					data_turntable_t *icon_own = NULL;
					int32_t img_id = 0;
					if(diff_angle < 0)
					{
						int32_t icon_id = i + (data->turntable_view.icon_num >> 1);
						if(icon_id >= data->turntable_view.icon_num)
							icon_id -= data->turntable_view.icon_num;
						icon_own = icon_buf+icon_id;
						icon_id++;
						if(icon_id >= data->turntable_view.icon_num)
							icon_id -= data->turntable_view.icon_num;
						data_turntable_t *icon_next = icon_buf+icon_id;
						img_id = (int32_t)icon_next->obj_icon->user_data;
						img_id--;
					}
					else
					{
						int32_t icon_id = i + (data->turntable_view.icon_num >> 1) - 1;
						if(icon_id >= data->turntable_view.icon_num)
							icon_id -= data->turntable_view.icon_num;
						icon_own = icon_buf+icon_id;
						icon_id--;
						if(icon_id < 0)
							icon_id += data->turntable_view.icon_num;
						data_turntable_t *icon_up = icon_buf+icon_id;
						img_id = (int32_t)icon_up->obj_icon->user_data;
						img_id++;
					}
					if(img_id >= NUM_ITEMS)
						img_id -= NUM_ITEMS;
					else if(img_id < 0)
						img_id += NUM_ITEMS;
					simple_img_set_src(icon_own->obj_icon , applist_get_icon(data,img_id));
					icon_own->obj_icon->user_data = (void *)(img_id);

					int32_t icon_id = i + (data->turntable_view.icon_num >> 1);
					if(icon_id >= data->turntable_view.icon_num)
						icon_id -= data->turntable_view.icon_num;
					icon_own = icon_buf+icon_id;
					icon_id = (int32_t)icon_own->obj_icon->user_data;
					int16_t scrl_y = icon_id;
					data->presenter->save_scroll_value(0,scrl_y);
				}
			}
		}
	}
	icon_data = icon_buf;
	if(save)
		data->turntable_view.icon_up_angle = _turntable_angle_correct(icon_data->angle);
	else
		data->turntable_view.icon_up_angle = icon_data->angle + angle;
}

static void _turntable_scroll_anim(void * var, int32_t v)
{
	applist_ui_data_t *data = var;
	_turntable_icon_change(false , data , v);
}

static void _turntable_deleted_anim(lv_anim_t * a)
{
	applist_ui_data_t *data = a->var;
	_turntable_icon_change(true , data , a->current_value);
}

static bool _turntable_get_angle(int32_t *g_angle,lv_point_t *at_point,lv_point_t *front_point)
{
	int32_t move_len = 0;
	int32_t angle = 0;
	bool ret = false;
	if(front_point->x > (DEF_UI_WIDTH >> 1))
	{
		move_len = -(at_point->y - front_point->y);
		if(at_point->x <= (DEF_UI_WIDTH >> 1))
			ret = true;
	}
	else
	{
		move_len = at_point->y - front_point->y;
		if(at_point->x > (DEF_UI_WIDTH >> 1))
			ret = true;
	}
	angle = move_len * 180 / DEF_UI_HEIGHT;
	if(front_point->y > (DEF_UI_HEIGHT >> 1))
	{
		move_len = at_point->x - front_point->x;
		if(at_point->y <= (DEF_UI_HEIGHT >> 1))
			ret = true;
	}
	else
	{
		move_len = -(at_point->x - front_point->x);
		if(at_point->y > (DEF_UI_HEIGHT >> 1))
			ret = true;
	}
	angle += move_len * 180 / DEF_UI_WIDTH;
	*g_angle = angle;
	return ret;
}

static void _turntable_scroll_event_cb(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	applist_ui_data_t *data = lv_event_get_user_data(e);
	lv_indev_t *indev = lv_indev_get_act();
	lv_point_t point = {0};
	lv_indev_get_point(indev,&point);
	switch (code)
	{
	case LV_EVENT_PRESSED:
		//memcpy(&data->turntable_view.first_point,&point,sizeof(lv_point_t));
		memcpy(&data->turntable_view.qua_point,&point,sizeof(lv_point_t));
		data->turntable_view.b_move = false;
		data->turntable_view.angle = 0;
		data->turntable_view.move_time = lv_tick_get();
		lv_anim_delete(data,_turntable_scroll_anim);
		break;
	case LV_EVENT_PRESSING:
	case LV_EVENT_RELEASED:
		{
			int32_t angle = 0;
			bool ret = false;
			ret = _turntable_get_angle(&angle, &point, &data->turntable_view.qua_point);

			if(ret)
			{
				memcpy(&data->turntable_view.qua_point,&point,sizeof(lv_point_t));
				data->turntable_view.angle += angle;
				angle = 0;
			}
			if(angle + data->turntable_view.angle != 0)
			{
				data->turntable_view.b_move = true;
				bool save = false;
				if(LV_EVENT_RELEASED == code)
					save = true;
				_turntable_icon_change(save , data , data->turntable_view.angle + angle);
			}

			if(LV_EVENT_RELEASED == code)
			{
				ret = true;
				angle += data->turntable_view.angle;
				int32_t r_time = lv_tick_get() - data->turntable_view.move_time;
				if(angle != 0 && r_time)
				{
					lv_point_t v_point = {0};
					lv_indev_get_vect(indev,&v_point);
					//SYS_LOG_ERR("LV_EVENT_RELEASED %d,%d",v_point.x,v_point.y);
					if(LV_ABS(v_point.x) > 15 || LV_ABS(v_point.y) > 15)
					{
						int32_t auto_time = (LV_ABS(angle) * 1000)/(r_time * D_ICON_ACCELERATED_SPEED);
						int32_t dx = 0;
						if(angle > 0)
							dx = (angle * auto_time / r_time) + ((D_ICON_ACCELERATED_SPEED * auto_time * auto_time/ 1000)>>1);
						else
							dx = (angle * auto_time / r_time) - ((D_ICON_ACCELERATED_SPEED * auto_time * auto_time/ 1000)>>1);
						//SYS_LOG_ERR("_turntable_scroll_event_cb %d,%d,%d,%d",dx,auto_time,data->turntable_view.angle,r_time);
						auto_time = auto_time*2;
						if(auto_time > 1000)
							auto_time = 1000;
						dx += _turntable_icon_angle_calibration(data,dx);
						if(auto_time != 0 && dx != 0)
						{
							ret = false;
							lv_anim_t a;
							lv_anim_init(&a);
							lv_anim_set_var(&a, data);
							lv_anim_set_duration(&a, auto_time);
							lv_anim_set_values(&a, 0, dx);
							lv_anim_set_exec_cb(&a, _turntable_scroll_anim);
							lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
							lv_anim_set_deleted_cb(&a, _turntable_deleted_anim);
							lv_anim_start(&a);
						}
					}
				}
				if(ret)
				{
					int32_t dx = _turntable_icon_angle_calibration(data,0);
					if(dx != 0)
					{
						lv_anim_t a;
						lv_anim_init(&a);
						lv_anim_set_var(&a, data);
						lv_anim_set_duration(&a, D_ICON_SPEED);
						lv_anim_set_values(&a, 0, dx);
						lv_anim_set_exec_cb(&a, _turntable_scroll_anim);
						lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
						lv_anim_set_deleted_cb(&a, _turntable_deleted_anim);
						lv_anim_start(&a);
					}
				}
			}
		}
		break;
	default:
		break;
	}
}

static void _turntable_event_icon_handler(lv_event_t * e)
{
	applist_ui_data_t *data = lv_event_get_user_data(e);
	if(!data->turntable_view.b_move)
		applist_btn_event_def_handler(e);
}

static int _turntable_view_create(lv_obj_t * scr)
{
	applist_ui_data_t *data = lv_obj_get_user_data(scr);
	data->cont = lv_obj_create(scr);
	lv_obj_set_pos(data->cont,0,0);
	lv_obj_set_size(data->cont,DEF_UI_WIDTH,DEF_UI_HEIGHT);
	lv_obj_set_style_pad_all(data->cont,0,LV_PART_MAIN);
	lv_obj_set_style_border_width(data->cont,0,LV_PART_MAIN);
	lv_obj_set_scroll_dir(data->cont, LV_DIR_NONE);
	lv_obj_set_scrollbar_mode(data->cont, LV_SCROLLBAR_MODE_OFF); /* default no scrollbar */
	lv_obj_add_event_cb(data->cont, _turntable_scroll_event_cb, LV_EVENT_ALL, data);
	coder_simulation_tp_register(data->cont, APPLIST_VIEW, LV_DIR_VER, 50, NULL);

	uint32_t icon_num = (D_ICON_NUM>>1)<<1;
	if(icon_num == 0)
		icon_num = 2;

	data_turntable_t *icon_buf = app_mem_malloc(sizeof(data_turntable_t)*icon_num);
	if(icon_buf == NULL)
		return -ENOMEM;
	memset(icon_buf, 0, sizeof(data_turntable_t)*icon_num);
	data->turntable_view.icon_num = icon_num;
	int16_t scrl_y = 0;
	data->presenter->load_scroll_value(NULL, &scrl_y);
	int32_t st_img_id = scrl_y;
	for(uint32_t i=0;i<icon_num;i++)
	{
		data_turntable_t *icon_data = icon_buf+i;
		icon_data->obj_icon = simple_img_create(data->cont);
		icon_data->angle = _turntable_angle_correct(360 * i / icon_num + 180);
		//_turntable_icon_x_y(data,icon_data->obj_icon,icon_data->angle);
		//int32_t img_id = (NUM_ITEMS - (icon_num>>1) + i) % NUM_ITEMS;
		int32_t img_id = (st_img_id + i) % NUM_ITEMS;
		simple_img_set_src(icon_data->obj_icon,applist_get_icon(data, img_id));
		simple_img_set_pivot(icon_data->obj_icon,data->icon[0].header.w>>1,data->icon[0].header.h>>1);
		lv_obj_add_event_cb(icon_data->obj_icon, _turntable_event_icon_handler, LV_EVENT_SHORT_CLICKED, data);
		icon_data->obj_icon->user_data = (void *)(img_id);
		lv_obj_add_flag(icon_data->obj_icon,LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_flag(icon_data->obj_icon,LV_OBJ_FLAG_EVENT_BUBBLE);
	}
	data->turntable_view.user_data = icon_buf;

	data->turntable_view.middle_icon = simple_img_create(data->cont);
	simple_img_set_pivot(data->turntable_view.middle_icon,data->icon[0].header.w>>1,data->icon[0].header.h>>1);
	simple_img_set_scale(data->turntable_view.middle_icon,D_ICON_ZOOM);
	lv_obj_align(data->turntable_view.middle_icon,LV_ALIGN_CENTER,0,0);
	lv_obj_add_flag(data->turntable_view.middle_icon,LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_event_cb(data->turntable_view.middle_icon, applist_btn_event_def_handler, LV_EVENT_SHORT_CLICKED, data);

	lv_obj_t *indicate_dot = lv_obj_create(data->cont);
	lv_obj_align(indicate_dot,LV_ALIGN_TOP_MID,0,
		(DEF_UI_HEIGHT>>1)-D_CIRCLE_RADIUS + (data->icon[0].header.h>>1) + 20);
	lv_obj_set_size(indicate_dot,6,6);
	lv_obj_set_style_pad_all(indicate_dot,0,LV_PART_MAIN);
	lv_obj_set_style_border_width(indicate_dot,0,LV_PART_MAIN);
	lv_obj_set_style_radius(indicate_dot,3,LV_PART_MAIN);
	lv_obj_set_style_bg_color(indicate_dot,lv_color_white(),LV_PART_MAIN);
	lv_obj_set_style_bg_opa(indicate_dot,LV_OPA_100,LV_PART_MAIN);

	data_turntable_t *icon_data = icon_buf;
	data->turntable_view.icon_up_angle = _turntable_angle_correct(icon_data->angle);
	_turntable_icon_change(false,data,0);
	//lv_obj_update_layout(data->cont);
	return 0;
}

static void _turntable_view_delete(lv_obj_t * scr)
{
	applist_ui_data_t *data = lv_obj_get_user_data(scr);
	lv_anim_delete(data,_turntable_scroll_anim);
	app_mem_free(data->turntable_view.user_data);
	data->turntable_view.user_data = NULL;
	//data->presenter->save_scroll_value(0, lv_obj_get_scroll_y(data->cont));
}

static void _turntable_view_update_icon(applist_ui_data_t * data, uint8_t idx, const lv_image_dsc_t * src)
{
	data_turntable_t *icon_buf = (data_turntable_t *)data->turntable_view.user_data;
	for(uint32_t i=0;i<data->turntable_view.icon_num;i++)
	{
		data_turntable_t *icon_data = icon_buf+i;
		if((int32_t)(icon_data->obj_icon->user_data) == idx)
			simple_img_set_src(icon_data->obj_icon, applist_get_icon(data, idx));
	}
	if((int32_t)data->turntable_view.middle_icon->user_data == idx)
		simple_img_set_src(data->turntable_view.middle_icon, applist_get_icon(data, idx));
}



