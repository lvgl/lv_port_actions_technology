# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_library_sources(
    input_manager.c
)

zephyr_library_sources_ifdef(
	CONFIG_INPUT_POINTER
	input_pointer.c
)


zephyr_library_sources_ifdef(
	CONFIG_INPUT_KEYPAD
	input_keypad.c
)


zephyr_library_sources_ifdef(
	CONFIG_INPUT_ENCODER
	input_encoder.c
)

zephyr_library_sources_ifdef(
	CONFIG_INPUT_DEV_ACTS_KNOB
	input_knob.c
)
