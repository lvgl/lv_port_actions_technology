/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SDFS nand and sd  interface
 */

#ifndef __SDFS_NAND_SD_H__
#define __SDFS_NAND_SD_H__

#include <kernel.h>

#ifdef CONFIG_SD_FS_NAND_SD_STORAGE
struct sd_dir * nand_sd_find_dir(u8_t stor_id, u8_t part, const char *filename, void *buf_size_32);
int nand_sd_sd_fread(u8_t stor_id, struct sd_file *sd_file, void *buffer, int len);
#else
static inline struct sd_dir * nand_sd_find_dir(u8_t stor_id, u8_t part, const char *filename, void *buf_size_32)
{
	return NULL;	
}
static inline int nand_sd_sd_fread(u8_t stor_id, struct sd_file *sd_file, void *buffer, int len)

{
	return -1;
}

#endif

#endif/* #define __SDFS_NAND_SD_H__ */
