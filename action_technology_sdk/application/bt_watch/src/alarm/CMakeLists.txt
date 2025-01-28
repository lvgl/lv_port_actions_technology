# SPDX-License-Identifier: Apache-2.0

target_sources(app PRIVATE alarm_main.c)
target_sources(app PRIVATE alarm_fun.c)
target_sources(app PRIVATE alarm_event.c)
target_sources(app PRIVATE alarm_file_selector.c)
target_sources_ifdef(CONFIG_UI_MANAGER app PRIVATE alarm_msgbox_view.c)

