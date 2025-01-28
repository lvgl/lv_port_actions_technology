#!/usr/bin/env python
#
# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0
#

import os
import sys
import glob
import time
import argparse
import platform
import struct
import shutil
import subprocess
import re
import xml.etree.ElementTree as ET

sdk_arch = "arm"
sdk_root = os.getcwd()
sdk_root_parent = os.path.dirname(sdk_root)
#sdk_mcuboot_dir=os.path.join(sdk_root_parent, "bootloader", "mcuboot")
#sdk_mcuboot_imgtool = os.path.join(sdk_mcuboot_dir, "scripts", "imgtool.py")
#sdk_mcuboot_key= os.path.join(sdk_mcuboot_dir, "root-rsa-2048.pem")

#sdk_mcuboot_app_path=os.path.join(sdk_mcuboot_dir, "boot", "zephyr")
sdk_boards_dir = os.path.join(sdk_root, "boards", sdk_arch)
#sdk_application_dir = os.path.join(sdk_root_parent, "application")
sdk_application_dir=""
sdk_app_dir = os.path.join(sdk_root_parent, "application")
sdk_samples_dir = os.path.join(sdk_root, "samples")
sdk_tests_dir = os.path.join(sdk_root, "tests")
sdk_script_dir = os.path.join(sdk_root, "tools")
sdk_script_prebuilt_dir = os.path.join(sdk_script_dir, "prebuilt")
build_config_file = os.path.join(sdk_root, ".build_config")

application = ""
sub_app = ""
board_conf = ""
app_conf = ""

def is_windows():
    sysstr = platform.system()
    if (sysstr.startswith('Windows') or \
       sysstr.startswith('MSYS') or     \
       sysstr.startswith('MINGW') or    \
       sysstr.startswith('CYGWIN')):
        return True
    else:
        return False

def to_int(str):
    try:
        int(str)
        return int(str)
    except ValueError:
        try:
            float(str)
            return int(float(str))
        except ValueError:
            return False

def read_choice(  tip, path):
    print("")
    print(" %s" %(tip))
    print("")
    dirs = next(os.walk(path))[1]
    i = 0

    for file in dirs:
        file_path = os.path.join(path, file)
        if os.path.isdir(file_path):
            i += 1
            print("    %d. %s" %(i, file))

    print("")
    input_prompt = "Which would you like? [" + dirs[0] + "] "
    str = ""
    try:
        str = input(input_prompt)
    except Exception as ex:
        print("")
    if to_int(str) != False:
        j = to_int(str)
    else:
        j = 1

    if j > i:
        j = i
    j -= 1
    return dirs[j]


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
    #print("%s" % (output.rstrip()))
    return (output, p.returncode)

def build_nvram_bin(out_dir, board_dir, app_dir, app_cfg_dir):
    nvram_path = os.path.join(out_dir, "nvram.bin")
    app_cfg_path =  os.path.join(app_cfg_dir, "nvram.prop")
    if os.path.exists(app_cfg_path) == False:
        app_cfg_path =  os.path.join(app_dir, "nvram.prop")
    board_cnf_path =  os.path.join(board_dir, "nvram.prop")
    print("\n build_nvram \n")
    if not os.path.isfile(board_cnf_path):
        print("\n  board nvram %s not exists\n" %(board_cnf_path))
        return
    if not os.path.isfile(app_cfg_path):
        print("\n  app nvram %s not exists\n" %(app_cfg_path))
        return
    script_nvram_path = os.path.join(sdk_script_dir, 'build_nvram_bin.py')
    cmd = ['python', '-B', script_nvram_path,  '-o', nvram_path, app_cfg_path, board_cnf_path]
    (outmsg, exit_code) = run_cmd(cmd)
    if exit_code !=0:
        print('make build_nvram_bin error')
        print(outmsg.decode('utf-8'))
        sys.exit(1)
    print("\n build_nvram  finshed\n\n")

def dir_cp(out_dir, in_dir):
    print(" cp dir %s  to dir %s"  %(in_dir, out_dir))
    if os.path.exists(in_dir) == True:
        if os.path.exists(out_dir) == True:
            for parent, dirnames, filenames in os.walk(in_dir):
                for filename in filenames:
                    pathfile = os.path.join(parent, filename)
                    shutil.copyfile(pathfile, os.path.join(out_dir, filename))
        else:
            shutil.copytree(in_dir, out_dir)

def dir_tree_cp(out_dir, in_dir):
    # keep directory structure
    print(" cp dir %s  to dir %s"  %(in_dir, out_dir))
    if os.path.exists(in_dir):
        if os.path.exists(out_dir):
            for parent, dirnames, filenames in os.walk(in_dir):
                for filename in filenames:
                    copy_to_dir = os.path.join(out_dir, os.path.relpath(parent, in_dir))
                    if not os.path.isdir(copy_to_dir):
                        os.mkdir(copy_to_dir)
                    pathfile = os.path.join(parent, filename)
                    shutil.copyfile(pathfile, os.path.join(copy_to_dir, filename))
        else:
            shutil.copytree(in_dir, out_dir)

def check_cp(out_dir, in_dir, name):
    in_cp = os.path.join(in_dir, name)
    out_cp = os.path.join(out_dir, name)
    if os.path.exists(in_cp):
        if os.path.isfile(in_cp) == True:
            print(" cp file %s  to dir %s" %(in_cp,out_cp))
            shutil.copyfile(in_cp, out_cp)
        else :
            dir_tree_cp(out_dir, in_cp)

def build_sdfs_bin_name(out_dir, board_dir, app_dir, app_cfg_dir, sdfs_name):
    board_sdfs_dir = os.path.join(board_dir, sdfs_name)
    app_sdfs_dir = os.path.join(app_dir, sdfs_name)
    app_cfg_sdfs_dir = os.path.join(app_cfg_dir, sdfs_name)
    #print("\n build_sdfs : sdfs_dir %s, board dfs %s\n\n" %(sdfs_dir, board_sdfs_dir))
    if (os.path.exists(board_sdfs_dir) == False) \
        and (os.path.exists(app_sdfs_dir) == False) \
        and (os.path.exists(app_cfg_sdfs_dir) == False):
        print("\n build_sdfs :  %s not exists\n\n" %(sdfs_name))
        return

    sdfs_dir = os.path.join(out_dir, sdfs_name)
    if os.path.exists(sdfs_dir) == True:
        shutil.rmtree(sdfs_dir)
        time.sleep(0.1)

    if os.path.exists(board_sdfs_dir) ==  True:
        dir_cp(sdfs_dir, board_sdfs_dir)

    if os.path.exists(app_sdfs_dir) == True:
        dir_cp(sdfs_dir, app_sdfs_dir)

    if os.path.exists(app_cfg_sdfs_dir) == True:
        dir_cp(sdfs_dir, app_cfg_sdfs_dir)

    print("\n build_sdfs : sdfs_dir %s, sdfs_name %s\n\n" %(sdfs_dir, sdfs_name))
    if (sdfs_name == 'fonts') or (sdfs_name == 'res') or (sdfs_name == 'res_b'):
        txtdir = os.path.join(sdfs_dir, "txt")
        if os.path.exists(txtdir) == True:
            shutil.rmtree(txtdir)
        for infile in glob.glob(os.path.join(sdfs_dir, '*.txt')):
            print(" remove file :  %s\n" %(infile))
            os.remove(infile)

    if (sdfs_name == 'ksdfs') or (sdfs_name == 'sdfs'):
        ksym_bin = os.path.join(out_dir, "ksym.bin")
        if os.path.exists(ksym_bin):
            shutil.move(ksym_bin, os.path.join(sdfs_dir, "ksym.bin"))

    script_sdfs_path = os.path.join(sdk_script_dir, 'build_sdfs.py')
    sdfs_bin_path = os.path.join(out_dir, sdfs_name+".bin")
    cmd = ['python', '-B', script_sdfs_path,  '-o', sdfs_bin_path, '-d', sdfs_dir]
    (outmsg, exit_code) = run_cmd(cmd)
    if exit_code !=0:
        print('make build_sdfs_bin error')
        print(outmsg.decode('utf-8'))
        sys.exit(1)
    print("\n build_sdfs %s finshed\n\n" %(sdfs_name))

def build_sdfs_bin_bydir(out_dir, board_dir, app_dir, app_cfg_dir):
    bpath = os.path.join(board_dir, "fs_sdfs");
    apath = os.path.join(app_cfg_dir, "fs_sdfs");
    if os.path.exists(apath) == False:
        apath = os.path.join(app_dir, "fs_sdfs");
    if (os.path.exists(bpath) == False) and (os.path.exists(apath) == False):
        print('not fs_sdfs')
        return
    all_dirs = {}
    if os.path.exists(bpath) == True:
        for file in os.listdir(bpath):
            print("board list %s ---\n" %(file))
            if os.path.isdir(os.path.join(bpath,file)):
                all_dirs[file] = file
    if os.path.exists(apath) == True:
        for file in os.listdir(apath):
            print("app list %s ---\n" %(file))
            if os.path.isdir(os.path.join(apath,file)):
                all_dirs[file] = file
    for dir in all_dirs.keys():
        print("--build_sdfs %s ---\n" %(dir))
        build_sdfs_bin_name(out_dir, bpath, apath, app_cfg_dir, dir)


def build_sdfs_bin(out_dir, board_dir, app_dir, app_cfg_dir):
    build_sdfs_bin_name(out_dir, board_dir, app_dir, app_cfg_dir,"ksdfs")
    build_sdfs_bin_name(out_dir, board_dir, app_dir, app_cfg_dir,"sdfs")
    build_sdfs_bin_name(out_dir, board_dir, app_dir, app_cfg_dir,"fatfs")
    build_sdfs_bin_name(out_dir, board_dir, app_dir, app_cfg_dir,"res")
    build_sdfs_bin_name(out_dir, board_dir, app_dir, app_cfg_dir,"fonts")
    build_sdfs_bin_name(out_dir, board_dir, app_dir, app_cfg_dir,"res_b")
    build_sdfs_bin_bydir(out_dir, board_dir, app_dir, app_cfg_dir)

def file_link(filename, file_add):
    if not os.path.exists(filename):
        return
    if not os.path.exists(file_add):
        return
    print("file %s, link %s \n" %(filename, file_add))
    with open(file_add, 'rb') as fa:
        file_data = fa.read()
        fa.close()
    fsize = os.path.getsize(filename)
    with open(filename, 'rb+') as f:
        f.seek(fsize, 0)
        f.write(file_data)
        f.close()

def is_config_KALLSYMS(cfg_path):
    bret = False
    print("\n cfg_path =%s\n" %cfg_path)
    fp = open(cfg_path,"r")
    lines = fp.readlines()
    for elem in lines:
        if elem[0] != '#':
            _c = elem.strip().split("=")
            if _c[0] == "CONFIG_KALLSYMS":
                print("CONFIG_KALLSYMS");
                bret = True
                break;
    fp.close
    print("\nCONFIG_KALLSYMS=%d\n" %bret )
    return bret

def build_mbr_bin(mbr_file, udisk_offset, udisk_size):
    bootcode = b'\x00' * 446
    part1 = struct.pack("<BBBBBBBBII", 0x0, 0x01, 0x01, 0x00, \
                0xb, 0xfe, 0xff, 0xff, udisk_offset//512, udisk_size//512)
    part2 = struct.pack("<BBBBBBBBII", 0, 0, 0, 0, \
                0, 0, 0, 0, 0, 0)
    part3 = struct.pack("<BBBBBBBBII", 0, 0, 0, 0, \
                0, 0, 0, 0, 0, 0)
    part4 = struct.pack("<BBBBBBBBII", 0, 0, 0, 0, \
                0, 0, 0, 0, 0, 0)
    id = struct.pack("<BB", 0x55, 0xaa)
    with open(mbr_file, 'wb') as f:
        f.write(bootcode)
        f.write(part1)
        f.write(part2)
        f.write(part3)
        f.write(part4)
        f.write(id)

def build_datafs_bin(out_dir, app_dir, app_cfg_dir, fw_cfgfile):
    script_datafs_path = os.path.join(sdk_script_dir, 'build_datafs.py')

    tree = ET.ElementTree(file=fw_cfgfile)
    root = tree.getroot()
    if (root.tag != 'firmware'):
        sys.stderr.write('error: invalid firmware config file')
        sys.exit(1)

    part_list = root.find('partitions').findall('partition')
    for part in part_list:
        part_prop = {}
        for prop in part:
            part_prop[prop.tag] = prop.text.strip()
        if ('storage_id' in part_prop.keys()) and (part_prop['name'] == 'udisk'):
            mbr_bin_path = os.path.join(out_dir, 'u_mbr.bin')
            build_mbr_bin(mbr_bin_path, int(part_prop['address'], 0), int(part_prop['size'], 0))
            app_datafs_dir = os.path.join(app_cfg_dir, part_prop['name'])
            if os.path.exists(app_datafs_dir) == False:
                app_datafs_dir = os.path.join(app_dir, part_prop['name'])
                if os.path.exists(app_datafs_dir) == False:
                    print("\n build_datafs : app config datafs %s not exists\n\n" %(app_datafs_dir))
                    continue
            datafs_cap = int(part_prop['size'], 0)
            datafs_bin_path = os.path.join(out_dir, part_prop['file_name'])
            print('DATAFS (%s) capacity => 0x%x' %(datafs_bin_path, datafs_cap))
            cmd = ['python', '-B', script_datafs_path,  '-o', datafs_bin_path, '-s', str(datafs_cap), '-d', app_datafs_dir]
            (outmsg, exit_code) = run_cmd(cmd)
            if exit_code !=0:
                print('make build_datafs_bin %s error' %(datafs_bin_path))
                print(outmsg.decode('utf-8'))
                sys.exit(1)

    print("\n build_datafs finshed\n\n")

def read_littlefs_block_size_config(cfg_path):
    block_size = 4096
    fp = open(cfg_path,"r")
    lines = fp.readlines()
    for elem in lines:
        if elem[0] != '#':
            _c = elem.strip().split("=")
            if _c[0] == "CONFIG_FS_LITTLEFS_BLOCK_SIZE":
                block_size = int(_c[1].strip('"'), 0)
                break;
    fp.close
    return block_size

def build_littlefs_bin(out_dir, app_dir, app_cfg_dir, fw_cfgfile):
    script_littlefs_path = os.path.join(sdk_script_dir, 'build_littlefs.py')
    OS_OUTDIR = os.path.join(out_dir, "../../zephyr")

    tree = ET.ElementTree(file=fw_cfgfile)
    root = tree.getroot()
    if (root.tag != 'firmware'):
        sys.stderr.write('error: invalid firmware config file')
        sys.exit(1)

    part_list = root.find('partitions').findall('partition')
    for part in part_list:
        part_prop = {}
        for prop in part:
            part_prop[prop.tag] = prop.text.strip()
        if ('storage_id' in part_prop.keys()) and (part_prop['name'] == 'littlefs'):
            app_littlefs_dir = os.path.join(app_cfg_dir, part_prop['name'])
            if os.path.exists(app_littlefs_dir) == False:
                app_littlefs_dir = os.path.join(app_dir, part_prop['name'])
                if os.path.exists(app_littlefs_dir) == False:
                    print("\n build_datafs : app config littlefs %s not exists\n\n" %(app_littlefs_dir))
                    continue
            littlefs_cap = int(part_prop['size'], 0)
            littlefs_bin_path = os.path.join(out_dir, part_prop['file_name'])
            blocksize = read_littlefs_block_size_config(os.path.join(OS_OUTDIR, ".config"))
            print('LITTLEFS (%s) capacity => 0x%x' %(littlefs_bin_path, littlefs_cap))
            cmd = ['python', '-B', script_littlefs_path, '-o', littlefs_bin_path, '-s', str(littlefs_cap), '-d', app_littlefs_dir, '-b', str(blocksize)]
            (outmsg, exit_code) = run_cmd(cmd)
            if exit_code !=0:
                print('make build_littlefs_bin %s error' %(littlefs_bin_path))
                print(outmsg.decode('utf-8'))
                sys.exit(1)

    print('\n build_littlefs finshed \n\n')

def read_val_by_config(cfg_path, cfg_name):
    val = "0"
    fp = open(cfg_path,"r")
    lines = fp.readlines()
    for elem in lines:
        if elem[0] != '#':
            _c = elem.strip().split("=")
            if _c[0] == cfg_name:
                val = _c[1].strip('"')
                break;
    fp.close
    return val

def read_soc_by_config(cfg_path):
    soc = "lark"
    fp = open(cfg_path,"r")
    lines = fp.readlines()
    for elem in lines:
        if elem[0] != '#':
            _c = elem.strip().split("=")
            if _c[0] == "CONFIG_SOC_SERIES":
                soc = _c[1].strip('"')
                break;
    fp.close
    return soc

def image_add_checksum(fw_dir_bin, img_bin):
    img_bin_file = os.path.join(fw_dir_bin, img_bin)
    if os.path.exists(img_bin_file) == False:
        return
    print("\nFW: Post build %s\n"%img_bin)
    script_firmware_path = os.path.join(sdk_script_dir, 'pack_app_image.py')
    cmd = ['python', '-B', script_firmware_path, img_bin_file]
    print("\n build cmd : %s\n\n" %(cmd))
    (outmsg, exit_code) = run_cmd(cmd)
    if exit_code !=0:
        print("pack %s error\n"%img_bin)
        print(outmsg)
        sys.exit(1)


def build_firmware(board, out_dir, board_dir, app_dir, app_cfg_dir):
    print("\n build_firmware : out %s, board %s, app_dir %s cfg_dir %s\n\n" %(out_dir, board_dir, app_dir, app_cfg_dir))
    res_files = ["fonts", "res", "res_b"]
    full_files = ["full","firmware"]
    FW_DIR = os.path.join(out_dir, "_firmware")
    OS_OUTDIR = os.path.join(out_dir, "zephyr")
    if os.path.exists(FW_DIR) == False:
        os.mkdir(FW_DIR)
    FW_DIR_BIN = os.path.join(FW_DIR, "bin")
    if os.path.exists(FW_DIR_BIN) == False:
        os.mkdir(FW_DIR_BIN)

    if is_config_KALLSYMS(os.path.join(OS_OUTDIR, ".config")):
        print("cpy ksym.bin- to fw dir-");
        check_cp(FW_DIR_BIN , OS_OUTDIR, "ksym.bin")


    build_nvram_bin(FW_DIR_BIN, board_dir, app_dir, app_cfg_dir)
    build_sdfs_bin(FW_DIR_BIN, board_dir, app_dir, app_cfg_dir)
    soc = read_soc_by_config(os.path.join(OS_OUTDIR, ".config"))
    print("\n build_firmware : soc name= %s\n" %(soc))
    dir_cp(FW_DIR_BIN, os.path.join(sdk_script_prebuilt_dir, soc, "common", "bin"))
    dir_cp(FW_DIR_BIN, os.path.join(sdk_script_prebuilt_dir, soc, "bin"))
    dir_tree_cp(FW_DIR_BIN, os.path.join(sdk_script_prebuilt_dir, soc, "prebuild"))

    fw_cfgfile = os.path.join(FW_DIR, "firmware.xml")

    #check copy firmware.xml
    check_cp(FW_DIR, board_dir, "firmware.xml")
    check_cp(FW_DIR, app_dir, "firmware.xml")
    check_cp(FW_DIR, app_cfg_dir, "firmware.xml")
    #check copy firmware_debug.xml
    check_cp(FW_DIR, app_cfg_dir, "firmware_debug.xml")
    if not os.path.exists(fw_cfgfile):
        print("failed to find out firmware.xml")
        sys.exit(1)

    #check copy firmware_res.xml
    #res_cfgfile = os.path.join(FW_DIR, "firmware_res.xml")
    #check_cp(FW_DIR, app_dir, "firmware_res.xml")
    #check_cp(FW_DIR, app_cfg_dir, "firmware_res.xml")
    
    for res_file in res_files:
        res_cfgfile = os.path.join(FW_DIR, "ota_"+ res_file +".xml")
        check_cp(FW_DIR, app_dir, "ota_"+ res_file +".xml")
        check_cp(FW_DIR, app_cfg_dir, "ota_"+ res_file +".xml")

    #full
    for full_file in full_files:
        res_cfgfile = os.path.join(FW_DIR, "ota_"+ full_file +".xml")
        check_cp(FW_DIR, app_dir, "ota_"+ full_file +".xml")
        check_cp(FW_DIR, app_cfg_dir, "ota_"+ full_file +".xml")

    # check override files
    check_cp(FW_DIR_BIN, board_dir, "adfus_u.bin")
    check_cp(FW_DIR_BIN, board_dir, "adfus.bin")
    check_cp(FW_DIR_BIN, board_dir, "mbrec.bin")
    check_cp(FW_DIR_BIN, board_dir, "afi.bin")
    check_cp(FW_DIR_BIN, board_dir, "bootloader.ini")
    check_cp(FW_DIR_BIN, board_dir, "nand_id.bin")
    check_cp(FW_DIR_BIN, board_dir, "recovery.bin")
    check_cp(FW_DIR_BIN, board_dir, "fw_product.cfg")
    check_cp(FW_DIR_BIN, app_dir, "prebuild")
    check_cp(FW_DIR_BIN, app_cfg_dir, "prebuild")
    check_cp(FW_DIR_BIN, app_cfg_dir, "E_CHECK.FW")
    check_cp(FW_DIR_BIN, app_cfg_dir, "fwimage.cfg")

    ota_app_path = os.path.join(FW_DIR_BIN, "ota_app.bin")
    if os.path.exists(ota_app_path) == True:
         os.remove(ota_app_path)
    check_cp(FW_DIR_BIN, board_dir, "ota_app.bin")

    # signed img
    sign_dir =os.path.join(board_dir, "sign-img")
    if os.path.exists(sign_dir) == True:
        check_cp(FW_DIR_BIN, sign_dir, "mbrec.bin")


    print("\nFW: Post build mbrec.bin\n")
    script_firmware_path = os.path.join(sdk_script_dir, 'build_boot_image.py')
    cmd = ['python', '-B', script_firmware_path, os.path.join(FW_DIR_BIN, "mbrec.bin"), \
            os.path.join(FW_DIR_BIN, "bootloader.ini"), \
            os.path.join(FW_DIR_BIN, "nand_id.bin")]
    print("\n build cmd : %s\n\n" %(cmd))
    (outmsg, exit_code) = run_cmd(cmd)
    if exit_code !=0:
        print('make build_boot_image error')
        print(outmsg.decode('utf-8'))
        sys.exit(1)


    build_datafs_bin(FW_DIR_BIN, app_dir, app_cfg_dir, fw_cfgfile)
    build_littlefs_bin(FW_DIR_BIN, app_dir, app_cfg_dir, fw_cfgfile)
    
    #copy zephyr.bin
    app_bin_src_path = os.path.join(OS_OUTDIR, "zephyr.bin")
    if os.path.exists(app_bin_src_path) == False:
        print("\n app.bin not eixt=%s\n\n"  %(app_bin_src_path))
        return
    #app_bin_dst_path=os.path.join(FW_DIR_BIN, "zephyr.bin")
    app_bin_dst_path=os.path.join(FW_DIR_BIN, "app.bin")
    shutil.copyfile(app_bin_src_path, app_bin_dst_path)

    # write checksum

    for res_file in res_files:
        script_firmware_path = os.path.join(sdk_script_dir, 'pack_res_chksum.py')
        cmd = ['python', '-B', script_firmware_path, '-b', app_bin_dst_path, '-r', res_file + ".bin"]
        print("\n build cmd : %s\n\n" %(cmd))
        (outmsg, exit_code) = run_cmd(cmd)
        if exit_code !=0:
            print('pack app error')
            print(outmsg.decode('utf-8'))
            sys.exit(1)

    image_add_checksum(FW_DIR_BIN, "recovery.bin");
    image_add_checksum(FW_DIR_BIN, "app.bin");
    image_add_checksum(FW_DIR_BIN, "ota_app.bin");

    # signed img
    if os.path.exists(sign_dir) == True:
        check_cp(FW_DIR_BIN, sign_dir, "recovery.bin")
        check_cp(FW_DIR_BIN, sign_dir, "app.bin")
        check_cp(FW_DIR_BIN, sign_dir, "ota_app.bin")

    print("\n--board %s ==--\n\n"  %( board))
    script_firmware_path = os.path.join(sdk_script_dir, 'build_firmware.py')
    cmd = ['python', '-B', script_firmware_path,  '-c', fw_cfgfile, \
            '-e', os.path.join(FW_DIR_BIN, "encrypt.bin"),\
            '-ef', os.path.join(FW_DIR_BIN, "E_CHECK.FW"),\
            '-s', "lark",\
            '-b', board]
    print("\n build cmd : %s\n\n" %(cmd))
    (outmsg, exit_code) = run_cmd(cmd)
    if exit_code !=0:
        print('make build_firmware error\n')
        print(outmsg.decode('utf-8'))
        sys.exit(1)

    app_bin_dst_path=os.path.join(FW_DIR_BIN, "app.bin")
    shutil.copyfile(app_bin_src_path, app_bin_dst_path)
    image_add_checksum(FW_DIR_BIN, "app.bin");

     # signed img
    if os.path.exists(sign_dir) == True:
        check_cp(FW_DIR_BIN, sign_dir, "app.bin")

    fw_cfgfile = os.path.join(FW_DIR, "firmware_debug.xml")
    if os.path.exists(fw_cfgfile):
        cmd = ['python', '-B', script_firmware_path,  '-c', fw_cfgfile, \
                '-e', os.path.join(FW_DIR_BIN, "encrypt.bin"),\
                '-ef', os.path.join(FW_DIR_BIN, "E_CHECK.FW"),\
                '-s', "lark",\
                '-b', board,\
			    '-x', "debug"]
        print("\n build cmd : %s\n\n" %(cmd))
        (outmsg, exit_code) = run_cmd(cmd)
        if exit_code !=0:
            print('make build_firmware_debug error\n')
            print(outmsg.decode('utf-8'))
            sys.exit(1)

    for res_file in res_files:
        ota_xml_path = os.path.join(FW_DIR,"ota_"+ res_file +".xml")
        if os.path.exists(ota_xml_path) == True:
             os.remove(ota_xml_path)

    for full_file in full_files:
        ota_xml_path = os.path.join(FW_DIR,"ota_"+ full_file +".xml")
        if os.path.exists(ota_xml_path) == True:
             os.remove(ota_xml_path)


def build_zephyr_app_by_keil(board, out_dir, app_dir, app_cfg_p, libname, silent):
    if (is_windows()):
        os.chdir(sdk_root)
        build_args = "ninja2mdk.cmd " + board
        build_args += "  " + os.path.relpath(app_dir)
        build_args += "  " + app_cfg_p
        if libname is not None:
            build_args += "  " + libname
        print("\n bulid cmd:%s \n\n" %(build_args))
        ret = os.system(build_args)
        if ret != 0:
            print("\n bulid error\n")
            sys.exit(1)
        print("\n Warning: please build using Keil-MDK! \n\n")
        if silent == False:
            mdk_dir = os.path.join(out_dir, "mdk_clang")
            os.startfile(os.path.join(mdk_dir, "mdk_clang.uvprojx"))
        sys.exit(0)
    else:
        print("\n Error: please build on windows! \n\n")
        sys.exit(1)

def build_zephyr_app_by_gcc(board, out_dir, app_dir, app_cfg_p, libname, silent):
    build_args = "west build -p auto -b " + board
    build_args += " -d " + out_dir + "  " + app_dir
    if app_cfg_p != ".":
        cfg_file = os.path.join(app_cfg_p, "prj.conf")
        if os.path.exists(os.path.join(app_dir,cfg_file)) == True:
            if (is_windows()):
                cfg_file = cfg_file.replace('\\', '/')
            build_args += " -- " + " -DCONF_FILE=" + cfg_file

    os.chdir(sdk_root)
    print("\n bulid cmd:%s \n\n" %(build_args))
    ret = os.system(build_args)
    if ret != 0:
        print("\n bulid error\n")
        sys.exit(1)


def build_zephyr_menuconfig(out_dir):
    build_args = "west build -d " + out_dir
    build_args += " -t menuconfig"
    print("\n bulid cmd:%s \n\n" %(build_args))
    ret = os.system(build_args)
    if ret != 0:
        print("\n bulid error\n")
        sys.exit(1)

def main(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument('-n', dest="cfgdel", help="remove cur build_config", action="store_true", required=False)
    parser.add_argument('-c', dest="clear", help="remove board out dir and build_config", action="store_true", required=False)
    parser.add_argument('-m', dest='menuconfig', help="do sdk Configuration", action="store_true", default=False)
    parser.add_argument('-p', dest='pack', help="pack firmware only", action="store_true", default=False)
    parser.add_argument('-l', dest="libname", help="build lib", required=False)
    parser.add_argument('-s', dest="silent", help="silent mode", action="store_true", required=False)
    parser.add_argument('-t', dest="sample", help="build sample, default build applicaction", action="store_true", required=False)
    parser.add_argument('-u', dest="test", help="build test, default build applicaction", action="store_true", required=False)
    parser.add_argument('-k', dest="bkeil", help="build by keil, default by gcc ", action="store_true", required=False)

    print("build")
    args = parser.parse_args()

    if args.sample == True:
        sdk_application_dir = sdk_samples_dir
    elif args.test == True:
        sdk_application_dir = sdk_tests_dir
    else:
        sdk_application_dir = sdk_app_dir

    application = ""
    sub_app = ""
    board_conf = ""
    app_conf  = ""

    if args.cfgdel == True:
        if os.path.exists(build_config_file) == True:
            os.remove(build_config_file)
            #sys.exit(0)

    if os.path.exists(build_config_file) == True:
        fp = open(build_config_file,"r")
        lines = fp.readlines()
        for elem in lines:
            if elem[0] != '#':
                _c = elem.strip().split("=")
                if _c[0] == "APPLICATION":
                    application = _c[1]
                elif _c[0] == "BOARD":
                    board_conf  = _c[1]
                elif _c[0] == "SUB_APP":
                    sub_app = _c[1]
                elif _c[0] == "APP_CONF":
                    app_conf = _c[1]
        fp.close


    if os.path.exists(build_config_file) == False:
        board_conf = read_choice("Select boards configuration:", sdk_boards_dir)
        application = read_choice("Select application:", sdk_application_dir)
        sub_app = ""
        app_cfg_dir=os.path.join(sdk_application_dir, application)
        if os.path.exists(os.path.join(app_cfg_dir, "CMakeLists.txt")) == False:
            sub_app = read_choice("Select "+ application+" sub app:", os.path.join(sdk_application_dir, application))
            app_cfg_dir=os.path.join(app_cfg_dir, sub_app)
        if os.path.exists(os.path.join(app_cfg_dir, "CMakeLists.txt")) == False:
            print("application  %s not have CMakeLists.txt\n\n" %(app_cfg_dir))
            sys.exit(0)
        app_conf = ""
        if os.path.exists(os.path.join(app_cfg_dir, "app_conf")) == True:
            app_conf = read_choice("Select application conf:", os.path.join(app_cfg_dir, "app_conf"))
            app_cfg_dir = os.path.join(app_cfg_dir, "app_conf", app_conf);

        fp = open(build_config_file,"w")
        fp.write("# Build config\n")
        fp.write("BOARD=" + board_conf + "\n")
        fp.write("APPLICATION=" + application + "\n")
        fp.write("SUB_APP=" + sub_app + "\n")
        fp.write("APP_CONF=" + app_conf + "\n")
        fp.close()


    print("\n--== Build application %s (sub %s) (app_conf %s)  board %s ==--\n\n"  %(application, sub_app, app_conf,
        board_conf))


    if os.path.exists(os.path.join(sdk_application_dir, application, "CMakeLists.txt")) == True:
        application_path = os.path.join(sdk_application_dir, application)
    else:
        application_path = os.path.join(sdk_application_dir, application, sub_app)

    if os.path.exists(application_path) == False:
        print("\nNo application at %s \n\n" %(sdk_application_dir))
        sys.exit(1)

    if os.path.exists(os.path.join(application_path,  "app_conf")) == True:
        application_cfg_reldir=os.path.join("app_conf", app_conf)
    elif os.path.exists(os.path.join(application_path,  "boards")) == True:
        application_cfg_reldir =os.path.join("boards", board_conf)
    else :
        application_cfg_reldir = "."

    print(" application cfg dir %s, \n\n" %(application_cfg_reldir))

    sdk_out = os.path.join(application_path, "outdir")
    sdk_build = os.path.join(sdk_out, board_conf)
    if args.clear == True:
        print("\n Clean build out=%s \n\n"  %(sdk_out))
        if os.path.exists(sdk_out) == True:
            shutil.rmtree(sdk_out, True)
            time.sleep(0.01)
        sys.exit(0)

    if args.menuconfig == True:
        if os.path.exists(sdk_build) == True:
            build_zephyr_menuconfig(sdk_build)
        else:
            print("please build")
        sys.exit(1)

    print("\n sdk_build=%s \n\n"  %(sdk_build))
    if os.path.exists(sdk_build) == False:
        os.makedirs(sdk_build)


    board_conf_path = os.path.join(sdk_boards_dir, board_conf)
    if os.path.exists(board_conf_path) == False:
        print("\nNo board at %s \n\n" %(board_conf_dir))
        sys.exit(1)

    #build_zephyr_app(board_conf, sdk_build_boot, sdk_mcuboot_app_path)
    if args.pack == False:
        if args.bkeil == True:
            build_zephyr_app_by_keil(board_conf, sdk_build, application_path, application_cfg_reldir, args.libname, args.silent)
        else:
            build_zephyr_app_by_gcc(board_conf, sdk_build, application_path, application_cfg_reldir, args.libname, args.silent)

    build_firmware(board_conf, sdk_build, board_conf_path, application_path, os.path.join(application_path, application_cfg_reldir))
    print("build finished")


if __name__ == "__main__":
    main(sys.argv)
