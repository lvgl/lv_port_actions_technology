/*
 * Copyright (c) 2019 Actions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief CDC ACM descriptors (Device, Configuration, Interface ...)
 */
#ifndef __CDC_ACM_DESC_H__
#define __CDC_ACM_DESC_H__

#include <usb/usb_common.h>
#include <usb/class/usb_cdc.h>

/* Size in bytes of the configuration sent to
 * the Host on GetConfiguration() request
 * For Communication Device: CONF + IAD + (2 x ITF) +
 * (3 x EP) + HF + CMF + ACMF + UF -> 75 bytes
 */
#define CDC_CONF_SIZE   (USB_CONFIGURATION_DESC_SIZE + 8 + \
	(2 * USB_INTERFACE_DESC_SIZE) + (3 * USB_ENDPOINT_DESC_SIZE) + 19)

#define USB_DEVICE_VID	0x10D6
#define USB_DEVICE_PID	0xB00A

/* Misc. macros */
#define LOW_BYTE(x)  ((x) & 0xFF)
#define HIGH_BYTE(x) ((x) >> 8)

/* high-speed */
#define HS_BULK_EP_MPS	512
#define HS_INTERRUPT_EP_MPS	CONFIG_CDC_ACM_INTERRUPT_EP_MPS

/* Structure representing the global USB description */
static const u8_t cdc_acm_usb_fs_descriptor[] = {
	/* Device descriptor */
	USB_DEVICE_DESC_SIZE,           /* Descriptor size */
	USB_DEVICE_DESC,                /* Descriptor type */
	LOW_BYTE(USB_2_0),
	HIGH_BYTE(USB_2_0),             /* USB version in BCD format */
	COMMUNICATION_DEVICE_CLASS,     /* Class */
	0x00,                           /* SubClass - Interface specific */
	0x00,                           /* Protocol - Interface specific */
	MAX_PACKET_SIZE0,               /* Max Packet Size */
	/* Vendor Id */
	LOW_BYTE(USB_DEVICE_VID),
	HIGH_BYTE(USB_DEVICE_VID),
	/* Product Id */
	LOW_BYTE(USB_DEVICE_PID),
	HIGH_BYTE(USB_DEVICE_PID),
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
	LOW_BYTE(CDC_CONF_SIZE),
	HIGH_BYTE(CDC_CONF_SIZE),
	0x02,                           /* Number of interfaces */
	0x01,                           /* Configuration value */
	0x00,                           /* Index of the Configuration string */
	USB_CONFIGURATION_ATTRIBUTES,   /* Attributes */
	MAX_LOW_POWER,                  /* Max power consumption */

	/* Association descriptor */
	/* Descriptor size */
	sizeof(struct usb_association_descriptor),
	USB_ASSOCIATION_DESC,           /* Descriptor type */
	0x00,                           /* The first interface number */
	0x02,                           /* Number of interfaces */
	COMMUNICATION_DEVICE_CLASS,     /* Function Class */
	ACM_SUBCLASS,                   /* Function SubClass */
	V25TER_PROTOCOL,                /* Function Protocol */
	/* Index of the Interface String Descriptor */
	0x00,

	/* Interface descriptor */
	USB_INTERFACE_DESC_SIZE,        /* Descriptor size */
	USB_INTERFACE_DESC,             /* Descriptor type */
	0x00,                           /* Interface index */
	0x00,                           /* Alternate setting */
	0x01,                           /* Number of Endpoints */
	COMMUNICATION_DEVICE_CLASS,     /* Class */
	ACM_SUBCLASS,                   /* SubClass */
	V25TER_PROTOCOL,                /* Protocol */
	/* Index of the Interface String Descriptor */
	0x00,

	/* Header Functional Descriptor */
	/* Descriptor size */
	sizeof(struct cdc_header_descriptor),
	CS_INTERFACE,                   /* Descriptor type */
	HEADER_FUNC_DESC,               /* Descriptor SubType */
	LOW_BYTE(USB_1_1),
	HIGH_BYTE(USB_1_1),             /* CDC Device Release Number */

	/* Call Management Functional Descriptor */
	/* Descriptor size */
	sizeof(struct cdc_cm_descriptor),
	CS_INTERFACE,                   /* Descriptor type */
	CALL_MANAGEMENT_FUNC_DESC,      /* Descriptor SubType */
	0x02,                           /* Capabilities */
	0x01,                           /* Data Interface */

	/* ACM Functional Descriptor */
	/* Descriptor size */
	sizeof(struct cdc_acm_descriptor),
	CS_INTERFACE,                   /* Descriptor type */
	ACM_FUNC_DESC,                  /* Descriptor SubType */
	/* Capabilities - Device supports the request combination of:
	 *	Set_Line_Coding,
	 *	Set_Control_Line_State,
	 *	Get_Line_Coding
	 *	and the notification Serial_State
	 */
	0x02,

	/* Union Functional Descriptor */
	/* Descriptor size */
	sizeof(struct cdc_union_descriptor),
	CS_INTERFACE,                   /* Descriptor type */
	UNION_FUNC_DESC,                /* Descriptor SubType */
	0x00,                           /* Master Interface */
	0x01,                           /* Slave Interface */

	/* Endpoint INT */
	USB_ENDPOINT_DESC_SIZE,         /* Descriptor size */
	USB_ENDPOINT_DESC,              /* Descriptor type */
	/* Endpoint address */
	CONFIG_CDC_ACM_INTERRUPT_EP_ADDR,
	USB_DC_EP_INTERRUPT,            /* Attributes */
	/* Max packet size */
	LOW_BYTE(CONFIG_CDC_ACM_INTERRUPT_EP_MPS),
	HIGH_BYTE(CONFIG_CDC_ACM_INTERRUPT_EP_MPS),
	/* Interval */
	CONFIG_CDC_ACM_INTERRUPT_EP_INTERVAL,

	/* Interface descriptor */
	USB_INTERFACE_DESC_SIZE,        /* Descriptor size */
	USB_INTERFACE_DESC,             /* Descriptor type */
	0x01,                           /* Interface index */
	0x00,                           /* Alternate setting */
	0x02,                           /* Number of Endpoints */
	COMMUNICATION_DEVICE_CLASS_DATA,/* Class */
	0x00,                           /* SubClass */
	0x00,                           /* Protocol */
	/* Index of the Interface String Descriptor */
	0x00,

	/* First Endpoint IN */
	USB_ENDPOINT_DESC_SIZE,         /* Descriptor size */
	USB_ENDPOINT_DESC,              /* Descriptor type */
	CONFIG_CDC_ACM_BULK_IN_EP_ADDR, /* Endpoint address */
	USB_DC_EP_BULK,                 /* Attributes */
	/* Max packet size */
	LOW_BYTE(CONFIG_CDC_ACM_BULK_EP_MPS),
	HIGH_BYTE(CONFIG_CDC_ACM_BULK_EP_MPS),
	0x00,                           /* Interval */

	/* Second Endpoint OUT */
	USB_ENDPOINT_DESC_SIZE,         /* Descriptor size */
	USB_ENDPOINT_DESC,              /* Descriptor type */
	CONFIG_CDC_ACM_BULK_OUT_EP_ADDR,/* Endpoint address */
	USB_DC_EP_BULK,                 /* Attributes */
	/* Max packet size */
	LOW_BYTE(CONFIG_CDC_ACM_BULK_EP_MPS),
	HIGH_BYTE(CONFIG_CDC_ACM_BULK_EP_MPS),
	0x00,                           /* Interval */
};

static const u8_t cdc_acm_usb_hs_descriptor[] = {
	/* Device descriptor */
	USB_DEVICE_DESC_SIZE,           /* Descriptor size */
	USB_DEVICE_DESC,                /* Descriptor type */
	LOW_BYTE(USB_2_0),
	HIGH_BYTE(USB_2_0),             /* USB version in BCD format */
	COMMUNICATION_DEVICE_CLASS,     /* Class */
	0x00,                           /* SubClass - Interface specific */
	0x00,                           /* Protocol - Interface specific */
	MAX_PACKET_SIZE0,               /* Max Packet Size */
	/* Vendor Id */
	LOW_BYTE(USB_DEVICE_VID),
	HIGH_BYTE(USB_DEVICE_VID),
	/* Product Id */
	LOW_BYTE(USB_DEVICE_PID),
	HIGH_BYTE(USB_DEVICE_PID),
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
	LOW_BYTE(CDC_CONF_SIZE),
	HIGH_BYTE(CDC_CONF_SIZE),
	0x02,                           /* Number of interfaces */
	0x01,                           /* Configuration value */
	0x00,                           /* Index of the Configuration string */
	USB_CONFIGURATION_ATTRIBUTES,   /* Attributes */
	MAX_LOW_POWER,                  /* Max power consumption */

	/* Association descriptor */
	/* Descriptor size */
	sizeof(struct usb_association_descriptor),
	USB_ASSOCIATION_DESC,           /* Descriptor type */
	0x00,                           /* The first interface number */
	0x02,                           /* Number of interfaces */
	COMMUNICATION_DEVICE_CLASS,     /* Function Class */
	ACM_SUBCLASS,                   /* Function SubClass */
	V25TER_PROTOCOL,                /* Function Protocol */
	/* Index of the Interface String Descriptor */
	0x00,

	/* Interface descriptor */
	USB_INTERFACE_DESC_SIZE,        /* Descriptor size */
	USB_INTERFACE_DESC,             /* Descriptor type */
	0x00,                           /* Interface index */
	0x00,                           /* Alternate setting */
	0x01,                           /* Number of Endpoints */
	COMMUNICATION_DEVICE_CLASS,     /* Class */
	ACM_SUBCLASS,                   /* SubClass */
	V25TER_PROTOCOL,                /* Protocol */
	/* Index of the Interface String Descriptor */
	0x00,

	/* Header Functional Descriptor */
	/* Descriptor size */
	sizeof(struct cdc_header_descriptor),
	CS_INTERFACE,                   /* Descriptor type */
	HEADER_FUNC_DESC,               /* Descriptor SubType */
	LOW_BYTE(USB_1_1),
	HIGH_BYTE(USB_1_1),             /* CDC Device Release Number */

	/* Call Management Functional Descriptor */
	/* Descriptor size */
	sizeof(struct cdc_cm_descriptor),
	CS_INTERFACE,                   /* Descriptor type */
	CALL_MANAGEMENT_FUNC_DESC,      /* Descriptor SubType */
	0x02,                           /* Capabilities */
	0x01,                           /* Data Interface */

	/* ACM Functional Descriptor */
	/* Descriptor size */
	sizeof(struct cdc_acm_descriptor),
	CS_INTERFACE,                   /* Descriptor type */
	ACM_FUNC_DESC,                  /* Descriptor SubType */
	/* Capabilities - Device supports the request combination of:
	 *	Set_Line_Coding,
	 *	Set_Control_Line_State,
	 *	Get_Line_Coding
	 *	and the notification Serial_State
	 */
	0x02,

	/* Union Functional Descriptor */
	/* Descriptor size */
	sizeof(struct cdc_union_descriptor),
	CS_INTERFACE,                   /* Descriptor type */
	UNION_FUNC_DESC,                /* Descriptor SubType */
	0x00,                           /* Master Interface */
	0x01,                           /* Slave Interface */

	/* Endpoint INT */
	USB_ENDPOINT_DESC_SIZE,         /* Descriptor size */
	USB_ENDPOINT_DESC,              /* Descriptor type */
	/* Endpoint address */
	CONFIG_CDC_ACM_INTERRUPT_EP_ADDR,
	USB_DC_EP_INTERRUPT,            /* Attributes */
	/* Max packet size */
	LOW_BYTE(HS_INTERRUPT_EP_MPS),
	HIGH_BYTE(HS_INTERRUPT_EP_MPS),
	/* Interval */
	CONFIG_CDC_ACM_HS_INTERRUPT_EP_INTERVAL,

	/* Interface descriptor */
	USB_INTERFACE_DESC_SIZE,        /* Descriptor size */
	USB_INTERFACE_DESC,             /* Descriptor type */
	0x01,                           /* Interface index */
	0x00,                           /* Alternate setting */
	0x02,                           /* Number of Endpoints */
	COMMUNICATION_DEVICE_CLASS_DATA,/* Class */
	0x00,                           /* SubClass */
	0x00,                           /* Protocol */
	/* Index of the Interface String Descriptor */
	0x00,

	/* First Endpoint IN */
	USB_ENDPOINT_DESC_SIZE,         /* Descriptor size */
	USB_ENDPOINT_DESC,              /* Descriptor type */
	CONFIG_CDC_ACM_BULK_IN_EP_ADDR, /* Endpoint address */
	USB_DC_EP_BULK,                 /* Attributes */
	/* Max packet size */
	LOW_BYTE(HS_BULK_EP_MPS),
	HIGH_BYTE(HS_BULK_EP_MPS),
	0x00,                           /* Interval */

	/* Second Endpoint OUT */
	USB_ENDPOINT_DESC_SIZE,         /* Descriptor size */
	USB_ENDPOINT_DESC,              /* Descriptor type */
	CONFIG_CDC_ACM_BULK_OUT_EP_ADDR,/* Endpoint address */
	USB_DC_EP_BULK,                 /* Attributes */
	/* Max packet size */
	LOW_BYTE(HS_BULK_EP_MPS),
	HIGH_BYTE(HS_BULK_EP_MPS),
	0x00,                           /* Interval */
};
#endif /* __CDC_ACM_DESC_H__ */