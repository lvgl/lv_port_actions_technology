# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

SET(LIB_NAME minlzma)
SET(ZEPHYR_CURRENT_LIBRARY ${LIB_NAME})
#SET(LIBRARY_OUTPUT_PATH "${CMAKE_CURRENT_LIST_DIR}/../")

zephyr_include_directories(./)

zephyr_library_named(${LIB_NAME})

zephyr_library_compile_options(-DCC_${LIB_NAME} -O3)

zephyr_library_sources_ifdef(CONFIG_OTA_UPGRADE
    dictbuf.c
	inputbuf.c
	lzma2dec.c
	lzmadec.c
	rangedec.c
	xzcrc.c
	xzstream.c
)

target_link_libraries(${LIB_NAME} INTERFACE zephyr_interface)
