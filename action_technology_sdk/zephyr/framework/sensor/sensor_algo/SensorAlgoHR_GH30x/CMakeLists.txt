# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(
	GH3011_Port
	Inc
)

zephyr_sources(
	GH3011_Port/gh3011_example_comm_pkg.c
	GH3011_Port/gh3011_example_ctrl.c
	GH3011_Port/gh3011_example_process.c
	GH3011_Port/gh3011_example_reg_array.c
)

zephyr_sources(
	Src/gh3011_port.c
	Src/hr_algo.c
)

target_link_libraries(${ZEPHYR_CURRENT_LIBRARY} PRIVATE 
	${CMAKE_CURRENT_LIST_DIR}/GH3011_Lib/hbd_communicate_M4.a
	${CMAKE_CURRENT_LIST_DIR}/GH3011_Lib/hbd_ctrl_M4.a
)
