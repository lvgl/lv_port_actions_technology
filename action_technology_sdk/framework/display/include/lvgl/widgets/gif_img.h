/**
 * @file gif_img.h
 *
 */

#ifndef GIF_IMG_H
#define GIF_IMG_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <lvgl/lvgl.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

extern const lv_obj_class_t gif_img_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create a gif object
 * @param parent    pointer to an object, it will be the parent of the new gif.
 * @return          pointer to the gif obj
 */
lv_obj_t * gif_img_create(lv_obj_t * parent);

/**
 * Set the gif data to display on the object
 * @param obj       pointer to a gif object
 * @param src       1) pointer to an ::lv_image_dsc_t descriptor (which contains gif raw data) or
 *                  2) path to a gif file (e.g. "S:/dir/anim.gif")
 * @param optimized indicate using the optimized decompression method or not
 */
void gif_img_set_src(lv_obj_t * obj, const void * src, bool optimized);

/**
 * Restart a gif animation.
 * @param obj pointer to a gif obj
 */
void gif_img_restart(lv_obj_t * gif);

/**
 * Pause a gif animation.
 * @param obj pointer to a gif obj
 */
void gif_img_pause(lv_obj_t * obj);

/**
 * Pause a gif animation.
 * @param obj pointer to a gif obj
 */
void gif_img_resume(lv_obj_t * obj);

/**
 * Pause a gif animation.
 * @param obj pointer to a gif obj
 */
bool gif_img_get_paused(lv_obj_t * obj);

/**********************
 *      MACROS
 **********************/


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_GIF_H*/
