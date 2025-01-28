# Copyright(C) 2020 Actions Corporation.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ARCH_DIR}/${ARCH}/include
)

zephyr_library_sources_ifdef(CONFIG_AUDIO_OUT_ACTS audio_out_acts.c)
zephyr_library_sources_ifdef(CONFIG_AUDIO_IN_ACTS audio_in_acts.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_LARK audio_acts_utils.c)

add_subdirectory_ifdef(CONFIG_AUDIO_DRIVER_SHELL shell)
add_subdirectory_ifdef(CONFIG_SOC_SERIES_LARK phy_lark)
add_subdirectory_ifdef(CONFIG_SOC_SERIES_LEOPARD phy_leopard)
add_subdirectory_ifdef(CONFIG_BOARD_EXTERNAL_PA_ENABLE phy_external_pa)
