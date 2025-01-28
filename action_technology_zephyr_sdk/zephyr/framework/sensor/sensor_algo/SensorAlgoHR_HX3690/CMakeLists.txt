# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(
	HX3690_Port
	Inc
)

zephyr_sources(
	HX3690_Port/hx3690l.c
	HX3690_Port/hx3690l_check_touch.c
	HX3690_Port/hx3690l_hrs_agc.c
	HX3690_Port/hx3690l_spo2_agc.c
	HX3690_Port/hx3690q_factory_test.c
)

zephyr_sources(
	Src/hr_algo.c
)

target_link_libraries(${ZEPHYR_CURRENT_LIBRARY} PRIVATE 
	${CMAKE_CURRENT_LIST_DIR}/HX3690_Lib/m33_hx3690l_hrs_spo2_hrv.a
)
