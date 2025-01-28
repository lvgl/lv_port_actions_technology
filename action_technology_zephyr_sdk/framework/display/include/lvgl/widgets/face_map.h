/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file face_map.h
 *
 */

#ifndef FRAMEWORK_DISPLAY_LIBDISPLAY_LVGL_WIDGETS_FACE_MAP_H_
#define FRAMEWORK_DISPLAY_LIBDISPLAY_LVGL_WIDGETS_FACE_MAP_H_

#ifdef CONFIG_VG_LITE

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <lvgl/lvgl.h>
#include <vg_lite/vglite_util.h>

/**********************
 *      TYPEDEFS
 **********************/

extern const lv_obj_class_t face_map_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create an cube map object
 * @param parent pointer to an object, it will be the parent of the new image object
 * @return pointer to the created image object
 */
lv_obj_t * face_map_create(lv_obj_t * parent);

/*=====================
 * Setter functions
 *====================*/

/**
 * Set the pixel map to display by the image
 * @param obj pointer to an image object
 * @param src pointer to an image source variable array
 */
void face_map_set_src(lv_obj_t *obj, const lv_image_dsc_t * src);

/**
 * Set the rotation center of the image.
 * The image will be rotated around this point
 * @param obj pointer to an image object
 * @param pivot_x rotation/zoom center x of the image
 * @param pivot_y rotation/zoom center y of the image
 * @param pivot_z rotation/zoom center z of the image
 */
void face_map_set_pivot(lv_obj_t * obj, float pivot_x, float pivot_y, float pivot_z);


/**
 * Set the observe of the image.
 * The image will be rotated around this point
 * @param obj pointer to an image object
 * @param pivot_x observe x of the image
 * @param pivot_y observe y of the image
 * @param pivot_z observe z of the image
 * @param on_off true-on observe
 */
void face_map_set_observe(lv_obj_t * obj, float observe_x, float observe_y, float observe_z, bool on_off);


/**
 * Set the periphery dot .
 * The image will be rotated around this point
 * @param obj pointer to an image object
 * @param vertex four dots coordinates
 */
void face_map_set_periphery_dot(lv_obj_t * obj, vertex_t *vertex);

/**
 * Rest periphery dot .
 * @param obj pointer to an image object
 * @param draw whether draw
 */
void face_map_periphery_dot_rest(lv_obj_t * obj,bool draw);

/**
 * Set the rotation angle vect.
 * The image will be rotated by ZYX order.
 * @param obj pointer to an image object
 * @param angle_x rotation angle around x-axis in degree with 0.1 degree resolution
 * @param angle_y rotation angle around y-axis in degree with 0.1 degree resolution
 * @param angle_z rotation angle around z-axis in degree with 0.1 degree resolution
 * @param draw whether draw
 */
void face_map_set_angle_vect(lv_obj_t * obj, int16_t angle_x, int16_t angle_y, int16_t angle_z ,bool draw);

/**
 * Set the whether reverse side no draw.
 * The image will be rotated by ZYX order.
 * @param obj pointer to an image object
 * @param normals whether reverse side no draw
 */
void face_map_set_normals(lv_obj_t * obj, bool normals);

/**
 * Set the zoom factor of the image.
 * @param obj       pointer to an image object
 * @param zoom      the zoom factor XYZ.
 * @example 256 or LV_ZOOM_IMG_NONE for no zoom
 * @example <256: scale down
 * @example >256 scale up
 * @example 128 half size
 * @example 512 double size
 */
void face_map_set_zoom(lv_obj_t * obj, uint16_t zoom);
void face_map_set_zoom_x(lv_obj_t * obj, uint16_t zoom_x);
void face_map_set_zoom_y(lv_obj_t * obj, uint16_t zoom_x);

void face_map_set_observe_zoom(lv_obj_t * obj, uint16_t zoom);

/*=====================
 * Getter functions
 *====================*/

/**
 * Get the source of the image.
 * @param obj pointer to an image object
 * @return the image source
 */
const void * face_map_get_src(lv_obj_t * obj);

/**
 * Get the periphery dot .
 * The image will be rotated around this point
 * @param obj pointer to an image object
 * @param vertex four dots coordinates
 */
void face_map_get_periphery_dot(lv_obj_t * obj, vertex_t *vertex);


/**
 * Get the area.
 * Get area Can be used for clicks, etc
 * @param obj pointer to an image object
 * @param area
 */
void face_map_get_area(lv_obj_t * obj, lv_area_t *area);

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
#endif /*FRAMEWORK_DISPLAY_LIBDISPLAY_LVGL_WIDGETS_FACE_MAP_H_*/
