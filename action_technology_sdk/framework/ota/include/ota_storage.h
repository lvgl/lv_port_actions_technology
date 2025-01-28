/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA storage interface
 */

#ifndef __OTA_STORAGE_H__
#define __OTA_STORAGE_H__

#include <device.h>
#ifdef CONFIG_FILE_SYSTEM
#include <fs/fs.h>
#endif

#define OTA_STORAGE_DEFAULT_WRITE_SEGMENT_SIZE		(4*1024)
#define OTA_STORAGE_DEFAULT_READ_SEGMENT_SIZE		(4*1024)

#define OTA_STORAGE_DEFAULT_ERASE_SEGMENT_SIZE		(64*1024)
#define OTA_STORAGE_MAX_ERASE_SEGMENT_SIZE			(1024*1024)

struct ota_storage;

/**
 * @brief ota storage init funcion
 *
 * This routine calls to init ota storage,called by libota
 *
 * @param storage_name the storage name
 *
 * @return pointer to storage struct address.
 * @return NULL if init failed.
 */

struct ota_storage *ota_storage_init(const char *storage_name);

/**
 * @brief ota storage find funcion
 *
 * This routine calls to get storage pinter,called by libota
 *
 * @param storage_id the storage id
 *
 * @return pointer to storage struct address.
 * @return NULL if init failed.
 */
struct ota_storage *ota_storage_find(int storage_id);

/**
 * @brief ota storage exit funcion
 *
 * This routine calls to make storage pinter to NULL,called by libota
 *
 * @param storage pointer to storage struct address
 *
 */

void ota_storage_exit(struct ota_storage *storage);

/**
 * Read data from storage.
 * @param storage pointer to storage
 * @param offs starting offset of storage for read
 * @param buf storage will write its data here
 * @param size bytes user want to read from storage
 * @return 0: read success.
 * @return other: read fail.
 */

int ota_storage_read(struct ota_storage *storage, int offs,
		     uint8_t *buf, int size);

/**
 * write data to storage.
 * @param storage pointer to storage
 * @param offs starting offset of storage for the write
 * @param buf data poninter of write
 * @param size bytes user want to write to storage
 * @return 0: write success.
 * @return other: write fail.
 */

int ota_storage_write(struct ota_storage *storage, int offs,
		      uint8_t *buf, int size);

/**
 * erase the storage.
 * @param storage pointer to storage
 * @param offs erase area starting offset
 * @param size size of area to be erased
 * @return 0: erase success.
 * @return other: erase fail.
 */

int ota_storage_erase(struct ota_storage *storage, int offs, int size);

/**
 * check the area of the storage is clean.
 * @param storage pointer to storage
 * @param offs starting offset of storage for the check
 * @param size bytes user want to check from storage
 * @param buf read data to here from storage
 * @param buf_size size of the buf
 * @return 0: clean.
 * @return other: dirty.
 */

int ota_storage_is_clean(struct ota_storage *storage, int offs, int size,
			uint8_t *buf, int buf_size);

void ota_storage_set_max_write_seg(struct ota_storage *storage, int max_write_seg);

void ota_storage_set_max_erase_seg(struct ota_storage *storage, int max_erase_seg);

/**
 * get the storage id.
 * @param storage pointer to storage
 * @return storage id.
 */

int ota_storage_get_storage_id(struct ota_storage *storage);

#ifdef CONFIG_FILE_SYSTEM

/**
 * bind fs to the the storage.
 * @param fs pointer to fs
 * @return 0.
 */

int ota_storage_bind_fs(struct fs_file_t *fs);

/**
 * unbind fs to the the storage.
 * @param fs pointer to fs
 * @return 0.
 */

int ota_storage_unbind_fs(struct fs_file_t *fs);
#endif

/**
 * @brief Flush cached write data buffers of an open file
 *
 * The function flushes the cache of an open file; it can be invoked to ensure
 * data gets written to the storage media immediately, e.g. to avoid data loss
 * @param storage pointer to storage
 * in case if power is removed unexpectedly.
 * @return 0 on success;
 */
int ota_storage_sync(struct ota_storage *storage);
#endif /* __OTA_STORAGE_H__ */
