/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*********************
 *      INCLUDES
 *********************/

#include <zephyr.h>
#include "lvgl_inner.h"
#include "../gpu/lvgl_gpu.h"

#if LV_USE_THORVG
#  if LV_USE_THORVG_EXTERNAL
#    include <thorvg_capi.h>
#  else
#    include "../../src/libs/thorvg/thorvg_capi.h"
#  endif
#endif

#include <logging/log.h>
LOG_MODULE_REGISTER(lvgl, LOG_LEVEL_INF);

/**********************
 *  STATIC PROTOTYPES
 **********************/

#if LV_USE_LOG
static void lvgl_log(lv_log_level_t level, const char * buf);
#endif

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
lv_result_t lvx_port_init(void)
{
    lv_init();

#if CONFIG_SYS_CLOCK_TICKS_PER_SEC >= 1000
    lv_tick_set_cb(k_uptime_get_32);
#else
    lv_tick_set_cb(lv_port_tick_get);
#endif

#if LV_USE_LOG
    lv_log_register_print_cb(lvgl_log);
#endif

#if defined(CONFIG_LV_Z_USE_FILESYSTEM)
    lv_port_z_fs_init();
#endif

#if LV_USE_THORVG
    tvg_engine_init(TVG_ENGINE_SW, 0);
#endif

#if !defined(CONFIG_UI_SERVICE)
    if (lv_port_display_init()) {
        return LV_RESULT_INVALID;
    }

    lv_port_indev_pointer_init();
#endif

    return LV_RESULT_OK;
}

/**********************
 *  STATIC FUNCTIONS
 **********************/

#if LV_USE_LOG
static void lvgl_log(lv_log_level_t level, const char * buf)
{
    switch (level) {
    case LV_LOG_LEVEL_ERROR:
        LOG_ERR("%s", buf);
        break;
    case LV_LOG_LEVEL_WARN:
        LOG_WRN("%s", buf);
        break;
    case LV_LOG_LEVEL_INFO:
    case LV_LOG_LEVEL_USER:
        LOG_INF("%s", buf);
        break;
    case LV_LOG_LEVEL_TRACE:
        LOG_DBG("%s", buf);
        break;
    default:
        break;
    }
}
#endif /* LV_USE_LOG */
