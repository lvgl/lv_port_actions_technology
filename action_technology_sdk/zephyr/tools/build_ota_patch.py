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

script_path = os.path.split(os.path.realpath(__file__))[0]

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

def memcpy_n(cbuffer, bufsize, pylist):
    size = min(bufsize, len(pylist))
    for i in range(size):
        cbuffer[i]= ord(pylist[i])

def align_down(data, alignment):
    return data & ~(alignment - 1)

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

def build_ota_image(image_path, ota_file_dir, files = []):
    script_ota_path = os.path.join(script_path, 'build_ota_image.py')
    cmd = ['python3', script_ota_path,  '-o', image_path]

    if os.path.exists(image_path):
        os.remove(image_path)

    if files == []:
        for parent, dirnames, filenames in os.walk(ota_file_dir):
            for filename in filenames:
                cmd.append(os.path.join(parent, filename))
    else:
        cmd = cmd + files

    (outmsg, exit_code) = run_cmd(cmd)
    if exit_code !=0:
        print('make ota error')
        print(outmsg.decode('utf-8'))
        sys.exit(1)


def extract_ota_bin(ota_image_file, out_dir):
    script_ota_path = os.path.join(script_path, 'build_ota_image.py')

    cmd = ['python3', script_ota_path,  '-i', ota_image_file,  '-x', out_dir]
    (outmsg, exit_code) = run_cmd(cmd)
    if exit_code !=0:
        print('extract ota image error')
        print(outmsg.decode('utf-8'))
        sys.exit(1)


def generate_diff_patch(old_file, new_file, patch_file):
    if (is_windows()):
        # windows
        hdiff_path = os.path.join(script_path, 'utils/windows/hdiff.exe')
    else:
        #print(platform.architecture()[0])
        if ('32bit' == platform.architecture()[0]):
            # linux x86
            hdiff_path = os.path.join(script_path, 'utils/linux-x86/hdiff')
        else:
            # linux x86_64
            hdiff_path = os.path.join(script_path, 'utils/linux-x86_64/hdiff')

    cmd = [hdiff_path, old_file, new_file, patch_file]
    #print(cmd)
    (outmsg, exit_code) = run_cmd(cmd)
    if exit_code !=0:
        print('gernerate patch error')
        print(outmsg.decode('utf-8'))
        sys.exit(1)

class ota_file(object):
    def __init__(self, ota_xml_file):
        self.version = 0
        self.part_num = 0
        self.partitions = []
        self.fw_version = {}
        self.tree = None
        self.xml_file = ota_xml_file

        self.parse_xml(ota_xml_file)

    def update_xml_part_item(self, file_id, prop, text):
        print('OTA: update xml file_id: %s' %(file_id))
        root = self.tree.getroot()
        if (root.tag != 'ota_firmware'):
            sys.stderr.write('error: invalid OTA xml file')

        part_list = root.find('partitions').findall('partition')
        for part in part_list:
            #print(part)
            if part.find('file_id').text == file_id:
                item = part.find(prop)
                if item != None:
                    item.text = text

    def write_xml(self):
        print('OTA: write back xml file: %s' %(self.xml_file))
        self.tree.write(self.xml_file, xml_declaration=True, method="xml", encoding='UTF-8')

    def parse_xml(self, xml_file):
        print('OTA: Parse xml file: %s' %(xml_file))
        self.tree = ET.ElementTree(file=xml_file)
        root = self.tree.getroot()
        if (root.tag != 'ota_firmware'):
            sys.stderr.write('error: invalid OTA xml file')
            sys.exit(1)

        firmware_version = root.find('firmware_version')
        for prop in firmware_version:
            self.fw_version[prop.tag] = prop.text.strip()

        part_num_node = root.find('partitions').find('partitionsNum')
        part_num = int(part_num_node.text.strip())
        part_list = root.find('partitions').findall('partition')

        for part in part_list:
            part_prop = {}
            for prop in part:
                part_prop[prop.tag] = prop.text.strip()

            self.partitions.append(part_prop)
            self.part_num = self.part_num + 1

        self.part_num = len(self.partitions);
        #print(self.partitions)
        if self.part_num == 0 or part_num != self.part_num:
            panic('cannot found paritions')

    def get_file_seq(self, file_name):
        seq = 0
        #for part in self.partitions:
        for i, part in enumerate(self.partitions):
            if ('file_name' in part.keys()):
                if os.path.basename(file_name) == part['file_name']:
                    seq = i + 1;
                    # boot is the last file in ota firmware
                    if 'BOOT' == part['type']:
                        seq = 0x10000
        return seq

    def get_sorted_ota_files(self, ota_dir):
        files = []
        for parent, dirnames, filenames in os.walk(ota_dir):
            for filename in filenames:
                files.append(os.path.join(parent, filename))
        files.sort(key = self.get_file_seq)
        return files

def dump_fw_version(ota_ver):
    print('\t' + 'version name: ' + ota_ver.find('version_name').text)
    print('\t' + 'version code: ' + ota_ver.find('version_code').text)
    print('\t' + '  board name: ' + ota_ver.find('board_name').text)

def update_patch_ota_xml(old_ota, new_ota, patch_ota):
    old_ota_root = old_ota.tree.getroot()
    new_ota_root = new_ota.tree.getroot()
    patch_ota_root = patch_ota.tree.getroot()

    old_ota_ver = old_ota_root.find('firmware_version')
    new_ota_ver = new_ota_root.find('firmware_version')

    print('Old FW version:')
    dump_fw_version(old_ota_ver)

    print('New FW version:')
    dump_fw_version(new_ota_ver)

    old_ota_ver_code = int(old_ota_ver.find('version_code').text.strip(), 0)
    new_ota_ver_code = int(new_ota_ver.find('version_code').text.strip(), 0)
    if old_ota_ver_code >= new_ota_ver_code:
        panic('new ota fw version vode is smaller than old fw version')

    old_ota_ver.tag = 'old_firmware_version'
    patch_ota_root.insert(0, old_ota_ver)


def generate_ota_patch_image(patch_ota_file, old_ota_file, new_ota_file, temp_dir):
    temp_old_ota_dir = os.path.join(temp_dir, 'old_ota')
    temp_new_ota_dir = os.path.join(temp_dir, 'new_ota')
    temp_patch_dir = os.path.join(temp_dir, 'new_ota_patch')

    extract_ota_bin(old_ota_file, temp_old_ota_dir)
    extract_ota_bin(new_ota_file, temp_new_ota_dir)

    old_ota = ota_file(os.path.join(temp_old_ota_dir, 'ota.xml'))
    new_ota = ota_file(os.path.join(temp_new_ota_dir, 'ota.xml'))

    if not os.path.exists(temp_patch_dir):
        os.makedirs(temp_patch_dir)

    shutil.copyfile(os.path.join(temp_new_ota_dir, 'ota.xml'), \
                    os.path.join(temp_patch_dir, 'ota.xml'))

    patch_ota = ota_file(os.path.join(temp_patch_dir, 'ota.xml'))

    for part in new_ota.partitions:
        if 'file_name' in part.keys():
            file_name = part['file_name']
            old_file = os.path.join(temp_old_ota_dir, file_name);
            new_file = os.path.join(temp_new_ota_dir, file_name);
            patch_file = os.path.join(temp_patch_dir, file_name);

            generate_diff_patch(old_file, new_file, patch_file)

            #file_size = os.path.getsize(patch_file)
            #checksum = crc32_file(patch_file)
            #patch_ota.update_xml_part_item(part['file_id'], 'file_size', str(hex(file_size)));
            #patch_ota.update_xml_part_item(part['file_id'], 'checksum', str(hex(checksum)));

    update_patch_ota_xml(old_ota, new_ota, patch_ota)
    patch_ota.write_xml()

    files = patch_ota.get_sorted_ota_files(temp_patch_dir)
    build_ota_image(patch_ota_file, '', files)

def main(argv):
    parser = argparse.ArgumentParser(
        description='Build OTA patch firmware',
    )
    parser.add_argument('-o', dest = 'old_ota_file', required=True)
    parser.add_argument('-n', dest = 'new_ota_file', required=True)
    parser.add_argument('-p', dest = 'patch_ota_file', required=True)
    args = parser.parse_args();

    old_ota_file = args.old_ota_file
    new_ota_file = args.new_ota_file
    patch_ota_file = args.patch_ota_file

    if (not os.path.isfile(old_ota_file)):
        panic('cannot found file' + old_ota_file)

    if (not os.path.isfile(new_ota_file)):
        panic('cannot found file' + new_ota_file)

    try:
        temp_dir = os.path.dirname(patch_ota_file)
        generate_ota_patch_image(patch_ota_file, old_ota_file, new_ota_file, temp_dir)

    except Exception as e:
        print('\033[1;31;40m')
        print('unknown exception, %s' %(e));
        print('\033[0m')
        sys.exit(2)

if __name__ == '__main__':
    main(sys.argv[1:])
