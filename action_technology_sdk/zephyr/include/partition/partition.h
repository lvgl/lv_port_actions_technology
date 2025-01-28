/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief partition interface
 */

#ifndef __PARTITION_H__
#define __PARTITION_H__

#define MAX_PARTITION_COUNT	30

struct partition_entry {
	u8_t name[8];
	u8_t type;
	u8_t file_id;
	u8_t mirror_id:4;
	u8_t storage_id:4;
	u8_t flag;
	u32_t offset;
	u32_t size;
	u32_t file_offset;
}  __attribute__((packed));


//storage_id
#define STORAGE_ID_NOR 		0
#define STORAGE_ID_SD	 	1
#define STORAGE_ID_NAND  	2
#define STORAGE_ID_DATA_NOR 3
#define STORAGE_ID_BOOTNAND 4
#define STORAGE_ID_MAX		5


#define PARTITION_FLAG_ENABLE_CRC			(1 << 0)
#define PARTITION_FLAG_ENABLE_ENCRYPTION	(1 << 1)
#define PARTITION_FLAG_BOOT_CHECK			(1 << 2)
#define PARTITION_FLAG_USED_SECTOR			(1 << 3)

#define PARTITION_MIRROR_ID_A				(0)
#define PARTITION_MIRROR_ID_B				(1)
#define PARTITION_MIRROR_ID_INVALID			(0xf)

#define PARTITION_TYPE_INVALID				(0)
#define PARTITION_TYPE_BOOT					(1)
#define PARTITION_TYPE_SYSTEM				(2)
#define PARTITION_TYPE_RECOVERY				(3)
#define PARTITION_TYPE_DATA					(4)
#define PARTITION_TYPE_TEMP					(5)
#define PARTITION_TYPE_PARAM				(6)

/* predefined file ids */
#define PARTITION_FILE_ID_INVALID			(0)
#define PARTITION_FILE_ID_BOOT				(1)
#define PARTITION_FILE_ID_PARAM				(2)
#define PARTITION_FILE_ID_RECOVERY			(3)
#define PARTITION_FILE_ID_SYSTEM			(4)
#define PARTITION_FILE_ID_SDFS				(5)
#define PARTITION_FILE_ID_NVRAM_FACTORY		(6)
#define PARTITION_FILE_ID_NVRAM_FACTORY_RW	(7)
#define PARTITION_FILE_ID_NVRAM_USER		(8)
#define PARTITION_FILE_ID_IMAGE_SCRATCH		(9)
/*sdfs 10-40*/
#define PARTITION_FILE_ID_SDFS_PART_BASE	(10)
#define PARTITION_FILE_ID_SDFS_PART0		(PARTITION_FILE_ID_SDFS_PART_BASE+0)
#define PARTITION_FILE_ID_SDFS_PART1		(PARTITION_FILE_ID_SDFS_PART_BASE+1)
#define PARTITION_FILE_ID_SDFS_PART2		(PARTITION_FILE_ID_SDFS_PART_BASE+2)
#define PARTITION_FILE_ID_SDFS_PART30		(PARTITION_FILE_ID_SDFS_PART_BASE+30)

#define PARTITION_FILE_ID_UDISK				40
#define PARTITION_FILE_ID_LITTLEFS			41

#define PARTITION_FILE_ID_RUNTIME_LOGBUF      (0xfa)
#define PARTITION_FILE_ID_LOGBUF            (0xfb)
#define PARTITION_FILE_ID_EVTBUF            (0xfc)
#define PARTITION_FILE_ID_COREDUMP          (0xfd)
#define PARTITION_FILE_ID_OTA_TEMP			(0xfe)
#define PARTITION_FILE_ID_RESERVED			(0xff)

const struct partition_entry *partition_get_part(u8_t file_id);
const struct partition_entry *partition_get_mirror_part(u8_t file_id);
const struct partition_entry *partition_find_part(u32_t nor_phy_addr);
const struct partition_entry *partition_get_part_by_id(u8_t part_id);
const struct partition_entry *partition_get_temp_part(void);
const struct partition_entry *partition_get_stf_part(u8_t stor_id, u8_t file_id);


u8_t partition_get_current_file_id(void);
u8_t partition_get_current_mirror_id(void);
int partition_is_mirror_part(const struct partition_entry *part);
struct device *partition_get_storage_dev(const struct partition_entry *part);

int partition_get_max_file_size(const struct partition_entry *part);
int partition_is_param_part(const struct partition_entry *part);
int partition_is_boot_part(const struct partition_entry *part);
int partition_file_mapping(u8_t file_id, u32_t vaddr);
bool partition_is_used_sector(const struct partition_entry *part);

#endif /* __PARTITION_H__ */
