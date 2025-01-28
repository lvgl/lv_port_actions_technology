#
# Copyright (c) 2020 Actions Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Actions
#

zephyr_include_directories(
	include
	include/al
	include/sensor
)

if (CONFIG_ACTIONS_FRAMEWORK)

add_subdirectory(osal)
add_subdirectory(audio)
add_subdirectory(key)
add_subdirectory_ifdef(CONFIG_ACTIONS_LIBRARY arithmetic)
add_subdirectory_ifdef(CONFIG_DSP_HAL dsp)
add_subdirectory_ifdef(CONFIG_DMA2D_HAL dma2d)
add_subdirectory_ifdef(CONFIG_JPEG_HAL jpeg)
add_subdirectory_ifdef(CONFIG_DISPLAY display)
add_subdirectory_ifdef(CONFIG_MEMORY memory)
add_subdirectory_ifdef(CONFIG_SENSOR sensor)
add_subdirectory_ifdef(CONFIG_ANC_HAL anc)
add_subdirectory_ifdef(CONFIG_WATCHDOG watchdog)

endif()
