# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(include)

add_subdirectory(config)
add_subdirectory_ifdef(CONFIG_TOOL usp)
add_subdirectory_ifdef(CONFIG_TOOL_ASET aset)
add_subdirectory_ifdef(CONFIG_TOOL stub)