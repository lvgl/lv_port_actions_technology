# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_DISPLAY_LCDC_LARK lcdc_lark.c)
zephyr_library_sources_ifdef(CONFIG_DISPLAY_LCDC_LEOPARD lcdc_leopard.c)
