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
IMAGE_HEAD_SIZE = 512

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

def img_header_add(img_bin_f):
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
        f.seek(IMAGE_HEAD_SIZE, 0)
        data = f.read(8)
        i_sp,i_run_addr = struct.unpack('II',data)
        print('i_run_addr 0x%x.' %i_run_addr)
        f.seek(0, 0)
        f.write(struct.pack('<I', 0x48544341))
        f.seek(4, 0)
        f.write(struct.pack('<I', 0x41435448))
        f.seek(8, 0)
        f.write(struct.pack('<I', i_run_addr))
        f.seek(0xc, 0)
        f.write(struct.pack('<s', b'boot'))
        f.seek(0x14, 0)
        f.write(struct.pack('<I', i_run_addr))
        h_img_size = img_len - IMAGE_HEAD_SIZE
        f.seek(0x18, 0)
        f.write(struct.pack('<I', h_img_size))

        f.seek(0x20, 0)
        f.write(struct.pack('<I', 0x0))

        f.seek(0x24, 0)
        f.write(struct.pack('<I', IMAGE_HEAD_SIZE))
        """
        h_ptlv_size = 0;
        """
        f.seek(0x26, 0)
        f.write(struct.pack('<h', 0))
        f.close()



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


def bootloader_img_post_build(imge_name):
    print('bootloader image %s add header' % imge_name)
    with open(imge_name, 'rb+') as f:
        f.seek(0x0, 0)
        data = f.read(8)
        f.close()
        h_magic0,h_magic1= struct.unpack('II',data)
        if(h_magic0 == 0x48544341 and h_magic1 == 0x41435448) or (h_magic0 == 0x0 and h_magic1 == 0x0) :
            img_header_add(imge_name)
            image_add_cksum(imge_name, 0)
            boot_padding(imge_name)
        else:
            boot_padding(imge_name)
            print('bootloader  %s not have header' % imge_name)
            sys.exit(1)

def app_header_add(img_bin_f):
    """
    check img header, and add h_img_size
    """
    img_len = os.path.getsize(img_bin_f)
    print('app  origin length %d' %(img_len))
    if img_len % 4:
         boot_padding(img_bin_f, 4)
         img_len = os.path.getsize(img_bin_f)
         print('img  align 4 length %d' %(img_len))

    with open(img_bin_f, 'rb+') as f:
        f.seek(0, 0)
        data = f.read(8)
        i_sp,i_run_addr = struct.unpack('II',data)
        print('i_run_addr 0x%x.' %i_run_addr)
        f.seek(0x208, 0)
        f.write(struct.pack('<I', i_run_addr))
        f.seek(0x214, 0)
        f.write(struct.pack('<I', i_run_addr))
        f.seek(0x218, 0)
        f.write(struct.pack('<I', img_len))
        f.seek(0x21c, 0)
        f.write(struct.pack('<I', 0x0))
        f.seek(0x220, 0)
        f.write(struct.pack('<I', 0x0))
        f.seek(0x226, 0)
        f.write(struct.pack('<h', 0))
        f.seek(0x228, 0)
        f.write(struct.pack('<H', 0))

        f.close()

def app_add_cksum(filename):
    with open(filename, 'rb+') as f:
        f.seek(0x200, 0)
        data = f.read(48)
        h_magic0,h_magic1,h_load_addr,h_name,h_run_addr,h_img_size,h_img_chksum, h_hdr_chksum, \
        h_header_size,h_ptlv_size,h_tlv_size,h_version,h_flags \
        = struct.unpack('III8sIIIIHHHHI',data)
        if(h_magic0 != 0x48544341 or h_magic1 != 0x41435448):
            print('app magic header check fail.')
            sys.exit(1)

        print('app header_size %d, img size=%d, ptlv_size=%d' %(h_header_size,h_img_size, h_ptlv_size))
        f.seek(0, 0)
        data = f.read(h_img_size)
        checksum = image_calc_checksum(data)
        checksum = 0xffffffff - checksum
        f.seek(0x21c, 0)
        f.write(struct.pack('<I', checksum))
        print('img checksum 0x%x.' %checksum)

        f.seek(0x200, 0)
        data = f.read(h_header_size)
        checksum = image_calc_checksum(data)
        checksum = 0xffffffff - checksum
        f.seek(0x220, 0)
        f.write(struct.pack('<I', checksum))
        print('app header checksum 0x%x.' %checksum)
        f.close()
        print('app add cksum pass.')


def app_img_post_build(imge_name):
    print('app image %s add header' % imge_name)
    app_header_add(imge_name)
    app_add_cksum(imge_name)
    #boot_padding(imge_name)

def boot_post_build(imge_name):
    with open(imge_name, 'rb+') as f:
        f.seek(0x200, 0)
        data = f.read(8)
        f.close()
        h_magic0,h_magic1= struct.unpack('II',data)
        if(h_magic0 != 0x48544341 or h_magic1 != 0x41435448):
            bootloader_img_post_build(imge_name)
        else:
            app_img_post_build(imge_name)

def main(argv):
    boot_post_build(argv[1])

if __name__ == "__main__":
    main(sys.argv)
