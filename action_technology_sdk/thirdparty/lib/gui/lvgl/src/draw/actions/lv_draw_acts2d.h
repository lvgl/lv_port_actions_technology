/**
 * @file lv_draw_acts2d.h
 *
 */

#ifndef LV_DRAW_ACTS2D_H
#define LV_DRAW_ACTS2D_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../../lv_conf_internal.h"

#if LV_USE_DRAW_ACTS2D

#include "../lv_draw.h"

/*********************
 *      DEFINES
 *********************/

#define ACTS2D_DRAW_UNIT_ID 3
#define ACTS2D_DRAW_PREFERENCE_SCORE 70 /*should be lower than VG-Lite*/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * @brief Initialize Actions 2D draw unit
 *
 * @retval the created draw unit.
 */
lv_draw_unit_t * lv_draw_acts2d_init(bool subunit);

/**
 * @brief Deinitialize Actions 2D draw unit
 *
 */
void lv_draw_acts2d_deinit(void);

/**
 * @brief Draw buffer clear using Actions 2D
 *
 * @return LV_RESULT_OK on success, LV_RESULT_INVALID on error.
 */
lv_result_t lv_draw_buf_acts2d_clear(lv_draw_buf_t * draw_buf, const lv_area_t * area);

/**
 * @brief Draw buffer copy using Actions 2D
 *
 * @return LV_RESULT_OK on success, LV_RESULT_INVALID on error.
 */
lv_result_t lv_draw_buf_acts2d_copy(lv_draw_buf_t * dest, const lv_area_t * dest_area,
                                    const lv_draw_buf_t * src, const lv_area_t * src_area);

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_DRAW_ACTS2D*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_DRAW_ACTS2D_H*/
