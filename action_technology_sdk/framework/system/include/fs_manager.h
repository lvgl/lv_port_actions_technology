/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file fs manager interface 
 */

#ifndef __FS_MANGER_H__
#define __FS_MANGER_H__
#ifdef CONFIG_FILE_SYSTEM
#include <ff.h>
#include <fs/fs.h>
#include <diskio.h>
#include <disk/disk_access.h>
#endif

/**
 * @defgroup fs_manager_apis App FS Manager APIs
 * @ingroup system_apis
 * @{
 */

/** disk type */
enum{
	/** disk type of nor */
	DISK_NOR,
	/** disk type of nand */
	DISK_NAND,
	/** disk type of psram */
	DISK_PSRAM,
	/** disk type of usb card read */
	DISK_USB,
	/** disk type of sdcard */
	DISK_SD_CARD,
	/** max number of disk*/
	DISK_MAX_NUM,
};

/**
 * @brief init disk of volume_name
 *
 * @param volume_name disk volume name 
 * @return 0 if successful, others if failed
 */
int fs_manager_disk_init(const uint8_t *volume_name);

/**
 * @brief uninit disk of volume_name
 *
 * @param volume_name disk volume name 
 * @return 0 if successful, others if failed
 */
int fs_manager_disk_uninit(const uint8_t *volume_name);

/**
 * @brief check disk of volume_name
 *
 * @param volume_name disk volume name 
 * @return 0 if successful, others if failed
 */
int fs_manager_disk_check(const uint8_t *volume_name);

/**
 * @brief Obtaining the residual capacity of the card
 *
 * @param volume_name disk volume name 
 * @return free capacity of card ,in units of MByte
 * @return 0 if no card detected or no free capacity of card
 */
int fs_manager_get_free_capacity(const uint8_t *volume_name);

/**
 * @brief Obtaining the total capacity of the card
 * @details if volume not mount ,return 0 , otherwise return 
 *  the total capacity of target volume.
 * @param volume_name disk volume name 
 * @return total capacity of card ,in units of MByte
 * @return 0 if no card detected
 */
int fs_manager_get_total_capacity(const uint8_t *volume_name);

/**
 * @brief Obtaining the volume state of the card
 *
 * @details if volume not mount ,return 0 , otherwise return 1
 * @param volume_name disk volume name 
 *
 * @return 1 volume mounted
 * @return 0 volume not mounted
 */

int fs_manager_get_volume_state(const uint8_t *volume_name);

/**
 * @brief fs manager init function
 *
 * @return 0 int success others failed
 */
int fs_manager_init(void);
/**
 * @brief little fs manager init function
 *
 * @return 0 int success others failed
 */
int littlefs_manager_init(void);

/**
 * @brief fs manager check function
 *
 * @return 0 int success others failed
 */
int fs_manager_check(void);

/**
 * @} end defgroup fs_manager_apis
 */

int fs_manager_sdcard_exit_high_speed(void);

int fs_manager_sdcard_enter_high_speed(void);

#endif
