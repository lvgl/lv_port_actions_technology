# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_OTA)

set(ZEPHYR_CURRENT_LIBRARY acts_ota)

add_library(acts_ota INTERFACE)

zephyr_include_directories(
	.
	include
)

target_include_directories(acts_ota INTERFACE
	include
)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_OTA_BACKEND_BLUETOOTH
    ota_backend_bt.c
)
zephyr_library_sources_ifdef(CONFIG_OTA_BACKEND_SDCARD
    ota_backend_sdcard.c
)
zephyr_library_sources_ifdef(CONFIG_OTA_BACKEND_TEMP_PART
    ota_backend_temp_part.c
)
zephyr_library_sources_ifdef(CONFIG_OTA_STORAGE_FS
    ota_storage_fs.c
)
zephyr_library_sources_ifndef(CONFIG_OTA_STORAGE_FS
    ota_storage.c
)
zephyr_library_sources_ifdef(CONFIG_SPP_TEST_SUPPORT
    spp_test_backend.c
)
zephyr_library_sources_ifdef(CONFIG_OTA_PRODUCT_SUPPORT
    ota_trans_bt.c
)
zephyr_library_sources_ifdef(CONFIG_OTA_BLE_MASTER_SUPPORT
    ota_trans_bt.c
)

add_subdirectory(libota)
add_subdirectory_ifdef(CONFIG_OTA_LZMA minlzma)

zephyr_library_link_libraries(acts_ota)

zephyr_library_link_libraries_ifdef(CONFIG_FAT_FILESYSTEM_ELM ELMFAT INTERFACE)

target_link_libraries(acts_ota INTERFACE zephyr_interface)


endif()


