/*
 * Copyright (c) 2019 Actions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Mass-Storage Class descriptors (Device, Configuration, Interface ...)
 */
#ifndef __MASS_STORAGE_DESC_H__
#define __MASS_STORAGE_DESC_H__

#include <usb/usb_common.h>

/* Size in bytes of the configuration sent to
 * the Host on GetConfiguration() request
 * For Communication Device: CONF + ITF) + 2 x EP -> 32 bytes
 */
#define MSC_CONF_SIZE   (USB_CONFIGURATION_DESC_SIZE + \
	USB_INTERFACE_DESC_SIZE + (2 * USB_ENDPOINT_DESC_SIZE))

/* Actions USB Mass Storage */
#define USB_DEVICE_VID	0x10D6
#define USB_DEVICE_PID	0xB00B

#define LOW_BYTE(x)  ((x) & 0xFF)
#define HIGH_BYTE(x) ((x) >> 8)

/* high-speed */
#define MSC_HS_BULK_EP_MPS	512

static const u8_t mass_storage_fs_descriptor[] = {
	/* Device descriptor */
	USB_DEVICE_DESC_SIZE,           /* Descriptor size */
	USB_DEVICE_DESC,                /* Descriptor type */
	LOW_BYTE(USB_2_0),
	HIGH_BYTE(USB_2_0),             /* USB version in BCD format */
	0x00,                           /* Class */
	0x00,                           /* SubClass - Interface specific */
	0x00,                           /* Protocol - Interface specific */
	MAX_PACKET_SIZE0,               /* bMaxPacketSize0 */
	LOW_BYTE(USB_DEVICE_VID),
	HIGH_BYTE(USB_DEVICE_VID),      /* Vendor Id */
	LOW_BYTE(USB_DEVICE_PID),
	HIGH_BYTE(USB_DEVICE_PID),      /* Product Id */
	LOW_BYTE(BCDDEVICE_RELNUM),
	HIGH_BYTE(BCDDEVICE_RELNUM),    /* Device Release Number */
	/* Index of Manufacturer String Descriptor */
	0x01,
	/* Index of Product String Descriptor */
	0x02,
	/* Index of Serial Number String Descriptor */
	0x03,
	/* Number of Possible Configuration */
	0x01,

	/* Configuration descriptor */
	USB_CONFIGURATION_DESC_SIZE,    /* Descriptor size */
	USB_CONFIGURATION_DESC,         /* Descriptor type */
	/* Total length in bytes of data returned */
	LOW_BYTE(MSC_CONF_SIZE),
	HIGH_BYTE(MSC_CONF_SIZE),
	0x01,                           /* Number of interfaces */
	0x01,                           /* Configuration value */
	0x00,                           /* Index of the Configuration string */
	USB_CONFIGURATION_ATTRIBUTES,   /* Attributes */
	MAX_LOW_POWER,                  /* Max power consumption */

	/* Interface descriptor */
	USB_INTERFACE_DESC_SIZE,        /* Descriptor size */
	USB_INTERFACE_DESC,             /* Descriptor type */
	0x00,                           /* Interface index */
	0x00,                           /* Alternate setting */
	0x02,                           /* Number of Endpoints */
	MASS_STORAGE_CLASS,             /* Class */
	SCSI_TRANSPARENT_SUBCLASS,      /* SubClass */
	BULK_ONLY_PROTOCOL,             /* Protocol */
	/* Index of the Interface String Descriptor */
	0x00,

	/* Endpoint descriptor */
	USB_ENDPOINT_DESC_SIZE,         /* Descriptor size */
	USB_ENDPOINT_DESC,              /* Descriptor type */
	CONFIG_MASS_STORAGE_IN_EP_ADDR, /* Endpoint address */
	USB_DC_EP_BULK,                 /* Attributes */
	/* Max packet size */
	LOW_BYTE(CONFIG_MASS_STORAGE_BULK_EP_MPS),
	HIGH_BYTE(CONFIG_MASS_STORAGE_BULK_EP_MPS),
	0x00,                           /* Interval */

	/* Endpoint descriptor */
	USB_ENDPOINT_DESC_SIZE,         /* Descriptor size */
	USB_ENDPOINT_DESC,              /* Descriptor type */
	CONFIG_MASS_STORAGE_OUT_EP_ADDR,/* Endpoint address */
	USB_DC_EP_BULK,                 /* Attributes */
	/* Max packet size */
	LOW_BYTE(CONFIG_MASS_STORAGE_BULK_EP_MPS),
	HIGH_BYTE(CONFIG_MASS_STORAGE_BULK_EP_MPS),
	0x00,                           /* Interval */
};

static const u8_t mass_storage_hs_descriptor[] = {
	/* Device descriptor */
	USB_DEVICE_DESC_SIZE,           /* Descriptor size */
	USB_DEVICE_DESC,                /* Descriptor type */
	LOW_BYTE(USB_2_0),
	HIGH_BYTE(USB_2_0),             /* USB version in BCD format */
	0x00,                           /* Class */
	0x00,                           /* SubClass - Interface specific */
	0x00,                           /* Protocol - Interface specific */
	MAX_PACKET_SIZE0,               /* bMaxPacketSize0 */
	LOW_BYTE(USB_DEVICE_VID),
	HIGH_BYTE(USB_DEVICE_VID),      /* Vendor Id */
	LOW_BYTE(USB_DEVICE_PID),
	HIGH_BYTE(USB_DEVICE_PID),      /* Product Id */
	LOW_BYTE(BCDDEVICE_RELNUM),
	HIGH_BYTE(BCDDEVICE_RELNUM),    /* Device Release Number */
	/* Index of Manufacturer String Descriptor */
	0x01,
	/* Index of Product String Descriptor */
	0x02,
	/* Index of Serial Number String Descriptor */
	0x03,
	/* Number of Possible Configuration */
	0x01,

	/* Configuration descriptor */
	USB_CONFIGURATION_DESC_SIZE,    /* Descriptor size */
	USB_CONFIGURATION_DESC,         /* Descriptor type */
	/* Total length in bytes of data returned */
	LOW_BYTE(MSC_CONF_SIZE),
	HIGH_BYTE(MSC_CONF_SIZE),
	0x01,                           /* Number of interfaces */
	0x01,                           /* Configuration value */
	0x00,                           /* Index of the Configuration string */
	USB_CONFIGURATION_ATTRIBUTES,   /* Attributes */
	MAX_LOW_POWER,                  /* Max power consumption */

	/* Interface descriptor */
	USB_INTERFACE_DESC_SIZE,        /* Descriptor size */
	USB_INTERFACE_DESC,             /* Descriptor type */
	0x00,                           /* Interface index */
	0x00,                           /* Alternate setting */
	0x02,                           /* Number of Endpoints */
	MASS_STORAGE_CLASS,             /* Class */
	SCSI_TRANSPARENT_SUBCLASS,      /* SubClass */
	BULK_ONLY_PROTOCOL,             /* Protocol */
	/* Index of the Interface String Descriptor */
	0x00,

	/* Endpoint descriptor */
	USB_ENDPOINT_DESC_SIZE,         /* Descriptor size */
	USB_ENDPOINT_DESC,              /* Descriptor type */
	CONFIG_MASS_STORAGE_IN_EP_ADDR, /* Endpoint address */
	USB_DC_EP_BULK,	                /* Attributes */
	/* Max packet size */
	LOW_BYTE(MSC_HS_BULK_EP_MPS),
	HIGH_BYTE(MSC_HS_BULK_EP_MPS),
	0x00,                           /* Interval */

	/* Endpoint descriptor */
	USB_ENDPOINT_DESC_SIZE,         /* Descriptor size */
	USB_ENDPOINT_DESC,              /* Descriptor type */
	CONFIG_MASS_STORAGE_OUT_EP_ADDR,/* Endpoint address */
	USB_DC_EP_BULK,                 /* Attributes */
	/* Max packet size */
	LOW_BYTE(MSC_HS_BULK_EP_MPS),
	HIGH_BYTE(MSC_HS_BULK_EP_MPS),
	0x00,                           /* Interval */
};
#endif /* __MASS_STORAGE_DESC_H__ */
