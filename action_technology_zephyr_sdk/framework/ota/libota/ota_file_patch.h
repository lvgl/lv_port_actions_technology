/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA  file patch interface
 */

#ifndef __OTA_FILE_PATCH_H__
#define __OTA_FILE_PATCH_H__

struct ota_file_patch_info {
	struct ota_storage *storage;
	struct ota_image *img;

	uint32_t flag_use_crc:1;
	uint32_t flag_use_encrypt:1;

	int write_cache_size;
	int write_cache_pos;
	uint32_t write_cache_offs;
	uint8_t *write_cache;


	uint8_t *old_file_mapping_addr;
	int old_file_offset;
	int old_file_size;

	int new_file_offset;
	int new_file_size;

	int patch_file_offset;
	int patch_file_size;
};

int ota_file_patch_write(struct ota_file_patch_info *ota_patch);

#endif /* __OTA_FILE_PATCH_H__ */
