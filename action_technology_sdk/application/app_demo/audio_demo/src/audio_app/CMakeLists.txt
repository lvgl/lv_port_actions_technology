# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

target_sources(app PRIVATE audio_app.c)

target_sources(app PRIVATE pcm_player.c)

target_sources(app PRIVATE mic_recorder.c)
