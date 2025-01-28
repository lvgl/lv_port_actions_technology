/*
 * Copyright (c) 2019 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file 
 * @brief 
 *
 * Change log:
 *
 */

#ifndef __NV_BLE_STREAM_CTRL_
#define __NV_BLE_STREAM_CTRL_

#include <thread_timer.h>
#include <user_comm/sys_comm.h>

#ifndef BOOL
#define BOOL int
#endif

#ifndef SYS_LOG_DOMAIN
#define SYS_LOG_DOMAIN "nv_ble"
#endif

#ifndef SYS_LOG_LEVEL
#define SYS_LOG_LEVEL 3	/* SYS_LOG_LEVEL_INFO */
#endif
#define NV_RXTX_INTERVAL		(5)

struct nv_info_t {
	loop_buffer_t  tx_loop_buf;
	loop_buffer_t  record_send_buf;

	struct thread_timer auth_timer;
};

void nv_ble_stream_init(void);
void nv_ble_active_disconnect(os_work *work);
void nv_auth_timeout_start(u32_t duration);
void nv_auth_timeout_stop(void);
int nv_send_pkg_to_stream(u8_t *data_ptr, u16_t length, int num);
int nv_uuid_tx_chrc_attr_get(u8_t num);
int nv_uuid_tx_attr_get(u8_t num);
void nv_record_resource_release(void);
void nv_auth_timeout_start(u32_t duration);
void nv_auth_timeout_stop(void);

#endif /*__NV_BLE_STREAM_CTRL_*/






