#!/bin/bash -e
#
# Copyright (c) 2017 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0
#

SDK_ROOT=`pwd`

source ${SDK_ROOT}/../../../../../.build_config

echo "BOARD = ${BOARD}"

export BOARD

Firmware_Release_Dir=${SDK_ROOT}/../../../../../../Firmware_Release_${BOARD}

echo "Firmware_Release_Dir = ${Firmware_Release_Dir}"

export Firmware_Release_Dir

print_usage() {
	echo "Usage:"
	echo "  Build firmware"
	echo "    $0"
	echo "  Clean build out directory"
	echo "    $0 -c"
	echo
}

# build

if [ $# = 1 ]; then
# clean build directory?
if [ "$1" = "-c" ]; then
	BUILD_CLEAN=1
else
	print_usage
	exit 1
fi
fi

if [ "${BUILD_CLEAN}" = "1" ]; then
	make clean
	exit 0
fi

make
