/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA breakpoint interface
 */

#ifndef __OTA_BREAKPOINT_H__
#define __OTA_BREAKPOINT_H__

#include <kernel.h>

#define OTA_BP_SAVE_SIZE			(512*1024)

#define OTA_FW_MAX_FILE				8

#define OTA_BP_STATE_UNKOWN			0x0
#define OTA_BP_STATE_CLEAN			0x1
#define OTA_BP_STATE_WRITING_IMG		0x2
#define OTA_BP_STATE_UPGRADE_PENDING		0x3
#define OTA_BP_STATE_UPGRADE_WRITING		0x4
#define OTA_BP_STATE_UPGRADE_DONE		0xa
#define OTA_BP_STATE_WRITING_IMG_FAIL		0xe
#define OTA_BP_STATE_UPGRADING_FAIL		0xf

#define OTA_BP_FILE_STATE_UNKOWN		0x0
#define OTA_BP_FILE_STATE_CLEAN			0x1
#define OTA_BP_FILE_STATE_WRITE_START		0x2
#define OTA_BP_FILE_STATE_WRITING		0x3
#define OTA_BP_FILE_STATE_WRITING_DIRTY		0x4
#define OTA_BP_FILE_STATE_WRITING_CLEAN		0x5
#define OTA_BP_FILE_STATE_WRITE_DONE		0x6
#define OTA_BP_FILE_STATE_VERIFY_PASS		0x7
#define OTA_BP_FILE_STATE_WRITE_FAIL		0xe
#define OTA_BP_FILE_STATE_VERIFY_FAIL		0xf

struct ota_breakpoint_file_state {
	uint8_t file_id;
	uint8_t state;
};

struct ota_breakpoint {
	uint8_t bp_id;
	uint8_t mirror_id;
	uint8_t backend_type;
	uint8_t state;

	uint32_t old_version;
	uint32_t new_version;

	uint8_t total_file_num;
	uint8_t write_done_file_num;

	struct ota_file cur_file;
	uint32_t cur_file_write_offset;
	uint32_t cur_orig_write_offset;
	struct ota_breakpoint_file_state file_state[OTA_FW_MAX_FILE];

	uint32_t data_checksum;
} __attribute__((packed));

struct ota_breakpoint_file {
	uint8_t bp_id;
	uint8_t file_id;
	uint8_t reserved[2];

	uint32_t write_offset;
} __attribute__((packed));

void ota_breakpoint_dump(struct ota_breakpoint *bp);

int ota_breakpoint_save(struct ota_breakpoint *bp);
int ota_breakpoint_load(struct ota_breakpoint *bp);

int ota_breakpoint_get_current_state(struct ota_breakpoint *bp);

int ota_breakpoint_set_file_state(struct ota_breakpoint *bp, int file_id, int state);
int ota_breakpoint_get_file_state(struct ota_breakpoint *bp, int file_id);

int ota_breakpoint_update_state(struct ota_breakpoint *bp, int state);
int ota_breakpoint_update_file_state(struct ota_breakpoint *bp, struct ota_file *file,
		int state, int cur_file_offset, int cur_orig_offset, int force);

int ota_breakpoint_init_default_value(struct ota_breakpoint *bp);

int ota_breakpoint_init(struct ota_breakpoint *bp);
void ota_breakpoint_exit(struct ota_breakpoint *bp);

#endif /* __OTA_BREAKPOINT_H__ */
