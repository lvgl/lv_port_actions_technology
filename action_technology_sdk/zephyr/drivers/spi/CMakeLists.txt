# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_SPI_ACTS		spi_acts.c)
zephyr_library_sources_ifdef(CONFIG_SPIMT_LARK		spimt_lark.c)
zephyr_library_sources_ifdef(CONFIG_SPIMT_LEOPARD		spimt_leopard.c)
