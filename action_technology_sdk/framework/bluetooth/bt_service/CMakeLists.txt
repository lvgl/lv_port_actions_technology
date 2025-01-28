#
# Copyright (c) 2020 Actions Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Actions
#

if (CONFIG_ACTS_BT)
  target_include_directories(acts_bluetooth INTERFACE
    .
	./include
  )

  zephyr_library_sources(btsrv_main.c)
  add_subdirectory(core)
  add_subdirectory(btif)
 if(CONFIG_BT_BR_ACTS)
  add_subdirectory(profile)
 endif()

endif()
