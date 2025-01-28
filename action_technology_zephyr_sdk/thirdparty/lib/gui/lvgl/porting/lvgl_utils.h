/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file lvgl_utils.h
 *
 */

#ifndef PORTING_LVGL_UTILS_H
#define PORTING_LVGL_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../lvgl.h"
#ifdef CONFIG_VG_LITE
  #include <vg_lite/vg_lite.h>
#endif

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * @brief Convert lvgl image cf to hal pixel format
 *
 * @param cf lvgl image cf.
 * @param bits_per_pixel optional address to store the bits_per_pixel of format.
 *
 * @retval hal pixel format
 */
uint32_t lvx_color_format_to_display(lv_color_format_t cf, uint8_t * bits_per_pixel);

/**
 * @brief Convert lvgl image cf from hal pixel format
 *
 * @param format hal pixel format
 *
 * @retval lvgl color format
 */
lv_color_format_t lvx_color_format_from_display(uint32_t format);

/**
 * @brief Convert lvgl color to ABGR-8888
 *
 * @param color lvgl color.
 *
 * @retval result color.
 */
static inline uint32_t lvx_color_to_argb32(lv_color_t color, lv_opa_t opa)
{
    return (color.red << 16) | (color.green << 8) | color.blue | (opa << 24);
}

/**
 * @brief Convert lvgl color to ABGR-8888
 *
 * @param color lvgl color.
 *
 * @retval result color.
 */
static inline uint32_t lvx_color_to_abgr32(lv_color_t color, lv_opa_t opa)
{
    return (color.blue << 16) | (color.green << 8) | color.red | (opa << 24);
}

#ifdef CONFIG_VG_LITE

/**
 * @brief Convert lvgl image cf to vglite buffer format
 *
 * @param cf lvgl image cf.
 * @param bits_per_pixel optional address to store the bits_per_pixel of format.
 *
 * @retval vglite buffer format
 */
int lvx_color_format_to_vglite(lv_color_format_t cf, uint8_t * bits_per_pixel);

/**
 * @brief Map a lvgl draw buf to vglite buffer
 *
 * @param vgbuf pointer to structure vg_lite_buffer_t.
 * @param draw_buf pointer to structure lv_draw_buf_t.
 *
 * @retval 0 on success else negative code.
 */
int lvx_vglite_map_draw_buf(vg_lite_buffer_t * vgbuf, const lv_draw_buf_t * draw_buf);

/**
 * @brief Map a lvgl image to vglite buffer
 *
 * @param vgbuf pointer to structure vg_lite_buffer_t.
 * @param img_dsc pointer to structure lv_image_dsc_t.
 *
 * @retval 0 on success else negative code.
 */
static inline int lvx_vglite_map_image(vg_lite_buffer_t * vgbuf, const lv_image_dsc_t * img_dsc)
{
    return lvx_vglite_map_draw_buf(vgbuf, (lv_draw_buf_t *)img_dsc);
}

/**
 * @brief Unmap a vglite buffer
 *
 * @param vgbuf pointer to structure vg_lite_buffer_t.
 *
 * @retval 0 on success else negative code.
 */
int lvx_vglite_unmap(vg_lite_buffer_t * vgbuf);

/**
 * @brief Set clut for lvgl indexed image
 *
 * @param img_dsc pointer to structure lv_image_dsc_t.
 *
 * @retval 0 on success else negative code.
 */
int lvx_vglite_set_image_palette(const lv_image_dsc_t * img_dsc);

#endif /* CONFIG_VG_LITE */

#ifdef __cplusplus
}
#endif

#endif /* PORTING_LVGL_UTILS_H */
