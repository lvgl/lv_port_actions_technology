# SPDX-License-Identifier: Apache-2.0


zephyr_library_sources_ifdef(CONFIG_BT_DIS dis.c)

zephyr_library_sources_ifdef(CONFIG_BT_BAS bas.c)

zephyr_library_sources_ifdef(CONFIG_BT_HIDS hog.c)

zephyr_library_sources_ifdef(CONFIG_BT_HRS hrs.c)

zephyr_library_sources_ifdef(CONFIG_BT_TPS tps.c)

add_subdirectory_ifdef(CONFIG_BT_OTS ots)
