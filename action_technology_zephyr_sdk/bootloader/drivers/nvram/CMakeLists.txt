# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_NVRAM_CONFIG nvram_config.c nvram_storage.c)

