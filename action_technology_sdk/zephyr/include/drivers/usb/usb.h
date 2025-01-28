/*
 * Copyright (c) 2020 Actions Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief useful constants and macros for the USB controller
 *
 * This file contains useful constants and macros for the USB controller.
 */

#ifndef __USB_H__
#define __USB_H__

/*
 * USB directions
 *
 * This bit flag is used in endpoint descriptors' bEndpointAddress field.
 * It's also one of three fields in control requests bRequestType.
 */
#define USB_DIR_OUT	0x00	/* to device */
#define USB_DIR_IN	0x80	/* to host */

/*
 * NOTE: USB types, recipients, Standard requests, feature flags are also
 * defined in usb/usbstruct.h (with device names), it is more reasonable
 * to place here.
 */

/*
 * USB types, the second of three bRequestType fields
 */
#define USB_TYPE_MASK		(0x03 << 5)
#define USB_TYPE_STANDARD	(0x00 << 5)
#define USB_TYPE_CLASS		(0x01 << 5)
#define USB_TYPE_VENDOR		(0x02 << 5)
#define USB_TYPE_RESERVED	(0x03 << 5)

/*
 * USB recipients, the third of three bRequestType fields
 */
#define USB_RECIP_MASK		0x1f
#define USB_RECIP_DEVICE	0x00
#define USB_RECIP_INTERFACE	0x01
#define USB_RECIP_ENDPOINT	0x02
#define USB_RECIP_OTHER		0x03
/* From Wireless USB 1.0 */
#define USB_RECIP_PORT		0x04
#define USB_RECIP_RPIPE		0x05

/*
 * Standard requests, for the bRequest field of a SETUP packet.
 *
 * These are qualified by the bRequestType field, so that for example
 * TYPE_CLASS or TYPE_VENDOR specific feature flags could be retrieved
 * by a GET_STATUS request.
 */
#define USB_REQ_GET_STATUS		0x00
#define USB_REQ_CLEAR_FEATURE		0x01
#define USB_REQ_SET_FEATURE		0x03
#define USB_REQ_SET_ADDRESS		0x05
#define USB_REQ_GET_DESCRIPTOR		0x06
#define USB_REQ_SET_DESCRIPTOR		0x07
#define USB_REQ_GET_CONFIGURATION	0x08
#define USB_REQ_SET_CONFIGURATION	0x09
#define USB_REQ_GET_INTERFACE		0x0A
#define USB_REQ_SET_INTERFACE		0x0B
#define USB_REQ_SYNCH_FRAME		0x0C
#define USB_REQ_SET_SEL			0x30
#define USB_REQ_SET_ISOCH_DELAY		0x31

#define USB_REQ_SET_ENCRYPTION		0x0D	/* Wireless USB */
#define USB_REQ_GET_ENCRYPTION		0x0E
#define USB_REQ_RPIPE_ABORT		0x0E
#define USB_REQ_SET_HANDSHAKE		0x0F
#define USB_REQ_RPIPE_RESET		0x0F
#define USB_REQ_GET_HANDSHAKE		0x10
#define USB_REQ_SET_CONNECTION		0x11
#define USB_REQ_SET_SECURITY_DATA	0x12
#define USB_REQ_GET_SECURITY_DATA	0x13
#define USB_REQ_SET_WUSB_DATA		0x14
#define USB_REQ_LOOPBACK_DATA_WRITE	0x15
#define USB_REQ_LOOPBACK_DATA_READ	0x16
#define USB_REQ_SET_INTERFACE_DS	0x17

/* The Link Power Management (LPM) ECN defines USB_REQ_TEST_AND_SET command,
 * used by hubs to put ports into a new L1 suspend state, except that it
 * forgot to define its number ...
 */

/*
 * USB feature flags are written using USB_REQ_{CLEAR,SET}_FEATURE, and
 * are read as a bit array returned by USB_REQ_GET_STATUS.  (So there
 * are at most sixteen features of each type.)  Hubs may also support a
 * new USB_REQ_TEST_AND_SET_FEATURE to put ports into L1 suspend.
 */
#define USB_DEVICE_SELF_POWERED		0	/* (read only) */
#define USB_DEVICE_REMOTE_WAKEUP	1	/* dev may initiate wakeup */
#define USB_DEVICE_TEST_MODE		2	/* (wired high speed only) */
#define USB_DEVICE_BATTERY		2	/* (wireless) */
#define USB_DEVICE_B_HNP_ENABLE		3	/* (otg) dev may initiate HNP */
#define USB_DEVICE_WUSB_DEVICE		3	/* (wireless)*/
#define USB_DEVICE_A_HNP_SUPPORT	4	/* (otg) RH port supports HNP */
#define USB_DEVICE_A_ALT_HNP_SUPPORT	5	/* (otg) other RH port does */
#define USB_DEVICE_DEBUG_MODE		6	/* (special devices only) */

#define USB_ENDPOINT_HALT		0	/* IN/OUT will STALL */

/*
 * Test Mode Selectors
 * See USB 2.0 spec Table 9-7
 */
#define	TEST_J		1
#define	TEST_K		2
#define	TEST_SE0_NAK	3
#define	TEST_PACKET	4
#define	TEST_FORCE_EN	5

/** setup packet definitions */
struct usb_setup_packet {
	uint8_t bmRequestType;  /**< characteristics of the specific request */
	uint8_t bRequest;       /**< specific request */
	uint16_t wValue;        /**< request specific parameter */
	uint16_t wIndex;        /**< request specific parameter */
	uint16_t wLength;       /**< length of data transferred in data phase */
} __packed;

#define USB_REQ_DIR_IN(bmRequestType)	(bmRequestType & USB_DIR_IN)


/*
 * Endpoints
 */
#define USB_EP_NUM_MASK	0x07	/* in bEndpointAddress */
#define	USB_EP_DIR_MASK	0x80

/* Default USB control EP */
#define USB_CONTROL_OUT_EP0	0
#define USB_CONTROL_IN_EP0	0x80

/* For compatibility */
#define	USB_EP_DIR_IN	USB_DIR_IN
#define	USB_EP_DIR_OUT	USB_DIR_OUT

/* Convert from endpoint address to hardware endpoint index */
#define USB_EP_ADDR2IDX(ep)	((ep) & USB_EP_NUM_MASK)
/* Get direction from endpoint address */
#define USB_EP_ADDR2DIR(ep)	((ep) & USB_EP_DIR_MASK)
/* Convert from hardware endpoint index and direction to endpoint address */
#define USB_EP_IDX2ADDR(idx, dir)	((idx) | ((dir) & USB_EP_DIR_MASK))
/* If endpoint direction is out */
#define USB_EP_DIR_IS_OUT(ep)	(USB_EP_ADDR2DIR(ep) == USB_EP_DIR_OUT)
/* If endpoint direction is in */
#define USB_EP_DIR_IS_IN(ep)	(USB_EP_ADDR2DIR(ep) == USB_EP_DIR_IN)

static inline uint8_t usb_endpoint_num(const uint8_t ep)
{
	return ep & USB_EP_NUM_MASK;
}

static inline bool usb_ep_dir_in(const uint8_t ep)
{
	return ((ep & USB_EP_DIR_MASK) == USB_EP_DIR_IN);
}

static inline bool usb_ep_dir_out(const uint8_t ep)
{
	return ((ep & USB_EP_DIR_MASK) == USB_EP_DIR_OUT);
}

#define USB_EP_TYPE_MASK	0x03	/* in bmAttributes */

enum usb_ep_type {
	USB_EP_CONTROL = 0,  /* Control type endpoint */
	USB_EP_ISOCHRONOUS,  /* Isochronous type endpoint */
	USB_EP_BULK,         /* Bulk type endpoint */
	USB_EP_INTERRUPT     /* Interrupt type endpoint  */
};

/* The USB 3.0 spec redefines bits 5:4 of bmAttributes as interrupt ep type. */
#define USB_EP_INTRTYPE			0x30
#define USB_EP_INTR_PERIODIC		(0 << 4)
#define USB_EP_INTR_NOTIFICATION	(1 << 4)

#define USB_EP_SYNCTYPE		0x0c
#define USB_EP_SYNC_NONE	(0 << 2)
#define USB_EP_SYNC_ASYNC	(1 << 2)
#define USB_EP_SYNC_ADAPTIVE	(2 << 2)
#define USB_EP_SYNC_SYNC	(3 << 2)

#define USB_EP_USAGE_MASK		0x30
#define USB_EP_USAGE_DATA		0x00
#define USB_EP_USAGE_FEEDBACK		0x10
#define USB_EP_USAGE_IMPLICIT_FB	0x20	/* Implicit feedback Data endpoint */

static inline enum usb_ep_type usb_endpoint_type(uint8_t bmAttributes)
{
	return bmAttributes & USB_EP_TYPE_MASK;
}

static inline int usb_endpoint_xfer_bulk(uint8_t bmAttributes)
{
	return ((bmAttributes & USB_EP_TYPE_MASK) == USB_EP_BULK);
}

static inline int usb_endpoint_xfer_control(uint8_t bmAttributes)
{
	return ((bmAttributes & USB_EP_TYPE_MASK) == USB_EP_CONTROL);
}

static inline int usb_endpoint_xfer_int(uint8_t bmAttributes)
{
	return ((bmAttributes & USB_EP_TYPE_MASK) == USB_EP_INTERRUPT);
}

static inline int usb_endpoint_xfer_isoc(uint8_t bmAttributes)
{
	return ((bmAttributes & USB_EP_TYPE_MASK) == USB_EP_ISOCHRONOUS);
}

/*
 * USB endpoint max packet size.
 */
#define USB_MAX_FS_BULK_MPS	64   /**< full speed MPS for bulk EP */
#define USB_MAX_FS_INTR_MPS	64   /**< full speed MPS for interrupt EP */
#define USB_MAX_FS_ISOC_MPS	1023 /**< full speed MPS for isochronous EP */
#define USB_MAX_HS_BULK_MPS	512  /**< high speed MPS for bulk EP */
#define USB_MAX_HS_INTR_MPS	1024 /**< high speed MPS for interrupt EP */
#define USB_MAX_HS_ISOC_MPS	1024 /**< high speed MPS for isochronous EP */

#define MAX_PACKET_SIZE0	64  /**< maximum packet size for EP 0 */
#define USB_MAX_CTRL_MPS	MAX_PACKET_SIZE0	/* alias */

enum usb_device_state {
	/* NOTATTACHED isn't in the USB spec, and this state acts
	 * the same as ATTACHED ... but it's clearer this way.
	 */
	USB_STATE_NOTATTACHED = 0,

	/* chapter 9 and authentication (wireless) device states */
	USB_STATE_ATTACHED,
	USB_STATE_POWERED,			/* wired */
	USB_STATE_RECONNECTING,			/* auth */
	USB_STATE_UNAUTHENTICATED,		/* auth */
	USB_STATE_DEFAULT,			/* limited function */
	USB_STATE_ADDRESS,
	USB_STATE_CONFIGURED,			/* most functions */

	USB_STATE_SUSPENDED

	/* NOTE:  there are actually four different SUSPENDED
	 * states, returning to POWERED, DEFAULT, ADDRESS, or
	 * CONFIGURED respectively when SOF tokens flow again.
	 * At this level there's no difference between L1 and L2
	 * suspend states.  (L2 being original USB 1.1 suspend.)
	 */
};

/*
 * USB Device Speed
 */
enum usb_device_speed {
	USB_SPEED_UNKNOWN = 0,			/* enumerating */
	USB_SPEED_LOW, USB_SPEED_FULL,		/* usb 1.1 */
	USB_SPEED_HIGH,				/* usb 2.0 */
	USB_SPEED_WIRELESS,			/* wireless (usb 2.5) */
	USB_SPEED_SUPER,			/* usb 3.0 */
};

#endif /* __USB_H__ */
