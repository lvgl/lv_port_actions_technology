import struct
import sys
import os
import time
import re
from ctypes import *


class RamdumpHead(Structure):
    _fields_ = [
        ("magic", c_uint32),
        ("version", c_uint16),
        ("hdr_sz", c_uint16),
        ("img_sz", c_uint32),
        ("org_sz", c_uint32),
        ("datetime", c_uint8 * 16),
        ("inc_uid", c_uint16),
        ("cpu_id", c_uint16),
        ("reserved", c_uint8 * 4),
        ("img_chksum", c_uint32),
        ("hdr_chksum", c_uint32),
    ]


class RamdumpRegionHead(Structure):
    _fields_ = [
        ("magic", c_uint32),
        ("type", c_uint16),
        ("hdr_sz", c_uint16),
        ("img_sz", c_uint32),
        ("org_sz", c_uint32),
        ("address", c_uint32),
        ("reserved", c_uint8 * 12),
    ]


class CompressHead(Structure):
    _fields_ = [
        ("algo_id", c_uint32),
        ("hdr_size", c_uint32),
        ("img_size", c_uint32),
        ("org_size", c_uint32),
    ]


class McpuDebug(Structure):
    _fields_ = [
        ("r", c_uint32 * 16),
        ("xpsr", c_uint32),
        ("msp", c_uint32),
        ("psp", c_uint32),
        ("exc_ret", c_uint32),
        ("control", c_uint32),
        ("reserved", c_uint32 * 11),
    ]


class BtcpuDebug(Structure):
    _fields_ = [
        ("bt_info", c_uint8 * 128),
    ]


class RamdumpAddr(Structure):
    _fields_ = [
        ("start", c_int32),
        ("next", c_int32),
        ("filter", c_int32),
    ]


RAMD_FLASH_BLK_SZ = 4 * 1024
RAMD_COMPR_BLK_SZ = 32 * 1024

RAMD_VERSION = 0x0001

MAGIC_RAMD = 0x444d4152
MAGIC_RAMR = 0x524d4152
MAGIC_LZ4 = 0x20345a4c
MAGIC_FLZ = 0x205a4c46

COMPR_NULL = 0,
COMPR_RLE = 0x20454c52
COMPR_BZ3 = 0x20335a42
COMPR_QLZ = 0x205a4c51
COMPR_LZ4 = 0x20345a4c
COMPR_FL2 = 0x20324c46
COMPR_XZ = 0x414d5a4c
COMPR_BROT = 0x544f5242
COMPR_ZSTD = 0x4454535a
COMPR_FLZ = 0x205a4c46
COMPR_MLZ = 0x205a4c4d
COMPR_HTSH = 0x48535448
COMPR_SHOC = 0x434f4853
COMPR_SMAZ = 0x5a414d53
COMPR_UNIS = 0x53494e55

TYPE_ADDR = 0
TYPE_MCPU_DBG = 1
TYPE_BTCPU_DBG = 2
TYPE_TRACEDUMP_DBG = 3

CORTEX_M4 = 0xc24
CORTEX_M33 = 0x132

strOnProjectLoadStart = \
    "void OnProjectLoad(void) {\r\n" \
    "	Debug.SetConnectMode(CM_ATTACH);\r\n" \
    "	Debug.SetResetMode(RM_BREAK_AT_SYMBOL);\r\n" \
    "	Project.SetHostIF(\"USB\", \"\");\r\n" \
    "	Project.SetTargetIF(\"SWD\");\r\n" \
    "	Project.SetTIFSpeed(\"4 MHz\");\r\n" \
    "	//Project.SetOSPlugin(\"ZephyrPlugin_CM4\");\r\n"

strOnProjectLoadEnd = \
    "	Project.SetDevice(\"%s\");\r\n" \
    "	//Project.SetDevice(\"LEOPARD\");\r\n" \
    "	Project.AddSvdFile(\"$(InstallDir)/Config/CPU/%sF.svd\");\r\n" \
    "	//Project.AddPathSubstitute (\"\",\"\");\r\n" \
    "	File.Open(\"$(ProjectDir)/%s\");\r\n" \
    "}\r\n\r\n"

strOnStartupComplete = \
    "void OnStartupComplete(void) {\r\n" \
    "	LoadRamDump();\r\n" \
    "}\r\n\r\n"

strAfterTargetReset = \
    "void AfterTargetReset(void) {\r\n" \
    "	Target.SetReg(\"SP\", Target.ReadU32(0x0));\r\n" \
    "	Target.SetReg(\"PC\", Target.ReadU32(0x4));\r\n" \
    "}\r\n\r\n"

strRamDumpStart = \
    "void LoadRamDump(void) {\r\n" \
    "	Target.WriteU32(0x40018004, 0x9);\r\n"

strRamDumpLoadMem = \
    "	Target.LoadMemory(\"0x%08x.bin\", 0x%08x);\r\n"

strRamDumpSetRegIdx = \
    "	Target.SetReg(\"R%d\", 0x%08x);\r\n"

strRamDumpSetRegStr = \
    "	Target.SetReg(\"%s\", 0x%08x);\r\n"

strRamDumpSetPC = \
    "	Debug.SetNextPC(0x%08x);\r\n" \
    "	Show.PC();\r\n" \
    "	Show.Memory(0x%08x);\r\n"

strRamDumpEnd = \
    "}\r\n\r\n"

strZephyrElf = "../../zephyr.elf"

iodev_addr = RamdumpAddr()
iodev_addr.start = 0x40000000
iodev_addr.next = 0x50000000
iodev_addr.filter = 0

s_crc32 = [
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
]


def utils_crc32(crc, data):
    crcu32 = crc
    crcu32 = ~crcu32 & 0xFFFFFFFF

    for b in data:
        crcu32 = (crcu32 >> 4) ^ s_crc32[(crcu32 & 0xF) ^ (b & 0xF)]
        crcu32 = (crcu32 >> 4) ^ s_crc32[(crcu32 & 0xF) ^ (b >> 4)]

    return ~crcu32 & 0xFFFFFFFF


def print_usage(argv0):
    print("Usage:")
    # print("  -c	ramdump compress")
    print("  -d	ramdump decompress")
    print("  -o	output file/dir")
    print("\nExample:")
    # print("  %s -c in-dir -o out-file" % argv0)
    print("  %s -d in-file -o out-dir" % argv0)


def process_argvs():
    is_compress = None
    in_file = None
    out_file = None

    for i in range(len(sys.argv)):
        if sys.argv[i] == "-c":
            is_compress = True
            in_file = sys.argv[i + 1]
        elif sys.argv[i] == "-d":
            is_compress = False
            in_file = sys.argv[i + 1]
        elif sys.argv[i] == "-o":
            out_file = sys.argv[i + 1]
    return is_compress, in_file, out_file


def init_decompress_ramdump_head(src):
    ramdump_head = RamdumpHead()
    memmove(addressof(ramdump_head), src, sizeof(RamdumpHead))
    if ramdump_head.magic != MAGIC_RAMD:
        return None

    checksum = utils_crc32(0, src[:ramdump_head.hdr_sz - 4])
    if ramdump_head.hdr_chksum != checksum:
        print("verify header failed");
        return None

    if ramdump_head.hdr_sz + ramdump_head.img_sz <= len(src):
        checksum = utils_crc32(0, src[ramdump_head.hdr_sz: ramdump_head.hdr_sz + ramdump_head.img_sz])
        if ramdump_head.img_chksum != checksum:
            print("verify image failed");
            return None
    else:
        print("The file is not complete!");

    return ramdump_head


def ramdump_dir_list(folder):
    dir_list = []
    for file in os.listdir(folder):
        if not os.path.isdir(os.path.join(folder, file)):
            continue
        pattern = r"\d{8}-\d{6}-\d{2}"

        if re.match(pattern, file):
            dir_list.append(os.path.join(folder, file))

    return dir_list


def ramdum_file_list(folder):
    file_list = []
    for file in os.listdir(folder):
        if os.path.isfile(os.path.join(folder, file)):
            file_list.append(os.path.join(folder, file))

    return file_list


def ramdump_compress_file(in_dir, out_file):
    pass


def round_up(x, a):
    return (x + (a - 1)) & ~(a - 1)


def fastlz1_decompress(compr_data):
    raw_data = bytes()
    compr_idx = 0
    while compr_idx < len(compr_data):
        ctrl = compr_data[compr_idx]
        if compr_idx == 0:
            ctrl &= 31
        compr_idx += 1
        if ctrl >= 32:
            lens = (ctrl >> 5) - 1
            ofs = (ctrl & 31) << 8
            ref_idx = len(raw_data) - ofs - 1
            if lens == 6:
                lens += compr_data[compr_idx]
                compr_idx += 1
            ref_idx -= compr_data[compr_idx]
            compr_idx += 1
            lens += 3
            if ref_idx < 0:
                return b''
            while lens > 0:
                mlen = lens
                if ref_idx + lens > len(raw_data):
                    mlen = len(raw_data) - ref_idx
                raw_data += raw_data[ref_idx: ref_idx + mlen]
                ref_idx += mlen
                lens -= mlen
        else:
            ctrl += 1
            if compr_idx + ctrl > len(compr_data):
                return b''
            raw_data += compr_data[compr_idx: compr_idx + ctrl]
            compr_idx += ctrl
    return raw_data


def __decompress_single(algo, src):
    dst = bytes()
    if algo == COMPR_FLZ:
        dst = fastlz1_decompress(src)

    return dst


def __decompress(algo, src):
    src_length = len(src)

    dst = bytes()
    offset = 0
    while src_length > 0:
        compress_head = CompressHead()
        memmove(addressof(compress_head), src[offset:], sizeof(CompressHead))
        offset += sizeof(CompressHead)
        src_length -= sizeof(CompressHead)

        if compress_head.hdr_size != sizeof(CompressHead):
            return dst

        in_size = compress_head.img_size
        sub_dst = __decompress_single(compress_head.algo_id, src[offset:offset + in_size])
        if len(sub_dst) != compress_head.org_size:
            return dst

        in_size = round_up(in_size, 4)
        offset += in_size
        src_length -= in_size
        dst += sub_dst

    return dst


def ramdump_decompress(ramdump_src):
    if len(ramdump_src) < sizeof(RamdumpRegionHead):
        return None

    region_head = RamdumpRegionHead()
    memmove(addressof(region_head), ramdump_src, sizeof(RamdumpRegionHead))

    dst = __decompress(COMPR_FLZ,
                       ramdump_src[sizeof(RamdumpRegionHead):region_head.img_sz + sizeof(RamdumpRegionHead)])

    if len(dst) <= 0:
        return region_head, None

    return region_head, dst


def get_file_info(ramdump_type, address, addr_file_list, dst):
    mcpu_debug = None
    btcpu_debug = None
    file_name = str()
    if ramdump_type == TYPE_MCPU_DBG:
        file_name = 'dbg_mcpu.bin'
        mcpu_debug = McpuDebug()
        memmove(addressof(mcpu_debug), dst, sizeof(McpuDebug))

    elif ramdump_type == TYPE_BTCPU_DBG:
        file_name = 'dbg_btcpu.bin'
        btcpu_debug = BtcpuDebug()
        memmove(addressof(btcpu_debug), dst, sizeof(BtcpuDebug))

    elif ramdump_type == TYPE_TRACEDUMP_DBG:
        file_name = 'trace_dump.bin'

    elif ramdump_type == TYPE_ADDR:
        file_name = '0x%08x.bin' % address
        addr_file_list.append(address)

    return file_name, mcpu_debug, btcpu_debug


def out_ramdump_decompress_files(ramdump_src, ramdump_out_dir, addr_file_list):
    sub_offset = 0
    out_size = 0
    mcpu_debug = None
    btcpu_debug = None
    while True:
        if sub_offset > len(ramdump_src) - 1:
            break

        region_head, dst = ramdump_decompress(ramdump_src[sub_offset:])
        if dst is None:
            break

        sub_offset += region_head.hdr_sz
        sub_offset += region_head.img_sz
        out_size += len(dst)

        file_name, mcpu_debug1, btcpu_debug1 = get_file_info(region_head.type,
                                                             region_head.address,
                                                             addr_file_list,
                                                             dst)

        if mcpu_debug1 is not None:
            mcpu_debug = mcpu_debug1

        if btcpu_debug1 is not None:
            btcpu_debug = btcpu_debug1

        print("  name: %15s, type: %d, address: 0x%08x, size: %d, img_sz: %d"
              % (file_name, region_head.type, region_head.address, len(dst), region_head.img_sz))

        with open(ramdump_out_dir + '\\' + file_name, "wb") as out_file:
            out_file.write(dst)

    return out_size, mcpu_debug, btcpu_debug


def build_ozone_jdebug(cpu_id, addr_file_list, mcpu_debug, btcpu_debug):
    cup_str = str()

    if cpu_id == CORTEX_M4:
        cup_str = 'Cortex-M4'
    elif cpu_id == CORTEX_M33:
        cup_str = 'Cortex-M33'

    dst = strOnProjectLoadStart.encode()

    cmd_str = strOnProjectLoadEnd % (cup_str, cup_str, strZephyrElf)
    dst += cmd_str.encode()

    dst += strOnStartupComplete.encode()
    dst += strAfterTargetReset.encode()
    dst += strRamDumpStart.encode()

    for addr in addr_file_list:
        if iodev_addr.start <= addr < iodev_addr.next:
            continue
        cmd_str = strRamDumpLoadMem % (addr, addr)
        dst += cmd_str.encode()

    if mcpu_debug is not None:
        cmd_str = strRamDumpSetRegStr % ('Control', (mcpu_debug.control << 24))
        dst += cmd_str.encode()

        for i in range(len(mcpu_debug.r)):
            cmd_str = strRamDumpSetRegIdx % (i, mcpu_debug.r[i])
            dst += cmd_str.encode()

        if cpu_id != CORTEX_M33:
            cmd_str = strRamDumpSetRegStr % ('MSP', mcpu_debug.msp)
            dst += cmd_str.encode()

            cmd_str = strRamDumpSetRegStr % ('PSP', mcpu_debug.psp)
            dst += cmd_str.encode()

            cmd_str = strRamDumpSetRegStr % ('XPSR', mcpu_debug.xpsr)
            dst += cmd_str.encode()

        cmd_str = strRamDumpSetPC % (mcpu_debug.r[15], mcpu_debug.r[13])
        dst += cmd_str.encode()

    dst += strRamDumpEnd.encode()
    return dst


def step_offset(offset):
    if offset % RAMD_FLASH_BLK_SZ:
        offset = round_up(offset, RAMD_FLASH_BLK_SZ)
    else:
        offset += RAMD_FLASH_BLK_SZ
    return offset


def check_ramdump_head(src):
    offset = 0
    ramdump_head = None
    while offset < len(src):
        ramdump_head = init_decompress_ramdump_head(src[offset:])
        if ramdump_head is not None:
            break
        else:
            offset = step_offset(offset)

    if offset >= len(src):
        return -1, None

    return offset, ramdump_head


def ramdump_decompress_file(in_file, out_dir):
    os.makedirs(out_dir, exist_ok=True)

    f = open(in_file, "rb")
    src = f.read()
    src_length = len(src)
    f.close()

    if src_length <= 0:
        print("Read %s size 0" % in_file)
        return -4

    offset, _ = check_ramdump_head(src)

    if offset < 0 or offset >= src_length:
        return -5

    src = src[offset:] + src[:offset]
    start_offset = offset
    offset = 0
    while offset < src_length:
        addr_file_list = []
        ramdump_head = init_decompress_ramdump_head(src[offset:])
        if ramdump_head is None:
            offset = step_offset(offset)
            continue

        in_size = ramdump_head.hdr_sz + ramdump_head.img_sz
        ramdump_src = src[offset + ramdump_head.hdr_sz:offset + in_size]

        ramdump_out_dir = "{}\\{}-{}".format(out_dir, bytes(ramdump_head.datetime[:-1]).decode(), ramdump_head.inc_uid)
        os.makedirs(ramdump_out_dir, exist_ok=True)
        print("[ramdump v%d] %s org_sz=%d, img_sz = %d, uid = %d, offset = 0x%x." % (ramdump_head.version,
                                                                                     bytes(ramdump_head.datetime[
                                                                                           :-1]).decode(),
                                                                                     ramdump_head.org_sz,
                                                                                     ramdump_head.img_sz,
                                                                                     ramdump_head.inc_uid,
                                                                                     start_offset + offset))
        print("Output dir: %s." % ramdump_out_dir)
        out_size, mcpu_debug, btcpu_debug = out_ramdump_decompress_files(ramdump_src, ramdump_out_dir, addr_file_list)
        offset += in_size
        print("  Decompressed %d -> %d (%d%%)" % (in_size, out_size, in_size * 100 / out_size))
        print("[Ozone] Build ozone.jdebug\r\n")

        dst = build_ozone_jdebug(ramdump_head.cpu_id, addr_file_list, mcpu_debug, btcpu_debug)

        with open(ramdump_out_dir + '\\' + 'ozone.jdebug', "wb") as out_file:
            out_file.write(dst)


def main():
    argvs = len(sys.argv)
    if argvs < 5:
        print_usage(sys.argv[0])
        return -1

    is_compress, in_argv, out_argv = process_argvs()
    if not in_argv or not out_argv:
        print_usage(sys.argv[0])
        return -2

    if is_compress:
        ramdump_compress_file(in_argv, out_argv)
    else:
        ramdump_decompress_file(in_argv, out_argv)


if __name__ == '__main__':
    start_time = time.time() * 1000
    main()
    end_time = time.time() * 1000
    print("Consume：", end_time - start_time, "ms.")
