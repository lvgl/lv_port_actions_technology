# SPDX-License-Identifier: Apache-2.0

target_sources(app PRIVATE card_reader_main.c)
target_sources_ifdef(CONFIG_UI_MANAGER app PRIVATE card_reader_view.c)

