# Copyright (c) 2020 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_include_directories(
  .
)

zephyr_library_sources(
  boot.c
  check.c
  dir.c
  fat.c
  porting.c
)
