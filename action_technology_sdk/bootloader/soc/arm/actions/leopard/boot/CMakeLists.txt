# SPDX-License-Identifier: Apache-2.0

SET(LIB_NAME boot)
SET(ZEPHYR_CURRENT_LIBRARY ${LIB_NAME})
SET(LIBRARY_OUTPUT_PATH "${CMAKE_CURRENT_LIST_DIR}/../")
zephyr_library_named(${LIB_NAME})

zephyr_include_directories(./)
zephyr_library_sources(
  soc_boot_priv.c
  libboot_version.c
)

target_link_libraries(${LIB_NAME} INTERFACE zephyr_interface)