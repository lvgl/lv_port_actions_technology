# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_USB_DEVICE_STUB usb_stub_dev.c)