# SPDX-License-Identifier: Apache-2.0

zephyr_library()
if(CONFIG_SOC_LEOPARD)
zephyr_library_sources_ifdef(CONFIG_SPI_FLASH_ACTS	spi_flash_leopard.c)
else()
zephyr_library_sources_ifdef(CONFIG_SPI_FLASH_ACTS	spi_flash_acts.c)
add_subdirectory_ifdef(CONFIG_SPI_NOR_FLASH_DRV_EXT spinor)
endif()
zephyr_library_sources_ifdef(CONFIG_FLASH_PAGE_LAYOUT flash_page_layout.c)
zephyr_library_sources_ifdef(CONFIG_SPINOR_TEST_DELAYCHAIN flash_test_delaychain.c)
zephyr_library_sources_ifdef(CONFIG_NOR_ACTS_DATA_PROTECTION_ENABLE flash_write_protection.c)
zephyr_library_sources_ifdef(CONFIG_NOR_ACTS_DQ_MODE_ENABLE flash_dq_mode.c)

