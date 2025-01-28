#!/usr/bin/env python3
#
# Build ATF image file
#
# Copyright (c) 2019 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0
#

import os
import sys
import struct
import argparse
import zlib

ATF_DATA_ALIGN_SIZE = 512
ATF_MAX_FILE_CNT = 15
ATF_VERSION = 0x01000000

'''
/* ATF image */
typedef struct
{
    uint8 magic[8];
    uint8 sdk_version[4];
    uint8 file_total;
    uint8 reserved[19];
    atf_dir_t atf_dir[15];
}atf_head_t;

typedef struct
{
    uint8 filename[12];
    uint8 reserved1[4];
    uint32 offset;
    uint32 length;
    uint8 reserved2[4];
    uint32 checksum;
}atf_dir_t;
'''

class atf_dir:
    def __init__(self):
        self.name = ''
        self.offset = 0
        self.length = 0
        self.aligned_length = 0
        self.checksum = 0
        self.data = None

atf_file_list = []
cur_data_offset = ATF_DATA_ALIGN_SIZE
atf_header = None

def atf_add_crc(filename):
    if os.path.isfile(filename):
        # update data crc
        with open(filename, 'rb+') as f:
            #skip magic header
            f.read(512)
            crc = zlib.crc32(f.read(), 0) & 0xffffffff
            #update crc field in header
            f.seek(24)
            f.write(struct.pack('<I',crc))

        # update head crc
        with open(filename, 'rb+') as f:
            #skip magic header
            f.read(12)
            crc = zlib.crc32(f.read(512 - 12), 0) & 0xffffffff
            #update crc field in header
            f.seek(8)
            f.write(struct.pack('<I',crc))

def atf_add_file(fpath):
    global atf_file_list, cur_data_offset

    if not os.path.isfile(fpath):
        print('ATF: file %s is not exist' %(fpath));
        return False

    with open(fpath, 'rb') as f:
        fdata = f.read()

    af = atf_dir()

    af.name = os.path.basename(fpath)
    # check filename: 8.3
    if len(af.name) > 12:
        print('ATF: file %s name is too long' %(fpath))
        return False

    af.length = len(fdata)
    if af.length == 0:
        print('ATF: file %s length is zero' %(fpath))
        return False

    padsize = ATF_DATA_ALIGN_SIZE - af.length % ATF_DATA_ALIGN_SIZE
    af.aligned_length = af.length + padsize

    af.data = fdata + bytearray(padsize)
    af.offset = cur_data_offset

    # caculate file crc
    af.checksum = zlib.crc32(fdata, 0) & 0xffffffff

    cur_data_offset = cur_data_offset + af.aligned_length

    atf_file_list.append(af)

    return True

def atf_gen_header():
    global atf_header
    atf_dir = bytearray(0)

    if len(atf_file_list) == 0:
        return;
    for af in atf_file_list:
        atf_dir = atf_dir + struct.pack('<12s4xII4xI', af.name.encode('utf8'), \
                              af.offset, af.length, af.checksum)

    total_len = cur_data_offset

    atf_header = bytearray("ACTTEST0", 'utf8') + \
                            struct.pack('<IB3xIII4x', 0, len(atf_file_list), ATF_VERSION, total_len, 0)

    atf_header = atf_header + atf_dir
    padsize = ATF_DATA_ALIGN_SIZE - len(atf_header) % ATF_DATA_ALIGN_SIZE
    atf_header = atf_header + bytearray(padsize)

def main(argv):
    global atf_header

    parser = argparse.ArgumentParser(
        description='Build ATT firmware image',
    )
    parser.add_argument('-o', dest = 'output_file')
    parser.add_argument('input_files', nargs = '*')
    args = parser.parse_args();

    print('ATF: Build ATT firmware image: %s' %args.output_file)

    if len(args.input_files) > ATF_MAX_FILE_CNT:
        print('ATF: too much input files')
        sys.exit(1)

    for input_file in args.input_files:
        if not os.path.isfile(input_file):
            continue

        print('ATF: Add file: %s' %input_file)
        if atf_add_file(input_file) != True:
            sys.exit(1)

    atf_gen_header()

    # write the merged property file
    with open(args.output_file, 'wb+') as f:
        f.write(atf_header)
        for af in atf_file_list:
            f.write(af.data)

    atf_add_crc(args.output_file)
    
if __name__ == "__main__":
    main(sys.argv)
