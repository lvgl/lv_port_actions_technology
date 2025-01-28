/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file svg_img.h
 *
 */

#ifndef WIDGETS_SVG_IMG_H_
#define WIDGETS_SVG_IMG_H_

#ifdef CONFIG_VG_LITE

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <lvgl/lvgl.h>
#include "../svgrender/vglite_renderer.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/** Data of rotate image */
typedef struct {
	lv_obj_t obj;

	struct NSVGimage *image;
	int w;
	int h;

	lv_area_t trans_area; /* image transformed area (relative to the top-left of the object) */
	lv_point_t pivot;     /* Rotation center of the image, relative to the top-left corner of the image */
	uint16_t angle;       /* Rotation angle in 0.1 degree [0, 3600) of the image */
	uint16_t zoom;
} svg_img_t;

extern const lv_obj_class_t svg_img_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create an svg image objects
 * @param parent pointer to an object, it will be the parent of the new image object
 * @return pointer to the created object
 */
lv_obj_t * svg_img_create(lv_obj_t * parent);

/*=====================
 * Setter functions
 *====================*/
/**
 * Set the pixel map to display by the image
 *
 * Important note: the data will be changed.
 *
 * @param obj pointer to an image object
 * @param data pointer to binary data.
 * @param len length of data.
 * @return LV_RES_OK on success else LV_RES_INV.
 */
lv_res_t svg_img_parse(lv_obj_t *obj, const void * data, size_t len);

/**
 * Set the pixel map to display by the image
 * @param obj pointer to an image object
 * @param filename pointer to data string with null terminated '/0'
 * @return LV_RES_OK on success else LV_RES_INV.
 */
lv_res_t svg_img_parse_file(lv_obj_t *obj, const char * filename);

/**
 * Set the rotation center of the image.
 * The image will be rotated around this point
 * @param obj pointer to an image object
 * @param pivot_x rotation/zoom center x of the image
 * @param pivot_y rotation/zoom center y of the image
 */
void svg_img_set_pivot(lv_obj_t * obj, lv_coord_t pivot_x, lv_coord_t pivot_y);

/**
 * Set the rotation angle of the image.
 * The image will be rotated around the set pivot set by `svg_img_set_pivot()`
 * @param obj pointer to an image object
 * @param angle rotation angle in degree with 0.1 degree resolution ([0, 3600): clock wise)
 */
void svg_img_set_angle(lv_obj_t * obj, int16_t angle);

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
void svg_img_set_zoom(lv_obj_t * obj, uint16_t zoom);

/*=====================
 * Getter functions
 *====================*/

/**
 * Get the pivot (rotation center) of the image.
 * @param obj pointer to an image object
 * @param pivot store the rotation center here
 */
void svg_img_get_pivot(lv_obj_t * obj, lv_point_t * pivot);

/**
 * Get the rotation angle of the image.
 * @param obj pointer to an image object
 * @return rotation angle in 0.1 degrees [0, 3600)
 */
uint16_t svg_img_get_angle(lv_obj_t * obj);

/**
 * Get the zoom factor of the image.
 * @param obj pointer to an image object
 * @return    zoom factor (256: no zoom)
 */
uint16_t svg_img_get_zoom(lv_obj_t * obj);

/**
 * Get the SVG size.
 * @param obj pointer to an image object
 * @param size_res pointer to a 'point_t' variable to store the result
 */
void svg_img_get_size(lv_obj_t * obj, lv_point_t * size_res);

/*=====================
 * Other functions
 *====================*/

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CONFIG_VG_LITE */
#endif /*WIDGETS_SVG_IMG_H_*/
