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

/** Data of rotate image */
typedef struct {
	lv_obj_t obj;

	const void *src; /* image source */
	lv_img_header_t header; /* image header */
	lv_area_t trans_area;   /* image transformed area (relative to the top-left of the object) */
	lv_point_t pivot;  /* Rotation center of the image, relative to the top-left corner of the image */
	uint16_t angle;    /* Rotation angle in 0.1 degree [0, 3600) of the image */
	uint16_t zoom;

	uint8_t rounded : 1;
} simple_img_t;

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
void simple_img_set_angle(lv_obj_t * obj, int16_t angle);

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
void simple_img_set_zoom(lv_obj_t * obj, uint16_t zoom);

/**
 * Set if the image shape is rounded.
 *
 * If set rounded, the width and height of image src should be the same. And it
 * will ignored the pivot set by simple_img_set_pivot() and consider the pivot
 * as the center of image src.
 *
 * @param obj       pointer to an image object
 * @param rounded   the image shape is rounded or not.
 */
void simple_img_set_rounded(lv_obj_t * obj, bool rounded);

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
uint16_t simple_img_get_angle(lv_obj_t * obj);

/*=====================
 * Other functions
 *====================*/

/**********************
 *      MACROS
 **********************/

/*Use this macro to declare an image in a c file*/
#ifndef LV_IMG_DECLARE
#define LV_IMG_DECLARE(var_name) extern const lv_img_dsc_t var_name;
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*WIDGETS_SIMPLE_IMG_H_*/
