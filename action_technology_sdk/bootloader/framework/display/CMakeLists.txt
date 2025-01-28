#
# Copyright (c) 2020 Actions Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Actions
#

zephyr_sources(sw_math.c sw_blend.c sw_rotate.c)
zephyr_sources_ifdef(CONFIG_UI_DMA_MEMSETCPY ui_memsetcpy.c)
zephyr_sources_ifdef(CONFIG_DISPLAY_COMPOSER display_composer_opt.c)

set(DISPLAY_COMPOSER_LIB_NAME display_composer)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/composer/CMakeLists.txt)
  add_subdirectory(composer)
else()
  zephyr_library_import(${DISPLAY_COMPOSER_LIB_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/lib${DISPLAY_COMPOSER_LIB_NAME}.a)
endif()

zephyr_link_libraries_ifdef(CONFIG_DISPLAY_COMPOSER ${DISPLAY_COMPOSER_LIB_NAME})
