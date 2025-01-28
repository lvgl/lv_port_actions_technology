# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_AUDIO)

set(ZEPHYR_CURRENT_LIBRARY acts_audio)

add_library(acts_audio INTERFACE)

zephyr_include_directories(
	.
	include
)

target_include_directories(acts_audio INTERFACE
	include
)

zephyr_library()

zephyr_library_sources(
    audio_aps.c
    audio_policy.c
    audio_record.c
    audio_system.c
    audio_track.c
)
zephyr_library_sources_ifdef(CONFIG_VOLUME_MANAGER
    volume_manager.c
)
zephyr_library_sources_ifdef(CONFIG_TWS
    audio_tws_aps.c
)


zephyr_library_link_libraries(acts_audio)

zephyr_library_link_libraries_ifdef(CONFIG_FAT_FILESYSTEM_ELM ELMFAT INTERFACE)

target_link_libraries(acts_audio INTERFACE zephyr_interface)

endif()

