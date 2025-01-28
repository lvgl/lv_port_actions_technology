# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_SE se.c)
zephyr_library_sources_ifdef(CONFIG_AES_ACTS aes.c)
zephyr_library_sources_ifdef(CONFIG_CRC_ACTS crc.c)
