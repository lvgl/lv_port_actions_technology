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
#ifdef CONFIG_WATCHDOG
#include <watchdog_hal.h>
#endif

#define CONFIG_XSPI_NOR_ACTS_DEV_NAME "spi_flash"
#define CONFIG_XSPI_EXT_NOR_ACTS_DEV_NAME "spi_flash_2"
#define OTA_STORAGE_EXT_DEVICE_NAME "spinand"
#define OTA_STORAGE_SD_DEVICE_NAME "sd"

#define XIP_DEV_NAME	CONFIG_XSPI_NOR_ACTS_DEV_NAME
#define OTA_STORAGE_SD_SECTOR_SIZE (512)

enum ota_storage_type {
	OTA_STORAGE_SPINOR = 0,
	OTA_STORAGE_SD,
	OTA_STORAGE_NAND,
	OTA_STORAGE_EXT_NOR,
	OTA_STORAGE_BOOTNAND,
	OTA_STORAGE_MAX_TYPE
};

struct ota_storage
{
	const struct device *dev;
	const char *dev_name;
	int max_write_seg;
	int max_erase_seg;
	int storage_id;		/* code run on this device? */
	int storage_type;	/* storage type and refer to enum ota_storage_type */
};

#define IS_STORAGE_TYPE_SD_NAND(x) ((x)->storage_type == OTA_STORAGE_SD || (x)->storage_type == OTA_STORAGE_NAND || (x)->storage_type == OTA_STORAGE_BOOTNAND)
static struct ota_storage global_ota_storage[OTA_STORAGE_MAX_TYPE];

int ota_storage_sync(struct ota_storage *storage)
{
	if (storage && storage->dev && (storage->storage_type == OTA_STORAGE_NAND || storage->storage_type == OTA_STORAGE_BOOTNAND))
		flash_flush(storage->dev, true);
	return 0;
}

void ota_storage_set_max_write_seg(struct ota_storage *storage, int max_write_seg)
{
	if (max_write_seg <= 0)
		return;

	storage->max_write_seg = max_write_seg;
}

void ota_storage_set_max_erase_seg(struct ota_storage *storage, int max_erase_seg)
{
	if (max_erase_seg <= 0)
		return;

	storage->max_erase_seg = max_erase_seg;
}

int ota_storage_get_storage_id(struct ota_storage *storage)
{
	return storage->storage_id;
}

static int ota_storage_write_sd_nand(struct ota_storage *storage, int offs,
		      uint8_t *buf, int size)
{
	int err = 0;
	uint32_t count, left;
	uint8_t *buffer;

	SYS_LOG_DBG("offs 0x%x, buf %p, size %d", offs, buf, size);

	if (storage == NULL || storage->dev == NULL)
		return -EINVAL;

	if (offs % OTA_STORAGE_SD_SECTOR_SIZE) {
		SYS_LOG_ERR("invalid offset %d", offs);
		return -EINVAL;
	}

	count = size / OTA_STORAGE_SD_SECTOR_SIZE;
	left = size % OTA_STORAGE_SD_SECTOR_SIZE;
	offs /= OTA_STORAGE_SD_SECTOR_SIZE;

	if (count) {
		err = flash_write(storage->dev, offs<<9, buf, count<<9);
		if (err) {
			SYS_LOG_ERR("write error %d, offs 0x%x, count %d", err, offs, count);
			return -EIO;
		}
	}

	if (left) {
		SYS_LOG_INF("write residual data %d", left);
		buffer = (uint8_t *)mem_malloc(OTA_STORAGE_SD_SECTOR_SIZE);
		if (!buffer) {
			SYS_LOG_ERR("can not malloc %d size", OTA_STORAGE_SD_SECTOR_SIZE);
			return -ENOMEM;
		}

		memset(buffer, 0, OTA_STORAGE_SD_SECTOR_SIZE);

		buf += (count * OTA_STORAGE_SD_SECTOR_SIZE);
		offs += count;
		memcpy(buffer, buf, left);

		err = flash_write(storage->dev, offs<<9, buffer, 1<<9);

		mem_free(buffer);
	}

	return err;
}

static int ota_storage_write_default(struct ota_storage *storage, int offs,
		      uint8_t *buf, int size)
{
	int wlen, err;

	SYS_LOG_DBG("offs 0x%x, buf %p, size %d", offs, buf, size);

	if (storage == NULL || storage->dev == NULL)
		return -EINVAL;

	wlen = storage->max_write_seg;

	while (size > 0) {
		if (size < storage->max_write_seg)
			wlen = size;

		err = flash_write(storage->dev, offs, buf, wlen);
		if (err < 0) {
			SYS_LOG_ERR("write error %d, offs 0x%x, buf %p, size %d", err, offs, buf, size);
			return -EIO;
		}

		offs += wlen;
		buf += wlen;
		size -= wlen;
	}

	return 0;
}

int ota_storage_write(struct ota_storage *storage, int offs,
		      uint8_t *buf, int size)
{
	if (storage == NULL || storage->dev == NULL)
		return -EINVAL;

	if (IS_STORAGE_TYPE_SD_NAND(storage))
		return ota_storage_write_sd_nand(storage, offs, buf, size);
	else
		return ota_storage_write_default(storage, offs, buf, size);
}

static int ota_storage_read_sd_nand(struct ota_storage *storage, int offs,
		     uint8_t *buf, int size)
{
	int err = 0;
	uint32_t count, left;
	uint8_t *buffer = NULL;

	SYS_LOG_DBG("offs 0x%x, buf %p, size %d", offs, buf, size);

	if (storage == NULL || storage->dev == NULL)
		return -EINVAL;

	if (offs % OTA_STORAGE_SD_SECTOR_SIZE) {
		SYS_LOG_ERR("invalid offset %d", offs);
		return -EINVAL;
	}

	count = size / OTA_STORAGE_SD_SECTOR_SIZE;
	left = size % OTA_STORAGE_SD_SECTOR_SIZE;
	offs /= OTA_STORAGE_SD_SECTOR_SIZE;

	if (count) {
		err = flash_read(storage->dev, offs<<9, buf, count<<9);
		if (err) {
			SYS_LOG_ERR("read error %d, offs 0x%x, count %d", err, offs, count);
			return -EIO;
		}
	}

	if (left) {
		buffer = (uint8_t *)mem_malloc(OTA_STORAGE_SD_SECTOR_SIZE);
		if (!buffer) {
			SYS_LOG_ERR("can not malloc %d size", OTA_STORAGE_SD_SECTOR_SIZE);
			return -ENOMEM;
		}

		buf += (count * OTA_STORAGE_SD_SECTOR_SIZE);
		offs += count;

		err = flash_read(storage->dev, offs<<9, buffer, 1<<9);
		if (err) {
			SYS_LOG_ERR("read error %d, offs 0x%x", err, offs);
			err = -EIO;
		}

		memcpy(buf, buffer, left);

		mem_free(buffer);
	}

	return err;
}

static int ota_storage_read_default(struct ota_storage *storage, int offs,
		     uint8_t *buf, int size)
{
	int err;
	int rlen = OTA_STORAGE_DEFAULT_READ_SEGMENT_SIZE;

	SYS_LOG_DBG("offs 0x%x, buf %p, size %d", offs, buf, size);

	if (storage == NULL || storage->dev == NULL)
		return -EINVAL;

	while (size > 0) {
		if (size < OTA_STORAGE_DEFAULT_READ_SEGMENT_SIZE)
			rlen = size;

		err = flash_read(storage->dev, offs, buf, rlen);
		if (err < 0) {
			SYS_LOG_ERR("read error %d, offs 0x%x, buf %p, size %d", err, offs, buf, size);
			return -EIO;
		}

		offs += rlen;
		buf += rlen;
		size -= rlen;
	}

	return 0;

}

int ota_storage_read(struct ota_storage *storage, int offs,
		     uint8_t *buf, int size)
{
	if (storage == NULL || storage->dev == NULL)
		return -EINVAL;

	if (IS_STORAGE_TYPE_SD_NAND(storage))
		return ota_storage_read_sd_nand(storage, offs, buf, size);
	else
		return ota_storage_read_default(storage, offs, buf, size);
}
int ota_storage_sd_nand_is_clean(struct ota_storage *storage, int offs, int size,
			uint8_t *buf, int buf_size)
{
	int i, err;
	uint32_t *wptr = NULL;
	uint8_t *cptr;


	if (storage == NULL || buf == NULL || buf_size == 0)
		return -EINVAL;

	while (size > 0) {
		if (size < buf_size)
			buf_size = size;
		err = ota_storage_read_sd_nand(storage, offs, buf, buf_size);
		if (err) {
			SYS_LOG_ERR("read error 0x%x, offs 0x%x, size %d", err, offs, size);
			return err;
		}

		wptr = (uint32_t *)buf;
		for (i = 0; i < (buf_size >> 2); i++) {
			if (*wptr++ != 0xffffffff)
				return 0;
		}

		offs += buf_size;
		size -= buf_size;
	}
	/* check unaligned data */
	cptr = (uint8_t *)wptr;
	if (cptr && buf != cptr) {
		for (i = 0; i < (buf_size & 0x3); i++) {
			if (*cptr++ != 0xff)
				return 0;
		}
	}

	return 1;
}

int ota_storage_is_clean(struct ota_storage *storage, int offs, int size,
			uint8_t *buf, int buf_size)
{
	int i, err, read_size;
	uint32_t *wptr = NULL;
	uint8_t *cptr;

	//SYS_LOG_INF("offs 0x%x, size %d", offs, size);
	if (storage == NULL)
		return -EINVAL;

	if (IS_STORAGE_TYPE_SD_NAND(storage)) {
		return 0;  // always dirty for sd/nand
		//return ota_storage_sd_nand_is_clean(storage, offs, size, buf, size);
	}

	if (((unsigned int)buf & 0x3) || (buf_size & 0x3)) {
		return -EINVAL;
	}

	read_size = (buf_size > OTA_STORAGE_DEFAULT_READ_SEGMENT_SIZE)?
			OTA_STORAGE_DEFAULT_READ_SEGMENT_SIZE : buf_size;
	while (size > 0) {
		if (size < read_size)
			read_size = size;

		err = flash_read(storage->dev, offs, buf, read_size);
		if (err) {
			SYS_LOG_ERR("read error 0x%x, offs 0x%x, size %d", err, offs, size);
			return -EIO;
		}

		wptr = (uint32_t *)buf;
		for (i = 0; i < (read_size >> 2); i++) {
			if (*wptr++ != 0xffffffff)
				return 0;
		}

		offs += read_size;
		//buf += read_size;
		size -= read_size;
	}

	/* check unaligned data */
	cptr = (uint8_t *)wptr;
	if (cptr && buf != cptr) {
		for (i = 0; i < (read_size & 0x3); i++) {
			if (*cptr++ != 0xff)
				return 0;
		}
	}

	return 1;
}
int ota_storage_erase_sd_nand(struct ota_storage *storage, int offs, int size)
{
	int err = 0;
	uint32_t count;
	uint8_t *buffer;

	if (storage == NULL)
		return -EINVAL;

	if (offs % OTA_STORAGE_SD_SECTOR_SIZE) {
		SYS_LOG_ERR("invalid offset %d", offs);
		return -EINVAL;
	}

	count = size / OTA_STORAGE_SD_SECTOR_SIZE;
	if (size % OTA_STORAGE_SD_SECTOR_SIZE)
		count++;
	offs /= OTA_STORAGE_SD_SECTOR_SIZE;

	buffer = (uint8_t *)mem_malloc(OTA_STORAGE_SD_SECTOR_SIZE);
	if (!buffer) {
		SYS_LOG_ERR("can not malloc %d size", OTA_STORAGE_SD_SECTOR_SIZE);
		return -ENOMEM;
	}
	memset(buffer, 0xff, OTA_STORAGE_SD_SECTOR_SIZE);

	while (count--) {
		err = flash_write(storage->dev, (offs++)<<9, buffer, 1<<9);
		if (err) {
			SYS_LOG_ERR("write error %d, offs 0x%x, count %d", err, offs, count);
			mem_free(buffer);
			return -EIO;
		}
	#ifdef CONFIG_WATCHDOG
		watchdog_clear();
	#endif
	}
	mem_free(buffer);

	return err;
}

int ota_storage_erase_spinor(struct ota_storage *storage, int offs, int size)
{
	int err = 0;
	int erase_size = 0;

	if (storage == NULL)
		return -EINVAL;

	/* write aligned page data */
	while (size > 0) {
		if (size < storage->max_erase_seg) {
			erase_size = size;
		} else if (offs & (storage->max_erase_seg - 1)) {
			erase_size = storage->max_erase_seg - (offs & (storage->max_erase_seg - 1));
		} else {
			erase_size = storage->max_erase_seg;
		}

		err = flash_erase(storage->dev, offs, erase_size);
		if (err) {
			SYS_LOG_ERR("write error %d, offs 0x%x", err, offs);
			return -EIO;
		}
		size -= erase_size;
		offs += erase_size;
	#ifdef CONFIG_WATCHDOG
		watchdog_clear();
	#endif
	}
	return err;
}

int ota_storage_erase(struct ota_storage *storage, int offs, int size)
{
	SYS_LOG_INF("offs 0x%x, size %d", offs, size);
	if (storage == NULL)
		return -EINVAL;

	if (IS_STORAGE_TYPE_SD_NAND(storage)) {
		// improve perf: only erase first page for sd_nand
		return ota_storage_erase_sd_nand(storage, offs, OTA_STORAGE_SD_SECTOR_SIZE);
	} else if (storage->storage_type == OTA_STORAGE_SPINOR || storage->storage_type == OTA_STORAGE_EXT_NOR) {
		return ota_storage_erase_spinor(storage, offs, size);
	}

	return flash_erase(storage->dev, offs, size);
}

struct ota_storage *ota_storage_find(int storage_id)
{
	if (storage_id < OTA_STORAGE_MAX_TYPE && global_ota_storage[storage_id].dev)
		return &global_ota_storage[storage_id];
	return NULL;
}

struct ota_storage *ota_storage_init(const char *storage_name)
{
	struct ota_storage *storage = NULL;
	const struct device *nor_dev;

	SYS_LOG_INF("init storage %s\n", storage_name);

	nor_dev = device_get_binding(storage_name);
	if (!nor_dev) {
		SYS_LOG_ERR("cannot found storage device %s", storage_name);
		return NULL;
	}

	if (strcmp(storage_name, CONFIG_XSPI_NOR_ACTS_DEV_NAME) == 0) {
		storage = &global_ota_storage[OTA_STORAGE_SPINOR];
		memset(storage, 0x0, sizeof(struct ota_storage));
		storage->storage_id = OTA_STORAGE_SPINOR;
		storage->storage_type = OTA_STORAGE_SPINOR;

		storage->dev = nor_dev;
		storage->dev_name = storage_name;
		storage->max_write_seg = OTA_STORAGE_DEFAULT_WRITE_SEGMENT_SIZE;
		storage->max_erase_seg = OTA_STORAGE_DEFAULT_ERASE_SEGMENT_SIZE;

	} else if (strcmp(storage_name, OTA_STORAGE_EXT_DEVICE_NAME) == 0) {
#ifdef CONFIG_BOARD_NANDBOOT
		storage = &global_ota_storage[OTA_STORAGE_BOOTNAND];
		memset(storage, 0x0, sizeof(struct ota_storage));
		storage->storage_id = OTA_STORAGE_BOOTNAND;
		storage->storage_type = OTA_STORAGE_BOOTNAND;
#else
		storage = &global_ota_storage[OTA_STORAGE_NAND];
		memset(storage, 0x0, sizeof(struct ota_storage));
		storage->storage_id = OTA_STORAGE_NAND;
		storage->storage_type = OTA_STORAGE_NAND;
#endif

		storage->dev = nor_dev;
		storage->dev_name = storage_name;
		storage->max_write_seg = OTA_STORAGE_DEFAULT_WRITE_SEGMENT_SIZE;
		storage->max_erase_seg = OTA_STORAGE_DEFAULT_ERASE_SEGMENT_SIZE;
	} else if (strcmp(storage_name, OTA_STORAGE_SD_DEVICE_NAME) == 0) {
		storage = &global_ota_storage[OTA_STORAGE_SD];
		memset(storage, 0x0, sizeof(struct ota_storage));
		storage->storage_id = OTA_STORAGE_SD;
		storage->storage_type = OTA_STORAGE_SD;

		storage->dev = nor_dev;
		storage->dev_name = storage_name;
		storage->max_write_seg = OTA_STORAGE_DEFAULT_WRITE_SEGMENT_SIZE;
		storage->max_erase_seg = OTA_STORAGE_DEFAULT_ERASE_SEGMENT_SIZE;
	}  else if (strcmp(storage_name, CONFIG_XSPI_EXT_NOR_ACTS_DEV_NAME) == 0) {
		storage = &global_ota_storage[OTA_STORAGE_EXT_NOR];
		memset(storage, 0x0, sizeof(struct ota_storage));
		storage->storage_id = OTA_STORAGE_EXT_NOR;
		storage->storage_type = OTA_STORAGE_EXT_NOR;

		storage->dev = nor_dev;
		storage->dev_name = storage_name;
		storage->max_write_seg = OTA_STORAGE_DEFAULT_WRITE_SEGMENT_SIZE;
		storage->max_erase_seg = OTA_STORAGE_DEFAULT_ERASE_SEGMENT_SIZE;
	} else {
		SYS_LOG_ERR("unmatch storage name %s\n", storage_name);
	}
	return storage;
}

void ota_storage_exit(struct ota_storage *storage)
{
	SYS_LOG_INF("exit");
	if (storage && storage->dev)
		flash_flush(storage->dev, false);
	if (storage)
		memset(storage, 0, sizeof(struct ota_storage));
	storage = NULL;
}
