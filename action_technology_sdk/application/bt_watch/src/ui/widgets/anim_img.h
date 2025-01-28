/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file anim_img.h
 *
 */

#ifndef WIDGETS_IMG_ANIM_H_
#define WIDGETS_IMG_ANIM_H_

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <lvgl/lvgl.h>
#include <lvgl/lvgl_img_loader.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/** Callback used when the animation is ready or repeat ready. */
typedef void (*anim_img_ready_cb_t)(lv_obj_t *);

extern const lv_obj_class_t anim_img_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create an animated image object
 * @param parent pointer to an object, it will be the parent of the new object
 * @return pointer to the created animated image object
 */
lv_obj_t * anim_img_create(lv_obj_t * parent);

/**
 * Clean the image frame resource
 *
 * @param obj pointer to an image animation object
 */
void anim_img_clean(lv_obj_t * obj);

/*=====================
 * Setter functions
 *====================*/

/**
 * Set the resource image array where to get pixel map array
 *
 * @param obj pointer to an image animation object
 * @param src array of image
 * @param offset position translate offset array of the images
 * @param cnt number of image
 */
void anim_img_set_src_picarray(lv_obj_t *obj, const lv_image_dsc_t * src, const lv_point_t * offset, uint16_t cnt);

/**
 * Set the resource region where to get pixel map array
 *
 * @param obj pointer to an image animation object
 * @param scene_id scene id
 * @param picregion picture region where to load images
 */
void anim_img_set_src_picregion(lv_obj_t *obj, uint32_t scene_id, lvgl_res_picregion_t *picregion);

/**
 * Set the resource group where to get pixel map array
 *
 * @param obj pointer to an image animation object
 * @param scene_id scene id
 * @param picgroup picture group where to load images
 * @param ids child ids of picture group
 * @param cnt number of child ids
 */
void anim_img_set_src_picgroup(lv_obj_t *obj, uint32_t scene_id,
		lvgl_res_group_t *picgroup, const uint32_t *ids, uint16_t cnt);

/**
 * Set a delay and duration the animation
 *
 * @param obj pointer to an image animation object
 * @param delay delay before the animation in milliseconds
 * @param duration duration of the animation in milliseconds
 */
void anim_img_set_duration(lv_obj_t *obj, uint32_t delay, uint32_t duration);

/**
 * Make the animation repeat itself.
 *
 * @param obj pointer to an image animation object
 * @param delay delay delay in milliseconds before starting the playback animation.
 * @param count repeat count or `LV_ANIM_REPEAT_INFINITE` for infinite repetition. 0: to disable repetition.
 */
void anim_img_set_repeat(lv_obj_t *obj, uint32_t delay, uint32_t count);

/**
 * Enable/disable the preload during the animation.
 *
 * The preload is default enabled.
 *
 * @param obj pointer to an image animation object
 * @param en enabled or not.
 */
void anim_img_set_preload(lv_obj_t *obj, bool en);

/**
 * Set a function call when the animation is ready.
 *
 * @param obj pointer to an image animation object
 * @param ready_cb  a function call when the animation is ready or repeat ready
 */
void anim_img_set_ready_cb(lv_obj_t *obj, anim_img_ready_cb_t ready_cb);

/*=====================
 * Getter functions
 *====================*/

/**
 * Get the image count
 *
 * @param obj pointer to an image animation object
 * @return the image count
 */
uint16_t anim_img_get_src_count(lv_obj_t *obj);

/**
 * Get whether the animation is running
 *
 * @param obj pointer to an image animation object
 * @return true: running; false: not running
 */
bool anim_img_get_running(lv_obj_t *obj);

/*=====================
 * Other functions
 *====================*/

/**
 * Play one frame of the animation
 *
 * The animation must not be running.
 *
 * @param obj pointer to an image animation object
 * @param idx the frame index to play
 */
void anim_img_play_static(lv_obj_t *obj, uint16_t idx);

/**
 * Start the animation
 *
 * @param obj pointer to an image animation object
 * @param continued true: continue the last played frame
 *                  false: restart from the first frame
 */
void anim_img_start(lv_obj_t *obj, bool continued);

/**
 * Stop the animation
 *
 * @param obj pointer to an image animation object
 * @return the current play image index
 */
void anim_img_stop(lv_obj_t *obj);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* WIDGETS_IMG_ANIM_H_ */
