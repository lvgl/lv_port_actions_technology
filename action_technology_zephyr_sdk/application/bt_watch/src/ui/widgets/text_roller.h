/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file text_roller.h
 *
 */

#ifndef _TEXT_ROLLER_CANVAS_H_
#define _TEXT_ROLLER_CANVAS_H_

#ifdef __cplusplus
extern "C" {
#endif

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
typedef void (*text_id_update_cb_t)(struct _lv_obj_t *obj);

extern const lv_obj_class_t text_roller_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create an text rollers object
 * @param parent pointer to an object, it will be the parent of the new object
 * @return pointer to the created text rollers object
 */
lv_obj_t * text_roller_create(lv_obj_t * parent);


/**
 * set text rollers show num(min 3 and odd number)
 * @param buf text list , attention not part
 */
void text_roller_set_visible_row_count(lv_obj_t *obj , uint8_t cnt);

/**
 * set text rollers text id
 * @param id text list at id
 */
void text_roller_set_text_id(lv_obj_t *obj , uint32_t id);

/**
 * set text rollers str
 * @param buf text str list
 * @param str_byte str byte	(abandon param)
 * @param str_num text str list num(min 3)
 */
void text_roller_set_str(lv_obj_t *obj , char **str, uint32_t str_byte,uint32_t str_num);

/**
 * set text rollers str
 * @param num_start text num list start
 * @param num_end text num list end
 * @param increment text num list increment (num_end = num_start + increment * x)
 * @param num_digit show num digit
 */
void text_roller_set_num(lv_obj_t *obj , lv_coord_t num_start , lv_coord_t num_end , lv_coord_t increment);

/**
 * set num digit
 * @param num_digit show num digit
 */
void text_roller_set_num_digit(lv_obj_t *obj , uint8_t num_digit);

/**
 * set update cb
 * @param update_cb cb
 */
void text_roller_set_update_cb(lv_obj_t *obj , text_id_update_cb_t update_cb);

/**
 * stop rolling
 */
void text_roller_stop(lv_obj_t *obj);

/**
 * set text rollers text cycle
 * @param cycle 
 */
void text_roller_set_cycle(lv_obj_t *obj , bool cycle);

/**
 * set text rollers open click update id
 * @param click 
 */
void text_roller_open_click_update(lv_obj_t *obj , bool click);

/*=====================
 * Getter functions
 *====================*/
uint8_t text_roller_get_visible_row_count(lv_obj_t *obj);

lv_coord_t text_roller_get_texe_id(lv_obj_t *obj);

/*=====================
 * Other functions
 *====================*/

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* WIDGETS_TEXT_CANVAS_H_ */
