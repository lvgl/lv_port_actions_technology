/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file simple_img.h
 *
 */

#ifndef WIDGETS_SIMPLE_IMG_H_
#define WIDGETS_SIMPLE_IMG_H_

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <lvgl/lvgl.h>
#include <display/sw_rotate.h>


/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

extern const lv_obj_class_t simple_img_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create an simple image objects
 * @param parent pointer to an object, it will be the parent of the new image object
 * @return pointer to the created image object
 */
lv_obj_t * simple_img_create(lv_obj_t * parent);

/*=====================
 * Setter functions
 *====================*/

/**
 * Set the pixel map to display by the image
 * @param obj pointer to an image object
 * @param src pointer to an image source (a C array or path to a file)
 */
void simple_img_set_src(lv_obj_t *obj, const void * src);

/**
 * Set the rotation center of the image.
 * The image will be rotated around this point
 * @param obj pointer to an image object
 * @param pivot_x rotation/zoom center x of the image
 * @param pivot_y rotation/zoom center y of the image
 */
void simple_img_set_pivot(lv_obj_t * obj, lv_coord_t pivot_x, lv_coord_t pivot_y);

/**
 * Set the rotation angle of the image.
 * The image will be rotated around the set pivot set by `simple_img_set_pivot()`
 * @param obj pointer to an image object
 * @param angle rotation angle in degree with 0.1 degree resolution ([0, 3600): clock wise)
 */
void simple_img_set_rotation(lv_obj_t * obj, int16_t angle);

/**
 * Set the zoom factor of the image.
 * @param obj       pointer to an image object
 * @param zoom      the zoom factor.
 * @example 256 or LV_ZOOM_IMG_NONE for no zoom
 * @example <256: scale down
 * @example >256 scale up
 * @example 128 half size
 * @example 512 double size
 */
void simple_img_set_scale(lv_obj_t * obj, uint16_t zoom);

/*=====================
 * Getter functions
 *====================*/

/**
 * Get the source of the image
 * @param obj pointer to an image object
 * @return the image source (symbol, file name or C array)
 */
const void * simple_img_get_src(lv_obj_t * obj);

/**
 * Get the rotation center of the image.
 * @param obj pointer to an image object
 * @param pivot rotation center of the image
 */
void simple_img_get_pivot(lv_obj_t * obj, lv_point_t * pivot);

/**
 * Get the rotation angle of the image.
 * @param obj pointer to an image object
 * @return rotation angle in 0.1 degree [0, 3600)
 */
uint16_t simple_image_get_rotation(lv_obj_t * obj);

/**
 * Get the zoom factor of the image.
 * @param obj       pointer to an image object
 * @return          zoom factor (256: no zoom)
 */
uint16_t simple_image_get_scale(lv_obj_t * obj);

/*=====================
 * Other functions
 *====================*/

/**********************
 *      MACROS
 **********************/

/*Use this macro to declare an image in a c file*/
#ifndef LV_IMG_DECLARE
#define LV_IMG_DECLARE(var_name) extern const lv_image_dsc_t var_name;
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*WIDGETS_SIMPLE_IMG_H_*/
