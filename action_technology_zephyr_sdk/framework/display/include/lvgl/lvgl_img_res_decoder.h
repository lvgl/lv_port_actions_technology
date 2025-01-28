/**
 * @file lv_img_decoder_acts.h
 *
 */

/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LVGL_IMG_DECODER_ACTS_RES_H
#define LVGL_IMG_DECODER_ACTS_RES_H

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

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/**
 * Initialize the image res ecoder on Actions platform
 */
void lvgl_img_decoder_acts_res_init(void);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
}
#endif

#endif /* LVGL_IMG_DECODER_ACTS_RES_H */
