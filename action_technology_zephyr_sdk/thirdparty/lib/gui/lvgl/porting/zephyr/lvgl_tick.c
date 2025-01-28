/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*********************
 *      INCLUDES
 *********************/

#include "lvgl_inner.h"
#include <zephyr.h>

/**********************
 *  STATIC VARIABLES
 **********************/

static uint32_t sys_cycle;
static uint32_t sys_time_ms;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

uint32_t lv_port_tick_get(void)
{
    uint32_t cycle = k_cycle_get_32();
    uint32_t delta_ms = k_cyc_to_ms_floor32(cycle - sys_cycle);

    sys_cycle += delta_ms * (sys_clock_hw_cycles_per_sec() / 1000);
    sys_time_ms += delta_ms;

    return sys_time_ms;
}
