#!/usr/bin/env python
#
# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0
#

import os
import sys
import time
import argparse
import platform
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
sdk_app_dir = os.path.join(sdk_root_parent, "bootloader", "application")
sdk_samples_dir = os.path.join(sdk_root, "samples")
sdk_script_dir = os.path.join(sdk_root, "tools")
sdk_script_prebuilt_dir = os.path.join(sdk_script_dir, "prebuilt")
build_config_file = os.path.join(sdk_root, ".build_config")

sdk_parent_boards_dir = os.path.join(sdk_root_parent, "zephyr","boards", sdk_arch)

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

def build_nvram_bin(out_dir, board_dir, app_confg_dir):
    nvram_path = os.path.join(out_dir, "nvram.bin")
    app_cfg_path =  os.path.join(app_confg_dir, "nvram.prop")
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
        print(outmsg)
        sys.exit(1)
    print("\n build_nvram  finshed\n\n")

def dir_cp(out_dir, in_dir):
    print("\n cp dir %s  to dir %s \n"  %(in_dir, out_dir))
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
    print("\n cp dir %s  to dir %s \n"  %(in_dir, out_dir))
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

def build_sdfs_bin_name(out_dir, board_dir, app_confg_dir, sdfs_name):
    board_sdfs_dir = os.path.join(board_dir, sdfs_name)
    app_sdfs_dir = os.path.join(app_confg_dir, sdfs_name)
    #print("\n build_sdfs : sdfs_dir %s, board dfs %s\n\n" %(sdfs_dir, board_sdfs_dir))
    if (os.path.exists(board_sdfs_dir) == False) and (os.path.exists(app_sdfs_dir) == False):
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

    script_sdfs_path = os.path.join(sdk_script_dir, 'build_sdfs.py')
    sdfs_bin_path = os.path.join(out_dir, sdfs_name+".bin")
    cmd = ['python', '-B', script_sdfs_path,  '-o', sdfs_bin_path, '-d', sdfs_dir]
    (outmsg, exit_code) = run_cmd(cmd)
    if exit_code !=0:
        print('make build_sdfs_bin error')
        print(outmsg)
        sys.exit(1)
    print("\n build_sdfs %s finshed\n\n" %(sdfs_name))

def build_sdfs_bin_bydir(out_dir, board_dir, app_confg_dir):
    bpath = os.path.join(board_dir, "fs_sdfs");
    apath = os.path.join(app_confg_dir, "fs_sdfs");
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
        build_sdfs_bin_name(out_dir, bpath, apath, dir)


def build_sdfs_bin(out_dir, board_dir, app_confg_dir):
    build_sdfs_bin_name(out_dir, board_dir, app_confg_dir,"ksdfs")
    build_sdfs_bin_name(out_dir, board_dir, app_confg_dir,"sdfs")
    build_sdfs_bin_name(out_dir, board_dir, app_confg_dir,"fatfs")
    build_sdfs_bin_bydir(out_dir, board_dir, app_confg_dir)

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

def build_ksdfs_bin(out_dir, board_dir, app_confg_dir, os_out_dir):
    sdfs_dir = os.path.join(out_dir, "ksdfs")
    if os.path.exists(sdfs_dir) == True:
        shutil.rmtree(sdfs_dir)
        time.sleep(0.01)
    board_sdfs_dir = os.path.join(board_dir, "ksdfs")
    print("\n build_ksdfs : ksdfs_dir %s, board dfs %s\n\n" %(sdfs_dir, board_sdfs_dir))
    if os.path.exists(board_sdfs_dir) == False:
        print("\n build_ksdfs : board dfs %s not exists\n\n" %(board_sdfs_dir))
        return
    dir_cp(sdfs_dir, board_sdfs_dir)
    if is_config_KALLSYMS(os.path.join(os_out_dir, ".config")) == True:
        print("cpy ksym.bin");
        shutil.copyfile(os.path.join(os_out_dir, "ksym.bin"), os.path.join(sdfs_dir, "ksym.bin"))
    app_sdfs_dir = os.path.join(app_confg_dir, "ksdfs")
    if os.path.exists(app_sdfs_dir) == True:
        dir_cp(sdfs_dir, app_sdfs_dir)

    script_sdfs_path = os.path.join(sdk_script_dir, 'build_sdfs.py')
    sdfs_bin_path = os.path.join(out_dir, "ksdfs.bin")
    cmd = ['python', '-B', script_sdfs_path,  '-o', sdfs_bin_path, '-d', sdfs_dir]
    (outmsg, exit_code) = run_cmd(cmd)
    if exit_code !=0:
        print('make build_ksdfs_bin error')
        print(outmsg)
        sys.exit(1)
    print("\n build_ksdfs finshed---\n\n")
    file_link(os.path.join(out_dir, "app.bin"), sdfs_bin_path)
    #file_link(os.path.join(out_dir, "recovery.bin"), sdfs_bin_path)

def build_datafs_bin(out_dir, app_confg_dir, fw_cfgfile):
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
        if ('storage_id' in part_prop.keys()) and (0 != int(part_prop['storage_id'], 0)):
            app_datafs_dir = os.path.join(app_confg_dir, part_prop['name'])
            if os.path.exists(app_datafs_dir) == False:
                print("\n build_datafs : app config datafs %s not exists\n\n" %(app_datafs_dir))
                continue
            datafs_cap = 0
            for parent, dirs, files in os.walk(app_datafs_dir):
                for file in files:
                    datafs_cap += os.path.getsize(os.path.join(parent, file))

            if datafs_cap == 0:
                print("\n build_datafs: no file in %s\n\n" %(app_datafs_dir))
                continue

            # reserved 0x80000 space for FAT filesystem region such as boot sector, FAT region, root directory region.
            datafs_cap += 0x80000

            datafs_cap = int((datafs_cap + 511) / 512) * 512

            if datafs_cap > int(part_prop['size'], 0):
                print("\n build_datafs: too large file:%d and max:%d\n\n" %(datafs_cap, int(part_prop['size'], 0)))
                continue

            datafs_bin_path = os.path.join(out_dir, part_prop['file_name'])
            print('DATAFS (%s) capacity => 0x%x' %(datafs_bin_path, datafs_cap))
            cmd = ['python', '-B', script_datafs_path,  '-o', datafs_bin_path, '-s', str(datafs_cap), '-d', app_datafs_dir]
            (outmsg, exit_code) = run_cmd(cmd)
            if exit_code !=0:
                print('make build_datafs_bin %s error' %(datafs_bin_path))
                print(outmsg)
                sys.exit(1)

    print("\n build_datafs finshed\n\n")


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

def build_firmware(board, out_dir, board_dir, app_dir):
    print("\n build_firmware : out %s, board %s, app_dir %s\n\n" %(out_dir, board_dir, app_dir))
    FW_DIR = os.path.join(out_dir, "_firmware")
    OS_OUTDIR = os.path.join(out_dir, "zephyr")
    if os.path.exists(FW_DIR) == False:
        os.mkdir(FW_DIR)
    FW_DIR_BIN = os.path.join(FW_DIR, "bin")
    if os.path.exists(FW_DIR_BIN) == False:
        os.mkdir(FW_DIR_BIN)

    ksdfs_dir = os.path.join(FW_DIR_BIN, "ksdfs")
    if is_config_KALLSYMS(os.path.join(OS_OUTDIR, ".config")) and os.path.isdir(ksdfs_dir):
        print("cpy ksym.bin");
        shutil.copyfile(os.path.join(OS_OUTDIR, "ksym.bin"), os.path.join(ksdfs_dir, "ksym.bin"))

    build_nvram_bin(FW_DIR_BIN, board_dir, app_dir)
    build_sdfs_bin(FW_DIR_BIN, board_dir, app_dir)
    soc = read_soc_by_config(os.path.join(OS_OUTDIR, ".config"))
    print("\n build_firmware : soc name= %s\n" %(soc))
    dir_cp(FW_DIR_BIN, os.path.join(sdk_script_prebuilt_dir, soc, "common", "bin"))
    dir_cp(FW_DIR_BIN, os.path.join(sdk_script_prebuilt_dir, soc, "bin"))

    fw_cfgfile = os.path.join(FW_DIR, "firmware.xml")

    #copy firmware.xml
    if os.path.exists(os.path.join(board_dir, "firmware.xml")):
        print("FW: Override the default firmware.xml")
        shutil.copyfile(os.path.join(board_dir, "firmware.xml"), fw_cfgfile)

    if os.path.exists(os.path.join(app_dir, "firmware.xml")):
        print("FW: Override the project firmware.xml")
        shutil.copyfile(os.path.join(app_dir, "firmware.xml"), fw_cfgfile)

    if not os.path.exists(fw_cfgfile):
        print("failed to find out firmware.xml")
        sys.exit(1)

    # check override mbrec.bin
    if os.path.exists(os.path.join(board_dir, "mbrec.bin")):
        print("FW: Override the board mbrec.bin")
        shutil.copyfile(os.path.join(board_dir, "mbrec.bin"), os.path.join(FW_DIR_BIN, "mbrec.bin"))

    # check override afi.bin
    if os.path.exists(os.path.join(board_dir, "afi.bin")):
        print("FW: Override the board afi.bin")
        shutil.copyfile(os.path.join(board_dir, "afi.bin"), os.path.join(FW_DIR_BIN, "afi.bin"))

    # check override bootloader.ini
    if os.path.exists(os.path.join(board_dir, "bootloader.ini")):
        print("FW: Override the board bootloader.ini")
        shutil.copyfile(os.path.join(board_dir, "bootloader.ini"), os.path.join(FW_DIR_BIN, "bootloader.ini"))

    if os.path.exists(os.path.join(board_dir, "recovery.bin")):
        print("FW: Copy recovery.bin")
        shutil.copyfile(os.path.join(board_dir, "recovery.bin"), os.path.join(FW_DIR_BIN, "recovery.bin"))

    prebuild_dir = os.path.join(app_dir, "prebuild")
    if os.path.exists(prebuild_dir):
        print("FW: Copy prebuild files")
        dir_tree_cp(FW_DIR_BIN, prebuild_dir)
        # shutil.copytree(prebuild_dir, self.bin_dir, dirs_exist_ok = True)
        # cmd = ['xcopy', prebuild_dir, FW_DIR_BIN, '/F', '/S', '/y']
        # run_cmd(cmd)

    print("\nFW: Post build mbrec.bin\n")
    script_firmware_path = os.path.join(sdk_script_dir, 'build_boot_image.py')
    cmd = ['python', '-B', script_firmware_path, os.path.join(FW_DIR_BIN, "mbrec.bin"), \
            os.path.join(FW_DIR_BIN, "bootloader.ini"), \
            os.path.join(FW_DIR_BIN, "nand_id.bin")]
    print("\n build cmd : %s\n\n" %(cmd))
    (outmsg, exit_code) = run_cmd(cmd)
    if exit_code !=0:
        print('make build_boot_image error')
        print(outmsg)
        sys.exit(1)


    build_datafs_bin(FW_DIR_BIN, app_dir, fw_cfgfile)

    #copy zephyr.bin
    app_bin_src_path = os.path.join(OS_OUTDIR, "zephyr.bin")
    if os.path.exists(app_bin_src_path) == False:
        print("\n app.bin not eixt=%s\n\n"  %(app_bin_src_path))
        return
    #app_bin_dst_path=os.path.join(FW_DIR_BIN, "zephyr.bin")
    app_bin_dst_path=os.path.join(FW_DIR_BIN, "app.bin")
    shutil.copyfile(app_bin_src_path, app_bin_dst_path)

    # build_ksdfs_bin(FW_DIR_BIN, board_dir, app_dir, OS_OUTDIR)
    # signed img
    """
    app_bin_sign_path=os.path.join(FW_DIR_BIN, "app.bin")
    cmd = ['python', '-B', sdk_mcuboot_imgtool,  'sign', \
            '--key', sdk_mcuboot_key,\
            '--header-size', "0x200",\
            '--align', "8", \
            '--version', "1.2",\
            '--slot-size', "0x60000",\
            app_bin_dst_path,\
            app_bin_sign_path]
    print("\n signed cmd : %s\n\n" %(cmd))
    (outmsg, exit_code) = run_cmd(cmd)
    if exit_code !=0:
        print('signed img error')
        print(outmsg)
        sys.exit(1)

    #copy mcuboot.bin
    boot_bin_src_path = os.path.join(out_dir, "boot", "zephyr", "zephyr.bin")
    if os.path.exists(boot_bin_src_path) == False:
        print("\n mcuboot not eixt=%s\n\n"  %(boot_bin_src_path))
        return
    shutil.copyfile(boot_bin_src_path, os.path.join(FW_DIR_BIN, "mcuboot.bin"))
    """
    print("\n--board %s ==--\n\n"  %( board))
    script_firmware_path = os.path.join(sdk_script_dir, 'build_firmware.py')
    cmd = ['python', '-B', script_firmware_path,  '-c', fw_cfgfile, \
            '-e', os.path.join(FW_DIR_BIN, "encrypt.bin"),\
            '-ef', os.path.join(FW_DIR_BIN, "efuse.bin"),\
            '-s', "lark",\
            '-b', board]
    print("\n build cmd : %s\n\n" %(cmd))
    (outmsg, exit_code) = run_cmd(cmd)
    if exit_code !=0:
        print('make build_firmware error')
        print(outmsg)
        sys.exit(1)

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
    parser.add_argument('-k', dest="bkeil", help="build by keil, default by gcc ", action="store_true", required=False)

    print("build")
    args = parser.parse_args()

    if args.sample == True:
        sdk_application_dir = sdk_samples_dir
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
        app_conf = ""
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

    #build_firmware(board_conf, sdk_build, board_conf_path, os.path.join(application_path, application_cfg_reldir))
    print("build finished %s \n\n" %(board_conf_path))
    shutil.copyfile(os.path.join(sdk_build, "zephyr" ,"zephyr.bin"), os.path.join(sdk_parent_boards_dir, board_conf, application+".bin"))
    print("copy finished")

if __name__ == "__main__":
    main(sys.argv)
