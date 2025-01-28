#!/usr/bin/env python3

import os
import sys
import argparse
import subprocess
import struct
import re

'''
/**
// 1. tracedump structure
+========+=========+=========+=====+=========+
| header | trace 1 | trace 2 | ... | trace N |
+========+=========+=========+=====+=========+

// 2. trace info
+=========+========+========+========+=====+========+
| uniq id | data 1 | data 2 | func 1 | ... | func N |
+=========+========+========+========+=====+========+
**/

/* tracedump magic and version */
#define TRACED_MAGIC	        (0x44435254) /* TRACED (trace dump) */

/* trace info */
typedef struct traced_info_s {
	uint32_t uniq_id;         /* uniq id */
	uint32_t data[2];         /* user data */
	uint32_t func[9];         /* backtrace */
} traced_info_t;

typedef struct traced_data_s {
	uint32_t magic;         /* magic (file format) */
	uint16_t max_cnt;       /* max count for saving */
	uint16_t cur_cnt;       /* trace count for saving */
	uint16_t start_idx;     /* start index for saving */
	uint16_t end_idx;       /* end index for saving */
	uint32_t uniq_id;       /* uniq id for saving */
	uint32_t data_off;      /* data offset for filter */
	uint32_t data_sz;       /* data size for filter */
	atomic_t locked;        /* lock counter for saving*/
	uint8_t drop_flag : 1;  /* drop flag */
	uint8_t  reserved[3];   /* reserved for extension */
} traced_data_t;
'''

TRACED_MAGIC = 0x44435254  #TRCD
TRACED_HDR_SIZE = 32
TRACED_INFO_SIZE = 48
TRACED_FUNC_OFF = 3
TRACED_FUNC_CNT = 9

ADDR2LINE_EXE = "gcc-arm-none-eabi-9-2020-q2-update-win32\\bin\\arm-none-eabi-addr2line.exe"

def addr2line(elf_file, addr_list):
    tool = os.path.join(os.environ.get('ZEPHYR_TOOLS'),ADDR2LINE_EXE)
    cmd = [tool, "-e", elf_file, "-a", "-f", "-p", "-i", "-s"] + addr_list
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = p.communicate()
    return output.decode()

def log2bin(log_file, bin_file):
    pattern = r'.*\] 0x'
    with open(log_file, 'r') as f1, open(bin_file, 'wb') as f2:
        for line in f1:
            if re.match(pattern, line):
                index = line.rfind("]")
                hex_list = line[index+1:-1].split()
                for str in hex_list:
                    bin_data = struct.pack("<I", int(str, 16))
                    f2.write(bin_data)

def tracedump(elf_file, bin_file, out_file):
    with open(bin_file, 'rb') as f1, open(out_file, 'w') as f2:
        traced_data = f1.read(TRACED_HDR_SIZE)
        if len(traced_data) < TRACED_HDR_SIZE:
            return
        magic, max_cnt, cur_cnt, start_idx, end_idx, uptime, data_off, data_sz  = struct.unpack("<IHHHHIII8x", traced_data)
        if magic != TRACED_MAGIC:
            print('error magic')
            return
        if cur_cnt == 0xffff:
            cur_cnt = (os.path.getsize(bin_file) - TRACED_HDR_SIZE) // TRACED_INFO_SIZE
        f2.write("[tracedump] [0x%08x] (0x%x + 0x%x) (%d ~ %d) (%d / %d)\n" 
                %(uptime, data_off, data_sz, start_idx, end_idx, cur_cnt, max_cnt))

        addr_list = []
        func_list = []
        f1.seek(TRACED_HDR_SIZE + start_idx * TRACED_INFO_SIZE, 0)

        while cur_cnt > 0:
            info_data = f1.read(TRACED_INFO_SIZE)
            if len(info_data) < TRACED_INFO_SIZE:
                f1.seek(TRACED_HDR_SIZE, 0)
                continue
            result = struct.unpack("<"+str(TRACED_INFO_SIZE//4)+"I", info_data)
            if result[TRACED_FUNC_OFF] == 0:
                continue
            cur_cnt = cur_cnt - 1

            res_str = ""
            for addr in result:
                res_str += hex(addr) + " "
            addr_list.append(res_str)
            for addr in result[TRACED_FUNC_OFF:]:
                func_list.append(hex(addr))
            if len(func_list) >= 256 * TRACED_FUNC_CNT or cur_cnt == 0:
                result = addr2line(elf_file, func_list)
                line_cnt = 0
                for line in result.splitlines():
                    if line[0] != " " and (line_cnt % TRACED_FUNC_CNT) == 0:
                        item_idx = line_cnt // TRACED_FUNC_CNT
                        f2.write("\n" + "=" * 132 + "\n")
                        f2.write("[tracedump] %s\n\n" %(addr_list[item_idx]))
                    if line[:10] != "0x00000000":
                        f2.write(line + "\n")
                    if line[0] == " ":
                        continue
                    else:
                        line_cnt = line_cnt + 1
                addr_list = []
                func_list = []

def main(argv):
    parser = argparse.ArgumentParser(
        description='tracedump parser',
    )
    parser.add_argument('elf_file')
    parser.add_argument('bin_file')
    parser.add_argument('out_file')
    args = parser.parse_args()

    (in_name,ext) = os.path.splitext(args.bin_file)
    if ext == ".bin":
        bin_file = args.bin_file
    else:
        bin_file = in_name + '.bin'
        log2bin(args.bin_file, bin_file)
    
    print('tracedump: %s -> %s' %(args.bin_file, args.out_file))
    tracedump(args.elf_file, bin_file, args.out_file)

    return 0

if __name__ == '__main__':
    main(sys.argv[1:])
