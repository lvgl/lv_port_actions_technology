/**
 * @file lv_draw_actions.h
 *
 */

#ifndef LV_DRAW_ACTIONS_H
#define LV_DRAW_ACTIONS_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../../lv_conf_internal.h"

#if LV_USE_ACTIONS

#include "../lv_draw.h"

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
 * @brief Initialize Actions' draw engine
 *
 * @retval N/A.
 */
void lv_draw_actions_init(void);

/**
 * @brief Deinitialize Actions' draw engine
 *
 * @retval N/A.
 */
void lv_draw_actions_deinit(void);

/**
 * @brief Draw buffer width to stride handler for GPU
 *
 * @retval N/A.
 */
uint32_t lv_acts_draw_buf_width_to_stride(uint32_t w, lv_color_format_t color_format);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_USE_ACTIONS*/

#endif /* LV_DRAW_ACTIONS_H */
