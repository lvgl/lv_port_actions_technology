/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file sd file system  manager interface
 */

#include <os_common_api.h>
#include <zephyr.h>
//#include <logging/sys_log.h>
#include <init.h>
#include <kernel.h>
#include <kernel_structs.h>
//#include <flash.h>

#include <mem_manager.h>
#include <sdfs_manager.h>
#include <board_cfg.h>

static const char * const sd_fs_volume_names[] = {
#ifdef CONFIG_SPINAND_ACTS
	 "/NAND:A", "/NAND:K", "/NAND:C", "/NAND:B",
#elif CONFIG_BOARD_EMMCBOOT
	"/SD:A", "/SD:K", "/SD:C", "/SD:B"
#elif CONFIG_DISK_ACCESS_SD
	"/SD:A", "/SD:K", "/SD:C", "/SD:B"
#elif CONFIG_SDFS_NOR_NOT_XIP
#if IS_ENABLED(CONFIG_SPI_FLASH_2)
	"/DNOR:A", "/DNOR:K", "/DNOR:C", "/DNOR:B"
#else
	"/NOR:A", "/NOR:K", "/NOR:C", "/NOR:B"
#endif
#else
	"/DNOR:A", "/DNOR:K", "/DNOR:C", "/DNOR:B"
#endif
	

};


struct sd_fs_volume {
	struct fs_mount_t *mp;
	uint8_t allocated : 1;	/* 0: buffer not allocated, 1: buffer allocated */
	uint8_t mounted : 1;	/* 0: volume not mounted, 1: volume mounted */
};

static struct sd_fs_volume sd_fs_volumes[ARRAY_SIZE(sd_fs_volume_names)];

int sdfs_manager_init(void)
{
	int i;
	int res = 0;
	struct fs_mount_t *mp = NULL;

	for (i = 0; i < ARRAY_SIZE(sd_fs_volume_names); i++) {
		/* fs_manager_init() should be called only once */
		if (sd_fs_volumes[i].allocated == 1) {
			continue;
		}

		mp = mem_malloc(sizeof(struct fs_mount_t));
		if (mp == NULL) {
			goto malloc_failed;
		}

		mp->type = FS_SDFS;
		mp->mnt_point = sd_fs_volume_names[i];

		res = fs_mount(mp);
		if (res != 0) {
//			SYS_LOG_WRN("fs (%s) init failed (%d)", sd_fs_volume_names[i], res);
			mem_free(mp);
		} else {
			SYS_LOG_INF("fs (%s) init success", sd_fs_volume_names[i]);
			sd_fs_volumes[i].mp = mp;
			sd_fs_volumes[i].allocated = 1;
			sd_fs_volumes[i].mounted = 1;
		}
	}

	return res;
malloc_failed:
	SYS_LOG_ERR("malloc failed");
	if (mp) {
		mem_free(mp);
	}
	return -ENOMEM;
}

