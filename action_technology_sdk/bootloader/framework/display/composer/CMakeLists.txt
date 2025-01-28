# SPDX-License-Identifier: Apache-2.0

set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../)

if(CONFIG_DISPLAY_COMPOSER)
  zephyr_library_named(${DISPLAY_COMPOSER_LIB_NAME})
  zephyr_library_sources(display_composer.c)
endif()
