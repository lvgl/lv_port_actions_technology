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


int btif_pts_a2dp_abort(void)
{
	if (btsrv_is_pts_test()) {
		btsrv_pts_a2dp_abort();
	}
	return 0;
}

int btif_pts_a2dp_discover(void)
{
	if (btsrv_is_pts_test()) {
		btsrv_pts_a2dp_discover();
	}
	return 0;
}

int btif_pts_a2dp_get_capabilities(void)
{
	if (btsrv_is_pts_test()) {
		btsrv_pts_a2dp_get_capabilities();
	}
	return 0;
}

int btif_pts_a2dp_get_all_capabilities(void)
{
	if (btsrv_is_pts_test()) {
		btsrv_pts_a2dp_get_all_capabilities();
	}
	return 0;
}

int btif_pts_a2dp_set_configuration(void)
{
	if (btsrv_is_pts_test()) {
		btsrv_pts_a2dp_set_configuration();
	}
	return 0;
}

int btif_pts_a2dp_open(void)
{
	if (btsrv_is_pts_test()) {
		btsrv_pts_a2dp_open();
	}
	return 0;
}

int btif_pts_a2dp_start(void)
{
	if (btsrv_is_pts_test()) {
		btsrv_pts_a2dp_start();
	}
	return 0;
}

int btif_pts_a2dp_suspend(void)
{
	if (btsrv_is_pts_test()) {
		btsrv_pts_a2dp_suspend();
	}
	return 0;
}

int btif_pts_a2dp_close(void)
{
	if (btsrv_is_pts_test()) {
		btsrv_pts_a2dp_close();
	}
	return 0;
}

int btif_pts_a2dp_disconnect_media_session(void)
{
	if (btsrv_is_pts_test()) {
		btsrv_pts_a2dp_disconnect_media_session();
	}
	return 0;
}

int btif_pts_a2dp_connect(uint8_t type)
{
	if (btsrv_is_pts_test()) {
		btsrv_pts_a2dp_connect(type);
	}
	return 0;
}
int btif_pts_a2dp_reconfig(void *ptr)
{
	if (btsrv_is_pts_test()) {
		btsrv_pts_a2dp_reconfig(ptr);
	}
	return 0;
}

int btif_pts_a2dp_send_audio_data(uint8_t *data, uint16_t len)
{
	if (btsrv_is_pts_test()) {
		btsrv_pts_a2dp_send_audio_data(data, len);
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
