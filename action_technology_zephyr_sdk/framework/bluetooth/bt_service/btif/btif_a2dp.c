/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt a2dp interface
 */

#define SYS_LOG_DOMAIN "btif_a2dp"
#include "btsrv_os_common.h"
#include "btsrv_inner.h"

int btif_a2dp_register_processer(void)
{
	return btsrv_register_msg_processer(MSG_BTSRV_A2DP, &btsrv_a2dp_process);
}

int btif_a2dp_start(struct btsrv_a2dp_start_param *param)
{
	return btsrv_function_call_malloc(MSG_BTSRV_A2DP, MSG_BTSRV_A2DP_START, (uint8_t *)param, sizeof(struct btsrv_a2dp_start_param), 0);
}

int btif_a2dp_stop(void)
{
	return btsrv_function_call(MSG_BTSRV_A2DP, MSG_BTSRV_A2DP_STOP, NULL);
}

int btif_a2dp_disable(void)
{
	return btsrv_function_call(MSG_BTSRV_A2DP, MSG_BTSRV_A2DP_DISABLE, NULL);
}

int btif_a2dp_enable(void)
{
	return btsrv_function_call(MSG_BTSRV_A2DP, MSG_BTSRV_A2DP_ENABLE, NULL);
}

int btif_a2dp_connect(bool is_src, bd_address_t *bd)
{
	return btsrv_function_call_malloc(MSG_BTSRV_A2DP, MSG_BTSRV_A2DP_CONNECT_TO, (uint8_t *)bd, sizeof(bd_address_t), (is_src ? 1 : 0));
}

int btif_a2dp_disconnect(bd_address_t *bd)
{
	return btsrv_function_call_malloc(MSG_BTSRV_A2DP, MSG_BTSRV_A2DP_DISCONNECT, (uint8_t *)bd, sizeof(bd_address_t), 0);
}

int btif_a2dp_check_state(void)
{
	return btsrv_function_call(MSG_BTSRV_A2DP, MSG_BTSRV_A2DP_CHECK_STATE, NULL);
}

/* delay_time: 1/10 milliseconds */
int btif_a2dp_send_delay_report(uint16_t delay_time)
{
	return btsrv_function_call(MSG_BTSRV_A2DP, MSG_BTSRV_A2DP_SEND_DELAY_REPORT, (void *)((uint32_t)delay_time));
}

void btif_a2dp_get_active_mac(bd_address_t *addr)
{
	int flags;

	flags = btsrv_set_negative_prio();
	btsrv_rdm_get_a2dp_acitve_mac(addr);
	btsrv_revert_prio(flags);
}
