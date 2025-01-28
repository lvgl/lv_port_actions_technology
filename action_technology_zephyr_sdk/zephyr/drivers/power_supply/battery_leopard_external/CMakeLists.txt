# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(bat_charge_driver.c  bat_charge_status.c  bat_charge_cw6305.c)

add_subdirectory_ifdef(CONFIG_ACTS_BATTERY_SUPPLY_EXT_COULOMETER  coulometer)
add_subdirectory_ifdef(CONFIG_ACTS_BATTERY_DRIVER_SHELL shell)
