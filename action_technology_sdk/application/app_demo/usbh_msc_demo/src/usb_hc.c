/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "usbh_core.h"
#include "usbh_hub.h"

#ifndef USBH_IRQHandler
#define USBH_IRQHandler OTG_HS_IRQHandler
#endif


__WEAK void usb_hc_low_level_init(struct usbh_bus *bus)
{
}

int usb_hc_init(struct usbh_bus *bus)
{

    usb_hc_low_level_init(bus);

    return 0;
}

uint16_t usbh_get_frame_number(struct usbh_bus *bus)
{
    return 0;
}

int usbh_roothub_control(struct usbh_bus *bus, struct usb_setup_packet *setup, uint8_t *buf)
{
    return 0;
}

int usbh_submit_urb(struct usbh_urb *urb)
{
    return 0;
}

int usbh_kill_urb(struct usbh_urb *urb)
{
    return 0;
}

static inline void dwc2_urb_waitup(struct usbh_bus *bus, struct usbh_urb *urb)
{
}

void USBH_IRQHandler(uint32_t busid)
{
}