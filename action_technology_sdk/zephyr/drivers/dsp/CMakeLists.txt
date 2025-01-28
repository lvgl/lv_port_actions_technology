# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_include_directories(.)

zephyr_library_sources(
	dsp_dev.c
	dsp_image.c
	dsp_loader.c
	dsp_mailbox.c
)

