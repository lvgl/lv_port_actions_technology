/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA firmware file patch
 */

#include <kernel.h>
#include <string.h>
#include <soc.h>
#include <mem_manager.h>
#include <ota_backend.h>
#include <ota_storage.h>
#include "ota_image.h"
#include "ota_file_patch.h"
#include "hpatch.h"
#include <os_common_api.h>

static int g_patch_data_offs = 0;

static const unsigned short crc16_table[256] = {
        0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
        0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
        0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
        0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
        0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
        0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
        0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
        0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
        0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
        0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
        0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
        0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
        0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
        0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
        0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
        0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
        0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
        0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
        0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
        0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
        0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
        0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
        0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
        0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
        0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
        0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
        0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
        0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
        0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
        0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
        0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
        0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78,
};

static unsigned short reflect16(unsigned short b) {
    unsigned short v = 0;
    v |= (b & 0x8000) >> 15;
    v |= (b & 0x4000) >> 13;
    v |= (b & 0x2000) >> 11;
    v |= (b & 0x1000) >> 9;
    v |= (b & 0x0800) >> 7;
    v |= (b & 0x0400) >> 5;
    v |= (b & 0x0200) >> 3;
    v |= (b & 0x0100) >> 1;

    v |= (b & 0x0080) << 1;
    v |= (b & 0x0040) << 3;
    v |= (b & 0x0020) << 5;
    v |= (b & 0x0010) << 7;
    v |= (b & 0x0008) << 9;
    v |= (b & 0x0004) << 11;
    v |= (b & 0x0002) << 13;
    v |= (b & 0x0001) << 15;
    return v;
}

static unsigned short rom_crc16(const unsigned char *buf, int len, unsigned short initial_crc)
{
	unsigned short crc = initial_crc;

	crc = reflect16(crc);

	while (len--) {
		crc = ((crc >> 8) & 0xff) ^ crc16_table[(crc & 0xff) ^ *buf++];
	}

	crc = reflect16(crc);

	crc = ((crc >> 8) & 0xff) | ((crc & 0xff) << 8);

	return crc;
}

static int ota_patch_read_old_data(hpatch_TStreamInputHandle streamHandle, const hpatch_StreamPos_t readFromPos,
	unsigned char* out_data, unsigned char* out_data_end)
{
	struct ota_file_patch_info *ota_patch;
	int read_len;

	ota_patch = (struct ota_file_patch_info *)streamHandle;

	if (!ota_patch || !ota_patch->storage)
		return -1;

	read_len = ((int)out_data_end - (int)out_data);

	memcpy(out_data, ota_patch->old_file_mapping_addr + readFromPos, read_len);

	return read_len;
}

static int ota_patch_read_patch_data(hpatch_TStreamInputHandle streamHandle, const hpatch_StreamPos_t readFromPos,
	unsigned char* out_data, unsigned char* out_data_end)
{
	struct ota_file_patch_info *ota_patch;
	int read_len;
	int err;

	ota_patch = (struct ota_file_patch_info *)streamHandle;

	if (!ota_patch || !ota_patch->img)
		return -1;

	read_len = ((int)out_data_end - (int)out_data);

	err = ota_image_read(ota_patch->img,
		ota_patch->patch_file_offset + g_patch_data_offs + readFromPos,
		out_data, read_len);
	if (err) {
		SYS_LOG_ERR("cannot read data, offs 0x%x", readFromPos);
		return -EIO;
	}

	return read_len;
}

static int write_cache_data(struct ota_file_patch_info *ota_patch, int write_pos)
{
	uint32_t addr;
	uint16_t data_crc;
	int seg_size;

	if (write_pos % 0x20) {
		SYS_LOG_ERR("BUG: write_pos 0x%x\n", write_pos);
		return -EINVAL;
	}

	if (ota_patch->flag_use_crc) {
		addr = ota_patch->new_file_offset + (write_pos / 0x20) * 0x22;

		data_crc = rom_crc16(ota_patch->write_cache, 0x20, 0xffff);
		ota_patch->write_cache[0x20] = data_crc & 0xff;
		ota_patch->write_cache[0x21] = (data_crc >> 8) & 0xff;
		seg_size = 0x22;
	} else {
		addr = ota_patch->new_file_offset + write_pos;
		seg_size = 0x20;
	}

	if (ota_patch->flag_use_encrypt)
		addr |= 0x80000000;

	ota_storage_write(ota_patch->storage, addr,
		ota_patch->write_cache, seg_size);

	ota_patch->write_cache_pos = 0;
	ota_patch->write_cache_offs = write_pos + 0x20;

	return 0;
}

static int flush_cache_data(struct ota_file_patch_info *ota_patch)
{
	SYS_LOG_INF("flush ota cache write_cache_offs 0x%x, write_cache_pos 0x%x",
		ota_patch->write_cache_offs,
		ota_patch->write_cache_pos);

	if (ota_patch->write_cache_pos == 0)
		return 0;

	memset(ota_patch->write_cache + ota_patch->write_cache_pos, 0x0, 0x20 - ota_patch->write_cache_pos);

	return write_cache_data(ota_patch, ota_patch->write_cache_offs);
}

int write_data_with_crc_rand(struct ota_file_patch_info *ota_patch, int write_pos,
		const unsigned char* data, int write_len)
{
	int wlen, write_seg, unaligned_pos;

	wlen = write_len;

	if ((ota_patch->write_cache_offs + ota_patch->write_cache_pos) != write_pos) {
		SYS_LOG_ERR("BUG: write_cache_offs 0x%x, cur write_pos 0x%x",
			ota_patch->write_cache_offs, write_pos);
		return 0;
	}

	unaligned_pos = write_pos % 0x20;
	if (unaligned_pos) {
		write_seg = 0x20 - unaligned_pos;
		if (wlen < write_seg) {
			write_seg = wlen;
		}

		memcpy(ota_patch->write_cache + ota_patch->write_cache_pos, data, write_seg);

		ota_patch->write_cache_pos += write_seg;
		data += write_seg;
		wlen -= write_seg;
		write_pos += write_seg;

		if (ota_patch->write_cache_pos == 0x20) {
			write_cache_data(ota_patch, ota_patch->write_cache_offs);
		} else {
			return write_seg;
		}
	}

	while (wlen >= 0x20) {
		memcpy(ota_patch->write_cache, data, 0x20);
		write_cache_data(ota_patch, write_pos);

		write_pos += 0x20;
		data += 0x20;
		wlen -= 0x20;
	}

	if (wlen > 0) {
		SYS_LOG_INF("write to cache wlen 0x%x, write_cache_pos 0x%x",
			wlen, ota_patch->write_cache_pos);
		memcpy(ota_patch->write_cache, data, wlen);
		ota_patch->write_cache_offs = write_pos;
		ota_patch->write_cache_pos = wlen;
	}

	return write_len;
}

static int write_new_fw_file(hpatch_TStreamInputHandle streamHandle, const hpatch_StreamPos_t write_pos,
	const unsigned char* data, const unsigned char* data_end)
{
	struct ota_file_patch_info *ota_patch;
	int write_len = ((int)data_end - (int)data);

	ota_patch = (struct ota_file_patch_info *)streamHandle;
	if (!ota_patch || !ota_patch->storage)
		return -1;

	SYS_LOG_INF("write pos 0x%x len 0x%x", write_pos, write_len);

	if (ota_patch->flag_use_crc || ota_patch->flag_use_encrypt) {
		write_data_with_crc_rand(ota_patch, write_pos, data, write_len);
	} else {
		ota_storage_write(ota_patch->storage, ota_patch->new_file_offset + write_pos,
			(uint8_t *)data, write_len);
	}

	return write_len;
}

static void open_old_fw_file(struct ota_file_patch_info *ota_patch, hpatch_TStreamInput *fw_stream)
{
	fw_stream->streamHandle = ota_patch;
	fw_stream->streamSize = ota_patch->old_file_size;
	fw_stream->read = ota_patch_read_old_data;
}


static void open_patch_file(struct ota_file_patch_info *ota_patch, hpatch_TStreamInput *patch_stream)
{
	patch_stream->streamHandle = ota_patch;
	patch_stream->streamSize = ota_patch->patch_file_size;
	patch_stream->read = ota_patch_read_patch_data;

	g_patch_data_offs = 0;
}

static void open_new_fw_file(struct ota_file_patch_info *ota_patch, unsigned int size, hpatch_TStreamOutput *stream_out)
{
	stream_out->streamHandle = ota_patch;
	stream_out->streamSize = size;
	stream_out->write = write_new_fw_file;
}

static hpatch_StreamPos_t readSavedSize(unsigned char * diffData, unsigned int diffDataSize, int* out_sizeCodeLength)
{
	hpatch_StreamPos_t highSize;
	unsigned int diffFileDataSize = diffDataSize;
	int sizeCodeLength = -1;

	hpatch_StreamPos_t newDataSize = diffData[0] | (diffData[1] << 8) | (diffData[2] << 16);
	if (diffData[3] != 0xFF) {
		sizeCodeLength = 4;
		newDataSize |= (diffData[3] << 24);
	} else {
		sizeCodeLength = 9;

		if ((sizeof(int) <= 4) || (diffFileDataSize < 9)) {
			SYS_LOG_ERR("diffFileDataSize error!");
			return 0;
		}

		newDataSize |= (diffData[4] << 24);
		highSize = diffData[5] | (diffData[6] << 8) | (diffData[7] << 16) | (diffData[8] << 24);
		newDataSize |= ((highSize << 16) << 16);
	}

	*out_sizeCodeLength = sizeCodeLength;

	return newDataSize;
}

static hpatch_StreamPos_t readNewDataSize(hpatch_TStreamInput *diffFileData)
{
	char diffData[64];

	int readLength = 9;

	unsigned char diffData_begin = 0;

	int kNewDataSizeSavedSize = -1;

	hpatch_StreamPos_t newDataSize;

	if (readLength > diffFileData->streamSize) {
		readLength = (int)diffFileData->streamSize;
	}

	diffFileData->read(diffFileData->streamHandle, 0, &diffData[diffData_begin],
		&diffData[diffData_begin + readLength]);

	//print_buffer(diffData, 1, readLength, 16, 0);

	newDataSize = readSavedSize(diffData, sizeof(diffData), &kNewDataSizeSavedSize);

	diffFileData->streamSize -= kNewDataSizeSavedSize;

	g_patch_data_offs = kNewDataSizeSavedSize;

	return newDataSize;
}

int ota_file_patch_write(struct ota_file_patch_info *ota_patch)
{
	hpatch_StreamPos_t newDataSize;
	hpatch_TStreamInput old_fw_stream;
	hpatch_TStreamInput patch_fw_stream;
	hpatch_TStreamOutput new_fw_stream;

	SYS_LOG_INF("new fw: offs 0x%x size 0x%x, old fw: mapping addr %p offs 0x%x size 0x%x, patch fw: offs 0x%x size 0x%x",
		ota_patch->new_file_offset, ota_patch->new_file_size,
		ota_patch->old_file_mapping_addr, ota_patch->old_file_offset, ota_patch->old_file_offset,
		ota_patch->patch_file_offset, ota_patch->patch_file_size);
	SYS_LOG_INF("flag_use_crc %d, flag_use_encrypt %d",
		ota_patch->flag_use_crc, ota_patch->flag_use_encrypt);

	open_old_fw_file(ota_patch, &old_fw_stream);
	open_patch_file(ota_patch, &patch_fw_stream);
	newDataSize = readNewDataSize(&patch_fw_stream);

	SYS_LOG_INF("new fw: file size 0x%x", newDataSize);

	open_new_fw_file(ota_patch, newDataSize, &new_fw_stream);

	if (!patch_stream(&new_fw_stream, &old_fw_stream, &patch_fw_stream)) {
		SYS_LOG_ERR("  patch_stream run error!!!");
		return -1;
	}

	flush_cache_data(ota_patch);

	return 0;
}
