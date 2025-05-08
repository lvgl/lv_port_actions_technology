/*
 * Copyright (c) 2022 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file gps manager interface
 */

#include <os_common_api.h>
#include <msg_manager.h>
#include <mem_manager.h>
#include <srv_manager.h>
#include <string.h>
#include <gps_manager.h>

gps_res_t gps_res;

gps_res_t *gps_manager_get_res_info(void)
{
    return &gps_res;
}

struct minmea_sentence_gga *
gps_manager_get_gga_info(void)
{
#ifdef CONFIG_GPS_PARSE_GGA_ENABLE
    return &gps_res.gga_data;
#else
    return NULL;
#endif
}

bool gps_manager_get_located(void)
{
    return gps_res.gga_data.satellites_tracked;
}

uint8_t gps_manager_get_signal(void)
{
    return gps_res.gga_data.fix_quality;
}

int gps_send_msg(uint32_t cmd, uint32_t len, void *ptr, uint8_t notify)
{
    struct app_msg msg = {0};
    os_sem return_notify;
    int ret;

    if (notify)
    {
        os_sem_init(&return_notify, 0, 1);
    }

    msg.type = MSG_SENSOR_EVENT;
    msg.cmd = cmd;
    msg.reserve = len;
    msg.ptr = ptr;

    if (notify)
    {
        msg.sync_sem = &return_notify;
    }

    ret = send_async_msg(GPS_SERVICE_NAME, &msg);
    if (false == ret)
    {
        return -EBUSY;
    }

    if (notify)
    {
        if (os_sem_take(&return_notify, OS_FOREVER))
        {
            return -ETIME;
        }
    }

    return 0;
}

static int _gps_service_start(void)
{
    struct app_msg msg = {0};

    if (!srv_manager_check_service_is_actived(GPS_SERVICE_NAME))
    {
        SYS_LOG_INF("gps service not actived\n");
        if (srv_manager_active_service(GPS_SERVICE_NAME))
        {
            SYS_LOG_INF("gps service start ok");
        }
        else
        {
            SYS_LOG_ERR("gps service start failed");
            return -ESRCH;
        }
    }
    else
    {
        SYS_LOG_ERR("gps service is actived\n");
    }

    msg.type = MSG_INIT_APP;

    int ret = send_async_msg(GPS_SERVICE_NAME, &msg);
    SYS_LOG_INF("_gps_service_start, send_async_msg MSG_INIT_APP:%d\n", ret);
    return !ret;
}

int gps_manager_init(void)
{
    SYS_LOG_INF("gps_manager_init");
    _gps_service_start();

    return 0;
}

int gps_manager_enable(uint32_t id, uint32_t func)
{
    SYS_LOG_INF("gps_manager_enable");
    /*
    SYS_LOG_INF("gps_manager_enable MSG_INIT_APP");
    struct app_msg msg = {0};
    msg.type = MSG_INIT_APP;
    send_async_msg(GPS_SERVICE_NAME, &msg);
*/
    return gps_send_msg(MSG_GPS_ENABLE, id, (void *)func, 0);
}

int gps_manager_disable(uint32_t id, uint32_t func)
{
    SYS_LOG_INF("gps_manager_disable");
    return gps_send_msg(MSG_GPS_DISABLE, id, (void *)func, 0);
}

int gps_manager_add_callback(gps_res_cb_t cb)
{
    SYS_LOG_INF("gps_manager_add_callback");
    return gps_send_msg(MSG_GPS_ADD_CB, 0, cb, 0);
}

int gps_manager_remove_callback(gps_res_cb_t cb)
{
    SYS_LOG_INF("gps_manager_remove_callback");
    return gps_send_msg(MSG_GPS_REMOVE_CB, 0, cb, 0);
}
