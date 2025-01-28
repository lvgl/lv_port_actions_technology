/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt spp interface
 */

#define SYS_LOG_DOMAIN "btif_spp"
#include "btsrv_os_common.h"
#include "btsrv_inner.h"

int btif_spp_register_processer(void)
{
	return btsrv_register_msg_processer(MSG_BTSRV_SPP, &btsrv_spp_process);
}

int btif_spp_reg(struct bt_spp_reg_param *param)
{
	if (!param || !param->app_id || !param->uuid) {
		return -EINVAL;
	}

	return btsrv_function_call_malloc(MSG_BTSRV_SPP, MSG_BTSRV_SPP_REGISTER, (void *)param, sizeof(struct bt_spp_reg_param), 0);
}

int btif_spp_send_data(uint8_t app_id, uint8_t *data, uint32_t len)
{
	return btsrv_spp_send_data(app_id, data, len);
}

int btif_spp_connect(struct bt_spp_connect_param *param)
{
	if (!param->app_id || !param->uuid) {
		return -EINVAL;
	}

	return btsrv_function_call_malloc(MSG_BTSRV_SPP, MSG_BTSRV_SPP_CONNECT, (void *)param, sizeof(struct bt_spp_connect_param), 0);
}

int btif_spp_disconnect(uint8_t app_id)
{
	int id = app_id;

	return btsrv_function_call(MSG_BTSRV_SPP, MSG_BTSRV_SPP_DISCONNECT, (void *)id);
}

int btif_spp_start(btsrv_spp_callback cb)
{
	return btsrv_function_call(MSG_BTSRV_SPP, MSG_BTSRV_SPP_START, cb);
}

int btif_spp_stop(void)
{
	return btsrv_function_call(MSG_BTSRV_SPP, MSG_BTSRV_SPP_STOP, NULL);
}

int btif_spp_sdp_restore(void)
{
	return btsrv_function_call(MSG_BTSRV_SPP, MSG_BTSRV_SPP_SDP_SERVICE_RESTORE, NULL);
}
