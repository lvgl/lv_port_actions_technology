# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(bat_charge_driver.c bat_charge_ctrl.c bat_charge_status.c)
add_subdirectory_ifdef(CONFIG_ACTS_BATTERY_DRIVER_SHELL shell)
