/*
 * Copyright (c) 2019 Actions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB STUB descriptors (Device, Configuration, Interface ...)
 */
#ifndef __USB_STUB_DESC_H__
#define __USB_STUB_DESC_H__

#include <usb/usb_common.h>

/* Total length of configuration descriptor: 9+9+7+7=32Byte (0x0020) */
#define STUB_CONF_SIZE	(USB_CONFIGURATION_DESC_SIZE + \
	USB_INTERFACE_DESC_SIZE + (USB_ENDPOINT_DESC_SIZE * 2))

/* Misc. macros */
#define LOW_BYTE(x)  ((x) & 0xFF)
#define HIGH_BYTE(x) ((x) >> 8)

#define USB_DEVICE_VID	0x10D6
#define USB_DEVICE_PID	0xFF00

static const u8_t usb_stub_fs_descriptor[] = {
	/* Device descriptor */
	USB_DEVICE_DESC_SIZE,		/* Descriptor size */
	USB_DEVICE_DESC,		/* Descriptor type */
	LOW_BYTE(USB_2_0),		/* USB release version 2.0 */
	HIGH_BYTE(USB_2_0),
	0x00,				/* class code */
	0x00,				/* sub-class code */
	0x00,				/* protocol code */
	MAX_PACKET_SIZE0,		/* max packet size = 64byte */
	/* Vendor Id */
	LOW_BYTE(USB_DEVICE_VID),
	HIGH_BYTE(USB_DEVICE_VID),
	/* Product Id */
	LOW_BYTE(USB_DEVICE_PID),
	HIGH_BYTE(USB_DEVICE_PID),
	/* Device Release Number */
	LOW_BYTE(BCDDEVICE_RELNUM),
	HIGH_BYTE(BCDDEVICE_RELNUM),
	0x01,	/* index of string descriptor of manufacturer */
	0x02,	/* index of string descriptor of product */
	0x03,	/* index of string descriptor of serial number */
	0x01,	/* number of possible configuration */

	/* 1. Configuration1_descriptor */
	USB_CONFIGURATION_DESC_SIZE,		/* Descriptor size */
	USB_CONFIGURATION_DESC,			/* Descriptor type */
	/* Total length of Configuration descriptor Set */
	LOW_BYTE(STUB_CONF_SIZE),
	HIGH_BYTE(STUB_CONF_SIZE),
	0x01,	/* number of interface */
	0x01,	/* configuration value */
	0x00,	/* configuration string index */
	/* attribute (bus powered, remote wakeup disable) */
	USB_CONFIGURATION_ATTRIBUTES,
	/* max power (500mA),96h(300mA) */
	0x96,

	/* 2.Interface0_Descriptor */
	USB_INTERFACE_DESC_SIZE,
	USB_INTERFACE_DESC,
	0x00,		/* interface number */
	0x00,		/* alternative setting */
	0x02,		/* number of endpoint */
	0xff,		/* interface class code */
	0xff,		/* interface sub-class code */
	0xff,		/* interface protocol code */
	0x00,

	/* 2.1 Endpoint1  Data_IN */
	USB_ENDPOINT_DESC_SIZE,
	USB_ENDPOINT_DESC,
	/* bEndpointAddress:-> Direction: in - EndpointID */
	CONFIG_STUB_IN_EP_ADDR,
	/* bmAttributes:-> Bulk Transfer Type */
	USB_DC_EP_BULK,
	LOW_BYTE(CONFIG_STUB_EP_MPS),		/* wMaxPacketSize */
	HIGH_BYTE(CONFIG_STUB_EP_MPS),
	0x00,					/* bInterval */

	/* 2.2 Endpoint2 Data_OUT */
	USB_ENDPOINT_DESC_SIZE,
	USB_ENDPOINT_DESC,
	/* bEndpointAddress:-> Direction: Out - EndpointID */
	CONFIG_STUB_OUT_EP_ADDR,
	/* bmAttributes:-> Bulk Transfer Type */
	USB_DC_EP_BULK,
	LOW_BYTE(CONFIG_STUB_EP_MPS),		/* wMaxPacketSize */
	HIGH_BYTE(CONFIG_STUB_EP_MPS),
	0x00,					/* bInterval */
};

static const u8_t usb_stub_hs_descriptor[] = {
	/* Device descriptor */
	USB_DEVICE_DESC_SIZE,		/* Descriptor size */
	USB_DEVICE_DESC,		/* Descriptor type */
	LOW_BYTE(USB_2_0),		/* USB release version 2.0 */
	HIGH_BYTE(USB_2_0),
	0x00,				/* class code */
	0x00,				/* sub-class code */
	0x00,				/* protocol code */
	MAX_PACKET_SIZE0,		/* max packet size = 64byte */
	/* Vendor Id */
	LOW_BYTE(USB_DEVICE_VID),
	HIGH_BYTE(USB_DEVICE_VID),
	/* Product Id */
	LOW_BYTE(USB_DEVICE_PID),
	HIGH_BYTE(USB_DEVICE_PID),
	/* Device Release Number */
	LOW_BYTE(BCDDEVICE_RELNUM),
	HIGH_BYTE(BCDDEVICE_RELNUM),
	0x01,	/* index of string descriptor of manufacturer */
	0x02,	/* index of string descriptor of product */
	0x03,	/* index of string descriptor of serial number */
	0x01,	/* number of possible configuration */

	/* 1. Configuration1_descriptor */
	USB_CONFIGURATION_DESC_SIZE,		/* Descriptor size */
	USB_CONFIGURATION_DESC,			/* Descriptor type */
	/* Total length of Configuration descriptor Set */
	LOW_BYTE(STUB_CONF_SIZE),
	HIGH_BYTE(STUB_CONF_SIZE),
	0x01,	/* number of interface */
	0x01,	/* configuration value */
	0x00,	/* configuration string index */
	/* attribute (bus powered, remote wakeup disable) */
	USB_CONFIGURATION_ATTRIBUTES,
	/* max power (500mA),96h(300mA) */
	0x96,

	/* 2.Interface0_Descriptor */
	USB_INTERFACE_DESC_SIZE,
	USB_INTERFACE_DESC,
	0x00,		/* interface number */
	0x00,		/* alternative setting */
	0x02,		/* number of endpoint */
	0xff,		/* interface class code */
	0xff,		/* interface sub-class code */
	0xff,		/* interface protocol code */
	0x00,

	/* 2.1 Endpoint1  Data_IN */
	USB_ENDPOINT_DESC_SIZE,
	USB_ENDPOINT_DESC,
	/* bEndpointAddress:-> Direction: in - EndpointID */
	CONFIG_STUB_IN_EP_ADDR,
	/* bmAttributes:-> Bulk Transfer Type */
	USB_DC_EP_BULK,
	LOW_BYTE(USB_MAX_HS_BULK_MPS),		/* wMaxPacketSize */
	HIGH_BYTE(USB_MAX_HS_BULK_MPS),
	0x00,					/* bInterval */

	/* 2.2 Endpoint2 Data_OUT */
	USB_ENDPOINT_DESC_SIZE,
	USB_ENDPOINT_DESC,
	/* bEndpointAddress:-> Direction: Out - EndpointID */
	CONFIG_STUB_OUT_EP_ADDR,
	/* bmAttributes:-> Bulk Transfer Type */
	USB_DC_EP_BULK,
	LOW_BYTE(USB_MAX_HS_BULK_MPS),		/* wMaxPacketSize */
	HIGH_BYTE(USB_MAX_HS_BULK_MPS),
	0x00,					/* bInterval */
};
#endif	/* __USB_STUB_DESC_H__ */
