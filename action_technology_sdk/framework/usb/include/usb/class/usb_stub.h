/*
 * USB Stub device header file.
 *
 * Copyright (c) 2019 Actions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Stub device public header
 */

#ifndef __USB_STUB_H__
#define __USB_STUB_H__
bool usb_stub_enabled(void);
bool usb_stub_configured(void);
int usb_stub_init(struct device *dev);
int usb_stub_exit(void);
int usb_stub_ep_read(uint8_t *data_buffer, uint32_t data_len, uint32_t timeout);
int usb_stub_ep_write(uint8_t *data_buffer, uint32_t data_len, uint32_t timeout);
#endif /* __USB_STUB_H__ */

