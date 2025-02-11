/**
 * @file lv_draw_actsw.h
 *
 */

#ifndef LV_DRAW_ACTSW_H
#define LV_DRAW_ACTSW_H

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

#define ACTSW_DRAW_UNIT_ID 4
#define ACTSW_DRAW_PREFERENCE_SCORE 90

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * @brief Initialize Actions SW draw unit
 *
 * @retval the created draw unit.
 */
lv_draw_unit_t * lv_draw_actsw_init(bool subunit);

/**
 * @brief Deinitialize Actions SW draw unit
 *
 */
void lv_draw_actsw_deinit(void);

/**
 * @brief Blit using Actions SW renderer
 *
 * @retval N/A.
 */
void lv_draw_actsw_blit(lv_draw_unit_t * draw_unit, const lv_draw_buf_t * src_buf,
                        const lv_area_t * src_a, lv_color_t recolor, lv_opa_t opa);

/**
 * @brief Draw buffer clear using Actions SW renderer
 *
 * @retval N/A.
 */
void lv_draw_buf_actsw_clear(lv_draw_buf_t * draw_buf, const lv_area_t * area);

/**
 * @brief Draw buffer copy using Actions SW renderer
 *
 * @retval N/A.
 */
void lv_draw_buf_actsw_copy(lv_draw_buf_t * dest, const lv_area_t * dest_area,
                            const lv_draw_buf_t * src, const lv_area_t * src_area);

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_ACTIONS*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_DRAW_ACTSW_H*/
