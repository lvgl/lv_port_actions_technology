/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file sdfs manager interface 
 */

#ifndef __SDFS_MANGER_H__
#define __SDFS_MANGER_H__
#ifdef CONFIG_FILE_SYSTEM
#include <fs/fs.h>
#include <disk/disk_access.h>
#endif

/**
 * @defgroup sdfs_manager_apis App FS Manager APIs
 * @ingroup system_apis
 * @{
 */

/**
 * @brief sdfs manager init function
 *
 * @return 0 int success others failed
 */
int sdfs_manager_init(void);
/**
 * @} end defgroup fs_manager_apis
 */

#endif