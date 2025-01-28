# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_LIS2DW12          lis2dw12)
add_subdirectory_ifdef(CONFIG_LPS22HH		lps22hh)
add_subdirectory_ifdef(CONFIG_LSM6DSO		lsm6dso)

if(CONFIG_USERSPACE OR CONFIG_SENSOR_SHELL OR CONFIG_SENSOR_SHELL_BATTERY)
# The above if() is needed or else CMake would complain about
# empty library.

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_USERSPACE sensor_handlers.c)
zephyr_library_sources_ifdef(CONFIG_SENSOR_SHELL sensor_shell.c)
zephyr_library_sources_ifdef(CONFIG_SENSOR_SHELL_BATTERY shell_battery.c)

endif()
