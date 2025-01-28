# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_DISPLAY_CONTROLLER controller)
add_subdirectory_ifdef(CONFIG_DISPLAY_CONTROLLER panel)
add_subdirectory_ifdef(CONFIG_DISPLAY_ENGINE engine)
add_subdirectory_ifdef(CONFIG_DMA2D_LITE dma2d_lite)

zephyr_sources(display_graphics.c)
zephyr_sources_ifdef(CONFIG_DISPLAY_MMU display_mmu.c)
