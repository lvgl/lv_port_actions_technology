/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager spp profile.
 */
#define SYS_LOG_DOMAIN "bt manager"

#include <os_common_api.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>
#include <sys_event.h>
#include <bt_manager.h>
#include "bt_manager_inner.h"
#include "btservice_api.h"

#define HID_SDP_ATT_OPTION			0
#define HID_MAX_REPORT     			(3)
#define HID_CONNECTED_TIMEOUT		2000		/* 2000ms */

enum {
	HID_STATE_IDLE,
	HID_STATE_WAIT_CONNECTED,
	HID_STATE_WAIT_DELAY_OPT,
	HID_STATE_WAIT_DISCONNECT,
	HID_STATE_FORCE_CONNECT,
};

struct report_info{
	uint8_t report_id;
	uint8_t type;
	uint16_t max_len;
	uint8_t last_report[64];
	uint16_t data_len;
};

struct btmgr_hid_info {
	uint8_t			sub_cls;			/* HID device subclass */
	struct report_info info[HID_MAX_REPORT];
	uint16_t disconnect_delay;
	uint16_t opt_delay;
};

static struct btmgr_hid_info mgr_hid_info;
static OS_MUTEX_DEFINE(hid_mgr_lock);

#if CONFIG_BT_PTS_TEST
static const uint8_t hid_descriptor[] =
{
    0x05, 0x01, //USAGE_PAGE (Generic Desktop Controls)
    0x09, 0x06, //USAGE (Keyboard)
    0xa1, 0x01, //COLLECTION (Application (mouse, keyboard))
    0x85, 0x01, //REPORT_ID (1)
    0x75, 0x01, //report size(1)
    0x95, 0x08, //report count(8)
    0x05, 0x07, //usage page(Keyboard/Keypad )
    0x19, 0xe0, //Usage Minimum
    0x29, 0xe7, //Usage Maximum
    0x15, 0x00, //Logical Minimum
    0x25, 0x01, //Logical Maxiimum
    0x81, 0x02, //Input()

    0x75, 0x08, //report size()
    0x95, 0x01, //report count()
    0x91, 0x03, //Output

    0x95, 0x03, //Report Count
    0x75, 0x08, //report size
    0x15, 0x00, //Logical Minimum
    0x26, 0xff, 0x00, //Logical Maxiimum
    0x05, 0x07, //usage page(Keyboard/Keypad )
    0x19, 0x00, //Usage Minimum
    0x29, 0xff, //usage Maximum
    0x81, 0x00, //input()

    0xc0,       //END_COLLECTION
    
    0x05, 0x0c, // USAGE_PAGE (Consumer)
    0x09, 0x01, //USAGE (Consumer control)
    0xa1, 0x01, //COLLECTION (Application (mouse, keyboard))
    0x85, 0x02, //REPORT_ID (2)
    0x15, 0x00, //Logical Minimum
    0x25, 0x01, //Logical Maximum
    0x75, 0x01, //Report size(1)
    0x95, 0x08, //Report Count(8)

    0x09, 0xea, //USAGE (volume down)
    0x09, 0xe9, //USAGE (volume up)
    0x09, 0xe2, //USAGE (mute)
    0x09, 0xcd, //USAGE (play/pause)
    0x09, 0xb6, //USAGE (scan previous track)
    0x09, 0xb5, //USAGE (scan next track)
    0x09, 0x83, //USAGE (fast forward)
    0x09, 0xb4, //USAGE (rewind)
    0x81, 0x02, //input(data, variable, absolute)
    0xc0,       //END_COLLECTION
};
#else
static const uint8_t hid_descriptor[] =
{
    0x05, 0x01, //USAGE_PAGE (Generic Desktop Controls)
    0x09, 0x02, //USAGE (Mouse)
    0xa1, 0x01, //COLLECTION (Application (mouse, keyboard))
    0x85, 0x01, //REPORT_ID (1)
    0x75, 0x08, //report size()
    0x95, 0x01, //report count()
    0x91, 0x03, //Output
    0xc0,       //END_COLLECTION

    0x05, 0x0c, // USAGE_PAGE (Consumer)
    0x09, 0x01, //USAGE (Consumer control)
    0xa1, 0x01, //COLLECTION (Application (mouse, keyboard))
    0x85, 0x02, //REPORT_ID (2)
    0x15, 0x00, //Logical Minimum
    0x25, 0x01, //Logical Maximum
    0x75, 0x01, //Report size(1)
    0x95, 0x08, //Report Count(8)

    0x09, 0xea, //USAGE (volume down)
    0x09, 0xe9, //USAGE (volume up)
    0x09, 0xe2, //USAGE (mute)
    0x09, 0xcd, //USAGE (play/pause)
    0x09, 0xb6, //USAGE (scan previous track)
    0x09, 0xb5, //USAGE (scan next track)
    0x09, 0x83, //USAGE (fast forward)
    0x09, 0xb4, //USAGE (rewind)
    0x81, 0x02, //input(data, variable, absolute)
    0xc0,       //END_COLLECTION
};
#endif

/*
Report id for hid key:
report id         		 hid key
1			0x09, 0xea, //USAGE (volume down)
2			0x09, 0xe9, //USAGE (volume up)
3			0x09, 0xe2, //USAGE (mute)
4			0x09, 0xcd, //USAGE (play/pause)
5			0x09, 0xb6, //USAGE (scan previous track)
6			0x09, 0xb5, //USAGE (scan next track)
7			0x09, 0x83, //USAGE (fast forward)
8			0x09, 0xb4, //USAGE (rewind)
*/

static const struct bt_sdp_attribute hid_dev_attrs[] = {
	BT_SDP_NEW_RECORD_HANDLE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST_CONST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16_CONST(BT_SDP_HID_SVCLASS)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 13),
		BT_SDP_DATA_ELEM_LIST_CONST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST_CONST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16_CONST(BT_SDP_PROTO_L2CAP)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16_CONST(0x0011)			/* HID-CONTROL */
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
			BT_SDP_DATA_ELEM_LIST_CONST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16_CONST(0x0011)
			},
			)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_LANG_BASE_ATTR_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 9),
		BT_SDP_DATA_ELEM_LIST_CONST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			BT_SDP_ARRAY_8_CONST('n', 'e')
		},
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			BT_SDP_ARRAY_16_CONST(106)
		},
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			BT_SDP_ARRAY_16_CONST(BT_SDP_PRIMARY_LANG_BASE)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST_CONST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST_CONST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16_CONST(BT_SDP_HID_SVCLASS)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16_CONST(0x0101)		/* Version 1.1 */
			},
			)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_ADD_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 15),
		BT_SDP_DATA_ELEM_LIST_CONST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 13),
			BT_SDP_DATA_ELEM_LIST_CONST(
			{
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
				BT_SDP_DATA_ELEM_LIST_CONST(
				{
					BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
					BT_SDP_ARRAY_16_CONST(BT_SDP_PROTO_L2CAP)
				},
				{
					BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
					BT_SDP_ARRAY_16_CONST(0x0013)  /* HID-INTERRUPT */
				},
				)
				
			},
			{
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
				BT_SDP_DATA_ELEM_LIST_CONST(
				{
					BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
					BT_SDP_ARRAY_16_CONST(0x0011)
				},
				)
			},
			)
		},
		)
	),
#if HID_SDP_ATT_OPTION
	BT_SDP_SERVICE_NAME("HID CONTROL"),
#endif
	{
		BT_SDP_ATTR_HID_PARSER_VERSION,
		{ BT_SDP_TYPE_SIZE(BT_SDP_UINT16), BT_SDP_ARRAY_16_CONST(0x0111) }
	},
	{
		BT_SDP_ATTR_HID_DEVICE_SUBCLASS,
		{ BT_SDP_TYPE_SIZE(BT_SDP_UINT8), &mgr_hid_info.sub_cls }
	},
	{
		BT_SDP_ATTR_HID_COUNTRY_CODE,
		{ BT_SDP_TYPE_SIZE(BT_SDP_UINT8), BT_SDP_ARRAY_16_CONST(0x21) }
	},
	{
		BT_SDP_ATTR_HID_VIRTUAL_CABLE,
		{ BT_SDP_TYPE_SIZE(BT_SDP_BOOL), BT_SDP_ARRAY_8_CONST(0x1) }
	},
	{
		BT_SDP_ATTR_HID_RECONNECT_INITIATE,
		{ BT_SDP_TYPE_SIZE(BT_SDP_BOOL), BT_SDP_ARRAY_8_CONST(0x1) }
	},
	BT_SDP_LIST(
		BT_SDP_ATTR_HID_DESCRIPTOR_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ16, sizeof(hid_descriptor) + 8),
		BT_SDP_DATA_ELEM_LIST_CONST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ16, sizeof(hid_descriptor) + 5),
			BT_SDP_DATA_ELEM_LIST_CONST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
				BT_SDP_ARRAY_8_CONST(0x22),
			},
			{
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_TEXT_STR16,sizeof(hid_descriptor)),
				hid_descriptor,
			},
			),
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_HID_LANG_ID_BASE_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST_CONST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST_CONST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16_CONST(0x409),
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16_CONST(0x100),
			},
			),
		},
		)
	),
	{
		BT_SDP_ATTR_HID_BOOT_DEVICE,
		{ BT_SDP_TYPE_SIZE(BT_SDP_BOOL), BT_SDP_ARRAY_8_CONST(0x0) }
	},
	{
		BT_SDP_ATTR_HID_SUPERVISION_TIMEOUT,
		{ BT_SDP_TYPE_SIZE(BT_SDP_UINT16), BT_SDP_ARRAY_16_CONST(5000) }
	},
	{
		BT_SDP_ATTR_HID_MAX_LATENCY,
		{ BT_SDP_TYPE_SIZE(BT_SDP_UINT16), BT_SDP_ARRAY_16_CONST(240) }
	},
	{
		BT_SDP_ATTR_HID_MIN_LATENCY,
		{ BT_SDP_TYPE_SIZE(BT_SDP_UINT16), BT_SDP_ARRAY_16_CONST(0x0) }
	},
};

static struct report_info *bt_manager_hid_find_id(int report_id)
{
	int i;
	struct report_info *info = NULL;

	for (i = 0; i < HID_MAX_REPORT; i++) {
		if (mgr_hid_info.info[i].report_id == report_id) {
			info = &mgr_hid_info.info[i];
			break;
		}
	}

	return info;
}

static struct report_info *bt_manager_hid_find(int report_id, int type)
{
	int i;
	struct report_info *info = NULL;

	for (i = 0; i < HID_MAX_REPORT; i++) {
		if(mgr_hid_info.info[i].report_id == report_id
			&& mgr_hid_info.info[i].type == type){
			info = &mgr_hid_info.info[i];
			break;
		}
	}

	return info;
}

int bt_manager_hid_register_sdp()
{
	memset(&mgr_hid_info,0,sizeof(struct btmgr_hid_info));
	mgr_hid_info.sub_cls = 0xc0;
	mgr_hid_info.info[0].report_id = 1;
	mgr_hid_info.info[0].type = BTSRV_HID_REP_TYPE_INPUT;
	mgr_hid_info.info[0].data_len = 5;
	mgr_hid_info.info[0].last_report[0]= mgr_hid_info.info[0].report_id;

	mgr_hid_info.info[1].report_id = 1;
	mgr_hid_info.info[1].type = BTSRV_HID_REP_TYPE_OUTPUT;
	mgr_hid_info.info[1].data_len = 2;
	mgr_hid_info.info[1].last_report[0]= mgr_hid_info.info[1].report_id;

	mgr_hid_info.info[2].report_id = 2;
	mgr_hid_info.info[2].type = BTSRV_HID_REP_TYPE_INPUT;
	mgr_hid_info.info[2].data_len = 2;
	mgr_hid_info.info[2].last_report[0]= mgr_hid_info.info[2].report_id;
	int ret = btif_hid_register_sdp((struct bt_sdp_attribute *)hid_dev_attrs, ARRAY_SIZE(hid_dev_attrs));
	if (ret) {
		SYS_LOG_INF("Failed %d\n", ret);
	}

	return ret;
}

static struct bt_device_id_info device_id;

int bt_manager_did_register_sdp()
{
	memset(&device_id,0,sizeof(struct bt_device_id_info));
	device_id.id_source = 2;//usb
	device_id.product_id = 0xb009;
	device_id.vendor_id = 0x10d6;//actions
	int ret = btif_did_register_sdp((uint8_t*)&device_id,sizeof(struct bt_device_id_info));
	if (ret) {
		SYS_LOG_INF("Failed %d\n", ret);
	}

	return ret;
}

int bt_manager_hid_send_ctrl_data(uint16_t hdl, uint8_t report_type, uint8_t *data, uint32_t len)
{
	uint16_t send_hdl;

	if (hdl) {
		send_hdl = hdl;
	} else {
		send_hdl = btif_br_get_active_phone_hdl();
	}
	return btif_hid_send_ctrl_data(send_hdl, report_type, data, len);
}

int bt_manager_hid_send_intr_data(uint16_t hdl, uint8_t report_type, uint8_t *data, uint32_t len)
{
	uint16_t send_hdl;
	uint8_t report_id = data[0];
	struct report_info * info = bt_manager_hid_find(report_id, report_type);

	if (info && len == info->data_len) {
		if (hdl) {
			send_hdl = hdl;
		} else {
			send_hdl = btif_br_get_active_phone_hdl();
		}

		memcpy(info->last_report, data, len);
		return btif_hid_send_intr_data(send_hdl, report_type, data, len);
	} else {
		SYS_LOG_INF("Format error %p %d != %d", info, len, (info ? info->data_len : 0));
		return -EINVAL;
	}
}

static uint8_t bt_manager_hid_check_report(uint16_t hdl, void *packet, int size, bool get)
{
	struct bt_hid_report *report;
	struct report_info *info;
	int report_id;

	if (!bt_manager_config_pts_test()) {
		return BTSRV_HID_HANDSHAKE_RSP_SUCCESS;
	}

	report = (struct bt_hid_report*)packet;
	SYS_LOG_INF("report %d %d %d %d\n", report->report_type, report->has_size, report->reserved, report->len);

	if (report->reserved) {
		return BTSRV_HID_HANDSHAKE_RSP_ERR_INVALID_REP_ID;
	}

	if (report->has_size && (report->len < 3)) {
		return BTSRV_HID_HANDSHAKE_RSP_ERR_INVALID_PARAM;
	}

	report_id = report->data[0];
	info = bt_manager_hid_find(report_id, report->report_type);
	if (info) {
		if (!get) {
			if ((report->report_type == BTSRV_HID_REP_TYPE_INPUT) &&
				(report_id == 1) && (report->len < 3)) {
				return BTSRV_HID_HANDSHAKE_RSP_ERR_INVALID_PARAM;
			}
		}

		return BTSRV_HID_HANDSHAKE_RSP_SUCCESS;
	} else {
		if (bt_manager_hid_find_id(report_id)) {
			return BTSRV_HID_HANDSHAKE_RSP_ERR_INVALID_PARAM;
		} else {
			return BTSRV_HID_HANDSHAKE_RSP_ERR_INVALID_REP_ID;
		}
	}
}

static void bt_manager_hid_report_opt(uint16_t hdl, void *packet, int size, bool get)
{
	struct bt_hid_report *report;
	struct report_info *info;
	int report_id;
	uint8_t ret;

	os_mutex_lock(&hid_mgr_lock, OS_FOREVER);
	ret = bt_manager_hid_check_report(hdl, packet, size, get);
	if (ret) {
		btif_hid_send_rsp(hdl, ret);
		os_mutex_unlock(&hid_mgr_lock);
		return;
	}

	report = (struct bt_hid_report*)packet;
	report_id = report->data[0];
	info = bt_manager_hid_find(report_id, report->report_type);
	if (info) {
		if (get) {
			if (report->has_size) {
				info->max_len = *(uint16_t*)&(report->data[1]);
			} else {
				info->max_len = info->data_len;
			}
			bt_manager_hid_send_ctrl_data(hdl, info->type, info->last_report, info->max_len);
		} else {
			memcpy(info->last_report, report->data, report->len);
			btif_hid_send_rsp(hdl, BTSRV_HID_HANDSHAKE_RSP_SUCCESS);
		}
	} else {
		btif_hid_send_rsp(hdl, BTSRV_HID_HANDSHAKE_RSP_ERR_INVALID_REP_ID);
	}

	os_mutex_unlock(&hid_mgr_lock);
}

static void bt_manager_hid_status_reset(uint16_t hdl)
{
	struct bt_mgr_dev_info *info;

	SYS_LOG_INF("");
	os_mutex_lock(&hid_mgr_lock, OS_FOREVER);
	info = bt_mgr_find_dev_info_by_hdl(hdl);
	if (!info) {
		os_mutex_unlock(&hid_mgr_lock);
		return;
	}

	info->hid_state = HID_STATE_IDLE;
	os_mutex_unlock(&hid_mgr_lock);

	return;
}

static void bt_manager_hid_callback(uint16_t hdl, btsrv_hid_event_e event, void *packet, int size)
{
	switch(event){
	case BTSRV_HID_GET_REPORT:
		SYS_LOG_INF("GET_REPORT");
		bt_manager_hid_report_opt(hdl, packet, size, true);
		break;
	case BTSRV_HID_SET_REPORT:
		SYS_LOG_INF("SET_REPORT");
		bt_manager_hid_report_opt(hdl, packet, size, false);
		break;
	case BTSRV_HID_GET_PROTOCOL:
		SYS_LOG_INF("GET_PROTOCOL");
		btif_hid_send_rsp(hdl, BTSRV_HID_HANDSHAKE_RSP_ERR_UNSUPPORTED_REQ);
		break;
	case BTSRV_HID_SET_PROTOCOL:
		SYS_LOG_INF("SET_PROTOCOL");
		btif_hid_send_rsp(hdl, BTSRV_HID_HANDSHAKE_RSP_ERR_UNSUPPORTED_REQ);
		break;
	case BTSRV_HID_INTR_DATA:
		SYS_LOG_INF("DATA");
		break;
	case BTSRV_HID_UNPLUG:
		SYS_LOG_INF("UNPLUG");
		break;
	case BTSRV_HID_SUSPEND:
		SYS_LOG_INF("SUSPEND");
		break;
	case BTSRV_HID_EXIT_SUSPEND:
		SYS_LOG_INF("EXIT SUSPEND");
		break;
	case BTSRV_HID_DISCONNECTED:
		SYS_LOG_INF("DISCONNECTED");
		bt_manager_hid_status_reset(hdl);
		break;
	default:
		break;
	}
}

static void btmgr_hid_send_hid_key(uint16_t hdl, uint8_t report_id)
{
	uint8_t tmp_data[2];

	SYS_LOG_INF("Hid report id 0x%x", report_id);

	memset(tmp_data, 0, sizeof(tmp_data));
	/* report id */
	tmp_data[0] = report_id;

	/* push */
	tmp_data[1] = 0x01<<1;
	bt_manager_hid_send_intr_data(hdl, BTSRV_HID_REP_TYPE_INPUT, tmp_data, 2);
	/* release */
	tmp_data[1] = 0x00;
	bt_manager_hid_send_intr_data(hdl, BTSRV_HID_REP_TYPE_INPUT, tmp_data, 2);
}

void bt_manager_hid_delay_work(os_work *work)
{
	os_mutex_lock(&hid_mgr_lock, OS_FOREVER);
	struct bt_mgr_dev_info *info = CONTAINER_OF(work, struct bt_mgr_dev_info, hid_delay_work);

	SYS_LOG_INF("Hid state %d connected %d", info->hid_state, info->hid_connected);

	if (bt_manager_config_pts_test()) {
		os_mutex_unlock(&hid_mgr_lock);
		return;
	}

	if (!info->hid_connected) {
		if (info->hid_state == HID_STATE_WAIT_CONNECTED) {
			/* Hid connect timeout */
			info->hid_state = HID_STATE_IDLE;
		}
		os_mutex_unlock(&hid_mgr_lock);
		return;
	}

	switch (info->hid_state) {
	case HID_STATE_IDLE:
	case HID_STATE_FORCE_CONNECT:
		info->hid_state = HID_STATE_IDLE;
		break;
	case HID_STATE_WAIT_CONNECTED:
		info->hid_state = HID_STATE_IDLE;
		break;
	case HID_STATE_WAIT_DELAY_OPT:
		btmgr_hid_send_hid_key(info->hdl, info->hid_report_id);
		info->hid_state = HID_STATE_WAIT_DISCONNECT;
		os_delayed_work_submit(&info->hid_delay_work, mgr_hid_info.disconnect_delay);
		break;
	case HID_STATE_WAIT_DISCONNECT:
		btif_hid_disconnect(info->hdl);
		info->hid_state = HID_STATE_IDLE;
		break;
	}

	os_mutex_unlock(&hid_mgr_lock);
}

void bt_manager_hid_connected_check_work(uint16_t hdl)
{
	os_mutex_lock(&hid_mgr_lock, OS_FOREVER);
	struct bt_mgr_dev_info *info = bt_mgr_find_dev_info_by_hdl(hdl);

	if (!info) {
		SYS_LOG_ERR("is not connected?");
		os_mutex_unlock(&hid_mgr_lock);
		return;
	}

	if (info->hid_state == HID_STATE_FORCE_CONNECT) {
		info->hid_state = HID_STATE_IDLE;
		os_delayed_work_cancel(&info->hid_delay_work);
		os_mutex_unlock(&hid_mgr_lock);
		return;
	}

	if (info->hid_state == HID_STATE_WAIT_CONNECTED) {
		info->hid_state = HID_STATE_WAIT_DELAY_OPT;
		os_delayed_work_submit(&info->hid_delay_work, mgr_hid_info.opt_delay);
	} else {
		info->hid_state = HID_STATE_WAIT_DISCONNECT;
		os_delayed_work_submit(&info->hid_delay_work, mgr_hid_info.disconnect_delay);
	}

	os_mutex_unlock(&hid_mgr_lock);
}

static int bt_manager_hid_send_report_id(uint8_t report_id)
{
	struct bt_mgr_dev_info *info;
	uint16_t hdl;

	SYS_LOG_INF("");
	//if (bt_manager_tws_get_dev_role() == BTSRV_TWS_SLAVE) {
	//	bt_manager_tws_send_message(TWS_BT_MGR_EVENT, TWS_EVENT_BT_HID_SEND_KEY, &report_id, sizeof(report_id));
	//	return 0;
	//}

	os_mutex_lock(&hid_mgr_lock, OS_FOREVER);
	hdl = btif_br_get_active_phone_hdl();
	if (!hdl) {
		os_mutex_unlock(&hid_mgr_lock);
		return 0;
	}

	info = bt_mgr_find_dev_info_by_hdl(hdl);
	if (!info) {
		os_mutex_unlock(&hid_mgr_lock);
		return 0;
	}

	if (info->hid_connected) {
		btmgr_hid_send_hid_key(hdl, report_id);
		info->hid_state = HID_STATE_WAIT_DISCONNECT;
		os_delayed_work_submit(&info->hid_delay_work, mgr_hid_info.disconnect_delay);
		//bt_manager_sys_event_notify(SYS_EVENT_HID_PHOTO_SHOT);
	} else if (info->hid_state == HID_STATE_IDLE) {
		btif_hid_connect(info->hdl);
		info->hid_report_id = report_id;
		info->hid_state = HID_STATE_WAIT_CONNECTED;
		os_delayed_work_submit(&info->hid_delay_work, HID_CONNECTED_TIMEOUT);
	}

	os_mutex_unlock(&hid_mgr_lock);
	return 0;
}

int bt_manager_hid_take_photo(void)
{
	return bt_manager_hid_send_report_id(0x02);		/* 0x09, 0xe9, //USAGE (volume up) */
}

int bt_manager_hid_send_key(uint8_t report_id)
{
	return bt_manager_hid_send_report_id(report_id);
}

int bt_manager_hid_profile_start(uint16_t disconnect_delay, uint16_t opt_delay)
{
	mgr_hid_info.disconnect_delay = disconnect_delay;
	mgr_hid_info.opt_delay = opt_delay;

	return btif_hid_start(&bt_manager_hid_callback);
}

int bt_manager_hid_profile_stop(void)
{
	return btif_hid_stop();
}

int bt_manager_hid_reconnect(void)
{
	struct bt_mgr_dev_info *info;
	uint16_t hdl;

	SYS_LOG_INF("");

	os_mutex_lock(&hid_mgr_lock, OS_FOREVER);
	hdl = btif_br_get_active_phone_hdl();
	if (!hdl) {
		os_mutex_unlock(&hid_mgr_lock);
		return 0;
	}

	info = bt_mgr_find_dev_info_by_hdl(hdl);
	if (!info) {
		os_mutex_unlock(&hid_mgr_lock);
		return 0;
	}

	if (info->hid_connected) {
		SYS_LOG_INF("hid connected.");
		os_mutex_unlock(&hid_mgr_lock);
		return 0;
	} else if (info->hid_state == HID_STATE_IDLE) {
		btif_hid_connect(info->hdl);
		info->hid_state = HID_STATE_FORCE_CONNECT;
		os_delayed_work_submit(&info->hid_delay_work, HID_CONNECTED_TIMEOUT);
	}

	os_mutex_unlock(&hid_mgr_lock);
	return 0;
}

int bt_manager_hid_disconnect(void)
{
	struct bt_mgr_dev_info *info;
	uint16_t hdl;

	SYS_LOG_INF("");

	os_mutex_lock(&hid_mgr_lock, OS_FOREVER);
	hdl = btif_br_get_active_phone_hdl();
	if (!hdl) {
		return 0;
	}

	info = bt_mgr_find_dev_info_by_hdl(hdl);
	if (!info) {
		os_mutex_unlock(&hid_mgr_lock);
		return 0;
	}

	if (!info->hid_connected) {
		SYS_LOG_INF("hid disconnected.");
		os_mutex_unlock(&hid_mgr_lock);
		return 0;
	} else if (info->hid_state == HID_STATE_IDLE) {
		btif_hid_disconnect(info->hdl);
	}

	os_mutex_unlock(&hid_mgr_lock);
	return 0;
}

