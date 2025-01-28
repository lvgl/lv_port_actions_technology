#
# Copyright (c) 2020 Actions Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Actions
#

zephyr_sources(
	sw_math.c sw_blend.c sw_convert.c sw_rotate.c
	ui_region.c ui_math.c ui_memsetcpy.c graphic_buffer.c
)

add_subdirectory_ifdef(CONFIG_DISPLAY_COMPOSER composer)
