/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA firmware image interface
 */

#include <kernel.h>
#include <string.h>
#include <soc.h>
#include <mem_manager.h>
#include <ota_backend.h>
#include <crc.h>
#include <ota_storage.h>
#include "ota_image.h"
#include <os_common_api.h>

#define OTA_FW_HDR_MAGIC	0x41544f41 //'AOTA'

struct ota_fw_hdr {
	uint32_t	magic;
	uint32_t	header_checksum;
	uint16_t	header_version;
	uint16_t	header_size;
	uint16_t	file_cnt;
	uint16_t	flag;
	uint16_t	dir_offset;
	uint16_t	data_offset;
	uint32_t	data_size;
	uint32_t	data_checksum;
	uint8_t	reserved[4];
} __attribute__((packed));

/* size 0x60 */
struct ota_fw_ver {
	uint8_t version_name[32];
	uint8_t board_name[32];
	uint8_t hardware_ver[4];
	uint32_t version_code;
	uint8_t reserved[8];
	uint8_t build_time[16];
} __attribute__((packed));

struct ota_dir_entry {
    uint8_t filename[12];
    uint8_t reserved1[4];
    uint32_t offset;
    uint32_t length;
    uint8_t reserved2[4];
    uint32_t checksum;
} __attribute__((packed));

struct ota_fw_dir {
	struct ota_dir_entry	entrys[16];
};

struct ota_fw_head {
	/* offset: 0x0 */
	struct ota_fw_hdr	hdr;

	/* offset: 0x20 */
	uint8_t			reserved1[32];

	/* offset: 0x40, new OTA firmware version */
	struct ota_fw_ver	new_ver;

	/* offset: 0xa0, old ota firmware version
	 * only used for ota patch firmware
	 */
	struct ota_fw_ver	old_ver;

	/* offset: 0x100 */
	uint8_t			reserved2[256];

	/* offset: 0x200, OTA file dir entries */
	struct ota_fw_dir	dir;
} __attribute__((packed));

struct ota_progress {
#define OTA_PROGRESS_UP_LIMIT (99)
	uint32_t total_size;
	uint32_t cur_size;
	uint8_t cursor;
	uint8_t on;
};

struct ota_image {
	struct ota_backend *backend;
	struct ota_fw_hdr hdr;
	struct ota_fw_dir dir;
	struct ota_dir_entry *cur_dir;
	//struct ota_fw_head *fw_head;
	struct ota_progress progress;
};

/* 16MB */
//#define OTA_IMAGE_MAX_LENGTH		0x3000000
#define OTA_IMAGE_DATA_CHECK_BUF_SIZE	0x800

static struct ota_dir_entry *ota_image_find_file(struct ota_image *img, const char *filename)
{
	struct ota_dir_entry *dir;
	int i;

	SYS_LOG_INF("find file %s", filename);

	if (img->cur_dir && (0 == strncmp(img->cur_dir->filename, filename, 12))) {
		dir = img->cur_dir;
	} else {
		for (i = 0; i < img->hdr.file_cnt; i++) {
			dir = &img->dir.entrys[i];

			if (0 == strncmp(dir->filename, filename, 12)) {
				SYS_LOG_INF("found file %s at [%d]", filename, i);
				return dir;
			}
		}
	}

	return NULL;
}

int ota_image_get_file_length(struct ota_image *img, const char *filename)
{
	struct ota_dir_entry *dir;

	if (filename == NULL) {
		SYS_LOG_INF("get image length 0x%x", img->hdr.data_size);
		return img->hdr.data_size;
	}

	dir = ota_image_find_file(img, filename);
	if (!dir) {
		SYS_LOG_ERR("cannot found file %s", filename);
		return -1;
	}

	return dir->length;
}

int ota_image_get_file_offset(struct ota_image *img, const char *filename)
{
	struct ota_dir_entry *dir;

	if (filename == NULL) {
		SYS_LOG_INF("get image offset 0");
		return 0;
	}

	dir = ota_image_find_file(img, filename);
	if (!dir) {
		SYS_LOG_ERR("cannot found file %s", filename);
		return -1;
	}

	return dir->offset;
}

int ota_image_ioctl(struct ota_image *img, int cmd, unsigned int param)
{
	SYS_LOG_DBG("cmd 0x%x param 0x%x", cmd, param);

	return ota_backend_ioctl(img->backend, cmd, param);
}

int ota_image_progress_on(struct ota_image *img, uint32_t total_size, uint32_t start_offset)
{
	if (img && !img->progress.on) {
		img->progress.total_size = total_size;
		img->progress.cur_size = start_offset;
		img->progress.cursor = 100 * start_offset / total_size;
		img->progress.on = 1;
		SYS_LOG_INF("OTA image progress total size %d", total_size);
		SYS_LOG_INF("OTA upgrade progress ==> %d%%", img->progress.cursor);
		return 0;
	}

	return -1;
}

int ota_image_progress_reset(struct ota_image *img)
{
	if (img) {
		img->progress.cur_size = 0;
		img->progress.cursor = 0;
	}

	return 0;
}

int ota_image_report_progress(struct ota_image *img, uint32_t pre_xfer_size, bool is_final)
{
	struct ota_progress *progress = &img->progress;
	uint32_t grade_size;

	if (img && progress->on) {
		if (progress->total_size < pre_xfer_size) {
			SYS_LOG_ERR("error total size %d xfer_size %d",
				progress->total_size, pre_xfer_size);
			return -1;
		}

		if ((pre_xfer_size + progress->cur_size) > progress->total_size) {
			SYS_LOG_ERR("error xfer_size %d cur_size %d total_size %d",
				pre_xfer_size, progress->cur_size, progress->total_size);
			return -1;
		}

		progress->cur_size += pre_xfer_size;
		grade_size = progress->total_size / 100;
		grade_size *= (progress->cursor + 1);

		if (!is_final) {
			if (progress->cur_size > grade_size) {
				progress->cursor = 100 * progress->cur_size / progress->total_size;
				if (progress->cursor > OTA_PROGRESS_UP_LIMIT) {
					ota_image_ioctl(img, OTA_BACKEND_IOCTL_REPORT_PROCESS, OTA_PROGRESS_UP_LIMIT);
					SYS_LOG_INF("OTA upgrade progress ==> %d%%", OTA_PROGRESS_UP_LIMIT);
				} else {
					ota_image_ioctl(img, OTA_BACKEND_IOCTL_REPORT_PROCESS, progress->cursor);
					SYS_LOG_INF("OTA upgrade progress ==> %d%%", img->progress.cursor);
				}
			}
		} else {
			ota_image_ioctl(img, OTA_BACKEND_IOCTL_REPORT_PROCESS, 100);
		}
	}

	return 0;
}

int ota_image_read_prepare(struct ota_image *img, int offset, uint8_t *buf, int size)
{
	int ret;
	SYS_LOG_DBG("offset 0x%x, buf %p, size %d", offset, buf, size);

	ret = ota_backend_read_prepare(img->backend, offset, buf, size);
	return ret;
}

int ota_image_read_complete(struct ota_image *img, int offset, uint8_t *buf, int size)
{
	int ret;
	SYS_LOG_DBG("offset 0x%x, buf %p, size %d", offset, buf, size);

	ret = ota_backend_read_complete(img->backend, offset, buf, size);
	ota_image_report_progress(img, size, false);
	return ret;
}

int ota_image_read(struct ota_image *img, int offset, uint8_t *buf, int size)
{
	int ret;
	SYS_LOG_DBG("offset 0x%x, buf %p, size %d", offset, buf, size);

	ret = ota_backend_read(img->backend, offset, buf, size);
	ota_image_report_progress(img, size, false);
	return ret;
}

int ota_image_check_file(struct ota_image *img, const char *filename, const uint8_t *buf, int size)
{
	struct ota_dir_entry *dir;
	uint32_t crc;

	dir = ota_image_find_file(img, filename);
	if (!dir) {
		SYS_LOG_ERR("cannot found file %s", filename);
		return -1;
	}

	crc = utils_crc32(0, buf, size);
	if (crc != dir->checksum) {
		SYS_LOG_INF("file %s checksum error", filename);
		return -1;
	}

	SYS_LOG_INF("file %s checksum pass", filename);

	return 0;
}

#if 1
static int ota_image_calc_crc(struct ota_image *img, uint32_t offset, int size)
{
	char *buf;
	int err, rlen;
	uint32_t crc, addr;

	SYS_LOG_INF("caculate image crc offset 0x%x size 0x%x", offset, size);

	buf = mem_malloc(OTA_IMAGE_DATA_CHECK_BUF_SIZE);
	if (!img) {
		SYS_LOG_ERR("malloc failed");
		return 0;
	}

	addr = offset;
	rlen = OTA_IMAGE_DATA_CHECK_BUF_SIZE;
	crc = 0;
	while (size > 0) {
		if (size < rlen)
			rlen = size;

		err = ota_image_read(img, addr, buf, rlen);
		if (err) {
			SYS_LOG_INF("read image err, addr 0x%x, rlen 0x%x return %d",
				addr, rlen, err);
			crc = 0;
			goto exit;
		}

		crc = utils_crc32(crc, buf, rlen);

		size -= rlen;
		addr += rlen;
	}

exit:
	mem_free(buf);

	return crc;
}
#endif

static int ota_image_check_head_crc(struct ota_fw_head *fw_head)
{
	uint32_t crc;

	/* skip magic & head crc self field */
	crc = utils_crc32(0, (const char *)fw_head + 8, sizeof(struct ota_fw_head) - 8);
	if (crc == -1 || crc != fw_head->hdr.header_checksum) {
		SYS_LOG_INF("image head crc error, calc crc 0x%x, head->crc 0x%x",
			crc, fw_head->hdr.header_checksum);
		return -1;
	}

	SYS_LOG_INF("image head check pass");

	return 0;
}

int ota_image_check_data(struct ota_image *img)
{
#if 1
	struct ota_fw_hdr *hdr = &img->hdr;
	uint32_t crc;

	crc = ota_image_calc_crc(img, hdr->data_offset, hdr->data_size - hdr->data_offset);
	if (crc == 0 || crc != hdr->data_checksum) {
		SYS_LOG_INF("image data crc error, calc crc 0x%x, head->data_checksum 0x%x",
			crc, hdr->data_checksum);
		return -1;
	}

	SYS_LOG_INF("image data check pass");
#endif
	return 0;
}

int ota_image_check(struct ota_image *img, struct ota_fw_head *fw_head)
{
	struct ota_fw_hdr *hdr = &fw_head->hdr;
	int err;

	SYS_LOG_INF("checking image");

	if (!img || !img->backend)
		return -EINVAL;

	if (hdr->magic != OTA_FW_HDR_MAGIC) {
		SYS_LOG_ERR("wrong magic 0x%x\n", hdr->magic);
		return -1;
	}

	if (hdr->header_size != sizeof(struct ota_fw_head)) {
		SYS_LOG_ERR("invalid header size %d", hdr->header_size);
		return -1;
	}

//	if (hdr->data_size > OTA_IMAGE_MAX_LENGTH) {
//		return -1;
//	}

	err = ota_image_check_head_crc(fw_head);
	if (err) {
		SYS_LOG_ERR("bad head crc");
		return -1;
	}

#if 0
	/* only check data in recovery app to save time */
	err = ota_image_check_data(img);
	if (err) {
		SYS_LOG_ERR("bad data crc");
		return -1;
	}
#endif

	SYS_LOG_INF("image check pass");

	return 0;
}

int ota_image_open(struct ota_image *img)
{
	struct ota_fw_head *fw_head;
	int err;

	SYS_LOG_INF("open type %d", img->backend->type);

	err = ota_backend_open(img->backend);
	if (err) {
		SYS_LOG_INF("backend open: err %d", err);
		return err;
	}

	SYS_LOG_INF("read image header");

	fw_head = mem_malloc(sizeof(struct ota_fw_head));
	if (!fw_head) {
		SYS_LOG_ERR("malloc image header failed");
		return -ENOMEM;
	}

	/* read image header */
	err = ota_image_read(img, 0, (uint8_t *)fw_head, sizeof(struct ota_fw_head));
	if (err) {
		SYS_LOG_INF("read head err, return %d", err);
		goto exit;
	}

	SYS_LOG_INF("image header:");
	//print_buffer(fw_head, 1, sizeof(struct ota_fw_hdr), 16, 0);

	/* check image */
	err = ota_image_check(img, fw_head);
	if (err) {
		SYS_LOG_INF("check: err %d", err);
		goto exit;
	}

	/* read image header */
	memcpy(&img->hdr, &fw_head->hdr, sizeof(struct ota_fw_hdr));
	memcpy(&img->dir, &fw_head->dir, sizeof(struct ota_fw_dir));

	mem_free(fw_head);

	return 0;

exit:
	mem_free(fw_head);
	ota_backend_close(img->backend);
	img->backend = NULL;

	return err;
}

int ota_image_close(struct ota_image *img)
{
	int err = 0;

	SYS_LOG_INF("close");

	if (img->backend) {
		// delay 1s to close bt for sending remain data
		//if (img->backend->type == OTA_BACKEND_TYPE_BLUETOOTH) {
		//	os_sleep(1000);
		//}
		err = ota_backend_close(img->backend);
		if (err) {
			SYS_LOG_INF("open: err %d", err);

		}

		//img->backend = NULL;
	}

	memset(&img->progress, 0, sizeof(struct ota_progress));

	return err;
}

int ota_image_bind(struct ota_image *img, struct ota_backend *backend)
{
	SYS_LOG_INF("bind backend %d", backend->type);

	img->backend = backend;

	return 0;
}

int ota_image_unbind(struct ota_image *img, struct ota_backend *backend)
{
	SYS_LOG_INF("unbind backend %d", backend->type);

	if (img->backend == backend) {
		img->backend = NULL;
	}

	return 0;
}

struct ota_backend *ota_image_get_backend(struct ota_image *img)
{
	return img->backend;
}

uint32_t ota_image_get_checksum(struct ota_image *img)
{
	return img->hdr.data_checksum;
}

#if defined(CONFIG_OTA_PRODUCT_SUPPORT) || defined(CONFIG_OTA_BLE_MASTER_SUPPORT)
uint32_t ota_image_get_datasize(struct ota_image *img)
{
	return img->hdr.data_size;
}
#endif

static struct ota_image global_ota_image;

struct ota_image *ota_image_init(void)
{
	struct ota_image *img;

	SYS_LOG_INF("init");

	img = &global_ota_image;

	memset(img, 0x0, sizeof(struct ota_image));

	return img;
}


void ota_image_exit(struct ota_image *img)
{
	SYS_LOG_INF("exit");

	if (img->backend && (img->backend->type == OTA_BACKEND_TYPE_CARD)) {
		ota_backend_exit(img->backend);
	}
	img->backend = NULL;
}
