/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../../src/stdlib/lv_mem.h"

#if LV_USE_STDLIB_MALLOC == LV_STDLIB_CUSTOM
#include <zephyr.h>

#if defined(CONFIG_UI_MEMORY_MANAGER)
    #include <ui_mem.h>
#elif defined(CONFIG_LV_Z_MEM_POOL_SYS_HEAP)
    #include <init.h>
    #include <sys/sys_heap.h>
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

#if defined(CONFIG_LV_Z_MEM_POOL_SYS_HEAP) && !defined(CONFIG_UI_MEMORY_MANAGER)
static char lvgl_heap_mem[CONFIG_LV_Z_MEM_POOL_SIZE] __aligned(8) __in_section_unique(lvgl.noinit.malloc);
static struct sys_heap lvgl_heap;
static struct k_spinlock lvgl_heap_lock;
#endif

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
#elif defined(CONFIG_LV_Z_MEM_POOL_SYS_HEAP)
    sys_heap_init(&lvgl_heap, &lvgl_heap_mem[0], CONFIG_LV_Z_MEM_POOL_SIZE);
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
#elif defined(CONFIG_LV_Z_MEM_POOL_SYS_HEAP)
    k_spinlock_key_t key;
    void *ret;

    key = k_spin_lock(&lvgl_heap_lock);
    ret = sys_heap_alloc(&lvgl_heap, size);
    k_spin_unlock(&lvgl_heap_lock, key);

    return ret;
#endif
}

void * lv_realloc_core(void * p, size_t new_size)
{
#if defined(CONFIG_UI_MEMORY_MANAGER)
    return ui_mem_realloc(MEM_GUI, p, new_size, __func__);
#elif defined(CONFIG_LV_Z_MEM_POOL_SYS_HEAP)
    k_spinlock_key_t key;
    void *ret;

    key = k_spin_lock(&lvgl_heap_lock);
    ret = sys_heap_realloc(&lvgl_heap, ptr, size);
    k_spin_unlock(&lvgl_heap_lock, key);

    return ret;
#endif
}

void lv_free_core(void * p)
{
#if defined(CONFIG_UI_MEMORY_MANAGER)
    ui_mem_free(MEM_GUI, p);
#elif defined(CONFIG_LV_Z_MEM_POOL_SYS_HEAP)
    k_spinlock_key_t key;

    key = k_spin_lock(&lvgl_heap_lock);
    sys_heap_free(&lvgl_heap, ptr);
    k_spin_unlock(&lvgl_heap_lock, key);
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
#elif defined(CONFIG_LV_Z_MEM_POOL_SYS_HEAP)
    return sys_heap_validate(&lvgl_heap) ? LV_RESULT_OK : LV_RESULT_INVALID;
#endif

    /*Not supported*/
    return LV_RESULT_OK;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif /* LV_USE_STDLIB_MALLOC == LV_STDLIB_CUSTOM */
