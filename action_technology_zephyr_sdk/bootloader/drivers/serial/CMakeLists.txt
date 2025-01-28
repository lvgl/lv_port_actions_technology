# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_UART_ACTS uart_acts.c)
zephyr_library_sources_ifdef(CONFIG_UART_RTT_DRIVER uart_rtt.c)
zephyr_library_sources_ifdef(CONFIG_USB_CDC_ACM ${ZEPHYR_BASE}/misc/empty_file.c)
