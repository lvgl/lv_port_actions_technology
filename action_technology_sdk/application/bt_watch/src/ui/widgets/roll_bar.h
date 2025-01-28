/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file roll_bar.h
 *
 */

#ifndef WIDGETS_ROLL_BAR_H_
#define WIDGETS_ROLL_BAR_H_

#ifdef __cplusplus
extern "C" {
#endif


/*********************
 *      REFERENCE
 *********************/
//Attention : Do not use bar user_data
/*
static lv_obj_t *opa_obj = NULL;

void view_roll_bar_cb(int32_t v)
{
	lv_obj_set_style_bg_opa(opa_obj,v,LV_PART_MAIN);
}

void bar_create(lv_obj_t *obj)
{
	lv_obj_t *bar = roll_bar_create(obj);
	lv_obj_set_size(bar,10,100);
	roll_bar_set_color(bar,lv_color_white(),lv_color_hex(0xFF0000));
	roll_bar_set_min(bar,10);
	roll_bar_set_opa(bar,LV_OPA_0,LV_OPA_100);
	lv_obj_set_style_radius(bar,5,LV_PART_MAIN);
	roll_bar_set_fade(bar,true);
	//	close auto roll mod
	//roll_bar_set_auto(bar, false);
	//roll_bar_set_roll_value(bar,50);
	lv_obj_align(bar,LV_ALIGN_CENTER,0,0);
	roll_bar_set_fade_cb(bar,view_roll_bar_cb);

	lv_obj_t *roll_arc = roll_arc_create(obj);
	roll_arc_set_diameter_width(roll_arc,100,10);
	roll_arc_set_bg_angles(roll_arc,0,180);
	roll_arc_set_rotation(roll_arc,270);
	roll_bar_set_opa(roll_arc,LV_OPA_0,LV_OPA_100);
	roll_bar_set_color(roll_arc,lv_color_white(),lv_color_hex(0xFF0000));
	lv_obj_center(roll_arc);

	opa_obj = lv_obj_create(obj);
}
*/

/*********************
 *      INCLUDES
 *********************/
#include <lvgl/lvgl.h>

/*********************
 *      DEFINES
 *********************/
enum {
	ROLL_HOR_MOD = 0,	//lengthways
	ROLL_VER_MOD,	//crosswise
};

#define ROLL_BAR_MAX 1000

typedef void (*roll_bar_cb_t)(int32_t v);	//roll_bar transform trigger cb

/**********************
 *      TYPEDEFS
 **********************/


/**********************
 * GLOBAL PROTOTYPES
 **********************/
/**
 * Create an roll_bar object
 * @param parent pointer to an object, it will be the parent of the new object
 * @return pointer to the created roll_bar object
 */
lv_obj_t * roll_bar_create(lv_obj_t * parent);

/**
 * Create an roll_arc object
 * @param parent pointer to an object, it will be the parent of the new object
 * @return pointer to the created roll_arc object
 */
lv_obj_t * roll_arc_create(lv_obj_t * parent);

/**
 * Set roll_arc diameter and width
 * @param obj widget
 * @param dia diameter
 * @param arc_width arc width
 */
void roll_arc_set_diameter_width(lv_obj_t * obj,uint16_t dia,uint16_t arc_width);

/**
 * Set roll_arc st and end angles
 * @param obj widget
 * @param start start angles
 * @param end end angles
 */
void roll_arc_set_bg_angles(lv_obj_t * obj,uint16_t start, uint16_t end);

/**
 * Set roll_arc rotation
 * @param obj widget
 * @param rotation rotation
 */
void roll_arc_set_rotation(lv_obj_t * obj,uint16_t rotation);

/**
 * Set roll_bar direction
 * @param obj widget
 * @param mod ROLL_HOR_MOD or ROLL_VER_MOD
 */
void roll_bar_set_direction(lv_obj_t * obj, uint8_t mod);

/**
 * Set roll_bar fade over opa
 * @param obj widget
 * @param min_opa min opa
 * @param max_opa max opa
 */
void roll_bar_set_opa(lv_obj_t * obj, uint8_t min_opa , uint8_t max_opa);

/**
 * Open roll_bar fade over anim
 * @param obj widget
 * @param fade true:open ,false:close
 */
void roll_bar_set_fade(lv_obj_t * obj, bool fade);

/**
 * Set bar min width or height,arc min value
 * @param obj widget
 * @param min_value min value >= 0
 */
void roll_bar_set_min(lv_obj_t * obj, uint16_t min_value);

/**
 * Set roll_bar up down color
 * @param obj widget
 * @param color down color
 * @param color_1 up color
 */
void roll_bar_set_color(lv_obj_t * obj, lv_color_t color , lv_color_t color_1);

/**
 * Set roll_bar auto follow parent roll value transform
 * @param obj widget
 * @param b_auto true:parent auto transform ,false:user set value
 */
void roll_bar_set_auto(lv_obj_t * obj, bool b_auto);

/**
 * Set roll_bar value
 * @param obj widget
 * @param value roll_bar value
 */
void roll_bar_set_roll_value(lv_obj_t * obj, lv_coord_t value);

/**
 * Roll_bar transform cb
 * @param obj widget
 * @param cb roll_bar transform trigger cb
 */
void roll_bar_set_fade_cb(lv_obj_t * obj, roll_bar_cb_t cb);

/**
 * Execute roll_bar fade over result
 * @param obj widget
 * @param fade_in true:min opa to max opa ,false:max opa to min opa
 */
void roll_bar_set_fade_in_out(lv_obj_t * obj, bool fade_in);

/**
 * Set roll_bar current opa
 * @param obj widget
 * @param value current opa value
 */
void roll_bar_set_current_opa(lv_obj_t * obj, lv_opa_t value);

/*=====================
 * Getter functions
 *====================*/
/**
 * Get roll_bar down obj
 */
lv_obj_t *roll_bar_get_obj(lv_obj_t * obj);

/**
 * Get roll_bar up obj
 */
lv_obj_t *roll_bar_get_obj_up(lv_obj_t * obj);

/*=====================
 * Other functions
 *====================*/

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* WIDGETS_roll_bar_H_ */
