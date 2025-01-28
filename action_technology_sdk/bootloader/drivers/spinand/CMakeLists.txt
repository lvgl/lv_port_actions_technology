# SPDX-License-Identifier: Apache-2.0

if (CONFIG_SOC_LEOPARD)
if (CONFIG_SPINAND_LIB)
  if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/libspinand/CMakeLists.txt)
    add_subdirectory(libspinand)
  else()
    zephyr_library_import(libspinand ${CMAKE_CURRENT_LIST_DIR}/libspinand.a)
  endif()
endif()  #SPINAND_LIB
zephyr_library()
zephyr_library_sources_ifdef(CONFIG_SPINAND_ACTS spinand_acts.c)
endif()  #SOC_LEOPARD

