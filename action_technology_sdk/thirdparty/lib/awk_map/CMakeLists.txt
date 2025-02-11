# SPDX-License-Identifier: Apache-2.0

if (CONFIG_AWK_LIB)

set(ZEPHYR_CURRENT_LIBRARY awk_lib)

add_library(awk_lib INTERFACE)

zephyr_include_directories(
	include
)

target_include_directories(awk_lib INTERFACE
	include
)

zephyr_ld_options(-Wl,--no-wchar-size-warning)
zephyr_library()

zephyr_library_link_libraries(awk_lib)
target_link_libraries(awk_lib INTERFACE zephyr_interface)

zephyr_library_import(libWatchSDK ${CMAKE_CURRENT_LIST_DIR}/lib/libWatchSDK.a)

add_subdirectory(adapter)
add_subdirectory(srv)
#add_subdirectory(libzip)

endif()