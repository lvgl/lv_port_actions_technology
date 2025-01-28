/** @file
 *  @brief USB UART driver header file.
 *
 *  Public API for USB UART driver
 *
 */

/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __UART_USB_H__
#define __UART_USB_H__

#ifdef __cplusplus
 extern "C" {
#endif

/** @brief Notify UART USB send data completely event.
 *
 *  This function is used to notify the data have been send out completely by USB CDC ACM.
 *
 *  @param void.
 */
void uart_usb_update_tx_done(void);


/** @brief Send data over USB UART.
 *
 *  This function is used to send data over USB UART.
 *
 *  @param data Buffer with data to be send.
 *  @param len Size of data.
 *
 *  @return 0 on success or negative error
 */
int uart_usb_send(const u8_t *data, int len);

/** @brief receive data over USB UART.
 *
 *  This function is used to receive data over USB UART.
 *
 *  @param data Buffer with data to be received.
 *  @param max len size of data.
 *
 *  @return positive value for received data length or negative error
 */
int uart_usb_receive(u8_t *data, int max_len);

/** @brief Initialize the USB UART environment.
 *
 *  This function is used to initialize USB UART.
 *
 */
int uart_usb_init(void);

/** @brief Uninitialize the USB UART environment.
 *
 *  This function is used to uninitialize USB UART.
 *
 */
void uart_usb_uninit(void);

/** @brief check USB UART is enable
 *
 *  @return true usb uart is enable false is disable
 *
 */
bool uart_usb_is_enabled(void);

#ifdef __cplusplus
}
#endif

#endif /* __UART_USB_H__ */

