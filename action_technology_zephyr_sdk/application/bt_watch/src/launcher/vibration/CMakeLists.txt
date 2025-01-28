# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(
	.
)

target_sources(app PRIVATE
	vibration_ui.c
	vibration_view.c
)
