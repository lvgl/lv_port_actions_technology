#
# Copyright (c) 2020 Actions Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Actions
#

if (CONFIG_ACTS_BT)
  if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/libbt_trans/CMakeLists.txt)
    add_subdirectory(libbt_trans)
  else()
    zephyr_library_include_directories(${CMAKE_CURRENT_LIST_DIR})
    zephyr_library_import(bt_trans ${CMAKE_CURRENT_LIST_DIR}/libbt_trans.a)
  endif()
endif()
