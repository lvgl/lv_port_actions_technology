/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file watch_list.h
 *
 */

#ifndef WIDGETS_WATCH_LIST_H_
#define WIDGETS_WATCH_LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <lvgl/lvgl.h>
#include "simple_img.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef void (*watch_list_scroll_cb_t)(lv_obj_t *list, lv_point_t *icon_center);

extern const lv_obj_class_t watch_list_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create an list object
 * @param par pointer to an object, it will be the parent of the new object
 * @return pointer to the created list object
 */
lv_obj_t * watch_list_create(lv_obj_t * parent);

/**
 * Delete all children of the object
 * @param obj pointer to an object
 */
void watch_list_clean(lv_obj_t * obj);

/*=====================
 * Setter functions
 *====================*/

/**
 * Add list elements to the list
 *
 * The text will be stored as A4 image internally. 
 *
 * @param obj pointer to list object
 * @param icon icon image before the text
 * @param text text of the list element
 * @return pointer to the new list element which can be customized (a button)
 */
lv_obj_t * watch_list_add_btn(lv_obj_t *obj, const lv_image_dsc_t * icon, const char * text);

/**
 * Remove the index of the element in the list
 * @param obj pointer to a list object
 * @param index the element index in the list
 */
void watch_list_remove(lv_obj_t * obj, uint16_t index);

/**
 * Set list icon pivot compution callback
 * @param obj pointer to list object
 * @param scroll_cb callback to provide the expected pivot y of the icon (provided the pivot x and y)
 */
void watch_list_set_scroll_cb(lv_obj_t *obj, watch_list_scroll_cb_t scroll_cb);

/*=====================
 * Getter functions
 *====================*/

/**
 * Get the index of the button in the list
 * @param obj pointer to a list object. If NULL, assumes btn is part of a list.
 * @param btn pointer to a list element (button)
 * @return the index of the button in the list, or -1 of the button not in this list
 */
int16_t watch_list_get_btn_index(const lv_obj_t * obj, const lv_obj_t * btn);

/**
 * Get the text image object from a list element
 * @param btn pointer to a list element (button)
 * @return pointer to the text image from the list element or NULL if not found
 */
lv_obj_t * watch_list_get_icon(const lv_obj_t * obj, uint16_t index);

/**
 * Get the image object from a list element
 * @param btn pointer to a list element (button)
 * @return pointer to the icon image from the list element or NULL if not found
 */
lv_obj_t * watch_list_get_text(const lv_obj_t * obj, uint16_t index);

/**
 * Get the selected (clicked) item index
 * @param list pointer to a list object
 * @return the index of the selected (clicked) item or -1 if not found
 */
int16_t watch_list_get_sel_index(const lv_obj_t * obj);

/**
 * Get the number of buttons in the list
 * @param list pointer to a list object
 * @return the number of buttons in the list
 */
uint16_t watch_list_get_size(const lv_obj_t * obj);

/*=====================
 * Other functions
 *====================*/

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* WIDGETS_WATCH_LIST_H_ */
