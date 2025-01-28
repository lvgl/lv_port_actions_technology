# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_SPP_TEST_SUPPORT spp_test)

target_sources_ifdef(CONFIG_TOOL app PRIVATE tool_app_main.c)
target_sources_ifdef(CONFIG_TOOL app PRIVATE tool_uart.c)
target_sources_ifdef(CONFIG_TOOL_ASET app PRIVATE tool_aset.c)
target_sources_ifdef(CONFIG_TOOL_ECTT app PRIVATE tool_ectt.c)
target_sources_ifdef(CONFIG_ACTIONS_ATT app PRIVATE tool_att.c)
