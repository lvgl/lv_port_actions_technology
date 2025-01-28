# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(
	.
)
target_sources(app PRIVATE btcall_main.c)
target_sources(app PRIVATE btcall_event.c)
target_sources(app PRIVATE btcall_media.c)
target_sources(app PRIVATE btcall_ring.c)
