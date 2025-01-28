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
import lzma

# lzma compress for ota and temp bin
LZMA_OTA_BIN = 1
LZMA_TEMP_BIN = 1

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
    u32_t   magic;
    u32_t   header_checksum;
    u16_t   header_version;
    u16_t   header_size;
    u16_t   file_cnt;
    u16_t   flag;
    u16_t   dir_offset;
    u16_t   data_offset;
    u32_t   data_size;
    u32_t   data_checksum;
    u8_t    reserved[8];

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
    struct ota_fw_hdr   hdr;
    u8_t            reserved1[32];

    /* offset: 0x40 */
    struct ota_fw_ver   new_ver;

    /* offset: 0xa0 */
    struct ota_fw_ver   old_ver;

    u8_t            reserved2[32];

    /* offset: 0x200 */
    struct ota_fw_dir   dir;
};

#define LZMA_MAGIC      0x414d5a4c

typedef struct lzma_head {
	uint32_t  ih_magic;
	uint32_t ih_hdr_size;    /* Size of image header (bytes). */
	uint32_t ih_img_size;    /* Size of image body (bytes). */
	uint32_t ih_org_size;   /* Size of origin data (bytes) */ 
}lzma_head_t;

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

class OTAFile(object):
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

LZMA_HDR_SIZE = 0x10
LZMA_MAGIC = 0x414d5a4c   #LZMA

class ota_fw(object):
    def __init__(self):
        self.old_version = None
        self.new_version = None
        self.disk_size = 0
        self.fw_version = {}
        self.partitions = []

    def _get_partition_info(self, keyword, keystr):
        if self.partitions:
            for info_dict in self.partitions:
                if keyword in info_dict and keystr == info_dict[keyword]:
                    return info_dict
        return {}

    def parse_config(self, cfg_file, board_name, output_file):
        #print('OTA: Parse config file: %s' %cfg_file)
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
            file_name = os.path.basename(output_file)
            cur_time = time.strftime('%y%m%d%H%M',time.localtime(time.time()))
            version_name = self.fw_version['version_name'].replace('$(build_time)', cur_time)
            self.fw_version['version_name'] = version_name

        self.fw_version['board_name'] = board_name
        part_list = root.find('partitions').findall('partition')
        for part in part_list:
            part_prop = {}
            for prop in part:
                part_prop[prop.tag] = prop.text.strip()

            self.partitions.append(part_prop)

    def dump_info(self, ota_files):
        i = 0
        for file in ota_files:
            print('[%d]' %(i))
            i = i + 1
            file.dump_info()

    def generate_ota_xml(self, ota_file_list, ota_xml):
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

        # write part_num firstly for fast search
        part_num = ET.SubElement(parts, 'partitionsNum')
        part_num.text=str(len(ota_file_list))
        part_num.tail = '\n\t'

        for partfile in ota_file_list:
            orig_partfile = partfile + ".orig"
            if not os.path.exists(orig_partfile):
                orig_partfile = partfile

            part = self._get_partition_info('file_name', os.path.basename(partfile))
            if not part:
                continue

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

            type = ET.SubElement(part_node, 'orig_size')
            type.text=str(hex(os.path.getsize(orig_partfile)))
            type.tail = '\n\t\t'

            crc = ET.SubElement(part_node, 'checksum')
            crc.text = str(hex(crc32_file(orig_partfile)))
            crc.tail = '\n\t'

        tree = ET.ElementTree(root)
        tree.write(ota_xml, xml_declaration=True, method="xml", encoding='UTF-8')

    def ota_get_file_seq(self, file_name):
        seq = 0
        file_name = os.path.basename(file_name)
        #for part in self.partitions:
        for i, part in enumerate(self.partitions):
            if ('file_name' in part.keys()) and ('true' == part['enable_ota']):
                if file_name == part['file_name']:
                    seq = i + 1;
                    # boot is the last file in ota firmware
                    if 'BOOT' == part['type']:
                        seq = 0x10000
                    if 'SYS_PARAM' == part['type']:
                        seq = 0x10001
        return seq

    def pack(self, ota_file_list, output_file):
        if len(ota_file_list) > 14:
            panic('OTA: too much input files')

        if len(ota_file_list) == 1 and os.path.isdir(ota_file_list[0]):
            files = [name for name in os.listdir(ota_file_list[0]) \
                     if os.path.isfile(os.path.join(ota_file_list[0], name))]
        else:
            files = ota_file_list

        ota_files = []
        for file in files:
            ota_files.append(OTAFile(file))
        self.dump_info(ota_files)

        file_data = bytearray(0)
        head_data = bytearray(0)
        dir_data = bytearray(0)

        data_offset = OTA_FIRMWARE_DATA_OFFSET
        dir_entry = bytearray(0)
        for file in ota_files:
            dir_entry = struct.pack("<12s4xII4xI", file.file_name, data_offset, file.length, file.checksum)
            pad_len = calc_pad_length(file.length, 512)
            data_offset = data_offset + file.length + pad_len
            file_data = file_data + file.data + bytearray(pad_len)
            dir_data = dir_data + dir_entry

        data_crc = zlib.crc32(file_data, 0) & 0xffffffff

        pad_len = calc_pad_length(len(dir_data), 512)
        dir_data = dir_data + bytearray(pad_len)

        head_data = struct.pack("<4sIHHHHHHII36x", OTA_FIRMWARE_MAGIC, 0, \
                                    OTA_FIRMWARE_VERSION, OTA_FIRMWARE_HEADER_SIZE, \
                                    len(ota_files), 0, \
                                    OTA_FIRMWARE_DIR_OFFSET, OTA_FIRMWARE_DATA_OFFSET, \
                                    data_offset, data_crc)

        # add fw version
        xml_fpath = ''
        for file in ota_files:
            if file.file_name == b'ota.xml':
                xml_fpath = file.file_path
        if xml_fpath == '':
            panic('cannot found ota.xml file')

        fw_ver = fw_version(xml_fpath, 'firmware_version')
        if not fw_ver.valid:
            panic('cannot found ota.xml file')

        head_data = head_data + fw_ver.get_data()

        old_fw_ver = fw_version(xml_fpath, 'old_firmware_version')
        if old_fw_ver.valid:
            head_data = head_data + old_fw_ver.get_data()

        head_data = head_data + bytearray(calc_pad_length(len(head_data), 512))

        header_crc = zlib.crc32(head_data[8:] + dir_data, 0) & 0xffffffff
        head_data = head_data[0:4] + struct.pack('<I', header_crc) + \
            head_data[8:]

        with open(output_file, 'wb') as f:
            f.write(head_data)
            f.write(dir_data)
            f.write(file_data)

    def __build_ota_image(self, ota_file_list, image_name, lzma=0):
        if not ota_file_list:
            panic('NO OTA files input')

        if lzma > 0:
            for partfile in ota_file_list:
                if os.path.basename(partfile) != 'TEMP.bin':
                    self.__build_lzma_image(partfile, 0x8000, 1)  #32K
            
        ota_file_list.sort(key=self.ota_get_file_seq)
        ota_dir = os.path.dirname(ota_file_list[0])
        ota_xml = os.path.join(ota_dir, 'ota.xml')
        self.generate_ota_xml(ota_file_list, ota_xml)

        ota_file_list.insert(0, ota_xml)
        self.pack(ota_file_list, image_name)
        if not os.path.exists(image_name):
            panic('Failed to generate OTA image %s' %image_name)

    def __build_lzma_image(self, image_name, blk_size, backup=0):
        if not os.path.exists(image_name):
            return

        orig_image = image_name + ".orig"
        if os.path.exists(orig_image):
            os.remove(orig_image)
        os.rename(image_name,orig_image)

        with open(image_name, 'wb') as f1, open(orig_image, 'rb') as f2:
            while True:
                raw_data = f2.read(blk_size)
                if len(raw_data) == 0:
                    break
                xz_data = lzma.compress(raw_data)
                hdr_data = struct.pack("<IIII", LZMA_MAGIC, LZMA_HDR_SIZE, len(xz_data), len(raw_data))
                f1.write(hdr_data)
                f1.write(xz_data)

        if backup == 0:
            os.remove(orig_image)

    def __get_psram_size(self, zephyr_cfg, boot_ini):
        bret = 0
        with open(zephyr_cfg, "r") as f:
            lines = f.readlines()
            for elem in lines:
                _c = elem.strip().split("=")
                if _c[0] == "CONFIG_PSRAM_SIZE":
                    bret = int(_c[1])
                    break;
        with open(boot_ini, "r") as f:
            lines = f.readlines()
            for elem in lines:
                _c = elem.strip().split("=")
                if _c[0].strip() == "psram_code":
                    bret = bret - (int(_c[1],16) >> 10)
                    break;
        return bret

    def __build_temp_image(self, zephyr_cfg, boot_ini, temp_bin, ota_app_bin, lzma=0):
        if not os.path.exists(temp_bin):
            return

        if lzma > 0:
            psram_size = self.__get_psram_size(zephyr_cfg, boot_ini)
            if psram_size >= 4096:
                blk_size = 0x200000  #2048K
            elif psram_size >= 1024:
                blk_size = 0x40000  #256K
            else:
                blk_size = 0x8000  #32K
            self.__build_lzma_image(temp_bin, blk_size, 0)
        
        if os.path.exists(ota_app_bin):
            with open(ota_app_bin, 'rb') as f1, open(temp_bin, 'rb') as f2:
                app_data = f1.read()
                temp_data = f2.read()
            with open(temp_bin, 'wb') as f1:
                f1.write(app_data)
                f1.write(temp_data)

    def generate_ota_image_internal(self, ota_file, ota_dir = '', ota_xml = ''):
        if os.path.exists(ota_file):
            os.remove(ota_file)

        files = []
        # embed ota file info: embed type: embed file list
        embed_ota_image_dict = {}
        for part in self.partitions:
            if 'file_name' in part and ('true' == part['enable_ota']):
                file_name = os.path.join(ota_dir, part['file_name'])
                if 'ota_embed' in part:
                    ota_embed_type = part['ota_embed']
                    if ota_embed_type in embed_ota_image_dict:
                        embed_ota_image_dict[ota_embed_type].append(file_name)
                    else:
                        embed_ota_image_dict[ota_embed_type] = [file_name]
                else:
                    files.append(file_name)

        for ota_embed_type in embed_ota_image_dict:
            embed_file_list = embed_ota_image_dict[ota_embed_type]
            embed_image = os.path.join(ota_dir, "%s.bin" %ota_embed_type)
            self.__build_ota_image(embed_file_list, embed_image, 0)
            files.append(embed_image)
            part = self._get_partition_info('type', ota_embed_type)
            part['file_name'] = os.path.basename(embed_image)

        temp_bin = os.path.join(ota_dir, 'TEMP.bin')
        ota_app_bin = os.path.join(ota_dir, 'ota_app.bin')
        out_dir = os.path.dirname(os.path.dirname(ota_dir))
        zephyr_cfg = os.path.join(out_dir, 'zephyr', ".config")
        boot_ini = os.path.join(out_dir, '_firmware', 'bin', "bootloader.ini")
        self.__build_temp_image(zephyr_cfg,boot_ini,temp_bin,ota_app_bin, LZMA_TEMP_BIN)

        self.__build_ota_image(files, ota_file, LZMA_OTA_BIN)
        if not os.path.exists(ota_file):
            panic('Failed to generate OTA image')

    def copy_ota_files(self, bin_dir, ota_dir):
        ota_app_bin = os.path.join(bin_dir, 'ota_app.bin')
        if os.path.exists(ota_app_bin):
            shutil.copyfile(ota_app_bin, os.path.join(ota_dir, 'ota_app.bin'))
        for part in self.partitions:
            if ('file_name' in part.keys()) and ('true' == part['enable_ota']):
                shutil.copyfile(os.path.join(bin_dir, part['file_name']), \
                                os.path.join(ota_dir, part['file_name']))

    def generate_ota_image(self, input_file_dir, ota_filename):
        print('FW: Build OTA image \'' +  ota_filename + '\'')

        ota_file_dir = os.path.join(os.path.dirname(ota_filename),
            os.path.splitext(os.path.basename(ota_filename))[0])

        if os.path.isdir(ota_file_dir):
            shutil.rmtree(ota_file_dir)
        os.mkdir(ota_file_dir)
        # generate OTA firmware with crc/randomizer
        self.copy_ota_files(input_file_dir, ota_file_dir)
        self.generate_ota_image_internal(ota_filename, ota_file_dir)


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
    parser.add_argument('-b', dest = 'board_name')
    parser.add_argument('-c', dest = 'fw_cfgfile')
    parser.add_argument('-i', dest = 'input_file_dir')
    parser.add_argument('-o', dest = 'output_file')
    parser.add_argument('-x', dest = 'extract_out_dir')
    args = parser.parse_args();

    print('ATF: Build OTA firmware image: %s' %args.output_file)

    if args.extract_out_dir:
        image_file = args.input_file_dir
        unpack_fw(image_file, args.extract_out_dir)
    else:
        if (not args.output_file) :
            panic('no file')

        fw = ota_fw()
        fw.parse_config(args.fw_cfgfile, args.board_name, args.output_file)
        fw.generate_ota_image(args.input_file_dir, args.output_file)

    return 0

if __name__ == '__main__':
    main(sys.argv[1:])
