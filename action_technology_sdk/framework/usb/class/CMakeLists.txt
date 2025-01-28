# SPDX-License-Identifier: Apache-2.0

if(CONFIG_USB_MASS_STORAGE)
  zephyr_interface_library_named(mass_storage)

  zephyr_library()
  zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
  zephyr_library_sources(mass_storage.c)

  zephyr_library_link_libraries(mass_storage)

  target_link_libraries_ifdef(CONFIG_FAT_FILESYSTEM_ELM mass_storage INTERFACE ELMFAT)
endif()

add_subdirectory_ifdef(CONFIG_USB_AUDIO_CLASS audio)
add_subdirectory_ifdef(CONFIG_USB_DEVICE_HID hid)
add_subdirectory_ifdef(CONFIG_USB_DEVICE_STUB stub)
add_subdirectory_ifdef(CONFIG_USB_DEVICE_DFU dfu)

zephyr_library_sources_ifdef(CONFIG_USB_CDC_ACM	cdc_acm.c)
