# SPDX-License-Identifier: Apache-2.0

set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../)

zephyr_library_named(${DRV_DISPLAY_ENGINE_LIB_NAME})

zephyr_library_sources(drv_de_version.c)
zephyr_library_sources_ifdef(CONFIG_DISPLAY_ENGINE_LARK de_lark.c)
zephyr_library_sources_ifdef(CONFIG_DISPLAY_ENGINE_LEOPARD de_leopard.c)

