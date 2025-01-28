#!/usr/bin/env python3

# Copyright (c) 2019, Nordic Semiconductor
# SPDX-License-Identifier: BSD-3-Clause

import os
import shutil
import sys
import argparse
import dtlib
import xml.etree.ElementTree as ET

# Test suite for dtlib.py. Run it directly as an executable, in this directory:
#
#   $ ./testdtlib.py

# TODO: Factor out common code from error tests



def find_partions_node(dt):
    for node in dt.node_iter():
        #print("1.node: %s \n" %(node.name))
        if ("partitions" == node.name):
            return node
    return None


def get_type_by_label(label):
    if label == "mbrec-0" or label == "mbrec-1":
        return "BOOT"
    if label == "param-0" or label == "param-1":
        return "SYS_PARAM"
    if label == "mcuboot":
        return "RECOVERY"
    if label == "image-0":
        return "SYSTEM"
    if label == "image-1":
        return "SYSTEM"
    return "DATA"

def get_name_by_label(label):
    if label == "mbrec-0":
        return "fw0_boot"
    if label == "mbrec-1":
        return "fw1_boot"
    if label == "param-0":
        return "fw0_param"
    if label == "param-1":
        return "fw1_param"
    if label == "mcuboot":
        return "fw0_rec"
    if label == "image-0":
        return "fw0_sys"
    if label == "image-1":
        return "fw1_sys"
    if label == "sdfs-0":
        return "fw0_sdfs"
    if label == "sdfs-1":
        return "fw1_sdfs"

    return label

def get_filename_by_label(label):
    if label == "mbrec-0":
        return "mbrec.bin"
    if label == "param-0":
        return "param.bin"
    if label == "mcuboot":
        return "mcuboot.bin"
    if label == "image-0":
        return "app.bin"
    if label == "sdfs-0":
        return "sdfs.bin"
    if label == "nvram_factory":
        return "nvram.bin"        
    return None

def is_need_mirror_field(label):
    if label == "mbrec-0" or label == "mbrec-1":
        return True
    if label == "param-0" or label == "param-1":
        return True
    if label == "image-0" or label == "image-1":
        return True
    if label == "sdfs-0" or label == "sdfs-1":
        return True
    
    return False

def main(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument("--dts", required=True, help="DTS file")
    parser.add_argument("--xml", required=True, help="out XML file")
    args = parser.parse_args()
    dt = dtlib.DT(args.dts)
    node = find_partions_node(dt)
    if (node == None):
        print("error:not find partitions node\n")
        return
    parent = node.parent
    pparent = parent.parent
    flash_size=int.from_bytes(parent._get_prop('reg').value[4:], "big", signed=False)
    flash_addr=int.from_bytes(parent._get_prop('reg').value[:4], "big", signed=False)    
    name=pparent._get_prop('label').to_string()
    print("parent lable=%s flash_size 0x%x\n"%(name,  flash_size))

    if os.path.exists(args.xml):
        os.remove(args.xml)

    root = ET.Element('firmware')
    root.text = '\n\t'
    root.tail = '\n'

    fw_des =  ET.SubElement(root, 'descirption')
    fw_des.text = 'Firmware layout for LARK'
    fw_des.tail = '\n\t'

    fw_disk =  ET.SubElement(root, 'disk_size')
    fw_disk.text = ("0x%x"%(flash_size))
    fw_disk.tail = '\n\n\t'

    fw_ver =  ET.SubElement(root, 'firmware_version')
    fw_ver.text = '\n\t\t'
    fw_vcode =  ET.SubElement(fw_ver, 'version_code')
    fw_vcode.text= '0x10000'
    fw_vcode.tail = '\n\t\t'
    fw_vname =  ET.SubElement(fw_ver, 'version_name')
    fw_vname.text= "1.00_$(build_time)"
    fw_vname.tail = '\n\t'
    fw_ver.tail = '\n\n\t'


    fw_paris =  ET.SubElement(root, 'partitions')
    fw_paris.text = '\n\n\t\t'

    file_id = 1
    for ch_node in node.node_iter():
        if node !=  ch_node :
            print("ch node: %s \n" %(ch_node.name))
            addr=int.from_bytes(ch_node._get_prop('reg').value[:4], "big", signed=False)
            size=int.from_bytes(ch_node._get_prop('reg').value[4:], "big", signed=False)
            name=ch_node._get_prop('label').to_string()
            #print("ch node lable=%s reg=0x%x, 0x%x\n"%(name, addr, size))
            part_node = ET.SubElement(fw_paris, 'partition')
            part_node.text = '\n\t\t\t'
            part_node.tail = '\n\n\t\t'

            part_addr = ET.SubElement(part_node, 'address')
            part_addr.text=("0x%x"%(addr))
            part_addr.tail = '\n\t\t\t'

            part_size = ET.SubElement(part_node, 'size')
            part_size.text=("0x%x"%(size))
            part_size.tail = '\n\t\t\t'

            part_type = ET.SubElement(part_node, 'type')
            part_type.text= get_type_by_label(name)
            part_type.tail = '\n\t\t\t'

            part_name = ET.SubElement(part_node, 'name')
            part_name.text= get_name_by_label(name)
            part_name.tail = '\n\t\t\t'

            part_fid = ET.SubElement(part_node, 'file_id')
            part_fid.text=("%d"%(file_id))
            part_fid.tail = '\n\t\t\t'

            fname = get_filename_by_label(name)
            if fname != None :
                part_fname = ET.SubElement(part_node, 'file_name')
                part_fname.text = fname
                part_fname.tail = '\n\t\t\t'

            part_faddr = ET.SubElement(part_node, 'file_address')
            part_faddr.text=("0x%x"%(addr))
            part_faddr.tail = '\n\t\t\t'


            part_crc = ET.SubElement(part_node, 'enable_crc')
            part_crc.text="false"
            part_crc.tail = '\n\t\t\t'

            part_encryption = ET.SubElement(part_node, 'enable_encryption')
            part_encryption.text="false"
            part_encryption.tail = '\n\t\t\t'

            part_ota = ET.SubElement(part_node, 'enable_ota')
            if fname != None :
                part_ota.text="true"
            else :
                part_ota.text="false"
            part_ota.tail = '\n\t\t\t'

            part_raw = ET.SubElement(part_node, 'enable_raw')
            if fname != None :
                part_raw.text="true"
            else :
                part_raw.text="false"
            part_raw.tail = '\n\t\t\t'

            part_dfu = ET.SubElement(part_node, 'enable_dfu')
            if fname != None :
                part_dfu.text="true"
            else :
                part_dfu.text="false"
            part_dfu.tail = '\n\t\t\t'

            if is_need_mirror_field(name):
                part_mid = ET.SubElement(part_node, 'mirror_id')
                if fname != None :
                    part_mid.text="0"
                else :
                    part_mid.text="1"
                part_mid.tail = '\n\t\t'

            file_id = file_id + 1

    fw_paris.tail = '\n\n'
    tree = ET.ElementTree(root)
    tree.write(args.xml, xml_declaration=True, method="xml", encoding='UTF-8')

if __name__ == "__main__":
    main(sys.argv)

