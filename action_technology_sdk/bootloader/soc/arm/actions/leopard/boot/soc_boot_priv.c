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

#define APP_RUN_ADDR 0x10000000
#define BOOT_RUN_ADDR 0x11000000
#define APP_STOARGE_ADDR 0x39000000
#define APP_STOARGE_END 0x39800000

#if 0
#define APP_STOARGE_ADDR_END 0x39200000
#define APP_MAX_LEN	 (APP_STOARGE_ADDR_END - APP_STOARGE_ADDR)
#define SRAM_ADDR    0x01000000
#define SRAM_SIZE    0x0001c000
/*for nand boot*/
#define NAND_APP_PSRAM	0x39600000
#define NAND_APP_MAX_LEN	 (APP_STOARGE_ADDR_END - NAND_APP_PSRAM)
#endif


#define CONFIG_BOOT_DBG

#ifdef CONFIG_BOOT_DBG
/* UART registers struct */
struct acts_uart_reg {
    volatile uint32_t ctrl;

    volatile uint32_t rxdat;

    volatile uint32_t txdat;

    volatile uint32_t stat;

    volatile uint32_t br;
} ;

__ramfunc static void uart_poll_my_ch(int c)
{
	struct acts_uart_reg *uart = (struct acts_uart_reg*)UART0_REG_BASE;
	/* Wait for transmitter to be ready */
	while (uart->stat &  BIT(6));
	/* send a character */

	uart->txdat = (uint32_t)c;

}
__ramfunc static void uart_out_ch(char c)
{
	/* send a character */
	if(c == '\n')
		uart_poll_my_ch('\r');
	uart_poll_my_ch(c);
}
__ramfunc static char int_to_hex(unsigned int val)
{
		if(val < 10)
			return '0'+ val;
		else
			return 'A'+ (val-10);
}
__ramfunc  void dbg_printk_uint(unsigned int val)
{
	int i;
	uart_out_ch('0');
	uart_out_ch('x');
	for(i = 0; i < 8; i++){
		uart_out_ch(int_to_hex((val&0xf0000000)>> 28));
		val = val << 4;
	}
	uart_out_ch('\n');
}

#else
#define   dbg_printk_uint(unsigned int val)  do{}while(0)
#endif

typedef void (*app_main_t)(void);


#if 0
static void dump_mamp(void)
{
	int i;
	uint32_t val;
	for (i = 0;  i < CACHE_MAPPING_ITEM_NUM ; i++) {
		val = sys_read32(SPI_CACHE_MAPPING_ADDR0 + i * 8);
		if (val) {
			printk("slot=%d  to map nor phy addr 0x%x, map=0x%x\n", i,
					sys_read32(SPI_CACHE_ADDR0_ENTRY + i * 8), val&(~0xf));

		}else{
			printk("slot=%d not map\n", i);
		}
	}
}
#endif


__ramfunc static void cache0_invalid(void)
{
	sys_write32(1, SPICACHE_INVALIDATE);//cache0 invalid
	while ((sys_read32(SPICACHE_INVALIDATE) & 0x1) == 1);
}

__ramfunc void cache0_map(uint32_t slot, uint32_t addr, uint32_t offset)
{
	sys_write32(addr | 0xF, SPI_CACHE_MAPPING_ADDR0 + slot * 8);
	sys_write32(offset, SPI_CACHE_ADDR0_ENTRY + slot * 8);
	cache0_invalid();
}

__ramfunc static void cache0_unmap(uint32_t slot)
{
	sys_write32(0x0, SPI_CACHE_ADDR0_ENTRY + slot * 8);
	sys_write32(0, SPI_CACHE_MAPPING_ADDR0 + slot * 8);
	cache0_invalid();
}
__ramfunc static void code_flush(void)
{
	sys_write32(0x1f, SPI1_CACHE_OPERATE);
	while ((sys_read32(SPI1_CACHE_OPERATE) & 0x1) == 1);
	sys_write32(0x19, SPI1_CACHE_OPERATE);
	while ((sys_read32(SPI1_CACHE_OPERATE) & 0x1) == 1);
}
static void map_clear(uint32_t start_slot)
{
	int i;
	for(i = start_slot; i < CACHE_MAPPING_ITEM_NUM; i++)
		cache0_unmap(start_slot);
}

static unsigned int code_max_len;

static int  get_sdfs_offset_size(uint32_t *sys_offset, uint32_t *sys_size,  uint32_t *offset, uint32_t *size, int *sdfs_crc)
{
	const struct partition_entry *part, *sys_part;

	sys_part =  partition_get_part(PARTITION_FILE_ID_SYSTEM);
	if(sys_part == NULL){
		printk("invalid app part--\n");
		return -1;
	}
	*sys_offset = sys_part->offset;
	*sys_size =  sys_part->size;
	*offset = sys_part->offset+sys_part->size;
	*size = 0;
	part = partition_get_part(PARTITION_FILE_ID_SDFS);
	if(part == NULL){
		printk("not sdfs part--\n");
		return 0;
	}
	if(sys_part->size + part->size > code_max_len){
		printk("error, part size 0x%xB over 0x%xB --\n", sys_part->size + part->size, code_max_len);
		return -1;
	}
	*sdfs_crc = part->flag & PARTITION_FLAG_ENABLE_CRC ? 1 : 0;
	printk("sys=0x%x,0x%x,sdfs=0x%x,0x%x, crc=%d\n", sys_part->offset, sys_part->size,  part->offset, part->size, *sdfs_crc);
	*offset = part->offset;
	*size = part->size;

	return 0;
}

__ramfunc static unsigned int image_checksum32(void *data, unsigned int len)
{
	uint32_t i, cs = 0;
	uint32_t *p = (uint32_t *)data;
	for(i = 0; i < len/4; i++)
		cs += p[i];
	return cs;
}


static unsigned int g_psp;
__ramfunc static int app_image_check(uint32_t *buf, uint32_t *run_p, uint32_t code_max_len)
{
	uint32_t run_addr, ck_sum, run_off;
	int ret;
	image_head_t *head;
	head = (image_head_t *)&buf[0];	
	run_off = 0x204;
	if(head->ih_magic0 != IMAGE_MAGIC0 || head->ih_magic1 != IMAGE_MAGIC1){
		head = (image_head_t *)&buf[0x200/4];
		if(head->ih_magic0 != IMAGE_MAGIC0 || head->ih_magic1 != IMAGE_MAGIC1){
			head = (image_head_t *)&buf[0];
			if(head->ih_img_size != 0){ //  not keil download
				dbg_printk_uint(0x10);
				return -10;
			}
		}else{
			run_off = 4;
		}
	}
	g_psp = buf[run_off/4 - 1];
	run_addr = buf[run_off/4];
	if ((run_addr < APP_RUN_ADDR) || (run_addr > APP_RUN_ADDR+code_max_len) || ((run_addr & 0x1) != 1)){
		dbg_printk_uint(0x12);
		dbg_printk_uint(run_addr);
		return -12;
	}
	if(run_p != NULL)
		*run_p = run_addr;
	if(head->ih_img_size == 0){ // keil download
		dbg_printk_uint(0x01);
		dbg_printk_uint(run_addr);
		return 0;
	}
	if(code_max_len && head->ih_img_size > code_max_len){
		dbg_printk_uint(0x13);
		dbg_printk_uint(head->ih_img_size);
		return -13;
	}

	if(0xFFFFFFFF != image_checksum32(head, head->ih_hdr_size)) {
		dbg_printk_uint(0x14);
		return -14;
	}
	//cs = head->ih_img_chksum;
	//head->ih_hdr_chksum = 0; // calc img checksum
	if(run_off == 0x204) {
		#define IMAGE_F_ENCRYPTED 0x04
		if((head->ih_flags & IMAGE_F_ENCRYPTED)){
			dbg_printk_uint(0x66660000);
			ret = image_security_data_check((image_head_t *)APP_RUN_ADDR);
			dbg_printk_uint(ret);
			if(ret)
				ck_sum = 0;
			else
				ck_sum = 0xFFFFFFFF; //sign check ok
		}else{
			ck_sum = image_checksum32(buf+head->ih_hdr_size/4, head->ih_img_size) + head->ih_img_chksum;
		}
	}else{
		ck_sum = image_checksum32(buf, head->ih_img_size) - head->ih_hdr_chksum;
	}
	if(ck_sum != 0xFFFFFFFF){
		dbg_printk_uint(0x15);
		dbg_printk_uint(ck_sum);
		return -15;
	}
	dbg_printk_uint(0x00);
	dbg_printk_uint(run_addr);
	return 0;
}

static void  soc_set_app_map_for_sleep(uint32_t offset, int enable_crc)
{
	uint32_t *poff =(uint32_t *)soc_boot_get_info();
	if (enable_crc) {
		offset |= (1 << 31);
	}
	poff[10] = offset;
}

static __ramfunc  void run_app(unsigned int p_sp, unsigned run_addr)
{
	app_main_t main_fun;
	__asm__ volatile(
		"msr psp, %0;"
		:  : "r"(p_sp) : "memory");

	main_fun =(app_main_t) run_addr;
	main_fun();
}


__ramfunc static int __boot_ext_nor(uint32_t offset, int enable_crc, uint32_t head_len)
{
	int ret,sdfs_crc;
	uint32_t run_addr;

	uint32_t *pbuf = (uint32_t *)(APP_RUN_ADDR+head_len);
	uint32_t i, *ptmp;
	uint32_t sdfs_offset, sdfs_size, sys_a_off, sys_a_size, sdfs_start;

	__asm__ volatile("cpsid	i");
	__asm__ volatile("dsb");
	map_clear(3);

	sdfs_crc = enable_crc;
	if(get_sdfs_offset_size(&sys_a_off, &sys_a_size, &sdfs_offset, &sdfs_size, &sdfs_crc))
		return -1;
	sdfs_start = sdfs_offset-sys_a_off;

	sys_write32(1 | (1<<5) , SPICACHE_CTL); //spi0cache to nor
	cache0_unmap(0);

	if (enable_crc) {
		cache0_map(0, APP_RUN_ADDR, offset|(1 << 31));
	}else{
		cache0_map(0, APP_RUN_ADDR, offset);
	}

	ret = app_image_check((uint32_t *)APP_RUN_ADDR, &run_addr, code_max_len);

	if (ret) {
		sys_write32(1 | (1<<15) | (1<<5) , SPICACHE_CTL); //spi0cache for code, lowpower mode
		cache0_unmap(0);
		cache0_map(0, BOOT_RUN_ADDR, 0);
		return ret;
	}
	


	ptmp = (uint32_t *)APP_STOARGE_ADDR;
	//if(offset == sys_a_off){// A
		//for(i = 0; i <(sdfs_start+sdfs_size)/4; i++)
				//ptmp[i] = pbuf[i];
	//}else{// B
	for(i = 0; i <(sdfs_start)/4; i++) // copy sys
			*ptmp++ = *pbuf++;
	if(sdfs_size){
		cache0_unmap(0);
		if (sdfs_crc) {
			cache0_map(0, APP_RUN_ADDR, sdfs_offset|(1 << 31));
		}else{
			cache0_map(0, APP_RUN_ADDR, sdfs_offset);
		}
		pbuf = (uint32_t *)(APP_RUN_ADDR);
		for(i = 0; i <(sdfs_size)/4; i++) // sdfs
			*ptmp++ = *pbuf++;
	}
	//}
	//memcpy((void *)APP_STOARGE_ADDR, (void *)APP_RUN_ADDR, 0x200000);
	code_flush();
	cache0_unmap(0);
	sys_write32(1 | (1<<15) | (1<<5) , SPICACHE_CTL); //spi0cache for code
	cache0_map(0, APP_RUN_ADDR, 0);
	run_app(g_psp, run_addr);
	return 0;
}

static void boot_code_len_init(void)
{
	const boot_info_t * bf =  soc_boot_get_info();
	code_max_len = bf->code_len;
	if(code_max_len == 0){
		printk("code len use default\n");
		code_max_len = 0x800000;
	}
	printk("app code len =0x%x\n", code_max_len);
}
/*code run in psram*/
int boot_ext_nor(uint32_t offset, int enable_crc, uint32_t head_len)
{
	int ret;
	boot_code_len_init();
	soc_set_app_map_for_sleep(offset,enable_crc);
	ret = __boot_ext_nor(offset, enable_crc, head_len);
	printk("boot ext fail ret=%d\n", ret);
	return ret;
}

 __ramfunc int boot_nor_ram(uint32_t offset, int enable_crc, uint32_t head_len)
{
	uint32_t boot_entry, boot_addr;
	//volatile int loop =1;
	int ret;
	//uint32_t *pbuf = (uint32_t *)(APP_RUN_ADDR+head_len);
	__asm__ volatile("cpsid	i");
	__asm__ volatile("dsb");
	map_clear(3);
	boot_entry = sys_read32(SPI_CACHE_ADDR0_ENTRY);
	boot_addr = sys_read32(SPI_CACHE_MAPPING_ADDR0);

	if (enable_crc) {
		offset |= (1 << 31);
	}

	cache0_map(0, APP_RUN_ADDR|0xf, offset); // map 16MB
	ret = app_image_check((uint32_t *)APP_RUN_ADDR, &boot_addr, code_max_len);
	if (ret) {
		cache0_unmap(0);
		sys_write32(boot_addr, SPI_CACHE_MAPPING_ADDR0); // recovery to bootloader
		sys_write32(boot_entry, SPI_CACHE_ADDR0_ENTRY);
		return ret;
	}
	run_app(g_psp, boot_addr);
	return 0;
}

 /*code run in nor*/
 int boot_nor(uint32_t offset, int enable_crc, uint32_t head_len)
{
	int ret;
	boot_code_len_init();
	soc_set_app_map_for_sleep(offset,enable_crc);
	ret = boot_nor_ram(offset, enable_crc, head_len);
	printk("boot fail ret=%d\n", ret);
	return ret;
}
 
 __ramfunc static int __boot_nand(uint32_t offset,  uint32_t head_len)
 {
	 int ret;
	 uint32_t boot_addr, size;
	 uint32_t i, *pc;
	 uint32_t *pbuf;

	 size = *((uint32_t *)(APP_STOARGE_ADDR+code_max_len -4));
	 if(size > (code_max_len-0x400))
	 	size  = code_max_len-0x400;
	 printk("app len=0x%x\n",size);
	 
	 __asm__ volatile("cpsid i");
	 __asm__ volatile("dsb");
	 map_clear(3);
	 pbuf = (uint32_t *)(APP_STOARGE_END-code_max_len +head_len);
	 ret = app_image_check(pbuf, &boot_addr, code_max_len); 
	 if (ret) {
		pbuf = (uint32_t *)(APP_STOARGE_ADDR);
		ret = app_image_check(pbuf, &boot_addr, code_max_len);
		if(ret)
			return ret;
	 } else {
		pc = (uint32_t *)APP_STOARGE_ADDR;
		for(i = 0; i < size/4; i++)
			pc[i] = pbuf[i];
	 }
	 code_flush();
	 cache0_unmap(0);
	 sys_write32(1 | (1<<15) | (1<<5) , SPICACHE_CTL); //spi0cache for code
	 cache0_map(0, APP_RUN_ADDR, 0x0);
	 run_app(g_psp, boot_addr);;
	 return 0;
 }
 
 #if 0
 __ramfunc static int __boot_nand_mini(uint32_t offset)
 {
	 int ret;
	 uint32_t boot_addr, size;
	 app_main_t main_fun;
	 uint32_t *pbuf = (uint32_t *)(APP_STOARGE_ADDR);

	 size = *((uint32_t *)APP_STOARGE_ADDR_END-4);
	 if(size > (APP_MAX_LEN-0x400))
	 	size  = APP_MAX_LEN-0x400;
	 printk("app len=0x%x\n",size);
	 
	 __asm__ volatile("cpsid i");
	 __asm__ volatile("dsb");
	 map_clear(3);

	 ret = app_image_check(pbuf, &boot_addr, APP_MAX_LEN); 
	 if (ret) {
		 return ret;
	 }
	 main_fun =(app_main_t) boot_addr;
	 code_flush();
	 cache0_unmap(0);
	 sys_write32(1 | (1<<15) | (1<<5) , SPICACHE_CTL); //spi0cache for code
	 cache0_map(0, APP_RUN_ADDR, 0x0);
	 main_fun();
	 return 0;
 }
 #endif

 /*code run in psram*/
  int boot_nand(uint32_t offset, uint32_t head_len)
 {
	int ret;
	boot_code_len_init();
	ret = __boot_nand(offset,  0);  // 3085c
	printk("boot fail ret=%d\n", ret);
	return ret;
 }



