/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*********************
 *      INCLUDES
 *********************/

#include "../lvgl_port.h"
#include <Windows.h>

#if LV_USE_THORVG
    #if LV_USE_THORVG_EXTERNAL
        #include <thorvg_capi.h>
    #else
        #include "../../src/libs/thorvg/thorvg_capi.h"
    #endif
#endif

/**********************
 *  STATIC PROTOTYPES
 **********************/

static uint32_t _get_tick(void);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_result_t lvx_port_init(void)
{
    lv_init();
    lv_tick_set_cb(_get_tick);

#if LV_USE_THORVG
    tvg_engine_init(TVG_ENGINE_SW, 0);
#endif

    return LV_RESULT_OK;
}

/**********************
 *  STATIC FUNCTIONS
 **********************/

static uint32_t _get_tick(void)
{
    return GetTickCount();
}
