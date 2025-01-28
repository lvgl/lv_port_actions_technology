# SPDX-License-Identifier: Apache-2.0

file(GLOB SOURCES *.c)

target_include_directories(app PRIVATE .)
target_sources(app PRIVATE ${SOURCES})

add_subdirectory_ifdef(CONFIG_LVGL widgets)
add_subdirectory_ifdef(CONFIG_VG_LITE svgrender)
