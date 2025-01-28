# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(
	CWM_Inc
	Inc
)

zephyr_sources(
	Src/sensor_algo.c
)

target_link_libraries(${ZEPHYR_CURRENT_LIBRARY} PRIVATE 
	${CMAKE_CURRENT_LIST_DIR}/CWM_Lib/CWM_LIB_DML_m4.a
	${CMAKE_CURRENT_LIST_DIR}/CWM_Lib/CWM_LIB_m4.a
)