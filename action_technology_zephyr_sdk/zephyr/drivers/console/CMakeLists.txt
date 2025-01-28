# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_UART_CONSOLE uart_console.c)
zephyr_sources_ifdef(CONFIG_RAM_CONSOLE ram_console.c)
zephyr_sources_ifdef(CONFIG_RTT_CONSOLE rtt_console.c)
zephyr_sources_ifdef(CONFIG_UART_PIPE uart_pipe.c)
zephyr_sources_ifdef(CONFIG_USB_UART_CONSOLE uart_usb.c)
zephyr_sources_ifdef(CONFIG_DC5V_UART_CONSOLE dc5v_uart.c)
