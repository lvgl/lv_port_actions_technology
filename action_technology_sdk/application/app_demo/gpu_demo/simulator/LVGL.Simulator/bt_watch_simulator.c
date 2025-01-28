/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <app_manager.h>
#include <srv_manager.h>
#include <view_manager.h>
#include <app_ui.h>
#include <simulator_config.h>

#include "bt_watch_simulator.h"

extern void launcher_app_loop(void* p1, void* p2, void* p3);

struct app_entry_t __app_entry_table[] = {
    {
        .name = "main",
        .stack = 0x10,
        .stack_size = 0,
        .priority = APP_PRIORITY,
        .attribute = RESIDENT_APP,
        .p1 = 0,
        .p2 = 0,
        .p3 = 0,
        .thread_loop = NULL,
    },
    {
        .name = "launcher",
        .stack = 0x11,
        .stack_size = 0,
        .priority = CONFIG_APP_PRIORITY,
        .attribute = DEFAULT_APP | FOREGROUND_APP,
        .p1 = 0,
        .p2 = 0,
        .p3 = 0,
        .thread_loop = launcher_app_loop,
    },
};

struct app_entry_t * __app_entry_end = NULL;

extern void _ui_service_main_loop(void* p1, void* p2, void* p3);

struct service_entry_t __service_entry_table[] = {
    {
        .name = "ui_service",
        .stack = 0x12,
        .stack_size = 0x12,
        .priority = CONFIG_UISRV_PRIORITY,
        .attribute = BACKGROUND_APP,
        .p1 = 0,
        .p2 = 0,
        .p3 = 0,
        .thread_loop = _ui_service_main_loop,
    },
};

struct service_entry_t *__service_entry_end = NULL;

extern int app_main_view_handler(uint16_t view_id, view_data_t* view_data,
           uint8_t msg_id, void* msg_data);

view_entry_t __view_entry_table[] = {
    {
        .app_id = "app_main_view",
        .proc2 = app_main_view_handler,
        .key_map = NULL,        
        .id = APP_MAIN_VIEW,
        .default_order = 0,
        .type = UI_VIEW_LVGL,
        .width = DEF_UI_VIEW_WIDTH,
        .height = DEF_UI_VIEW_HEIGHT,
    },
};

view_entry_t *__view_entry_end = NULL;

extern int mem_manager_init(void);
extern void bt_watch_main(void);

static void _bt_watch_simlulator_handler(void *p1, void *p2, void *p3)
{
    bt_watch_main();
}

void bt_watch_simlulator_main(void)
{
    __app_entry_end = &__app_entry_table[ARRAY_SIZE(__app_entry_table)];
    __service_entry_end = &__service_entry_table[ARRAY_SIZE(__service_entry_table)];
    __view_entry_end = &__view_entry_table[ARRAY_SIZE(__view_entry_table)];

    mem_manager_init();

    os_thread_create(NULL, 0, _bt_watch_simlulator_handler,
            NULL, NULL, NULL, 0, 0, 0);
}
