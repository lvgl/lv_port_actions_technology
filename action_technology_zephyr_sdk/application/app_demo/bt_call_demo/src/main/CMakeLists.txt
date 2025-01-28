# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

target_sources(app PRIVATE system_app_main.c)

target_sources(app PRIVATE system_app_shell.c)

target_sources(app PRIVATE system_audio_policy.c)

target_sources(app PRIVATE btcall_app.c)
