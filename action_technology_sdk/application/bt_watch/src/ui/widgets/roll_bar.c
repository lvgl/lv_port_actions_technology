/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file roll_bar.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <assert.h>
#include "roll_bar.h"
#include <ui_mem.h>
#include <lvgl/src/lvgl_private.h>

/*********************
 *      DEFINES
 *********************/
#define ROLL_BAR_HIDDEN_DELAY_TIME 500
#define ROLL_BAR_HIDDEN_ANIM_TIME 500

#define ROLL_BAR_SHOW_DELAY_TIME 0
#define ROLL_BAR_SHOW_ANIM_TIME 0
/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
	lv_obj_t *obj; /*Ext. of ancestor*/
	lv_obj_t *obj_up;
	lv_obj_t *target_obj;
	roll_bar_cb_t cb;
	lv_coord_t min_value;
	lv_coord_t at_value;
	uint8_t min_opa;
	uint8_t max_opa;
	uint8_t b_hor_ver;
	bool arc_mode :1;
	bool ring_bar :1;
	bool fade_in_out :1;
	bool b_auto :1;
	bool bar_type :1;
} roll_bar_t;

/**********************
 *  EXTERNAL PROTOTYPES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/


/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void _roll_bar_set_opa(roll_bar_t *roll_bar,lv_opa_t opa)
{
	if(!roll_bar->arc_mode)
	{
		lv_obj_set_style_bg_opa(roll_bar->obj,opa,LV_PART_MAIN);
		lv_obj_set_style_bg_opa(roll_bar->obj_up,opa,LV_PART_MAIN);
	}
	else
	{
		lv_obj_set_style_arc_opa(roll_bar->obj,opa,LV_PART_MAIN);
		lv_obj_set_style_arc_opa(roll_bar->obj,opa,LV_PART_INDICATOR);
	}
}

static void _roll_bar_anim_cb(void * var, int32_t v)
{
	roll_bar_t *roll_bar = (roll_bar_t *)var;
	_roll_bar_set_opa(roll_bar,v);
	if(roll_bar->cb)
		roll_bar->cb(v);
}

static void _roll_bar_fade_in_out(roll_bar_t *roll_bar, bool fade_in)
{
	if(roll_bar->fade_in_out)
	{
		lv_anim_delete(roll_bar,_roll_bar_anim_cb);
		lv_anim_t a;
		lv_anim_init(&a);
		lv_anim_set_var(&a, roll_bar);
		lv_opa_t at_opa = roll_bar->arc_mode ? lv_obj_get_style_arc_opa(roll_bar->obj , LV_PART_MAIN) : lv_obj_get_style_bg_opa(roll_bar->obj , LV_PART_MAIN);
		if(fade_in)
		{
			lv_anim_set_values(&a, at_opa, roll_bar->max_opa);
			lv_anim_set_duration(&a, ROLL_BAR_SHOW_ANIM_TIME);
			lv_anim_set_delay(&a, ROLL_BAR_SHOW_DELAY_TIME);
		}
		else
		{
			lv_anim_set_values(&a, at_opa, roll_bar->min_opa);
			lv_anim_set_duration(&a, ROLL_BAR_HIDDEN_ANIM_TIME);
			lv_anim_set_delay(&a, ROLL_BAR_HIDDEN_DELAY_TIME);
		}
		lv_anim_set_exec_cb(&a, _roll_bar_anim_cb);
		lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
		lv_anim_start(&a);
	}
}

static void roll_bar_up(roll_bar_t * roll_bar,lv_coord_t value)
{
	roll_bar->at_value = value;
	if(!roll_bar->arc_mode)
	{
		if(roll_bar->b_hor_ver == ROLL_HOR_MOD)
			lv_obj_set_height(roll_bar->obj_up,value);
		else
			lv_obj_set_width(roll_bar->obj_up,value);
	}
	else
		lv_arc_set_value(roll_bar->obj,value);
}

static void roll_bar_scroll_cb(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	roll_bar_t *roll_bar = lv_event_get_user_data(e);
	lv_obj_t *obj = lv_event_get_current_target(e);
	if(!roll_bar->b_auto)
		return;
	switch (code)
	{
		case LV_EVENT_SCROLL_BEGIN:
		case LV_EVENT_SCROLL_END:
		case LV_EVENT_SCROLL:
			{
				lv_coord_t roll_coord = 0;
				lv_coord_t all_roll_coord = 0;
				lv_coord_t obj_value = ROLL_BAR_MAX;
				if(roll_bar->b_hor_ver == ROLL_HOR_MOD)
				{
					roll_coord = lv_obj_get_scroll_top(obj);
					all_roll_coord = roll_coord + lv_obj_get_scroll_bottom(obj);
					if(!roll_bar->arc_mode)
						obj_value = lv_obj_get_height(roll_bar->obj);
				}
				else
				{
					roll_coord = lv_obj_get_scroll_left(obj);
					all_roll_coord = roll_coord + lv_obj_get_scroll_right(obj);
					if(!roll_bar->arc_mode)
						obj_value = lv_obj_get_width(roll_bar->obj);
				}
				if(roll_coord < 0)
					roll_coord = 0;
				if(roll_coord > all_roll_coord)
					roll_coord = all_roll_coord;
				lv_coord_t at_value = (roll_bar->min_value + roll_coord * (obj_value - roll_bar->min_value) / all_roll_coord);
				roll_bar_up(roll_bar,at_value);
				if(LV_EVENT_SCROLL != code)
				{
					if(LV_EVENT_SCROLL_BEGIN == code)
						_roll_bar_fade_in_out(roll_bar,true);
					else
						_roll_bar_fade_in_out(roll_bar,false);
				}
			}
			break;
		default:
			break;
	}
}

static void roll_bar_obj_cb(lv_event_t * e)
{
	roll_bar_t *roll_bar = lv_event_get_user_data(e);
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
		case LV_EVENT_DELETE:
			if(roll_bar->target_obj)
				lv_obj_remove_event_cb(roll_bar->target_obj,roll_bar_scroll_cb);
			lv_anim_delete(roll_bar,_roll_bar_anim_cb);
			ui_mem_free(MEM_GUI, roll_bar);
			break;
		case LV_EVENT_SIZE_CHANGED:
			if(!roll_bar->arc_mode)
			{
				if(roll_bar->b_hor_ver == ROLL_HOR_MOD)
				{
					lv_coord_t obj_height = lv_obj_get_height(roll_bar->obj);
					lv_coord_t obj_up_height = lv_obj_get_height(roll_bar->obj_up);
					if(roll_bar->min_value > obj_height)
						roll_bar->min_value = obj_height;
					if(obj_up_height > obj_height)
						roll_bar_up(roll_bar,obj_height);
					lv_obj_set_width(roll_bar->obj_up,lv_obj_get_width(roll_bar->obj));
				}
				else
				{
					lv_coord_t obj_width = lv_obj_get_width(roll_bar->obj);
					lv_coord_t obj_up_width = lv_obj_get_width(roll_bar->obj_up);
					if(roll_bar->min_value > obj_width)
						roll_bar->min_value = obj_width;
					if(obj_up_width > obj_width)
						roll_bar_up(roll_bar,obj_width);
					lv_obj_set_height(roll_bar->obj_up,lv_obj_get_height(roll_bar->obj));
				}
			}
			break;
		case LV_EVENT_STYLE_CHANGED:
			if(!roll_bar->arc_mode)
				lv_obj_set_style_radius(roll_bar->obj_up,lv_obj_get_style_radius(roll_bar->obj,LV_PART_MAIN),LV_PART_MAIN);
			break;
		default:
			break;
	}
}

static void roll_bar_parent_del_cb(lv_event_t * e)
{
	roll_bar_t *roll_bar = lv_event_get_user_data(e);
	roll_bar->target_obj = NULL;
	lv_obj_delete(roll_bar->obj);
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
lv_obj_t * roll_bar_create(lv_obj_t * parent)
{
	roll_bar_t *roll_bar = ui_mem_alloc(MEM_GUI, sizeof(roll_bar_t), __func__);
	if (!roll_bar) {
		return NULL;
	}
	lv_memzero(roll_bar,sizeof(roll_bar_t));
	roll_bar->target_obj = parent;
	roll_bar->b_auto = true;
	roll_bar->obj = lv_obj_create(lv_obj_get_parent(roll_bar->target_obj));
	lv_obj_set_style_pad_all(roll_bar->obj,0,LV_PART_MAIN);
	lv_obj_set_style_border_width(roll_bar->obj,0,LV_PART_MAIN);
	lv_obj_set_user_data(roll_bar->obj,roll_bar);

	roll_bar->obj_up = lv_obj_create(roll_bar->obj);
	lv_obj_set_size(roll_bar->obj_up,0,0);
	lv_obj_set_style_pad_all(roll_bar->obj_up,0,LV_PART_MAIN);
	lv_obj_set_style_border_width(roll_bar->obj_up,0,LV_PART_MAIN);
	_roll_bar_set_opa(roll_bar,LV_OPA_100);

	lv_obj_add_event_cb(roll_bar->target_obj,roll_bar_scroll_cb,LV_EVENT_ALL,roll_bar);
	lv_obj_add_event_cb(roll_bar->target_obj,roll_bar_parent_del_cb,LV_EVENT_DELETE,roll_bar);
	lv_obj_add_event_cb(roll_bar->obj,roll_bar_obj_cb,LV_EVENT_ALL,roll_bar);
	return roll_bar->obj;
}

lv_obj_t * roll_arc_create(lv_obj_t * parent)
{
	roll_bar_t *roll_bar = ui_mem_alloc(MEM_GUI, sizeof(roll_bar_t), __func__);
	if (!roll_bar) {
		return NULL;
	}
	lv_memzero(roll_bar,sizeof(roll_bar_t));
	roll_bar->target_obj = parent;
	roll_bar->b_auto = true;
	roll_bar->arc_mode = true;
	roll_bar->obj = lv_arc_create(lv_obj_get_parent(roll_bar->target_obj));
	lv_arc_set_range(roll_bar->obj,0,ROLL_BAR_MAX);
	lv_arc_set_value(roll_bar->obj,ROLL_BAR_MAX);
	lv_obj_set_user_data(roll_bar->obj,roll_bar);
	lv_obj_set_style_arc_rounded(roll_bar->obj,true,LV_PART_MAIN);
	lv_obj_set_style_arc_rounded(roll_bar->obj,true,LV_PART_INDICATOR);
	lv_obj_remove_flag(roll_bar->obj, LV_OBJ_FLAG_CLICKABLE);
	_roll_bar_set_opa(roll_bar,LV_OPA_100);
	roll_bar_up(roll_bar,0);

	lv_obj_add_event_cb(roll_bar->target_obj,roll_bar_scroll_cb,LV_EVENT_ALL,roll_bar);
	lv_obj_add_event_cb(roll_bar->target_obj,roll_bar_parent_del_cb,LV_EVENT_DELETE,roll_bar);
	lv_obj_add_event_cb(roll_bar->obj,roll_bar_obj_cb,LV_EVENT_ALL,roll_bar);
	return roll_bar->obj;
}
/*=====================
 * Setter functions
 *====================*/
void roll_arc_set_diameter_width(lv_obj_t * obj,uint16_t dia,uint16_t arc_width)
{
	roll_bar_t *roll_bar = lv_obj_get_user_data(obj);
	if(roll_bar->arc_mode)
	{
		lv_obj_set_size(roll_bar->obj,dia,dia);
		lv_obj_set_style_arc_width(roll_bar->obj,arc_width,LV_PART_MAIN);
		lv_obj_set_style_arc_width(roll_bar->obj,arc_width,LV_PART_INDICATOR);
	}
}

void roll_arc_set_bg_angles(lv_obj_t * obj,uint16_t start, uint16_t end)
{
	roll_bar_t *roll_bar = lv_obj_get_user_data(obj);
	if(roll_bar->arc_mode)
		lv_arc_set_bg_angles(roll_bar->obj,start,end);
}

void roll_arc_set_rotation(lv_obj_t * obj,uint16_t rotation)
{
	roll_bar_t *roll_bar = lv_obj_get_user_data(obj);
	if(roll_bar->arc_mode)
		lv_arc_set_rotation(roll_bar->obj,rotation);
}

void roll_bar_set_direction(lv_obj_t * obj, uint8_t mod)
{
	roll_bar_t *roll_bar = lv_obj_get_user_data(obj);
	roll_bar->b_hor_ver = mod;
}

void roll_bar_set_opa(lv_obj_t * obj, uint8_t min_opa , uint8_t max_opa)
{
	roll_bar_t *roll_bar = lv_obj_get_user_data(obj);
	roll_bar->max_opa = max_opa;
	roll_bar->min_opa = min_opa;
}

void roll_bar_set_fade(lv_obj_t * obj, bool fade)
{
	roll_bar_t *roll_bar = lv_obj_get_user_data(obj);
	roll_bar->fade_in_out = fade;
}

void roll_bar_set_min(lv_obj_t * obj, uint16_t min_value)
{
	roll_bar_t *roll_bar = lv_obj_get_user_data(obj);
	roll_bar->min_value = min_value;
	if(!roll_bar->arc_mode)
	{
		if(roll_bar->b_hor_ver == ROLL_HOR_MOD)
		{
			lv_obj_update_layout(roll_bar->obj);
			lv_coord_t obj_height = lv_obj_get_height(roll_bar->obj);
			if(min_value > obj_height)
				roll_bar->min_value = obj_height;
		}
		else
		{
			lv_obj_update_layout(roll_bar->obj);
			lv_coord_t obj_width = lv_obj_get_width(roll_bar->obj);
			if(min_value > obj_width)
				roll_bar->min_value = obj_width;
		}
	}
	else
		roll_bar->min_value = min_value > ROLL_BAR_MAX ? ROLL_BAR_MAX : min_value;
	if(roll_bar->at_value < roll_bar->min_value)
		roll_bar_up(roll_bar,roll_bar->min_value);
}

void roll_bar_set_color(lv_obj_t * obj, lv_color_t color , lv_color_t color_1)
{
	roll_bar_t *roll_bar = lv_obj_get_user_data(obj);
	if(!roll_bar->arc_mode)
	{
		lv_obj_set_style_bg_color(roll_bar->obj,color,LV_PART_MAIN);
		lv_obj_set_style_bg_color(roll_bar->obj_up,color_1,LV_PART_MAIN);
	}
	else
	{
		lv_obj_set_style_arc_color(roll_bar->obj,color,LV_PART_MAIN);
		lv_obj_set_style_arc_color(roll_bar->obj,color_1,LV_PART_INDICATOR);
	}
}

void roll_bar_set_auto(lv_obj_t * obj, bool b_auto)
{
	roll_bar_t *roll_bar = lv_obj_get_user_data(obj);
	roll_bar->b_auto = b_auto;
}

void roll_bar_set_fade_cb(lv_obj_t * obj, roll_bar_cb_t cb)
{
	roll_bar_t *roll_bar = lv_obj_get_user_data(obj);
	roll_bar->cb = cb;
}

void roll_bar_set_roll_value(lv_obj_t * obj, lv_coord_t value)
{
	roll_bar_t *roll_bar = lv_obj_get_user_data(obj);

	if(value > ROLL_BAR_MAX)
		value = ROLL_BAR_MAX;
	if(value < 0)
		value = 0;
	lv_coord_t obj_value = value;
	if(!roll_bar->arc_mode)
		obj_value = roll_bar->b_hor_ver == ROLL_HOR_MOD ? lv_obj_get_height(roll_bar->obj) - roll_bar->min_value : lv_obj_get_width(roll_bar->obj) - roll_bar->min_value;		
	else
		obj_value = ROLL_BAR_MAX - roll_bar->min_value;
	if(obj_value == 0)
		lv_obj_update_layout(roll_bar->obj);
	obj_value = obj_value * value / ROLL_BAR_MAX + roll_bar->min_value;
	roll_bar_up(roll_bar,obj_value);
}

void roll_bar_set_fade_in_out(lv_obj_t * obj, bool fade_in)
{
	roll_bar_t *roll_bar = lv_obj_get_user_data(obj);
	_roll_bar_fade_in_out(roll_bar,fade_in);
}

void roll_bar_set_current_opa(lv_obj_t * obj, lv_opa_t value)
{
	roll_bar_t *roll_bar = lv_obj_get_user_data(obj);
	_roll_bar_set_opa(roll_bar,value);
}

lv_obj_t *roll_bar_get_obj(lv_obj_t * obj)
{
	roll_bar_t *roll_bar = lv_obj_get_user_data(obj);
	return roll_bar->obj;
}

lv_obj_t *roll_bar_get_obj_up(lv_obj_t * obj)
{
	roll_bar_t *roll_bar = lv_obj_get_user_data(obj);
	return roll_bar->obj_up;
}

