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
#include "ota_image.h"
#include <ota_storage.h>
#include "ota_manifest.h"
#include "ota_breakpoint.h"
#include "ota_file_patch.h"
#include <os_common_api.h>
#include <drivers/nvram_config.h>
#include <sys/ring_buffer.h>
#include <ui_mem.h>
#ifdef CONFIG_OTA_LZMA
#include <minlzma.h>
#endif
#ifdef CONFIG_BT_MANAGER
#include <bt_manager.h>
#endif

// full upgrade config (mbrec/param/recovery)
// must set CONFIG_OTA_MUTIPLE_STORAGE=y for full upgrade
#define OTA_FULL_UPGRADE			(1)

#define OTA_REQ_MAX_SIZE			(256*1024)

#define OTA_RX_STACKSIZE			(1536)
#ifdef CONFIG_UI_MEMORY_MANAGER
#define OTA_RX_BUFSIZE				(64*1024)
#define OTA_IN_BUFSIZE				(32*1024)
#define OTA_OUT_BUFSIZE				(32*1024)
#else
#define OTA_RX_BUFSIZE				(8*1024)
#define OTA_IN_BUFSIZE				(4*1024)
#define OTA_OUT_BUFSIZE				(0)
#endif

#define OTA_MANIFESET_FILE_NAME		"ota.xml"

#define OTA_FLAG_USE_RECOVERY		(1 << 0)
#define OTA_FLAG_USE_RECOVERY_APP	(1 << 1)
#define OTA_FLAG_USE_NO_VERSION_CONTROL	(1 << 2)
#define OTA_FLAG_ERASE_PART_FOR_UPG	(1 << 3)
#define OTA_FLAG_KEEP_TEMP_PART		(1 << 4)

#define ota_use_recovery(ota)		((ota)->flags & OTA_FLAG_USE_RECOVERY)
#define ota_use_recovery_app(ota)	((ota)->flags & OTA_FLAG_USE_RECOVERY_APP)
#define ota_use_no_version_control(ota)	((ota)->flags & OTA_FLAG_USE_NO_VERSION_CONTROL)
#define ota_erase_part_for_upg(ota)	((ota)->flags & OTA_FLAG_ERASE_PART_FOR_UPG)
#define ota_keep_temp_part(ota)		((ota)->flags & OTA_FLAG_KEEP_TEMP_PART)

#define EIO_READ			(1001)

struct ota_rx_info {
	char *rx_stack;
	uint8_t *rx_buf;
	uint32_t rx_bufsize;
	uint8_t *in_buf;
	uint32_t in_bufsize;
	uint8_t *out_buf;
	uint32_t out_bufsize;

	os_sem rx_get_sem;
	os_sem rx_put_sem;
	struct ring_buf rbuf;
	os_tid_t rx_tid;

	uint32_t offset;
	uint32_t size;
	uint32_t seg_size;
	bool is_raw;
	int rx_errno;
};

struct ota_upgrade_info {
	int state;
	int backend_type;
	unsigned int flags;

	ota_notify_t notify;
	ota_ugrade_file_cb file_cb;

	struct ota_image *img;
	struct ota_storage *storage;
#ifdef CONFIG_OTA_MUTIPLE_STORAGE
	struct ota_storage *storage_ext;
#endif
	int data_buf_size;
	uint8_t *data_buf;
	uint32_t xml_offset;
	struct ota_manifest manifest;
	struct ota_breakpoint bp;
	struct ota_rx_info rx_info;
};

#ifdef CONFIG_UI_MEMORY_MANAGER
#define ota_rx_malloc		ui_mem_res_alloc
#define ota_rx_free			ui_mem_res_free
#else
#define ota_rx_malloc		mem_malloc
#define ota_rx_free			mem_free
#endif

static int ota_update_fw_version(struct ota_upgrade_info *ota, uint8_t file_id)
{
	struct code_res_version code_res;
	bool need_save = false;

	if (file_id == PARTITION_FILE_ID_SDFS_PART_BASE ||
		file_id == PARTITION_FILE_ID_SDFS_PART1 ||
		file_id == PARTITION_FILE_ID_SDFS_PART2) {
		code_res.version_code = fw_version_get_code();
		code_res.version_res = ota->manifest.fw_ver.version_res;
		need_save = true;
	} else if (file_id == PARTITION_FILE_ID_SYSTEM) {
		code_res.version_res = fw_version_get_res();
		code_res.version_code = ota->manifest.fw_ver.version_code;
		need_save = true;
	}
	if (need_save) {
		if (nvram_config_set_factory(FIRMWARE_VERSION, &code_res, sizeof(struct code_res_version))) {
			SYS_LOG_ERR("SAVE FIRMWARE_VERSION FAILD\n");
			return -1;
		}
	}
	return 0;
}
static int ota_save_res_version(struct ota_upgrade_info *ota)
{
	struct ota_manifest *manifest = &ota->manifest;
	struct ota_file *file;

	for (int i = 0; i < manifest->file_cnt; i++) {
		file = &manifest->wfiles[i];
		/*sdfs update need to save res version*/
		if (file->file_id == PARTITION_FILE_ID_SDFS_PART_BASE ||
			file->file_id == PARTITION_FILE_ID_SDFS_PART1 ||
			file->file_id == PARTITION_FILE_ID_SDFS_PART2) {
			/*update res version*/
			ota_update_fw_version(ota, file->file_id);
			break;
		}

	}
	return 0;

}
static void ota_upgrade_file_cb(struct ota_upgrade_info *ota, uint8_t file_id)
{
	if (ota->file_cb) {
		ota->file_cb(file_id);
	}

}
static void ota_update_state(struct ota_upgrade_info *ota, enum ota_state state)
{
    int old_state;

	SYS_LOG_INF("upadte ota state:  %d\n", state);

	old_state = ota->state;

	ota->state = state;

	if (ota->notify) {
		ota->notify(state, old_state);
	}
}

static int ota_partition_erase_part(struct ota_upgrade_info *ota,
			     const struct partition_entry *part,
			     int start_offset)
{
	int err, align_addr, align_size, is_clean;
	struct ota_storage *storage = ota->storage;

#ifdef CONFIG_OTA_MUTIPLE_STORAGE
	storage = ota_storage_find(part->storage_id);
	if(storage == NULL) {
		SYS_LOG_INF("storage not init, update failed\n");
		return -EINVAL;
	}
#endif

	SYS_LOG_INF("erase part %s: offset 0x%x size 0x%x, start_offset 0x%x\n",
		part->name, part->offset, part->size, start_offset);

	align_addr = ROUND_DOWN(part->offset + start_offset, OTA_ERASE_ALIGN_SIZE);
	align_size = ROUND_UP(part->size - start_offset, OTA_ERASE_ALIGN_SIZE);

	SYS_LOG_INF("erase aligned offset 0x%x, size 0x%x\n",
		align_addr, align_size);
	if (ota->data_buf && ota->data_buf_size) {
		is_clean = ota_storage_is_clean(storage, align_addr, align_size, ota->data_buf, ota->data_buf_size);
		if (is_clean == 1) {
			SYS_LOG_INF("part is clean\n");
			return 0;
		}
	}
	err = ota_storage_erase(storage, align_addr, align_size);
	if (err) {
		return err;
	}
	return 0;
}

static int ota_partition_update_prepare(struct ota_upgrade_info *ota)
{
	struct ota_breakpoint *bp = &ota->bp;
	const struct partition_entry *part;
	int i, file_state, erase_offset;

	SYS_LOG_INF("bp->state %d", bp->state);

	if (bp->state == OTA_BP_STATE_CLEAN) {
		/* state is clean, skip erase */
		SYS_LOG_INF("bp state is clean, skip erase parts");
		return 0;
	}

	if (ota_use_recovery(ota)) {
		if (bp->state == OTA_BP_STATE_UPGRADE_PENDING) {
			/* state is clean, skip erase */
			SYS_LOG_INF("upgrade pending, skip erase parts");
			return 0;
		}

		/* don't erase temp part is upgrading is going, it will be erased before write file */
		if (bp->state == OTA_BP_STATE_UPGRADE_WRITING ||
		    bp->state == OTA_BP_STATE_UPGRADING_FAIL) {
			SYS_LOG_INF("upgrade is in process, skip erase temp part");
			return 0;
		}
	}

	for (i = 0; i < MAX_PARTITION_COUNT; i++) {
		part = partition_get_part_by_id(i);
		if (part == NULL)
			return -EINVAL;

		if (part->file_id == 0)
			continue;

		if (!ota_use_recovery(ota)) {
			/* skip current firmware's partitions */
			if (!partition_is_mirror_part(part)) {
				SYS_LOG_INF("part[%d]: skip current used partition", i);
				continue;
			}
		} else {
			/* only temp partition need be erased when recovery is enabled */
			if (part->type != PARTITION_TYPE_TEMP &&
			    part->file_id != PARTITION_FILE_ID_OTA_TEMP) {
				SYS_LOG_DBG("part[%d]: file_id %d not ota temp partition, skip erase",
					i, part->file_id);
				continue;
			}

			/* don't erase partition that not in current storage */
			if (part->storage_id != ota_storage_get_storage_id(ota->storage)) {
				SYS_LOG_INF("part[%d]: part file_id %d storage_id %d not current storage_id, skip erase",
					i, part->file_id, part->storage_id);
				continue;
			}
		}

		if (bp->state == OTA_BP_STATE_UPGRADE_WRITING ||
		    bp->state == OTA_BP_STATE_WRITING_IMG) {
			/* check breakpoint */
			file_state = ota_breakpoint_get_file_state(bp, part->file_id);

			SYS_LOG_INF("bp->state %d file_state %d", bp->state, file_state);

			if (file_state == OTA_BP_FILE_STATE_CLEAN ||
			    file_state == OTA_BP_FILE_STATE_WRITE_DONE ||
			    file_state == OTA_BP_FILE_STATE_VERIFY_PASS ||
			    file_state == OTA_BP_FILE_STATE_WRITING_CLEAN) {
				SYS_LOG_INF("part[%d]: file_id %d file_state %d, skip erase",
					i, part->file_id, file_state);

				/* skip erase this partition */
				continue;
			} else if (file_state == OTA_BP_FILE_STATE_WRITING) {
				if (ota_use_recovery(ota) ||
					(bp->mirror_id == partition_get_current_mirror_id())) {
					/* parition is writing, not clean */
					erase_offset = bp->cur_file.offset + bp->cur_orig_write_offset;
					erase_offset &= ~(OTA_ERASE_ALIGN_SIZE - 1);
					erase_offset -= part->offset;

					SYS_LOG_INF("part[%d]: file_id %d writing",
						i, part->file_id);
					SYS_LOG_INF("file offset 0x%x, write_offset 0x%x, need erase from 0x%x",
						bp->cur_file.offset, bp->cur_orig_write_offset, erase_offset);

					/* update write offset aligned with erase sector */
					bp->cur_orig_write_offset = part->offset + erase_offset - bp->cur_file.offset;

					ota_partition_erase_part(ota, part, erase_offset);
					ota_breakpoint_set_file_state(bp, part->file_id, OTA_BP_FILE_STATE_WRITING_CLEAN);
					continue;
				}
			}
		}

		if (!ota_keep_temp_part(ota)) {
			ota_partition_erase_part(ota, part, 0);
			ota_breakpoint_set_file_state(bp, part->file_id, OTA_BP_FILE_STATE_CLEAN);
		}
	}

	if (bp->state != OTA_BP_STATE_UPGRADE_WRITING &&
	    bp->state != OTA_BP_STATE_WRITING_IMG) {
		if (bp->state != OTA_BP_STATE_UNKOWN) {
			/* clear all old status */
			SYS_LOG_INF("clear old bp status");
			ota_breakpoint_init_default_value(&ota->bp);
		}

		bp->state = OTA_BP_STATE_CLEAN;
		SYS_LOG_INF("bp state is clean");
	}

	ota_breakpoint_save(bp);

	return 0;
}

static int ota_caculate_storage_file_crc(struct ota_upgrade_info *ota, struct ota_file *file)
{
	struct ota_storage *storage = ota->storage;
	int addr, size, rlen;
	uint32_t crc;

	crc = 0;
	size = file->orig_size;
	addr = file->offset;

#ifdef CONFIG_OTA_MUTIPLE_STORAGE
	storage = ota_storage_find(file->storage_id);
#endif
	SYS_LOG_INF("check file %s: addr 0x%x, size 0x%x",
		file->name, addr, size);

	rlen = ota->data_buf_size;
	while (size > 0) {
		if (size < rlen)
			rlen = size;

		ota_storage_read(storage, addr, ota->data_buf, rlen);

		crc = utils_crc32(crc, ota->data_buf, rlen);

		size -= rlen;
		addr += rlen;
	}

	return crc;
}

static int ota_verify_file(struct ota_upgrade_info *ota, struct ota_file *file)
{
	uint32_t crc_calc, crc_orig;

	// FIXME
	//if (file->file_id != PARTITION_FILE_ID_OTA_TEMP)
	//{
		crc_calc = ota_caculate_storage_file_crc(ota, file);
		crc_orig = file->checksum;

		SYS_LOG_INF("check file %s: crc_orig 0x%x, crc_calc 0x%x",
			file->name, crc_orig, crc_calc);

		if (crc_calc != crc_orig) {
			return -1;
		}
	//}

	return 0;
}
#if 0
//#ifdef CONFIG_OTA_RES_PATCH
static int ota_is_patch_res(struct ota_upgrade_info *ota)
{
	const struct fw_version *old_fw_ver = &ota->manifest.old_fw_ver;

	return (old_fw_ver->version_res == 0) ? 0 : 1;
}
#endif

#ifdef CONFIG_OTA_FILE_PATCH
static int ota_is_patch_fw(struct ota_upgrade_info *ota)
{
	const struct fw_version *old_fw_ver = &ota->manifest.old_fw_ver;

	return (old_fw_ver->version_code == 0) ? 0 : 1;
}

static int ota_write_file_by_patch(struct ota_upgrade_info *ota, struct ota_file *file, int start_file_offs)
{
	struct ota_image *img = ota->img;
	struct ota_storage *storage = ota->storage;
	unsigned int img_file_offset;
	int err, is_clean, patch_file_size;
	uint32_t start_time, consume_time;
	struct ota_file_patch_info file_patch;
	const struct partition_entry *part;
	void *mapping_addr;

	SYS_LOG_INF("write file %s by patch to offset 0x%x", file->name, file->offset);

	start_time = k_uptime_get_32();

	if (start_file_offs != 0) {
		SYS_LOG_ERR("cannot support breakpoint for file patch by now");
		return -EINVAL;
	}

	img_file_offset = ota_image_get_file_offset(img, file->name);
	if (img_file_offset < 0) {
		SYS_LOG_ERR("cannot found file %s in image", file->name);
		return -EINVAL;
	}

	patch_file_size = ota_image_get_file_length(img, file->name);

	part = partition_get_part(file->file_id);
	if (part == NULL)
		return -EINVAL;

	/* check empty */
	os_printk("file->size 0x%x, ota->data_buf %p, data_buf_size 0x%x, part->flag 0x%x\n",
		file->size, ota->data_buf, ota->data_buf_size, part->flag);
	is_clean = ota_storage_is_clean(storage, file->offset, file->size,
		ota->data_buf, ota->data_buf_size);
	if (is_clean != 1) {
		SYS_LOG_ERR("storage is not clean, offs 0x%x size 0x%x", file->offset, file->size);
		return -EINVAL;
	}

	ota_breakpoint_update_file_state(&ota->bp, file, OTA_BP_FILE_STATE_WRITING_DIRTY, 0, 0, 0);

	mapping_addr = soc_memctrl_create_temp_mapping(part->file_offset, part->flag & PARTITION_FLAG_ENABLE_CRC);

	memset(&file_patch, 0x0, sizeof(struct ota_file_patch_info));

	file_patch.img = img;
	file_patch.storage = storage;
	file_patch.old_file_mapping_addr = mapping_addr;
	file_patch.old_file_offset = part->file_offset;
	file_patch.old_file_size = part->size;
	file_patch.new_file_offset = file->offset; // + start_file_offs;
	file_patch.new_file_size = file->size;
	file_patch.patch_file_offset = img_file_offset;
	file_patch.patch_file_size = patch_file_size;
	file_patch.flag_use_crc = (part->flag & PARTITION_FLAG_ENABLE_CRC) ? 1 : 0;
	file_patch.flag_use_encrypt = (part->flag & PARTITION_FLAG_ENABLE_ENCRYPTION) ? 1 : 0;

	file_patch.write_cache = ota->data_buf;
	file_patch.write_cache_size = 0x22;
	file_patch.write_cache_offs = 0;
	file_patch.write_cache_pos = 0;

	err = ota_file_patch_write(&file_patch);
	if (err) {
		SYS_LOG_ERR("storage write failed, offs 0x%x size 0x%x", file->offset, file->size);
		return -EIO;
	}

	consume_time = k_uptime_get_32() - start_time + 1;
	SYS_LOG_INF("write file %s: length %d KB patch size(%d KB), consume %d ms, %d KB/s\n",
		file->name, file->size / 1024, patch_file_size / 1024,
		consume_time, file->size / consume_time);

	soc_memctrl_clear_temp_mapping(mapping_addr);

	return 0;
}
#endif

static void ota_rx_thread(void *p1, void *p2, void *p3)
{
	struct ota_upgrade_info *ota = (struct ota_upgrade_info *)p1;
	struct ota_backend *backend = ota_image_get_backend(ota->img);
	struct ota_rx_info *rx_info = &ota->rx_info;
	bool is_bt_backend;
	int err, rlen, req_size, max_req_size;

	SYS_LOG_INF("ota_rx thread started");

	is_bt_backend = (ota_backend_get_type(backend) == OTA_BACKEND_TYPE_BLUETOOTH);
	max_req_size = OTA_REQ_MAX_SIZE;
	ota_backend_ioctl(backend, OTA_BACKEND_IOCTL_GET_MAX_SIZE, (unsigned int)&max_req_size);

	while ((rx_info->size > 0) && (rx_info->rx_errno == 0)) {
		if (rx_info->is_raw) {  //raw file
			req_size = rx_info->size;
		} else {  //lzma file
			while ((req_size = ring_buf_space_get(&rx_info->rbuf)) == 0) {
				SYS_LOG_INF("ota_rx wait rbuf");
				os_sem_take(&rx_info->rx_put_sem, OS_FOREVER);
			}
			if (req_size > rx_info->size) {
				req_size = rx_info->size;
			}
		}
		if (req_size > max_req_size) {
			req_size = max_req_size;
		}

		if (is_bt_backend) {
			err = ota_image_read_prepare(ota->img, rx_info->offset, ota->data_buf, req_size);
			if (err) {
				SYS_LOG_ERR("cannot read data, offs 0x%x", rx_info->offset);
				rx_info->rx_errno = -EAGAIN;
			}
		}

		while (req_size > 0) {
			if (req_size < rx_info->seg_size) {
				rlen = req_size;
			} else {
				rlen = rx_info->seg_size;
			}

			if (is_bt_backend) {
				err = ota_image_read_complete(ota->img, rx_info->offset, ota->data_buf, rlen);
			} else {
				err = ota_image_read(ota->img, rx_info->offset, ota->data_buf, rlen);
			}
			if (err) {
				SYS_LOG_ERR("cannot read data, offs 0x%x", rx_info->offset);
				rx_info->rx_errno = -EAGAIN;
				break;
			}

			while (ring_buf_space_get(&rx_info->rbuf) < rlen) {
				SYS_LOG_INF("ota_rx wait rbuf");
				os_sem_take(&rx_info->rx_put_sem, OS_FOREVER);
			}

			ring_buf_put(&rx_info->rbuf, (const uint8_t *)ota->data_buf, rlen);
			os_sem_give(&rx_info->rx_get_sem);

			req_size -= rlen;
			rx_info->offset += rlen;
			rx_info->size -= rlen;
		}
	}

	SYS_LOG_INF("ota_rx thread exited");
	os_sem_give(&rx_info->rx_get_sem);
}

static int ota_rx_init(struct ota_upgrade_info *ota)
{
	struct ota_rx_info *rx_info = &ota->rx_info;

	rx_info->rx_stack = mem_malloc(OTA_RX_STACKSIZE);
	if (rx_info->rx_stack == NULL) {
		SYS_LOG_ERR("failed to allocate %d bytes", OTA_RX_STACKSIZE);
		return -EINVAL;
	}

	rx_info->rx_buf = ota_rx_malloc(OTA_RX_BUFSIZE);
	if (rx_info->rx_buf == NULL) {
		SYS_LOG_ERR("failed to allocate %d bytes", OTA_RX_BUFSIZE);
		return -EINVAL;
	}
	rx_info->rx_bufsize = OTA_RX_BUFSIZE;

	rx_info->in_buf = ota_rx_malloc(OTA_IN_BUFSIZE);
	if (rx_info->in_buf == NULL) {
		SYS_LOG_ERR("failed to allocate %d bytes", OTA_IN_BUFSIZE);
		return -EINVAL;
	}
	rx_info->in_bufsize = OTA_IN_BUFSIZE;

#if OTA_OUT_BUFSIZE > 0
	rx_info->out_buf = ota_rx_malloc(OTA_OUT_BUFSIZE);
	if (rx_info->out_buf != NULL) {
		rx_info->out_bufsize = OTA_OUT_BUFSIZE;
	} else {
		SYS_LOG_ERR("failed to allocate %d bytes", OTA_OUT_BUFSIZE);
	}
#endif
	os_sem_init(&rx_info->rx_get_sem, 0, 5);
	os_sem_init(&rx_info->rx_put_sem, 0, 1);
	ring_buf_init(&rx_info->rbuf, rx_info->rx_bufsize, rx_info->rx_buf);

	return 0;
}

static int ota_rx_exit(struct ota_upgrade_info *ota)
{
	struct ota_rx_info *rx_info = &ota->rx_info;

	if (rx_info->rx_stack != NULL) {
		mem_free(rx_info->rx_stack);
		rx_info->rx_stack = NULL;
	}
	if (rx_info->rx_buf != NULL) {
		ota_rx_free(rx_info->rx_buf);
		rx_info->rx_buf = NULL;
		rx_info->rx_bufsize = 0;
	}
	if (rx_info->in_buf != NULL) {
		ota_rx_free(rx_info->in_buf);
		rx_info->in_buf = NULL;
		rx_info->in_bufsize = 0;
	}
	if (rx_info->out_buf != NULL) {
		ota_rx_free(rx_info->out_buf);
		rx_info->out_buf = NULL;
		rx_info->out_bufsize = 0;
	}

	return 0;
}

static void ota_rx_start(struct ota_upgrade_info *ota, uint32_t offset,
				uint32_t size, uint32_t seg_size, bool is_raw)
{
	struct ota_rx_info *rx_info = &ota->rx_info;
	char *stack_ptr;

	rx_info->offset = offset;
	rx_info->size = size;
	rx_info->seg_size = seg_size;
	rx_info->is_raw = is_raw;
	rx_info->rx_errno = 0;

	os_sem_reset(&rx_info->rx_get_sem);
	os_sem_reset(&rx_info->rx_put_sem);
	ring_buf_reset(&rx_info->rbuf);

	stack_ptr = (char *)ROUND_UP(rx_info->rx_stack, ARCH_STACK_PTR_ALIGN);

	rx_info->rx_tid = (os_tid_t)os_thread_create(stack_ptr, OTA_RX_STACKSIZE, ota_rx_thread,
									ota, NULL, NULL, 3, 0, OS_NO_WAIT);
	os_thread_name_set(rx_info->rx_tid, "ota_rx");
}

static void ota_rx_stop(struct ota_upgrade_info *ota)
{
	struct ota_rx_info *rx_info = &ota->rx_info;

	k_thread_join(rx_info->rx_tid, K_MSEC(5000));
}

static int ota_write_file_normal(struct ota_upgrade_info *ota, struct ota_file *file,
				const struct partition_entry *part,
				int start_file_offs, int start_orig_offs)
{
	struct ota_image *img = ota->img;
	struct ota_storage *storage = ota->storage;
	struct ota_breakpoint *bp = &ota->bp;
	struct ota_rx_info *rx_info = &ota->rx_info;
	struct ota_backend *backend;
	unsigned int offs, file_offs;
	int img_file_offset;
	int ret, seg_size, unit_size, wlen, in_size, out_size;
	uint32_t start_time, consume_time, ts_start, ts_cost;
	bool is_record = false, no_wait = false;
	uint8_t *out_buf;
	lzma_head_t lzma_h = {0};
	bool is_raw = (file->size == file->orig_size);
	uint32_t erase_off, erase_size, erase_blk_start, erase_blk_end;
	uint32_t blk_start = ROUND_UP(file->offset, OTA_ERASE_BLOCK_SIZE);
	uint32_t blk_end = ROUND_DOWN(file->offset + part->size, OTA_ERASE_BLOCK_SIZE);

	SYS_LOG_INF("write file %s size 0x%x(0x%x) to offset 0x%x start_offset 0x%x(0x%x)",
		file->name, file->size, file->orig_size, file->offset, start_file_offs, start_orig_offs);

	start_time = k_uptime_get_32();

#ifdef CONFIG_OTA_MUTIPLE_STORAGE
	storage = ota_storage_find(file->storage_id);
	if(storage == NULL) {
		SYS_LOG_INF("storage not init, update failed\n");
		return -EINVAL;
	}
#endif

	if ((start_file_offs >= file->size) || (start_orig_offs >= file->orig_size)) {
		SYS_LOG_ERR("file %s: start file offs 0x%x(0x%x) > file size 0x%x(0x%x)",
			file->name, start_file_offs, start_orig_offs, file->size, file->orig_size);
		return -EINVAL;
	}

	file_offs = start_file_offs;
	offs = start_orig_offs;

	if (strlen(file->name) == 0) {
		img_file_offset = ota_image_get_file_offset(img, NULL);
	} else {
		img_file_offset = ota_image_get_file_offset(img, file->name);
	}
	if (img_file_offset < 0) {
		SYS_LOG_ERR("cannot found file %s in image", file->name);
		return -EINVAL;
	}

	wlen = file->orig_size - offs;
	backend = ota_image_get_backend(img);
	unit_size = OTA_ERASE_ALIGN_SIZE;
	ota_backend_ioctl(backend, OTA_BACKEND_IOCTL_GET_UNIT_SIZE, (unsigned int)&unit_size);
	seg_size = (ota->data_buf_size / unit_size) * unit_size;

	/* clear nvram to avoid erase */
	nvram_config_clear(CONFIG_NVRAM_USER_REGION_SEGMENT_SIZE);

	ota_rx_start(ota, img_file_offset + file_offs, file->size - file_offs, seg_size, is_raw);

	while (wlen > 0) {
		if (!no_wait) {
			os_sem_take(&rx_info->rx_get_sem, OS_FOREVER);
			if (rx_info->rx_errno) {
				ota_breakpoint_update_file_state(bp, file, OTA_BP_FILE_STATE_WRITING, file_offs, offs, 1);
				return rx_info->rx_errno;
			}
		}
		no_wait = false;

		in_size = ring_buf_size_get(&rx_info->rbuf);
		//os_printk("rbuf size 0x%x\n", in_size);
		if (in_size > rx_info->in_bufsize) {
			if (in_size >= rx_info->in_bufsize * 2) {
				no_wait = true;
			}
			in_size = rx_info->in_bufsize;
		}
		if (is_raw) {  // raw file
			if (in_size < wlen) {
				if (in_size < OTA_ERASE_ALIGN_SIZE) {
					continue;
				}
				in_size = ROUND_DOWN(in_size, OTA_ERASE_ALIGN_SIZE);
			}
		} else {  // lzma file
			if (lzma_h.ih_magic != LZMA_MAGIC) {
				if (in_size < sizeof(lzma_head_t)) {
					continue;
				}

				ring_buf_get(&rx_info->rbuf, (uint8_t*)&lzma_h, sizeof(lzma_head_t));
				os_sem_give(&rx_info->rx_put_sem);

				if (lzma_h.ih_magic != LZMA_MAGIC) {
					SYS_LOG_ERR("lzma error magic: 0x%x", lzma_h.ih_magic);
					return -EAGAIN;
				}
				if (rx_info->in_bufsize < lzma_h.ih_img_size) {
					SYS_LOG_ERR("XzDecode error! in_bufsize 0x%x < 0x%x", rx_info->in_bufsize, lzma_h.ih_img_size);
					return -EINVAL;
				}
				if (rx_info->out_bufsize < lzma_h.ih_org_size) {
					SYS_LOG_ERR("XzDecode error! out_bufsize 0x%x < 0x%x", rx_info->out_bufsize, lzma_h.ih_org_size);
					return -EINVAL;
				}
				in_size -= sizeof(lzma_head_t);
			}
			if (in_size < lzma_h.ih_img_size) {
				continue;
			}
			if (in_size >= (lzma_h.ih_img_size + sizeof(lzma_head_t))) {
				no_wait = true;
			}
			in_size = lzma_h.ih_img_size;
		}

		if (!is_record) {
			ota_breakpoint_update_file_state(bp, file, OTA_BP_FILE_STATE_WRITING, file_offs, offs, 1);
			is_record = true;
		} else {
			ota_breakpoint_update_file_state(bp, file, OTA_BP_FILE_STATE_WRITING, file_offs, offs, 0);
		}

		ret = ring_buf_get(&rx_info->rbuf, rx_info->in_buf, in_size);
		os_sem_give(&rx_info->rx_put_sem);
		if (ret != in_size) {
			SYS_LOG_ERR("ring buf get failed, size 0x%x", ret);
			return -EAGAIN;
		}

		if (is_raw) {  // raw file
			out_buf = rx_info->in_buf;
			out_size = in_size;
		} else {  // lzma file
			ts_start = k_uptime_get_32();
			if (rx_info->out_buf == NULL) {
				SYS_LOG_ERR("XzDecode error! out_buf is NULL");
				return -EINVAL;
			}

			// decompress lzma block
			out_size = OTA_OUT_BUFSIZE;
#ifdef CONFIG_OTA_LZMA
			ret = XzDecode(rx_info->in_buf, in_size, rx_info->out_buf, &out_size);
#else
			ret = 0;
#endif
			if (ret == 0) {
				SYS_LOG_ERR("XzDecode error! size 0x%x", out_size);
				return -EAGAIN;
			}

			ts_cost = k_uptime_get_32() - ts_start + 1;
			os_printk("XzDecode 0x%x->0x%x (%d ms)\n", in_size, out_size, ts_cost);

			// check origin size
			if (out_size != lzma_h.ih_org_size) {
				SYS_LOG_ERR("XzDecode out_size mismatch! 0x%x", out_size);
				return -EAGAIN;
			}

			out_buf = rx_info->out_buf;
			lzma_h.ih_magic = 0;
		}

		if (!ota_erase_part_for_upg(ota)) {
			erase_off = file->offset + offs;
			erase_size = ROUND_UP(out_size, OTA_ERASE_ALIGN_SIZE);
			erase_blk_start = ROUND_UP(erase_off, OTA_ERASE_BLOCK_SIZE);
			erase_blk_end = ROUND_UP(erase_off + erase_size, OTA_ERASE_BLOCK_SIZE);

			if (erase_off < blk_start) {
				// first unaligned part
				if (((erase_off + erase_size) > blk_start) && (erase_blk_end <= blk_end)) {
					erase_size = erase_blk_end - erase_off;
				}
			} else if (erase_off < blk_end) {
				// middle aligned part
				erase_off = erase_blk_start;
				erase_size = erase_blk_end - erase_blk_start;
			} else {
				// last unaligned part
			}

			// check partition end
			if ((erase_off + erase_size) > (file->offset + part->size)) {
				erase_size = file->offset + part->size - erase_off;
			}

			if (erase_size > 0) {
				ts_start = k_uptime_get_32();

				ret = ota_storage_erase(storage, erase_off, erase_size);
				if (ret) {
					SYS_LOG_ERR("storage erase failed, offs 0x%x", offs);
					return -EIO;
				}

				ts_cost = k_uptime_get_32() - ts_start;
				os_printk("erase 0x%x(0x%x) (%d ms)\n", erase_off - file->offset, erase_size, ts_cost);
			}
		}

		ts_start = k_uptime_get_32();

		ret = ota_storage_write(storage, file->offset + offs, out_buf, out_size);
		if (ret) {
			SYS_LOG_ERR("storage write failed, offs 0x%x", offs);
			return -EIO;
		}

		ts_cost = k_uptime_get_32() - ts_start;
		os_printk("write 0x%x -> 0x%x(0x%x) (%d ms)\n", file_offs, offs, out_size, ts_cost);

		file_offs += (file->size != file->orig_size) ? in_size + sizeof(lzma_head_t) : in_size;
		offs += out_size;
		wlen -= out_size;
	}

	consume_time = k_uptime_get_32() - start_time + 1;
	SYS_LOG_INF("write file %s: length %d KB, consume %d ms, %d KB/s\n", file->name, file->size / 1024,
		consume_time, file->size / consume_time);

	ota_rx_stop(ota);

	return 0;
}

static int ota_write_file(struct ota_upgrade_info *ota, struct ota_file *file,
					const struct partition_entry *part,
					int start_file_offs, int start_orig_offs)
{
#ifdef CONFIG_OTA_FILE_PATCH
	if (ota_is_patch_fw(ota)) {
		return ota_write_file_by_patch(ota, file, start_file_offs);
	} else {
#endif
		return ota_write_file_normal(ota, file, part, start_file_offs, start_orig_offs);
#ifdef CONFIG_OTA_FILE_PATCH
	}
#endif
}

static int ota_write_and_verify_file(struct ota_upgrade_info *ota,
				     const struct partition_entry *part,
				     struct ota_file *file, bool need_verify)
{
	struct ota_breakpoint *bp = &ota->bp;
	struct ota_storage *storage = ota->storage;
	int bp_file_state, bp_file_offset = 0, bp_orig_offset = 0;
	int err = 0, cur_storage_id, need_erase = 0;

	bp_file_state = ota_breakpoint_get_file_state(bp, file->file_id);

	SYS_LOG_INF("file %s: file_id %d, bp file state %d",
		file->name, file->file_id, bp_file_state);
#ifdef CONFIG_OTA_MUTIPLE_STORAGE
	storage = ota_storage_find(file->storage_id);
	if(storage == NULL) {
		SYS_LOG_INF("storage not init, update failed\n");
		err = -EINVAL;
		goto failed;
	}
#endif

	switch (bp_file_state) {
	case OTA_BP_FILE_STATE_WRITE_DONE:
	case OTA_BP_FILE_STATE_VERIFY_PASS:
		SYS_LOG_INF("file %s: file_id %d, already write done\n",
			file->name, file->file_id);
		break;
	case OTA_BP_FILE_STATE_CLEAN:
		SYS_LOG_INF("file %s: file_id %d, part is clean\n",
			file->name, file->file_id);
		break;
	case OTA_BP_FILE_STATE_WRITING_CLEAN:
		SYS_LOG_INF("file %s: file_id %d, part is writing clean, write offset 0x%x(0x%x)\n",
			file->name, file->file_id, bp->cur_file_write_offset, bp->cur_orig_write_offset);
		bp_file_offset = bp->cur_file_write_offset;
		bp_orig_offset = bp->cur_orig_write_offset;
		break;
	case OTA_BP_FILE_STATE_WRITING:
		SYS_LOG_INF("file %s: file_id %d, part is writing not clean! , write_offset 0x%x(0x%x)\n",
			file->name, file->file_id, bp->cur_file_write_offset, bp->cur_orig_write_offset);
		bp_file_offset = bp->cur_file_write_offset;
		bp_orig_offset = bp->cur_orig_write_offset;
		need_erase = 1;
		break;
	default:
		SYS_LOG_INF("file %s: file_id %d, write offset 0 by default\n",
			file->name, file->file_id);
		need_erase = 1;
		break;
	}

	cur_storage_id = ota_storage_get_storage_id(storage);
	if (part->storage_id != cur_storage_id) {
		SYS_LOG_ERR("BUG: file_id %d storage_id %d not current storage_id %d",
			part->file_id, part->storage_id, cur_storage_id);
		err = -EINVAL;
		goto failed;
	}

	if (ota_erase_part_for_upg(ota)) {
		/* we can erase flash in recovery app */
		if (need_erase) {
			int erase_offset;

			if (!ota_use_recovery_app(ota) && !ota_erase_part_for_upg(ota)) {
				/* cannot erase flash if not in recovery app or single nor */
				if (cur_storage_id == 0) {
					SYS_LOG_INF("update file_id %d: storage %d is xip, skip erase\n",
						cur_storage_id, file->file_id);
					goto skip_erase;
				}
			}

			erase_offset = ROUND_DOWN(file->offset + bp_orig_offset, OTA_ERASE_ALIGN_SIZE);
			bp_orig_offset = erase_offset - file->offset;

			ota_partition_erase_part(ota, part, erase_offset - part->offset);

			SYS_LOG_INF("update file_id %d write_offset from 0x%x to 0x%x\n",
				file->file_id, bp->cur_orig_write_offset, bp_orig_offset);
			bp->cur_orig_write_offset = bp_orig_offset;
		}
	}
skip_erase:

	if (bp_file_state != OTA_BP_FILE_STATE_WRITE_DONE &&
	    bp_file_state != OTA_BP_FILE_STATE_VERIFY_PASS) {
		ota_breakpoint_update_file_state(bp, file, OTA_BP_FILE_STATE_WRITE_START,
						 bp_file_offset, bp_orig_offset, 0);

		err = ota_write_file(ota, file, part, bp_file_offset, bp_orig_offset);
		if (err) {
			SYS_LOG_ERR("failed to write file %s",
				file->name);
			goto failed;
		}
		/*base sdfs update need to erase extern sdfs part*/
		if (file->file_id == PARTITION_FILE_ID_SDFS_PART_BASE) {
			const struct partition_entry *part_fatfs = partition_get_part(PARTITION_FILE_ID_SDFS_PART1);

			if (part_fatfs) {
				// erase res_b partition
				ota_partition_erase_part(ota, part_fatfs, 0);
		
				// clear first 32bytes to 0xff
				memset(ota->data_buf, 0xff, 32);
				ota_storage_write(ota->storage, part_fatfs->offset, ota->data_buf, 32);
				SYS_LOG_INF("clear res_b: offset 0x%x", part_fatfs->offset);
			}
		}

		ota_breakpoint_update_file_state(bp, file, OTA_BP_FILE_STATE_WRITE_DONE, 0, 0, 0);
	}

	if (need_verify) {
		err = ota_verify_file(ota, file);
		if (err) {
			SYS_LOG_ERR("file %s, verify failed", file->name);
			ota_breakpoint_update_file_state(bp, file, OTA_BP_FILE_STATE_VERIFY_FAIL, 0, 0, 0);
			goto failed;
		}

		SYS_LOG_INF("file %s, verify pass", file->name);
		ota_breakpoint_update_file_state(bp, file, OTA_BP_FILE_STATE_VERIFY_PASS, 0, 0, 0);
	}

	return 0;

failed:
	if (err != -EIO && err != -EAGAIN) {
		/* we assume -EIO error that can be resumed */
		ota_breakpoint_update_file_state(bp, file, OTA_BP_FILE_STATE_WRITE_FAIL, 0, 0, 0);
	}

	return err;
}

static int ota_upgrade_verify_along(struct ota_upgrade_info *ota)
{
	const struct partition_entry *part;
	struct ota_manifest *manifest = &ota->manifest;
	struct ota_file *file;
	int i, err;
	int cur_file_id;

	for (i = 0; i < manifest->file_cnt; i++) {
		file = &manifest->wfiles[i];

		part = partition_get_mirror_part(file->file_id);
		if (part == NULL) {
			SYS_LOG_INF("cannt found mirror part entry for file_id %d",
				file->file_id);

			if (ota_use_recovery(ota)) {
				cur_file_id = partition_get_current_file_id();
				part = partition_get_part(file->file_id);
				if (cur_file_id == file->file_id || part == NULL) {
					SYS_LOG_ERR("cannt found part entry for file_id %d, cur_file_id %d",
						file->file_id, cur_file_id);
					return -EINVAL;
				}
				SYS_LOG_INF("found part entry for file_id %d, cur_file_id %d",
					file->file_id, cur_file_id);
			} else {
				return -EINVAL;
			}
		}

		/* ignore boot partition */
		if (partition_is_boot_part(part))
			continue;

		if (partition_is_param_part(part))
			continue;

		err = ota_verify_file(ota, file);
		if (err) {
			SYS_LOG_ERR("file %s, verify failed", file->name);
			ota_breakpoint_update_file_state(&ota->bp, file, OTA_BP_FILE_STATE_VERIFY_FAIL, 0, 0, 0);
			return -1;
		}
		ota_upgrade_file_cb(ota, file->file_id);

		SYS_LOG_INF("file %s, verify pass", file->name);
		ota_breakpoint_update_file_state(&ota->bp, file, OTA_BP_FILE_STATE_VERIFY_PASS, 0, 0, 0);
	}

	return 0;
}
#if 0
static int ota_auto_update_version(struct ota_upgrade_info *ota,
				     const struct partition_entry *part,
				     struct ota_file *file)
{
	struct ota_storage *storage = ota->storage;
	const struct fw_version *cur_ver;
	struct fw_version *new_ver;
	uint32_t start_time, consume_time;
	uint32_t addr, len = file->size, wlen;
	uint8_t *param_ptr, *temp_param_ptr, *param_map_ptr;

	SYS_LOG_INF("write file %s len %d to offset 0x%x by auto update version",
				file->name, len, file->offset);

#ifdef CONFIG_OTA_MUTIPLE_STORAGE
	storage = ota_storage_find(file->storage_id);
#endif

	start_time = k_uptime_get_32();

	cur_ver = (struct fw_version *)fw_version_get_current();

	param_ptr = (uint8_t *)mem_malloc(file->size);
	if (!param_ptr) {
		SYS_LOG_INF("failed to malloc size %d", file->size);
		return -ENOMEM;
	}

	temp_param_ptr = param_ptr;

	param_map_ptr = soc_memctrl_create_temp_mapping(file->offset, false);
	memcpy(param_ptr, param_map_ptr, file->size);
	soc_memctrl_clear_temp_mapping(param_map_ptr);

	new_ver = (struct fw_version *)(param_ptr + SOC_BOOT_FIRMWARE_VERSION_OFFSET);
	/* Allow ota upgrade even though new ota version is bigger than the current's */
	if (new_ver->version_code > cur_ver->version_code) {
		SYS_LOG_WRN("new fw version 0x%x is bigger than current's 0x%x",
			new_ver->version_code, cur_ver->version_code);
	}
	new_ver->version_code = cur_ver->version_code + 1;
	new_ver->checksum = utils_crc32(0, (const uint8_t *)new_ver, sizeof(struct fw_version) - 4);

	SYS_LOG_INF("current fw version: 0x%x", cur_ver->version_code);
	SYS_LOG_INF("current fw version name: %s", cur_ver->version_name);
	SYS_LOG_INF("new fw version: 0x%x", new_ver->version_code);
	SYS_LOG_INF("new fw version name: %s", new_ver->version_name);

	ota_partition_erase_part(ota, part, 0);

	addr = file->offset;
	/* enable encryption function */
	if (part->flag & PARTITION_FLAG_ENABLE_ENCRYPTION) {
		SYS_LOG_INF("enable encryption write");
		addr |= (1 << 31);
		if (len % 32) {
			SYS_LOG_ERR("len %d shall align with 32 bytes", len);
			mem_free(temp_param_ptr);
			return -EINVAL;
		}
	}

	wlen = 32;
	while (len) {
		if (len < wlen)
			wlen = len;
		if (ota_storage_write(storage, addr, param_ptr, wlen)) {
			SYS_LOG_ERR("storage write failed, offs 0x%x", addr);
			mem_free(temp_param_ptr);
			return -EFAULT;
		}
		param_ptr += wlen;
		addr += wlen;
		len -= wlen;
	}

	consume_time = k_uptime_get_32() - start_time + 1;
	SYS_LOG_INF("write file %s: length %d KB, consume %d ms, %d KB/s\n", file->name, len / 1024,
				consume_time, len / consume_time);

	mem_free(temp_param_ptr);
	return 0;
}
#endif

static int ota_do_upgrade(struct ota_upgrade_info *ota)
{
	const struct partition_entry *part, *boot_part = NULL, *param_part = NULL;
	struct ota_manifest *manifest = &ota->manifest;
	struct ota_file *file, *boot_file = NULL, *param_file = NULL;
	int i, err, max_file_size;
	int cur_file_id;
	int retry_times = 0;

	SYS_LOG_INF("ota file_cnt %d", manifest->file_cnt);

try_again:
	for (i = 0; i < manifest->file_cnt; i++) {
		file = &manifest->wfiles[i];

		part = partition_get_mirror_part(file->file_id);
		if (part == NULL) {
			SYS_LOG_INF("cannt found mirror part entry for file_id %d",
				file->file_id);

			if (ota_use_recovery(ota)) {
				cur_file_id = partition_get_current_file_id();
				part = partition_get_part(file->file_id);
				if (cur_file_id == file->file_id || part == NULL) {
					SYS_LOG_ERR("cannt found part entry for file_id %d, cur_file_id %d",
						file->file_id, cur_file_id);
					return -EINVAL;
				}
				SYS_LOG_INF("found part entry for file_id %d, cur_file_id %d",
					file->file_id, cur_file_id);
			} else {
				return -EINVAL;
			}
		}

		max_file_size = partition_get_max_file_size(part);
		if (file->orig_size > max_file_size) {
			SYS_LOG_ERR("part %s: file size 0x%x > part max file size 0x%x",
				part->name, file->orig_size, max_file_size);
			return -EINVAL;
		}

		SYS_LOG_INF("[%d]: file %s, file_id %d write to nor addr 0x%x",
			i, file->name, file->file_id, part->file_offset);
		file->offset = part->file_offset;

		if (partition_is_boot_part(part)) {
			boot_file = file;
			boot_part = part;
			continue;
		}

		if (partition_is_param_part(part)) {
			param_file = file;
			param_part = part;
			continue;
		}

		err = ota_write_and_verify_file(ota, part, file, false);
		if (err) {
			return err;
		}
	}

	err = ota_upgrade_verify_along(ota);
	if (err) {
		/* retry upgrade if verify failed */
		if (retry_times < 1) {
			SYS_LOG_ERR("OTA upgrade retry after verify failed");
			retry_times++;
			ota_image_progress_reset(ota->img);
			goto try_again;
		} else {
			return err;
		}
	}
	/* don't upgrade for boot file and para file*/
#if OTA_FULL_UPGRADE
	/* write boot file at secondly last */
	if (boot_file && boot_part) {
		/* write boot file at mirror part */
		err = ota_write_and_verify_file(ota, boot_part, boot_file, true);
		if (err) {
			return err;
		}
		/* erase boot at current part */
		boot_part = partition_get_part(boot_file->file_id);
		ota_partition_erase_part(ota, boot_part, 0);
		ota_breakpoint_set_file_state(&ota->bp, boot_file->file_id, OTA_BP_FILE_STATE_CLEAN);
	}

	/* write param file at last */
	if (param_file && param_part) {
		/* write param file at mirror part */
		err = ota_write_and_verify_file(ota, param_part, param_file, true);
		if (err) {
			return err;
		}
		/* erase param file at current part */
		param_part = partition_get_part(param_file->file_id);
		ota_partition_erase_part(ota, param_part, 0);
		ota_breakpoint_set_file_state(&ota->bp, param_file->file_id, OTA_BP_FILE_STATE_CLEAN);
//		if (ota_use_no_version_control(ota)) {
//			err = ota_auto_update_version(ota, param_part, param_file);
//			if (err) {
//				return err;
//			}
//		}
	}
#endif
	/* try to save res version */
	ota_save_res_version(ota);
	return 0;
}

static int ota_is_need_upgrade(struct ota_upgrade_info *ota)
{
	struct ota_breakpoint *bp = &ota->bp;
	const struct fw_version *cur_ver, *img_ver;
	struct ota_backend *backend;
	int backend_type;

#if defined(CONFIG_OTA_FILE_PATCH) || defined(CONFIG_OTA_RES_PATCH)
	const struct fw_version *patch_old_ver;
	patch_old_ver = &ota->manifest.old_fw_ver;

	SYS_LOG_INF("OTA patch old fw version:");
	fw_version_dump(patch_old_ver);
#endif

	img_ver = &ota->manifest.fw_ver;
	cur_ver = fw_version_get_current();

	SYS_LOG_INF("ota fw version:");
	fw_version_dump(img_ver);

	SYS_LOG_INF("current fw version:");
	fw_version_dump(cur_ver);

	backend = ota_image_get_backend(ota->img);
	backend_type = ota_backend_get_type(backend);

	if (backend_type != OTA_BACKEND_TYPE_TEMP_PART &&
		!(backend_type == OTA_BACKEND_TYPE_CARD && ota_use_recovery_app(ota)) &&
	    bp->backend_type != OTA_BACKEND_TYPE_UNKNOWN &&
	    bp->backend_type != backend_type) {
		SYS_LOG_ERR("backend type is chagned(%d -> %d), need erase old firmware",
			bp->backend_type, ota_backend_get_type(backend));
		return -1;
	}

	if (strcmp(cur_ver->board_name, img_ver->board_name)) {
		/* skip */
		SYS_LOG_ERR("unmatched board name, skip ota");
		return -1;
	}

#ifdef CONFIG_OTA_FILE_PATCH
	if (ota_is_patch_fw(ota)) {
		/* validate ota patch firmware version */
		if (cur_ver->version_code != patch_old_ver->version_code) {
			SYS_LOG_ERR("unmatched fw ver, curr 0x%x but OTA patch old ver is 0x%x",
				cur_ver->version_code, ota->manifest.old_fw_ver.version_code);
			return -1;
		}

		if (ota_use_no_version_control(ota)) {
			SYS_LOG_ERR("Patch FW only support with version control");
			return -1;
		}
	}
#endif
#if 0
//#ifdef CONFIG_OTA_RES_PATCH
		if (ota_is_patch_res(ota)) {
			/* validate ota patch firmware version */
			if (cur_ver->version_res != patch_old_ver->version_res) {
				SYS_LOG_ERR("unmatched fw ver, curr 0x%x but OTA patch old ver is 0x%x",
					cur_ver->version_res, ota->manifest.old_fw_ver.version_res);
				return -1;
			}

			if (ota_use_no_version_control(ota)) {
				SYS_LOG_ERR("Patch FW only support with version control");
				return -1;
			}
		}
#endif
	if (!ota_use_no_version_control(ota)) {
		if (cur_ver->version_code >= img_ver->version_code) {
			/* skip */
			SYS_LOG_INF("ota image is same or older, skip ota");
			return 0;
		}
	}

	if (bp->state == OTA_BP_STATE_WRITING_IMG ||
		bp->state == OTA_BP_STATE_UPGRADE_WRITING ||
		bp->state == OTA_BP_STATE_UPGRADE_PENDING) {
		if ((bp->new_version != 0 && bp->new_version != img_ver->version_code)
			|| (bp->data_checksum != ota_image_get_checksum(ota->img))) {
			/* FIXME: has new version fw, need erase partition */
			SYS_LOG_INF("has new version fw, need erase old firmware");
			return 2;
		}
	}

	return 1;
}

static int ota_upgrade_statistics(struct ota_upgrade_info *ota)
{
	const struct partition_entry *part;
	struct ota_manifest *manifest = &ota->manifest;
	struct ota_file *file;
	struct ota_breakpoint *bp = &ota->bp;
	int i, bp_file_state, start_write_offset = 0, erase_offset;
	int cur_file_id;
	uint32_t total_size = 0;


	for (i = 0; i < manifest->file_cnt; i++) {
		file = &manifest->wfiles[i];
		part = partition_get_mirror_part(file->file_id);
		if (part == NULL) {
			SYS_LOG_INF("cannt found mirror part entry for file_id %d",
				file->file_id);

			if (ota_use_recovery(ota)) {
				cur_file_id = partition_get_current_file_id();
				part = partition_get_part(file->file_id);
				if (cur_file_id == file->file_id || part == NULL) {
					SYS_LOG_ERR("cannt found part entry for file_id %d, cur_file_id %d",
						file->file_id, cur_file_id);
					return -EINVAL;
				}
				SYS_LOG_INF("found part entry for file_id %d, cur_file_id %d",
					file->file_id, cur_file_id);
			} else {
				return -EINVAL;
			}
		}

		bp_file_state = ota_breakpoint_get_file_state(bp, file->file_id);
		if (bp_file_state != OTA_BP_FILE_STATE_WRITE_DONE &&
    		bp_file_state != OTA_BP_FILE_STATE_VERIFY_PASS) {
			if (bp_file_state == OTA_BP_FILE_STATE_WRITING_CLEAN
				|| bp_file_state == OTA_BP_FILE_STATE_WRITING
				|| bp_file_state == OTA_BP_FILE_STATE_WRITE_START) {
				if (file->size == file->orig_size) { // raw file
					/* Align offset with erase size */
					erase_offset = ROUND_DOWN(file->offset + bp->cur_file_write_offset, OTA_ERASE_ALIGN_SIZE);
					start_write_offset += (erase_offset - file->offset);
				} else { // lzma file
					start_write_offset += bp->cur_file_write_offset;
				}
			}
		} else {
			start_write_offset += file->size;
		}
		total_size += file->size;

		SYS_LOG_INF("ota file[%d]%s: total size %d, bp offset 0x%x",
			file->file_id, file->name, file->size, start_write_offset);
	}

	if (total_size)
		ota_image_progress_on(ota->img, total_size, start_write_offset);

	return 0;
}

static int ota_temp_part_is_upgrade(struct ota_upgrade_info *ota)
{
	struct ota_manifest *manifest = &ota->manifest;
	struct ota_file *file;

	for (int i = 0; i < manifest->file_cnt; i++) {
		file = &manifest->wfiles[i];
		if (file->type == PARTITION_TYPE_TEMP && file->file_id == PARTITION_FILE_ID_OTA_TEMP)
			return 1;
	}
	return 0;
}
static void ota_upgrade_exit(struct ota_upgrade_info *ota)
{
	SYS_LOG_INF("exit");

	if (ota) {
		if (ota->img)
			ota_image_exit(ota->img);

		if (ota->storage)
			ota_storage_exit(ota->storage);
	}
}

int ota_upgrade_check(struct ota_upgrade_info *ota)
{
	struct ota_breakpoint *bp = &ota->bp;
	struct ota_backend *backend = NULL;
	int err, need_upgrade;
	int connect_type = 0;

	SYS_LOG_INF("handle upgrade");

	if (ota->state != OTA_INIT) {
		SYS_LOG_ERR("ota state <%d> is not OTA_INIT, skip upgrade", ota->state);
		return -EINVAL;
	}
	if (ota_image_get_backend(ota->img) == NULL) {
		SYS_LOG_ERR("ota backend null\n");
		return -EINVAL;
	}

	backend = ota_image_get_backend(ota->img);
	err = ota_image_open(ota->img);
	if (err) {
		if (ota_backend_get_type(backend) == OTA_BACKEND_TYPE_BLUETOOTH) {
			ota_backend_read_prepare(backend, 0, NULL, 0);
		}
		SYS_LOG_INF("ota image open failed");
		err = -EIO;
		goto exit_invalid;
	}

	if (ota_use_recovery_app(ota)) {
		/* only check data in recovery app to save time */
		err = ota_image_check_data(ota->img);
		if (err) {
			SYS_LOG_ERR("bad data crc");
			ota_breakpoint_update_state(bp, OTA_BP_STATE_WRITING_IMG_FAIL);
			goto exit;
		}
	}

	err = ota_manifest_parse_file(&ota->manifest, ota->img, OTA_MANIFESET_FILE_NAME);
	if (err) {
		SYS_LOG_INF("cannot get manifest file in image");
		err = -EAGAIN;
		goto exit;
	}

	/* need upgrade? */
	need_upgrade = ota_is_need_upgrade(ota);
	if (need_upgrade <= 0) {
		SYS_LOG_INF("skip upgrade");
		ota_breakpoint_update_state(bp, OTA_BP_STATE_WRITING_IMG_FAIL);
		err = -EINVAL;
		goto exit;
	}
	else if(need_upgrade == 2)
	{
	    SYS_LOG_INF("bp pending");
	    ota_breakpoint_update_state(bp, OTA_BP_STATE_UPGRADE_PENDING);
		ota_update_state(ota, OTA_FAIL);
		ota_partition_update_prepare(ota);
		ota_update_state(ota, OTA_INIT);
	}

	SYS_LOG_INF("burn firmware image");

	if (!ota->data_buf) {
		ota->data_buf = mem_malloc(ota->data_buf_size);
	}
	if (!ota->data_buf) {
		SYS_LOG_ERR("failed to allocate %d bytes", ota->data_buf_size);
		err = -EAGAIN;
		goto exit;
	}

	/* update breakpoint for new firmware */
	bp->backend_type = ota_backend_get_type(backend);
	bp->new_version = ota->manifest.fw_ver.version_code;
	bp->data_checksum = ota_image_get_checksum(ota->img);

	ota_upgrade_statistics(ota);

	ota_update_state(ota, OTA_RUNNING);

	err = ota_rx_init(ota);
	if (err) {
		goto exit;
	}

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
		/* set version code */
		ota_update_fw_version(ota, PARTITION_FILE_ID_SYSTEM);
		ota_breakpoint_update_state(bp, OTA_BP_STATE_UPGRADE_DONE);
	} else {
		ota_breakpoint_update_state(bp, OTA_BP_STATE_WRITING_IMG);
		err = ota_do_upgrade(ota);
		if (err) {
			SYS_LOG_INF("write ota image failed, err %d", err);
			if (err != -EIO && err != -EAGAIN) {
				ota_breakpoint_update_state(bp, OTA_BP_STATE_WRITING_IMG_FAIL);
			}

			goto exit;
		} else {
			if (ota_temp_part_is_upgrade(ota)) {
				ota_breakpoint_update_state(bp, OTA_BP_STATE_UPGRADE_PENDING);
			} else {
				ota_breakpoint_update_state(bp, OTA_BP_STATE_UPGRADE_DONE);
			}
		}
	}

	SYS_LOG_INF("upgrade successfully!");

	ota_image_report_progress(ota->img, 0, 1);
	ota_image_ioctl(ota->img, OTA_BACKEND_IOCTL_REPORT_IMAGE_VALID, 1);
	ota_update_state(ota, OTA_DONE);

exit:
	ota_rx_exit(ota);

	if (err) {
		ota_image_ioctl(ota->img, OTA_BACKEND_IOCTL_REPORT_IMAGE_VALID, 0);
		ota_update_state(ota, OTA_FAIL);

		if (err != -EAGAIN) {
		/* upgrade fail need to clear bp and erase dirty part*/
			ota_partition_update_prepare(ota);
		}
		/* wait for upgrade resume */
		SYS_LOG_INF("ota status -> OTA_INIT, wait for upgrading resume!");
		ota_update_state(ota, OTA_INIT);
	}

	if (ota->data_buf) {
		if (ota_backend_get_type(backend) == OTA_BACKEND_TYPE_BLUETOOTH) {
			ota_backend_read_prepare(backend, 0, NULL, 0);
		}
		mem_free(ota->data_buf);
		ota->data_buf = NULL;
	}

	if ((!err) && backend) {
		// delay 500ms to close bt for sending remain data
		if (ota_backend_get_type(backend) == OTA_BACKEND_TYPE_BLUETOOTH) {
			os_sleep(500);
		}
	}

	ota_image_close(ota->img);

exit_invalid:
	if (err) {
		if (ota_backend_get_type(backend) == OTA_BACKEND_TYPE_BLUETOOTH) {
			/* waiting for 500ms to disconnect ble.*/
			int sp_cnt = 0;
			do {
				ota_backend_ioctl(backend, 
					OTA_BACKEND_IOCTL_GET_CONNECT_TYPE, (unsigned int)&connect_type);
				SYS_LOG_INF("connect_type %d sp_cnt %d.",connect_type, sp_cnt);
		#ifdef CONFIG_BT_MANAGER
				if (BLE_CONNECT_TYPE != connect_type) {
					break;
				}
		#endif
				os_sleep(50);
				sp_cnt++;
				if (10 == sp_cnt) {
					ota_backend_ioctl(backend, OTA_BACKEND_IOCTL_EXECUTE_EXIT, 0);
				}
			} while (sp_cnt < 10);
		}

		ota_image_unbind(ota->img, ota_image_get_backend(ota->img));
	} else {
		ota_upgrade_exit(ota);
	}
	return err;
}

int ota_upgrade_attach_backend(struct ota_upgrade_info *ota, struct ota_backend *backend)
{
	struct ota_backend *img_backend = ota_image_get_backend(ota->img);

	SYS_LOG_INF("attach backend type %d", backend->type);

	if (img_backend != NULL && img_backend->type != backend->type) {
		SYS_LOG_ERR("already attached backend %d %d", img_backend->type, backend->type);
		return -EBUSY;
	}

	ota_image_bind(ota->img, backend);

	return 0;
}

void ota_upgrade_detach_backend(struct ota_upgrade_info *ota, struct ota_backend *backend)
{
	SYS_LOG_INF("detach backend %p", backend);

	/* to avoid empty pointer */
#if 0
	struct ota_backend *img_backend = ota_image_get_backend(ota->img);

	if (img_backend == backend)
		ota_image_unbind(ota->img, backend);
#endif
}

int ota_upgrade_is_in_progress(struct ota_upgrade_info *ota)
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

int ota_upgrade_set_in_progress(struct ota_upgrade_info *ota)
{
	if (!ota_upgrade_is_in_progress(ota)) {
		ota_breakpoint_update_state(&ota->bp, OTA_BP_STATE_UPGRADE_PENDING);
	}

	return 0;
}

static struct ota_upgrade_info global_ota_upgrade_info;
struct ota_upgrade_info *ota_upgrade_init(struct ota_upgrade_param *param)
{
	struct ota_upgrade_info *ota;

	SYS_LOG_INF("init");

	ota = &global_ota_upgrade_info;

	memset(ota, 0x0, sizeof(struct ota_upgrade_info));

	if (param->no_version_control) {
		SYS_LOG_INF("enable no version control");
		ota->flags |= OTA_FLAG_USE_NO_VERSION_CONTROL;
	}

	/* allocate data buffer later */
	ota->data_buf_size = OTA_DATA_BUFFER_SIZE;
	if (!ota->data_buf) {
		ota->data_buf = mem_malloc(ota->data_buf_size);
	}

	ota->storage = ota_storage_init(param->storage_name);
	if (!ota->storage) {
		SYS_LOG_INF("storage open err");
		ota = NULL;
		goto init_exit;
	}
#ifdef CONFIG_OTA_MUTIPLE_STORAGE
	ota->storage_ext = ota_storage_init(param->storage_ext_name);
	if (!ota->storage_ext) {
		SYS_LOG_INF("storage ext open err");
		ota = NULL;
		goto init_exit;
	}
#endif

	if (param->flag_use_recovery) {
		ota->flags |= OTA_FLAG_USE_RECOVERY;
	}
	if (param->flag_erase_part_for_upg) {
		ota->flags |= OTA_FLAG_ERASE_PART_FOR_UPG;
	}
	if (param->flag_keep_temp_part) {
		ota->flags |= OTA_FLAG_KEEP_TEMP_PART;
	}

	if (param->flag_use_recovery_app) {
		if (!param->flag_use_recovery) {
			SYS_LOG_ERR("invalid flag_is_recovery_app");
			ota = NULL;
			goto init_exit;
		}

		ota->flags |= OTA_FLAG_USE_RECOVERY_APP;
	}

	ota_breakpoint_init(&ota->bp);

	ota_partition_update_prepare(ota);

	ota->img = ota_image_init();
	if (!ota->img) {
		SYS_LOG_ERR("image init failed");
		ota = NULL;
		goto init_exit;
	}

	ota->notify = param->notify;
	ota->file_cb = param->file_cb;

	ota_update_state(ota, OTA_INIT);

	// disable nor-suspend to reduce erase time when erase size >= 256KB
	ota_storage_set_max_erase_seg(ota->storage, OTA_STORAGE_MAX_ERASE_SEGMENT_SIZE);

init_exit:
	if (ota && ota->data_buf) {
		mem_free(ota->data_buf);
		ota->data_buf = NULL;
	}

	return ota;
}
struct ota_storage *ota_upgrade_storage_fine(struct ota_file *file)
{
	struct ota_storage *storage = global_ota_upgrade_info.storage;

#ifdef CONFIG_OTA_MUTIPLE_STORAGE
	storage = ota_storage_find(file->storage_id);
#endif
	return storage;
}

