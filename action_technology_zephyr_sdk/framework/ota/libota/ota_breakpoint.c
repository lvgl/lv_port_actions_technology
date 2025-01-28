/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <string.h>
#include <drivers/nvram_config.h>
#include <partition/partition.h>
#include "ota_manifest.h"
#include "ota_breakpoint.h"
#include <os_common_api.h>
#include <ota_storage.h>
#ifdef CONFIG_WATCHDOG
#include <watchdog_hal.h>
#endif

void ota_breakpoint_dump(struct ota_breakpoint *bp)
{
#if CONFIG_SYS_LOG_DEFAULT_LEVEL > 2
	struct ota_file *cur_file = &bp->cur_file;
	int i;

	SYS_LOG_INF("breakpoint: %p", bp);
	SYS_LOG_INF("  old_version 0x%x, new_version 0x%x, data_checksum 0x%x",
		bp->old_version, bp->new_version, bp->data_checksum);
	SYS_LOG_INF("  bp_id %d, state %d, backend_type %d",
		bp->bp_id, bp->state, bp->backend_type);

	for (i = 0; i < ARRAY_SIZE(bp->file_state); i++) {
		SYS_LOG_INF("  [%d] file_id %d state 0x%x", i,
			bp->file_state[i].file_id, bp->file_state[i].state);
	}

	SYS_LOG_INF("  cur file %s: file_id %d, offset 0x%x, size 0x%x(0x%x)",
		cur_file->name, cur_file->file_id, cur_file->offset, cur_file->size, cur_file->orig_size);
	SYS_LOG_INF("  write_offset 0x%x(0x%x)\n",
		bp->cur_file_write_offset, bp->cur_orig_write_offset);
#endif

}

int ota_breakpoint_save(struct ota_breakpoint *bp)
{
	int err;

	SYS_LOG_INF("save bp: state %d\n", bp->state);

	/* update seq id */
	bp->bp_id++;

	ota_breakpoint_dump(bp);

	err = nvram_config_set("OTA_BP", bp, sizeof(struct ota_breakpoint));
	if (err) {
		return -1;
	}

	return 0;
}

int ota_breakpoint_file_state(struct ota_breakpoint *bp)
{
	int err;

	SYS_LOG_INF("save bp: state %d\n", bp->state);

	/* update seq id */
	bp->bp_id++;

	ota_breakpoint_dump(bp);

	err = nvram_config_set("OTA_BP", bp, sizeof(struct ota_breakpoint));
	if (err) {
		return -1;
	}

	return 0;
}

int ota_breakpoint_load(struct ota_breakpoint *bp)
{
	int rlen;

	SYS_LOG_INF("save bp: state %d\n", bp->state);

	rlen = nvram_config_get("OTA_BP", bp, sizeof(struct ota_breakpoint));
	if (rlen < 0) {
		SYS_LOG_INF("cannot found OTA_BP");
		return -1;
	}

	ota_breakpoint_dump(bp);

	return 0;
}

int ota_breakpoint_get_current_state(struct ota_breakpoint *bp)
{
	return bp->state;
}

int ota_breakpoint_set_file_state(struct ota_breakpoint *bp, int file_id, int state)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(bp->file_state); i++) {
		if (bp->file_state[i].file_id == file_id) {
			//SYS_LOG_INF("set file_id %d file state 0x%x\n", file_id, state);
			bp->file_state[i].state = state;
			return 0;
		}
	}

	/* not found, allocate new slot for this file */
	for (i = 0; i < ARRAY_SIZE(bp->file_state); i++) {
		if (bp->file_state[i].file_id == 0) {
			//SYS_LOG_INF("set file_id %d file state 0x%x\n", file_id, state);
			bp->file_state[i].file_id = file_id;
			bp->file_state[i].state = state;
			return 0;
		}
	}

	/* no slot? */
	SYS_LOG_INF("cannot found state slot for file_id %d\n", file_id);
	return -1;
}

int ota_breakpoint_get_file_state(struct ota_breakpoint *bp, int file_id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(bp->file_state); i++) {
		if (bp->file_state[i].file_id == file_id) {
			return bp->file_state[i].state;
		}
	}

	/* no slot? */
	SYS_LOG_INF("cannot found state slot of file_id %d\n", file_id);
	return -1;
}

int ota_breakpoint_clear_all_file_state(struct ota_breakpoint *bp)
{
	int i;

	SYS_LOG_INF("clear all file state");

	for (i = 0; i < ARRAY_SIZE(bp->file_state); i++) {
		bp->file_state[i].file_id = 0;
		bp->file_state[i].state = 0;
	}

	memset(&bp->cur_file, 0x0, sizeof(struct ota_file));
	bp->cur_file_write_offset = 0;
	bp->cur_orig_write_offset = 0;

	return 0;
}

int ota_breakpoint_update_state(struct ota_breakpoint *bp, int state)
{
	SYS_LOG_INF("update breakpoint state %d", state);

	bp->state = state;

	if (state == OTA_BP_STATE_UPGRADE_DONE ||
	    state == OTA_BP_STATE_UPGRADE_PENDING) {
		ota_breakpoint_clear_all_file_state(bp);
	}

	ota_breakpoint_save(bp);

	return 0;
}
extern struct ota_storage *ota_upgrade_storage_fine(struct ota_file *file);

int ota_breakpoint_update_file_state(struct ota_breakpoint *bp, struct ota_file *file,
		int state, int cur_file_offset, int cur_orig_offset, int force)
{
	int need_save;

	SYS_LOG_DBG("update bp file_id %d state %d offs 0x%x(0x%x)",
		file->file_id, state, cur_file_offset, cur_orig_offset);

	need_save = 0;

	/* aligned with erase sector */
	switch (state) {
	case OTA_BP_FILE_STATE_WRITE_START:
		memcpy(&bp->cur_file, file, sizeof(struct ota_file));
		bp->cur_file_write_offset = cur_file_offset;
		bp->cur_orig_write_offset = cur_orig_offset;
		break;

	case OTA_BP_FILE_STATE_WRITING:
		if (force || (cur_orig_offset - bp->cur_orig_write_offset) == 0 ||
		    (cur_orig_offset - bp->cur_orig_write_offset) >= OTA_BP_SAVE_SIZE) {
			need_save = 1;
			bp->cur_file_write_offset = cur_file_offset;
			bp->cur_orig_write_offset = cur_orig_offset;
		}

		break;

	default:
		need_save = 1;
		break;
	}

	ota_breakpoint_set_file_state(bp, file->file_id, state);

	if (need_save){
		ota_storage_sync(ota_upgrade_storage_fine(file));
		ota_breakpoint_save(bp);
#ifdef CONFIG_WATCHDOG
		watchdog_clear();
#endif
	}

	return 0;
}

int ota_breakpoint_init_default_value(struct ota_breakpoint *bp)
{
	memset(bp, 0x0, sizeof(struct ota_breakpoint));

	bp->old_version = fw_version_get_code();
	bp->mirror_id = !partition_get_current_mirror_id();
	bp->state = OTA_BP_STATE_UNKOWN;

	return 0;
}

int ota_breakpoint_init(struct ota_breakpoint *bp)
{
	int err;

	SYS_LOG_INF("init bp %p\n", bp);

	err = ota_breakpoint_load(bp);
	if (err) {
		SYS_LOG_INF("no bp in nvram, use default bp");
		ota_breakpoint_init_default_value(bp);
		ota_breakpoint_save(bp);
	}

	return 0;
}

void ota_breakpoint_exit(struct ota_breakpoint *bp)
{
	SYS_LOG_INF("deinit bp %p\n", bp);
}
