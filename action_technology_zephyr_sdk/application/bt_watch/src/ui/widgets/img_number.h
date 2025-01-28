/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file img_number.h
 *
 */

#ifndef WIDGETS_IMG_NUMBER_H_
#define WIDGETS_IMG_NUMBER_H_

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "img_label.h"
#include "draw_util.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

extern const lv_obj_class_t img_number_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create an img number object
 * @param parent pointer to an object, it will be the parent of the new object
 * @return pointer to the created img number object
 */
lv_obj_t * img_number_create(lv_obj_t * parent);

/*=====================
 * Setter functions
 *====================*/

/**
 * Set align of the img number
 *
 * @param obj pointer to an img number object
 * @param align `LV_ALIGN_...`, align the digit number image inside the object
 */
void img_number_set_align(lv_obj_t *obj, lv_align_t align);

/**
 * Set the pixel map array
 *
 * @param obj pointer to an img number object
 * @param src array of 10 or 11 images, the first 10 of which stand for number 0-9;
 *            while the 11 image for the unit if exists.
 * @param cnt the image count
 */
void img_number_set_src(lv_obj_t *obj, const lv_image_dsc_t * srcs, uint8_t cnt);

/**
 * Set value of the img number
 *
 * @param obj pointer to an img number object
 * @param value the number to show
 * @param min_w minimum width, fill 0 if number too small
 */
void img_number_set_value(lv_obj_t *obj, uint32_t value, uint8_t min_w);

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

#endif /* WIDGETS_IMG_NUMBER_H_ */
