if(CONFIG_CHERRYUSB)

set(CONFIG_CHERRYUSB_OSAL "zephyr")
include (${CMAKE_CURRENT_LIST_DIR}/../cherryusb.cmake)

zephyr_include_directories(${cherryusb_incs})

if (CONFIG_CHERRYUSB_HOST)
zephyr_linker_sources(SECTIONS usbh_class_info.ld)
endif()

if (CONFIG_SHELL)
    list(APPEND cherryusb_srcs ${CMAKE_CURRENT_LIST_DIR}/../platform/zephyr/usb_cmd.c)
endif ()

zephyr_library()
zephyr_library_sources(${cherryusb_srcs})
endif()