#!/usr/bin/env python3
#
# Build Actions NVRAM config binary file
#
# Copyright (c) 2017 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0
#

import os
import sys
import time
import struct
import argparse
import platform
import subprocess

RES_CHKSUM_OFFSET = 28
APP_CHKSUM_OFFSET = 28

def res_read_chksum(res_bin_f):
    chksum = b'\x00\x00\x00\x00'
    with open(res_bin_f, 'rb+') as f:
        f.seek(RES_CHKSUM_OFFSET, 0)
        chksum = f.read(4)
        f.close()
    return chksum

def app_write_chksum(app_bin_f, chksum, res_index):
    with open(app_bin_f, 'rb+') as f:
        f.seek(APP_CHKSUM_OFFSET + (res_index * 4), 0)
        f.write(chksum)
        f.close()
        print('app_write_chksum: [%d] chksum: %s' %(res_index, chksum.hex()))

def pack_res_chksum(app_bin_f, res_bin_name):
    bin_dir = os.path.dirname(app_bin_f)
    res_bin_f = os.path.join(bin_dir, res_bin_name)
    if os.path.exists(res_bin_f):
        chksum = res_read_chksum(res_bin_f)

        if(res_bin_name == "res.bin"):
            res_index = 0
        else:
            return

        app_write_chksum(app_bin_f, chksum, res_index)
        print('%s -> %s: %s' %(res_bin_f, app_bin_f, chksum.hex()))

def main(argv):

    parser = argparse.ArgumentParser(
        description='Build firmware',
    )
    parser.add_argument('-b', dest = 'app_bin_dst_path')
    parser.add_argument('-r', dest = 'res_file')

    args = parser.parse_args();

    app_bin_dst_path = args.app_bin_dst_path
    res_file = args.res_file

    pack_res_chksum(app_bin_dst_path, res_file)

if __name__ == '__main__':
    main(sys.argv[1:])
