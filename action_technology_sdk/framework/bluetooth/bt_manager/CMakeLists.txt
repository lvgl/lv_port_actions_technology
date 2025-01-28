#
# Copyright (c) 2020 Actions Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Actions
#

zephyr_library_sources(bt_manager.c)
zephyr_library_sources(bt_manager_config.c)
zephyr_library_sources(bt_manager_event.c)
zephyr_library_sources(bt_manager_check_mac_name.c)
if(CONFIG_BT_BR_ACTS)
zephyr_library_sources(bt_manager_discover.c)
zephyr_library_sources(bt_manager_connect.c)
zephyr_library_sources(bt_stream_manager.c)
endif()
zephyr_library_sources_ifdef(CONFIG_BT_A2DP bt_manager_a2dp.c)
zephyr_library_sources_ifdef(CONFIG_BT_AVRCP bt_manager_avrcp.c)
zephyr_library_sources_ifdef(CONFIG_BT_HFP_HF bt_manager_hfp.c)
zephyr_library_sources_ifdef(CONFIG_BT_HFP_HF bt_manager_sco.c)
zephyr_library_sources_ifdef(CONFIG_BT_HFP_AG bt_manager_hfp_ag.c)
zephyr_library_sources_ifdef(CONFIG_BT_SPP bt_manager_spp.c)
zephyr_library_sources_ifdef(CONFIG_BT_PBAP_CLIENT bt_manager_pbap.c)
zephyr_library_sources_ifdef(CONFIG_BT_MAP_CLIENT bt_manager_map.c)
zephyr_library_sources_ifdef(CONFIG_BT_HID bt_manager_hid.c)
zephyr_library_sources_ifdef(CONFIG_BT_BLE bt_manager_ble.c)
zephyr_library_sources_ifdef(CONFIG_BT_SPP bt_manager_sppble_stream.c)
zephyr_library_sources_ifdef(CONFIG_BT_BLE bt_manager_sppble_stream.c)
zephyr_library_sources_ifdef(CONFIG_TWS bt_manager_tws.c)
zephyr_library_sources_ifdef(CONFIG_BT_PTS_TEST bt_manager_pts_test.c)
zephyr_library_sources_ifdef(CONFIG_MGR_TEST_SAMPLE bt_manager_test_sample.c)
zephyr_library_sources_ifdef(CONFIG_SHELL_DBG bt_manager_shell.c)
zephyr_library_sources_ifdef(CONFIG_BT_ACTIONS_SUPER_SERVICE bt_manager_super_service.c)
zephyr_library_sources_ifdef(CONFIG_BT_PNP_INFO_SEARCH bt_manager_pnp.c)
zephyr_library_sources_ifdef(CONFIG_BT_BLE_IPS bt_manager_ips.c)

add_subdirectory_ifdef(CONFIG_BT_ANCS_AMS ancs_ams)
add_subdirectory_ifdef(CONFIG_BLE_MASTER ble_master)
