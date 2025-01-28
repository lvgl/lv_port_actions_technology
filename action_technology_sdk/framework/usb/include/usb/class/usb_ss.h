/*
 * Source/Sink USB class core header
 *
 * Copyright (c) 2018 Actions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __USB_SS_H__
#define __USB_SS_H__

#include <usb/usb_device.h>

/* Register Source/Sink status callback */
void ss_register_status_cb(usb_status_callback cb_usb_status);

/* Register Source/Sink class handler */
void ss_register_class_handler(usb_request_handler class_handler,
				uint8_t *payload_data);

/* Register Source/Sink vendor handler */
void ss_register_vendor_handler(usb_request_handler vendor_handler,
				uint8_t *vendor_data);

/* Register Source/Sink custom handler */
void ss_register_custom_handler(usb_request_handler custom_handler);

/* Register Source/Sink read/write callbacks */
void ss_register_ops(usb_dc_ep_callback in_ready,
				usb_dc_ep_callback out_ready);

/* Initialize USB Source/Sink */
int usb_ss_init(void);

/* Write to Source/Sink out-endpoint */
int ss_ep_write(const uint8_t *data, uint32_t data_len, uint32_t *bytes_ret);

/* Read from Source/Sink in-endpoint */
int ss_ep_read(uint8_t *data, uint32_t data_len, uint32_t *bytes_ret);

/* Flush FIFO for Source/Sink in-endpoint */
int ss_in_ep_flush(void);

/* Flush FIFO for Source/Sink out-endpoint */
int ss_out_ep_flush(void);

#endif /* __USB_SS_H__ */
