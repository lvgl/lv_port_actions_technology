# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(
	HX3605_Port
	Inc
)

zephyr_sources(
	HX3605_Port/hx3605.c
	HX3605_Port/hx3605_factory_test.c
	HX3605_Port/hx3605_hrs_agc.c
	HX3605_Port/hx3605_hrv_agc.c
	HX3605_Port/hx3605_spo2_agc.c
)

zephyr_sources(
	Src/hr_algo.c
)

target_link_libraries(${ZEPHYR_CURRENT_LIBRARY} PRIVATE 
	${CMAKE_CURRENT_LIST_DIR}/HX3605_Lib/m33_3605_small_lib_hrs_spo2.a
)
