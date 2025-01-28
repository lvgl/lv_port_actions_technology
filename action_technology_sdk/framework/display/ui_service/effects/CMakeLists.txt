# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_UI_SCROLL_EFFECT scroll_effect.c)
zephyr_library_sources_ifdef(CONFIG_UI_SWITCH_EFFECT switch_effect.c)
