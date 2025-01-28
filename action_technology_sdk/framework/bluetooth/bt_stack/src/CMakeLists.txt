#
# Copyright (c) 2020 Actions Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Actions
#

zephyr_library_sources(common_internal.c)
zephyr_library_sources(hci_data_log.c)
zephyr_library_sources(host_interface.c)
zephyr_library_sources_ifdef(CONFIG_BT_SNOOP	btsnoop.c)
zephyr_library_sources_ifdef(CONFIG_ACTS_NET_BUF	buf.c)

add_subdirectory(bt_stack)
add_subdirectory_ifdef(CONFIG_ACTS_BT_SHELL shell)
