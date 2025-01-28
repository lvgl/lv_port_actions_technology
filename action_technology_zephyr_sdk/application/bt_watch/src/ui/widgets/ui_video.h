/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#if CONFIG_VIDEO_PLAYER
/**
 * @file ui_video.h
 *
 */

#ifndef _UI_VIDEO_CANVAS_H_
#define _UI_VIDEO_CANVAS_H_

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <lvgl/lvgl.h>
#include "vp_engine.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef void (*ui_video_play_cb_t)(lv_obj_t *obj);

extern const lv_obj_class_t ui_video_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create an ui video object
 * @param parent pointer to an object, it will be the parent of the new object
 * @return pointer to the created ui video object
 */
lv_obj_t *ui_video_create(lv_obj_t * parent);

/**
 * Set ui_video the path
 * @param obj pointer to an image object
 * @param path pointer to an path , cannot open the same path
 */
void ui_video_set_path(lv_obj_t *obj, const char *path);

/**
 * Set the rotation center of the ui_video.
 * The image will be rotated around this point
 * @param obj pointer to an ui_video object
 * @param pivot_x rotation/zoom center x of the image
 * @param pivot_y rotation/zoom center y of the image
 */
void ui_video_set_pivot(lv_obj_t * obj, lv_coord_t pivot_x, lv_coord_t pivot_y);

/**
 * Set the rotation angle of the ui_video.
 * The image will be rotated around the set pivot set by `simple_img_set_pivot()`
 * @param obj pointer to an ui_video object
 * @param angle rotation angle in degree with 0.1 degree resolution ([0, 3600): clock wise)
 */
void ui_video_set_rotation(lv_obj_t * obj, int16_t angle);

/**
 * Set the zoom factor of the ui_video.
 * @param obj       pointer to an ui_video object
 * @param zoom      the zoom factor.
 * @example 256 or LV_ZOOM_IMG_NONE for no zoom
 * @example <256: scale down
 * @example >256 scale up
 * @example 128 half size
 * @example 512 double size
 */
void ui_video_set_scale(lv_obj_t * obj, uint16_t zoom);

/**
 * Resume/pause a ui_video.
 */
void ui_video_resume(lv_obj_t *obj);
void ui_video_pause(lv_obj_t *obj);

/**
 * Reset a ui_video.
 */
void ui_video_reset(lv_obj_t *obj);


/**
 * Set ui_video play_cb.
 */
void ui_video_play_cb(lv_obj_t *obj, ui_video_play_cb_t cb);

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
#endif
