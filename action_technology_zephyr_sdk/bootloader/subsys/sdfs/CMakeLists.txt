#
# Copyright (c) 2020 Actions Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Actions
#

zephyr_sources_ifdef(CONFIG_SD_FS sdfs.c)
zephyr_sources_ifdef(CONFIG_SD_FS_NAND_SD_STORAGE sdfs_nand_sd.c)
