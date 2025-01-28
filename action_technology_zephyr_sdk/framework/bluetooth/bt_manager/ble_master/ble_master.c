/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt ble master.
 */
#ifndef	CONFIG_OTA_BLE_MASTER_SUPPORT
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/byteorder.h>
#include <zephyr.h>
#include <init.h>
#include <msg_manager.h>
#include <bt_manager.h>
#include "ble_master_inner.h"

#define CONFIG_BLE_MATER_STACK_SIZE (2048)

static struct k_thread ble_master_data;
static K_KERNEL_STACK_DEFINE(ble_master_stack, CONFIG_BLE_MATER_STACK_SIZE);

static void ble_master_handler_thread(void)
{
	int err;
	struct app_msg msg = {0};
	int result = 0;

	err = msg_manager_add_listener(BLE_MASTER, os_current_get());
	if (!err) {
		SYS_LOG_ERR("failed to add listener %s", BLE_MASTER);
	}

	le_master_env_init();

	while (1) {
		if (receive_msg(&msg, OS_FOREVER)) {
			switch (msg.type) {
			case MSG_BLE_MASTER_EVENT:
				le_master_event_handle(msg.cmd, (void *)msg.value);
				break;

			default:
				SYS_LOG_ERR("error message type msg.type %d", msg.type);
				break;
			}
			if (msg.callback != NULL)
				msg.callback(&msg, result, NULL);
		}
	}
	return;
}


/*
 * Ble master initialize
 * Config master parameter in conn_master_manager.h
*/
void ble_master_init(void)
{
	k_thread_create(&ble_master_data, ble_master_stack, K_KERNEL_STACK_SIZEOF(ble_master_stack),
				(k_thread_entry_t)ble_master_handler_thread, NULL, NULL, NULL,5,0, K_NO_WAIT);
	k_thread_name_set(&ble_master_data, "BLE MASTER");
    SYS_LOG_INF("ble_master_init init");
}
#endif

