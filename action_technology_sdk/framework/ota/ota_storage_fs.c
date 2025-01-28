/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA storage interface
 */

#include <kernel.h>
#include <drivers/flash.h>
#include <string.h>
#include <mem_manager.h>
#include <ota_storage.h>
#include <os_common_api.h>
#ifdef CONFIG_FILE_SYSTEM
#include <fs/fs.h>
#endif
#include <stream.h>

#define CONFIG_XSPI_NOR_ACTS_DEV_NAME "spi_flash"

#define XIP_DEV_NAME	CONFIG_XSPI_NOR_ACTS_DEV_NAME
#define CONFIG_MMC_SDCARD_DEV_NAME "sd"


struct ota_storage
{
	struct device *dev;
	const char *dev_name;
	struct fs_file_t *fs;
	int max_write_seg;
	int storage_id;		/* code run on this device? */
};

static struct ota_storage global_ota_storage;


int ota_storage_bind_fs(struct fs_file_t *fs)
{
	global_ota_storage.fs = fs;
	return 0;
}
int ota_storage_unbind_fs(struct fs_file_t *fs)
{
	if (global_ota_storage.fs == fs)
		global_ota_storage.fs = NULL;
	return 0;
}

int ota_storage_sync(struct ota_storage *storage)
{
	int res = -EINVAL;

	if (storage && storage->fs)
		res = fs_sync(storage->fs);

	return res;
}


void ota_storage_set_max_write_seg(struct ota_storage *storage, int max_write_seg)
{
	if (max_write_seg <= 0)
		return;

	storage->max_write_seg = max_write_seg;
}

int ota_storage_get_storage_id(struct ota_storage *storage)
{
	return storage->storage_id;
}

int ota_storage_write(struct ota_storage *storage, int offs, uint8_t *buf, int size)
{
	if (storage->fs) {
		fs_seek(storage->fs, offs, SEEK_DIR_BEG);
		fs_write(storage->fs, buf, size);
	}
	return 0;
}

int ota_storage_read(struct ota_storage *storage, int offs, uint8_t *buf, int size)
{
	if (storage->fs) {
		fs_seek(storage->fs, offs, SEEK_DIR_BEG);
		fs_read(storage->fs, buf, size);
	}

	return 0;
}

int ota_storage_is_clean(struct ota_storage *storage, int offs, int size,
			uint8_t *buf, int buf_size)
{
	SYS_LOG_INF("fs: ignore check clean");
	return 1;
}

int ota_storage_erase(struct ota_storage *storage, int offs, int size)
{
	return 0;
}

struct ota_storage *ota_storage_init(const char *storage_name)
{
	struct ota_storage *storage;
	const struct device *nor_dev;

	SYS_LOG_INF("init storage %s\n", storage_name);

	nor_dev = device_get_binding(storage_name);
	if (!nor_dev) {
		SYS_LOG_ERR("cannot found storage device %s", storage_name);
		return NULL;
	}

	storage = &global_ota_storage;

	memset(storage, 0x0, sizeof(struct ota_storage));

	storage->dev = nor_dev;
	storage->dev_name = storage_name;
	storage->max_write_seg = OTA_STORAGE_DEFAULT_WRITE_SEGMENT_SIZE;

	if (strcmp(storage_name, CONFIG_XSPI_NOR_ACTS_DEV_NAME) == 0)
		storage->storage_id = 0;
	else
		storage->storage_id = 1;

	return storage;
}

void ota_storage_exit(struct ota_storage *storage)
{
	SYS_LOG_INF("exit");

	storage = NULL;
}
