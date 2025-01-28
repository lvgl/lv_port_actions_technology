/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA upgrade interface
 */
#include <kernel.h>
#include <string.h>
#include <device.h>
#include <drivers/flash.h>
#include <soc.h>
#include <fw_version.h>
#include <partition/partition.h>
#include <mem_manager.h>
#include <crc.h>
#include <ota_upgrade.h>
#include <ota_backend.h>
#include <ota_trans.h>
#include "ota_image.h"
#include <ota_storage.h>
#include "ota_manifest.h"
#include "ota_breakpoint.h"
#include "ota_file_patch.h"
#include <os_common_api.h>
#include <drivers/nvram_config.h>
#include <sys/ring_buffer.h>

#define OTA_HOST_DATA_BUFFER_SIZE		(16*1024)//3072
	
#define OTA_MANIFESET_FILE_NAME		"ota.xml"
	
#define OTA_FLAG_USE_RECOVERY		(1 << 0)
#define OTA_FLAG_USE_RECOVERY_APP	(1 << 1)
	
#define ota_use_recovery(ota)		((ota)->flags & OTA_FLAG_USE_RECOVERY)
#define ota_use_recovery_app(ota)	((ota)->flags & OTA_FLAG_USE_RECOVERY_APP)
	
#define EIO_READ			(1001)
	
struct ota_host_upgrade_info {
	int state;
	int backend_type;
	unsigned int flags;

	ota_notify_t notify;

	struct ota_image *img;
	struct ota_trans *trans;

	int data_buf_size;
	u8_t *data_buf;

	struct ota_manifest manifest;
	struct ota_breakpoint bp;
};

struct ota_upgrade_image_info{
	u8_t *buffer;
	int size;
	u32_t crc;
	int ctn;
};

void ota_host_update_state(struct ota_host_upgrade_info *ota, enum ota_state state)
{
	int old_state;

	SYS_LOG_INF("upadte ota state:	%d\n", state);

	old_state = ota->state;

	ota->state = state;

	if (ota->notify) {
		ota->notify(state, old_state);
	}
}

static int ota_host_is_need_upgrade(struct ota_host_upgrade_info *ota)
{
#ifdef CONFIG_OTA_IMAGE_CHECK
	struct ota_breakpoint *bp = &ota->bp;
	const struct fw_version *img_ver;
	struct ota_backend *backend;
	int backend_type;

	img_ver = &ota->manifest.fw_ver;

	SYS_LOG_INF("ota fw version:");
	fw_version_dump(img_ver);

	backend = ota_image_get_backend(ota->img);
	backend_type = ota_backend_get_type(backend);

	if (backend_type != OTA_BACKEND_TYPE_TEMP_PART &&
		bp->backend_type != OTA_BACKEND_TYPE_UNKNOWN &&
		bp->backend_type != backend_type) {
		SYS_LOG_ERR("backend type is chagned(%d -> %d), need erase old firmware",
			bp->backend_type, ota_backend_get_type(backend));
		return -1;
	}
#endif

#if 0
	if (strcmp(cur_ver->board_name, img_ver->board_name)) {
		/* skip */
		SYS_LOG_ERR("unmatched board name, skip ota");
		return -1;
	}

	if ((bp->state == OTA_BP_STATE_WRITING_IMG ||
		 bp->state == OTA_BP_STATE_UPGRADE_WRITING ||
		 bp->state == OTA_BP_STATE_UPGRADE_PENDING) &&
		 (bp->new_version != 0 &&
		 bp->new_version != img_ver->version_code)) {
		/* FIXME: has new version fw, need erase partition */
		SYS_LOG_INF("has new version fw, need erase old firmware");
		return -1;
	}

	if (cur_ver->version_code >= img_ver->version_code) {
		/* skip */
		SYS_LOG_INF("ota image is same or older, skip ota");
		return 0;
	}
#endif

	return 0;
}

int ota_host_upgrade_check(struct ota_host_upgrade_info *ota)
{
//	struct ota_breakpoint *bp = &ota->bp;
//	struct ota_backend *backend;
	int err = 0;
	//int need_upgrade;

	SYS_LOG_INF("handle upgrade");

	if (ota->state != OTA_INIT) {
		SYS_LOG_ERR("ota state <%d> is not OTA_INIT, skip upgrade", ota->state);
		return -EINVAL;
	}

	err = ota_image_open(ota->img);
	if (err) {
		SYS_LOG_INF("ota image open failed");
		return -EIO;
	}

	err = ota_manifest_parse_file(&ota->manifest, ota->img, OTA_MANIFESET_FILE_NAME);
	if (err) {
		SYS_LOG_INF("cannot get manifest file in image");
		return -1;
//		goto exit;
	}

	err = ota_host_is_need_upgrade(ota);

	ota_host_update_state(ota, OTA_RUNNING);

	return err;
#if 0
	/* need upgrade? */
	need_upgrade = ota_is_need_upgrade(ota);
	if (need_upgrade == 0) {
		SYS_LOG_INF("skip upgrade");
		goto exit;
	} else if (need_upgrade < 0) {
		SYS_LOG_INF("new firmware is not same to the upgrading firmware, need erase at next boot");
		ota_breakpoint_update_state(bp, OTA_BP_STATE_UPGRADING_FAIL);
		ota_update_state(ota, OTA_FAIL);
		err = -1;
		goto exit;
	}

	SYS_LOG_INF("burn firmware image");

	backend = ota_image_get_backend(ota->img);

	/* update breakpoint for new firmware */
	bp->backend_type = ota_backend_get_type(backend);
	bp->new_version = ota->manifest.fw_ver.version_code;

	ota->data_buf = mem_malloc(ota->data_buf_size);
	if (!ota->data_buf) {
		SYS_LOG_ERR("faield to allocate %d bytes", ota->data_buf_size);
		return -ENOMEM;
	}

	ota_update_state(ota, OTA_RUNNING);

	if (!ota_use_recovery(ota) || ota_use_recovery_app(ota)) {
		ota_breakpoint_update_state(bp, OTA_BP_STATE_UPGRADE_WRITING);
		err = ota_do_upgrade(ota);
		if (err) {
			SYS_LOG_INF("upgrade failed, err %d", err);
			if (err != -EIO && err != -EAGAIN) {
				ota_breakpoint_update_state(bp, OTA_BP_STATE_UPGRADING_FAIL);
			}

			goto exit;
		}

		ota_breakpoint_update_state(bp, OTA_BP_STATE_UPGRADE_DONE);
	} else {
		ota_breakpoint_update_state(bp, OTA_BP_STATE_WRITING_IMG);
		err = ota_write_temp_img(ota);
		if (err) {
			SYS_LOG_INF("write ota image failed, err %d", err);
			if (err != -EIO && err != -EAGAIN) {
				ota_breakpoint_update_state(bp, OTA_BP_STATE_WRITING_IMG_FAIL);
			}

			goto exit;
		}

		ota_breakpoint_update_state(bp, OTA_BP_STATE_UPGRADE_PENDING);
	}

	SYS_LOG_INF("upgrade successfully!");

	ota_image_ioctl(ota->img, OTA_BACKEND_IOCTL_REPORT_IMAGE_VALID, 1);
	ota_update_state(ota, OTA_DONE);

exit:
	if (err) {
		ota_image_ioctl(ota->img, OTA_BACKEND_IOCTL_REPORT_IMAGE_VALID, 0);
		ota_update_state(ota, OTA_FAIL);
		if (err == -EAGAIN) {
			/* wait for upgrade resume */
			SYS_LOG_INF("ota status -> OTA_INIT, wait for upgrading resume!");
			ota_update_state(ota, OTA_INIT);
		}
	}

	if (ota->data_buf) {
		mem_free(ota->data_buf);
		ota->data_buf = NULL;
	}

	ota_image_close(ota->img);
#endif
	return err;
}

int ota_host_upgrade_attach_trans(struct ota_host_upgrade_info *ota, struct ota_trans *trans)
{
	ota->trans = trans;
	return 0;
}

int ota_host_upgrade_attach_backend(struct ota_host_upgrade_info *ota, struct ota_backend *backend)
{
	struct ota_backend *img_backend = ota_image_get_backend(ota->img);

	SYS_LOG_INF("attach backend type %d", backend->type);

	if (img_backend != NULL) {
		SYS_LOG_ERR("already attached backend %d", img_backend->type);
		return -EBUSY;
	}

	ota_image_bind(ota->img, backend);
	return 0;
}

void ota_host_upgrade_detach_backend(struct ota_host_upgrade_info *ota, struct ota_backend *backend)
{
	struct ota_backend *img_backend = ota_image_get_backend(ota->img);

	SYS_LOG_INF("detach backend %p", backend);

	if (img_backend == backend)
		ota_image_unbind(ota->img, backend);
}

int ota_host_upgrade_is_in_progress(struct ota_host_upgrade_info *ota)
{
	struct ota_breakpoint *bp = &ota->bp;
	int bp_state;

	bp_state = ota_breakpoint_get_current_state(bp);
	switch (bp_state) {
	case OTA_BP_STATE_UPGRADE_PENDING:
	case OTA_BP_STATE_UPGRADE_WRITING:
	case OTA_BP_STATE_UPGRADE_DONE:
		return 1;
	default:
		break;
	}

	return 0;
}

u32_t ota_host_upgrade_get_image_size(struct ota_host_upgrade_info *ota)
{
	struct ota_image *img = ota->img;
	return ota_image_get_datasize(img);
}

u32_t ota_host_upgrade_get_head_crc(struct ota_host_upgrade_info *ota)
{
	struct ota_image *img = ota->img;
	return ota_image_get_checksum(img);
}

u32_t ota_host_upgrade_get_image_ver(struct ota_host_upgrade_info *ota)
{
	//struct ota_image *img = ota->img;
	//return img->hdr.header_version;
	return 0;
}

int ota_host_upgrade_send_image(struct ota_host_upgrade_info *ota,int offset,int len)
{
	struct ota_image *img = ota->img;
	int err = 0, send_len = 0, res_len = len;
	struct ota_upgrade_image_info image_info;
	int unit_size, max_size;
//	u32_t crc;
	SYS_LOG_INF("state %d",ota->state);

	memset(&image_info, 0 , sizeof(struct ota_upgrade_image_info));
	unit_size = ota_trans_ioctl(ota->trans, OTA_TRANS_IOCTL_UNITSIZE_GET, NULL);
	if (0 == unit_size) {
		SYS_LOG_ERR("unit_size %d.",unit_size);
		return -EAGAIN;
	} else {
		max_size = (OTA_HOST_DATA_BUFFER_SIZE/unit_size*unit_size);
	}

	do {
		if(ota->state != OTA_RUNNING) {
			SYS_LOG_INF("error state %d",ota->state);
			return -EPERM;		
		}

		if(res_len > max_size) {
			send_len = max_size;
		} else {
			send_len = res_len;
		}
	
		err = ota_image_read(img, offset, ota->data_buf, send_len);
		if (err) {
			SYS_LOG_ERR("cannot read data, offs 0x%x", offset);
			return -EAGAIN;
		}
	
		image_info.buffer = ota->data_buf;
		image_info.size = send_len;
	//	image_info.crc = crc;
		SYS_LOG_INF("send_len %d",send_len);
		err = ota_trans_ioctl(ota->trans,OTA_TRANS_IOCTL_SEND_IMAGE,(void *)&image_info);
		if (err) {
			SYS_LOG_ERR("send data error 0x%x", offset);
			return err;
		}

		offset += send_len;
		res_len -= send_len;
		image_info.ctn = 1;
		//os_sleep(2);
	} while(res_len > 0);

	return err;
	
}

struct ota_host_upgrade_info *ota_host_upgrade_init(struct ota_upgrade_param *param)
{
	struct ota_host_upgrade_info *ota;

	SYS_LOG_INF("init");

	ota = mem_malloc(sizeof(struct ota_host_upgrade_info));
	if (!ota)
		return NULL;

	memset(ota, 0x0, sizeof(struct ota_host_upgrade_info));

	/* allocate data buffer later */
	
	ota->data_buf_size = OTA_HOST_DATA_BUFFER_SIZE;

	ota->data_buf = mem_malloc(ota->data_buf_size);

//	ota_breakpoint_init(&ota->bp);

//	ota_partition_update_prepare(ota);

	ota->img = ota_image_init();
	if (!ota->img) {
		SYS_LOG_ERR("image init failed");
		return NULL;
	}

	ota->notify = param->notify;

	ota->trans = NULL;

	ota_host_update_state(ota, OTA_INIT);

	return ota;
}

void ota_host_upgrade_exit(struct ota_host_upgrade_info *ota)
{
	SYS_LOG_INF("exit");

	/* TODO */

	if (ota) {
			
		if (ota->img){
			ota_image_exit(ota->img);
		}

		if(ota->data_buf){
			mem_free(ota->data_buf);
		}

		mem_free(ota);
	}
}
