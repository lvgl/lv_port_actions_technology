/**
 * @file lv_draw_vector_custom.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_draw_vector_custom.h"
#include "lv_draw_private.h"
#include "../stdlib/lv_mem.h"
#include "../stdlib/lv_string.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_draw_vector_custom_dsc_init(lv_draw_vector_custom_dsc_t * dsc)
{
    lv_memzero(dsc, sizeof(lv_draw_vector_custom_dsc_t));
    dsc->base.dsc_size = sizeof(lv_draw_vector_custom_dsc_t);
}

lv_draw_vector_custom_dsc_t * lv_draw_task_get_vector_custom_dsc(lv_draw_task_t * task)
{
    return task->type == LV_DRAW_TASK_TYPE_VECTOR_CUSTOM ? (lv_draw_vector_custom_dsc_t *)task->draw_dsc : NULL;
}

void lv_draw_vector_custom(lv_layer_t * layer, const lv_draw_vector_custom_dsc_t * dsc, const lv_area_t * coords)
{
    lv_draw_task_t * t = lv_draw_add_task(layer, coords ? coords : &(layer->_clip_area));
    t->type = LV_DRAW_TASK_TYPE_VECTOR_CUSTOM;
    t->draw_dsc = lv_malloc(sizeof(*dsc));
    lv_memcpy(t->draw_dsc, dsc, sizeof(*dsc));
    lv_draw_finalize_task_creation(layer, t);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
