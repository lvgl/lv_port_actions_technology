# Copyright (c) 2020 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_include_directories(
  .
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
)

zephyr_library_sources(
  tracedump_core.c
)
