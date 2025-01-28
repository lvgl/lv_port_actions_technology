/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt avrcp interface
 */

#define SYS_LOG_DOMAIN "btif_avrcp"
#include "btsrv_os_common.h"
#include "btsrv_inner.h"

int btif_avrcp_register_processer(void)
{
	return btsrv_register_msg_processer(MSG_BTSRV_AVRCP, &btsrv_avrcp_process);
}

int btif_avrcp_start(btsrv_avrcp_callback_t *cb)
{
	return btsrv_function_call(MSG_BTSRV_AVRCP, MSG_BTSRV_AVRCP_START, (void *)cb);
}

int btif_avrcp_stop(void)
{
	return btsrv_function_call(MSG_BTSRV_AVRCP, MSG_BTSRV_AVRCP_STOP, (void *)NULL);
}

int btif_avrcp_connect(bd_address_t *addr)
{
	return btsrv_function_call_malloc(MSG_BTSRV_AVRCP, MSG_BTSRV_AVRCP_CONNECT_TO, (void *)addr, sizeof(bd_address_t), 0);
}

int btif_avrcp_disconnect(bd_address_t *addr)
{
	return btsrv_function_call_malloc(MSG_BTSRV_AVRCP, MSG_BTSRV_AVRCP_DISCONNECT, (void *)addr, sizeof(bd_address_t), 0);
}

int btif_avrcp_send_command(int command)
{
	return btsrv_function_call(MSG_BTSRV_AVRCP, MSG_BTSRV_AVRCP_SEND_CMD, (void *)command);
}

int btif_avrcp_sync_vol(uint32_t volume)
{
	return btsrv_function_call(MSG_BTSRV_AVRCP, MSG_BTSRV_AVRCP_SYNC_VOLUME, (void *)volume);
}

int btif_avrcp_get_id3_info()
{
	return btsrv_function_call(MSG_BTSRV_AVRCP, MSG_BTSRV_AVRCP_GET_ID3_INFO, (void *)NULL);
}

bool btif_avrcp_is_support_get_playback_pos()
{
	bool ret;
	int flags;

	flags = btsrv_set_negative_prio();
	ret = btsrv_avrcp_is_support_get_playback_pos();
	btsrv_revert_prio(flags);

	return ret;
}

int btif_avrcp_get_playback_pos()
{
	return btsrv_function_call(MSG_BTSRV_AVRCP, MSG_BTSRV_AVRCP_GET_PLAYBACK_POS, (void *)NULL);
}

int btif_avrcp_set_absolute_volume(uint8_t dev_type, uint8_t *data, uint8_t len)
{
	union {
		uint8_t c_param[4]; /* 0:dev_type, 1:len, 2~3:data */
		int32_t i_param;
	} param;

	if (len < 1 || len > 2) {
		return -EINVAL;
	}

	param.c_param[0] = dev_type;
	param.c_param[1] = len;
	param.c_param[2] = data[0];
	param.c_param[3] = (len == 2) ? data[1] : 0;

	return btsrv_event_notify_value(MSG_BTSRV_AVRCP, MSG_BTSRV_AVRCP_SET_ABSOLUTE_VOLUME, param.i_param);
}

int btif_avrcp_get_play_status(void)
{
	return btsrv_function_call(MSG_BTSRV_AVRCP, MSG_BTSRV_AVRCP_GET_PLAY_STATE, (void *)NULL);
}
