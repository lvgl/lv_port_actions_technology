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
import struct
import argparse
import zlib
#import crc8

NVRAM_REGION_SEG_MAGIC	= 0x5253564E
NVRAM_REGION_ITEM_MAGIC	= 0x49
NVRAM_REGION_SEG_VERSION = 0x1

# for python2
if sys.version_info < (3, 0):
    reload(sys)
    sys.setdefaultencoding('utf8')

# private module
from nvram_prop import *;

NVRAM_WRITE_REGION_ALIGN_SIZE = 512
NVRAM_SEG_HEADER_SIZE = 16
NVRAM_ITEM_HEADER_SIZE = 8

def calc_hash(key):
    hash = 0
    for byte in key:
        hash = hash + byte

    hash = hash ^ 0xa5
    hash = hash & 0xff
    return hash


CRC8_MAXIM_TABLE = [
    0x00, 0x5E, 0xBC, 0xE2, 0x61, 0x3F, 0xDD, 0x83,
    0xC2, 0x9C, 0x7E, 0x20, 0xA3, 0xFD, 0x1F, 0x41,
    0x00, 0x9D, 0x23, 0xBE, 0x46, 0xDB, 0x65, 0xF8,
    0x8C, 0x11, 0xAF, 0x32, 0xCA, 0x57, 0xE9, 0x74
    ]

def calc_crc8(data, length):
    """
    * Name:    CRC-8/MAXIM         x8+x5+x4+1
    * Poly:    0x31
    * Init:    0x00
    * Refin:   True
    * Refout:  True
    * Xorout:  0x00
    * Alias:   DOW-CRC,CRC-8/IBUTTON,MSB first
    * Use:     Maxim(Dallas)'s some devices,e.g. DS18B20
    """
    crc = 0
    for i in range(length):
        crc = data[i] ^ crc
        crc = CRC8_MAXIM_TABLE[crc & 0x0f] ^ CRC8_MAXIM_TABLE[16 + ((crc >> 4) & 0x0f)] 

    return crc


'''
struct region_seg_header
{
	u32_t magic;
	u8_t state;
	u8_t crc;
	u8_t version;
	u8_t reserved;
	u8_t seq_id;
	u8_t head_size;
	u16_t seg_size;
	u8_t reserved2[4];
};

struct nvram_item {
	u8_t magic;
	u8_t state;
	u8_t crc;
	u8_t hash;
	u8_t reserved;
	u8_t name_size;
	u16_t data_size;
	char data[0];
};
'''
def gen_item(key, value):
        # append '\0' to string
        key_data = bytearray(key, 'utf8') + bytearray(1)
        key_data_len = len(key_data)
        value_data = bytearray(value, 'utf8') + bytearray(1)
        value_data_len = len(value_data)
        hash = calc_hash(key_data)

        total_len = NVRAM_ITEM_HEADER_SIZE + key_data_len + value_data_len

        # 16 byte aligned
        pad_data = bytearray(0)
        if total_len & 0xf:
             pad_len = 16 - (total_len & 0xf)
             pad_data = bytearray(0xff for i in range(pad_len))
             total_len = total_len + pad_len

        item_header = struct.pack('<BBBH', hash, 0x00, key_data_len, value_data_len)
        item = item_header + key_data + value_data
        crc = calc_crc8(item, len(item));
        item = struct.pack('<BBB', NVRAM_REGION_ITEM_MAGIC, 0xff, crc) + item + pad_data

        return (item, total_len)

def build_nvram_region(filename, props):
    fr_file = open(filename,'wb+')

    # write NVRAM region header
    seg_header = struct.pack('<BxBBH4x', NVRAM_REGION_SEG_VERSION, 0x0, 16, 4096)
    crc = calc_crc8(bytearray(seg_header), len(seg_header));
    fr_info = struct.pack('<IBB', NVRAM_REGION_SEG_MAGIC, 0xff, crc) + seg_header

    fr_file.seek(0, 0)
    fr_file.write(fr_info);

    buildprops = props.get_all()

    # write NVRAM items
    data_len = 0;
    for key, value in buildprops.items():
        print('NVRAM: Property: ' + key + '=' + value);

        (item, item_len) = gen_item(key, value)
        data_len = data_len + item_len
        fr_file.write(item);

    # padding region aligned to NVRAM_WRITE_REGION_ALIGN_SIZE
    pos = fr_file.tell()
    pad_len = 0
    if (pos % NVRAM_WRITE_REGION_ALIGN_SIZE):
        pad_len = NVRAM_WRITE_REGION_ALIGN_SIZE - pos % NVRAM_WRITE_REGION_ALIGN_SIZE
        fr_file.write(bytearray(0xff for i in range(pad_len)));

    file_len = pos + pad_len

    fr_file.close()

def main(argv):
    parser = argparse.ArgumentParser(
        description='Pack config files to NVRAM binary data',
    )
    parser.add_argument('-o', dest = 'output_file')
    parser.add_argument('input_files', nargs = '*')
    args = parser.parse_args();

    print('NVRAM: Build Factory NVRAM binary file')

    lines = []
    for input_file in args.input_files:
        if not os.path.isfile(input_file):
            continue

        print('NVRAM: Process property file: %s' %input_file)
        with open(input_file) as f:
            lines = lines + f.readlines()        

    properties = PropFile(lines)

    # write the merged property file
    properties.write(os.path.join(os.path.dirname(args.output_file), 'nvram.prop'))

    print('NVRAM: Generate NVRAM file: %s.' %args.output_file)
    build_nvram_region(args.output_file, properties)
   
if __name__ == "__main__":
    main(sys.argv)
