/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Actions LEOPARD family boot related infomation public interfaces.
 */
#ifndef __SOC_BOOT_H
#define __SOC_BOOT_H

#define BOOT_INFO_SRAM_ADDR	(CONFIG_SRAM_BASE_ADDRESS-0x400)

/* The macro to define the mapping address of SPI1 cache for PSRAM which configured by bootloader stage */
#define SOC_BOOT_PSRAM_MAPPING_ADDRESS			(0x38000000)

/* The total size of ROM data that range from TEXT to RODATA sections */
extern uint32_t z_rom_data_size;

#define	SOC_BOOT_FIRMWARE_VERSION_OFFSET		(0x2e8)	/* The offset of firmware version table within system parameter paritition */

/*!
 * struct boot_info_t
 * @brief The boot infomation that transfer from bootloader to system.
 */
typedef struct {
	uint32_t mbrec_phy_addr; /* The physical address of MBREC that current system running */
	uint32_t param_save_addr; /* PARAM save addr(xip/sram/psram)*/
	uint32_t system_phy_addr; /* The physical address of SYSTEM that current using */
	uint32_t reboot_reason; /* Reboot reason */
	uint32_t nand_id_offs; /* nand id table offset */
	uint32_t nand_id_len; /* nand id table length */
	uint32_t watchdog_reboot : 1; /* The reboot event that occured from bootrom watchdog expired */
	uint32_t is_mirror : 1; /* The indicator of launching system is a mirror partition */
	uint32_t is_apm : 1; /* The indicator of 3085s4 is apm */
	uint32_t is_psram_off : 1; /* The indicator of psram power can off*/
	uint32_t bit_res : 28; /* The indicator of launching system is a mirror partition */
	uint32_t reserved[2];
	uint32_t param_phy_addr;  /* The physical address of PARAM that current system using */
	uint32_t nor_offset; /*cur code mapping off nor offset*/
	uint32_t code_len; // code in psram len(nand boot/emmc boot use)
	uint32_t reserved0[1];
	uint32_t dvfs;
	uint32_t reserved1[2];	
} boot_info_t;


/* @brief The function to get the address of current partition table */
uint32_t soc_boot_get_part_tbl_addr(void);

/* @brief The function to get the address of current firmware version */
uint32_t soc_boot_get_fw_ver_addr(void);

/* @brief The function to get the current boot infomation  */
const boot_info_t *soc_boot_get_info(void);

/* @brief The function to get the reboot reason */
u32_t soc_boot_get_reboot_reason(void);

/* @brief The function to get the indicator of watchdog reboot */
bool soc_boot_get_watchdog_is_reboot(void);

/* @brief The function to get the address of nand id table */
uint32_t soc_boot_get_nandid_tbl_addr(void);

/* @brief return mini type */
inline bool soc_boot_is_mini(void)
{
	return (((const boot_info_t *)BOOT_INFO_SRAM_ADDR)->dvfs == 0x23185C0c) ;
}

 bool soc_psram_is_apm(void);

#endif
