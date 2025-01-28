# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

target_sources(app PRIVATE system_app_main.c)
target_sources(app PRIVATE system_app_launcher.c)
target_sources(app PRIVATE system_app_input.c)
target_sources(app PRIVATE system_app_event.c)
target_sources_ifdef(CONFIG_AUDIO app PRIVATE system_audio_policy.c)
target_sources(app PRIVATE system_tts_policy.c)
target_sources(app PRIVATE system_event_map.c)
target_sources(app PRIVATE system_fcc_entry.c)
target_sources(app PRIVATE system_app_shell.c)
target_sources_ifdef(CONFIG_UI_MANAGER app PRIVATE system_lpower_msgbox_view.c)
target_sources_ifdef(CONFIG_UI_MANAGER app PRIVATE system_welcome_view.c)
target_sources_ifdef(CONFIG_UI_MANAGER app PRIVATE system_goodbye_view.c)
target_sources_ifdef(CONFIG_UI_MANAGER app PRIVATE system_app_ui.c)
target_sources_ifdef(CONFIG_MONKEY_TEST app PRIVATE system_monkey_test.c)
