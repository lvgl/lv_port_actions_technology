/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PORTING_GPU_LVGL_GPU_H
#define PORTING_GPU_LVGL_GPU_H

/*********************
 *      INCLUDES
 *********************/

#include "../../lvgl.h"
#include "../../src/lvgl_private.h"
#include <display/display_hal.h>

/*********************
 *      DEFINES
 *********************/

#ifndef LV_USE_DRAW_ACTS_DMA2D
  #define LV_USE_DRAW_ACTS_DMA2D 0
#endif

#define DMA2D_DRAW_UNIT_ID 3
#define DMA2D_DRAW_PREFERENCE_SCORE 85
#define DMA2D_DRAW_LABEL_PREFERENCE_SCORE 75 /* VGLite's score is 80 */

#define SWRENDER_DRAW_UNIT_ID 4
#define SWRENDER_DRAW_PREFERENCE_SCORE 95

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
void lv_draw_custom_gpu_init(void);

/**
 * @brief Initialize draw buffer handlers for GPU
 *
 * @retval N/A.
 */
void lv_gpu_draw_buf_init_handlers(void);

/**
 * @brief Draw buffer width to stride handler for GPU
 *
 * @retval N/A.
 */
uint32_t lv_gpu_draw_buf_width_to_stride(uint32_t w, lv_color_format_t color_format);

/**
 * @brief Initialize Actions' DMA2D draw unit
 *
 * @retval N/A.
 */
void lv_gpu_draw_dma2d_init(void);

/**
 * @brief Draw buffer clear using DMA2D
 *
 * @return LV_RESULT_OK on success, LV_RESULT_INVALID on error.
 */
lv_result_t lv_gpu_draw_buf_dma2d_clear(lv_draw_buf_t * draw_buf, const lv_area_t * area);

/**
 * @brief Draw buffer copy using DMA2D
 *
 * @return LV_RESULT_OK on success, LV_RESULT_INVALID on error.
 */
lv_result_t lv_gpu_draw_buf_dma2d_copy(lv_draw_buf_t * dest, const lv_area_t * dest_area,
                                       const lv_draw_buf_t * src, const lv_area_t * src_area);

/**
 * @brief Initialize Actions' SW renderer draw unit
 *
 * @retval N/A.
 */
void lv_gpu_draw_swrender_init(void);

/**
 * @brief Draw buffer clear using SWRenderer
 *
 * @retval N/A.
 */
void lv_gpu_draw_buf_sw_clear(lv_draw_buf_t * draw_buf, const lv_area_t * area);

/**
 * @brief Draw buffer copy using SWRenderer
 *
 * @retval N/A.
 */
void lv_gpu_draw_buf_sw_copy(lv_draw_buf_t * dest, const lv_area_t * dest_area,
                             const lv_draw_buf_t * src, const lv_area_t * src_area);

#endif /* PORTING_GPU_LVGL_GPU_H */
