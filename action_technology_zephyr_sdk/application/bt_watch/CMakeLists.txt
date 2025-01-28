# SPDX-License-Identifier: Apache-2.0

#list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/../../modules/framework)


cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bt_watch)

target_include_directories(app PRIVATE
	./prebuild/bt_production_test
)

target_link_libraries(app INTERFACE ${CMAKE_CURRENT_LIST_DIR}/prebuild/bt_production_test/libbt_test.a)

add_subdirectory(src)

if(CONFIG_STACK_CANARIES)
	zephyr_library_compile_options(-DAPP_BT_WATCH -fstack-protector-all)
endif()
