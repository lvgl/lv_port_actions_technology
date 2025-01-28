/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <debug/actions_exception.h>

#define MAX_EXCEPTION_CALLBACKS   2

typedef struct
{
    int index;
    actions_exception_callback_routine_t callback[MAX_EXCEPTION_CALLBACKS];
} exception_ctx_t;


static exception_ctx_t g_exception_ctx;

int exception_register_callbacks(actions_exception_callback_routine_t *cb)
{
    int i;

    if (g_exception_ctx.index >= MAX_EXCEPTION_CALLBACKS) {
       return -ENOMEM;
    } else {
       for (i = 0; i < g_exception_ctx.index; i++) {
           if ( g_exception_ctx.callback[i].init_cb == cb->init_cb
             && g_exception_ctx.callback[i].run_cb == cb->run_cb) {
                return 0;
           }
       }
       g_exception_ctx.callback[g_exception_ctx.index].init_cb = cb->init_cb;
       g_exception_ctx.callback[g_exception_ctx.index].run_cb = cb->run_cb;
       g_exception_ctx.index++;
       return true;
    }
}

void exception_init(void)
{
    int i;

    for (i = 0; i < g_exception_ctx.index; i++) {
        if (g_exception_ctx.callback[i].init_cb) {
            g_exception_ctx.callback[i].init_cb();
        }
    }
}

void exception_run(void)
{
    int i;

    for (i = 0; i < g_exception_ctx.index; i++) {
        if (g_exception_ctx.callback[i].run_cb) {
            g_exception_ctx.callback[i].run_cb();
        }
    }    
}