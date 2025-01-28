# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_library_sources_ifdef(CONFIG_SENSOR_GPS_SERVICE minmea.c)


