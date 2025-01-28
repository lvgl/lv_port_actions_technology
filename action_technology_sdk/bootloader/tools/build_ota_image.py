#!/usr/bin/env python3
#
# Build Actions SoC OTA firmware
#
# Copyright (c) 2019 Actions Semiconductor Co., Ltd
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
import array
import hashlib
import shutil
import zipfile
import xml.etree.ElementTree as ET
import zlib

script_path = os.path.split(os.path.realpath(__file__))[0]

def align_down(data, alignment):
    return data & ~(alignment - 1)

def align_up(data, alignment):
    return align_down(data + alignment - 1, alignment)

def calc_pad_length(length, alignment):
    return align_up(length, alignment) - length

def crc32_file(filename):
    if os.path.isfile(filename):
        with open(filename, 'rb') as f:
            crc = zlib.crc32(f.read(), 0) & 0xffffffff
            return crc
    return 0

def panic(err_msg):
    print('\033[1;31;40m')
    print('FW: Error: %s\n' %err_msg)
    print('\033[0m')
    sys.exit(1)

def print_notice(msg):
    print('\033[1;32;40m%s\033[0m' %msg)


'''
/* OTA image */

typedef struct
{
    uint8 filename[12];
    uint8 reserved1[4];
    uint32 offset;
    uint32 length;
    uint8 reserved2[4];
    uint32 checksum;
}atf_dir_t;

struct ota_fw_hdr {
	u32_t	magic;
	u32_t	header_checksum;
	u16_t	header_version;
	u16_t	header_size;
	u16_t	file_cnt;
	u16_t	flag;
	u16_t	dir_offset;
	u16_t	data_offset;
	u32_t	data_size;
	u32_t	data_checksum;
	u8_t	reserved[8];

} __attribute__((packed));

struct fw_version {
	u32_t magic;
	u32_t version_code;
        u32_t version_res;
	u32_t system_version_code;
	char version_name[64];
	char board_name[32];
	u8_t reserved1[12];
	u32_t checksum;
};

struct ota_fw_head {
	struct ota_fw_hdr	hdr;
	u8_t			reserved1[32];

	/* offset: 0x40 */
	struct ota_fw_ver	new_ver;

	/* offset: 0xa0 */
	struct ota_fw_ver	old_ver;

	u8_t			reserved2[32];

	/* offset: 0x200 */
	struct ota_fw_dir	dir;
};

'''

class fw_version(object):
    def __init__(self, xml_file, tag_name):
        self.valid = False

        print('FWVER: Parse xml file: %s tag %s' %(xml_file, tag_name))
        tree = ET.ElementTree(file=xml_file)
        root = tree.getroot()
        if (root.tag != 'ota_firmware'):
            panic('OTA: invalid OTA xml file')

        ver = root.find(tag_name)
        if ver == None:
            return

        self.version_name = ver.find('version_name').text.strip()
        self.version_code = int(ver.find('version_code').text.strip(), 0)
        self.version_res = int(ver.find('version_res').text.strip(), 0)
        self.board_name = ver.find('board_name').text.strip()
        self.valid = True

    def get_data(self):
        return struct.pack('<32s24s4xIxI', bytearray(self.version_name, 'utf8'), \
                        bytearray(self.board_name, 'utf8'), self.version_code, self.version_res)

    def dump_info(self):
        print('\t' + 'version name: ' + self.version_code)
        print('\t' + 'version name: ' + self.version_res)
        print('\t' + 'version code: ' + self.version_name)
        print('\t' + '  board name: ' + self.board_name)

class ota_file(object):
    def __init__(self, fpath):
        if (not os.path.isfile(fpath)):
            panic('invalid file: ' + fpath)

        self.file_path = fpath
        self.file_name = bytearray(os.path.basename(fpath), 'utf8')
        if len(self.file_name) > 12:
            print('OTA: file %s name is too long' %(self.file_name))
            return

        self.length = os.path.getsize(fpath)
        self.checksum = int(crc32_file(fpath))
        self.data = bytearray(0)
        with open(fpath, 'rb') as f:
            self.data = f.read()

    def dump_info(self):
        print('      name: ' + self.file_name.decode('utf8').rstrip('\0'))
        print('    length: ' + str(self.length))
        print('  checksum: ' + str(self.checksum))

FIRMWARE_VERSION_MAGIC = 0x52455646   #FVER


OTA_FIRMWARE_MAGIC = b'AOTA'
OTA_FIRMWARE_DIR_OFFSET = 0x200
OTA_FIRMWARE_DIR_ENTRY_SIZE = 0x20
OTA_FIRMWARE_HEADER_SIZE = 0x400
OTA_FIRMWARE_DATA_OFFSET = 0x400
OTA_FIRMWARE_VERSION = 0x0100

class ota_fw(object):
    def __init__(self, path_list):
        self.old_version = None
        self.new_version = None
        self.length = 0
        self.offset = 0
        self.ota_files = []
        self.file_data = bytearray(0)
        self.head_data = bytearray(0)
        self.dir_data = bytearray(0)

        if len(path_list) > 14:
            panic('OTA: too much input files')

        if len(path_list) == 1 and os.path.isdir(path_list[0]):
            files = [name for name in os.listdir(path_list[0]) \
                     if os.path.isfile(os.path.join(path_list[0], name))]
        else:
            files = path_list

        for file in files:
            self.ota_files.append(ota_file(file))

    def dump_info(self):
        i = 0
        for file in self.ota_files:
            print('[%d]' %(i))
            i = i + 1
            file.dump_info()

    def pack(self, output_file):
        data_offset = OTA_FIRMWARE_DATA_OFFSET
        dir_entry = bytearray(0)
        for file in self.ota_files:
            dir_entry = struct.pack("<12s4xII4xI", file.file_name, data_offset, file.length, file.checksum)
            pad_len = calc_pad_length(file.length, 512)
            data_offset = data_offset + file.length + pad_len
            self.file_data = self.file_data + file.data + bytearray(pad_len)
            self.dir_data = self.dir_data + dir_entry

        data_crc = zlib.crc32(self.file_data, 0) & 0xffffffff
        
        pad_len = calc_pad_length(len(self.dir_data), 512)
        self.dir_data = self.dir_data + bytearray(pad_len)

        self.head_data = struct.pack("<4sIHHHHHHII36x", OTA_FIRMWARE_MAGIC, 0, \
                                    OTA_FIRMWARE_VERSION, OTA_FIRMWARE_HEADER_SIZE, \
                                    len(self.ota_files), 0, \
                                    OTA_FIRMWARE_DIR_OFFSET, OTA_FIRMWARE_DATA_OFFSET, \
                                    data_offset, data_crc)

        # add fw version
        xml_fpath = ''
        for file in self.ota_files:
            if file.file_name == b'ota.xml':
                xml_fpath = file.file_path
        if xml_fpath == '':
            panic('cannot found ota.xml file')

        fw_ver = fw_version(xml_fpath, 'firmware_version')
        if not fw_ver.valid:
            panic('cannot found ota.xml file')
        
        self.head_data = self.head_data + fw_ver.get_data()

        old_fw_ver = fw_version(xml_fpath, 'old_firmware_version')
        if old_fw_ver.valid:
            self.head_data = self.head_data + old_fw_ver.get_data()

        self.head_data = self.head_data + bytearray(calc_pad_length(len(self.head_data), 512))
        
        header_crc = zlib.crc32(self.head_data[8:] + self.dir_data, 0) & 0xffffffff
        self.head_data = self.head_data[0:4] + struct.pack('<I', header_crc) + \
            self.head_data[8:]

        with open(output_file, 'wb') as f:
            f.write(self.head_data)
            f.write(self.dir_data)
            f.write(self.file_data)


def unpack_fw(fpath, out_dir):
    f = open(fpath, 'rb')
    if f == None:
        panic('cannot open file')

    ota_data = f.read()

    magic, checksum, header_version, header_size, file_cnt, header_flag, \
    dir_offs, data_offs, data_len, data_crc \
        = struct.unpack("<4sIHHHHHHII36x", ota_data[0:64])

    if magic != OTA_FIRMWARE_MAGIC:
        panic('invalid magic')

    if not os.path.exists(out_dir):
        os.makedirs(out_dir)

    for i in range(file_cnt):
        entry_offs = dir_offs + i * OTA_FIRMWARE_DIR_ENTRY_SIZE
        name, offset, file_length, checksum = \
            struct.unpack("<12s4xII4xI", ota_data[entry_offs : entry_offs + OTA_FIRMWARE_DIR_ENTRY_SIZE])

        if name[0] != 0:
            with open(os.path.join(out_dir, name.decode('utf8').rstrip('\0')), 'wb') as wf:
                wf.write(ota_data[offset : offset + file_length])

def main(argv):
    parser = argparse.ArgumentParser(
        description='Build OTA firmware image',
    )
    parser.add_argument('-o', dest = 'output_file')
    parser.add_argument('-i', dest = 'image_file')
    parser.add_argument('-x', dest = 'extract_out_dir')
    parser.add_argument('input_files', nargs = '*')
    args = parser.parse_args();

    print('ATF: Build OTA firmware image: %s' %args.output_file)

    if args.extract_out_dir:
        unpack_fw(args.image_file, args.extract_out_dir)
    else:
        if (not args.output_file) :
            panic('no file')

        fw = ota_fw(args.input_files)
        fw.dump_info()
        fw.pack(args.output_file)

    return 0

if __name__ == '__main__':
    main(sys.argv[1:])
