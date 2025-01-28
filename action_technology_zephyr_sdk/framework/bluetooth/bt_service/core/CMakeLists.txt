#
# Copyright (c) 2020 Actions Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Actions
#

zephyr_library_sources(btsrv_adapter.c)
zephyr_library_sources(btsrv_message.c)
zephyr_library_sources(btsrv_utils.c)
zephyr_library_sources(btsrv_storage.c)
if(CONFIG_BT_BR_ACTS)
zephyr_library_sources(btsrv_rdm.c)
zephyr_library_sources(btsrv_connect.c)
zephyr_library_sources(btsrv_scan.c)
zephyr_library_sources(btsrv_link_adjust.c)
zephyr_library_sources(btsrv_sniff.c)
zephyr_library_sources(btsrv_pts_test.c)
endif()
