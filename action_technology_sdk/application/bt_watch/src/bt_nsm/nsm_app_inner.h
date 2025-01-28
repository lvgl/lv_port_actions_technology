/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __NSM_APP_INNER_H__
#define __NSM_APP_INNER_H__

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <zephyr.h>
#include <stub_command.h>
#include <stub.h>
#include <usp_protocol.h>
#include <app_defines.h>
#include <app_manager.h>
#include <app_switch.h>
#include <os_common_api.h>
#include <config.h>
#include "config_al.h"
#include <drivers/hrtimer.h>
#include <sys/ring_buffer.h>
#include <thread_timer.h>

typedef struct {
    char *stack;
    uint16_t quit : 1;
    uint16_t quited : 1;

    uint8_t connect;

} act_nsm_data_t;

extern act_nsm_data_t g_nsm_data;

static inline uint16_t nsm_is_quitting(void)
{
    return g_nsm_data.quit;
}

u16_t nsm_cmd_xml_parse(void);

void nsm_spp_test_main(void *p1, void *p2, void *p3);


#endif /* __NSM_APP_INNER_H__ */
