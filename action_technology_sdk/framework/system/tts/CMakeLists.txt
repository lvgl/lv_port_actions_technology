# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0


zephyr_library_sources(
    tts_manager.c
)

zephyr_library_sources_ifdef(CONFIG_PLAY_KEYTONE
	key_tone.c
)
