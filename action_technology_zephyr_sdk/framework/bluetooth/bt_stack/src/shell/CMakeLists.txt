#
# Copyright (c) 2021 Actions Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Actions
#

zephyr_library_sources(bt_test_sample.c bt_test_data.c bt.c)
zephyr_library_sources_ifdef(CONFIG_BT_CONN gatt.c)
zephyr_library_sources_ifdef(CONFIG_BT_ISO iso.c)
