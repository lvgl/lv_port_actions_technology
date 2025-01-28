/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA manifest file interface
 */

#ifndef __OTA_MANIFEST_H__
#define __OTA_MANIFEST_H__

#include <fw_version.h>

#define OTA_MANIFEST_MAX_FILE_CNT	15

struct ota_file {
	uint8_t name[13];
	uint8_t type;
	uint8_t file_id;
#ifdef CONFIG_OTA_MUTIPLE_STORAGE
	uint8_t storage_id;
#else
	uint8_t reserved;
#endif
	uint32_t offset;
	uint32_t size;
	uint32_t orig_size;
	uint32_t checksum;
} __attribute__((packed));

struct ota_manifest {
	struct fw_version fw_ver;
#if defined(CONFIG_OTA_FILE_PATCH) || defined(CONFIG_OTA_RES_PATCH)

	struct fw_version old_fw_ver;
#endif

	int file_cnt;
	uint8_t *manifest_data;
	uint32_t manifest_size;

	struct ota_file wfiles[OTA_MANIFEST_MAX_FILE_CNT];
};

struct ota_image;

int ota_manifest_parse_file(struct ota_manifest *manifest, struct ota_image *img,
			    const char *file_name);

#endif /* __OTA_MANIFEST_H__ */
