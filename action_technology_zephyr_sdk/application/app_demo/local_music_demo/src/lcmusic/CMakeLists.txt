# SPDX-License-Identifier: Apache-2.0
zephyr_include_directories(
	.
)

target_sources(app PRIVATE local_player.c)
target_sources(app PRIVATE lcmusic_event.c)
target_sources(app PRIVATE lcmusic_file_selector.c)
target_sources(app PRIVATE lcmusic_breakpoint.c)
target_sources(app PRIVATE lcmusic_app.c)