# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_MMC mmc_ops.c)
zephyr_library_sources_ifdef(CONFIG_MMC_SDCARD sd.c)
zephyr_library_sources_ifdef(CONFIG_MMC_SDIO sdio.c)
zephyr_library_sources_ifdef(CONFIG_MMC_ACTS mmc_acts.c)
