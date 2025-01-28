#
# Copyright (c) 2020 Actions Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Actions
#

zephyr_library_sources(btif_base.c)
if(CONFIG_BT_BR_ACTS)
zephyr_library_sources(btif_a2dp.c)
zephyr_library_sources(btif_avrcp.c)
zephyr_library_sources(btif_hfp.c)
zephyr_library_sources(btif_hfp_ag.c)
zephyr_library_sources(btif_hid.c)
zephyr_library_sources(btif_map.c)
zephyr_library_sources(btif_pbap.c)
zephyr_library_sources(btif_pts_test.c)
zephyr_library_sources(btif_spp.c)
zephyr_library_sources(btif_tws.c)
zephyr_library_sources(btif_pnp.c)
endif()