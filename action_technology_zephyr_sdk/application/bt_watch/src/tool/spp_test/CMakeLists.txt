# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

target_sources(app PRIVATE spp_test_background.c)
target_sources(app PRIVATE show_fw_version.c)
target_sources(app PRIVATE abtp_entry.c)
target_sources(app PRIVATE spp_mic_upload.c)
target_sources(app PRIVATE spp_audio_play.c)
target_sources(app PRIVATE mic_test_entry.c)
