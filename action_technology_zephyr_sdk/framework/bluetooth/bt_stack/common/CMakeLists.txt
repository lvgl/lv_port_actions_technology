#
# Copyright (c) 2020 Actions Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Actions
#

zephyr_library_sources(dummy.c)
zephyr_library_sources(log.c)

zephyr_library_sources_ifdef(CONFIG_BT_RPA   rpa.c)

