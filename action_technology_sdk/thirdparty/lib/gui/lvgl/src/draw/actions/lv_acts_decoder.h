/**
 * @file lv_acts_decoder.h
 *
 */

#ifndef LV_ACTS_DECODER_H
#define LV_ACTS_DECODER_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../../lv_conf_internal.h"

#if LV_USE_ACTIONS

#include "../lv_image_decoder.h"
#include "../lv_image_decoder_private.h"

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
 * @brief Initialize Actions decoder
 *
 */
void lv_acts_decoder_init(void);

/**
 * @brief Deinitialize Actions decoder
 *
 */
void lv_acts_decoder_deinit(void);

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_ACTIONS*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_ACTS_DECODER_H*/
