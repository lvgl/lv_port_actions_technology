/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt service core interface
 */
#define SYS_LOG_DOMAIN "btsrv_pts"
#include "btsrv_os_common.h"
#include "btsrv_inner.h"

extern int bt_pts_conn_creat_add_sco_cmd(struct bt_conn *brle_conn);

int btsrv_pts_send_hfp_cmd(char *cmd)
{
	struct bt_conn *br_conn = btsrv_rdm_hfp_get_actived();

	if (br_conn == NULL) {
		return -EIO;
	}

	hostif_bt_hfp_hf_send_cmd(br_conn, cmd);
	return 0;
}

int btsrv_pts_hfp_active_connect_sco(void)
{
	struct bt_conn *br_conn = btsrv_rdm_hfp_get_actived();

	if (br_conn == NULL) {
		return -EIO;
	}

	hostif_bt_conn_create_sco(br_conn);
	return 0;
}

int btsrv_pts_avrcp_pass_through_cmd(uint8_t opid)
{
	struct bt_conn *br_conn = btsrv_rdm_avrcp_get_connected_dev();

	if (br_conn == NULL) {
		return -EIO;
	}

	hostif_bt_avrcp_ct_pass_through_cmd(br_conn, opid, true);
	os_sleep(5);
	hostif_bt_avrcp_ct_pass_through_cmd(br_conn, opid, false);
	return 0;
}

int btsrv_pts_avrcp_notify_volume_change(uint8_t volume)
{
	struct bt_conn *br_conn = btsrv_rdm_avrcp_get_connected_dev();

	if (br_conn == NULL) {
		return -EIO;
	}

	hostif_bt_avrcp_tg_notify_change(br_conn, volume);
	return 0;
}

int btsrv_pts_avrcp_reg_notify_volume_change(void)
{
	struct bt_conn *br_conn = btsrv_rdm_avrcp_get_connected_dev();

	if (br_conn == NULL) {
		return -EIO;
	}

	hostif_bt_pts_avrcp_ct_get_capabilities(br_conn);
	os_sleep(100);
	hostif_bt_pts_avrcp_ct_register_notification(br_conn);
	return 0;
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	hostif_bt_conn_auth_pairing_confirm(conn);
}

static void auth_pincode_entry(struct bt_conn *conn, bool highsec)
{
}

static void auth_cancel(struct bt_conn *conn)
{
}

static void auth_pairing_confirm(struct bt_conn *conn)
{
}

static const struct bt_conn_auth_cb auth_cb_display_yes_no = {
	.passkey_display = auth_passkey_display,
	.passkey_entry = NULL,
	.passkey_confirm = auth_passkey_confirm,
	.pincode_entry = auth_pincode_entry,
	.cancel = auth_cancel,
	.pairing_confirm = auth_pairing_confirm,
};

int btsrv_pts_register_auth_cb(bool reg_auth)
{
	if (reg_auth) {
		hostif_bt_conn_auth_cb_register(&auth_cb_display_yes_no);
	} else {
		hostif_bt_conn_auth_cb_register(NULL);
	}

	return 0;
}
