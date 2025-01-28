/*
 * hid_internal.h - hid handling

 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <acts_bluetooth/hid.h>

#define BT_L2CAP_PSM_HID_CTL 0x0011
#define BT_L2CAP_PSM_HID_INT 0x0013

/* Define the HID transaction types
*/
#define BT_HID_TYPE_HANDSHAKE                       (0)
#define BT_HID_TYPE_CONTROL                         (1)
#define BT_HID_TYPE_GET_REPORT                      (4)
#define BT_HID_TYPE_SET_REPORT                      (5)
#define BT_HID_TYPE_GET_PROTOCOL                    (6)
#define BT_HID_TYPE_SET_PROTOCOL                    (7)
#define BT_HID_TYPE_DATA                            (10)

/* Parameters for Control
*/
#define BT_HID_CONTROL_SUSPEND                      (3)
#define BT_HID_CONTROL_EXIT_SUSPEND                 (4)
#define BT_HID_CONTROL_VIRTUAL_CABLE_UNPLUG         (5)

/* Different report types in get, set, data
*/
#define BT_HID_REP_TYPE_OTHER                       (0)
#define BT_HID_REP_TYPE_INPUT                       (1)
#define BT_HID_REP_TYPE_OUTPUT                      (2)
#define BT_HID_REP_TYPE_FEATURE                     (3)

/* Parameters for Protocol Type
*/
#define BT_HID_PROTOCOL_MASK                        (0x01)
#define BT_HID_PROTOCOL_BOOT_MODE                   (0)
#define BT_HID_PROTOCOL_REPORT                      (1)

/* @brief HID DEV STATE */
#define BT_HID_STATE_IDLE                           (0)
#define BT_HID_STATE_CTRL_CONNECTING                (1)
#define BT_HID_STATE_CTRL_CONNECTED                 (2)
#define BT_HID_STATE_INTR_CONNECTING                (3)
#define BT_HID_STATE_CONNECTED                      (4)
#define BT_HID_STATE_DISCONNECT                     (5)


#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
struct bt_hid_hdr {
	uint8_t param:4;
	uint8_t type:4;
} __packed;

#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
struct bt_hid_hdr {
	uint8_t type:4;
	uint8_t param:4;
} __packed;

#endif

#define BT_HID_HDR_LEN sizeof(struct bt_hid_hdr)

typedef enum bt_hid_role {
	BT_HID_ROLE_ACCEPTOR,
	BT_HID_ROLE_INITIATOR
} __packed bt_hid_role_t;

typedef enum bt_hid_session_role {
	BT_HID_SESSION_ROLE_CTRL,
	BT_HID_SESSION_ROLE_INTR
} __packed bt_hid_session_role_t;

/** @brief Global HID session structure. */
struct bt_hid {
	struct bt_l2cap_br_chan br_chan;
	bt_hid_session_role_t role;			/* As ctrl or intr */
};

struct bt_hid_conn {
	struct bt_conn *conn;
	struct bt_hid  ctrl_session;
	struct bt_hid  intr_session;
	bt_hid_role_t init_role;			/* As initial or accept */
	uint8_t hid_state;
	uint8_t hid_unplug;
	struct net_buf *buf;
};

#define HID_SESSION_BY_CHAN(_ch) CONTAINER_OF(_ch, struct bt_hid, br_chan.chan)

/* Initialize HID layer*/
int bt_hiddev_init(void);
void bt_hid_dump_info(void);
