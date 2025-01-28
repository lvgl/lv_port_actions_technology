# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_USB_DEVICE_STACK OR CONFIG_USB_HOST_STACK)

set(ZEPHYR_CURRENT_LIBRARY acts_framework_usb)

add_library(acts_framework_usb INTERFACE)

zephyr_include_directories(
	.
	include
)

target_include_directories(acts_framework_usb INTERFACE
	include
)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_USB_DEVICE_STACK usb_device.c)
add_subdirectory_ifdef(CONFIG_USB_DEVICE_STACK class)
	
zephyr_library_sources_ifdef(CONFIG_USB_HOST_STACK usb_host.c)
add_subdirectory_ifdef(CONFIG_USB_HOST_STACK host)

zephyr_library_link_libraries(acts_framework_usb)

target_link_libraries(acts_framework_usb INTERFACE zephyr_interface)

endif() 
