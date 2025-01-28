# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_ACTIONS_FRAMEWORK)

set(ZEPHYR_CURRENT_LIBRARY acts_framework_base)

add_library(acts_framework_base INTERFACE)

zephyr_include_directories(
	include
	include/core
	include/utils
	include/utils/stream
	include/utils/iterator
)

target_include_directories(acts_framework_base INTERFACE
	include
	include/core
	include/utils
	include/utils/stream
	include/utils/iterator
)

zephyr_library()

add_subdirectory_ifdef(CONFIG_CORE core)
add_subdirectory_ifdef(CONFIG_PROPERTY property)
add_subdirectory_ifdef(CONFIG_MEMORY memory)
add_subdirectory_ifdef(CONFIG_ACTIONS_UTILS utils)

zephyr_library_link_libraries(acts_framework_base)

zephyr_library_link_libraries_ifdef(CONFIG_FAT_FILESYSTEM_ELM ELMFAT INTERFACE)

target_link_libraries(acts_framework_base INTERFACE zephyr_interface)

endif()
