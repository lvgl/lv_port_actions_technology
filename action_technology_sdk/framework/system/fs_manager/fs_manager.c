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
#include <fs/fsck_msdos.h>

#ifdef CONFIG_TASK_WDT
#include <task_wdt_manager.h>
#endif

static const char * const fat_fs_disk_names[] = {
	"NOR", "NAND", "PSRAM", "USB", "SD", ""
};

static const char * const fat_fs_volume_names[] = {
	"/NOR:", "/NAND:", "/PSRAM:", "/USB:", "/SD:", ""
};

struct fat_fs_volume {
	struct fs_mount_t *mp;
	uint8_t allocated : 1;	/* 0: buffer not allocated, 1: buffer allocated */
	uint8_t mounted : 1;	/* 0: volume not mounted, 1: volume mounted */
};

static struct fat_fs_volume fat_fs_volumes[_VOLUMES];

int fs_manager_init(void)
{
	int i;
	int res = 0;
	struct fs_mount_t *mp = NULL;

	for (i = 0; i < DISK_MAX_NUM; i++) {
		/* fs_manager_init() should be called only once */
		if (fat_fs_volumes[i].allocated == 1) {
			continue;
		}

		if (disk_access_init(fat_fs_disk_names[i]) != 0) {
			continue;
		}

		mp = mem_malloc(sizeof(struct fs_mount_t));
		if (mp == NULL) {
			goto malloc_failed;
		}

		mp->fs_data = mem_malloc(sizeof(FATFS));
		if (mp->fs_data == NULL) {
			goto malloc_failed;
		}
		mp->type = FS_FATFS;
		mp->mnt_point = fat_fs_volume_names[i];

#ifdef CONFIG_FS_MANAGER_INIT_CHECK
		fat_fs_volumes[i].mp = mp;
		fat_fs_volumes[i].allocated = 1;
		fat_fs_volumes[i].mounted = 0;
#else
		res = fs_mount(mp);
		if (res != FR_OK) {
			SYS_LOG_WRN("fs (%s) init failed (%d)", fat_fs_volume_names[i], res);
			mem_free(mp->fs_data);
			mem_free(mp);
		} else {
			SYS_LOG_INF("fs (%s) init success", fat_fs_volume_names[i]);
			fat_fs_volumes[i].mp = mp;
			fat_fs_volumes[i].allocated = 1;
			fat_fs_volumes[i].mounted = 1;
		}
#endif
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

static int fs_manager_get_volume(const uint8_t *volume_name)
{
	int index;

	for (index = 0; index < DISK_MAX_NUM; index++) {
		if (!strcmp(volume_name, fat_fs_volume_names[index])) {
			break;
		}
	}

	if (index >= DISK_MAX_NUM) {
		return -EINVAL;
	}

	if (fat_fs_volumes[index].allocated == 0) {
		return -EINVAL;
	}

	if (fat_fs_volumes[index].mounted == 0) {
		return -EINVAL;
	}

	return index;
}

int fs_manager_disk_init(const uint8_t *volume_name)
{
	int index, ret;
	struct fs_mount_t *mp = NULL;

	for (index = 0; index < DISK_MAX_NUM; index++) {
		if (!strcmp(volume_name, fat_fs_volume_names[index])) {
			break;
		}
	}

	if (index >= DISK_MAX_NUM) {
		return -EINVAL;
	}

	if (fat_fs_volumes[index].mounted) {
		return 0;
	}

	if (fat_fs_volumes[index].allocated == 0) {
		mp = mem_malloc(sizeof(struct fs_mount_t));
		if (mp == NULL) {
			goto malloc_failed;
		}

		mp->fs_data = mem_malloc(sizeof(FATFS));
		if (mp->fs_data == NULL) {
			goto malloc_failed;
		}
		mp->type = FS_FATFS;
		mp->mnt_point = fat_fs_volume_names[index];

		fat_fs_volumes[index].mp = mp;
		fat_fs_volumes[index].allocated = 1;
	}

	ret = fs_mount(fat_fs_volumes[index].mp);
	if (ret) {
		SYS_LOG_ERR("fs (%s) init failed", volume_name);
	} else {
		SYS_LOG_INF("fs (%s) init success", volume_name);
		fat_fs_volumes[index].mounted = 1;
	}

	return ret;

malloc_failed:
	SYS_LOG_ERR("malloc failed");
	if (mp) {
		if (mp->fs_data)
			mem_free(mp->fs_data);
		mem_free(mp);
	}
	return -ENOMEM;
}

int fs_manager_disk_uninit(const uint8_t *volume_name)
{
	int index, ret;

	index = fs_manager_get_volume(volume_name);
	if (index < 0) {
		return 0;	/* maybe umounted */
	}

	ret = fs_unmount(fat_fs_volumes[index].mp);
	if (ret) {
		SYS_LOG_ERR("fs (%s) uninit failed (%d)", volume_name, ret);
		return ret;
	}

	fat_fs_volumes[index].mounted = 0;

	return 0;
}

int fs_manager_disk_check(const uint8_t *volume_name)
{
	int index, ret = 0;

	for (index = 0; index < DISK_MAX_NUM; index++) {
		if (!strcmp(volume_name, fat_fs_volume_names[index])) {
			break;
		}
	}

	if (index >= DISK_MAX_NUM) {
		return -EINVAL;
	}

	if (fat_fs_volumes[index].allocated == 0) {
		return -EINVAL;
	}

#ifdef CONFIG_TASK_WDT
	task_wdt_stop();
#endif

#ifdef CONFIG_FSCK_MSDOS
	ret = fsck_msdos(fat_fs_disk_names[index], 0, 0);
#endif

	return ret;
}

int fs_manager_check(void)
{
	int i;

	for (i = 0; i < DISK_MAX_NUM; i++) {
		if (fat_fs_volumes[i].allocated) {
			fs_manager_disk_uninit(fat_fs_volume_names[i]);
			fs_manager_disk_check(fat_fs_volume_names[i]);
			fs_manager_disk_init(fat_fs_volume_names[i]);
		}
	}

	return 0;
}

int fs_manager_get_volume_state(const uint8_t *volume_name)
{
	if (fs_manager_get_volume(volume_name) < 0) {
		return 0;
	}

	return 1;
}

int fs_manager_get_free_capacity(const uint8_t *volume_name)
{
	struct fs_statvfs stat;
	int ret;

	if (fs_manager_get_volume(volume_name) < 0) {
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

int fs_manager_get_total_capacity(const uint8_t *volume_name)
{
	struct fs_statvfs stat;
	int ret;

	if (fs_manager_get_volume(volume_name) < 0) {
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

int fs_manager_sdcard_exit_high_speed(void)
{
#ifdef CONFIG_MMC_SDCARD_DEV_NAME
	struct device *sd_disk = device_get_binding(CONFIG_MMC_SDCARD_DEV_NAME);
	if (!sd_disk) {
		return -ENODEV;
	}

	return flash_ioctl(sd_disk, DISK_IOCTL_EXIT_HIGH_SPEED, NULL);
#else
	return 0;
#endif
}

int fs_manager_sdcard_enter_high_speed(void)
{
#ifdef CONFIG_MMC_SDCARD_DEV_NAME
	struct device *sd_disk = device_get_binding(CONFIG_MMC_SDCARD_DEV_NAME);

	if (!sd_disk) {
		return -ENODEV;
	}

	return flash_ioctl(sd_disk, DISK_IOCTL_ENTER_HIGH_SPEED, NULL);
#else
	return 0;
#endif
}

