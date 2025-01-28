# SPDX-License-Identifier: Apache-2.0
if (CONFIG_GPS)
zephyr_include_directories(.)


zephyr_sources_ifdef(CONFIG_SENSOR_GPS_AG3352B gps_ag3352b.c)
zephyr_sources_ifdef(CONFIG_SENSOR_GPS_TG9255 gps_tg9255.c)
zephyr_sources_ifdef(CONFIG_SENSOR_GPS_CC1165W gps_cc1165w.c)
endif ()
