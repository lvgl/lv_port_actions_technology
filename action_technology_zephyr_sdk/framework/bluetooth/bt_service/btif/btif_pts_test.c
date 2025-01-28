/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt srv api interface
 */

#define SYS_LOG_DOMAIN "btif_pts_test"
#include "btsrv_os_common.h"
#include "btsrv_inner.h"

int btif_pts_send_hfp_cmd(char *cmd)
{
	return btsrv_pts_send_hfp_cmd(cmd);
}

int btif_pts_hfp_active_connect_sco(void)
{
	return btsrv_pts_hfp_active_connect_sco();
}

int btif_pts_a2dp_set_err_code(uint8_t err_code)
{
	bt_pts_a2dp_set_err_code(err_code);
	return 0;
}

int btif_pts_avrcp_pass_through_cmd(uint8_t opid)
{
	if (btsrv_is_pts_test()) {
		btsrv_pts_avrcp_pass_through_cmd(opid);
	}
	return 0;
}

int btif_pts_avrcp_notify_volume_change(uint8_t volume)
{
	if (btsrv_is_pts_test()) {
		btsrv_pts_avrcp_notify_volume_change(volume);
	}
	return 0;
}

int btif_pts_avrcp_reg_notify_volume_change(void)
{
	if (btsrv_is_pts_test()) {
		btsrv_pts_avrcp_reg_notify_volume_change();
	}
	return 0;
}

int btif_pts_register_auth_cb(bool reg_auth)
{
	if (btsrv_is_pts_test()) {
		btsrv_pts_register_auth_cb(reg_auth);
	}

	return 0;
}

int btif_pts_set_class_of_device(uint32_t classOfDevice)
{
	return hostif_bt_set_class_of_device(classOfDevice);
}
