#
# Copyright (c) 2020 Actions Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Actions
#

if (CONFIG_ACTIONS_FRAMEWORK)

zephyr_include_directories(
	include
)

add_subdirectory(osal)
add_subdirectory(key)
add_subdirectory_ifdef(CONFIG_DMA2D_HAL dma2d)
add_subdirectory_ifdef(CONFIG_DISPLAY display)
add_subdirectory_ifdef(CONFIG_MEMORY memory)
add_subdirectory_ifdef(CONFIG_WATCHDOG watchdog)
endif()
