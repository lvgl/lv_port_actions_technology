# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_ACTIONS_FRAMEWORK_DISPLAY)

set(ZEPHYR_CURRENT_LIBRARY acts_display)

add_library(acts_display INTERFACE)

zephyr_include_directories(
  include
)

target_include_directories(acts_display INTERFACE
  include
)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_UI_MANAGER ui_manager.c)
zephyr_library_sources_ifdef(CONFIG_LED_MANAGER led_manager.c)
zephyr_library_sources_ifdef(CONFIG_SEG_LED_MANAGER seg_led_manager.c)

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/libdisplay/CMakeLists.txt)
  add_subdirectory(libdisplay)
else()
  zephyr_library_include_directories(${CMAKE_CURRENT_LIST_DIR})
  zephyr_library_import(display ${CMAKE_CURRENT_LIST_DIR}/libdisplay.a)
endif()

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/libgui/CMakeLists.txt)
  add_subdirectory(libgui)
else()
  zephyr_library_include_directories(${CMAKE_CURRENT_LIST_DIR})
  zephyr_library_import(gui ${CMAKE_CURRENT_LIST_DIR}/libdisplay_gui.a)
endif()

add_subdirectory(compression)
add_subdirectory(font)
add_subdirectory_ifdef(CONFIG_LVGL lvgl)
add_subdirectory_ifdef(CONFIG_RES_MANAGER res_manager)
add_subdirectory_ifdef(CONFIG_UI_MEMORY_MANAGER memory)
add_subdirectory_ifdef(CONFIG_UI_MEMORY_MANAGER surface)
add_subdirectory_ifdef(CONFIG_UI_SERVICE ui_service)
add_subdirectory_ifdef(CONFIG_VG_LITE vg_lite)

zephyr_library_link_libraries(acts_display)
target_link_libraries(acts_display INTERFACE zephyr_interface)

zephyr_library_link_libraries_ifdef(CONFIG_LVGL lvgl)
zephyr_library_link_libraries_ifdef(CONFIG_VG_LITE vg_lite)
zephyr_library_link_libraries_ifdef(CONFIG_FREETYPE_FONT freetype)

endif()
