# SPDX-License-Identifier: Apache-2.0

set(DRV_DISPLAY_ENGINE_LIB_NAME drv_display_engine)

zephyr_library_sources(de_common.c de_device.c)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/private/CMakeLists.txt)
  add_subdirectory(private)
else()
  zephyr_library_import(display_engine ${CMAKE_CURRENT_SOURCE_DIR}/lib${DRV_DISPLAY_ENGINE_LIB_NAME}.a)
endif()
