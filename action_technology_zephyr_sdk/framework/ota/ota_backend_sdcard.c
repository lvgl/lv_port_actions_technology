/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA SDCARD backend interface
 */

#include <kernel.h>
#include <fs/fs.h>
#include <file_stream.h>
#include <string.h>
#include <mem_manager.h>
#include <fs_manager.h>
#include <ota_backend.h>
#include <ota_backend_sdcard.h>

#undef CONFIG_OTA_SDCARD_DYNAMIC_DETECT

struct ota_backend_sdcard {
	struct ota_backend backend;
	io_stream_t stream;
	int stream_opened;
	const char *fpath;
};

static int sdcard_file_is_exist(const char* f_path)
{
	struct fs_dirent *entry;
	int err;

	entry = (struct fs_dirent *)mem_malloc(sizeof(struct fs_dirent));
	if (!entry) {
		return 0;
	}

	err = fs_stat(f_path, entry);
	mem_free(entry);
	if (err) {
		SYS_LOG_INF("file %s not found ", f_path);
		return 0;
	}

	SYS_LOG_INF("file %s found", f_path);

	return 1;
}

int ota_backend_sdcard_ioctl(struct ota_backend *backend, int cmd, unsigned int param)
{
	SYS_LOG_INF("cmd 0x%x: param %d\n", cmd, param);

	switch (cmd) {
	case OTA_BACKEND_IOCTL_REPORT_PROCESS:
		backend->cb(backend, OTA_BACKEND_UPGRADE_PROGRESS, param);
		break;
	default:
		SYS_LOG_ERR("unknow cmd 0x%x", cmd);
		return -EINVAL;
	}

	return 0;
}

int ota_backend_sdcard_read(struct ota_backend *backend, int offset, unsigned char *buf, int size)
{
	struct ota_backend_sdcard *backend_sdcard = CONTAINER_OF(backend,
		struct ota_backend_sdcard, backend);
	int err;

	SYS_LOG_DBG("offset 0x%x, size %d, buf %p", offset, size, buf);

	err = stream_seek(backend_sdcard->stream, offset, SEEK_DIR_BEG);
	if (err < 0) {
		SYS_LOG_INF("seek err %d", err);
		return -EIO;
	}

	err = stream_read(backend_sdcard->stream, buf, size);
	if (err < 0) {
		SYS_LOG_INF("read data err %d", err);
		return -EIO;
	}

	return 0;
}

int ota_backend_sdcard_open(struct ota_backend *backend)
{
	struct ota_backend_sdcard *backend_sdcard = CONTAINER_OF(backend,
		struct ota_backend_sdcard, backend);
	int err;
	if (!fs_manager_get_volume_state(CONFIG_APP_FAT_DISK) && !sdcard_file_is_exist(backend_sdcard->fpath)) {
		SYS_LOG_ERR("cannot found ota file \'%s\' in sdcard", backend_sdcard->fpath);
		return -ENOENT;
	}

	backend_sdcard->stream = file_stream_create((char *)backend_sdcard->fpath);
	if (!backend_sdcard->stream) {
		SYS_LOG_ERR("stream create failed \n");
		return -EIO;
	}

	SYS_LOG_INF("create stream %p", backend_sdcard->stream);
	err = stream_open(backend_sdcard->stream, MODE_IN);
	if (err) {
		SYS_LOG_ERR("stream open failed \n");
		return err;
	}

	backend_sdcard->stream_opened = 1;

	SYS_LOG_INF("open stream %p", backend_sdcard->stream);

	return 0;
}

int ota_backend_sdcard_close(struct ota_backend *backend)
{
	struct ota_backend_sdcard *backend_sdcard = CONTAINER_OF(backend,
		struct ota_backend_sdcard, backend);
	int err;

	SYS_LOG_INF("close: type %d", backend->type);

	if (backend_sdcard->stream) {
		err = stream_close(backend_sdcard->stream);
		if (err) {
			SYS_LOG_ERR("stream_close Failed");
			return err;
		}
	}

	backend_sdcard->stream_opened = 0;

	return 0;
}

void ota_backend_sdcard_exit(struct ota_backend *backend)
{
	struct ota_backend_sdcard *backend_sdcard = CONTAINER_OF(backend,
		struct ota_backend_sdcard, backend);

	if (backend_sdcard->stream) {
		stream_destroy(backend_sdcard->stream);
	}

	mem_free(backend_sdcard);
}

const struct ota_backend_api ota_backend_api_sdcard = {
	.init = (void *)ota_backend_sdcard_init,
	.exit = ota_backend_sdcard_exit,
	.open = ota_backend_sdcard_open,
	.close = ota_backend_sdcard_close,
	.read = ota_backend_sdcard_read,
	.ioctl = ota_backend_sdcard_ioctl,
};

struct ota_backend *ota_backend_sdcard_init(ota_backend_notify_cb_t cb,
		struct ota_backend_sdcard_init_param *param)
{
	struct ota_backend_sdcard *backend_sdcard;

	SYS_LOG_INF("init");

	backend_sdcard = mem_malloc(sizeof(struct ota_backend_sdcard));
	if (!backend_sdcard) {
		SYS_LOG_ERR("malloc failed");
		return NULL;
	}

	memset(backend_sdcard, 0x0, sizeof(struct ota_backend_sdcard));

	backend_sdcard->fpath = param->fpath;

	ota_backend_init(&backend_sdcard->backend, OTA_BACKEND_TYPE_CARD,
			(struct ota_backend_api *) &ota_backend_api_sdcard, cb);

	if ((fs_manager_get_volume_state(CONFIG_APP_FAT_DISK) && sdcard_file_is_exist(backend_sdcard->fpath))) {
		SYS_LOG_INF("found ota file \'%s\' in sdcard", backend_sdcard->fpath);
		cb(&backend_sdcard->backend, OTA_BACKEND_UPGRADE_STATE, 1);
	}
#ifndef CONFIG_OTA_SDCARD_DYNAMIC_DETECT
	else {
		mem_free(&backend_sdcard->backend);
		return NULL;
	}
#endif

	return &backend_sdcard->backend;
}

