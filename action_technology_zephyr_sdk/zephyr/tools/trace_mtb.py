#!/usr/bin/env python3
#
# MTB parser script
#
# Copyright (c) 2024 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0
#
import os
import sys
import argparse
import struct
import subprocess
import re

ADDR2LINE_EXE = "gcc-arm-none-eabi-9-2020-q2-update-win32\\bin\\arm-none-eabi-addr2line.exe"

def addr2line(elf_file, addr_list):
    tool = os.path.join(os.environ.get('ZEPHYR_TOOLS'),ADDR2LINE_EXE)
    cmd = [tool, "-e", elf_file, "-a", "-f", "-p"] + addr_list
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = p.communicate()
    return output.decode()

def log2bin(mtb_log, mtb_bin):
    pattern = r'.*31000..0:'
    with open(mtb_log, 'r') as f1, open(mtb_bin, 'wb') as f2:
        for line in f1:
            if re.match(pattern, line):
                index = line.rfind(":")
                mtb_list = line[index+1:-1].split()
                for str in mtb_list:
                    mtb_data = struct.pack("<I", int(str, 16))
                    f2.write(mtb_data)
    
def mtb_parse_bin(elf_file, mtb_bin):
    addr_list = []
    with open(mtb_bin, 'rb') as f1:
        while True:
            mtb_data = f1.read(8)
            if len(mtb_data) < 8:
                # output addr2line
                if len(addr_list) > 0:
                    print(addr2line(elf_file, addr_list))
                return (0)
            
            src_addr, dst_addr = struct.unpack("<II", mtb_data)
            if (src_addr & 0x01) > 0:
                # output addr2line
                if len(addr_list) > 0:
                    print(addr2line(elf_file, addr_list))
                addr_list = []
                print("=" * 64)
            
            addr_list.append(hex(src_addr))
            addr_list.append(hex(dst_addr))
    return (0)

def mtb_parse_log(elf_file, mtb_log):
    mtb_bin = "tmp.bin"
    log2bin(mtb_log, mtb_bin)
    mtb_parse_bin(elf_file, mtb_bin)
    if os.path.exists(mtb_bin):
        os.remove(mtb_bin)
    return (0)

def main(argv):
    parser = argparse.ArgumentParser(
        description='mtb parser (bin/log file)',
    )
    parser.add_argument('-e', dest = 'elf_file')
    parser.add_argument('-b', dest = 'mtb_bin')
    parser.add_argument('-l', dest = 'mtb_log')
    args = parser.parse_args()
    
    if not args.elf_file:
        parser.print_help()
        sys.exit(1)
    
    if args.mtb_bin:
        mtb_parse_bin(args.elf_file, args.mtb_bin)
    
    if args.mtb_log:
        mtb_parse_log(args.elf_file, args.mtb_log)
    
    return 0

if __name__ == '__main__':
    main(sys.argv[1:])
