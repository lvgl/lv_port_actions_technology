#
# Copyright (c) 2020 Actions Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Actions
#

add_subdirectory_ifdef(CONFIG_SENSOR_HAL	sensor_hal)
add_subdirectory_ifdef(CONFIG_SENSOR_ALGO	sensor_algo)
