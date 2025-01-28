/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt pbap interface
 */

#define SYS_LOG_DOMAIN "btif_pbap"
#include "btsrv_os_common.h"
#include "btsrv_inner.h"

int btif_pbap_register_processer(void)
{
	return btsrv_register_msg_processer(MSG_BTSRV_PBAP, &btsrv_pbap_process);
}

int btif_pbap_cmd_op(struct bt_pbap_op_param *param)
{
	return btsrv_function_call_malloc(MSG_BTSRV_PBAP, MSG_BTSRV_PBAP_CMD_OP, (void *)param, sizeof(struct bt_pbap_op_param), 0);
}
