# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cherryusb_msc)

if(CONFIG_CHERRYUSB)

set(CONFIG_CHERRYUSB_OSAL "zephyr")
include (${ZEPHYR_BASE}/../thirdparty/lib/CherryUSB/cherryusb.cmake)

zephyr_include_directories(${cherryusb_incs})

if (CONFIG_CHERRYUSB_HOST)
zephyr_linker_sources(SECTIONS usbh_class_info.ld)
endif()

#zephyr_library()
#zephyr_library_sources(${cherryusb_srcs})
endif()

zephyr_include_directories(src/include)
target_sources(app PRIVATE src/main.c src/usb_dc.c ${cherryusb_srcs})
