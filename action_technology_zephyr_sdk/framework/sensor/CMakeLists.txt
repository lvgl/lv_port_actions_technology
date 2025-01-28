# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_SENSOR_MANAGER)

set(ZEPHYR_CURRENT_LIBRARY acts_sensor)

add_library(acts_sensor INTERFACE)

zephyr_include_directories(
	.
	include
)

target_include_directories(acts_sensor INTERFACE
	include
)

zephyr_library()

zephyr_library_sources_ifdef(
	CONFIG_SENSOR_MANAGER
    sensor_manager.c
)

zephyr_library_sources_ifdef(
	CONFIG_SENSOR_SERVICE
    sensor_service.c
    sensor_sleep.c
    sensor_port.c
    algo_port.c
)

zephyr_library_sources_ifdef(
    CONFIG_SENSOR_GPS_SERVICE 
    gps_service.c
)

zephyr_library_sources_ifdef(
    CONFIG_GPS_MANAGER 
    gps_manager.c
)

add_subdirectory_ifdef(CONFIG_SENSOR_GPS_SERVICE minmea)

zephyr_library_link_libraries(acts_sensor)
target_link_libraries(acts_sensor INTERFACE zephyr_interface)

if(CONFIG_STACK_CANARIES)
	zephyr_library_compile_options(-DFRAMEWORK_SENSOR -fstack-protector-all)
endif()

endif()


