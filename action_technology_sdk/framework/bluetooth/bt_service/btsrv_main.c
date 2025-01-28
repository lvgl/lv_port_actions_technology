/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt service interface
 */

#define SYS_LOG_DOMAIN "btsrv_main"
#include "btsrv_os_common.h"
#include "btsrv_inner.h"

#define MAX_BTSRV_PROCESSER		(MSG_BTSRV_MAX - MSG_BTSRV_BASE)
#define BTSRV_PORC_MONITOR_TIME	(10)		/* 10ms */

static uint8_t btstack_ready_flag;
static btsrv_msg_process msg_processer[MAX_BTSRV_PROCESSER];

static int _bt_service_init(struct app_msg *msg)
{
	btsrv_callback cb = _btsrv_get_msg_param_callback(msg);

	btstack_ready_flag = 0;
	if (!btsrv_adapter_init(cb)) {
		SYS_LOG_ERR("btstack_init failed\n");
		return -EAGAIN;
	}

	btsrv_adapter_run();

	if (cb) {
		/* wait for ready */
		while (!btstack_ready_flag)
			os_sleep(10);
	}

	return 0;
}

static int _bt_service_exit(void)
{
	btsrv_adapter_stop();
	srv_manager_thread_exit(BLUETOOTH_SERVICE_NAME);

	return 0;
}

void bt_service_set_bt_ready(void)
{
	btstack_ready_flag = 1;
}

uint8_t bt_service_ready_status(void)
{
	return btstack_ready_flag;
}

int btsrv_register_msg_processer(uint8_t msg_type, btsrv_msg_process processer)
{
	if ((msg_type < MSG_BTSRV_BASE) || (msg_type >= MSG_BTSRV_MAX) || !processer) {
		SYS_LOG_WRN("Unknow processer %p or msg_type %d\n", processer, msg_type);
		return -EINVAL;
	}

	msg_processer[msg_type - MSG_BTSRV_BASE] = processer;
	SYS_LOG_INF("Register %d processer\n", msg_type);
	return 0;
}

#if CONFIG_BT_BR_ACTS
static void btsrv_print_cmd(uint8_t cmd)
{
	char *str = NULL;
	uint8_t start_cmd;

	if (cmd >= MSG_BTSRV_MAP_CONNECT) {
		start_cmd = MSG_BTSRV_MAP_CONNECT;
		str = STRINGIFY(MSG_BTSRV_MAP_CONNECT);
	} else if (cmd >= MSG_BTSRV_TWS_INIT) {
		start_cmd = MSG_BTSRV_TWS_INIT;
		str = STRINGIFY(MSG_BTSRV_TWS_INIT);
	} else if (cmd >= MSG_BTSRV_HID_START) {
		start_cmd = MSG_BTSRV_HID_START;
		str = STRINGIFY(MSG_BTSRV_HID_START);
	} else if (cmd >= MSG_BTSRV_PBAP_CONNECT_FAILED) {
		start_cmd = MSG_BTSRV_PBAP_CONNECT_FAILED;
		str = STRINGIFY(MSG_BTSRV_PBAP_CONNECT_FAILED);
	} else if (cmd >= MSG_BTSRV_SPP_START) {
		start_cmd = MSG_BTSRV_SPP_START;
		str = STRINGIFY(MSG_BTSRV_SPP_START);
	} else if (cmd >= MSG_BTSRV_HFP_AG_START) {
		start_cmd = MSG_BTSRV_HFP_AG_START;
		str = STRINGIFY(MSG_BTSRV_HFP_AG_START);
	} else if (cmd >= MSG_BTSRV_HFP_SWITCH_SOUND_SOURCE) {
		start_cmd = MSG_BTSRV_HFP_SWITCH_SOUND_SOURCE;
		str = STRINGIFY(MSG_BTSRV_HFP_SWITCH_SOUND_SOURCE);
	} else if (cmd >= MSG_BTSRV_HFP_START) {
		start_cmd = MSG_BTSRV_HFP_START;
		str = STRINGIFY(MSG_BTSRV_HFP_START);
	} else if (cmd >= MSG_BTSRV_AVRCP_START) {
		start_cmd = MSG_BTSRV_AVRCP_START;
		str = STRINGIFY(MSG_BTSRV_AVRCP_START);
	} else if (cmd >= MSG_BTSRV_A2DP_START) {
		start_cmd = MSG_BTSRV_A2DP_START;
		str = STRINGIFY(MSG_BTSRV_A2DP_START);
	} else {
		start_cmd = MSG_BTSRV_SET_DEFAULT_SCAN_PARAM;
		str = STRINGIFY(MSG_BTSRV_SET_DEFAULT_SCAN_PARAM);
	}

	SYS_LOG_INF("btsrv cmd %d = %s + %d", cmd, str, (cmd - start_cmd));
}
#endif

void bt_service_main_loop(void *parama1, void *parama2, void *parama3)
{
	struct app_msg msg = {0};
	bool terminaltion = false;
	int result = 0;
	uint32_t start_time, end_time;

	while (!terminaltion) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			switch (msg.type) {
			case MSG_EXIT_APP:
				_bt_service_exit();
				terminaltion = true;
				break;
			case MSG_INIT_APP:
				_bt_service_init(&msg);
				break;
			default:
				if (msg.type >= MSG_BTSRV_BASE && msg.type < MSG_BTSRV_MAX &&
					msg_processer[msg.type - MSG_BTSRV_BASE]) {
					#if CONFIG_BT_BR_ACTS
					if (!bt_service_ready_status()) {
						SYS_LOG_ERR("bt service not ready!\n");
						break;
					}
					#ifndef	CONFIG_BT_BREDR_DISABLE
					btsrv_adapter_srv_get_wake_lock();
					#endif
					#endif
					start_time = os_uptime_get_32();
					msg_processer[msg.type - MSG_BTSRV_BASE](&msg);
					end_time = os_uptime_get_32();
					if ((end_time - start_time) > BTSRV_PORC_MONITOR_TIME) {
						printk("xxxx:(%s) Btsrv type %d cmd %d proc used %d ms\n", __func__, msg.type, msg.cmd, (end_time - start_time));
						#if CONFIG_BT_BR_ACTS
						btsrv_print_cmd(msg.cmd);
						#endif
					}
				}
				break;
			}

			if (msg.callback) {
				msg.callback(&msg, result, NULL);
			}
		}
		thread_timer_handle_expired();
	}
}
