# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_DISK_ACCESS disk_access.c)
zephyr_sources_ifdef(CONFIG_DISK_ACCESS_FLASH disk_access_flash.c)
zephyr_sources_ifdef(CONFIG_DISK_ACCESS_RAM disk_access_ram.c)
zephyr_sources_ifdef(CONFIG_DISK_ACCESS_SPI_SDHC disk_access_spi_sdhc.c)
zephyr_sources_ifdef(CONFIG_DISK_ACCESS_STM32_SDMMC disk_access_stm32_sdmmc.c)
zephyr_sources_ifdef(CONFIG_DISK_ACCESS_USDHC disk_access_usdhc.c)
zephyr_sources_ifdef(CONFIG_DISK_ACCESS_SD disk_access_sd.c)
zephyr_sources_ifdef(CONFIG_DISK_ACCESS_SPINAND disk_access_spinand.c)
