# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_library_sources(
    property_manager.c
)

zephyr_library_sources_ifdef(CONFIG_PROPERTY_CACHE
    property_cache.c
)
