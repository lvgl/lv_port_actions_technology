/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file compass.h
 *
 */

#ifndef WIDGETS_COMPASS_H_
#define WIDGETS_COMPASS_H_

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

/* compass charactor image index */
enum {
	COMPASS_CHAR_0 = 0,
	COMPASS_CHAR_1,
	COMPASS_CHAR_2,
	COMPASS_CHAR_3,
	COMPASS_CHAR_4,
	COMPASS_CHAR_5,
	COMPASS_CHAR_6,
	COMPASS_CHAR_7,
	COMPASS_CHAR_8,
	COMPASS_CHAR_9,
	COMPASS_CHAR_DEGREE,
	COMPASS_CHAR_EAST,
	COMPASS_CHAR_SOUTH,
	COMPASS_CHAR_WEST,
	COMPASS_CHAR_NORTH,

	NUM_COMPASS_CHARS,
};

typedef uint8_t compass_char_t;

/**********************
 *      TYPEDEFS
 **********************/

extern const lv_obj_class_t compass_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create an compass object
 * @param parent pointer to an object, it will be the parent of the new object
 * @return pointer to the created compass object
 */
lv_obj_t * compass_create(lv_obj_t * parent);

/*=====================
 * Setter functions
 *====================*/

/**
 * Set the main image pixel map array
 *
 * @param obj pointer to an compass object
 * @param src main image which contains a circular image that will be rotated
 *            according the bearing. The image may has a circular hole at the center.
 * @param src_rot180 main image which has been rotated 180 degree, can be NULL.
 * @param hole_diameter the circular hole diameter, can be 0
 */
void compass_set_main_image(lv_obj_t *obj, const lv_image_dsc_t * src,
		const lv_image_dsc_t * src_rot180, uint16_t hole_diameter);

/**
 * Set the center decor image pixel map array
 *
 * @param obj pointer to an compass object
 * @param src decor image
 */
void compass_set_decor_image(lv_obj_t *obj, const lv_image_dsc_t * src);

/**
 * Set the charactor image label
 *
 * @param obj pointer to an compass object
 * @param srcs charactor image label
 * @param count number of images, must equal to NUM_COMPASS_CHARS
 */
void compass_set_char_image(lv_obj_t *obj, const lv_image_dsc_t * srcs, uint16_t count);

/**
 * Set the bearing
 *
 * @param obj pointer to an compass object
 * @param bearing clock-wise angle in degree
 */
void compass_set_bearing(lv_obj_t *obj, uint16_t bearing);

/*=====================
 * Getter functions
 *====================*/

/**
 * Get the bearing
 *
 * @param obj pointer to an compass object
 * @return the bearing in degree
 */
uint16_t compass_get_bearing(lv_obj_t *obj);

/*=====================
 * Other functions
 *====================*/

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* WIDGETS_COMPASS_H_ */
