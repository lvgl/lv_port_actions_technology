# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_SYSTEM)

set(ZEPHYR_CURRENT_LIBRARY acts_framework_system)

add_library(acts_framework_system INTERFACE)

zephyr_include_directories(
	.
	include
)

target_include_directories(acts_framework_system INTERFACE
	include
)

zephyr_library()

zephyr_library_sources(
    system_init.c
)

zephyr_library_sources_ifdef(CONFIG_CORE
    app_switcher.c
    sys_event.c
    sys_manager.c
    sys_monitor.c
    sys_power_off.c
)

zephyr_library_sources_ifdef(CONFIG_SYS_STANDBY
    sys_standby.c
)

zephyr_library_sources_ifdef(CONFIG_ESD_MANAGER
    esd_manager.c
)

zephyr_library_sources_ifdef(CONFIG_SYSTEM_SHELL
    sys_shell.c
)

zephyr_library_sources_ifdef(CONFIG_TASK_WDT
	task_wdt_manager.c
)

add_subdirectory_ifdef(CONFIG_PLAYTTS tts)
add_subdirectory_ifdef(CONFIG_POWER power)
add_subdirectory_ifdef(CONFIG_INPUT input)
add_subdirectory_ifdef(CONFIG_HOTPLUG hotplug)
add_subdirectory_ifdef(CONFIG_MUTIPLE_VOLUME_MANAGER fs_manager)
add_subdirectory_ifdef(CONFIG_ALARM_MANAGER alarm_manager)
add_subdirectory_ifdef(CONFIG_ACTLOG act_log)

zephyr_library_link_libraries(acts_framework_system)

zephyr_library_link_libraries_ifdef(CONFIG_FAT_FILESYSTEM_ELM ELMFAT INTERFACE)

zephyr_library_link_libraries_ifdef(CONFIG_FILE_SYSTEM_LITTLEFS LITTLEFS INTERFACE)

target_link_libraries(acts_framework_system INTERFACE zephyr_interface)

endif()