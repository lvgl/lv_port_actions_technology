#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import os;
import sys;
import time;
import argparse;
import random;
import datetime;
import subprocess;
import shutil
import struct
import array
from ctypes import *;
from string import *;
from math   import *;

# path
sign_root = os.getcwd()
sign_out = os.path.join(sign_root, "out")
sign_pubkey_f = os.path.join(sign_out, "pubkey.pem")
sign_pubkey_bin_f = os.path.join(sign_out, "client_pubkey.bin")
sign_pubkey_c_f  = os.path.join(sign_out, "client_pubkey.c")
sign_img_bin_f  = os.path.join(sign_out, "image_sign.bin")
sign_img_for_sign_bin_f  = os.path.join(sign_out, "image_for_sign.bin")
sign_img_sign_data_bin_f  = os.path.join(sign_out, "image_sign_data.bin")

IMAGE_TLV_INFO_MAGIC=0x5935
IMAGE_TLV_PROT_INFO_MAGIC=0x593a
"""
#define IMAGE_TLV_KEYHASH           0x01   /* hash of the public key */
#define IMAGE_TLV_PUBKEY            0x02   /* public key */
#define IMAGE_TLV_SHA256            0x04   /* SHA256 of image hdr and body */
#define IMAGE_TLV_RSA2048_PSS       0x08   /* RSA2048 of hash output */
#define IMAGE_TLV_PUBKEY_BROM       0x10   /* public key for mbrec */
"""
IMAGE_TLV_PUBKEY=0x2
IMAGE_TLV_PUBKEY_BROM=0x10
IMAGE_TLV_RSA2048_PSS=0x08

BINARY_BLOCK_SIZE_ALIGN = 512

IMAGE_F_ENCRYPTED=0x04

"""
/** Image TLV header.  All fields in little endian. */
struct image_tlv_info {
    uint16_t it_magic;
    uint16_t it_tlv_tot;  /* size of TLV area (including tlv_info header) */
};

/** Image trailer TLV format. All fields in little endian. */
struct image_tlv {
    uint16_t it_type;   /* IMAGE_TLV_[...]. */
    uint16_t it_len;    /* Data length (not including TLV header). */
};
"""

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

def genPubKeys(pri_act_pem_f):
    """
    Generates pubkey by private keys.
    """    
    cmd = [ "openssl", "rsa", "-in", pri_act_pem_f , "-pubout", sign_pubkey_f]
    (outmsg, exit_code) = run_cmd(cmd)
    if exit_code !=0:
        print("gen pubkey error prikey=%s\n"  %(pri_act_pem_f))
        print(outmsg)
        sys.exit(1)

def getClasspathSep():
    if os.name == 'nt':  # Check if running on Windows
        return ';'
    else:
        return ':'

def DumpPubKeys(pub_client_pem_f):
    """
    Generates cryptography keys.
    :return: String with generated keys encoded as PEM.
    :rtype: String
    :raises RuntimeError: when error occures while generating keys
    todo:: Consult feasibility of rewriting with PyOpenSSL library.
    java -cp bcprov-jdk15on-152.jar:. DumpPublicKey pub.pem > pubkey.c
    """
    
    cmd = [ "java", "-cp", "bcprov-jdk15on-152.jar"+getClasspathSep()+".", "DumpPublicKey", pub_client_pem_f, sign_pubkey_bin_f]
    (outmsg, exit_code) = run_cmd(cmd)
    if exit_code !=0:
        print("DumpPubKeys error pubkey=%s\n"  %(pub_client_pem_f))
        print(outmsg.decode('utf-8'))
        sys.exit(1)
    return outmsg

def boot_padding(filename, align = BINARY_BLOCK_SIZE_ALIGN):
    fsize = os.path.getsize(filename)
    if fsize % align:
        padding_size = align - (fsize % align)
        print('fsize 0x%x, padding_size 0x%x' %(fsize, padding_size))
        with open(filename, 'rb+') as f:
            f.seek(fsize, 0)
            buf = (c_byte * padding_size)();
            f.write(buf)
            f.close()

IMAGE_HEAD_SIZE = 512
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

def img_header_check():
    """
    check img header, and add h_img_size
    """    
    img_len = os.path.getsize(sign_img_bin_f)
    print('img  origin length 0x%x' %(img_len))
    if img_len % 4:
         boot_padding(sign_img_bin_f, 4)
         img_len = os.path.getsize(sign_img_bin_f)
         print('img  align 4 length 0x%x' %(img_len))
    #读取原始mbrec数据
    with open(sign_img_bin_f, 'rb+') as f:
        f.seek(0, 0)
        data = f.read(48)
        h_magic0,h_magic1,h_load_addr,h_name,h_run_addr,h_img_size,h_img_chksum, h_hdr_chksum, \
        h_header_size,h_ptlv_size,h_tlv_size,h_version,h_flags \
        = struct.unpack('III8sIIIIHHHHI',data)
        if(h_magic0 != 0x48544341 or h_magic1 != 0x41435448):
            print('magic header invalid, add head.')
            f.close()
            img_header_add(sign_img_bin_f)
            return 
        print('img h_header_size 0x%x.' %h_header_size)
        h_img_size = img_len - h_header_size
        f.seek(0x18, 0)
        f.write(struct.pack('<I', h_img_size))
        print('img h_img_size 0x%x.' %h_img_size)

        # header chk set 0
        f.seek(0x20, 0)
        f.write(struct.pack('<I', 0x0))

        f.seek(0x26, 0)
        f.write(struct.pack('<h', 0))
        f.close()

def AddPubKeys_to_prot_tlv(pubkey_file, tlv_type):
    """
    add public key to mbrec  prot_tlv
    """ 
    tlv_len = os.path.getsize(pubkey_file)
    print('sign_pubkey length 0x%x' %(tlv_len))
    #读取公钥数据   
    with open(sign_pubkey_bin_f, 'rb') as f:
        key = f.read()
        f.close()
   
    #读取原始mbrec数据
    with open(sign_img_bin_f, 'rb+') as f:
        f.seek(0, 0)
        data = f.read(48)
        h_magic0,h_magic1,h_load_addr,h_name,h_run_addr,h_img_size,h_img_chksum, h_hdr_chksum, \
        h_header_size,h_ptlv_size,h_tlv_size,h_version,h_flags \
        = struct.unpack('III8sIIIIHHHHI',data)
        if(h_magic0 != 0x48544341 or h_magic1 != 0x41435448):
            print('magic header check fail.')
            sys.exit(1)
        plv_start = h_header_size + h_img_size
        print('plv_start 0x%x,header_size=0x%x, img_len=0x%x  ptlv_size=0x%x' %(plv_start, h_header_size, h_img_size, h_ptlv_size))
        tlv_all_size = tlv_len + 4
        tlv_tol_size = 0
        f.seek(plv_start, 0)
        if(h_ptlv_size == 0):#not ptlv header
            print('add ptlv magic')
            tlv_tol_size = tlv_all_size
            f.write(struct.pack('<h', IMAGE_TLV_PROT_INFO_MAGIC))
            f.write(struct.pack('<h', tlv_tol_size))           
        else :
            data1 = f.read(4)
            tlv_magic, tlv_tol_size= struct.unpack('HH',data1)
            if(tlv_magic != IMAGE_TLV_PROT_INFO_MAGIC):
                print('ptlv magic  check fail.')
                sys.exit(1)
            print('old ptlv_tol_size 0x%x' %(tlv_tol_size))
            new_tlv_start = plv_start + tlv_tol_size + 4
            tlv_tol_size = tlv_tol_size + tlv_all_size
            f.seek(plv_start+2, 0)
            f.write(struct.pack('<h', tlv_tol_size))
            f.seek(new_tlv_start, 0)
        
 
        f.write(struct.pack('<h', tlv_type))
        f.write(struct.pack('<h', tlv_len))
        f.write(key)
        h_ptlv_new_size = tlv_tol_size + 4
        f.seek(0x26, 0)
        f.write(struct.pack('<h', h_ptlv_new_size))
        f.close()
        print('head h_ptlv_size=0x%x' %(h_ptlv_new_size))
  

def Add_data_to_tlv(img_bin_f, tlv_data_bin_f, tlv_type):
    """
    add data tlv
    """
    img_len = os.path.getsize(img_bin_f)
    tlv_len = os.path.getsize(tlv_data_bin_f)
    print('tlv data length 0x%x, img_len=0x%x' %(tlv_len,img_len))

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
        print('plv_start 0x%x,header_size=0x%x, img_len=0x%x  ptlv_size=0x%x' %(plv_start, h_header_size, h_img_size, h_ptlv_size))

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
            print('old tlv_tol_size 0x%x' %(tlv_tol_size))
            new_tlv_start = plv_start + tlv_tol_size + 4
            tlv_tol_size = tlv_tol_size + tlv_all_size
            f.seek(plv_start+2, 0)
            f.write(struct.pack('<h', tlv_tol_size))
            f.seek(new_tlv_start, 0)

        f.write(struct.pack('<h', tlv_type))
        f.write(struct.pack('<h', tlv_len))
        f.write(tlv_data)
        f.close()
        tlv_tol_size = tlv_tol_size + 4
        print('tlv_tol_size 0x%x, head h_tlv_size=0x%x' %(tlv_tol_size, h_tlv_size))
        if(tlv_tol_size > h_tlv_size):
            print('err:tlv_tol_size 0x%x > header tlv size 0x%x'%(tlv_tol_size, h_tlv_size))
            sys.exit(1)
        return tlv_tol_size
    return 0


def SignImageData(pri_pem_f):
    """
   openssl dgst -sha256 -sign ../pri.pem -out sign.bin  boot_for_sign.bin
    """
    print('sign file length 0x%x' %(os.path.getsize(sign_img_for_sign_bin_f)))
    #对公钥进行签名    
    cmd = [ "openssl", "dgst", "-sha256", "-sign", pri_pem_f, sign_img_for_sign_bin_f]
    (outmsg, exit_code) = run_cmd(cmd)
    if exit_code !=0:
        print("SignImageData error %d \n"  %(exit_code))
        sys.exit(1)
    with open(sign_img_sign_data_bin_f, 'wb') as f:
        f.write(outmsg)
        f.close()
    Add_data_to_tlv(sign_img_bin_f, sign_img_sign_data_bin_f,IMAGE_TLV_RSA2048_PSS)

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

        print('img header_size 0x%x, img size=0x%x, ptlv_size=0x%x' %(h_header_size,h_img_size, h_ptlv_size))
        f.seek(h_header_size, 0)
        data = f.read(h_img_size + h_ptlv_size)
        checksum = image_calc_checksum(data)
        checksum = 0xffffffff - checksum
        f.seek(0x1c, 0)
        f.write(struct.pack('<I', checksum))
        print('img checksum 0x%x.' %checksum)

        f.seek(0x28, 0)
        f.write(struct.pack('<H', tlv_size))
        #设置签名标记

        f.seek(0x2C, 0)
        f.write(struct.pack('<I', IMAGE_F_ENCRYPTED))

        f.seek(0x0, 0)
        data = f.read(h_header_size)
        checksum = image_calc_checksum(data)
        checksum = 0xffffffff - checksum
        f.seek(0x20, 0)
        f.write(struct.pack('<I', checksum))
        print('header checksum 0x%x.' %checksum)

        #生成待签名数据

        print('create img for sign')
        f.seek(0, 0)
        data = f.read(h_img_size+h_header_size + h_ptlv_size)
        with open(sign_img_for_sign_bin_f, 'wb+') as mf:
            mf.write(data)
            mf.close()

        f.close()
        print('boot loader add cksum pass.')

def main(argv):
    parser = argparse.ArgumentParser(
        description='ras digital signature.',
    )

    parser.add_argument('-i',   dest = 'in_img_bin',     required = True,    help = 'input sign img')
    parser.add_argument('-c',   dest = 'pub_key_pem',    required = False,   help = 'pukey for verify next img')
    parser.add_argument('-b',   dest = 'brom_pub_key',    required = False,   help = 'pukey for brom verify mbrec, must add to mbrec')
    parser.add_argument('-k',   dest = 'pri_key_pem',    required = False,    help = 'pri.pem of sign.')
    parser.add_argument('-o',   dest = 'out_img_bin',    required = True,    help = 'output sing img.')

    args   = parser.parse_args()
    if os.path.exists(sign_out) == True:
        shutil.rmtree(sign_out, True)
    os.makedirs(sign_out)
    out_img_dir = os.path.dirname(args.out_img_bin)
    if os.path.exists(out_img_dir) == False:
        os.makedirs(out_img_dir)

    shutil.copyfile(args.in_img_bin, sign_img_bin_f)

    img_header_check()

    if args.pri_key_pem != None:
        if args.pub_key_pem == None:
            #sign app.bin
            print("sign app.bin")
            image_add_cksum(sign_img_bin_f, 2048)
            SignImageData(args.pri_key_pem)
        else:
            #sign mbrec.bin
           
            if args.brom_pub_key == None:
                print("bootloader")
            else:
                print("mbrec")
                keys = DumpPubKeys(args.brom_pub_key)
                with open(sign_pubkey_c_f, "wb") as f:
                    f.write(keys)
                    f.close()
                AddPubKeys_to_prot_tlv(sign_pubkey_bin_f, IMAGE_TLV_PUBKEY_BROM)

            keys = DumpPubKeys(args.pub_key_pem)
            with open(sign_pubkey_c_f, "wb") as f:
                f.write(keys)
                f.close()
            AddPubKeys_to_prot_tlv(sign_pubkey_bin_f, IMAGE_TLV_PUBKEY)
            image_add_cksum(sign_img_bin_f, 2048)
            SignImageData(args.pri_key_pem)
        print("RSA Sign Pass.")
    else:
        image_add_cksum(sign_img_bin_f, 2048)
    boot_padding(sign_img_bin_f)
    shutil.copyfile(sign_img_bin_f, args.out_img_bin)
    print("img pack  Pass")


if __name__ == '__main__':
    main(sys.argv[1:])

