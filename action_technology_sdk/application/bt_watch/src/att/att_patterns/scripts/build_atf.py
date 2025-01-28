#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import os
import sys
import time
import struct
import array
import argparse
import collections
import re
import ctypes as ct
import zlib

# private module

class ATF_FILE_HEADER(ct.Structure): # import from ctypes
    _pack_ = 1
    _fields_ = [
        ("magic",           ct.c_uint8 * 8),
        ("sdk_version",     ct.c_uint8 * 4),
        ("file_total",      ct.c_uint8),
        ("reserved",        ct.c_uint8 * 7),
        ("buildtime",       ct.c_uint8 * 12)
        ] # 32 bytes
ATF_FILE_HEADER_SIZE        = ct.sizeof(ATF_FILE_HEADER)

class ATF_FILE_DIR(ct.Structure): # import from ctypes
    _pack_ = 1
    _fields_ = [
        ("filename",        ct.c_uint8 * 12),
        ("load_addr",       ct.c_uint32),
        ("offset",          ct.c_uint32),
        ("length",          ct.c_uint32),      
        ("run_addr",        ct.c_uint32),
        ("checksum",        ct.c_uint32)
        ] # 32 bytes
DIR_ITEM_SIZE               = ct.sizeof(ATF_FILE_DIR)

CRCUnit_Align = lambda x:(x + 511) // 512 * 512

def CheckSum32(cbuffer, size):
    ret = ct.c_uint32(0)
    ptr = ct.cast(cbuffer, ct.POINTER(ct.c_uint32))
    size= size // 4
    for i in range(size):
        ret.value = ret.value + ptr[i]
    return ret

def PreparseConfig(input_file):
    """Preprocess input file, filter out all comment line string.
    """
    config_list = []

    if os.path.isfile(input_file):
        with open(input_file, 'r') as p_file:
            str             = p_file.read()
            line_str_list   = str.split('\n')

            for line_str in line_str_list:
                line_str = line_str.strip()
                # filter out comment string and empty line
                if line_str and not re.search(r'^\s*($|//|#)', line_str):
                    config_list.append(line_str)

    return config_list


class ATF_FIRMWARE(object):
    def __init__(self, config_file, build_time_file):
        super(ATF_FIRMWARE, self).__init__()
        self.config_file       = config_file
        self.config_path       = os.path.dirname(os.path.abspath(config_file))
        self.build_time_file   = build_time_file
        self.atf_sub_file_list = []

        self.ConfigPattern = re.compile(r'''
        ^                       # beginning of string
        \s*                     # is optional and can be any number of whitespace character
        (ATT_PATTERN_FILE|ATT_BOOT_FILE|ATT_CONFIG_XML|ATT_CONFIG_TXT|ATT_DFU_FW)
        \s*                     # is optional and can be any number of whitespace character
        =                       # key word '='
        \s*                     # is optional and can be any number of whitespace character
        (.+?)                   # config string
        $                       # end of string
        ''', re.VERBOSE + re.IGNORECASE)


    def preparse(self):
        """Preparse the input config file, get all config string.
        """
        config_str_list = PreparseConfig(self.config_file)

        for i, config_string in enumerate(config_str_list):
            ConfigObject    = self.ConfigPattern.search(config_string)
            if ConfigObject:
                config_tupe = ConfigObject.groups()
                if config_tupe[0] == 'ATT_BOOT_FILE':
                    self.add_att_boot_file(config_tupe[1])
                elif config_tupe[0] == 'ATT_PATTERN_FILE':
                    self.add_att_pattern_file(config_tupe[1])
                elif config_tupe[0] == 'ATT_CONFIG_XML':
                    self.add_att_config_xml(config_tupe[1])
                elif config_tupe[0] == 'ATT_CONFIG_TXT':
                    self.add_att_config_txt(config_tupe[1])
                else:
                    # ATT_DFU_FW
                    self.add_att_dfu_fw(config_tupe[1])
            else:
                print("NOT support key word: %s in line %d of file %s." %(config_string, i, self.config_file))

    def add_att_boot_file(self, config_str):
        """Add ft test file.
        """
        str_list = config_str.strip().split(',')
        print(str_list)

        file        = os.path.join(self.config_path, str_list[0].strip())
        load_addr   = int(str_list[1].strip(), 0)
        run_addr    = int(str_list[2].strip(), 0)

        if len(os.path.basename(file)) > 12:
            raise Exception("file name %s is TOO LONG(8.3)!" %file)

        if os.path.isfile(file):
            self.atf_sub_file_list.append((file, load_addr, run_addr))
        else:
            print("File %s NOT exist!" %file)

    def add_att_pattern_file(self, config_str):
        """Add ft test file.
        """
        str_list = config_str.strip().split(',')
        print(str_list)

        file        = os.path.join(self.config_path, str_list[0].strip())
        load_addr   = int(str_list[1].strip(), 0)
        run_addr    = int(str_list[2].strip(), 0)

        if len(os.path.basename(file)) > 12:
            raise Exception("file name %s is TOO LONG(8.3)!" %file)

        if os.path.isfile(file):
            self.atf_sub_file_list.append((file, load_addr, run_addr))
        else:
            print("File %s NOT exist!" %file)

    def add_att_config_xml(self, config_str):
        """Add ft test file.
        """
        str_list = config_str.strip().split(',')
        print(str_list)

        file        = os.path.join(self.config_path, str_list[0].strip())

        if len(os.path.basename(file)) > 12:
            raise Exception("file name %s is TOO LONG(8.3)!" %file)

        if os.path.isfile(file):
            self.atf_sub_file_list.append((file, 0, 0))
        else:
            print("File %s NOT exist!" %file)

    def add_att_config_txt(self, config_str):
        """Add ft test file.
        """
        str_list = config_str.strip().split(',')
        print(str_list)

        file        = os.path.join(self.config_path, str_list[0].strip())

        if len(os.path.basename(file)) > 12:
            raise Exception("file name %s is TOO LONG(8.3)!" %file)

        if os.path.isfile(file):
            self.atf_sub_file_list.append((file, 0, 0))
        else:
            print("File %s NOT exist!" %file)

    def add_att_dfu_fw(self, config_str):
        """Add ft test file.
        """
        str_list = config_str.strip().split(',')
        print(str_list)

        file        = os.path.join(self.config_path, str_list[0].strip())

        if len(os.path.basename(file)) > 12:
            raise Exception("file name %s is TOO LONG(8.3)!" %file)

        if os.path.isfile(file):
            self.atf_sub_file_list.append((file, 0, 0))
        else:
            print("File %s NOT exist!" %file)

    def build_fw(self, atf_file):
        atf_sub_file_num    = len(self.atf_sub_file_list)
        # head data length algin with CRC unit
        dir_item_size       = (atf_sub_file_num + 1) * DIR_ITEM_SIZE
        dir_item_align_size = CRCUnit_Align(dir_item_size)
        atf_data_len        = dir_item_align_size
        for file_info_tuple in self.atf_sub_file_list:
            atf_data_len     += CRCUnit_Align(os.path.getsize(file_info_tuple[0]))

        print("atf bin file length %d" %atf_data_len)
        atf_data_buf        = (ct.c_uint8 * atf_data_len)()
        atf_header          = ATF_FILE_HEADER()
        dir_items           = ATF_FILE_DIR()
        dir_offset          = DIR_ITEM_SIZE
        offset              = dir_item_align_size

        #print(self.atf_sub_file_list)
        for file_info_tuple in self.atf_sub_file_list:
            abs_file_name   = file_info_tuple[0]
            ct.memset(ct.addressof(dir_items), 0, DIR_ITEM_SIZE)
            file_name       = os.path.basename(abs_file_name)
            ct.memmove(ct.addressof(dir_items), file_name.encode("UTF-8"), len(file_name.encode("UTF-8")))
            fsize               = os.path.getsize(abs_file_name)
            dir_items.offset    = offset
            dir_items.length    = fsize
            dir_items.load_addr = file_info_tuple[1]
            dir_items.run_addr  = file_info_tuple[2]

            with open(abs_file_name, 'rb') as f:
                s = f.read()
                ct.memmove(ct.addressof(atf_data_buf) + offset, s, fsize)

                dir_items.checksum = CheckSum32(s, fsize)

            ct.memmove(ct.addressof(atf_data_buf) + dir_offset, ct.addressof(dir_items), DIR_ITEM_SIZE)

            offset = offset + CRCUnit_Align(fsize)
            dir_offset = dir_offset + DIR_ITEM_SIZE

        # sd file head
        atf_magic_name = b"ACTTEST0"
        ct.memset(ct.addressof(atf_header), 0, ATF_FILE_HEADER_SIZE)
        ct.memmove(ct.addressof(atf_header), atf_magic_name, len(atf_magic_name))
        atf_header.file_total = atf_sub_file_num
        # add build data&time
        #with open(self.build_time_file, 'rb') as f:
        #    s = f.read(10)
        #    ct.memmove(ct.addressof(atf_header) + 20, s, 10)
        ct.memmove(ct.addressof(atf_data_buf), ct.addressof(atf_header), ATF_FILE_HEADER_SIZE)

        with open(atf_file, 'wb') as f:
            f.write(atf_data_buf)

        return True

def main(argv):
    parser = argparse.ArgumentParser(
        description='Build FT test firmware.',
    )

    parser.add_argument('-c',   dest = 'config_file',     required = True,    help = 'atf build config file')
    #parser.add_argument('-t',   dest = 'build_time_file', required = True,    help = 'fw build time file')
    parser.add_argument('-o',   dest = 'atf_file',        required = True,    help = 'atf build output file')

    args            = parser.parse_args()

    config_file     = args.config_file
    build_time_file = 0
    atf_file        = args.atf_file

    try:
        fw  = ATF_FIRMWARE(config_file, build_time_file)
        fw.preparse()
        fw.build_fw(atf_file)
    except Exception as e:
            print('%s(%s)' %(type(e), e))
            # traceback.print_exc()
            return -1

if __name__ == '__main__':
    main(sys.argv[1:])

