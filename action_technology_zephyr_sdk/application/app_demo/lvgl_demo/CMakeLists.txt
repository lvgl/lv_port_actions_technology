# SPDX-License-Identifier: Apache-2.0

#list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/../../modules/framework)


cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lvgl_demo)

add_subdirectory(src)

if(CONFIG_STACK_CANARIES)
	zephyr_library_compile_options(-DAPP_BT_WATCH -fstack-protector-all)
endif()
