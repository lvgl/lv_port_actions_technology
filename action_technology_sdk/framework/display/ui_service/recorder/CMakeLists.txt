# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(input_recorder.c)

if(CONFIG_UI_INPUT_RECORDER)
  zephyr_library_sources(
    input_recorder_slide_fixedstep.c
    input_recorder_buffer.c
   )

  zephyr_library_sources_ifdef(CONFIG_STREAM input_recorder_stream.c)
endif()
