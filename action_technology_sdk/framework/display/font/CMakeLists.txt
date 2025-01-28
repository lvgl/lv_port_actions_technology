# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_BITMAP_FONT OR CONFIG_FREETYPE_FONT)
  zephyr_library_sources(font_mempool.c)
endif()

add_subdirectory_ifdef(CONFIG_BITMAP_FONT bitmap_font)

if (CONFIG_FREETYPE_FONT)
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/freetype/CMakeLists.txt)
  add_subdirectory(freetype)
else()
  zephyr_library_include_directories(${CMAKE_CURRENT_LIST_DIR}/../)
  zephyr_library_import(freetype ${CMAKE_CURRENT_LIST_DIR}/../libfreetype.a)
endif()

  zephyr_library_sources_ifdef(CONFIG_FREETYPE_FONT_ENABLE_SVG_PATH svg_font.c)
endif()
