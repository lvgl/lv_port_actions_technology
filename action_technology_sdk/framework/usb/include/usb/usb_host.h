/*
 * Copyright (c) 2020 Actions Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB host core layer APIs and structures
 *
 * This file contains the USB device core layer APIs and structures.
 */

#ifndef USB_HOST_H_
#define USB_HOST_H_

#include <drivers/usb/usb_hc.h>
#include <usb/usbstruct.h>
#include <usb/usb_common.h>
#include <sys/slist.h>

#define USB_CTRL_TIMEOUT	5000	/* 5s timeout */

#define USB_MAXINTERFACES	CONFIG_USB_HOST_MAXINTERFACES
#define USB_MAXENDPOINTS	CONFIG_USB_HOST_MAXENDPOINTS
#define USB_RAW_DESCRIPTORS_SIZE	CONFIG_USB_HOST_RAW_DESCRIPTORS_SIZE

struct usb_interface {
	struct usb_if_descriptor desc;
	struct usb_ep_descriptor ep_desc[USB_MAXENDPOINTS];

	uint8_t no_of_ep;
	uint8_t num_altsetting;
	uint8_t act_altsetting;

	struct usb_driver *driver;
} __packed;

struct usb_config {
	union {
		struct usb_cfg_descriptor desc;
		uint8_t rawdescriptors[USB_RAW_DESCRIPTORS_SIZE];
	};

	uint8_t no_of_if;	/* number of interfaces */
	struct usb_interface intf[USB_MAXINTERFACES];
} __packed;

struct usb_device {
	struct usb_device_descriptor desc;

	uint8_t devnum;
	uint8_t scanning;
	uint8_t state;	/* enum usb_device_state */
	uint8_t speed;	/* enum usb_device_speed */

	struct usb_config config;
};

/*
 * struct usb_device_id - identifies USB devices for probing and hotplugging
 */
struct usb_device_id {
	/* which fields to match against? */
	uint16_t match_flags;

	/* Used for product specific matches; range is inclusive */
	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice_lo;
	uint16_t bcdDevice_hi;

	/* Used for device class matches */
	uint8_t bDeviceClass;
	uint8_t bDeviceSubClass;
	uint8_t bDeviceProtocol;

	/* Used for interface class matches */
	uint8_t bInterfaceClass;
	uint8_t bInterfaceSubClass;
	uint8_t bInterfaceProtocol;

	/* Used for vendor-specific interface matches */
	uint8_t bInterfaceNumber;

	/* not matched against */
	unsigned long driver_info;
};

/* Some useful macros to use to create struct usb_device_id */
#define USB_DEVICE_ID_MATCH_VENDOR		0x0001
#define USB_DEVICE_ID_MATCH_PRODUCT		0x0002
#define USB_DEVICE_ID_MATCH_DEV_LO		0x0004
#define USB_DEVICE_ID_MATCH_DEV_HI		0x0008
#define USB_DEVICE_ID_MATCH_DEV_CLASS		0x0010
#define USB_DEVICE_ID_MATCH_DEV_SUBCLASS	0x0020
#define USB_DEVICE_ID_MATCH_DEV_PROTOCOL	0x0040
#define USB_DEVICE_ID_MATCH_INT_CLASS		0x0080
#define USB_DEVICE_ID_MATCH_INT_SUBCLASS	0x0100
#define USB_DEVICE_ID_MATCH_INT_PROTOCOL	0x0200
#define USB_DEVICE_ID_MATCH_INT_NUMBER		0x0400

#define USB_DEVICE_ID_MATCH_DEVICE \
		(USB_DEVICE_ID_MATCH_VENDOR | USB_DEVICE_ID_MATCH_PRODUCT)
#define USB_DEVICE_ID_MATCH_DEV_RANGE \
		(USB_DEVICE_ID_MATCH_DEV_LO | USB_DEVICE_ID_MATCH_DEV_HI)
#define USB_DEVICE_ID_MATCH_DEVICE_AND_VERSION \
		(USB_DEVICE_ID_MATCH_DEVICE | USB_DEVICE_ID_MATCH_DEV_RANGE)
#define USB_DEVICE_ID_MATCH_DEV_INFO \
		(USB_DEVICE_ID_MATCH_DEV_CLASS | \
		USB_DEVICE_ID_MATCH_DEV_SUBCLASS | \
		USB_DEVICE_ID_MATCH_DEV_PROTOCOL)
#define USB_DEVICE_ID_MATCH_INT_INFO \
		(USB_DEVICE_ID_MATCH_INT_CLASS | \
		USB_DEVICE_ID_MATCH_INT_SUBCLASS | \
		USB_DEVICE_ID_MATCH_INT_PROTOCOL)

/* usb interface driver */
struct usb_driver {
	sys_snode_t node;

	int (*probe)(struct usb_interface *intf,
		      const struct usb_device_id *id);
	void (*disconnect)(struct usb_interface *intf);

	const struct usb_device_id *id_table;
};



/*
 * @brief turn on/off USB VBUS voltage
 *
 * @param on Set to false to turn off and to true to turn on VBUS
 *
 * @return 0 on success, negative errno code on fail
 */
int usbh_vbus_set(bool on);

/*
 * @brief issue an asynchronous transfer request for an endpoint
 *
 * If successful, it returns 0, otherwise a negative error number.
 */
int usbh_submit_urb(struct usb_request *urb);

/*
 * @brief cancel a transfer request
 *
 * If successful, it returns 0, otherwise a negative error number.
 */
int usbh_cancel_urb(struct usb_request *urb);

/*
 * @brief issue an synchronous transfer request for an endpoint
 *
 * If successful, it returns 0, otherwise a negative error number. The number
 * of actual bytes transferred will be stored in urb->actual.
 */
int usbh_transfer_urb_sync(struct usb_request *urb);

/*
 * @brief submits a control message and waits for comletion
 *
 * If successful, it returns the number of bytes transferred, otherwise a
 * negative error number.
 */
int usbh_control_msg(unsigned char request, unsigned char requesttype,
				unsigned short value, unsigned short index,
				void *data, unsigned short size, int timeout);

/*
 * @brief Builds a bulk urb, sends it off and waits for completion
 *
 * If successful, it returns 0, otherwise a negative error number. The number
 * of actual bytes transferred will be stored in the actual parameter.
 */
int usbh_bulk_msg(uint8_t ep, void *data, int len, uint32_t *actual, int timeout);

/*
 * @brief clear endpoint halt/stall condition
 *
 * This is used to clear halt conditions for bulk and interrupt endpoints.
 */
int usbh_clear_halt(uint8_t ep);

/*
 * @brief enable an endpoint for USB communications
 *
 * This is used to configure and enable endpoint.
 */
int usbh_enable_endpoint(struct usb_ep_descriptor *desc);

/*
 * @brief disable an endpoint by address
 *
 * Disables the endpoint for URB submission and nukes all pending URBs.
 */
int usbh_disable_endpoint(struct usb_ep_descriptor *desc);

/**
 * @brief enable all the endpoints for an interface
 *
 * Enables all the endpoints for the interface's current altsetting.
 */
int usbh_enable_interface(struct usb_interface *intf);

/*
 * @brief disable all endpoints for an interface
 *
 * Disables all the endpoints for the interface's current altsetting.
 */
int usbh_disable_interface(struct usb_interface *intf);

/*
 * @brief notify host core to prepare for enumeration
 *
 * If successful, it returns 0, otherwise a negative error number.
 */
int usbh_prepare_scan(void);

/*
 * @brief start to enumerate USB device
 *
 * If successful, it returns 0, otherwise a negative error number.
 */
int usbh_scan_device(void);

/*
 * @brief handle port reset
 *
 * If successful, it returns 0, otherwise a negative error number.
 */
int usbh_reset_device(void);

/*
 * @brief disconnect USB device (Synchronously)
 *
 * If successful, it returns 0, otherwise a negative error number.
 */
int usbh_disconnect(void);

/*
 * @brief poll if enumerating is in progress
 *
 * If enumerating, it returns true, otherwise false.
 */
bool usbh_is_scanning(void);

/*
 * @brief register a USB interface driver
 */
int usbh_register_driver(struct usb_driver *driver);

/*
 * @brief unregister a USB interface driver
 */
void usbh_unregister_driver(struct usb_driver *driver);

/*
 * @brief control the FIFO of USB controller
 *
 * @param enable    Set to false to make FIFO accessible by CPU as normal RAM
 *                          and to true to make FIFO accessible by USB.
 *
 * If successful, it returns 0, otherwise a negative error number.
 */
int usbh_fifo_control(bool enable);

#endif /* USB_HOST_H_ */
