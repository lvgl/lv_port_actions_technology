# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(
	Silan_Inc
	Inc
)

zephyr_sources(
	Src/sensor_algo.c
	Src/sensor_port.c
	Src/SL_Watch_Pedo_Kcal_Wrist_Sleep_Sway_L_Application.c
)

target_link_libraries(${ZEPHYR_CURRENT_LIBRARY} PRIVATE 
	${CMAKE_CURRENT_LIST_DIR}/Silan_Lib/SL_Watch_Pedo_Kcal_Wrist_Sleep_Sway_L_Algorithm.a
)