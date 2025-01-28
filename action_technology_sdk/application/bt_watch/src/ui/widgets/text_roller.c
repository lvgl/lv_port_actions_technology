/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file text_roller.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <assert.h>
#include <ui_mem.h>
#include "text_roller.h"
 #include <stdio.h>
 #include <lvgl/src/lvgl_private.h>

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &text_roller_class

#define NUM_MAX_POST 10
#define ACCELERATED_SPEED 5
#define SKIP_HEIGHT 5	//0 ~ 10
#define CLICK_HEIGHT 15

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
	lv_obj_t obj; /*Ext. of ancestor*/
	text_id_update_cb_t text_id_update_cb;
	lv_timer_t *_time;
	lv_area_t up_area;
	lv_area_t middle_area;
	lv_area_t down_area;
	lv_area_t *text_area;
	lv_area_t sel_area[3];
	lv_area_t main_area;
	lv_coord_t roller_y;
	lv_coord_t roller_sel_y;
	lv_coord_t roller_main_y;
	lv_coord_t touch_time;
	lv_coord_t touch_y;
	lv_coord_t text_id;
	char *str;
	uint32_t str_byte;
	lv_coord_t num_start;
	lv_coord_t num_increment;
	uint32_t str_num;
	uint8_t num_digit;
	uint8_t str_sign;
	uint8_t row_cnt;
	bool cycle;
	bool click;
} text_roller_t;

/**********************
 *  EXTERNAL PROTOTYPES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void text_roller_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void text_roller_event(const lv_obj_class_t * class_p, lv_event_t * e);
static void text_roller_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void text_roller_draw_label(lv_obj_t * obj, lv_layer_t * layer, lv_draw_label_dsc_t * dsc, lv_area_t * change_area , lv_area_t * static_area ,lv_coord_t text_id);
static void text_roller_set_show_area(lv_obj_t * obj);
static void text_roller_anim_cb(void * var, int32_t v);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t text_roller_class = {
	.destructor_cb = text_roller_destructor,
	.event_cb = text_roller_event,
	.constructor_cb = text_roller_constructor,
	.width_def = LV_SIZE_CONTENT,
	.height_def = LV_SIZE_CONTENT,
	.instance_size = sizeof(text_roller_t),
	.base_class = &lv_obj_class,
	.name = "text-roller",
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * text_roller_create(lv_obj_t * parent)
{
	LV_LOG_INFO("begin");
	lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
	lv_obj_class_init_obj(obj);
	return obj;
}

/*=====================
 * Setter functions
 *====================*/
void text_roller_set_visible_row_count(lv_obj_t *obj , uint8_t cnt)
{
	text_roller_t * roller = (text_roller_t *)obj;
	if(cnt == roller->row_cnt)
	{
		text_roller_set_show_area(obj);
		return;
	}
	if(cnt < 3)
		cnt = 3;
	if(roller->str_sign)
	{
		if(cnt > roller->str_num)
			cnt = roller->str_num;
	}
	if(cnt % 2 == 0)
		cnt--;
	roller->row_cnt = cnt;
	if(roller->text_area)
		roller->text_area = lv_realloc(roller->text_area,sizeof(lv_area_t) * (roller->row_cnt + 1));
	else
		roller->text_area = lv_malloc(sizeof(lv_area_t) * (roller->row_cnt + 1));
	lv_memzero(roller->text_area,sizeof(lv_area_t) * (roller->row_cnt + 1));
	text_roller_set_show_area(obj);
}

void text_roller_set_text_id(lv_obj_t *obj , uint32_t id)
{
	text_roller_t * roller = (text_roller_t *)obj;
	roller->text_id = id;
	lv_obj_invalidate(obj);
}

void text_roller_set_str(lv_obj_t *obj, char **str, uint32_t str_byte,uint32_t str_num)
{
	text_roller_t * roller = (text_roller_t *)obj;
	if(str && str_num >= 3)
	{
		if(str_byte)
		{
			if(roller->str)
				lv_free(roller->str);
			roller->str = lv_malloc(str_num * str_byte);
			lv_memcpy(roller->str,str,str_num * str_byte);
		}
		else
		{
			for(uint16_t i = 0 ; i < str_num ; i++)
			{
				uint32_t len = lv_strlen(str[i]) + 1;
				if(len > str_byte)
					str_byte = len;
			}
			if(roller->str)
				lv_free(roller->str);
			roller->str = lv_malloc(str_num * str_byte);
			lv_memzero(roller->str, str_num * str_byte);
			for(uint16_t i = 0 ; i < str_num ; i++)
				lv_strcpy(roller->str + i * str_byte, str[i]);
		}
		roller->str_sign = true;
		roller->str_byte = str_byte;
		roller->str_num = str_num;
	}
}

void text_roller_set_num(lv_obj_t *obj , lv_coord_t num_start , lv_coord_t num_end , lv_coord_t increment)
{
	text_roller_t * roller = (text_roller_t *)obj;
	lv_coord_t str_num = (num_end - num_start)/increment + 1;
	if(str_num > 0)
	{
		roller->str_sign = false;
		roller->num_start = num_start;
		roller->num_increment = increment;
		roller->str_num = str_num;
	}
}

void text_roller_set_num_digit(lv_obj_t *obj , uint8_t num_digit)
{
	text_roller_t * roller = (text_roller_t *)obj;
	roller->num_digit = num_digit;
}

void text_roller_set_update_cb(lv_obj_t *obj , text_id_update_cb_t update_cb)
{
	text_roller_t * roller = (text_roller_t *)obj;
	roller->text_id_update_cb = update_cb;
}

void text_roller_stop(lv_obj_t *obj)
{
	text_roller_t * roller = (text_roller_t *)obj;
	lv_anim_delete(roller,text_roller_anim_cb);
}

void text_roller_set_cycle(lv_obj_t *obj , bool cycle)
{
	text_roller_t * roller = (text_roller_t *)obj;
	roller->cycle = cycle;
}

void text_roller_open_click_update(lv_obj_t *obj , bool click)
{
	text_roller_t * roller = (text_roller_t *)obj;
	roller->click = click;
}

/*=====================
 * Getter functions
 *====================*/
uint8_t text_roller_get_visible_row_count(lv_obj_t *obj)
{
	text_roller_t * roller = (text_roller_t *)obj;
	return roller->row_cnt;
}

lv_coord_t text_roller_get_texe_id(lv_obj_t *obj)
{
	text_roller_t * roller = (text_roller_t *)obj;
	return roller->text_id;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static lv_coord_t text_roller_effects(lv_obj_t * obj , lv_coord_t y)
{
	text_roller_t * roller = (text_roller_t *)obj;
	const lv_font_t * font_main = lv_obj_get_style_text_font(obj, LV_PART_MAIN);
    const lv_font_t * font_sel = lv_obj_get_style_text_font(obj, LV_PART_SELECTED);
	if(font_main == NULL || font_sel == NULL)
		return 0;
    lv_coord_t font_main_h = lv_font_get_line_height(font_main);
    lv_coord_t font_sel_h = lv_font_get_line_height(font_sel);
	lv_coord_t line_space = lv_obj_get_style_text_line_space(obj, LV_PART_MAIN);
	lv_coord_t middle_h = font_sel_h + line_space;
	lv_coord_t up_down_h = font_main_h + line_space;
	lv_coord_t text_id = roller->text_id;
	while(LV_ABS(y) >= line_space + font_main_h)
	{
		if(y >= line_space + font_main_h)
		{
			y -= line_space + font_main_h;
			roller->text_id--;
			if(roller->text_id < 0)
			{
				if(!roller->cycle)
				{
					roller->text_id++;
					y = 0;
				}
				else
					roller->text_id += roller->str_num;
			}
		}
		else
		{
			y += line_space + font_main_h;
			roller->text_id++;
			if(roller->text_id >= (lv_coord_t)(roller->str_num))
			{
				if(!roller->cycle)
				{
					roller->text_id--;
					y = 0;
				}
				else
					roller->text_id -= roller->str_num;
			}
		}
	}
	if(!roller->cycle)
	{
		if(y > 0)
		{
			if(roller->text_id - 1 < 0)
				y = 0;
		}
		else if(y < 0)
		{
			if(roller->text_id + 1 >= (lv_coord_t)(roller->str_num))
				y = 0;
		}
	}
	//roller->roller_sel_y = y * (middle_h - ((font_sel_h - font_main_h)>>1)) / up_down_h;
	roller->roller_sel_y = y * middle_h / up_down_h;
	roller->roller_main_y = y * ((middle_h + up_down_h)>>1) / up_down_h;
	if(roller->text_id_update_cb && roller->text_id != text_id)
		roller->text_id_update_cb(obj);
	return y;
}

static void text_roller_anim_cb(void * var, int32_t v)
{
	lv_obj_t * obj = (lv_obj_t *)var;
	text_roller_t *roller = (text_roller_t *)var;
	roller->roller_y = text_roller_effects(obj,roller->roller_y + v - roller->touch_y);
	roller->touch_y = v;
	lv_obj_invalidate(obj);
}

static void text_roller_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
	LV_UNUSED(class_p);

	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * obj = lv_event_get_target(e);
	text_roller_t * roller = (text_roller_t *)obj;
	/*Ancestor events will be called during drawing*/
	switch (code)
	{
	case LV_EVENT_DRAW_MAIN:
		{
			lv_layer_t * layer = lv_event_get_layer(e);
			lv_area_t sel_area;
			lv_obj_get_coords(obj, &sel_area);
			lv_draw_rect_dsc_t sel_dsc;
			lv_draw_rect_dsc_init(&sel_dsc);
			lv_obj_init_draw_rect_dsc(obj, LV_PART_MAIN, &sel_dsc);
			lv_draw_rect(layer, &sel_dsc, &sel_area);

			lv_draw_rect_dsc_init(&sel_dsc);
			lv_obj_init_draw_rect_dsc(obj, LV_PART_SELECTED, &sel_dsc);
			lv_draw_rect(layer, &sel_dsc, &roller->middle_area);
		}
		break;
	case LV_EVENT_DRAW_MAIN_BEGIN:
		{
			lv_area_t sel_area;
			lv_obj_get_coords(obj, &sel_area);
			if(roller->up_area.x1 != sel_area.x1 || roller->up_area.x2 != sel_area.x2 ||
				roller->up_area.y1 != sel_area.y1 || roller->up_area.y2 != sel_area.y2)
				text_roller_set_show_area(obj);
		}
		break;
	case LV_EVENT_SIZE_CHANGED:
	case LV_EVENT_STYLE_CHANGED:
		text_roller_set_show_area(obj);
		break;
	case LV_EVENT_DRAW_POST:
		{
			lv_draw_label_dsc_t label_draw_dsc;
			lv_draw_label_dsc_init(&label_draw_dsc);
			lv_obj_init_draw_label_dsc(obj, LV_PART_MAIN, &label_draw_dsc);
			lv_coord_t text_area_id = 0;
			lv_layer_t * layer = lv_event_get_layer(e);
			uint8_t backup = 0;
			lv_area_t *up_down_area = &roller->up_area;
			lv_area_t at_area = {0};
			lv_coord_t text_id = 0;
			for(; text_area_id < roller->row_cnt + 1 ; text_area_id++)
			{
				lv_memcpy(&at_area , roller->text_area + text_area_id , sizeof(lv_area_t));
				at_area.y1 += roller->roller_y;
				at_area.y2 += roller->roller_y;
				if(text_area_id == ((roller->row_cnt + 1) >> 1))
				{
					backup = 1;
					up_down_area = &roller->down_area;
				}
				text_id = roller->text_id - ((roller->row_cnt + 1)>>1) + text_area_id + backup;
				text_roller_draw_label(obj , layer , &label_draw_dsc , &at_area , up_down_area , text_id);
			}

			lv_memcpy(&at_area , &roller->main_area , sizeof(lv_area_t));
			at_area.y1 += roller->roller_main_y;
			at_area.y2 += roller->roller_main_y;
			text_id = roller->text_id;
			if(at_area.y1 + (lv_area_get_height(&at_area) >> 1) > roller->middle_area.y1 + (lv_area_get_height(&roller->middle_area) >> 1))
				up_down_area = &roller->down_area;
			else
				up_down_area = &roller->up_area;
			text_roller_draw_label(obj , layer , &label_draw_dsc , &at_area , up_down_area , text_id);

			lv_draw_label_dsc_init(&label_draw_dsc);
			lv_obj_init_draw_label_dsc(obj, LV_PART_SELECTED, &label_draw_dsc);
			for(uint8_t i = 0 ; i < 3 ; i++)
			{
				lv_memcpy(&at_area , &roller->sel_area[i] , sizeof(lv_area_t));
				at_area.y1 += roller->roller_sel_y;
				at_area.y2 += roller->roller_sel_y;
				text_id = roller->text_id - 1 + i;
				text_roller_draw_label(obj , layer , &label_draw_dsc , &at_area , &roller->middle_area , text_id);
			}
		}
		break;
	case LV_EVENT_PRESSED:
		roller->touch_time = lv_tick_get();
		roller->touch_y = 0;
		lv_anim_delete(roller,text_roller_anim_cb);
		break;
	case LV_EVENT_PRESSING:
		{
			lv_indev_t *indev = lv_indev_get_act();
			lv_point_t point = {0};
			lv_indev_get_vect(indev,&point);
			roller->touch_y += point.y;
			if(LV_ABS(roller->touch_y) > CLICK_HEIGHT || !roller->click)
				roller->roller_y = text_roller_effects(obj,roller->roller_y + point.y);
			lv_obj_invalidate(obj);
		}
		break;
	case LV_EVENT_RELEASED:
		if(LV_ABS(roller->touch_y) < CLICK_HEIGHT && roller->click)
		{
			lv_obj_update_layout(obj);
			const lv_font_t * font_main = lv_obj_get_style_text_font(obj, LV_PART_MAIN);
			const lv_font_t * font_sel = lv_obj_get_style_text_font(obj, LV_PART_SELECTED);
			if(font_main == NULL || font_sel == NULL)
				return;
			lv_coord_t font_sel_h = lv_font_get_line_height(font_sel);
			lv_coord_t line_space = lv_obj_get_style_text_line_space(obj, LV_PART_MAIN);
			lv_coord_t up_down_h = (lv_obj_get_height(obj) - (font_sel_h + line_space)) >> 1;
			lv_coord_t font_main_h = lv_font_get_line_height(font_main);
			lv_indev_t *indev = lv_indev_get_act();
			lv_point_t point = {0};
			lv_indev_get_point(indev,&point);
			lv_area_t sel_area;
			lv_obj_get_coords(obj, &sel_area);
			if(point.x > sel_area.x1 && point.x < sel_area.x2)
			{
				if(point.y > sel_area.y1 && point.y < sel_area.y1 + up_down_h)
					roller->roller_y = text_roller_effects(obj,line_space + font_main_h);
				else if(point.y > sel_area.y2 - up_down_h && point.y < sel_area.y2)
					roller->roller_y = text_roller_effects(obj,-(line_space + font_main_h));
			}
			lv_obj_invalidate(obj);
		}
		else
		{
			int32_t r_time = lv_tick_elaps(roller->touch_time);
			if(r_time <= 0 || roller->touch_y == 0)
				return;
			int32_t auto_time = (LV_ABS(roller->touch_y) * 1000)/(r_time * ACCELERATED_SPEED);
			int32_t dy = 0;
			if(roller->touch_y > 0)
				dy = (roller->touch_y * auto_time / r_time) + ((ACCELERATED_SPEED * auto_time * auto_time/ 1000)>>1);
			else
				dy = (roller->touch_y * auto_time / r_time) - ((ACCELERATED_SPEED * auto_time * auto_time/ 1000)>>1);
			auto_time = auto_time*2;
			if(auto_time > 1000)
				auto_time = 1000;

			const lv_font_t * font_main = lv_obj_get_style_text_font(obj, LV_PART_MAIN);
			if(font_main == NULL)
				return;
			lv_coord_t font_main_h = lv_font_get_line_height(font_main);
			lv_coord_t line_space = lv_obj_get_style_text_line_space(obj, LV_PART_MAIN);
			lv_coord_t up_down_h = font_main_h + line_space;
			lv_coord_t target_y = 0;
			if(LV_ABS(roller->touch_y) > LV_ABS(dy))
				target_y = roller->roller_y;
			else
				target_y = dy - roller->touch_y + roller->roller_y;
			lv_coord_t surplus_y = target_y % up_down_h;
			if(target_y != 0)
			{
				if(target_y > 0)
				{
					if(surplus_y > (up_down_h * SKIP_HEIGHT / 10))
						target_y += up_down_h - surplus_y;
					else
						target_y -= surplus_y;
				}
				else
				{
					if(surplus_y < -(up_down_h * SKIP_HEIGHT / 10))
						target_y -= up_down_h + surplus_y;
					else
						target_y -= surplus_y;
				}
			}
			roller->touch_y = roller->roller_y;
			lv_anim_t a;
			lv_anim_init(&a);
			lv_anim_set_var(&a, roller);
			lv_anim_set_duration(&a, auto_time);
			lv_anim_set_values(&a,roller->roller_y,target_y);
			lv_anim_set_exec_cb(&a, text_roller_anim_cb);
			lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
			lv_anim_start(&a);
		}
		break;
	default:
		break;
	}
}

static void text_roller_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    text_roller_t * roller = (text_roller_t *)obj;

	roller->row_cnt = 3;
	roller->text_area = lv_malloc(sizeof(lv_area_t) * (roller->row_cnt + 1));
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLL_CHAIN_VER);
	roller->cycle = true;
	//text_roller_set_show_area(obj);
}

static lv_coord_t text_id_transition(lv_coord_t text_id , lv_coord_t min_id , lv_coord_t max_id)
{
	if(min_id < max_id)
	{
		while (text_id < min_id || text_id >= max_id)
		{
			while (text_id >= max_id)
				text_id -= max_id;
			while (text_id < min_id)
				text_id += max_id;
		}
	}
	else
		text_id = min_id;
	return text_id;
}

static void text_roller_draw_label(lv_obj_t * obj, lv_layer_t * layer, lv_draw_label_dsc_t * dsc, lv_area_t * change_area , lv_area_t * static_area ,lv_coord_t text_id)
{
	text_roller_t * roller = (text_roller_t *)obj;
	if(!roller->cycle)
		if(text_id < 0 || text_id >= (lv_coord_t)(roller->str_num))
			return;
	lv_area_t clipped_area;
	bool clip_ok = lv_area_intersect(&clipped_area, change_area, static_area);
	if (!clip_ok)
		return;
	lv_area_t label_area = layer->_clip_area;
	lv_area_t draw_area;
	clip_ok = lv_area_intersect(&draw_area, &clipped_area, &label_area);
	if (!clip_ok)
		return;
	lv_coord_t ofs_y = dsc->ofs_y;
	if(change_area->y1 + (lv_area_get_height(change_area) >> 1) < static_area->y1 + (lv_area_get_height(static_area)>>1))
	{
		if(static_area->y1 > change_area->y1)
			dsc->ofs_y = ofs_y - (static_area->y1 - change_area->y1);
	}
	layer->_clip_area = draw_area;
	text_id = text_id_transition(text_id , 0 , roller->str_num);
	if(roller->str_sign) {
		dsc->text = roller->str + (text_id * roller->str_byte);
		lv_draw_label(layer, dsc, &clipped_area);
	} else {
		char str[NUM_MAX_POST] = {0};
		char num_digit[NUM_MAX_POST] = {0};
		snprintf(num_digit,sizeof(num_digit) - 1,"%%0%dd",roller->num_digit);
		snprintf(str,sizeof(str) - 1,num_digit,roller->num_start + text_id * roller->num_increment);

		dsc->text = str;
		lv_draw_label(layer, dsc, &clipped_area);
	}

	layer->_clip_area = label_area;
	dsc->ofs_y = ofs_y;
}

static void text_roller_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
	LV_UNUSED(class_p);
	text_roller_t * roller = (text_roller_t *)obj;
	if(roller->text_area)
		lv_free(roller->text_area);
	if(roller->str)
		lv_free(roller->str);
}

static void text_roller_set_show_area(lv_obj_t * obj)
{
	text_roller_t * roller = (text_roller_t *)obj;
	const lv_font_t * font_main = lv_obj_get_style_text_font(obj, LV_PART_MAIN);
    const lv_font_t * font_sel = lv_obj_get_style_text_font(obj, LV_PART_SELECTED);
	if(font_main == NULL || font_sel == NULL)
		return;
    lv_coord_t font_main_h        = lv_font_get_line_height(font_main);
    lv_coord_t font_sel_h        = lv_font_get_line_height(font_sel);
    lv_coord_t line_space = lv_obj_get_style_text_line_space(obj, LV_PART_MAIN);

	lv_coord_t up_down_height = (roller->row_cnt >> 1) * (font_main_h + line_space);
	lv_coord_t middle_height = font_sel_h + line_space;

	if(up_down_height * 2 + middle_height != lv_obj_get_height(obj))
		lv_obj_set_height(obj,up_down_height * 2 + middle_height);

	lv_area_t roller_coords;
	lv_obj_get_coords(obj, &roller_coords);
	roller->up_area.x1 = roller_coords.x1;
	roller->up_area.x2 = roller_coords.x2;
	roller->up_area.y1 = roller_coords.y1;
	roller->up_area.y2 = roller->up_area.y1 + up_down_height - 1;
	
	roller->middle_area.x1 = roller_coords.x1;
	roller->middle_area.x2 = roller_coords.x2;
	roller->middle_area.y1 = roller->up_area.y2 + 1;
	roller->middle_area.y2 = roller->middle_area.y1 + middle_height - 1;

	roller->down_area.x1 = roller_coords.x1;
	roller->down_area.x2 = roller_coords.x2;
	roller->down_area.y1 = roller->middle_area.y2 + 1;
	roller->down_area.y2 = roller->down_area.y1 + up_down_height - 1;
	
	lv_coord_t text_area_id = 0;
	lv_coord_t start_y = roller_coords.y1 - (font_main_h + line_space);
	for(; text_area_id < ((roller->row_cnt + 1)>>1) ; text_area_id++)
	{
		lv_area_t *at_area = roller->text_area + text_area_id;
		at_area->x1 = roller_coords.x1;
		at_area->x2 = roller_coords.x2;
		at_area->y1 = start_y + (line_space >> 1);
		start_y += font_main_h + line_space;
		at_area->y2 = start_y - 1 - (line_space >> 1);
	}
	roller->sel_area[0].x1 = roller_coords.x1;
	roller->sel_area[0].x2 = roller_coords.x2;
	roller->sel_area[0].y1 = start_y - middle_height + (line_space >> 1);
	roller->sel_area[0].y2 = start_y - 1 - (line_space >> 1);

	roller->sel_area[1].x1 = roller_coords.x1;
	roller->sel_area[1].x2 = roller_coords.x2;
	roller->sel_area[1].y1 = start_y + (line_space >> 1);
	start_y += middle_height;
	roller->sel_area[1].y2 = start_y - 1 - (line_space >> 1);

	roller->main_area.x1 = roller_coords.x1;
	roller->main_area.x2 = roller_coords.x2;
	roller->main_area.y1 = roller->sel_area[1].y1 + (font_sel_h>>1) - (font_main_h>>1);
	roller->main_area.y2 = roller->main_area.y1 + font_main_h - 1;


	roller->sel_area[2].x1 = roller_coords.x1;
	roller->sel_area[2].x2 = roller_coords.x2;
	roller->sel_area[2].y1 = start_y + (line_space >> 1);
	roller->sel_area[2].y2 = start_y + middle_height - 1 - (line_space >> 1);

	for(; text_area_id < roller->row_cnt + 1 ; text_area_id++)
	{
		lv_area_t *at_area = roller->text_area + text_area_id;
		at_area->x1 = roller_coords.x1;
		at_area->x2 = roller_coords.x2;
		at_area->y1 = start_y + (line_space >> 1);
		start_y += font_main_h + line_space;
		at_area->y2 = start_y - 1 -  + (line_space >> 1);
	}
}

