#
# Copyright (c) 2020 Actions Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Actions
#

zephyr_include_directories_ifdef(
  CONFIG_SENSOR_HAL
  .
)

zephyr_sources_ifdef(
  CONFIG_SENSOR_HAL
	sensor_hal.c
	sensor_bus.c
	sensor_io.c
	sensor_dev.c
)

add_subdirectory_ifdef(CONFIG_SENSOR_HAL	devices)
