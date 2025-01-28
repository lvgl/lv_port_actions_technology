# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(./zephyr)

zephyr_sources(
	vg_lite_kernel.c
	zephyr/vg_lite_hal.c
)
