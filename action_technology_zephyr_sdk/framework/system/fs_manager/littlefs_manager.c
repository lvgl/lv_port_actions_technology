/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file file system  manager interface
 */

#include <os_common_api.h>
#include <mem_manager.h>
#include <fs_manager.h>
#include <fs/fs.h>
#include <fs/littlefs.h>
#include <partition/partition.h>
#include <ff.h>

static const char * const little_fs_disk_names[] = {
	"NOR", "NAND", "PSRAM", "USB", "SD", ""
};

static const char * const littlefs_volume_names[] = {
	"/NOR:", "/NAND:", "/PSRAM:", "/USB:", "/SD:", ""
};

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(storage);

struct little_fs_volume {
	struct fs_mount_t *mp;
	uint8_t allocated : 1;	/* 0: buffer not allocated, 1: buffer allocated */
	uint8_t mounted : 1;	/* 0: volume not mounted, 1: volume mounted */
};

static struct little_fs_volume little_fs_volume[_VOLUMES];

int littlefs_manager_init(void)
{
	int i;
	int res = 0;
	struct fs_mount_t *mp = NULL;

	for (i = 0; i < DISK_MAX_NUM; i++) {
		/* fs_manager_init() should be called only once */
		if (little_fs_volume[i].allocated == 1) {
			continue;
		}

		if (disk_access_init(little_fs_disk_names[i]) != 0) {
			continue;
		}

		mp = mem_malloc(sizeof(struct fs_mount_t));
		if (mp == NULL) {
			goto malloc_failed;
		}

		mp->fs_data = &storage;
		mp->type = FS_LITTLEFS;
		mp->mnt_point = "/littlefs";
		mp->storage_dev = (void *)PARTITION_FILE_ID_LITTLEFS;

		res = fs_mount(mp);
		if (res != FR_OK) {
			SYS_LOG_WRN("fs (%s) init failed (%d)", littlefs_volume_names[i], res);
			mem_free(mp->fs_data);
			mem_free(mp);
		} else {
			SYS_LOG_INF("fs (%s) init success", littlefs_volume_names[i]);
			little_fs_volume[i].mp = mp;
			little_fs_volume[i].allocated = 1;
			little_fs_volume[i].mounted = 1;
		}
	}

	return res;
malloc_failed:
	SYS_LOG_ERR("malloc failed");
	if (mp) {
		if (mp->fs_data)
			mem_free(mp->fs_data);
		mem_free(mp);
	}
	return -ENOMEM;
}

static int littlefs_manager_get_volume(const uint8_t *volume_name)
{
	int index;

	for (index = 0; index < DISK_MAX_NUM; index++) {
		if (!strcmp(volume_name, littlefs_volume_names[index])) {
			break;
		}
	}

	if (index >= DISK_MAX_NUM) {
		return -EINVAL;
	}

	if (little_fs_volume[index].allocated == 0) {
		return -EINVAL;
	}

	if (little_fs_volume[index].mounted == 0) {
		return -EINVAL;
	}

	return index;
}

int littlefs_manager_get_volume_state(const uint8_t *volume_name)
{
	if (littlefs_manager_get_volume(volume_name) < 0) {
		return 0;
	}

	return 1;
}

int littlefs_manager_get_free_capacity(const uint8_t *volume_name)
{
	struct fs_statvfs stat;
	int ret;

	if (littlefs_manager_get_volume(volume_name) < 0) {
		return 0;
	}

	ret = fs_statvfs(volume_name, &stat);
	if (ret) {
		SYS_LOG_ERR("Error [%d]", ret);
		ret = 0;
	} else {
		if (stat.f_frsize < 1024 || stat.f_bfree < 1024) {
			ret = (stat.f_frsize * stat.f_bfree) >> 20;
		} else {
			ret = stat.f_frsize / 1024 * stat.f_bfree / 1024;
		}
		SYS_LOG_INF("total %d Mbytes", ret);
	}

	return ret;
}

int littlefs_manager_get_total_capacity(const uint8_t *volume_name)
{
	struct fs_statvfs stat;
	int ret;

	if (littlefs_manager_get_volume(volume_name) < 0) {
		return 0;
	}

	ret = fs_statvfs(volume_name, &stat);
	if (ret) {
		SYS_LOG_ERR("Error [%d]", ret);
		ret = 0;
	} else {
		if (stat.f_frsize < 1024 || stat.f_blocks < 1024) {
			ret = (stat.f_frsize * stat.f_blocks) >> 20;
		} else {
			ret = stat.f_frsize / 1024 * stat.f_blocks / 1024;
		}
		SYS_LOG_INF("total %d Mbytes", ret);
	}

	return ret;
}

