# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_COMPENSATION_ACTS compensation.c)
zephyr_library_sources_ifdef(CONFIG_TEMP_COMPENSATION_ACTS TempComp/temperature_compensation.c)
