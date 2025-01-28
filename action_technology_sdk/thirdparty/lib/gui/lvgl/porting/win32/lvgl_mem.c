/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../../src/stdlib/lv_mem.h"

#if LV_USE_STDLIB_MALLOC == LV_STDLIB_CUSTOM

#if defined(CONFIG_UI_MEMORY_MANAGER)
    #include <ui_mem.h>
#endif

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

void lv_mem_init(void)
{
#if defined(CONFIG_UI_MEMORY_MANAGER)
#  if !defined(CONFIG_UI_SERVICE)
    ui_mem_init();
#  endif
#endif
}

void lv_mem_deinit(void)
{
    return; /*Nothing to deinit*/
}

lv_mem_pool_t lv_mem_add_pool(void * mem, size_t bytes)
{
    /*Not supported*/
    LV_UNUSED(mem);
    LV_UNUSED(bytes);
    return NULL;
}

void lv_mem_remove_pool(lv_mem_pool_t pool)
{
    /*Not supported*/
    LV_UNUSED(pool);
    return;
}

void * lv_malloc_core(size_t size)
{
#if defined(CONFIG_UI_MEMORY_MANAGER)
    return ui_mem_alloc(MEM_GUI, size, __func__);
#endif
}

void * lv_realloc_core(void * p, size_t new_size)
{
#if defined(CONFIG_UI_MEMORY_MANAGER)
    return ui_mem_realloc(MEM_GUI, p, new_size, __func__);
#endif
}

void lv_free_core(void * p)
{
#if defined(CONFIG_UI_MEMORY_MANAGER)
    ui_mem_free(MEM_GUI, p);
#endif
}

void lv_mem_monitor_core(lv_mem_monitor_t * mon_p)
{
    /*Not supported*/
    LV_UNUSED(mon_p);
    return;
}

lv_result_t lv_mem_test_core(void)
{
#if defined(CONFIG_UI_MEMORY_MANAGER)
    ui_mem_safe_check(0);
    return LV_RESULT_OK;
#endif

    /*Not supported*/
    return LV_RESULT_OK;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif /* LV_USE_STDLIB_MALLOC == LV_STDLIB_CUSTOM */
