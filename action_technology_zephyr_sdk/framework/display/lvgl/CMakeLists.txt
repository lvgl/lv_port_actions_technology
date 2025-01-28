# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_LVGL_USE_IMG_DECODER_ACTS_RES lvgl_img_res_decoder.c)
zephyr_library_sources_ifdef(CONFIG_LVGL_USE_RES_MANAGER lvgl_res_loader.c lvgl_img_loader.c)
zephyr_library_sources_ifdef(CONFIG_LVGL_USE_FREETYPE_FONT lvgl_freetype_font.c)
zephyr_library_sources_ifdef(CONFIG_LVGL_USE_BITMAP_FONT lvgl_bitmap_font.c)
zephyr_library_sources_ifdef(CONFIG_UI_MEMORY_MANAGER lvgl_virtual_display.c)
