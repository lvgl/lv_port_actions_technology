/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include "nsm_app_inner.h"
#include "tts_manager.h"
#include <spp_test_backend.h>
#include <soc_pm.h>
#include <msg_manager.h>
#ifdef CONFIG_SYS_WAKELOCK
#include <sys_wakelock.h>
#endif

act_nsm_data_t g_nsm_data;
#define CONFIG_NSM_STACK_SIZE (1536)

void nsm_init(void)
{
    // if (g_tool_data.stack)
        // return -EALREADY;

    SYS_LOG_INF("nsm_init");

    if (NULL == g_nsm_data.stack)
    {
        g_nsm_data.stack = app_mem_malloc(CONFIG_NSM_STACK_SIZE);
    }
    else
    {
        //if tools thread not exit, cannot init again
        if(!g_nsm_data.quited){         
            return;           
        }
    }

    if (!g_nsm_data.stack)
    {
        SYS_LOG_ERR("nsm stack mem_malloc failed");
        return;
    }

    g_nsm_data.quit = 0;
    g_nsm_data.quited = 0;

    // os_sem_init(&g_tool_data.init_sem, 0, 1);
    os_thread_create(g_nsm_data.stack, CONFIG_NSM_STACK_SIZE,
            nsm_spp_test_main, NULL, NULL, NULL, 8, 0, 0);

    // os_sem_take(&g_tool_data.init_sem, OS_FOREVER);
    
#ifdef CONFIG_SYS_WAKELOCK
    sys_wake_lock(PARTIAL_WAKE_LOCK);
#endif

    SYS_LOG_INF("begin trying to connect pc nsm");
}

void nsm_deinit(void)
{
    if (!g_nsm_data.stack)
        return;

    SYS_LOG_WRN("will disconnect pc nsm, then exit");

    g_nsm_data.quit = 1;
    while (!g_nsm_data.quited)
        os_sleep(1);

    app_mem_free(g_nsm_data.stack);
    g_nsm_data.stack = NULL;
#ifdef CONFIG_SYS_WAKELOCK
    sys_wake_unlock(PARTIAL_WAKE_LOCK);
#endif
    SYS_LOG_INF("exit now");
}
