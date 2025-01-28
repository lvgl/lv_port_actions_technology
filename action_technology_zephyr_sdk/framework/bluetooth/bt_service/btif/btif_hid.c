/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt hid interface
 */

#define SYS_LOG_DOMAIN "btif_hid"
#include "btsrv_os_common.h"
#include "btsrv_inner.h"

int btif_hid_register_processer(void)
{
	return btsrv_register_msg_processer(MSG_BTSRV_HID, &btsrv_hid_process);
}

int btif_hid_register_sdp(struct bt_sdp_attribute *hid_attrs, uint8_t attrs_size)
{
	return btsrv_event_notify_ext(MSG_BTSRV_HID, MSG_BTSRV_HID_REGISTER, hid_attrs, attrs_size);
}

int btif_did_register_sdp(uint8_t *data, uint32_t len)
{
	return btsrv_function_call_malloc(MSG_BTSRV_HID, MSG_BTSRV_DID_REGISTER, data,len,0);
}

static int btif_hid_send_data(uint8_t cmd, uint16_t hdl, uint8_t report_type, uint8_t *data, uint32_t len)
{
	struct bt_hid_report report;

	if (len > BT_HID_REPORT_DATA_LEN) {
		SYS_LOG_ERR("Need extern len %d", len);
		return -ENOMEM;
	}

	report.hdl = hdl;
	report.report_type = report_type;
	report.has_size = 0;
	memcpy(report.data, data, len);
	report.len = len;

	return btsrv_function_call_malloc(MSG_BTSRV_HID, cmd, (uint8_t *)&report, sizeof(report), 0);
}

int btif_hid_send_ctrl_data(uint16_t hdl, uint8_t report_type, uint8_t *data, uint32_t len)
{
	return btif_hid_send_data(MSG_BTSRV_HID_SEND_CTRL_DATA, hdl, report_type, data, len);
}

int btif_hid_send_intr_data(uint16_t hdl, uint8_t report_type,uint8_t *data, uint32_t len)
{
	return btif_hid_send_data(MSG_BTSRV_HID_SEND_INTR_DATA, hdl, report_type, data, len);
}

int btif_hid_send_rsp(uint16_t hdl, uint8_t status)
{
	return btsrv_event_notify_ext(MSG_BTSRV_HID, MSG_BTSRV_HID_SEND_RSP, (void *)(uint32_t)hdl, status);
}

int btif_hid_connect(uint16_t hdl)
{
	return btsrv_function_call(MSG_BTSRV_HID, MSG_BTSRV_HID_CONNECT, (void *)(uint32_t)hdl);
}

int btif_hid_disconnect(uint16_t hdl)
{
	return btsrv_function_call(MSG_BTSRV_HID, MSG_BTSRV_HID_DISCONNECT, (void *)(uint32_t)hdl);
}

int btif_hid_start(btsrv_hid_callback cb)
{
	return btsrv_function_call(MSG_BTSRV_HID, MSG_BTSRV_HID_START, cb);
}

int btif_hid_stop(void)
{
	return btsrv_function_call(MSG_BTSRV_HID, MSG_BTSRV_HID_STOP, NULL);
}
