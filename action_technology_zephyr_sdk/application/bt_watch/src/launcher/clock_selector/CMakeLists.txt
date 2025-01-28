# SPDX-License-Identifier: Apache-2.0

target_sources(app PRIVATE clock_selector.c)

if(CONFIG_VG_LITE)
  target_sources(app PRIVATE clock_select_view.c)
else()
  target_sources(app PRIVATE clock_selector_view.c)
endif()
