# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_sources(
  soc.c
  soc_pm.c
  soc_reset.c
  soc_clock.c
  soc_pinmux.c
  soc_pmu.c
  spicache.c
  soc_boot.c
  soc_se_trng.c
  soc_memctrl.c
  soc_ppi.c
  soc_pstore.c
  soc_psram.c
  soc_image_head.c
  soc_sleep.S
  soc_powergate.c
)
zephyr_sources_ifdef(CONFIG_SYS_POWER_MANAGEMENT power.c soc_sleep.c)
zephyr_sources_ifdef(CONFIG_PM_POLICY_APP policy_sleep.c)
zephyr_sources_ifdef(CONFIG_SOC_SPICACHE_PROFILE_STAT cacheprofile_stat.c)

zephyr_library_sources_ifdef(CONFIG_SYS_WAKELOCK
    sys_wakelock.c
)

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/socatp/CMakeLists.txt)
    add_subdirectory(socatp)
else()
    zephyr_library_import(libsocatp ${CMAKE_CURRENT_LIST_DIR}/libsocatp.a)
endif()