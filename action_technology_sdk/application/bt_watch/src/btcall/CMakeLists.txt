# SPDX-License-Identifier: Apache-2.0

target_sources(app PRIVATE btcall_main.c)
target_sources(app PRIVATE btcall_event.c)
target_sources_ifdef(CONFIG_UI_MANAGER app PRIVATE btcall_msgbox_view.c)
target_sources(app PRIVATE btcall_media.c)
target_sources(app PRIVATE btcall_ring.c)
