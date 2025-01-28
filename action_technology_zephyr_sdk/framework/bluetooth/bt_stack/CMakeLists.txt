#
# Copyright (c) 2020 Actions Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Actions
#

if (CONFIG_ACTS_BT)
  zephyr_include_directories(./include)

  target_include_directories(acts_bluetooth INTERFACE
	.
	./src/inc
  )

  add_subdirectory(src)
  add_subdirectory(common)
  add_subdirectory_ifdef(CONFIG_BT_CONN services)

  if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/libbt_stack/CMakeLists.txt)
    add_subdirectory(libbt_stack)
  else()
    zephyr_library_include_directories(${CMAKE_CURRENT_LIST_DIR})
    zephyr_library_import(bt_stack ${CMAKE_CURRENT_LIST_DIR}/libbt_stack.a)
  endif()
endif()
