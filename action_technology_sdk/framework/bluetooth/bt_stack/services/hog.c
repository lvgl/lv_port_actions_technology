/** @file
 *  @brief HoG Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include "errno.h"
#include <sys/printk.h>
#include <zephyr.h>

#include <acts_bluetooth/bluetooth.h>
#include <acts_bluetooth/hci.h>
#include <acts_bluetooth/conn.h>
#include <acts_bluetooth/uuid.h>
#include <acts_bluetooth/gatt.h>
#include <acts_bluetooth/services/hog.h>

#define LOG_LEVEL CONFIG_BT_BAS_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(hog);

#define HIDAPP_REMOTE_REPORT_ID				1
#define HIDAPP_VOICE_REPORT_ID				2
#define HIDAPP_MOUSE_REPORT_ID				3

/*! HID Report Type/ID and attribute handle map */
static const struct hid_report_id_map_t hid_app_report_id_set[] = {
	/* type                       ID                            handle */
	{HID_REPORT_TYPE_INPUT,       HIDAPP_REMOTE_REPORT_ID,      HID_INPUT_REPORT_1_HDL},     /* Remote Input Report */
#if 0 //def OLD_HOG
#if CONFIG_HID_MOUSE_USAGE
	{HID_REPORT_TYPE_INPUT,       HIDAPP_VOICE_REPORT_ID,       HID_INPUT_REPORT_2_HDL},     /* Keyboard Input Report */
	{HID_REPORT_TYPE_OUTPUT,      HIDAPP_VOICE_REPORT_ID,       HID_OUTPUT_REPORT_HDL},      /* Keyboard Output Report */
	{HID_REPORT_TYPE_FEATURE,     HIDAPP_VOICE_REPORT_ID,       HID_FEATURE_REPORT_HDL},     /* Keyboard Feature Report */
	{HID_REPORT_TYPE_INPUT,       HIDAPP_MOUSE_REPORT_ID,       HID_INPUT_REPORT_3_HDL},     /* Mouse Input Report */
#endif
	{HID_REPORT_TYPE_INPUT,       HID_KEYBOARD_BOOT_ID,         HID_KEYBOARD_BOOT_IN_HDL},   /* Boot Keyboard Input Report */
	{HID_REPORT_TYPE_OUTPUT,      HID_KEYBOARD_BOOT_ID,         HID_KEYBOARD_BOOT_OUT_HDL},  /* Boot Keyboard Output Report */
#if CONFIG_HID_MOUSE_USAGE
	{HID_REPORT_TYPE_INPUT,       HID_MOUSE_BOOT_ID,            HID_MOUSE_BOOT_IN_HDL},      /* Boot Mouse Input Report */
#endif
#endif
};

/*! HID control block */
struct hid_cb_t {
	const struct hid_config_t *p_config;
};

struct hid_cb_t hid_cb;

enum {
	HIDS_REMOTE_WAKE = BIT(0),
	HIDS_NORMALLY_CONNECTABLE = BIT(1),
};

struct hids_info {
	uint16_t version; /* version number of base USB HID Specification */
	uint8_t code; /* country HID Device hardware is localized for. */
	uint8_t flags;
} __packed;

struct hids_report {
	uint8_t id; /* report id */
	uint8_t type; /* report type */
} __packed;

/* HID Info Value: HID Spec version, country code, flags */
const struct hids_info hid_info_val = {
	.version = HID_VERSION,
	.code = 0x00,
	.flags = HIDS_REMOTE_WAKE | HIDS_NORMALLY_CONNECTABLE,
};

const uint8_t hid_report_map[] = {
	0x05, 0x0c, /*	Usage Page (Consumer Devices) */
	0x09, 0x01, /*	Usage (Consumer Control) */
	0xa1, 0x01, /*	Collection (Application) */
	0x85, HIDAPP_REMOTE_REPORT_ID, /*	report ID (0x01) */
#if 0 //def OLD_HOG
	0x19, 0x00, /*	USAGE_MINIMUM (0) */
	0x2a, 0x9c, 0x02, /*	USAGE_MINIMUM (0x29c) */
	0x15, 0x00, /*	Logical Minimum (0) */
	0x26, 0x9c, 0x02, /*	Logical Maximum (0x29c) */
	0x95, 0x01, /*	Report Count (1) */
	0x75, 0x10, /*	Report Size (16) */
	0x81, 0x00, /*	Input (Data, array, Absolute) */
	0x09, 0x02, /*	Usage (Numeric Key Pad ) */
	0xa1, 0x02, /*	Collection (Application) */
	0x05, 0x09, /*	Usage Page (button) */
	0x19, 0x01, /*	Usage Minimum (1) */
	0x29, 0x0a, /*	Usage Maximum (10) */
	0x15, 0x01, /*	Logical Minimum (1) */
	0x25, 0x0a, /*	Logical Maximum (10) */
	0x95, 0x01, /*	Report Count (1) */
	0x75, 0x08, /*	Report Size (8)  */
	0x81, 0x40, /*	Input (Data, Variable, Relative) */
	0xc0,    /*	End Collection */
	0xc0,    /*	End Collection */
#else
	0x15, 0x00, /*	Logical Minimum (0) */
	0x25, 0x01, /* LOGICAL_MAXIMUM (1) */
	0x75, 0x01, /*	Report Size (1) */
	0x95, 0x08, /*	Report Count (8) */
	0x09, 0xB5,/*  (Scan Next Track) */
	0x09, 0xB6,/*  (Scan Previous Track) */
	0x09, 0xB7,/*  (Stop) */
	0x09, 0x30,/*  (Power) */
	0x09, 0xCD,/*  (Play/Pause) */
	0x09, 0xE2,/*  (Mute)  */
	0x09, 0xE9,/*  (Volume Increment) */
	0x09, 0xEA,/*  (Volume Decrement) */
	0x81, 0x02,
	0xc0,	 /* End Collection */
#endif
#if CONFIG_HID_MOUSE_USAGE
	0x05, 0x01, /* Usage Page (Generic Desktop) */
	0x09, 0x02, /* USAGE (Mouse) */
	0xa1, 0x01, /* COLLECTION (Application) */
	0x09, 0x01, /* USAGE (Pointer) */
	0xa1, 0x00, /* COLLECTION (Physical) */
	0x85, HIDAPP_MOUSE_REPORT_ID, /* report ID (HIDAPP_MOUSE_REPORT_ID) */
	0x05, 0x09, /* USAGE_PAGE (Button) */
	0x19, 0x01, /* USAGE_MINIMUM (Button 1) */
	0x29, 0x05, /* USAGE_MAXIMUM (Button 5) */
	0x15, 0x00, /* LOGICAL_MINIMUM (0) */
	0x25, 0x01, /* LOGICAL_MAXIMUM (1) */
	0x75, 0x01, /* REPORT_SIZE (1) */
	0x95, 0x05, /* REPORT_COUNT (5) */
	0x81, 0x02, /* Input (Data, Variable, Absolute) */
	0x75, 0x03, /* REPORT_SIZE (3) */
	0x95, 0x01, /* REPORT_COUNT (1) */
	0x81, 0x03, /* INPUT (Const, Variable, Absolute) */
	0x05, 0x01, /* USAGE_PAGE (Generic Desktop) */
	0x09, 0x30, /* USAGE (X) */
	0x09, 0x31, /* USAGE (Y) */
	0x09, 0x38, /* USAGE (wheel)*/
	0x15, 0x80, /* LOGICAL_MINIMUM (-127) */
	0x25, 0x7f, /* LOGICAL_MAXIMUM (127) */
	0x75, 0x08, /* REPORT_SIZE (8) */
	0x95, 0x03, /* REPORT_COUNT (3) */
	0x81, 0x06, /* INPUT (Data, Variable, Relative) */
	0xc0, /* End Collection (Physical) */
	0xc0, /* End Collection (Application) */
	0x06, 0x01, 0xff,
	0x09, 0x01, /*	Usage (Consumer Control) */
	0xa1, 0x02, /*	COLLECTION () */
	0x85, HIDAPP_VOICE_REPORT_ID, /*	report ID (HIDAPP_KEYBOARD_REPORT_ID) */
	0x09, 0x14,
	0x75, 0x08, /*	Report Size (8) */
	0x95, 0x14, /*	Report Count (20) */
	0x15, 0x80, /*	Logical Minimum (0x80) */
	0x25, 0x7f, /*	Logical Minimum (0x7f) */
	0x81, 0x22, /*	Input () */
	0x85, 0x04, /*	report ID () */
	0x09, 0x04, /*	USAGE () */
	0x75, 0x08, /*	Report Size (8) */
	0x95, 0x01, /*	Report Count (1) */
	0x91, 0x02, /*	Output */
	0xc0,
#endif
};

/* HID Control Point Value */
uint8_t hid_cp_val;

/* HID Input Report Reference - ID, Type */
const struct hids_report hid_val_irep1_id_map = {
	.id = 0x01,
	.type = HID_REPORT_TYPE_INPUT,
};

#if 0 //def OLD_HOG
#if CONFIG_HID_MOUSE_USAGE
/* HID Input Report Reference - ID, Type */
const struct hids_report hid_val_irep2_id_map = {
	.id = 0x02,
	.type = HID_REPORT_TYPE_INPUT,
};

/* HID Input Report Reference - ID, Type */
const struct hids_report hid_val_irep3_id_map = {
	.id = 0x03,
	.type = HID_REPORT_TYPE_INPUT,
};
#endif
#endif

/* HID Output Report Reference - ID, Type */
const struct hids_report hid_val_orep_id_map = {
	.id = 0x0a,
	.type = HID_REPORT_TYPE_OUTPUT,
};

/* HID Feature Report Reference - ID, Type */
const struct hids_report hid_val_frep_id_map = {
	.id = 0x00,
	.type = HID_REPORT_TYPE_FEATURE,
};

/* HID Protocol Mode Value */
static uint8_t hid_pm_val = HID_PROTOCOL_MODE_REPORT;
static const uint16_t hid_len_pm_val = sizeof(hid_pm_val);

static uint8_t bt_attr_get_id(const struct bt_gatt_attr *attr);

static ssize_t read_info(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
				 sizeof(struct hids_info));
}

static ssize_t read_report_map(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, hid_report_map,
				 sizeof(hid_report_map));
}

static ssize_t read_ext_report(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	uint8_t hid_ext_report[] = {((uint8_t) (BT_UUID_BAS_BATTERY_LEVEL_VAL)), ((uint8_t)((BT_UUID_BAS_BATTERY_LEVEL_VAL) >> 8))};
	uint16_t hid_len_ext_report = sizeof(hid_ext_report);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, hid_ext_report,
				 hid_len_ext_report);
}

static ssize_t write_ctrl_point(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags)
{
	if (offset + len > 1)
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);

	return len;
}

static ssize_t read_report_reference(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
				 sizeof(struct hids_report));
}

static ssize_t read_report_value(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	if (offset > HID_MAX_REPORT_LEN)
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);

	return len;
}

static ssize_t write_report_value(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags)
{
	if (offset + len > HID_MAX_REPORT_LEN)
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);

	return len;
}

static void hid_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	uint16_t handle = bt_attr_get_id(attr);

	hid_set_ccc_table_value(handle, value);
	LOG_INF("handle: %d, ccc_value : %d", handle, value);
}

static ssize_t read_protocol_mode(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
				 hid_len_pm_val);
}


static ssize_t write_protocol_mode(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags)
{
	if (offset + len > 1)
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);

	return len;
}

/* HID Service Declaration */
BT_GATT_SERVICE_DEFINE(hog_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HIDS),
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_INFO,
					BT_GATT_CHRC_READ, BT_GATT_PERM_READ_ENCRYPT,
					read_info, NULL, (void *)&hid_info_val),

	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT_MAP,
					BT_GATT_CHRC_READ, BT_GATT_PERM_READ_ENCRYPT,
					read_report_map, NULL, NULL),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_EXT_REPORT, BT_GATT_PERM_READ_ENCRYPT,
			   read_ext_report, NULL, NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_CTRL_POINT,
					BT_GATT_CHRC_WRITE_WITHOUT_RESP,
					BT_GATT_PERM_WRITE_ENCRYPT,
					NULL, write_ctrl_point, &hid_cp_val),

#if 0 //def OLD_HOG
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_BOOT_KEYBOARD_IN,
					BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
					BT_GATT_PERM_READ_ENCRYPT,
					read_report_value, NULL, NULL),
	BT_GATT_CCC(hid_ccc_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_BOOT_KEYBOARD_OUT,
					BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_WRITE,
					BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT,
					read_report_value, write_report_value, NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_BOOT_MOUSE_IN,
					BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
					BT_GATT_PERM_READ_ENCRYPT,
					read_report_value, NULL, NULL),
	BT_GATT_CCC(hid_ccc_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
#endif

	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
					BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
					BT_GATT_PERM_READ_ENCRYPT,
					read_report_value, NULL, NULL),
	BT_GATT_CCC(hid_ccc_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ_ENCRYPT,
			   read_report_reference, NULL, (void *)&hid_val_irep1_id_map),

#if 0 //def OLD_HOG
#if CONFIG_HID_MOUSE_USAGE
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
					BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
					BT_GATT_PERM_READ_ENCRYPT,
					read_report_value, NULL, NULL),
	BT_GATT_CCC(hid_ccc_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ_ENCRYPT,
			   read_report_reference, NULL, (void *)&hid_val_irep2_id_map),

	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
					BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
					BT_GATT_PERM_READ_ENCRYPT,
					read_report_value, NULL, NULL),
	BT_GATT_CCC(hid_ccc_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ_ENCRYPT,
			   read_report_reference, NULL, (void *)&hid_val_irep3_id_map),
#endif
#endif

	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
					BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_WRITE,
					BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT,
					read_report_value, write_report_value, NULL),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ_ENCRYPT,
			   read_report_reference, NULL, (void *)&hid_val_orep_id_map),

	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
					BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
					BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT,
					read_report_value, write_report_value, NULL),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ_ENCRYPT,
			   read_report_reference, NULL, (void *)&hid_val_frep_id_map),

	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_PROTOCOL_MODE,
					BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
					BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT,
					read_protocol_mode, write_protocol_mode, &hid_pm_val),
);

static uint8_t bt_attr_get_id(const struct bt_gatt_attr *attr)
{
	return attr - attr_hog_svc;
}

void hog_init(void)
{
	LOG_INF("hog init");
}

struct hids_ccc {
	uint16_t handle; /* ccc handle */
	uint8_t value;    /* ccc value */
} __packed;


struct hids_ccc hids_ccc_table[] = {
	/* handle                           value*/
#if 0 //def OLD_HOG
	{HID_KEYBOARD_BOOT_IN_CH_CCC_HDL, 0},
	{HID_MOUSE_BOOT_IN_CH_CCC_HDL, 0},
#endif
	{HID_INPUT_REPORT_1_CH_CCC_HDL, 0},
#if 0 //def OLD_HOG
#if CONFIG_HID_MOUSE_USAGE
	{HID_INPUT_REPORT_2_CH_CCC_HDL, 0},
	{HID_INPUT_REPORT_3_CH_CCC_HDL, 0},
#endif
#endif
};

void hid_set_ccc_table_value(uint16_t handle, uint8_t value)
{
	uint8_t i = 0;

	for (i = 0; i < ARRAY_SIZE(hids_ccc_table); i++) {
		if (hids_ccc_table[i].handle == handle) {
			hids_ccc_table[i].value = value;
			break;
		}
	}
}

uint8_t hid_get_ccc_table_value(uint16_t handle)
{
	uint8_t i = 0;

	for (i = 0; i < ARRAY_SIZE(hids_ccc_table); i++) {
		if (hids_ccc_table[i].handle == handle) {
			return hids_ccc_table[i].value;
		}
	}
	return 0;
}

uint8_t hid_ccc_is_enabled(uint16_t handle)
{
	return (hid_get_ccc_table_value(handle) == BT_GATT_CCC_NOTIFY);
}

uint8_t all_hid_ccc_is_enabled(void)
{
	uint8_t ret = true;
	uint8_t i = 0;

	for (i = 2; i < ARRAY_SIZE(hids_ccc_table); i++) {
		if (hids_ccc_table[i].value != BT_GATT_CCC_NOTIFY) {
			ret = false;
			break;
		}
	}

	return ret;
}

uint16_t hid_get_report_handle(uint8_t type, uint8_t id)
{
	struct hid_report_id_map_t *p_map;
	uint8_t count;
	int8_t i;

	p_map = (struct hid_report_id_map_t *)hid_app_report_id_set;
	count = sizeof(hid_app_report_id_set)/sizeof(struct hid_report_id_map_t);

	for (i = 0; i < count; i++) {
		if (p_map[i].type == type && p_map[i].id == id)
			return p_map[i].handle;
	}

	return 0;
}

uint8_t hid_get_protocol_mode(void)
{
	return hid_pm_val;
}

void hid_set_protocol_mode(uint8_t protocol_mode)
{
	hid_pm_val = protocol_mode;
}

int hid_send_input_report(struct bt_conn *conn, uint8_t report_id, uint16_t len, uint8_t *p_value)
{
	uint16_t handle = hid_get_report_handle(HID_REPORT_TYPE_INPUT, report_id);

	if ((handle != 0) && hid_ccc_is_enabled(handle+1))
		return bt_gatt_notify(conn, &attr_hog_svc[handle], p_value, len);
	else
		return -EIO;
}
