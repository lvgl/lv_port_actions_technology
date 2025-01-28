# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_FS_MANAGER    fs_manager.c)
zephyr_library_sources_ifdef(CONFIG_SDFS_MANAGER  sdfs_manager.c)
zephyr_library_sources_ifdef(CONFIG_FILE_SYSTEM_LITTLEFS  littlefs_manager.c)
