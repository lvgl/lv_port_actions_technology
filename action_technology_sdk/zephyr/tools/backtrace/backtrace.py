#!/usr/bin/env python3

import os
import sys
import argparse
import subprocess

ADDR2LINE = "%ZEPHYR_TOOLS%\\gcc-arm-none-eabi-9-2020-q2-update-win32\\bin\\arm-none-eabi-addr2line.exe"
CMD_ZEPHYR = ADDR2LINE + " -e zephyr.elf -a -f -s -p -i "

def run_cmd(cmd):
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = p.communicate()
    return output.decode().replace("\r\n","\n")

def backtrace_log(log_file, out_file):
    with open(log_file, 'r', encoding='utf-8') as fr, open(out_file, 'w') as fw:
        for line in fr:
            index = line.find("%), cycle=")
            if index >= 0:
                index = line.find("  0x")
                if index < 0:
                    index = line.find(" *0x")
                if index >= 0:
                    fw.write("=" * 100 + "\n")
                    fw.write(line[index+2:] + "\n")
            else:
                index = line.find("%ZEPHYR_TOOLS%")
                if index >= 0:
                    line = line.replace("%ZEPHYR_TOOLS%", os.environ.get('ZEPHYR_TOOLS'))
                    outmsg = run_cmd(line[index:].split())
                    fw.write(outmsg)
                    fw.write("\n")

def backtrace_bin(bin_file, out_file):
    cmd_z = CMD_ZEPHYR.replace("%ZEPHYR_TOOLS%", os.environ.get('ZEPHYR_TOOLS'))
    size = os.path.getsize(bin_file)
    with open(bin_file, 'rb') as fr, open(out_file, 'w') as fw:
        for i in range(0, size, 4):
            b = fr.read(4)
            h = int.from_bytes(b, byteorder='little')
            if h > 0x10000000 and h < 0x10400000:
                cmd_z += (hex(h) + " ")
        outmsg = run_cmd(cmd_z.split())
        fw.write(outmsg)
        fw.write("\n")

def main(argv):
    parser = argparse.ArgumentParser(
        description='parse backtrace in log file',
    )
    parser.add_argument('log_file')
    parser.add_argument('out_file')
    args = parser.parse_args();
    
    print('backtrace: %s -> %s' %(args.log_file, args.out_file))
    os.chdir(os.path.dirname(args.log_file))
    
    (in_name,ext) = os.path.splitext(args.log_file)
    if ext == ".bin":
        backtrace_bin(args.log_file, args.out_file)
    else:
        backtrace_log(args.log_file, args.out_file)
    
    return 0

if __name__ == '__main__':
    main(sys.argv[1:])
