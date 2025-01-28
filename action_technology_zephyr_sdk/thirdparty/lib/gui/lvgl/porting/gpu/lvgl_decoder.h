/**
 * @file lv_vg_lite_decoder.h
 *
 */

#ifndef PORTING_GPU_LVGL_IMAGE_DECODER_H
#define PORTING_GPU_LVGL_IMAGE_DECODER_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../../src/draw/lv_image_decoder.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void lvgl_decoder_init(void);

void lvgl_decoder_deinit(void);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*PORTING_GPU_LVGL_IMAGE_DECODER_H*/
