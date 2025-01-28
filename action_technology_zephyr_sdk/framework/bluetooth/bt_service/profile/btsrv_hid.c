/*
 * Copyright (c) 2017 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief btsrvice hid
 */

#define SYS_LOG_DOMAIN "btsrv_hid"
#include "btsrv_os_common.h"
#include "btsrv_inner.h"

static btsrv_hid_callback hid_user_callback;

static void btsrv_hid_user_callback(struct bt_conn *conn, btsrv_hid_event_e event, void *packet, int size)
{
	if (hid_user_callback) {
		hid_user_callback(hostif_bt_conn_get_handle(conn), event, packet, size);
	}
}

static void hid_connected_cb(struct bt_conn *conn)
{
	btsrv_event_notify(MSG_BTSRV_CONNECT, MSG_BTSRV_HID_CONNECTED, conn);
}

static void hid_disconnected_cb(struct bt_conn *conn)
{
	btsrv_event_notify(MSG_BTSRV_CONNECT, MSG_BTSRV_HID_DISCONNECTED, conn);
}

static void hid_event_cb(struct bt_conn *conn, uint8_t event, uint8_t *data, uint16_t len)
{
	struct hid_stack_report stack_report, *p_stack_report;

	if (!data) {
		memset(&stack_report, 0, sizeof(stack_report));
		p_stack_report = &stack_report;
	} else {
		p_stack_report = (struct hid_stack_report *)data;
	}
	p_stack_report->conn = conn;

	btsrv_event_notify_malloc(MSG_BTSRV_HID, MSG_BTSRV_HID_EVENT_CB, (uint8_t *)p_stack_report, sizeof(stack_report), event);
}

static void hid_event_proc(void *data, uint8_t event)
{
	int cb_ev = -1;
	struct hid_stack_report *stack_report = data;
	struct bt_hid_report hid_report;

	/* Callback by hci_rx thread, in negative priority,
	 * just check hid_user_callback is enough, not need to lock.
	 */
	switch(event){
	case BT_HID_EVENT_GET_REPORT:
		cb_ev = BTSRV_HID_GET_REPORT;
		break;
	case BT_HID_EVENT_SET_REPORT:
		cb_ev = BTSRV_HID_SET_REPORT;
		break;
	case BT_HID_EVENT_GET_PROTOCOL:
		cb_ev = BTSRV_HID_GET_PROTOCOL;
		break;
	case BT_HID_EVENT_SET_PROTOCOL:
		cb_ev = BTSRV_HID_SET_PROTOCOL;
		break;
	case BT_HID_EVENT_INTR_DATA:
		cb_ev = BTSRV_HID_INTR_DATA;
	break;
	case BT_HID_EVENT_UNPLUG:
		cb_ev = BTSRV_HID_UNPLUG;
		btsrv_event_notify(MSG_BTSRV_CONNECT, MSG_BTSRV_HID_UNPLUG, stack_report->conn);
		break;
	case BT_HID_EVENT_SUSPEND:
		cb_ev = BTSRV_HID_SUSPEND;
		break;
	case BT_HID_EVENT_EXIT_SUSPEND:
		cb_ev = BTSRV_HID_EXIT_SUSPEND;
		break;
	}

	if (cb_ev >= 0) {
		memset(&hid_report, 0, sizeof(hid_report));
		hid_report.hdl = hostif_bt_conn_get_handle(stack_report->conn);
		hid_report.report_type = stack_report->report_type;
		hid_report.has_size = stack_report->has_size;
		hid_report.reserved = stack_report->reserved;
		hid_report.len = stack_report->len;
		if (stack_report->len) {
			memcpy(hid_report.data, stack_report->data, stack_report->len);
		}

		btsrv_hid_user_callback(stack_report->conn, cb_ev, &hid_report, sizeof(hid_report));
	}
}

static const struct bt_hid_app_cb hid_app_cb = {
	.connected = hid_connected_cb,
	.disconnected = hid_disconnected_cb,
	.event_cb = hid_event_cb,
};

void btsrv_hid_connect(struct bt_conn *conn)
{
	int ret;

	if (conn && !btsrv_rdm_is_hid_connected(conn)) {
		ret = hostif_bt_hid_connect(conn);
		if (!ret) {
			SYS_LOG_INF("Connect hid");
		} else {
			SYS_LOG_ERR("Connect hid failed %d", ret);
		}
	}
}

static void btsrv_hid_connect_by_hdl(void *param)
{
	uint16_t hdl = (uint16_t)(uint32_t)param;
	struct bt_conn *conn;

	if (!hdl) {
		return;
	}

	conn = btsrv_rdm_find_conn_by_hdl(hdl);
	if (conn) {
		btsrv_hid_connect(conn);
	}
}

static void btsrv_hid_disconnect_by_hdl(void *param)
{
	uint16_t hdl = (uint16_t)(uint32_t)param;
	struct bt_conn *conn;

	if (hdl) {
		conn = btsrv_rdm_find_conn_by_hdl(hdl);
		if (conn && btsrv_rdm_is_hid_connected(conn)) {
			SYS_LOG_INF("hid_disconnect");
			hostif_bt_hid_disconnect(conn);
		}
	}
}

static void btsrv_hid_connected(struct bt_conn *conn)
{
	btsrv_hid_user_callback(conn, BTSRV_HID_CONNECTED, NULL, 0);
}

static void btsrv_hid_disconnected(struct bt_conn *conn)
{
	btsrv_hid_user_callback(conn, BTSRV_HID_DISCONNECTED, NULL, 0);
}

static int btsrv_hid_send_ctrl_data(struct bt_hid_report *report)
{
	struct bt_conn *conn;

	if (report->hdl) {
		conn = btsrv_rdm_find_conn_by_hdl(report->hdl);
	} else {
		conn = btsrv_rdm_hid_get_actived();
	}

	if(conn) {
		return hostif_bt_hid_send_ctrl_data(conn, report->report_type, report->data, report->len);
	}

	return 0;
}

static int btsrv_hid_send_intr_data(struct bt_hid_report * report)
{
	struct bt_conn *conn;

	if (report->hdl) {
		conn = btsrv_rdm_find_conn_by_hdl(report->hdl);
	} else {
		conn = btsrv_rdm_hid_get_actived();
	}

	if (!btsrv_rdm_is_hid_connected(conn)) {
		return -EIO;
	}

	if(conn) {
		return hostif_bt_hid_send_intr_data(conn,report->report_type, report->data, report->len);
	}

	return 0;
}

static int btsrv_hid_send_rsp(void *param, uint8_t status)
{
	uint16_t hdl = (uint16_t)(uint32_t)param;
	struct bt_conn *conn = btsrv_rdm_find_conn_by_hdl(hdl);

	return hostif_bt_hid_send_rsp(conn, status);
}

int btsrv_hid_process(struct app_msg *msg)
{
	switch (_btsrv_get_msg_param_cmd(msg)) {
	case MSG_BTSRV_HID_START:
		hid_user_callback = (btsrv_hid_callback)msg->ptr;
		hostif_bt_hid_register_cb((struct bt_hid_app_cb *)&hid_app_cb);
		break;
	case MSG_BTSRV_HID_STOP:
		hid_user_callback = NULL;
		break;
	case MSG_BTSRV_HID_REGISTER:
		hostif_bt_hid_register_sdp(msg->ptr,_btsrv_get_msg_param_reserve(msg));
		break;
	case MSG_BTSRV_DID_REGISTER:
		hostif_bt_did_register_sdp(msg->ptr);
		break;
	case MSG_BTSRV_HID_CONNECT:
		SYS_LOG_INF("MSG_BTSRV_HID_CONNECT");
		btsrv_hid_connect_by_hdl(msg->ptr);
		break;
	case MSG_BTSRV_HID_DISCONNECT:
		SYS_LOG_INF("MSG_BTSRV_HID_DISCONNECT");
		btsrv_hid_disconnect_by_hdl(msg->ptr);
		break;
	case MSG_BTSRV_HID_CONNECTED:
		SYS_LOG_INF("MSG_BTSRV_HID_CONNECTED");
		btsrv_hid_connected(_btsrv_get_msg_param_ptr(msg));
		break;
	case MSG_BTSRV_HID_DISCONNECTED:
		SYS_LOG_INF("MSG_BTSRV_HID_DISCONNECTED");
		btsrv_hid_disconnected(_btsrv_get_msg_param_ptr(msg));
		break;
	case MSG_BTSRV_HID_EVENT_CB:
		SYS_LOG_INF("MSG_BTSRV_HID_EVENT_CB");
		hid_event_proc(msg->ptr, _btsrv_get_msg_param_reserve(msg));
	case MSG_BTSRV_HID_SEND_CTRL_DATA:
		SYS_LOG_INF("MSG_BTSRV_HID_SEND_CTRL_DATA");
		btsrv_hid_send_ctrl_data(_btsrv_get_msg_param_ptr(msg));
		break;
	case MSG_BTSRV_HID_SEND_INTR_DATA:
		SYS_LOG_INF("MSG_BTSRV_HID_SEND_INTR_DATA");
		btsrv_hid_send_intr_data(_btsrv_get_msg_param_ptr(msg));
		break;
	case MSG_BTSRV_HID_SEND_RSP:
		SYS_LOG_INF("MSG_BTSRV_HID_SEND_RSP");
		btsrv_hid_send_rsp(msg->ptr, _btsrv_get_msg_param_reserve(msg));
		break;
	default:
		break;
	}

	return 0;
}
