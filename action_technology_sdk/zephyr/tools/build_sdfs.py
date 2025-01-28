#!/usr/bin/env python3
#
# Build Actions sdfs image
#
# Copyright (c) 2017 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0
#

import os
import sys
import struct
import argparse
import platform
import subprocess

script_path = os.path.split(os.path.realpath(__file__))[0]

def run_cmd(cmd):
    #print(cmd)
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = p.communicate()
    #print("%s" % (output.rstrip()))
    return (output, p.returncode)

def is_windows():
    sysstr = platform.system()
    if (sysstr.startswith('Windows') or \
       sysstr.startswith('MSYS') or     \
       sysstr.startswith('MINGW') or    \
       sysstr.startswith('CYGWIN')):
        return 1
    else:
        return 0

def main(argv):
    parser = argparse.ArgumentParser(
        description='Build sdfs image (sdfs)',
    )
    parser.add_argument('-o', dest = 'output_file', required=True)
    parser.add_argument('-d', dest = 'sdfs_dir', required=True)
    args = parser.parse_args();

    print('SDFS: Build sdfs image (sdfs)')

    if (is_windows()):
        # windows
        make_sdfs_path = script_path + '/utils/windows/make_sdfs.exe'
    else:
        if ('32bit' == platform.architecture()[0]):
            # linux x86
            make_sdfs_path = script_path + '/utils/linux-x86/make_sdfs'
        else:
            # linux x86_64
            make_sdfs_path = script_path + '/utils/linux-x86_64/make_sdfs'

    cmd = [make_sdfs_path, args.sdfs_dir, args.output_file]

    (outmsg, exit_code) = run_cmd(cmd)
    if exit_code !=0:
        print('make fatfs error')
        print(outmsg.decode('utf-8'))
        sys.exit(1)

    print('SDFS: Generate sdfs file: %s.' %args.output_file)

if __name__ == "__main__":
    main(sys.argv)
