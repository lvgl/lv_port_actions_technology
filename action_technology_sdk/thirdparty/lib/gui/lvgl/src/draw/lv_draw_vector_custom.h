/**
 * @file lv_draw_vector_custom.h
 *
 */

#ifndef LV_DRAW_VECTOR_CUSTOM_H
#define LV_DRAW_VECTOR_CUSTOM_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lv_draw.h"
#include "../misc/lv_area.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef void (*lv_draw_vector_custom_cb_t)(lv_draw_task_t * task, lv_layer_t * layer, void * user_data);

typedef struct {
    lv_draw_dsc_base_t base;

    lv_draw_vector_custom_cb_t exec_cb;
    void * user_data;
} lv_draw_vector_custom_dsc_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Initialize a vector custom draw descriptor
 * @param dsc       pointer to a draw descriptor
 */
void lv_draw_vector_custom_dsc_init(lv_draw_vector_custom_dsc_t * dsc);

/**
 * Try to get a vector custom draw descriptor from a draw task.
 * @param task      draw task
 * @return          the task's draw descriptor or NULL if the task is not of type LV_DRAW_TASK_TYPE_VECTOR_CUSTOM
 */
lv_draw_vector_custom_dsc_t * lv_draw_task_get_vector_custom_dsc(lv_draw_task_t * task);

/**
 * Create a vector custom draw task
 * @param layer     pointer to a layer
 * @param dsc       pointer to an initialized `lv_draw_line_dsc_t` variable
 * @param coords    the coordinates of area to be drawn
 */
void lv_draw_vector_custom(lv_layer_t * layer, const lv_draw_vector_custom_dsc_t * dsc, const lv_area_t * coords);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_DRAW_VECTOR_CUSTOM_H*/
