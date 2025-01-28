/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file text_arc.h
 *
 */

#ifndef _TEXT_ARC_CANVAS_H_
#define _TEXT_ARC_CANVAS_H_

#ifdef __cplusplus
extern "C" {
#endif
/*********************
*      EXAMPLE		
*********************/
/*lv_obj_t *text_arc = text_arc_create(data->tv);
lv_obj_set_pos(text_arc,200,200);
lv_obj_set_size(text_arc,100,100);
lv_obj_set_style_text_font(text_arc,&data->font,LV_PART_MAIN);
lv_obj_set_style_text_color(text_arc,lv_color_white(),LV_PART_MAIN);
text_arc_set_radian(text_arc, 50, 80, 30);
text_arc_set_text(text_arc,"12345");
text_arc_refresh(text_arc);
*/

/*********************
 *      INCLUDES
 *********************/
#include <lvgl/lvgl.h>
//#include "draw_util.h"

/*********************
 *      DEFINES
 *********************/


/**********************
 *      TYPEDEFS
 **********************/

/** Data of text canvas */
typedef void (*fine_tuning_angle_cb_t)(uint32_t letter, int16_t *st_angle, int16_t *end_angle);

extern const lv_obj_class_t text_arc_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create an text arc object
 * @param parent pointer to an object, it will be the parent of the new object
 * @return pointer to the created text arc object
 */
lv_obj_t * text_arc_create(lv_obj_t * parent);

/**
 * Set text content
 * @param text_arc widget
 * @param text content
 */
void text_arc_set_text(lv_obj_t *text_arc , char *text);

/**
 * Set text property
 * @param obj widget
 * @param radius radius
 * @param angle_st begin angle
 * @param interval_angle each number interval angle
 */
void text_arc_set_radian(lv_obj_t *obj, uint16_t radius, int16_t angle_st, int16_t interval_angle);

/**
 * Set text arc center
 * @param obj widget
 * @param b_center center by angle_st
 */
void text_arc_set_center(lv_obj_t *obj, bool b_center);

/**
 * Set text arc overturn
 * @param obj widget
 * @param b_overturn overturn
 */
void text_arc_set_overturn(lv_obj_t *obj, bool b_overturn);

/**
 * Use for part unicode fine tuning angle call-back
 * @param obj widget
 * @param b_overturn overturn
 */
void text_arc_set_tuning_cb(lv_obj_t *obj, fine_tuning_angle_cb_t cb);

/**
 * text arc refresh
 * @param obj widget
 */
void text_arc_refresh(lv_obj_t *obj);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* WIDGETS_TEXT_CANVAS_H_ */
