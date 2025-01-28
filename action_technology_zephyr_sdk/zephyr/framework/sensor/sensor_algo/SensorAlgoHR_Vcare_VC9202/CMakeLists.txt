# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(
	VC9202_Inc
)

zephyr_sources(
	Src/hr_algo.c
	VC9202_Src/vc9202_driver.c
	VC9202_Src/vc9202_user.c
	VC9202_Src/vc9202_driver_testmode.c
)

target_link_libraries(${ZEPHYR_CURRENT_LIBRARY} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/VC9202_Lib/lib_BIO_V141_GCCM33_Ubuntu1804_JuXin_01.a
    ${CMAKE_CURRENT_LIST_DIR}/VC9202_Lib/lib_hr_v637_GccM33_Ubuntu104_JuXin_01.a
    ${CMAKE_CURRENT_LIST_DIR}/VC9202_Lib/lib_SP_int_GccM33_Ubuntu1804_JuXin_01.a
    ${CMAKE_CURRENT_LIST_DIR}/VC9202_Lib/lib_spo2_9202VP60A2_Int_GccM33_Ubuntu1804_JuXIn.a
)


