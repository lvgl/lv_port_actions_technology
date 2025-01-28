# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

target_sources(app PRIVATE video_player.c)
target_sources(app PRIVATE video_dec_srv.c)
target_sources(app PRIVATE adpcm.c)
target_sources(app PRIVATE adpcm_plugin.c)
target_sources(app PRIVATE avi_demuxer.c)
target_sources(app PRIVATE avi_plugin.c)
target_sources(app PRIVATE mjpeg_plugin.c)
target_sources(app PRIVATE video_mem.c)
