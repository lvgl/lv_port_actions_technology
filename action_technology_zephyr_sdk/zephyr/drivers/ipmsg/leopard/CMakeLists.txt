# Copyright(C) 2021 Actions Corporation.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_IPMSG_ACTS_BTC ipmsg_btc.c)
