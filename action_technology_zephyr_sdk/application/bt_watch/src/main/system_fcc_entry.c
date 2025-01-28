/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system app main
 */
#include <mem_manager.h>
#include <msg_manager.h>
//#include <fw_version.h>
#include <sys_event.h>
#include <sys_manager.h>
#include "app_switch.h"
#include "system_app.h"
#include "app_ui.h"
#include <stream.h>
#include <property_manager.h>
#include <soc.h>
#include <sdfs.h>

#ifdef CONFIG_BLUETOOTH
#include "mem_manager.h"
#include "btservice_api.h"
#endif

#ifdef CONFIG_BT_CONTROLER_RF_FCC

#define fccdrv_name  "fccdrv.bin"

int system_enter_FCC_mode(void)
{
	int ret;
	uint32_t fcc_entry_para;
	int prio_backup;
	struct sd_file *fccdrv_hd;
	int fccdrv_len, read_len;

	SYS_LOG_INF("prepare FCC test\n");

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "fcc");
#endif

	prio_backup = k_thread_priority_get(k_current_get());
	k_thread_priority_set(k_current_get(), 0);

	/* load fcc drv from sdfs */
	fccdrv_hd = sd_fopen(fccdrv_name);
	if (fccdrv_hd == NULL) {
		SYS_LOG_ERR("%s not find in sdfs\n", fccdrv_name);
		return -1;
	}

	sd_fseek(fccdrv_hd, 0, FS_SEEK_END);
	fccdrv_len = sd_ftell(fccdrv_hd);
	SYS_LOG_INF("fccdrv_len = %d\n", fccdrv_len);
	sd_fseek(fccdrv_hd, 0, FS_SEEK_SET);
	
	read_len = sd_fread(fccdrv_hd, (void *)(0xbfc3fa00), fccdrv_len);
	SYS_LOG_INF("read_len = %d\n", read_len);
	
	sd_fclose(fccdrv_hd);

	/* install fcc drv */
	fcc_entry_para = 0;
	fcc_entry_para |= (0 << FCC_WORD_MODE_SHIFT);
	fcc_entry_para |= (CONFIG_BT_FCC_UART_PORT << FCC_UART_PORT_SHIFT);
	fcc_entry_para |= ((FCC_UART_BAUD/100)<<FCC_UART_BAUD_SHIFT);
	ret = system_install_fcc_drv(fcc_entry_para);

	k_thread_priority_set(k_current_get(), prio_backup);

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "fcc");
#endif

	/* just switch to bqb test will return, so reboot to bqb test */
	sys_pm_reboot(REBOOT_TYPE_GOTO_SYSTEM|REBOOT_REASON_GOTO_BQB);

	return ret;
}

#endif
