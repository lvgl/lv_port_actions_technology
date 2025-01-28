/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file img2d_map.h
 *
 */

#ifndef FRAMEWORK_DISPLAY_LIBDISPLAY_LVGL_WIDGETS_IMG2D_MAP_H_
#define FRAMEWORK_DISPLAY_LIBDISPLAY_LVGL_WIDGETS_IMG2D_MAP_H_

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
#define IMG2D_MAP_USE_BLIT_WAR 1

enum {
	IMG2D_FACE_NONE  = 0x0,
	IMG2D_FACE_FRONT = 0x1,
	IMG2D_FACE_BACK  = 0x2,
};

typedef uint8_t img2d_face_t;

/**********************
 *      TYPEDEFS
 **********************/

extern const lv_obj_class_t img2d_map_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create an simple image objects
 * @param parent pointer to an object, it will be the parent of the new image object
 * @return pointer to the created image object
 */
lv_obj_t * img2d_map_create(lv_obj_t * parent);

/*=====================
 * Setter functions
 *====================*/

/**
 * Set the pixel map to display by the image
 * @param obj pointer to an image object
 * @param src pointer to an image source variable
 */
void img2d_map_set_src(lv_obj_t *obj, const lv_image_dsc_t * src);

/**
 * Set the rotation center of the image.
 * The image will be rotated around this point
 * @param obj pointer to an image object
 * @param pivot_x rotation/zoom center x of the image
 * @param pivot_y rotation/zoom center y of the image
 * @param pivot_z rotation/zoom center z of the image
 */
void img2d_map_set_pivot(lv_obj_t * obj, float pivot_x, float pivot_y, float pivot_z);

/**
 * Set the rotation angle around x-axis.
 * The image will be rotated around the set pivot set by `img2d_map_set_pivot()`
 * @param obj pointer to an image object
 * @param angle rotation angle in degree with 0.1 degree resolution
 */
void img2d_map_set_angle_x(lv_obj_t * obj, int16_t angle);

/**
 * Set the rotation angle around y-axis.
 * The image will be rotated around the set pivot set by `img2d_map_set_pivot()`
 * @param obj pointer to an image object
 * @param angle rotation angle in degree with 0.1 degree resolution
 */
void img2d_map_set_angle_y(lv_obj_t * obj, int16_t angle);

/**
 * Set the rotation angle around z-axis.
 * The image will be rotated around the set pivot set by `img2d_map_set_pivot()`
 * @param obj pointer to an image object
 * @param angle rotation angle in degree with 0.1 degree resolution
 */
void img2d_map_set_angle_z(lv_obj_t * obj, int16_t angle);

/**
 * Set the rotation angle.
 * @param obj pointer to an image object
 * @param angle_x rotation angle around x-axis in degree with 0.1 degree resolution
 * @param angle_y rotation angle around y-axis in degree with 0.1 degree resolution
 * @param angle_z rotation angle around z-axis in degree with 0.1 degree resolution
 */
void img2d_map_set_angle(lv_obj_t * obj, int16_t angle_x, int16_t angle_y, int16_t angle_z);

/**
 * Set axis rotation order
 * @param obj pointer to an image object
 * @param order axis rotation order
 */
void img2d_map_set_rotation_order(lv_obj_t * obj, uint8_t order);

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
void img2d_map_set_zoom(lv_obj_t * obj, uint16_t zoom);

/**
 * Cull face.
 * @param obj      pointer to an image object
 * @param face     face to cull, initial value is IMG2D_FACE_NONE.
 */
void img2d_map_set_cull(lv_obj_t * obj, img2d_face_t face);

/*=====================
 * Getter functions
 *====================*/

/**
 * Query the image is visible or not
 * @param obj pointer to an image object
 * @return visible or not
 */
bool img2d_map_is_visible(lv_obj_t * obj);

/**
 * Query which face is visible
 * @param obj pointer to an image object
 * @return face ID
 */
img2d_face_t img2d_map_get_visible_face(lv_obj_t * obj);

/**
 * Get the source of the image
 * @param obj pointer to an image object
 * @return the image source (symbol, file name or C array)
 */
const void * img2d_map_get_src(lv_obj_t * obj);

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
#endif /*FRAMEWORK_DISPLAY_LIBDISPLAY_LVGL_WIDGETS_IMG2D_MAP_H_*/
