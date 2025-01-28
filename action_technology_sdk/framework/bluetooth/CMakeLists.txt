# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BLUETOOTH OR CONFIG_BT_STACK)

set(ZEPHYR_CURRENT_LIBRARY acts_bluetooth)

add_library(acts_bluetooth INTERFACE)

zephyr_include_directories(
	include
    bt_porting	
)

target_include_directories(acts_bluetooth INTERFACE
	include
	./bt_porting
)

zephyr_library()

add_subdirectory(bt_porting)
add_subdirectory_ifdef(CONFIG_BT_MANAGER bt_manager)
add_subdirectory_ifdef(CONFIG_BT_STACK bt_stack)
add_subdirectory_ifdef(CONFIG_BT_SERVICE bt_service)
add_subdirectory_ifdef(CONFIG_BT_A2DP_TRS bt_trans)

zephyr_library_link_libraries(acts_bluetooth)
zephyr_library_link_libraries_ifdef(CONFIG_FAT_FILESYSTEM_ELM ELMFAT INTERFACE)
target_link_libraries(acts_bluetooth INTERFACE zephyr_interface)

endif()
