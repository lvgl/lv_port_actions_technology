/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Actions LARK family boot related infomation public interfaces.
 */

#include <kernel.h>
#include <device.h>
#include <string.h>
#include "soc.h"
#include <linker/linker-defs.h>
#include <partition/partition.h>
#include <drivers/flash.h>
#include <board_cfg.h>
#include "spicache.h"


 /*mbrec mem layout*/
 /*
 0x2ff60000-0x2ff60200 //sram 1KB save boot_info(0x20) +NANDID
 0x2ff60020-0x2ff60400  // NANDID
 */
#define BOOT_INFO_SRAM_ADDR	(CONFIG_SRAM_BASE_ADDRESS-0x400)


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

void soc_boot_set_system_phy_addr(uint32_t system_phy_addr)
{
	boot_info_t *p_boot_info = (boot_info_t*)soc_boot_get_info();
	p_boot_info->system_phy_addr = system_phy_addr;
}

#ifdef CONFIG_SPI_FLASH_ACTS
void  sys_norflash_exit_4b(void);
#endif

#ifdef CONFIG_PM_DEVICE
extern int pm_power_off_devices(void);
#endif


#if CONFIG_FLASH_LOAD_OFFSET == 0  //app
	 #ifdef CONFIG_SOC_SIGN_ENABLE
	 #undef CONFIG_SOC_SIGN_ENABLE
	 #endif
#endif

 #ifdef CONFIG_SOC_SIGN_ENABLE

#include "soc_rsa.c"

static uint8_t g_key_data[0x400] __aligned(8);
static uint16_t g_key_len;
#define BOOT_RUN_ADDR 0x11000000
#define APP_RUN_ADDR 0x10000000

void dbg_printk_uint(unsigned int val);
static uint32_t  app_part_offset;
__ramfunc const void *cache_to_dma_addr(const void * vaddr)
{
   uint32_t addr = (uint32_t)vaddr;
   dbg_printk_uint(addr);
   if(addr < SPI0_UNCACHE_ADDR) {// nor
	   vaddr = (const void *)(SPI0_UNCACHE_ADDR + addr - APP_RUN_ADDR + app_part_offset);
	   dbg_printk_uint((uint32_t)vaddr);
	   sys_set_bit(SPICACHE_CTL, 9);
   }
   return vaddr;
}

static void sign_load_key(void)
{
	uint16_t key_len;
    uint8_t *key;
	key = p_brom_api->p_image_tlv_find((void*)BOOT_RUN_ADDR, IMAGE_TLV_PUBKEY, &key_len, 1);
	if(key == NULL){
		printk("sign_load_key fail\n");
		g_key_len = 0;
	}else{
		g_key_len = key_len;
		memcpy(g_key_data, key, key_len);
		printk("sign_load_key ok, key_len=0x%x, 0x%x\n", key_len, g_key_data[0]);
	}

}

 /*0 is ok else fail*/
__ramfunc int image_security_data_check(image_head_t *ih_app)
{
	uint8_t *sig_data;
	uint32_t all_len, t;
	uint16_t sig_len;
	int ret;

	//sys_write32(0x1, T0_CTL);//stop and clear pending  // for brom p_verify_signature api
	//sys_write32(0xffffffff, T0_VAL);
	/* enable timer */
	//sys_write32(0x24, T0_CTL); // for brom p_verify_signature api

	sig_data =p_brom_api->p_image_tlv_find(ih_app, IMAGE_TLV_RSA2048_PSS, &sig_len, 0);
	if(sig_data == NULL){
		return 0x1001;
	}

	if(!g_key_len)
		return 0x1002;
	all_len =  ih_app->ih_hdr_size + ih_app->ih_img_size + ih_app->ih_ptlv_size;
	dbg_printk_uint(all_len);
	t = sys_read32(T2_CNT);
    //return p_brom_api->p_verify_signature(g_key_data, sig_data, (const uint8_t *)(cache_to_dma_addr(ih_app)), all_len);
    #ifdef CONFIG_SHA256_HARDWARE
	ret = verify_signature(g_key_data, sig_data, (const uint8_t *)(cache_to_dma_addr(ih_app)), all_len);
	#else
	dbg_printk_uint((uint32_t)(ih_app));
	ret =  verify_signature(g_key_data, sig_data, (const uint8_t *)(ih_app), all_len);
	#endif
	dbg_printk_uint(sys_read32(T2_CNT)-t);
	return ret;
}

#else
__ramfunc int image_security_data_check(image_head_t *ih_app)
{
	return 0;
}
static void sign_load_key(void)
{

}

#endif



extern uint32_t libboot_version_dump(void);

int boot_to_part(int part_type, int mirror_id, int flashid)
{
	const struct partition_entry *part;
	int crc_is_enabled;

#ifdef CONFIG_ACTIONS_PRINTK_DMA
	printk_dma_switch(0);
#endif
	part = partition_get_part_by_type_mirrorid(part_type, mirror_id);
	if(part == NULL){
		printk("not find  app parition--\n");
		return -1;
	}
	crc_is_enabled = part->flag & PARTITION_FLAG_ENABLE_CRC ? 1 : 0;
	printk("app offset=0x%x ,crc=%d\n", part->file_offset, crc_is_enabled);
	soc_boot_set_system_phy_addr(part->file_offset);
#ifdef CONFIG_SOC_SIGN_ENABLE
	app_part_offset = part->file_offset;
#endif

	if(flashid == BOOT_FLASH_ID_NAND){
		boot_nand(part->file_offset, 0);
	}else if(flashid == BOOT_FLASH_ID_EXTNOR){	
#ifdef CONFIG_SPI_FLASH_ACTS
		#ifdef CONFIG_SPI_NOR_FLASH_4B_ADDRESS
		sys_norflash_exit_4b();
		#endif
#endif
		boot_ext_nor(part->file_offset, crc_is_enabled, 0);
	}else{
		boot_nor(part->file_offset, crc_is_enabled, 0);
	}
	printk("boot fail\n");

	__asm__ volatile("cpsie	i");
	__asm__ volatile("dsb");

	return 0;
}
static void boot_clear_soc(void)
{
	int i;
	for(i = 0; i < CONFIG_NUM_IRQS; i++)  // all irq disable
		irq_disable(i);
	for(i = 0; i < GPIO_MAX_IRQ_GRP; i++ ){ // clear all gpio pending
		sys_write32(0xffffffff, GPIO_REG_IRQ_PD(GPIO_REG_BASE, i*32));
	}
}


int boot_to_app(int mirror_id, int flashid)
{
	libboot_version_dump();
	printk("boot main=%d \n", flashid);
	irq_lock();
	sign_load_key();
	boot_clear_soc();
#ifdef CONFIG_PM_DEVICE
	printk("dev power off\n");
	pm_power_off_devices();
	printk("dev power end\n");
#endif
#ifdef CONFIG_SPI_XIP_READ
	soc_memctrl_unmapping(CONFIG_SPI_XIP_VADDR);
#endif
	return boot_to_part(PARTITION_TYPE_SYSTEM, mirror_id, flashid);
}


