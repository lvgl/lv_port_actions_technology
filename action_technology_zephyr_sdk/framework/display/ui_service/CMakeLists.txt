# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(gesture.c view_stack.c view_cache.c view_cache_lite.c)

add_subdirectory(effects)
add_subdirectory_ifdef(CONFIG_LVGL lvgl)
