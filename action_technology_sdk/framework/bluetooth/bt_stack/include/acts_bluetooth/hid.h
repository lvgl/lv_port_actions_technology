/** @file
 * @brief Human Interface Device Profile header.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_HID_H
#define __BT_HID_H

#ifdef __cplusplus
extern "C" {
#endif

#include <acts_bluetooth/sdp.h>
#include <acts_bluetooth/hci.h>

#define BT_HID_MAX_MTU 64

enum {
	BT_HID_EVENT_GET_REPORT,
	BT_HID_EVENT_SET_REPORT,
	BT_HID_EVENT_GET_PROTOCOL,
	BT_HID_EVENT_SET_PROTOCOL,
	BT_HID_EVENT_INTR_DATA,
	BT_HID_EVENT_UNPLUG,
	BT_HID_EVENT_SUSPEND,
	BT_HID_EVENT_EXIT_SUSPEND,
};

/* Different report types in get, set, data
*/
enum {
	BT_HID_REP_TYPE_OTHER=0,
	BT_HID_REP_TYPE_INPUT,
	BT_HID_REP_TYPE_OUTPUT,
	BT_HID_REP_TYPE_FEATURE,
};

/* Parameters for Handshake
*/
enum {
	BT_HID_HANDSHAKE_RSP_SUCCESS=0,
	BT_HID_HANDSHAKE_RSP_NOT_READY,
	BT_HID_HANDSHAKE_RSP_ERR_INVALID_REP_ID,
	BT_HID_HANDSHAKE_RSP_ERR_UNSUPPORTED_REQ,
	BT_HID_HANDSHAKE_RSP_ERR_INVALID_PARAM,
	BT_HID_HANDSHAKE_RSP_ERR_UNKNOWN=14,
	BT_HID_HANDSHAKE_RSP_ERR_FATAL,
};

/* Parameters for set protocal
*/
enum {
	BT_HID_PROTOCOL_BOOT_MODE=0,
	BT_HID_PROTOCOL_REPORT_MODE,
};

#define HID_REPORT_DATA_LEN		64

struct hid_stack_report {
	struct bt_conn *conn;
	uint8_t report_type;
	uint8_t has_size:1;
	uint8_t reserved:3;
	uint8_t data[HID_REPORT_DATA_LEN];
	int len;
};

struct bt_hid_app_cb {
	void (*connected)(struct bt_conn *conn);
	void (*disconnected)(struct bt_conn *conn);
	void (*event_cb)(struct bt_conn *conn, uint8_t event,uint8_t *data, uint16_t len);
};

/** @brief HID Connect.
 *
 *  This function is to be called after the conn parameter is obtained by
 *  performing a GAP procedure. The API is to be used to establish HID
 *  connection between devices.
 *
 *  @param conn Pointer to bt_conn structure.
 *  @param role hid as source or sink role
 *
 *  @return 0 in case of success and error code in case of error.
 *  of error.
 */
int bt_hid_connect(struct bt_conn *conn);

/* Disconnect hid session */
int bt_hid_disconnect(struct bt_conn *conn);

/* Register app callback */
int bt_hid_register_cb(struct bt_hid_app_cb *cb);

/* send hid data by ctrl channel */
int bt_hid_send_ctrl_data(struct bt_conn *conn,uint8_t report_type,uint8_t * data,uint16_t len);

/* send hid data by intr channel */
int bt_hid_send_intr_data(struct bt_conn *conn,uint8_t report_type,uint8_t * data,uint16_t len);

/* send hid status by ctrl channel */
int bt_hid_send_rsp(struct bt_conn *conn,uint8_t status);

/* Register hid dev sdp record */
int bt_hid_register_sdp(struct bt_sdp_attribute * hid_attrs,int attrs_size);

#ifdef __cplusplus
}
#endif

#endif /* __BT_HID_H */
