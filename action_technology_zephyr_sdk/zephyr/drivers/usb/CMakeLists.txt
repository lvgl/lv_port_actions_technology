# SPDX-License-Identifier: Apache-2.0

if(CONFIG_USB_DEVICE OR CONFIG_USB_HOST)
       add_subdirectory(otg)
endif()
