/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file cube_map.h
 *
 */

#ifndef FRAMEWORK_DISPLAY_LIBDISPLAY_LVGL_WIDGETS_CUBE_MAP_H_
#define FRAMEWORK_DISPLAY_LIBDISPLAY_LVGL_WIDGETS_CUBE_MAP_H_

#ifdef CONFIG_VG_LITE

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <lvgl/lvgl.h>
#include <vg_lite/vglite_util.h>

/*********************
 *      DEFINES
 *********************/
/* workaround for vg_lite_blit() filter whose center located outside of image */
#define CUBE_MAP_USE_BLIT_WAR 1

#define CUBE_MAP_FRONT   0
#define CUBE_MAP_BACK    1
#define CUBE_MAP_RIGHT   2
#define CUBE_MAP_LEFT    3
#define CUBE_MAP_BOTTOM  4
#define CUBE_MAP_TOP     5

#define NUM_CUBE_FACES   6
#define NUM_CUBE_VERTS   8

/**********************
 *      TYPEDEFS
 **********************/

extern const lv_obj_class_t cube_map_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create an cube map object
 * @param parent pointer to an object, it will be the parent of the new image object
 * @return pointer to the created image object
 */
lv_obj_t * cube_map_create(lv_obj_t * parent);

/*=====================
 * Setter functions
 *====================*/

/**
 * Set the pixel map to display by the image
 * @param obj pointer to an image object
 * @param src pointer to an image source variable array
 */
void cube_map_set_src(lv_obj_t *obj, const lv_image_dsc_t * src);

/**
 * Set the rotation center of the image.
 * The image will be rotated around this point
 * @param obj pointer to an image object
 * @param pivot_x rotation/zoom center x of the image
 * @param pivot_y rotation/zoom center y of the image
 * @param pivot_z rotation/zoom center z of the image
 */
void cube_map_set_pivot(lv_obj_t * obj, float pivot_x, float pivot_y, float pivot_z);

/**
 * Set the rotation angle around x-axis.
 * The image will be rotated around the set pivot set by `cube_map_set_pivot()`
 * @param obj pointer to an image object
 * @param angle rotation angle in degree with 0.1 degree resolution
 */
void cube_map_set_angle_x(lv_obj_t * obj, int16_t angle);

/**
 * Set the rotation angle around y-axis.
 * The image will be rotated around the set pivot set by `cube_map_set_pivot()`
 * @param obj pointer to an image object
 * @param angle rotation angle in degree with 0.1 degree resolution
 */
void cube_map_set_angle_y(lv_obj_t * obj, int16_t angle);

/**
 * Set the rotation angle around z-axis.
 * The image will be rotated around the set pivot set by `cube_map_set_pivot()`
 * @param obj pointer to an image object
 * @param angle rotation angle in degree with 0.1 degree resolution
 */
void cube_map_set_angle_z(lv_obj_t * obj, int16_t angle);

/**
 * Set the rotation angle.
 * The image will be rotated by ZYX order.
 * @param obj pointer to an image object
 * @param angle_x rotation angle around x-axis in degree with 0.1 degree resolution
 * @param angle_y rotation angle around y-axis in degree with 0.1 degree resolution
 * @param angle_z rotation angle around z-axis in degree with 0.1 degree resolution
 */
void cube_map_set_angle(lv_obj_t * obj, int16_t angle_x, int16_t angle_y, int16_t angle_z);

/**
 * Set axis rotation order
 * @param obj pointer to an image object
 * @param order axis rotation order
 */
void cube_map_set_rotation_order(lv_obj_t * obj, uint8_t order);

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
void cube_map_set_zoom(lv_obj_t * obj, uint16_t zoom);

/**
 * Reset the appended extra angles.
 * @param obj pointer to an image object
 */
void cube_map_reset_appended_angles(lv_obj_t * obj);

/**
 * Append a extra rotation angle around x-axis after Euler angle rotation.
 * The image will be rotated around the set pivot set by `cube_map_set_pivot()`
 * @param obj pointer to an image object
 * @param angle rotation angle in degree with 0.1 degree resolution
 */
void cube_map_append_angle_x(lv_obj_t * obj, int16_t angle);

/**
 * Append a extra rotation angle around y-axis after Euler angle rotation.
 * The image will be rotated around the set pivot set by `cube_map_set_pivot()`
 * @param obj pointer to an image object
 * @param angle rotation angle in degree with 0.1 degree resolution
 */
void cube_map_append_angle_y(lv_obj_t * obj, int16_t angle);

/**
 * Append a extra rotation angle around z-axis after Euler angle rotation.
 * The image will be rotated around the set pivot set by `cube_map_set_pivot()`
 * @param obj pointer to an image object
 * @param angle rotation angle in degree with 0.1 degree resolution
 */
void cube_map_append_angle_z(lv_obj_t * obj, int16_t angle);

/*=====================
 * Getter functions
 *====================*/

/**
 * Get the source of the image.
 * @param obj pointer to an image object
 * @param face which image of face to get
 * @return the image source (symbol, file name or C array)
 */
const void * cube_map_get_src(lv_obj_t * obj, uint8_t face);

/**
 * Get the 8 vertices of the image.
 * @param obj pointer to an image object
 * @param face the image face
 * @param verts where to store the vertices (clock-wise sequence started from top-left corner)
 * @return N/A
 */
void cube_map_get_vertices(lv_obj_t * obj, uint8_t face, vertex_t * verts);

/**
 * Query the cube face is visible or not.
 * @param obj pointer to an image object
 * @param face which image of face to get
 * @return visible or not
 */
bool cube_map_is_visible(lv_obj_t * obj, uint8_t face);

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
#endif /*FRAMEWORK_DISPLAY_LIBDISPLAY_LVGL_WIDGETS_CUBE_MAP_H_*/
