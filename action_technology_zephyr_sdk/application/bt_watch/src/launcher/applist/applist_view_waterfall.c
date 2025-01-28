/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os_common_api.h>
#include <widgets/simple_img.h>
#include "applist_view_inner.h"
//#include <mem_manager.h>


#define D_ICON_H_SPACE	30	//icon height interval
#define D_ICON_W_SPACE	80	//icon width interval
#define D_ICON_EXTEND_NUM 4	//up down extend icon num
#define D_ICON_ZOOM_MIN (LV_SCALE_NONE>>1)	//min zoom
#define D_ICON_ZOOM_MAX (LV_SCALE_NONE + 125)	//middle max zoom
#define D_SIDE_ICON_ZOOM_MAX (LV_SCALE_NONE + 25)	//two flanks max zoom
#define D_CIRCLE_RADIUS (DEF_UI_HEIGHT>>1)	//curve radius
#define D_CIRCLE_EXTEND 10	//crooked size



static int _waterfall_view_create(lv_obj_t * scr);
static void _waterfall_view_delete(lv_obj_t * scr);
static void _waterfall_view_update_icon(applist_ui_data_t * data, uint8_t idx, const lv_image_dsc_t * src);

const applist_view_cb_t g_applist_waterfall_view_cb = {
	.create = _waterfall_view_create,
	.delete = _waterfall_view_delete,
	.update_icon = _waterfall_view_update_icon,
};

static uint32_t D_ICON_NUM(applist_ui_data_t *data)
{
	lv_coord_t num = DEF_UI_HEIGHT/(data->icon[0].header.h + D_ICON_H_SPACE) + (D_ICON_EXTEND_NUM<<1);
	if(DEF_UI_HEIGHT%(data->icon[0].header.h + D_ICON_H_SPACE))
		num++;
	data->waterfall_view.scroll_recover = (D_ICON_EXTEND_NUM << 1) * (data->icon[0].header.h + D_ICON_H_SPACE);
	data->waterfall_view.centre_scroll_y = D_ICON_EXTEND_NUM * (data->icon[0].header.h + D_ICON_H_SPACE);
	data->waterfall_view.end_y = data->waterfall_view.centre_scroll_y + (num - D_ICON_EXTEND_NUM) * (data->icon[0].header.h + D_ICON_H_SPACE);
	data->waterfall_view.diff_h = (data->icon[0].header.h + D_ICON_H_SPACE)>>1;
	return num * 3;
}

static void _waterfall_img_x_up(applist_ui_data_t *data,lv_coord_t scroll_y)
{
	lv_obj_t **icon_obj = data->waterfall_view.obj_icon;

	lv_coord_t size_max_y = DEF_UI_HEIGHT - (data->icon[0].header.h >> 1) + D_CIRCLE_EXTEND;
	lv_coord_t size_min_y = -((int)data->icon[0].header.h >> 1) - D_CIRCLE_EXTEND;
	lv_coord_t middle_len = (size_max_y - size_min_y) >> 1;
	lv_coord_t middle_y = size_max_y - middle_len;

	lv_coord_t circle_radius = D_CIRCLE_RADIUS + D_CIRCLE_EXTEND;
	uint32_t circle_short_r = 0;
	uint32_t circle_length_r = 0;

	if(data->waterfall_view.w_space > 0)
	{
		if(middle_len > circle_radius)
		{
			if(circle_radius < data->waterfall_view.w_space)
				circle_radius = data->waterfall_view.w_space;
			circle_length_r = middle_len * middle_len;
			circle_short_r = circle_radius * circle_radius;
		}
		else
		{
			circle_length_r = circle_radius * circle_radius;
			uint32_t ret_denominator = (circle_radius - data->waterfall_view.w_space) * (circle_radius - data->waterfall_view.w_space);
			ret_denominator = (circle_radius * circle_radius) - ret_denominator;
			double ret_molecule = middle_len * middle_len * (double)circle_radius * circle_radius;
			circle_short_r = ret_molecule/ret_denominator;
		}
	}
	if(circle_length_r && circle_short_r)
	{
		for (lv_coord_t i = 0; i < data->waterfall_view.icon_num; i++)
		{
			lv_coord_t icon_y = lv_obj_get_y(icon_obj[i]);
			icon_y -= scroll_y;
			//SYS_LOG_ERR("KKKK %d,%d,%d",icon_y,data->waterfall_view.centre_scroll_y);
			if(icon_y >= size_min_y && icon_y <= size_max_y)
			{
				uint32_t diff_centre_y =  LV_ABS(icon_y - middle_y);
				if((i % 3) != 0)
				{
					if((i % 3) == 1 && data->waterfall_view.w_space > 0)
					{
						uint32_t circle_y = diff_centre_y * diff_centre_y;
						int32_t circle_x = 0;
						int32_t diff_x = 0;
						if(middle_len > circle_radius)
						{
							double ret = ((double)circle_length_r - circle_y) * circle_short_r;
							circle_x = ret/circle_length_r;
							lv_sqrt_res_t res = {0};
							lv_sqrt(circle_x, &res, 0x8000);
							diff_x = res.i;
							if(circle_radius > data->waterfall_view.w_space)
							{
								lv_obj_set_x(icon_obj[i],(DEF_UI_WIDTH>>1) - diff_x - (data->icon[0].header.w>>1));
								lv_obj_set_x(icon_obj[i+1],(DEF_UI_WIDTH>>1) + diff_x - (data->icon[0].header.w>>1));
							}
							else
							{
								lv_obj_set_x(icon_obj[i],data->waterfall_view.st_x + data->waterfall_view.w_space - diff_x);
								lv_obj_set_x(icon_obj[i+1],data->waterfall_view.st_x + (data->waterfall_view.w_space + data->icon[0].header.w) * 2 - (data->waterfall_view.w_space - diff_x));
							}
						}
						else
						{
							double ret = (double)(circle_short_r - circle_y) * circle_length_r;
							circle_x = ret/circle_short_r;
							lv_sqrt_res_t res = {0};
							lv_sqrt(circle_x, &res, 0x8000);
							diff_x = res.i;
							diff_x -= (circle_radius - data->waterfall_view.w_space);
							lv_obj_set_x(icon_obj[i],data->waterfall_view.st_x + data->waterfall_view.w_space - diff_x);
							lv_obj_set_x(icon_obj[i+1],data->waterfall_view.st_x + (data->waterfall_view.w_space + data->icon[0].header.w) * 2 - (data->waterfall_view.w_space - diff_x));
						}
					}
				}
			}
		}
	}
}

static void _waterfall_img_zoom_up(applist_ui_data_t *data,lv_coord_t scroll_y)
{
	lv_obj_t **icon_obj = data->waterfall_view.obj_icon;

	lv_coord_t zoom_max_y = DEF_UI_HEIGHT;
	lv_coord_t zoom_min_y = -(int)data->icon[0].header.h;
	lv_coord_t middle_len = (zoom_max_y - zoom_min_y) >> 1;
	lv_coord_t middle_y = zoom_max_y - middle_len;

	for (lv_coord_t i = 0; i < data->waterfall_view.icon_num; i++)
	{
		lv_coord_t icon_y = lv_obj_get_y(icon_obj[i]);
		icon_y -= scroll_y;
		//SYS_LOG_ERR("KKKK %d,%d,%d",icon_y,data->waterfall_view.centre_scroll_y);
		if(icon_y >= zoom_min_y && icon_y <= zoom_max_y)
		{
			uint32_t diff_centre_y =  LV_ABS(icon_y - middle_y);
			uint32_t v_zoom = (middle_len - diff_centre_y) * (D_ICON_ZOOM_MAX - D_ICON_ZOOM_MIN) / middle_len + D_ICON_ZOOM_MIN;
			if((i % 3) != 0)
			{
				if(v_zoom > D_SIDE_ICON_ZOOM_MAX)
					v_zoom = D_SIDE_ICON_ZOOM_MAX;
			}
			simple_img_set_scale(icon_obj[i],v_zoom);
		}
		else
			simple_img_set_scale(icon_obj[i],LV_SCALE_NONE);
	}
}

static void _waterfall_img_y_up(applist_ui_data_t *data,lv_coord_t scroll_y)
{
	lv_obj_t **icon_obj = data->waterfall_view.obj_icon;
	for (lv_coord_t i = 0; i < data->waterfall_view.icon_num; i++)
	{
		lv_coord_t icon_y = lv_obj_get_y(icon_obj[i]);
		if(scroll_y <= 0)
		{
			icon_y += data->waterfall_view.centre_scroll_y;
			data->waterfall_view.scroll_len -= data->waterfall_view.centre_scroll_y;
			lv_coord_t end_y = data->waterfall_view.end_y;
			if((i % 3) != 0)
				end_y += data->waterfall_view.diff_h;
			if(icon_y >= end_y)
				icon_y -= data->waterfall_view.end_y;
		}
		else
		{
			icon_y -= (data->waterfall_view.centre_scroll_y);
			data->waterfall_view.scroll_len += data->waterfall_view.centre_scroll_y;
			lv_coord_t end_y = 0;
			if((i % 3) != 0)
				end_y += data->waterfall_view.diff_h;
			if(icon_y < end_y)
				icon_y += (data->waterfall_view.end_y);
		}
		lv_obj_set_y(icon_obj[i],icon_y);
		//SYS_LOG_ERR("XXXX %d",icon_y);
	}
}

static void _waterfall_scroll_event_cb(lv_event_t * e)
{
	//lv_event_code_t code = lv_event_get_code(e);
	applist_ui_data_t *data = lv_event_get_user_data(e);
	/*switch (code)
	{
		case LV_EVENT_SCROLL:
		{*/
			lv_coord_t y = lv_obj_get_scroll_y(data->cont);
			if(y <= 0 || y >= data->waterfall_view.scroll_recover)
			{
				SYS_LOG_ERR("_waterfall_scroll_event_cb %d",y);
				lv_coord_t diff_y = (y <= 0) ? y : (y - data->waterfall_view.scroll_recover);
				lv_obj_scroll_to_y(data->cont,data->waterfall_view.centre_scroll_y+diff_y, LV_ANIM_OFF);
				_waterfall_img_y_up(data,y);
				lv_obj_update_layout(data->cont);
				y = data->waterfall_view.centre_scroll_y+diff_y;
			}
			_waterfall_img_zoom_up(data,y);
			_waterfall_img_x_up(data,y);
	/*	}
		break;
	default:
		break;
	}*/
}

static int _waterfall_view_create(lv_obj_t * scr)
{
	applist_ui_data_t *data = lv_obj_get_user_data(scr);
	data->cont = lv_obj_create(scr);
	lv_obj_set_pos(data->cont,0,0);
	lv_obj_set_size(data->cont,DEF_UI_WIDTH,DEF_UI_HEIGHT);
	lv_obj_set_style_pad_all(data->cont,0,LV_PART_MAIN);
	lv_obj_set_style_border_width(data->cont,0,LV_PART_MAIN);
	lv_obj_set_scroll_dir(data->cont, LV_DIR_VER);
	lv_obj_set_scrollbar_mode(data->cont, LV_SCROLLBAR_MODE_OFF); /* default no scrollbar */
	lv_obj_add_event_cb(data->cont, _waterfall_scroll_event_cb, LV_EVENT_SCROLL, data);
	coder_simulation_tp_register(data->cont, APPLIST_VIEW, LV_DIR_VER, 50, NULL);

	
	lv_coord_t st_x = D_ICON_W_SPACE * 2 + data->icon[0].header.w * 3;
	lv_coord_t icon_w_space = D_ICON_W_SPACE;
	uint32_t icon_num = D_ICON_NUM(data);
	data->waterfall_view.icon_num = icon_num;
	if(st_x > DEF_UI_WIDTH)
	{
		icon_w_space = (DEF_UI_WIDTH - data->icon[0].header.w * 3)>>1;
		st_x = 0;
	}
	else
		st_x = (DEF_UI_WIDTH - st_x)>>1;
	data->waterfall_view.st_x = st_x;
	data->waterfall_view.w_space = icon_w_space;
	lv_obj_t **icon_obj = app_mem_malloc(sizeof(lv_obj_t *)*icon_num);
	if(icon_obj == NULL)
		return -ENOMEM;
	memset(icon_obj, 0, sizeof(lv_obj_t *)*icon_num);
	for(uint32_t i=0;i<icon_num;i++)
	{
		icon_obj[i] = simple_img_create(data->cont);
		uint32_t icon_x = st_x;
		uint32_t icon_y = (data->icon[0].header.h + D_ICON_H_SPACE) * (i/3);
		switch ((i % 3))
		{
		case 0:
			icon_x += icon_w_space + data->icon[0].header.w;
			break;
		case 2:
			icon_x += (icon_w_space + data->icon[0].header.w) * 2;
		case 1:
			icon_y += data->waterfall_view.diff_h;
			break;
		default:
			break;
		}
		lv_obj_set_pos(icon_obj[i] , icon_x , icon_y);
		int32_t img_id = i % NUM_ITEMS;
		simple_img_set_src(icon_obj[i],applist_get_icon(data, img_id));
		simple_img_set_pivot(icon_obj[i],data->icon[0].header.w>>1,data->icon[0].header.h>>1);
		icon_obj[i]->user_data = (void *)img_id;
		lv_obj_add_event_cb(icon_obj[i], applist_btn_event_def_handler, LV_EVENT_SHORT_CLICKED, data);
		lv_obj_add_flag(icon_obj[i],LV_OBJ_FLAG_CLICKABLE);
	}
	lv_obj_update_layout(data->cont);
	data->waterfall_view.obj_icon = icon_obj;
	int16_t scrl_y = 0;
	data->presenter->load_scroll_value(NULL, &scrl_y);
	if(scrl_y)
	{
		while(scrl_y < 0)
		{
			_waterfall_img_y_up(data,0);
			scrl_y += data->waterfall_view.centre_scroll_y;
		}
		lv_obj_scroll_to_y(data->cont,scrl_y,LV_ANIM_OFF);
	}
	else
		lv_obj_scroll_to_y(data->cont,data->waterfall_view.scroll_recover,LV_ANIM_OFF);
	return 0;
}

static void _waterfall_view_delete(lv_obj_t * scr)
{
	applist_ui_data_t *data = lv_obj_get_user_data(scr);
	lv_coord_t st_y = lv_obj_get_y(data->waterfall_view.obj_icon[0]);
	st_y = lv_obj_get_scroll_y(data->cont) - st_y;
	data->presenter->save_scroll_value(0, st_y);
	app_mem_free(data->waterfall_view.obj_icon);
	data->waterfall_view.obj_icon = NULL;
	//data->presenter->save_scroll_value(0, lv_obj_get_scroll_y(data->cont));
}

static void _waterfall_view_update_icon(applist_ui_data_t * data, uint8_t idx, const lv_image_dsc_t * src)
{
	for (lv_coord_t i = 0; i < data->waterfall_view.icon_num; i++)
	{
		if((int)(data->waterfall_view.obj_icon[i]->user_data) == idx)
			simple_img_set_src(data->waterfall_view.obj_icon[i], applist_get_icon(data, idx));
	}
}


