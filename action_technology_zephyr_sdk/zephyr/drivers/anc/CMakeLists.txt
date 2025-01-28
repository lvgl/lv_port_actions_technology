# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_include_directories(.)

zephyr_library_sources(
	anc_dev.c
	anc_image.c
	anc_loader.c
)

