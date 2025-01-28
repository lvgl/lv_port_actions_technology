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

extern void _launcher_app_loop(void* p1, void* p2, void* p3);

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
        .thread_loop = _launcher_app_loop,
    },
};

struct app_entry_t * __app_entry_end = NULL;

extern void _ui_service_main_loop(void* parama1, void* parama2, void* parama3);

//extern void media_service_main_loop(void* parama1, void* parama2, void* parama3);

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

extern int _clock_view_handler(uint16_t view_id, view_data_t* view_data, uint8_t msg_id, void* msg_data);
extern int _main_view_handler(uint16_t view_id, view_data_t* view_data, uint8_t msg_id, void* msg_data);
extern int _sport_view_handler(uint16_t view_id, view_data_t* view_data, uint8_t msg_id, void* msg_data);
extern int _heart_view_handler(uint16_t view_id, view_data_t* view_data, uint8_t msg_id, void* msg_data);
extern int _message_view_handler(uint16_t view_id, view_data_t* view_data, uint8_t msg_id, void* msg_data);
extern int applist_view_handler(uint16_t view_id, view_data_t* view_data, uint8_t msg_id, void* msg_data);
extern int clock_selector_view_handler(uint16_t view_id, view_data_t* view_data, uint8_t msg_id, void* msg_data);
extern int _music_view_handler(uint16_t view_id, view_data_t* view_data, uint8_t msg_id, void* msg_data);
extern int _bp_view_handler(uint16_t view_id, view_data_t* view_data, uint8_t msg_id, void *msg_data);
extern int _spo2_view_handler(uint16_t view_id, view_data_t* view_data, uint8_t msg_id, void *msg_data);
extern int _alipay_main_view_handler(uint16_t view_id, view_data_t* view_data, uint8_t msg_id, void *msg_data);
extern int _alipay_bind_view_handler(uint16_t view_id, view_data_t* view_data, uint8_t msg_id, void* msg_data);
extern int _alipay_pay_view_handler(uint16_t view_id, view_data_t* view_data, uint8_t msg_id, void* msg_data);
extern int _alipay_unbind_view_handler(uint16_t view_id, view_data_t* view_data, uint8_t msg_id, void* msg_data);
extern int _wxpay_main_view_handler(uint16_t view_id, view_data_t* view_data, uint8_t msg_id, void* msg_data);
extern int _wxpay_bind_view_handler(uint16_t view_id, view_data_t* view_data, uint8_t msg_id, void* msg_data);
extern int _wxpay_pay_view_handler(uint16_t view_id, view_data_t* view_data, uint8_t msg_id, void* msg_data);
extern int _wxpay_unbind_view_handler(uint16_t view_id, view_data_t* view_data, uint8_t msg_id, void* msg_data);
extern int _gps_view_handler(uint16_t view_id, view_data_t* view_data, uint8_t msg_id, void * msg_data);
extern int _setting_ui_handler(uint16_t view_id, view_data_t* view_data, uint8_t msg_id, void* msg_data);
extern int _applist_anim_set_ui_handler(uint16_t view_id, view_data_t* view_data, uint8_t msg_id, void* msg_data);
extern int _scroll_scroll_anim_set_handler(uint16_t view_id, view_data_t* view_data, uint8_t msg_id, void* msg_data);
extern int _switch_switch_anim_set_handler(uint16_t view_id, view_data_t* view_data, uint8_t msg_id, void* msg_data);
extern int _alipay_menu_view_handler(uint16_t view_id, view_data_t* view_data, uint8_t msg_id, void* msg_data);
extern int _alipay_cardlist_view_handler(uint16_t view_id, view_data_t* view_data, uint8_t msg_id, void *msg_data);
extern int _alipay_transitcode_view_handler(uint16_t view_id, view_data_t* view_data, uint8_t msg_id, void *msg_data);

view_entry_t __view_entry_table[] = {
    {
        .app_id = "main",
        .proc2 = _main_view_handler,    
        .id = MAIN_VIEW,
        .default_order = HIGH_ORDER,
        .type = UI_VIEW_LVGL,
        .width = DEF_UI_VIEW_WIDTH,
        .height = DEF_UI_VIEW_HEIGHT,
    },
    {
        .app_id = "clock_view",
        .proc2 = _clock_view_handler,
        .id = CLOCK_VIEW,
        .default_order = NORMAL_ORDER,
        .type = UI_VIEW_LVGL,
        .width = DEF_UI_VIEW_WIDTH,
        .height = DEF_UI_VIEW_HEIGHT,
    },
    {
        .app_id = "sport_view",
        .proc2 = _sport_view_handler,
        .id = SPORT_VIEW,
        .default_order = NORMAL_ORDER,
        .type = UI_VIEW_LVGL,
        .width = DEF_UI_VIEW_WIDTH,
        .height = DEF_UI_VIEW_HEIGHT,
    },
    {
        .app_id = "heart_view",
        .proc2 = _heart_view_handler,
        .id = HEART_VIEW,
        .default_order = NORMAL_ORDER,
        .type = UI_VIEW_LVGL,
        .width = DEF_UI_VIEW_WIDTH,
        .height = DEF_UI_VIEW_HEIGHT,
    },
    {
        .app_id = "message",
        .proc2 = _message_view_handler,
        .id = MSG_VIEW,
        .default_order = HIGH_ORDER,
        .type = UI_VIEW_LVGL,
        .width = DEF_UI_VIEW_WIDTH,
        .height = DEF_UI_VIEW_HEIGHT,
    },
    {
        .app_id = "longview",
        .proc2 = NULL,
        .id = TEST_LONG_VIEW,
        .default_order = NORMAL_ORDER,
        .type = UI_VIEW_LVGL,
        .width = DEF_UI_VIEW_WIDTH,
        .height = DEF_UI_VIEW_HEIGHT * 2,
    },
    {
        .app_id = "applist",
        .proc2 = applist_view_handler,
        .id = APPLIST_VIEW,
        .default_order = NORMAL_ORDER,
        .type = UI_VIEW_LVGL,
        .width = DEF_UI_VIEW_WIDTH,
        .height = DEF_UI_VIEW_HEIGHT,
    },
    {
        .app_id = "music_view",
        .proc2 = _music_view_handler,
        .id = MUSIC_VIEW,
        .default_order = NORMAL_ORDER,
        .type = UI_VIEW_LVGL,
        .width = DEF_UI_VIEW_WIDTH,
        .height = DEF_UI_VIEW_HEIGHT,
    },
    {
        .app_id = "clocksel_view",
        .proc2 = clock_selector_view_handler,
        .id = CLOCK_SELECTOR_VIEW,
        .default_order = NORMAL_ORDER,
        .type = UI_VIEW_LVGL,
        .width = DEF_UI_VIEW_WIDTH,
        .height = DEF_UI_VIEW_HEIGHT,
    },
    {
        .app_id = "health_bp",
        .proc2 = _bp_view_handler,
        .id = HEALTH_BP_VIEW,
        .default_order = NORMAL_ORDER,
        .type = UI_VIEW_LVGL,
        .width = DEF_UI_VIEW_WIDTH,
        .height = DEF_UI_VIEW_HEIGHT,
    },
    {
        .app_id = "clocksel_subview_4",
        .proc2 = _spo2_view_handler,
        .id = HEALTH_SPO2_VIEW,
        .default_order = NORMAL_ORDER,
        .type = UI_VIEW_LVGL,
        .width = DEF_UI_VIEW_WIDTH,
        .height = DEF_UI_VIEW_HEIGHT,
    },
    {
        .app_id = "alipay_main_view",
        .proc2 = _alipay_main_view_handler,
        .id = ALIPAY_MAIN_VIEW,
        .default_order = NORMAL_ORDER,
        .type = UI_VIEW_LVGL,
        .width = DEF_UI_VIEW_WIDTH,
        .height = DEF_UI_VIEW_HEIGHT,
    },
    {
        .app_id = "alipay_bind_view",
        .proc2 = _alipay_bind_view_handler,
        .id = ALIPAY_BIND_VIEW,
        .default_order = NORMAL_ORDER,
        .type = UI_VIEW_LVGL,
        .width = DEF_UI_VIEW_WIDTH,
        .height = DEF_UI_VIEW_HEIGHT,
    },
    {
        .app_id = "alipay_pay_view",
        .proc2 = _alipay_pay_view_handler,
        .id = ALIPAY_PAY_VIEW,
        .default_order = NORMAL_ORDER,
        .type = UI_VIEW_LVGL,
        .width = DEF_UI_VIEW_WIDTH,
        .height = DEF_UI_VIEW_HEIGHT,
    },
    {
        .app_id = "alipay_unbind_view",
        .proc2 = _alipay_unbind_view_handler,
        .id = ALIPAY_UNBIND_VIEW,
        .default_order = NORMAL_ORDER,
        .type = UI_VIEW_LVGL,
        .width = DEF_UI_VIEW_WIDTH,
        .height = DEF_UI_VIEW_HEIGHT,
    },
    {
        .app_id = "wxpay_main_view",
        .proc2 = _wxpay_main_view_handler,
        .id = WXPAY_MAIN_VIEW,
        .default_order = NORMAL_ORDER,
        .type = UI_VIEW_LVGL,
        .width = DEF_UI_VIEW_WIDTH,
        .height = DEF_UI_VIEW_HEIGHT,
    },
    {
        .app_id = "wxpay_bind_view",
        .proc2 = _wxpay_bind_view_handler,
        .id = WXPAY_BIND_VIEW,
        .default_order = NORMAL_ORDER,
        .type = UI_VIEW_LVGL,
        .width = DEF_UI_VIEW_WIDTH,
        .height = DEF_UI_VIEW_HEIGHT,
    },
    {
        .app_id = "wxpay_pay_view",
        .proc2 = _wxpay_pay_view_handler,
        .id = WXPAY_PAY_VIEW,
        .default_order = NORMAL_ORDER,
        .type = UI_VIEW_LVGL,
        .width = DEF_UI_VIEW_WIDTH,
        .height = DEF_UI_VIEW_HEIGHT,
    },
    {
        .app_id = "wxpay_unbind_view",
        .proc2 = _wxpay_unbind_view_handler,
        .id = WXPAY_UNBIND_VIEW,
        .default_order = NORMAL_ORDER,
        .type = UI_VIEW_LVGL,
        .width = DEF_UI_VIEW_WIDTH,
        .height = DEF_UI_VIEW_HEIGHT,
    },
    {
        .app_id = "gps_view",
        .proc2 = _gps_view_handler,
        .id = GPS_VIEW,
        .default_order = NORMAL_ORDER,
        .type = UI_VIEW_LVGL,
        .width = DEF_UI_VIEW_WIDTH,
        .height = DEF_UI_VIEW_HEIGHT,
    },
    {
        .app_id = "setting_view",
        .proc2 = _setting_ui_handler,
        .id = SETTING_VIEW,
        .default_order = NORMAL_ORDER,
        .type = UI_VIEW_LVGL,
        .width = DEF_UI_VIEW_WIDTH,
        .height = DEF_UI_VIEW_HEIGHT,
    },
    {
        .app_id = "applist_anim_set_view",
        .proc2 = _applist_anim_set_ui_handler,
        .id = APPLIST_ANIM_SET_VIEW,
        .default_order = NORMAL_ORDER,
        .type = UI_VIEW_LVGL,
        .width = DEF_UI_VIEW_WIDTH,
        .height = DEF_UI_VIEW_HEIGHT,
    },
    {
        .app_id = "scroll_anim_set_view",
        .proc2 = _scroll_scroll_anim_set_handler,
        .id = SCROLL_ANIM_SET_VIEW,
        .default_order = NORMAL_ORDER,
        .type = UI_VIEW_LVGL,
        .width = DEF_UI_VIEW_WIDTH,
        .height = DEF_UI_VIEW_HEIGHT,
    },
    {
        .app_id = "switch_anim_set_view",
        .proc2 = _switch_switch_anim_set_handler,
        .id = SWITCH_ANIM_SET_VIEW,
        .default_order = NORMAL_ORDER,
        .type = UI_VIEW_LVGL,
        .width = DEF_UI_VIEW_WIDTH,
        .height = DEF_UI_VIEW_HEIGHT,
    },
    {
        .app_id = "alipay_menu_view",
        .proc2 = _alipay_menu_view_handler,
        .id = ALIPAY_MENU_VIEW,
        .default_order = NORMAL_ORDER,
        .type = UI_VIEW_LVGL,
        .width = DEF_UI_VIEW_WIDTH,
        .height = DEF_UI_VIEW_HEIGHT,
    },
	{
        .app_id = "alipay_cardlist_view",
        .proc2 = _alipay_cardlist_view_handler,
        .id = ALIPAY_CARDLIST_VIEW,
        .default_order = NORMAL_ORDER,
        .type = UI_VIEW_LVGL,
        .width = DEF_UI_VIEW_WIDTH,
        .height = DEF_UI_VIEW_HEIGHT,
    },
	{
        .app_id = "alipay_transitcode_view",
        .proc2 = _alipay_transitcode_view_handler,
        .id = ALIPAY_TRANSITCODE_VIEW,
        .default_order = NORMAL_ORDER,
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

	os_thread_create(NULL, 0, _bt_watch_simlulator_handler, NULL, NULL,
			 NULL, 0, 0, 0);
}
