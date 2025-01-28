# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(
	.
)

target_sources(app PRIVATE
	gps_ui.c
	gps_view.c
)
