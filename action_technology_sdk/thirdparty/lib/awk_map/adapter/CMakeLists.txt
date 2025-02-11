# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_library_sources(
	awk_vglite.c
	awk_font.c
	awk_system_adapter.c
	awk_render_adapter.c
)

