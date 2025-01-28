/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Actions LEOPARD family boot related infomation public interfaces.
 */

#include <kernel.h>
#include <device.h>
#include <string.h>
#include "soc_boot.h"
#include <linker/linker-defs.h>

 /*mbrec mem layout*/
 /*
 0x1000000-0x1000200 //sram 1KB save boot_info(0x20) +NANDID
 0x1000020-0x1000400  // NANDID
 */



uint32_t soc_boot_get_part_tbl_addr(void)
{
	return soc_boot_get_info()->param_save_addr;
}

uint32_t soc_boot_get_fw_ver_addr(void)
{
	return (soc_boot_get_part_tbl_addr() + SOC_BOOT_FIRMWARE_VERSION_OFFSET);
}

const boot_info_t *soc_boot_get_info(void)
{
	return (const boot_info_t *)BOOT_INFO_SRAM_ADDR;
}

uint32_t soc_boot_get_nandid_tbl_addr(void)
{
	const boot_info_t *p_boot_info = soc_boot_get_info();
	return p_boot_info->nand_id_offs;
}

u32_t soc_boot_get_reboot_reason(void)
{
	const boot_info_t *p_boot_info = soc_boot_get_info();
	return p_boot_info->reboot_reason;
}

bool soc_boot_get_watchdog_is_reboot(void)
{
	const boot_info_t *p_boot_info = soc_boot_get_info();
	return !!p_boot_info->watchdog_reboot;
}

__sleepfunc bool soc_psram_is_apm(void)
{
	const boot_info_t *p_boot_info = soc_boot_get_info();
	return (p_boot_info->is_apm == 1);
}

