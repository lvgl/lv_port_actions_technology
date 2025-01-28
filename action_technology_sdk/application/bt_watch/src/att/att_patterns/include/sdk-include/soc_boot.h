/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file boot related interface
 */

#ifndef	_ACTIONS_SOC_BOOT_H_
#define	_ACTIONS_SOC_BOOT_H_

/*
 * current firmware's boot file always mapping to 0x100000,
 * the mirrored boot mapping to 0x200000
 */
#define	SOC_BOOT_A_ADDRESS				(0x100000)
#define	SOC_BOOT_B_ADDRESS				(0x200000)

#define	SOC_BOOT_PARTITION_TABLE_OFFSET			(0x0e00)
#define	SOC_BOOT_FIRMWARE_VERSION_OFFSET		(0x0f80)

#ifndef _ASMLANGUAGE

static inline u32_t soc_boot_get_part_tbl_addr(void)
{
	return (SOC_BOOT_A_ADDRESS + SOC_BOOT_PARTITION_TABLE_OFFSET);
}

static inline u32_t soc_boot_get_fw_ver_addr(void)
{
	return (SOC_BOOT_A_ADDRESS + SOC_BOOT_FIRMWARE_VERSION_OFFSET);
}

#endif /* _ASMLANGUAGE */

#endif /* _ACTIONS_SOC_BOOT_H_	*/
