# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0
zephyr_library_include_directories(${CMAKE_CURRENT_LIST_DIR})
zephyr_library_sources_ifdef(CONFIG_ACTLOG_OUTPUT_BINARY  ef_log.c ef_port.c)
