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

int gps_send_msg(uint32_t cmd, uint32_t len, void *ptr, uint8_t notify)
{
	struct app_msg msg = {0};
	os_sem return_notify;
	int ret;

	if (notify) {
		os_sem_init(&return_notify, 0, 1);
	}
	
	msg.type = MSG_SENSOR_EVENT;
	msg.cmd = cmd;
	msg.reserve = len;
	msg.ptr = ptr;
	
	if (notify) {
		msg.sync_sem = &return_notify;
	}
	
	ret = send_async_msg(GPS_SERVICE_NAME, &msg);
	if (false == ret) {
		return -EBUSY;
	}

	if (notify) {
		if (os_sem_take(&return_notify, OS_FOREVER)) {
			return -ETIME;
		}
	}

	return 0;
}


static int _gps_service_start(void)
{
	struct app_msg msg = {0};

	if (!srv_manager_check_service_is_actived(GPS_SERVICE_NAME)) {
		if (srv_manager_active_service(GPS_SERVICE_NAME)) {
			SYS_LOG_DBG("gps service start ok");
		} else {
			SYS_LOG_ERR("gps service start failed");
			return -ESRCH;
		}
	}

	msg.type = MSG_INIT_APP;

	return !send_async_msg(GPS_SERVICE_NAME, &msg);
}

int gps_manager_init(void)
{
	_gps_service_start();

	return 0;
}

int gps_manager_enable(uint32_t id, uint32_t func)
{
	return gps_send_msg(MSG_GPS_ENABLE, id, (void*)func, 0);
}

int gps_manager_disable(uint32_t id, uint32_t func)
{
	return gps_send_msg(MSG_GPS_DISABLE, id, (void*)func, 0);
}

int gps_manager_add_callback(gps_res_cb_t cb)
{
	return gps_send_msg(MSG_GPS_ADD_CB, 0, cb, 0);
}

int gps_manager_remove_callback(gps_res_cb_t cb)
{
	return gps_send_msg(MSG_GPS_REMOVE_CB, 0, cb, 0);
}
