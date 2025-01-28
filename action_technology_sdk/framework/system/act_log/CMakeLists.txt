# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_ACTLOG    act_log.c act_log_shell.c act_log_test.c)
#zephyr_library_sources_ifdef(CONFIG_ACTLOG    act_log_mem.c)
zephyr_library_sources_ifdef(CONFIG_ACTLOG    act_log_output_backend.c)
zephyr_library_sources_ifdef(CONFIG_ACTLOG_OUTPUT_BINARY  act_log_output_binary.c act_log_flash_buffer.c)
zephyr_library_sources_ifdef(CONFIG_ACTLOG_OUTPUT_FLOW  act_log_output_flow.c)
zephyr_library_sources_ifdef(CONFIG_ACTLOG_OUTPUT_FILE  act_log_output_file.c)

add_subdirectory_ifdef(CONFIG_ACTLOG_OUTPUT_BINARY easyflash)