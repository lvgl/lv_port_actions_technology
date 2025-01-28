#!/usr/bin/env python3
#
# Build Actions NVRAM config binary file
#
# Copyright (c) 2017 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0
#

import os
import sys
import struct
import array
import argparse
import zlib
import configparser

from ctypes import *;

BINARY_BLOCK_SIZE_ALIGN = 512

class BOOT_PARAMETERS(Structure): # import from ctypes
    _pack_ = 1
    _fields_ = [
        ("magic",           c_uint32),
        ("uart_baudrate",   c_uint32),
        ("uart_id",         c_uint8),
        ("uart_mfp",        c_uint8),
        ("jtag_groud",      c_uint8),
        ("psram_mfp",       c_uint8),
        ("adfu_txrx",       c_uint8),
        ("adfu_gpio",       c_uint8),
        ("reserved",        c_uint8 * 14),
        ("checksum",        c_uint32),
    ] # 32 bytes
SIZEOF_BOOT_PARAMETERS_VERSION   = 0x20
BOOT_PARAMETERS_MAGIC = 0x52415042   #'BPAR'
BOOT_PARAMETERS_OFFSET = 0x30

def c_struct_crc(c_struct, length):
    crc_buf = (c_byte * length)()
    memmove(addressof(crc_buf), addressof(c_struct), length)
    return zlib.crc32(crc_buf, 0) & 0xffffffff

def generate_boot_parameters(boot_name, boot_ini_name, out_ini_bin):
    if os.path.exists(boot_ini_name):
        boot_param = BOOT_PARAMETERS()
        memset(addressof(boot_param), 0, SIZEOF_BOOT_PARAMETERS_VERSION)
        boot_param.magic = BOOT_PARAMETERS_MAGIC

        configer = configparser.ConfigParser()
        configer.read(boot_ini_name)

        uart_baudrate = configer.get("serial config", "uart_baudrate")
        if uart_baudrate != None:
            boot_param.uart_baudrate = int(uart_baudrate, 0)
            print('uart id %s' %(uart_baudrate))

        uart_id = configer.get("serial config", "uart_id")
        if uart_id != None:
            boot_param.uart_id = int(uart_id, 0)
            print('uart id %s' %(uart_id))

        uart_mfp = configer.get("serial config", "uart_mfp")
        if uart_mfp != None:
            boot_param.uart_mfp = int(uart_mfp, 0)
            print('uart id %s' %(uart_mfp))

        jtag_groud = configer.get("jtag config", "jtag_groud")
        if jtag_groud != None:
            boot_param.jtag_groud = int(jtag_groud, 0)
            print('jtag groud %s' %(jtag_groud))

        psram_mfp = configer.get("psram_config", "psram_mfp")
        if psram_mfp != None:
            boot_param.psram_mfp = int(psram_mfp, 0)
            print('psram mfp %s' %(psram_mfp))

        if configer.has_option("adfu config", "adfu_txrx") :
            adfu_txrx = configer.get("adfu config", "adfu_txrx")
            boot_param.adfu_txrx = int(adfu_txrx, 0)
            print('adfu_txrx %s' %(adfu_txrx))

        if configer.has_option("adfu config", "adfu_gpio") :
            adfu_gpio = configer.get("adfu config", "adfu_gpio")
            boot_param.adfu_gpio = int(adfu_gpio, 0)
            print('adfu_gpio %s' %(adfu_gpio))

        boot_param.checksum = c_struct_crc(boot_param, SIZEOF_BOOT_PARAMETERS_VERSION - 4)

        if out_ini_bin is None :
            with open(boot_name, 'rb+') as f:
                f.seek(BOOT_PARAMETERS_OFFSET, 0)
                f.write(boot_param)
                f.close()
        else :
            with open(out_ini_bin, 'wb+') as f:
                f.write(boot_param)
                f.close()


def boot_padding(filename, align = BINARY_BLOCK_SIZE_ALIGN):
    fsize = os.path.getsize(filename)
    if fsize % align:
        padding_size = align - (fsize % align)
        print('fsize %d, padding_size %d' %(fsize, padding_size))
        with open(filename, 'rb+') as f:
            f.seek(fsize, 0)
            buf = (c_byte * padding_size)();
            f.write(buf)
            f.close()

def boot_append_nand_id_table(filename, id_table_name):
    if not os.path.exists(id_table_name):
        return None

    fsize = os.path.getsize(filename)
    id_table_size = os.path.getsize(id_table_name)

    if id_table_size != 0:
        print('padding nand id table size %d' %(id_table_size))
        with open(id_table_name, 'rb') as f:
            read_buf = f.read()
            f.close()
        with open(filename, 'rb+') as f:
            f.seek(fsize, 0)
            f.write(read_buf)
            f.close()

def boot_calc_checksum(data):
        s = sum(array.array('H',data))
        s = s & 0xffff
        return s

def lark_boot_post_build(boot_name, boot_ini_name, nand_id_name):
    boot_len = os.path.getsize(boot_name)
    print('lark boot loader origin length %d.' %boot_len)

    generate_boot_parameters(boot_name, boot_ini_name, None)
    boot_append_nand_id_table(boot_name, nand_id_name)
    boot_padding(boot_name)

    boot_len_new = os.path.getsize(boot_name)
    print('boot loader new length %d.' %boot_len_new)

    with open(boot_name, 'rb+') as f:
        f.seek(0, 0)
        data = f.read(32)
        h_magic0,h_magic1,h_load_addr,h_name,h_version, \
        h_header_size,h_header_chksm,h_data_chksm,\
        h_body_size,h_tail_size \
        = struct.unpack('III4sHHHHII',data)

        if(h_magic0 != 0x48544341 or h_magic1 != 0x41435448):
            print('boot loader header check fail.')
            return

        h_body_size = boot_len_new - h_header_size
        f.seek(0x18, 0)
        f.write(struct.pack('<I', h_body_size))

        # use tail_size to record origin boot size
        h_tail_size = boot_len
        f.seek(0x1c, 0)
        f.write(struct.pack('<I', h_tail_size))

        f.seek(h_header_size, 0)
        data = f.read(h_body_size)
        checksum = boot_calc_checksum(data)
        checksum = 0xffff - checksum
        f.seek(0x16, 0)
        f.write(struct.pack('<H', checksum))

        f.seek(0x14, 0)
        f.write(struct.pack('<H', 0))

        f.seek(0x0, 0)
        data = f.read(h_header_size)
        checksum = boot_calc_checksum(data)
        checksum = 0xffff - checksum
        f.seek(0x14, 0)
        f.write(struct.pack('<H', checksum))
        f.close()
        print('boot loader add cksum pass.')



"""
******lark use tlv*******************
"""

"""
#define IMAGE_TLV_BOOTINI           0x1000   /* INI*/
#define IMAGE_TLV_NANDID            0x2000   /* nand id */
"""
IMAGE_TLV_BOOTINI=0x1000
IMAGE_TLV_NANDID=0x2000

IMAGE_TLV_INFO_MAGIC=0x5935
IMAGE_TLV_PROT_INFO_MAGIC=0x593a

def img_header_check(img_bin_f):
    """
    check img header, and add h_img_size
    """
    img_len = os.path.getsize(img_bin_f)
    print('img  origin length %d' %(img_len))
    if img_len % 4:
         boot_padding(img_bin_f, 4)
         img_len = os.path.getsize(img_bin_f)
         print('img  align 4 length %d' %(img_len))

    with open(img_bin_f, 'rb+') as f:
        f.seek(0, 0)
        data = f.read(48)
        h_magic0,h_magic1,h_load_addr,h_name,h_run_addr,h_img_size,h_img_chksum, h_hdr_chksum, \
        h_header_size,h_ptlv_size,h_tlv_size,h_version,h_flags \
        = struct.unpack('III8sIIIIHHHHI',data)
        if(h_magic0 != 0x48544341 or h_magic1 != 0x41435448):
            print('magic header check fail.')
            sys.exit(1)
        print('img h_header_size %d.' %h_header_size)
        h_img_size = img_len - h_header_size
        f.seek(0x18, 0)
        f.write(struct.pack('<I', h_img_size))
        print('img h_img_size %d.' %h_img_size)

        f.seek(0x26, 0)
        f.write(struct.pack('<h', 0))
        f.close()


def Add_data_to_tlv(img_bin_f, tlv_data_bin_f, tlv_type):
    """
    add data tlv
    """
    if not os.path.exists(tlv_data_bin_f):
        return 0
    img_len = os.path.getsize(img_bin_f)
    tlv_len = os.path.getsize(tlv_data_bin_f)
    print('tlv data length %d, img_len=%d' %(tlv_len,img_len))

    with open(tlv_data_bin_f, 'rb') as f:
        tlv_data = f.read()
        f.close()

    with open(img_bin_f, 'rb+') as f:
        f.seek(0, 0)
        data = f.read(48)
        h_magic0,h_magic1,h_load_addr,h_name,h_run_addr,h_img_size,h_img_chksum, h_hdr_chksum, \
        h_header_size,h_ptlv_size,h_tlv_size,h_version,h_flags \
        = struct.unpack('III8sIIIIHHHHI',data)
        if(h_magic0 != 0x48544341 or h_magic1 != 0x41435448):
            print('magic header check fail.')
            sys.exit(1)
        plv_start = h_header_size + h_img_size + h_ptlv_size
        print('plv_start %d,header_size=%d, img_len=%d  ptlv_size=%d' %(plv_start, h_header_size, h_img_size, h_ptlv_size))

        f.seek(plv_start, 0)
        tlv_all_size = tlv_len + 4
        tlv_tol_size = 0
        if(plv_start == img_len):#not tlv header
            print('add tlv magic')
            tlv_tol_size = tlv_all_size
            f.write(struct.pack('<h', IMAGE_TLV_INFO_MAGIC))
            f.write(struct.pack('<h', tlv_tol_size))
        else :
            data1 = f.read(4)
            tlv_magic, tlv_tol_size= struct.unpack('HH',data1)
            if(tlv_magic != IMAGE_TLV_INFO_MAGIC):
                print('tlv magic  check fail.')
                sys.exit(1)
            print('old tlv_tol_size %d' %(tlv_tol_size))
            new_tlv_start = plv_start + tlv_tol_size + 4
            tlv_tol_size = tlv_tol_size + tlv_all_size
            f.seek(plv_start+2, 0)
            f.write(struct.pack('<h', tlv_tol_size))
            f.seek(new_tlv_start, 0)

        print('tlv_tol_size %d' %(tlv_tol_size))
        f.write(struct.pack('<h', tlv_type))
        f.write(struct.pack('<h', tlv_len))
        f.write(tlv_data)
        f.close()
        tlv_tol_size = tlv_tol_size + 4
        #if(tlv_tol_size > h_tlv_size):
        #    print('tlv_tol_size %d > header tlv size %d'%(tlv_tol_size, h_tlv_size))
         #   sys.exit(1)
        return tlv_tol_size
    return 0


def image_calc_checksum(data):
        s = sum(array.array('I',data))
        s = s & 0xffffffff
        return s

def image_add_cksum(filename, tlv_size):
    with open(filename, 'rb+') as f:
        f.seek(0, 0)
        data = f.read(48)
        h_magic0,h_magic1,h_load_addr,h_name,h_run_addr,h_img_size,h_img_chksum, h_hdr_chksum, \
        h_header_size,h_ptlv_size,h_tlv_size,h_version,h_flags \
        = struct.unpack('III8sIIIIHHHHI',data)
        if(h_magic0 != 0x48544341 or h_magic1 != 0x41435448):
            print('magic header check fail.')
            sys.exit(1)

        print('img header_size %d, img size=%d, ptlv_size=%d' %(h_header_size,h_img_size, h_ptlv_size))
        f.seek(h_header_size, 0)
        data = f.read(h_img_size)
        checksum = image_calc_checksum(data)
        checksum = 0xffffffff - checksum
        f.seek(0x1c, 0)
        f.write(struct.pack('<I', checksum))
        print('img checksum 0x%x.' %checksum)

        f.seek(0x28, 0)
        f.write(struct.pack('<H', tlv_size))

        f.seek(0x0, 0)
        data = f.read(h_header_size)
        checksum = image_calc_checksum(data)
        checksum = 0xffffffff - checksum
        f.seek(0x20, 0)
        f.write(struct.pack('<I', checksum))
        print('header checksum 0x%x.' %checksum)
        f.close()
        print('boot loader add cksum pass.')


def lark_boot_post_build(boot_name, boot_ini_name, nand_id_name):
    print('lark boot add ini & nand id to tlv')
    img_header_check(boot_name)
    boot_ini_f =   os.path.join(os.path.dirname(boot_name), 'bootini.bin')
    generate_boot_parameters(boot_name, boot_ini_name, boot_ini_f)
    tlv_size = Add_data_to_tlv(boot_name, boot_ini_f,IMAGE_TLV_BOOTINI)
    tmp_size = Add_data_to_tlv(boot_name, nand_id_name,IMAGE_TLV_NANDID)
    if (tmp_size != 0) :
        tlv_size = tmp_size
    image_add_cksum(boot_name, tlv_size)
    boot_padding(boot_name)

def pearlriver_boot_post_build(boot_name, boot_ini_name):
    print('pearlriver boot add ini')
    img_header_check(boot_name)
    boot_ini_f =   os.path.join(os.path.dirname(boot_name), 'bootini.bin')
    generate_boot_parameters(boot_name, boot_ini_name, boot_ini_f)
    tlv_size = Add_data_to_tlv(boot_name, boot_ini_f,IMAGE_TLV_BOOTINI)
    image_add_cksum(boot_name, tlv_size)
    boot_padding(boot_name)

def boot_post_build(boot_name, boot_ini_name, nand_id_name):

    with open(boot_name, 'rb+') as f:
        f.seek(0, 0)
        data = f.read(16)
        f.close()
        h_magic0,h_magic1,h_load_addr,h_name = struct.unpack('III4s',data)
        if(h_magic0 != 0x48544341 or h_magic1 != 0x41435448):
            print('boot loader header check fail.')
            return
        print('boot name =%s'% h_name)
        if(h_name == b'boot'):
            lark_boot_post_build(boot_name, boot_ini_name, nand_id_name)
        elif(h_name == b'brec'):
            pearlriver_boot_post_build(boot_name, boot_ini_name)
        else:
            lark_boot_post_build(boot_name, boot_ini_name, nand_id_name)

def main(argv):
    boot_post_build(argv[1], argv[2], argv[3])

if __name__ == "__main__":
    main(sys.argv)
