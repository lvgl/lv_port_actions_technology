/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief local player breakpoint.
 */

#include "lcmusic.h"
#include <property_manager.h>

void _lcmusic_bpinfo_save(const char *dir, struct music_bp_info_t music_bp_info)
{
	int ret = 0;
	char disk_tag_bpinfo[14] = {0};

	SYS_LOG_INF("bp time:%d ms,file:%d bytes\n", music_bp_info.bp_info.time_offset, music_bp_info.bp_info.file_offset);

	if (strstr(dir, "/SD:")) {
		strcpy(disk_tag_bpinfo, "SDCAR_BP_INFO");
	} else if (strstr(dir, "/USB:")) {
		strcpy(disk_tag_bpinfo, "USB_BP_INFO");
	} else if (strstr(dir, "/NAND:")) {
		strcpy(disk_tag_bpinfo, "NAND_BP_INFO");
	} else {
		strcpy(disk_tag_bpinfo, "NOR_BP_INFO");
	}
#ifdef CONFIG_PROPERTY
	ret = property_set(disk_tag_bpinfo,
					 (char *)&music_bp_info, sizeof(struct music_bp_info_t));
#endif
	if (ret < 0) {
		SYS_LOG_ERR("failed to set BPINFO %s, ret %d\n",
						 "SDCAR_BP_INFO", ret);
	}
}

int _lcmusic_bpinfo_resume(const char *dir, struct music_bp_info_t *music_bp_info)
{
	char disk_tag_bpinfo[14] = {0};
	int res = -EINVAL;

	if (strstr(dir, "/SD:")) {
		strcpy(disk_tag_bpinfo, "SDCAR_BP_INFO");
	} else if (strstr(dir, "/USB:")) {
		strcpy(disk_tag_bpinfo, "USB_BP_INFO");
	} else if (strstr(dir, "/NAND:")) {
		strcpy(disk_tag_bpinfo, "NAND_BP_INFO");
	}  else {
		strcpy(disk_tag_bpinfo, "NOR_BP_INFO");
	}

#ifdef CONFIG_PROPERTY
	res = property_get(disk_tag_bpinfo, (char *)music_bp_info, sizeof(struct music_bp_info_t));
#endif
	SYS_LOG_INF("music_bp_info:time_offset=%d,file_offset=%d\n",
	music_bp_info->bp_info.time_offset, music_bp_info->bp_info.file_offset);

	return res;
}
