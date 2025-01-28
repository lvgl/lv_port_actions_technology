# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(
	VCHR02_Inc
	Inc
)

zephyr_sources(
	VCHR02_Src/module_heart_vc_int.c
	VCHR02_Src/vcHr02DEBUG.c
	VCHR02_Src/vcHr02Hci.c
)

zephyr_sources(
	Src/hr_algo.c
)

target_link_libraries(${ZEPHYR_CURRENT_LIBRARY} PRIVATE 
	${CMAKE_CURRENT_LIST_DIR}/VCHR02_Lib/vcHrAlgoV623ForCM4F.a
	${CMAKE_CURRENT_LIST_DIR}/VCHR02_Lib/vcSpo2AlgoV151_VP60A2_B_v2_ForCM4F.a
	${CMAKE_CURRENT_LIST_DIR}/VCHR02_Lib/vcSportMotionAlgoV11ForCM4F.a
)
