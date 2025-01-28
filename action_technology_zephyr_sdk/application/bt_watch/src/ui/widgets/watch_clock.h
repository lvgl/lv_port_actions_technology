/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file watch_clock.h
 *
 */

#ifndef WIDGETS_WATCH_CLOCK_H_
#define WIDGETS_WATCH_CLOCK_H_

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

/* clock type */
enum {
	ANALOG_CLOCK = 0,
	DIGITAL_CLOCK,
};

typedef uint8_t clock_type_t;

/* clock time/pointers */
enum {
	CLOCK_HOUR = 0,
	CLOCK_MIN,
	CLOCK_SEC,
};

extern const lv_obj_class_t watch_clock_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create an actions rotation image objects
 * @param par pointer to an object, it will be the parent of the new image object
 * @return pointer to the created image object
 */
lv_obj_t * watch_clock_create(lv_obj_t * parent);

/*=====================
 * Setter functions
 *====================*/

/**
 * Set the type of the clock
 * @param obj pointer to an clock object
 * @param type clock type
 * @param show_sec whether to show seconds
 */
void watch_clock_set_type(lv_obj_t * obj, clock_type_t type, bool show_sec);

/**
 * Set the pointer images for the analog clock
 *
 * The pivot is offset of the image rotation center from the top-left corner,
 * and the images must all belongs to the top part of the first quadrant.
 *
 * @param obj pointer to an watch clock object
 * @param idx pointer index, allowed values CLOCK_HOUR/CLOCK_MIN/CLOCK_SEC
 * @param cnt number of image source
 * @param srcs image source array
 * @param pivot_x pivot x of the image
 * @param pivot_y pivot y of the image
 */
void watch_clock_set_pointer_images(lv_obj_t * obj, uint8_t idx, uint8_t cnt,
		const lv_image_dsc_t * srcs, lv_coord_t pivot_x, lv_coord_t pivot_y);

/**
 * Set the digit images for the digital clock
 *
 * @param obj pointer to an watch clock object
 * @param idx time index, allowed values CLOCK_HOUR/CLOCK_MIN/CLOCK_SEC
 * @param cnt number of image source
 * @param srcs image source array
 * @param area where to place the images
 */
void watch_clock_set_digit_images(lv_obj_t * obj, uint8_t idx, uint8_t cnt,
		const lv_image_dsc_t * srcs, lv_area_t *area);

/**
 * Set the separator (mostly colon) image between hour and minute for the digital clock
 *
 * The pivot is offset of the image rotation center from the left-bootom corner,
 * and the images must all belongs to the top part of the first quadrant.
 *
 * @param obj pointer to an watch clock object
 * @param src image source
 * @param pos_x position x of the image
 * @param pos_y position y of the image
 */
void watch_clock_set_separator_image(lv_obj_t * obj,
		const lv_image_dsc_t * src, lv_coord_t pos_x, lv_coord_t pos_y);

/**
 * Enable the 24-hour system for the digital clock
 *
 * @param obj pointer to an watch clock object
 * @param en enable or not
 */
void watch_clock_set_24hour(lv_obj_t * obj, bool en);

/**
 * Set the wall time of the clock
 * @param obj pointer to an watch clock object
 * @param hour hour value, range [0, 12)
 * @param minute minute value, range [0, 60)
 * @param msec millisecond value, range [0, 60000)
 */
void watch_clock_set_time(lv_obj_t * obj, uint8_t hour, uint8_t minute, uint16_t msec);

/**
 * Add info to the clock
 * @param obj pointer to an watch clock object
 * @param hour hour value, range [0, 12)
 * @param minute minute value, range [0, 60)
 * @param msec millisecond value, range [0, 60000)
 */
void watch_clock_add_info(lv_obj_t * obj, lv_obj_t * info_obj);

/*=====================
 * Getter functions
 *====================*/

/*=====================
 * Other functions
 *====================*/

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* WIDGETS_WATCH_CLOCK_H_ */
