# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_ACTS_LARK_BATTERY_SUPPLY_INTERNAL         battery_lark_internal)
add_subdirectory_ifdef(CONFIG_ACTS_LARK_BATTERY_SUPPLY_EXTERNAL         battery_lark_external)
add_subdirectory_ifdef(CONFIG_ACTS_LEOPARD_BATTERY_SUPPLY_INTERNAL         battery_leopard_internal)
add_subdirectory_ifdef(CONFIG_ACTS_LEOPARD_BATTERY_SUPPLY_EXTERNAL         battery_leopard_external)
