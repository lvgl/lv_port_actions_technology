# Copyright (c) 2020 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_compile_options(-O3 -DDEBUG_RAMDUMP)

zephyr_library_include_directories(
  .
)

zephyr_library_sources(
  ramdump_core.c
  ramdump_mem.c
  fastlz.c
)
