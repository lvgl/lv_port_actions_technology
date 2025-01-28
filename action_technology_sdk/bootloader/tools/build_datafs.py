#!/usr/bin/env python3
#
# Build Actions datafs image
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
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = p.communicate()
#    print("%s" % (output.rstrip()))
    return (output, p.returncode)

def dd_file(input_file, output_file, block_size=1, count=None, seek=None, skip=None):
    """Wrapper around the dd command"""
    cmd = [
        "dd", "if=%s" % input_file, "of=%s" % output_file,
        "bs=%s" % block_size, "conv=notrunc"]
    if count is not None:
        cmd.append("count=%s" % count)
    if seek is not None:
        cmd.append("seek=%s" % seek)
    if skip is not None:
        cmd.append("skip=%s" % skip)
    (_, exit_code) = run_cmd(cmd)

def new_file(filename, filesize, fillbyte = 0xff):
    with open(filename, 'wb') as f:
        f.write(bytearray([fillbyte]*filesize))

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
        description='Build datafs image (fatfs)',
    )
    parser.add_argument('-o', dest = 'output_file', required=True)
    parser.add_argument('-d', dest = 'datafs_dir', required=True)
    parser.add_argument('-s', dest = 'image_size', required=True, type=lambda x: hex(int(x,0)))
    args = parser.parse_args();

    print('DATAFS: Build datafs image (fatfs)')

    # generate empty image file
    new_file(args.output_file, int(args.image_size, 0), 0xff)

    if (is_windows()):
        # windows
        makebootfat_path = script_path + '/utils/windows/makebootfat.exe'
    else:
        if ('32bit' == platform.architecture()[0]):
            # linux x86
            makebootfat_path = script_path + '/utils/linux-x86/makebootfat'
        else:
            # linux x86_64
            makebootfat_path = script_path + '/utils/linux-x86_64/makebootfat'

    cmd = [makebootfat_path, '-b', script_path + '/utils/bootsect.bin', \
           '-o', args.output_file, args.datafs_dir]

    (outmsg, exit_code) = run_cmd(cmd)
    if exit_code !=0:
        print('make fatfs error')
        print(outmsg)
        sys.exit(1)

    print('DATAFS: Generate datafs file: %s.' %args.output_file)

if __name__ == "__main__":
    main(sys.argv)
