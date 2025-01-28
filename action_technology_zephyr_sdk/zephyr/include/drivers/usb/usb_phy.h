/* usb_phy.h - USB otg controller driver interface */

/*
 * Copyright (c) 2020 Actions Corporation.
 * Author: Jinang Lv <lvjinang@actions-semi.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB OTG/PHY layer APIs
 *
 * This file contains the USB OTG/PHY layer APIs. All otg controller
 * drivers should implement the APIs described in this file.
 */

#ifndef __USB_PHY_H__
#define __USB_PHY_H__

#include <device.h>

enum usb_otg_state {
	OTG_STATE_UNDEFINED = 0,

	/* single-role peripheral, and dual-role default-b */
	OTG_STATE_B_IDLE,
	OTG_STATE_B_SRP_INIT,
	OTG_STATE_B_PERIPHERAL,

	/* extra dual-role default-b states */
	OTG_STATE_B_WAIT_ACON,
	OTG_STATE_B_HOST,

	/* dual-role default-a */
	OTG_STATE_A_IDLE,
	OTG_STATE_A_WAIT_VRISE,
	OTG_STATE_A_WAIT_BCON,
	OTG_STATE_A_HOST,
	OTG_STATE_A_SUSPEND,
	OTG_STATE_A_PERIPHERAL,
	OTG_STATE_A_WAIT_VFALL,
	OTG_STATE_A_VBUS_ERR,
};

/* Idpin */
enum {
	USB_ID_LOW = 0,
	USB_ID_HIGH,
	USB_ID_INVALID,
};

/* Vbus */
enum {
	USB_VBUS_LOW = 0,
	USB_VBUS_HIGH,
	USB_VBUS_INVALID,
};

/* OTG mode */
enum {
	USB_OTG_UNKNOWN = 0,
	USB_OTG_DEVICE,
	USB_OTG_HOST,
};

/**
 * @brief USB PHY Idpin state
 *
 * @return USB_ID_HIGH if high, USB_ID_LOW if low, else USB_ID_INVALID.
 */
uint8_t usb_phy_get_id(void);

/**
 * @brief USB PHY Vbus state
 *
 * @return USB_VBUS_HIGH if high, USB_VBUS_LOW if low, else USB_VBUS_INVALID.
 */
uint8_t usb_phy_get_vbus(void);

/**
 * @brief USB PHY detects peripheral is connected
 *
 * @return true if connected, otherwise false.
 */
bool usb_phy_hc_attached(void);

/**
 * @brief USB host controller detects peripheral is connected
 *
 * @return true if connected, otherwise false.
 */
bool usb_phy_hc_connected(void);

/**
 * @brief USB host controller detects peripheral is disconnected
 *
 * @return true if disconnected, otherwise false.
 */
bool usb_phy_hc_disconnected(void);

/**
 * @brief USB PHY device mode is attached
 *
 * @return true if attached, otherwise false.
 */
bool usb_phy_dc_attached(void);

/**
 * @brief USB device controller is detached to host
 *
 * @return true if detached, otherwise false.
 */
bool usb_phy_dc_detached(void);

/**
 * @brief USB device controller is connected to host
 *
 * @return true if connected, otherwise false.
 */
bool usb_phy_dc_connected(void);

/**
 * @brief USB PHY device mode disconnected
 *
 * @return true if disconnected, otherwise false.
 */
bool usb_phy_dc_disconnected(void);

/**
 * @brief USB PHY disconnect device mode
 */
void usb_phy_dc_disconnect(void);

/**
 * @brief USB PHY enter a_idle state to prepare for a_wait_bcon state
 *
 * @return always 0, not used yet.
 */
int usb_phy_enter_a_idle(void);

/**
 * @brief USB PHY enter b_idle state to prepare for device mode
 *
 * @return always 0, not used yet.
 */
int usb_phy_enter_b_idle(void);

/**
 * @brief USB PHY enter a_wait_bcon state to prepare for a_host state
 *
 * @return always 0, not used yet.
 */
int usb_phy_enter_a_wait_bcon(void);

/**
 * @brief reset USB PHY
 *
 * @return always 0, not used yet.
 */
int usb_phy_reset(void);

/**
 * @brief initialize USB PHY (call once!)
 *
 * @return always 0, not used yet.
 */
int usb_phy_init(void);

/**
 * @brief de-initialize USB PHY
 *
 * @return always 0, not used yet.
 */
int usb_phy_exit(void);


#endif /* __USB_PHY_H__ */
