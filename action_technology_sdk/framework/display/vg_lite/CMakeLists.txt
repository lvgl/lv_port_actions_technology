# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_VG_LITE)

zephyr_include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../include/vg_lite
)

zephyr_library_sources(vglite_mem.c vglite_util.c)

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/verisilicon/CMakeLists.txt)
  add_subdirectory(verisilicon)
else()
  zephyr_library_include_directories(${CMAKE_CURRENT_LIST_DIR})
  zephyr_library_import(vg_lite ${CMAKE_CURRENT_LIST_DIR}/libvg_lite.a)
endif()

endif() # CONFIG_VG_LITE
