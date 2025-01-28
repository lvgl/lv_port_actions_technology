#!/usr/bin/env python3
#
# Build Actions SoC firmware (RAW/USB/OTA)
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
import array
import hashlib
import shutil
import zipfile
import xml.etree.ElementTree as ET
import zlib
import json
import traceback
import csv

from ctypes import *

# private module
from nvram_prop import *;

CFG_TEST_GENERATE_OTA_FIRMWARES_COUNT = 3

CFG_FIRMWARE_NEED_ENCYPT   = False

PRODUCTION_GLOBAL_BUFFER_DEFAULT_SIZE = 0x1000000

PARTITION_ALIGNMENT   = 0x1000

class PARTITION_ENTRY(Structure): # import from ctypes
    _pack_ = 1
    _fields_ = [
        ("name",            c_uint8 * 8),
        ("type",            c_uint8),
        ("file_id",         c_uint8),
        ("mirror_id",       c_uint8, 4),
        ("storage_id",      c_uint8, 4),
        ("flag",            c_uint8),
        ("offset",          c_uint32),
        ("size",            c_uint32),
        ("entry_offs",      c_uint32),
        ] # 64 bytes
SIZEOF_PARTITION_ENTRY   = 0x18

class PARTITION_TABLE(Structure): # import from ctypes
    _pack_ = 1
    _fields_ = [
        ("magic",           c_uint32),
        ("version",         c_uint16),
        ("table_size",      c_uint16),
        ("part_cnt",        c_uint16),
        ("part_entry_size", c_uint16),
        ("reserved1",       c_uint8 * 4),
        ("parts",           PARTITION_ENTRY * 15),
        ("reserved2",       c_uint8 * 4),
        ("table_crc",       c_uint32),
        ] # 64 bytes
SIZEOF_PARTITION_TABLE   = 0x180
PARTITION_TABLE_MAGIC = 0x54504341   #'ACPT'

partition_type_table = {'RESERVED':0, 'BOOT':1, 'SYSTEM':2, 'RECOVERY':3, 'DATA':4, 'TEMP':5, 'SYS_PARAM':6}

class FIRMWARE_VERSION(Structure): # import from ctypes
    _pack_ = 1
    _fields_ = [
        ("magic",           c_uint32),
        ("version_code",    c_uint32),
        ("version_res",     c_uint32),
        ("sys_version_code",c_uint32),
        ("version_name",    c_uint8 * 64),
        ("board_name",      c_uint8 * 32),
        ("reserved",        c_uint8 * 12),
        ("checksum",        c_uint32),
        ] # 128 bytes
SIZEOF_FIRMWARE_VERSION   = 0x80
FIRMWARE_VERSION_MAGIC = 0x52455646   #'FVER'


class IMAGE_HEADER(Structure): # import from ctypes
    _pack_ = 1
    _fields_ = [
        ("magic",           c_uint32),
        ("version",         c_uint16),
        ("header_size",     c_uint16),
        ("data_type",       c_uint16),
        ("data_flag",       c_uint16),
        ("data_offset",     c_uint32),
        ("data_size",       c_uint32),
        ("data_base_vaddr", c_uint32),
        ("data_entry_vaddr", c_uint32),
        ("data_checksum",   c_uint32),
        ("reserved",        c_uint8 * 28),
        ("header_crc",      c_uint32),
        ] # 64 bytes
SIZEOF_IMAGE_HEADER   = 64
IMAGE_HEADER_MAGIC = 0x4d494341     #'ACIM'

MP_CARD_CFG_NAME="mp_card.cfg"
FW_MAKER_EXT_CFG_NAME="fw_maker_ext.cfg"
FW_BUILD_TIME_FILE_NAME="fw_build_time.bin";

script_path = os.path.split(os.path.realpath(__file__))[0]

soc_name = ''
board_name = ''
encrypt_fw = ''
efuse_bin = ''

# table for calculating CRC
CRC16_TABLE = [
        0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
        0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
        0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
        0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
        0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
        0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
        0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
        0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
        0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
        0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
        0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
        0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
        0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
        0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
        0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
        0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
        0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
        0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
        0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
        0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
        0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
        0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
        0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
        0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
        0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
        0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
        0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
        0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
        0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
        0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
        0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
        0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78,
        ]

def reflect(crc):
    """
    :type n: int
    :rtype: int
    """
    m = ['0' for i in range(16)]
    b = bin(crc)[2:]
    m[:len(b)] = b[::-1]
    return int(''.join(m) ,2)

def _crc16(data, crc, table):
    """Calculate CRC16 using the given table.
    `data`      - data for calculating CRC, must be bytes
    `crc`       - initial value
    `table`     - table for caclulating CRC (list of 256 integers)
    Return calculated value of CRC

     polynom             :  0x1021
     order               :  16
     crcinit             :  0xffff
     crcxor              :  0x0
     refin               :  1
     refout              :  1
    """
    crc = reflect(crc)

    for byte in data:
        crc = ((crc >> 8) & 0xff) ^ table[(crc & 0xff) ^ byte]

    crc = reflect(crc)

    # swap byte
    crc = ((crc >> 8) & 0xff) | ((crc & 0xff) << 8)

    return crc

def crc16(data, crc=0xffff):
    """Calculate CRC16.
    `data`      - data for calculating CRC, must be bytes
    `crc`       - initial value
    Return calculated value of CRC
    """
    return _crc16(data, crc, CRC16_TABLE)

def md5_file(filename):
    if os.path.isfile(filename):
        with open(filename, 'rb') as f:
            md5 = hashlib.md5()
            md5.update(f.read())
            hash = md5.hexdigest()
            return str(hash)
    return None

def crc32_file(filename):
    if os.path.isfile(filename):
        with open(filename, 'rb') as f:
            crc = zlib.crc32(f.read(), 0) & 0xffffffff
            return crc
    return 0

def pad_file(filename, align = 4, fillbyte = 0xff):
        with open(filename, 'ab') as f:
            filesize = f.tell()
            if (filesize % align):
                padsize = align - filesize & (align - 1)
                f.write(bytearray([fillbyte]*padsize))

def new_file(filename, filesize, fillbyte = 0xff):
        with open(filename, 'wb') as f:
            f.write(bytearray([fillbyte]*filesize))

def dd_file(input_file, output_file, count=None, seek=None, skip=None):
    inf = open(input_file, mode='rb')
    outf = open(output_file, mode='rb+')
    if skip is not None:
        inf.seek(skip)
    if seek is not None:
        outf.seek(seek)
    if count is None:
        count = os.path.getsize(input_file)
    outf.write(inf.read(count))
    inf.close
    outf.close

def zip_dir(source_dir, output_filename):
    zf = zipfile.ZipFile(output_filename, 'w')
    pre_len = len(os.path.dirname(source_dir))
    for parent, dirnames, filenames in os.walk(source_dir):
        for filename in filenames:
            pathfile = os.path.join(parent, filename)
            arcname = pathfile[pre_len:].strip(os.path.sep)
            zf.write(pathfile, arcname)
    zf.close()

def memcpy_n(cbuffer, bufsize, pylist):
    size = min(bufsize, len(pylist))
    for i in range(size):
        cbuffer[i]= ord(pylist[i])

def c_struct_crc(c_struct, length):
    crc_buf = (c_byte * length)()
    memmove(addressof(crc_buf), addressof(c_struct), length)
    return zlib.crc32(crc_buf, 0) & 0xffffffff

def align_down(data, alignment):
    return data // alignment * alignment

def align_up(data, alignment):
    return align_down(data + alignment - 1, alignment)

def run_cmd(cmd):
    """Echo and run the given command.

    Args:
    cmd: the command represented as a list of strings.
    Returns:
    A tuple of the output and the exit code.
    """
#    print("Running: ", " ".join(cmd))
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = p.communicate()
#    print("%s" % (output.rstrip()))
    return (output, p.returncode)

def panic(err_msg):
    print('\033[1;31;40m')
    print('FW: Error: %s\n' %err_msg)
    print('\033[0m')
    sys.exit(1)

def print_notice(msg):
    print('\033[1;32;40m%s\033[0m' %msg)

def cygpath(upath):
    cmd = ['cygpath', '-w', upath]
    (wpath, exit_code) = run_cmd(cmd)
    if (0 != exit_code):
        return upath
    return wpath.decode().strip()

def is_windows():
    sysstr = platform.system()
    if (sysstr.startswith('Windows') or \
       sysstr.startswith('MSYS') or     \
       sysstr.startswith('MINGW') or    \
       sysstr.startswith('CYGWIN')):
        return True
    else:
        return False

def is_msys():
    sysstr = platform.system()
    if (sysstr.startswith('MSYS') or    \
        sysstr.startswith('MINGW') or   \
        sysstr.startswith('CYGWIN')):
        return True
    else:
        return False

def soc_is_andes():
    if soc_name == 'andes':
        return True
    else:
        return False

class firmware(object):
    def __init__(self, cfg_file):
        self.crc_chunk_size = 32
        self.crc_full_chunk_size = self.crc_chunk_size + 2
        self.part_num = 0
        self.partitions = []
        self.disk_size = 0x400000   # 4MB by default
        self.fw_dir = os.path.dirname(cfg_file)
        self.bin_dir = os.path.join(self.fw_dir, 'bin')
        self.ota_dir = os.path.join(self.fw_dir, 'ota')
        self.orig_bin_dir = self.bin_dir + '_orig'
        self.orig_ota_dir = self.ota_dir + '_orig'
        self.att_dir = os.path.join(self.bin_dir, 'ATT')
        self.fw_version = {}
        self.boot_file_name = ''
        self.param_file_name = ''
        self.is_earphone_app = False
        self.upgrade_baudrate = 1000000
        self.atf_file_list = []

        if "bt_earphone" in os.path.abspath(self.fw_dir).split('\\'):
            self.is_earphone_app = True
            print("is earphone SDK")

        if not os.path.isdir(self.att_dir):
            os.mkdir(self.att_dir)

        if not os.path.isdir(self.ota_dir):
            os.mkdir(self.ota_dir)
        if not os.path.isdir(self.orig_ota_dir):
            os.mkdir(self.orig_ota_dir)
        # self.parse_config(cfg_file)

    def parse_config(self, cfg_file, pre_crc = False, pre_encrypt = False):
        print('FW: Parse config file: %s' %cfg_file)
        tree = ET.ElementTree(file=cfg_file)
        root = tree.getroot()
        if (root.tag != 'firmware'):
            sys.stderr.write('error: invalid firmware config file')
            sys.exit(1)

        disk_size_prop = root.find('disk_size')
        if disk_size_prop is not None:
            self.disk_size = int(disk_size_prop.text.strip(), 0)

        firmware_version = root.find('firmware_version')
        for prop in firmware_version:
            self.fw_version[prop.tag] = prop.text.strip()

        if 'version_name' in self.fw_version.keys():
            cur_time = time.strftime('%y%m%d%H%M',time.localtime(time.time()))
            version_name = self.fw_version['version_name'].replace('$(build_time)', cur_time)
            self.fw_version['version_name'] = version_name
            # gen fw build time file
            fw_build_time_file = os.path.join(self.fw_dir, FW_BUILD_TIME_FILE_NAME)
            with open(fw_build_time_file, 'w') as f:
                f.write(cur_time)

        part_list = root.find('partitions').findall('partition')
        for part in part_list:
            part_prop = {}
            for prop in part:
                part_prop[prop.tag] = prop.text.strip()

            if 'file_name' in part_prop.keys():
                bin_file = os.path.join(self.bin_dir, part_prop['file_name'])
            else:
                bin_file = None

            if bin_file and ('SYS_PARAM' != part_prop['type']) and not os.path.exists(bin_file):
                print_notice('partition %s ignored: cannot found bin %s' %(part_prop['name'], bin_file))
            else:
                self.partitions.append(part_prop)
                self.part_num = self.part_num + 1

        self.part_num = len(self.partitions);
#        print(self.part_num)
#        print(self.partitions)
        if (0 == self.part_num):
            panic('cannot found parition config')

        for part in self.partitions:
            if ('file_name' in part.keys()) and ('BOOT' == part['type']):
                self.boot_file_name = part['file_name']

        for part in self.partitions:
            if ('file_name' in part.keys()) and ('SYS_PARAM' == part['type']):
                self.param_file_name = part['file_name']

        param_file = os.path.join(self.bin_dir, self.param_file_name)
        self.generate_partition_table(param_file)
        self.generate_firmware_version(param_file)

        if os.path.isdir(self.orig_bin_dir):
            shutil.rmtree(self.orig_bin_dir)
            time.sleep(0.1)
        shutil.copytree(self.bin_dir, self.orig_bin_dir)

        if pre_crc:
            self.generate_crc_file()

        if pre_encrypt:
            if os.path.isfile(efuse_bin):
                self.encrypt_files()
            elif CFG_FIRMWARE_NEED_ENCYPT:
                panic('firmware encryt must be enabled')

        self.check_part_file_size()

    def check_part_file_size(self):
        csv_file = os.path.join(self.bin_dir, 'partition_size.csv')
        csv_f = open(csv_file, 'w', newline = '')
        csv_writer = csv.writer(csv_f, dialect = 'excel')
        csv_writer.writerow(['partition', 'size', 'real size', 'free'])

        for part in self.partitions:
            if not 'file_name' in part.keys():
                continue
            partfile = os.path.join(self.bin_dir, part['file_name']);
            if os.path.isfile(partfile):
                partfile_size = os.path.getsize(partfile)
                part_size = int(part['size'], 0);
                part_addr = int(part['address'], 0);
                file_addr = int(part['file_address'], 0);

                if part['type'] == 'SYSTEM':
                    ksdfs_bin_file = os.path.join(self.bin_dir, "ksdfs.bin")
                    if os.path.isfile(ksdfs_bin_file):
                        partfile_size += os.path.getsize(ksdfs_bin_file)

                if file_addr < part_addr or file_addr >= (part_addr + part_size) :
                    panic('partition %s: invalid file_address 0x%x' \
                         %(part['name'], file_addr))

                part_file_max_size = part_size - (file_addr - part_addr)
                csv_writer.writerow([part['file_name'],
                    "%.2f" %(part_file_max_size / 1024),
                    "%.2f" %(partfile_size / 1024),
                    "%.2f" %((part_file_max_size - partfile_size) / 1024)])

                print('partition %s: \'%s\' file size 0x%x(%d KB), max size 0x%x(%d KB)!' \
                    %(part['name'], part['file_name'], partfile_size, partfile_size/1024, \
                    part_file_max_size, part_file_max_size/1024))

                if partfile_size > part_file_max_size:
                    csv_f.close()
                    panic('partition %s: \'%s\' file size 0x%x is bigger than partition size 0x%x!' \
                        %(part['name'], part['file_name'], partfile_size, part_size))

        csv_f.close()

    def boot_calc_checksum(self, data):
            s = sum(array.array('H',data))
            s = s & 0xffff
            return s

    def get_nvram_prop(self, name):
        prop_file = os.path.join(self.bin_dir, 'nvram.prop');
        if os.path.isfile(prop_file):
            with open(prop_file) as f:
                properties = PropFile(f.readlines())
                return properties.get(name)
        return ''

    def generate_crc_file(self):
        crc_files = []
        for part in self.partitions:
            if ('file_name' in part.keys()) and ('true' == part['enable_crc']):
                crc_files.append(part['file_name'])

        crc_files = list(set(crc_files))

        for crc_file in crc_files:
            self.add_crc(os.path.join(self.bin_dir, crc_file), self.crc_chunk_size)

    def encrypt_file(self, file_path, blk_size):
        #print ('FW: encrypt file %s' %(file_path))

        if not os.path.isfile(encrypt_fw):
            panic('Cannot found encrypt fw')

        if (is_windows()):
            # windows
            fw2x_path = script_path + '/utils/windows/fw2x.exe'
        else:
            if ('32bit' == platform.architecture()[0]):
                # linux x86
                fw2x_path = script_path + '/utils/linux-x86/fw2x/fw2x'
            else:
                # linux x86_64
                fw2x_path = script_path + '/utils/linux-x86_64/fw2x/fw2x'

        # extrace lfi.bin from btsys.fw
        fw2x_cmd = [fw2x_path, '-fsssfff', encrypt_fw, 'MakeEncryptBin',\
               'Encrypt', str(blk_size), file_path, file_path, efuse_bin]
        (outmsg, exit_code) = run_cmd(fw2x_cmd)
        if exit_code != 0:
            print('FW2X: encrypt file %s error' %(file_path))
            print(outmsg)
            sys.exit(1)

    def encrypt_files(self):
        for part in self.partitions:
            if CFG_FIRMWARE_NEED_ENCYPT:
                if ('BOOT' == part['type'] or 'SYSTEM' == part['type'] or 'RECOVERY' == part['type']) or 'SYS_PARAM' == part['type'] \
                    and ('true' != part['enable_encryption']):
                    panic('BOOT/SYSTEM partition must enable encryption')
                    sys.exit(3)

            if not 'file_name' in part.keys():
                continue

            if ('true' == part['enable_encryption']):
                print ('FW: encrypt file \'' + part['file_name'] + '\'')

                if ('true' == part['enable_crc']):
                    self.encrypt_file(os.path.join(self.bin_dir, part['file_name']), self.crc_full_chunk_size)
                else:
                    self.encrypt_file(os.path.join(self.bin_dir, part['file_name']), self.crc_chunk_size)

    def encrypt_param_file(self, param_file):
        for part in self.partitions:
            if ('file_name' in part.keys() and 'SYS_PARAM' == part['type']):
                if ('true' == part['enable_encryption']):
                    if ('true' == part['enable_crc']):
                        self.encrypt_file(param_file, self.crc_full_chunk_size)
                    else:
                        self.encrypt_file(param_file, self.crc_chunk_size)

    def generate_partition_table(self, param_file):
        part_tbl = PARTITION_TABLE()
        memset(addressof(part_tbl), 0, SIZEOF_PARTITION_TABLE)

        part_tbl.magic = PARTITION_TABLE_MAGIC
        part_tbl.ver = 0x0100
        part_tbl.table_size = SIZEOF_PARTITION_TABLE
        part_tbl.table_crc = 0x0
        part_tbl.part_entry_size = SIZEOF_PARTITION_ENTRY
        idx = 0
        for part in self.partitions:
            memcpy_n(part_tbl.parts[idx].name, 8, part['name'])
            part_tbl.parts[idx].type = partition_type_table[part['type']]
            part_tbl.parts[idx].file_id = int(part['file_id'])
            if 'mirror_id' in part.keys():
                part_tbl.parts[idx].mirror_id = int(part['mirror_id'])
            else:
                part_tbl.parts[idx].mirror_id = 0xf

            if 'storage_id' in part.keys():
                part_tbl.parts[idx].storage_id = int(part['storage_id'])
            else:
                part_tbl.parts[idx].storage_id = 0x0

            print('part [%d] mirror id %d' %(idx, part_tbl.parts[idx].mirror_id))
            part_tbl.parts[idx].flag = 0
            if 'enable_crc' in part.keys() and 'true' == part['enable_crc']:
                part_tbl.parts[idx].flag |= 0x1
            if 'enable_encryption' in part.keys() and 'true' == part['enable_encryption']:
                part_tbl.parts[idx].flag |= 0x2
            if 'enable_boot_check' in part.keys() and 'true' == part['enable_boot_check']:
                part_tbl.parts[idx].flag |= 0x4

            part_addr = int(part['address'], 0)
            part_size = int(part['size'], 0)
            file_addr = int(part['file_address'], 0)
            if part_addr & (PARTITION_ALIGNMENT - 1):
                panic('partition %s: unaligned part address 0x%x, need be aligned with 0x%x' \
                     %(part['name'], part_addr, PARTITION_ALIGNMENT))

            if part_size & (PARTITION_ALIGNMENT - 1):
                panic('partition %s: unaligned part size 0x%x, need be aligned with 0x%x' \
                     %(part['name'], part_size, PARTITION_ALIGNMENT))


            if file_addr < part_addr or file_addr >= (part_addr + part_size) :
                panic('partition %s: file_address 0x%x is not in part' \
                     %(part['name'], file_addr))

            part_tbl.parts[idx].offset = part_addr
            part_tbl.parts[idx].size = part_size
            part_tbl.parts[idx].entry_offs = file_addr
            #print('%d: 0x%x, 0x%x' %(idx, part_tbl.parts[idx].offset, part_tbl.parts[idx].size))
            idx = idx + 1
        part_tbl.part_cnt = idx
        part_tbl.table_crc = c_struct_crc(part_tbl, SIZEOF_PARTITION_TABLE - 4)

        """
        with open('parttbl.bin', 'wb') as f:
            f.write(part_tbl)
        """

        with open(param_file, 'wb') as f:
            f.seek(0, 0)
            f.write(part_tbl)

    def generate_firmware_version(self, param_file):
        global board_name
        if 'version_code' not in self.fw_version:
            print('\033[1;31;40minfo: no version code\033[0m');
            self.fw_version['version_code'] = '0'
        if 'version_res' not in self.fw_version:
            print('\033[1;31;40minfo: no version res\033[0m');
            self.fw_version['version_res'] = '0'
        print(self.fw_version)
        fw_ver = FIRMWARE_VERSION()
        memset(addressof(fw_ver), 0, SIZEOF_FIRMWARE_VERSION)

        fw_ver.magic = FIRMWARE_VERSION_MAGIC
        fw_ver.version_code = int(self.fw_version['version_code'], 0)
        fw_ver.version_res = int(self.fw_version['version_res'], 0)
        version_name = self.fw_version['version_name']
        if len(version_name) > len(fw_ver.version_name):
            panic('max version_name is t' %(len(fw_ver.version_name)))

        memcpy_n(fw_ver.version_name, len(self.fw_version['version_name']), \
                 self.fw_version['version_name'])

        memcpy_n(fw_ver.board_name, len(board_name), board_name)

        self.fw_version['board_name'] = board_name

        fw_ver.checksum = c_struct_crc(fw_ver, SIZEOF_FIRMWARE_VERSION - 4)
        #time.strftime('%y%m%d-%H%M%S',time.localtime(time.time()))

        """
        with open('fw.bin', 'wb') as f:
            f.write(fw_ver)
        """

        with open(param_file, 'rb+') as f:
            f.seek(SIZEOF_PARTITION_TABLE, 0)
            f.write(fw_ver)

    def generate_maker_ext_config(self, m_ext_cfg_file):
        with open(m_ext_cfg_file, 'a') as f:
            f.write('\n//nvram partition infor\n')

            nvram_partition_find = 0
            for part in self.partitions:
                if (('nvram_factory' == part['name'])):
                    f.write('NVRAM_FACTORY_RO=' + str(part['address']) + ',' + str(part['size']) + ';\n')
                    nvram_partition_find = 1;
            if (nvram_partition_find == 0):
                f.write('NVRAM_FACTORY_RO=0,0;\n')

            nvram_partition_find = 0
            for part in self.partitions:
                if (('nvram_user' == part['name'])):
                    f.write('NVRAM_USER_RW=' + str(part['address']) + ',' + str(part['size']) + ';\n')
                    nvram_partition_find = 1;
            if (nvram_partition_find == 0):
                f.write('NVRAM_USER_RW=0,0;\n')

            nvram_partition_find = 0
            for part in self.partitions:
                if (('nvram_factory_rw' == part['name'])):
                    f.write('NVRAM_FACTORY_RW=' + str(part['address']) + ',' + str(part['size']) + ';\n')
                    nvram_partition_find = 1;
            if (nvram_partition_find == 0):
                f.write('NVRAM_FACTORY_RW=0,0;\n')

    def generate_maker_config(self, mcfg_file):
        with open(mcfg_file, 'w') as f:
            f.write('SETPATH=\".\\";\n')
            f.write('SPI_STG_CAP=' + str(self.disk_size // 0x200) + ';\n')
            f.write('BASEFILE=\"afi.bin\";\n')

            """
            storage_id definition:
                - 0: NOR
                - 1: EMMC
                - 2: NAND
            INF_STORAGE_SOLUTION definition:
                - 0: NAND
                - 1: NOR
                - 2: MMC
                - 3: NOR + NAND
                - 4: NOR + EMMC
            """
            boot_storage_id = 0
            data_storage_id = 0
            # check boot storage id
            for part in self.partitions:
                if (('file_name' in part.keys()) and ('true' == part['enable_dfu']) and ('BOOT' == part['type'])):
                    boot_storage_id = int(part['storage_id'], 0)

            # write normal partition first
            for part in self.partitions:
                if (('file_name' in part.keys()) and ('true' == part['enable_dfu']) and ('BOOT' != part['type'])):
                    if (boot_storage_id == int(part['storage_id'], 0)) :
                        f.write('WFILE=\"' + part['file_name'] + '\",' + part['file_address'])
                        f.write(';\n')

            # nor boot ,can have extern data storage
            if (boot_storage_id == 0):
                for part in self.partitions:
                    if (('file_name' in part.keys()) and ('true' == part['enable_dfu']) and ('BOOT' != part['type'])):
                        if (boot_storage_id != int(part['storage_id'], 0) ) :
                            f.write('EXTERN_DATA_FILE=\"' + part['file_name'] + '\",' + part['file_address'])
                            f.write(';\n')
                            data_storage_id = int(part['storage_id'], 0)

            # write boot partition
            for part in self.partitions:
                if (('file_name' in part.keys()) and ('true' == part['enable_dfu']) and ('BOOT' == part['type'])):
                    if (boot_storage_id == int(part['storage_id'], 0)):
                        f.write('WFILE=\"' + part['file_name'] + '\",' + part['file_address'])
                        f.write(';\n')

            # check the current solution such as NOR-only, NAND-only, NOR + NAND

            # check the current solution such as NOR-only, EMMC-only NAND-only, NOR + NAND, NOR + EMMC
            if (boot_storage_id == 0):
                if (data_storage_id == 0):
                    f.write('INF_TDK_STORAGE_TYPE=1' + ';\n') # NOR-only solution
                elif (data_storage_id == 1):
                    f.write('INF_TDK_STORAGE_TYPE=4' + ';\n') # NOR + EMMC solution
                else :
                    f.write('INF_TDK_STORAGE_TYPE=3' + ';\n') # NOR + NAND solution
                f.write('EXTERN_DATA_STG_CAP=0x80000' + ';\n')
            elif (boot_storage_id == 1):  # EMMC-only solution
                f.write('INF_TDK_STORAGE_TYPE=2' + ';\n') # EMMC-only solution
            else:  # NAND-only solution
                f.write('INF_TDK_STORAGE_TYPE=0' + ';\n') # NAND-only solution

            global_buffer_size = max(self.disk_size, 0)
            if global_buffer_size > PRODUCTION_GLOBAL_BUFFER_DEFAULT_SIZE:
                f.write('INF_GLOBAL_BUFFER_SIZE=' + str(global_buffer_size) + ';\n')

            # The length of 'VER' field is limited to 32 bytes
            if len(self.fw_version['version_name']) > 32:
                fw_ver_temp = self.fw_version['version_name'][0:31]
            else:
                fw_ver_temp = self.fw_version['version_name']

            f.write('VER=\"' + fw_ver_temp + '\"' + ';\n');

            #add mp card config
            if os.path.exists(os.path.join(self.bin_dir, MP_CARD_CFG_NAME)):
                f.write('#include \"' + MP_CARD_CFG_NAME + '\"\n')

            #add extern data config
            if os.path.exists(os.path.join(self.bin_dir, FW_MAKER_EXT_CFG_NAME)):
                f.write('#include \"' + FW_MAKER_EXT_CFG_NAME + '\"\n')

    def generate_raw_image(self, rawfw_name):
        print('FW: Build raw spinor image')

        rawfw_file = os.path.join(self.fw_dir, rawfw_name)
        if os.path.exists(rawfw_file):
            os.remove(rawfw_file)

        new_file(rawfw_file, self.disk_size, 0xff)

        for part in self.partitions:
            if ('file_name' in part.keys()) and ('true' == part['enable_raw']) and (int(part['storage_id'], 0) == 0 or int(part['storage_id'], 0) == 2):
                addr = int(part["address"], 16)
                srcfile = os.path.join(self.bin_dir, part['file_name'])
                dd_file(srcfile, rawfw_file, seek=addr)

        if not os.path.exists(rawfw_file):
            panic('Failed to generate SPINOR raw image')

    def generate_firmware(self, fw_name):
        print('FW: Build USB DFU image')
        maker_cfgfile = os.path.join(self.bin_dir, 'fw_maker.cfg')
        self.generate_maker_config(maker_cfgfile);

        maker_ext_cfgfile = os.path.join(self.bin_dir, FW_MAKER_EXT_CFG_NAME)
        self.generate_maker_ext_config(maker_ext_cfgfile);

        fw_file = os.path.join(self.fw_dir, fw_name);

        if (is_windows()):
            cmd = [script_path + '/utils/windows/maker.exe', '-c',  maker_cfgfile, \
                   '-o', fw_file, '-mf']
        else:
            if os.path.exists(fw_file):
                os.remove(fw_file)

            if ('32bit' == platform.architecture()[0]):
                # linux x86
                maker_path = script_path + '/utils/linux-x86/maker/PyMaker.pyo'
            else:
                # linux x86_64
                maker_path = script_path + '/utils/linux-x86_64/maker/PyMaker.pyo'
            cmd = ['python2', '-O', maker_path, '-c', maker_cfgfile, '-o', fw_file, '--mf', '1']

        (outmsg, exit_code) = run_cmd(cmd)

        if not os.path.exists(fw_file):
            panic('Maker error: %s' %(outmsg))

        print_notice('Build successfully!')
        if is_msys():
            print_notice('Firmware: ' + cygpath(fw_file))
        else:
            print_notice('Firmware: ' + fw_file)

    def generate_ota_xml(self, ota_dir):
        root = ET.Element('ota_firmware')
        root.text = '\n\t'
        root.tail = '\n'

        fw_ver =  ET.SubElement(root, 'firmware_version')
        fw_ver.text = '\n\t\t'
        fw_ver.tail = '\n'

        for v in self.fw_version:
                type = ET.SubElement(fw_ver, v)
                type.text = self.fw_version[v]
                type.tail = '\n\t\t'

        parts = ET.SubElement(root, 'partitions')
        parts.text = '\n\t'
        parts.tail = '\n\t'

        ota_part = 0
        for part in self.partitions:
            if ('file_name' in part.keys()) and ('true' == part['enable_ota']):
                 ota_part += 1

        # write part_num firstly for fast search
        part_num = ET.SubElement(parts, 'partitionsNum')
        part_num.text=str(ota_part)
        part_num.tail = '\n\t'

        for part in self.partitions:
            if ('file_name' in part.keys()) and ('true' == part['enable_ota']):
                partfile = os.path.join(ota_dir, part['file_name']);

                part_node = ET.SubElement(parts, 'partition')
                part_node.text = '\n\t\t'
                part_node.tail = '\n\t'

                type = ET.SubElement(part_node, 'type')
                type.text=part['type']
                type.tail = '\n\t\t'

                type = ET.SubElement(part_node, 'name')
                type.text=part['name']
                type.tail = '\n\t\t'

                type = ET.SubElement(part_node, 'file_id')
                type.text=part['file_id']
                type.tail = '\n\t\t'

                type = ET.SubElement(part_node, 'storage_id')
                type.text=part['storage_id']
                type.tail = '\n\t\t'

                url = ET.SubElement(part_node, 'file_name')
                url.text = part['file_name']
                url.tail = '\n\t\t'

                type = ET.SubElement(part_node, 'file_size')
                type.text=str(hex(os.path.getsize(partfile)))
                type.tail = '\n\t\t'

                crc = ET.SubElement(part_node, 'checksum')
                crc.text = str(hex(crc32_file(partfile)))
                crc.tail = '\n\t'

        tree = ET.ElementTree(root)
        tree.write(os.path.join(ota_dir, 'ota.xml'), xml_declaration=True, method="xml", encoding='UTF-8')

    def ota_get_file_seq(self, file_name):
        seq = 0
        #for part in self.partitions:
        for i, part in enumerate(self.partitions):
            if ('file_name' in part.keys()) and ('true' == part['enable_ota']):
                if os.path.basename(file_name) == part['file_name']:
                    seq = i + 1;
                    # boot is the last file in ota firmware
                    if 'BOOT' == part['type']:
                        seq = 0x10000
                    if 'SYS_PARAM' == part['type']:
                        seq = 0x10001
        return seq

    def generate_ota_image_internal(self, ota_file, ota_dir = '', ota_xml = ''):
        if os.path.exists(ota_file):
            os.remove(ota_file)

        if ota_xml == '':
            self.generate_ota_xml(ota_dir)
        else:
            shutil.copyfile(ota_xml, os.path.join(ota_dir, 'ota.xml'))

        # sort files in OTA firmware by upgrade sequence
        files = []
        for parent, dirnames, filenames in os.walk(ota_dir):
            for filename in filenames:
                files.append(os.path.join(parent, filename))
        files.sort(key=self.ota_get_file_seq)

        self.build_ota_image(ota_file, '', files)
        if not os.path.exists(ota_file):
            panic('Failed to generate OTA image')

    def build_ota_image(self, image_path, ota_file_dir, files = []):
        script_ota_path = os.path.join(script_path, 'build_ota_image.py')
        cmd = [sys.executable, script_ota_path,  '-o', image_path]

        if files == []:
            for parent, dirnames, filenames in os.walk(ota_file_dir):
                for filename in filenames:
                    cmd.append(os.path.join(parent, filename))
        else:
            cmd = cmd + files

        (outmsg, exit_code) = run_cmd(cmd)
        if exit_code !=0:
            print('make ota error')
            print(outmsg)
            sys.exit(1)

    def copy_ota_files(self, bin_dir, ota_dir):
        for part in self.partitions:
            if ('file_name' in part.keys()) and ('true' == part['enable_ota']):
                shutil.copyfile(os.path.join(bin_dir, part['file_name']), \
                                os.path.join(ota_dir, part['file_name']))

    def generate_ota_image(self, ota_filename):
        print('FW: Build OTA image \'' +  ota_filename + '\'')

        # generate OTA firmware with crc/randomizer
        self.copy_ota_files(self.bin_dir, self.ota_dir)
        self.generate_ota_image_internal(os.path.join(self.fw_dir, ota_filename), self.ota_dir)

        # generate OTA firmware without crc/randomizer
        self.copy_ota_files(self.orig_bin_dir, self.orig_ota_dir)
        self.generate_ota_image_internal(os.path.join(self.fw_dir, \
                ota_filename + '.orig'), self.orig_ota_dir, os.path.join(self.ota_dir, 'ota.xml'))

    def generate_att_image(self, att_filename, fw_filename):
        self.att_dir = os.path.join(self.fw_dir, 'att')
        if not os.path.isdir(self.att_dir):
            return

        print('ATT: Build ATT image')

        for part in self.partitions:
            if ('file_name' in part.keys())and ('true' == part['enable_dfu']):
                shutil.copyfile(os.path.join(self.bin_dir, part['file_name']), \
                                os.path.join(self.att_dir, part['file_name']))

        maker_cfgfile = os.path.join(self.bin_dir, 'fw_maker.cfg')
        self.generate_maker_config(maker_cfgfile);

        temp_maker_cfgfile = os.path.join(self.bin_dir, 'fw_maker_temp.cfg')
        with open(temp_maker_cfgfile, 'wb') as f:
            with open(maker_cfgfile, 'rb') as f1:
                f1_data = f1.read()
                f.write(f1_data)

            mp_card_cfgfile = os.path.join(self.bin_dir, 'mp_card.cfg')
            with open(mp_card_cfgfile, 'rb') as f2:
                f2_data = f2.read()
                f.write(f2_data)

            maker_ext_cfgfile = os.path.join(self.bin_dir, 'fw_maker_ext.cfg')
            with open(maker_ext_cfgfile, 'rb') as f3:
                f3_data = f3.read()
                f.write(f3_data)

        shutil.copyfile(temp_maker_cfgfile, \
                        os.path.join(self.att_dir, 'fw_maker.cfg'))

        shutil.copyfile(os.path.join(self.fw_dir, fw_filename),
                        os.path.join(self.att_dir, 'attdfu.fw'))

        print('ATT: Generate ATT image %s' %(os.path.join(self.fw_dir, att_filename)))
        self.build_atf_image(os.path.join(self.fw_dir, att_filename), self.att_dir)

    def build_atf_image(self, image_path, atf_file_dir, files = []):
        script_atf_path = os.path.join(script_path, 'build_atf_image.py')
        cmd = [sys.executable, script_atf_path,  '-o', image_path]

        for parent, dirnames, filenames in os.walk(atf_file_dir):
            for filename in filenames:
                files.append(os.path.join(parent, filename))

        cmd = cmd + files

        (outmsg, exit_code) = run_cmd(cmd)
        if exit_code !=0:
            print('make att error')
            print(outmsg)
            sys.exit(1)

    def add_crc(self, filename, chunk_size):
        with open(filename, 'rb') as f:
            filedata = f.read()

        length = len(filedata)
        i = 0
        with open(filename, 'wb') as f:
            while (length > 0):
                if (length < chunk_size):
                    chunk = filedata[i : i + length] + bytearray(chunk_size - length)
                else:
                    chunk = filedata[i : i + chunk_size]
                crc = crc16(bytearray(chunk), 0xffff)
                f.write(chunk + struct.pack('<H',crc))
                length -= chunk_size
                i += chunk_size

    def test_generate_ota_firmwares(self):
        if CFG_TEST_GENERATE_OTA_FIRMWARES_COUNT > 0:
            test_ota_fws = os.path.join(self.fw_dir, 'test_ota_fws')
            if os.path.exists(test_ota_fws):
                shutil.rmtree(test_ota_fws)
                time.sleep(0.1)
            os.mkdir(test_ota_fws)

            ota_temp_dir = os.path.join(test_ota_fws, 'temp_ota')
            shutil.copytree(self.ota_dir, ota_temp_dir)

            ota_temp_dir_orig = os.path.join(test_ota_fws, 'temp_ota_orig')
            shutil.copytree(self.orig_ota_dir, ota_temp_dir_orig)

            for i in range(CFG_TEST_GENERATE_OTA_FIRMWARES_COUNT):
                new_param_file = os.path.join(ota_temp_dir, self.param_file_name)
                orig_param_file = os.path.join(self.orig_bin_dir, self.param_file_name)
                shutil.copyfile(orig_param_file, new_param_file)

                # increase firmware version code
                version_code = int(self.fw_version['version_code'], 0) + 1
                self.fw_version['version_code'] = str(hex(version_code))

                # update fw version field in boot file
                self.generate_firmware_version(new_param_file)

                # copy the origin boot file to temp_ota_orig
                shutil.copyfile(new_param_file, os.path.join(ota_temp_dir_orig, self.param_file_name))

                self.encrypt_param_file(new_param_file)

                ota_fw_name = 'ota_' + str(i + 1) + '.bin'
                print('FW: Build test OTA image \'' +  ota_fw_name + '\'')
                self.generate_ota_image_internal(os.path.join(test_ota_fws, ota_fw_name), ota_temp_dir)

                ota_fw_name = 'ota_' + str(i + 1) + '.bin.orig'
                print('FW: Build test origin OTA image \'' +  ota_fw_name + '\'')
                self.generate_ota_image_internal(os.path.join(test_ota_fws, ota_fw_name), ota_temp_dir_orig, os.path.join(ota_temp_dir, 'ota.xml'))

    def generate_earphone_config(self, mcfg_file, upg_ini_file):
        with open(mcfg_file, 'w') as f, open(upg_ini_file, 'w') as f_u:
            fw_ver_str = json.dumps(self.fw_version)
            fw_ver_str = fw_ver_str.replace('\"', '\'')
            f.write('\nFW_VERSION="%s";\n\n' %fw_ver_str)

            f.write('\nSETPATH="..\\";\n')
            f.write('FIRMWARE_XML="firmware.xml";\n')

            f.write('\nSETPATH=".\\";\n')
            f.write('SPI_STG_CAP=%d;\n' %(self.disk_size // 0x200))
            f.write('BASEFILE="afi.bin";\n')
            f.write('UPGRADE_BAUDRATE=%d;\n' %self.upgrade_baudrate)
            f.write('PRODUCT_CONFIG="%s";\n' %os.path.basename(upg_ini_file))
            f.write('SYS_SDFS="ksdfs.bin";\n')

            f_u.write("version\n")
            f_u.write("baudrate = %d\n" %self.upgrade_baudrate)
            f_u.write("adfu_switch\n")
            # f_u.write("adfu_start\n")

            swrite_data_list = []
            # write dfu data
            for part in self.partitions:
                if ('file_name' in part.keys()) and ('true' == part['enable_dfu']):
                    w_file_name = part['file_name']
                    storage_id = int(part['storage_id'], 0)
                    w_file_addr = int(part['file_address'], 0)
                    if 'BOOT' == part['type']:
                        info_tuple = (w_file_name, storage_id, w_file_addr, True)
                    else:
                        info_tuple = (w_file_name, storage_id, w_file_addr)
                    swrite_data_list.append(info_tuple)

            swrite_data_list.sort(key = lambda x:(x[1], x[2]))

            prv_storage_id = -1
            for info_tuple in swrite_data_list:
                w_file_name = info_tuple[0]
                storage_id = info_tuple[1]
                w_file_addr = info_tuple[2]

                if storage_id != prv_storage_id:
                    f.write("\n")
                    f_u.write("\n")
                    f_u.write("renew_flash_id %d\n" %storage_id)
                    f_u.write("sinit %d\n" %storage_id)
                    prv_storage_id = storage_id

                f_u.write("swrite 0x%08x 0 %s\n" %(w_file_addr, w_file_name))
                file_path = os.path.join(self.bin_dir, w_file_name)
                if os.path.getsize(file_path) > 0x400000:
                    self.atf_file_list.append(file_path)
                else:
                    f.write('WFILE="%s",0x%x;\n' %(w_file_name, w_file_addr))

            f_u.write("\nclear_history\n")
            f_u.write("disconnect reboot\n")

            #add extern data config
            f.write('\n#include "fw_product.cfg"\n')
            f.write('\nSETPATH=".\\config";\n')
            f.write('#include "fw_configuration.cfg"\n')

    def generate_earphone_firmware(self, fw_name):
        print('FW: Build earphone firmware image')
        maker_cfgfile = os.path.join(self.bin_dir, 'fw_maker.cfg')
        upg_cfgfile = os.path.join(self.bin_dir, 'product.ini')
        self.generate_earphone_config(maker_cfgfile, upg_cfgfile);

        upg_file = os.path.join(self.att_dir, 'upgrade.fw')
        fw_file = os.path.join(self.fw_dir, fw_name)

        if (is_windows()):
            cmd = [script_path + '/utils/windows/maker.exe', '-c',  maker_cfgfile, \
                   '-o', upg_file, '-mf']
        else:
            if ('32bit' == platform.architecture()[0]):
                # linux x86
                maker_path = script_path + '/utils/linux-x86/maker/PyMaker.pyo'
            else:
                # linux x86_64
                maker_path = script_path + '/utils/linux-x86_64/maker/PyMaker.pyo'
            cmd = ['python2', '-O', maker_path, '-c', maker_cfgfile, '-o', upg_file, '--mf', '1']

        (outmsg, exit_code) = run_cmd(cmd)
        if not os.path.exists(upg_file):
            panic('Maker error: %s' %(outmsg))

        self.build_atf_image(fw_file, self.att_dir, self.atf_file_list)

        print_notice('Build successfully!')

def main(argv):
    global soc_name, board_name, encrypt_fw, efuse_bin

    parser = argparse.ArgumentParser(
        description='Build firmware',
    )
    parser.add_argument('-c', dest = 'fw_cfgfile')
    parser.add_argument('-e', dest = 'encrypt_fw')
    parser.add_argument('-ef', dest = 'efuse_bin')
    parser.add_argument('-b', dest = 'board_name')
    parser.add_argument('-s', dest = 'soc_name')
    parser.add_argument('-v', dest = 'fw_ver_file')
    args = parser.parse_args();

    fw_cfgfile = args.fw_cfgfile
    encrypt_fw = args.encrypt_fw
    efuse_bin = args.efuse_bin
    board_name = args.board_name
    date_stamp = time.strftime('%y%m%d',time.localtime(time.time()))
    fw_prefix = board_name  + '_' + date_stamp
    soc_name = args.soc_name

    if (not os.path.isfile(fw_cfgfile)):
        print('firmware config file is not exist')
        sys.exit(1)

    try:
        fw = firmware(fw_cfgfile)
        fw.crc_chunk_size = 32
        fw.parse_config(fw_cfgfile)

        """
        if not fw.is_earphone_app:
            fw.generate_raw_image(fw_prefix + '_raw.bin')
            fw.generate_ota_image('ota.bin')

            fw.generate_firmware(fw_prefix + '.fw')
            #fw.generate_att_image(fw_prefix + '_att.fw', fw_prefix + '.fw')

            fw.test_generate_ota_firmwares()
        else:
        """
        fw.generate_earphone_firmware(fw_prefix + '.fw')

    except Exception as e:
        print('\033[1;31;40m')
        print('unknown exception, %s' %(e));
        print('\033[0m')
        # traceback.print_exc()
        sys.exit(2)

if __name__ == '__main__':
    main(sys.argv[1:])
