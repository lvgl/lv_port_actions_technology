/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Actions LARK family boot related infomation public interfaces.
 */
#ifndef __SOC_BOOT_H
#define __SOC_BOOT_H

/* The macro to define the mapping address of SPI1 cache for PSRAM which configured by bootloader stage */
#define SOC_BOOT_PSRAM_MAPPING_ADDRESS			(0x18000000)

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
	uint32_t bit_res : 30; /* The indicator of launching system is a mirror partition */
	uint32_t reserved[2];
	uint32_t param_phy_addr;  /* The physical address of PARAM that current system using */
	uint32_t nor_offset; /*cur code mapping off nor offset*/
	uint32_t code_len; // code in psram len(nand boot/emmc boot use)
	uint32_t reserved0[1];
	uint32_t dvfs;
	uint32_t reserved1[2];	
} boot_info_t;


#define IMAGE_TLV_KEYHASH           0x01   /* hash of the public key */
#define IMAGE_TLV_PUBKEY            0x02   /* public key */
#define IMAGE_TLV_SHA256            0x04   /* SHA256 of image hdr and body */
#define IMAGE_TLV_RSA2048_PSS       0x08   /* RSA2048 of hash output */

#define IMAGE_MAGIC0      0x48544341
#define IMAGE_MAGIC1      0x41435448

typedef struct image_head {
	uint32_t  ih_magic0;
	uint32_t  ih_magic1;   //
	uint32_t  ih_load_addr;    /*load addr, include header*/
	uint8_t   ih_name[8];  //
	uint32_t  ih_entry;
	uint32_t ih_img_size;
	uint32_t ih_img_chksum;   /*if not sign, use check sum��ih_img_size not include header */
	uint32_t ih_hdr_chksum;      /* include header. */
	uint16_t ih_hdr_size;       /* Size of image header (bytes). */
	uint16_t ih_ptlv_size;   /* Size of protected TLV area (bytes). */
	uint16_t ih_tlv_size;    /*tlv size*/
	uint16_t ih_version;
	uint32_t ih_flags;
	uint8_t  ih_ext_ictype[7];
	uint8_t  ih_storage_type;   // mbrec
}image_head_t;


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


#define BOOT_FLASH_ID_NOR 		0
#define BOOT_FLASH_ID_EXTNOR 	1
#define BOOT_FLASH_ID_NAND	 	2


/* @brief run app 
	mirror_id: defualt 0 is boot app,  if have A&B app, 0 is A, 1 is B
	flashid: 0 is nor, 1 is ext nor
*/

int boot_to_app(int mirror_id, int flashid);

/* @brief boot from ext nor 	
*/
int boot_ext_nor(uint32_t offset, int enable_crc, uint32_t head_len);
/* @brief boot from  nor 	
*/
int boot_nor(uint32_t offset, int enable_crc, uint32_t head_len);

/* @brief boot from  nand 	
*/
int boot_nand(uint32_t offset, uint32_t head_len);


 /* @brief check app by signature verification
 */
int image_security_data_check(image_head_t *ih_app);


#endif
