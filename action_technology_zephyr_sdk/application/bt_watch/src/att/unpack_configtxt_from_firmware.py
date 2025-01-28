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
import shutil

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

    
def atf_parse_header(input_file, output_file): 
#    shutil.rmtree(output_dir)
#    os.mkdir(output_dir)    
    basef = open(input_file, 'rb+')
    atf_header = basef.read(ATF_DATA_ALIGN_SIZE)
        
    offset = 32
      
    while True: 
        af = atf_dir()
            
        file_name, file_offset, file_length, file_checksum = struct.unpack_from('<12s4xII4xI', atf_header, offset)
        
        #file_name.strip('\0')
        file_name = file_name.split(b"\x00")[0].decode("gbk")
               
        if file_length == 0:
            break;
            
        if file_name == os.path.basename(output_file):
            f = open(output_file, 'wb+')
            basef.seek(file_offset, 0)
            atf_file = basef.read(file_length)
            f.write(atf_file)
            f.close()            
        #print('Extrace file %s' %file_name)

        offset += 32 
        
#        shutil.move(file_name, output_dir)

        if offset == ATF_DATA_ALIGN_SIZE:
            break; 

def main(argv):
    parser = argparse.ArgumentParser(
        description='Build ATT firmware image',
    )
    parser.add_argument('-i', dest = 'input_file')
    parser.add_argument('-o', dest = 'output_file')
    args = parser.parse_args();

    print('ATF: unpack ATT firmware image: %s' %args.input_file)
       
    atf_parse_header(args.input_file, args.output_file)

if __name__ == "__main__":
    main(sys.argv)
