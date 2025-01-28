/*
 * Copyright (c) 2020 Actions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
*/
/**
 * @file
 * @brief USB DFU device descriptors (Device, Configuration, Interface ...)
 */

#ifndef __USB_DFU_DEVICE_DESC_H__
#define __USB_DFU_DEVICE_DESC_H__
#include <usb/usb_common.h>
#include <usb/class/usb_dfu.h>

/* Macro define*/
#define LOW_BYTE(x)  ((x) & 0xFF)
#define HIGH_BYTE(x) ((x) >> 8)

#define	SAM_LOW_BYTE(x)	(u8_t)(x)
#define	SAM_MIDDLE_BYTE(x)	(u8_t)(x >> 8)
#define	SAM_HIGH_BYTE(x)	(u8_t)(x >> 16)

#define USB_DFU_MAX_XFER_SIZE		CONFIG_USB_REQUEST_BUFFER_SIZE

static u8_t usb_dfu_fs_descriptor[]=
{
	/* Device descriptor */
	0x12,	/* bLength */
	USB_DEVICE_DESC,	/* bDescriptorType */
	LOW_BYTE(USB_2_0),	/* bcdUSB */
	HIGH_BYTE(USB_2_0),
	0x00,	/* bDeviceClass */
	0x00,	/* bDeviceSubClass */
	0x00,	/* bDeviceProtocol */
	USB_MAX_CTRL_MPS,	/* bMaxPacketSize0 */
	LOW_BYTE(CONFIG_USB_DFU_DEVICE_VID),/* idVendor */
	HIGH_BYTE(CONFIG_USB_DFU_DEVICE_VID),
	LOW_BYTE(CONFIG_USB_DFU_DEVICE_PID),/* idProduct */
	HIGH_BYTE(CONFIG_USB_DFU_DEVICE_PID),
	/* bcdDevice */
	LOW_BYTE(BCDDEVICE_RELNUM),
	HIGH_BYTE(BCDDEVICE_RELNUM),
	0x01,	/* iManufacturer */
	0x02,	/* iProduct */
	0x03,	/* iSerialNumber */
	0x01,	/* bNumConfigurations */

	/* Configuration descriptor */
	0x09,	/* bLength */
	USB_CONFIGURATION_DESC,	/* bDescriptorType */
	LOW_BYTE(0x001b),	/* wTotalLength */
	HIGH_BYTE(0x001b),
	0x01,	/* bNumInterfaces */
	0x01,	/* bConfigurationValue */
	0x00,	/* iConfiguration */
	USB_CONFIGURATION_ATTRIBUTES, /* bmAttributes */
	MAX_LOW_POWER,	/* MaxPower = 100 mA */

	/* Interface descriptor */
	0x09,	/* bLength */
	USB_INTERFACE_DESC,	/* bDescriptorType */
	0x00,	/* bInterfaceNumber */
	0x00,	/* bAlternateSetting */
	0x00,	/* bNumEndpoints */
	DFU_DEVICE_CLASS,	/* bInterfaceClass */
	DFU_SUBCLASS,	/* bInterfaceSubClass */
	DFU_MODE_PROTOCOL,	/* bInterfaceProtocol */
	0x00,	/* iInterface */

	/* Run-Time Functional Descriptor */
	0x09,	/* bLength */
	DFU_FUNC_DESC,	/* bDescriptorType */
	DFU_ATTR_CAN_DNLOAD|DFU_ATTR_CAN_UPLOAD |DFU_ATTR_MANIFESTATION_TOLERANT,	/* bmAttributes */
	LOW_BYTE(CONFIG_USB_DFU_DETACH_TIMEOUT),	/* wDetachTimeOut */
	HIGH_BYTE(CONFIG_USB_DFU_DETACH_TIMEOUT),
	LOW_BYTE(USB_DFU_MAX_XFER_SIZE),	/* wTransferSize */
	HIGH_BYTE(USB_DFU_MAX_XFER_SIZE),
	LOW_BYTE(DFU_VERSION),	/* bcdDFUVersion */
	HIGH_BYTE(DFU_VERSION),
};

static u8_t usb_dfu_hs_descriptor[]=
{
	/* Device descriptor */
	0x12,	/* bLength */
	USB_DEVICE_DESC,	/* bDescriptorType */
	LOW_BYTE(USB_2_0),	/* bcdUSB */
	HIGH_BYTE(USB_2_0),
	0x00,	/* bDeviceClass */
	0x00,	/* bDeviceSubClass */
	0x00,	/* bDeviceProtocol */
	USB_MAX_CTRL_MPS,	/* bMaxPacketSize0 */
	LOW_BYTE(CONFIG_USB_DFU_DEVICE_VID),/* idVendor */
	HIGH_BYTE(CONFIG_USB_DFU_DEVICE_VID),
	LOW_BYTE(CONFIG_USB_DFU_DEVICE_PID),/* idProduct */
	HIGH_BYTE(CONFIG_USB_DFU_DEVICE_PID),
	/* bcdDevice */
	LOW_BYTE(BCDDEVICE_RELNUM),
	HIGH_BYTE(BCDDEVICE_RELNUM),
	0x01,	/* iManufacturer */
	0x02,	/* iProduct */
	0x03,	/* iSerialNumber */
	0x01,	/* bNumConfigurations */

	/* Configuration descriptor */
	0x09,	/* bLength */
	USB_CONFIGURATION_DESC,	/* bDescriptorType */
	LOW_BYTE(0x001b),	/* wTotalLength */
	HIGH_BYTE(0x001b),
	0x01,	/* bNumInterfaces */
	0x01,	/* bConfigurationValue */
	0x00,	/* iConfiguration */
	USB_CONFIGURATION_ATTRIBUTES, /* bmAttributes */
	MAX_LOW_POWER,	/* MaxPower = 100 mA */

	/* Interface descriptor */
	0x09,	/* bLength */
	USB_INTERFACE_DESC,	/* bDescriptorType */
	0x00,	/* bInterfaceNumber */
	0x00,	/* bAlternateSetting */
	0x00,	/* bNumEndpoints */
	DFU_DEVICE_CLASS,	/* bInterfaceClass */
	DFU_SUBCLASS,	/* bInterfaceSubClass */
	DFU_MODE_PROTOCOL,	/* bInterfaceProtocol */
	0x00,	/* iInterface */

	/* Run-Time Functional Descriptor */
	0x09,	/* bLength */
	DFU_FUNC_DESC,	/* bDescriptorType */
	DFU_ATTR_CAN_DNLOAD|DFU_ATTR_CAN_UPLOAD |DFU_ATTR_MANIFESTATION_TOLERANT,	/* bmAttributes */
	LOW_BYTE(CONFIG_USB_DFU_DETACH_TIMEOUT),	/* wDetachTimeOut */
	HIGH_BYTE(CONFIG_USB_DFU_DETACH_TIMEOUT),
	LOW_BYTE(USB_DFU_MAX_XFER_SIZE),	/* wTransferSize */
	HIGH_BYTE(USB_DFU_MAX_XFER_SIZE),
	LOW_BYTE(DFU_VERSION),	/* bcdDFUVersion */
	HIGH_BYTE(DFU_VERSION),
};

#endif /* __USB_DFU_DEVICE_DESC_H__*/