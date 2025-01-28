/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/flash.h>
#include <drivers/spi.h>
#include <logging/log.h>
#include <soc.h>
#include <board_cfg.h>
#include "spi_flash.h"
#include <linker/linker-defs.h>
#include <dvfs.h>
#include <drivers/gpio.h>

/* spinor parameters */
#define SPINOR_WRITE_PAGE_SIZE_BITS	8
#define SPINOR_ERASE_SECTOR_SIZE_BITS	12
#define SPINOR_ERASE_BLOCK_SIZE_BITS	16

#define SPINOR_WRITE_PAGE_SIZE		(1 << SPINOR_WRITE_PAGE_SIZE_BITS)
#define SPINOR_ERASE_SECTOR_SIZE	(1 << SPINOR_ERASE_SECTOR_SIZE_BITS)
#define SPINOR_ERASE_BLOCK_SIZE		(1 << SPINOR_ERASE_BLOCK_SIZE_BITS)
#define SPINOR_WRITE_PAGE_MASK		(SPINOR_WRITE_PAGE_SIZE - 1)
#define SPINOR_ERASE_SECTOR_MASK	(SPINOR_ERASE_SECTOR_SIZE - 1)
#define SPINOR_ERASE_BLOCK_MASK		(SPINOR_ERASE_BLOCK_SIZE - 1)


#define	SPINOR_CMD_PROGRAM_ERASE_RESUME		0x7a  	/* nor resume */
#define	SPINOR_CMD_PROGRAM_ERASE_SUSPEND	0x75   /* nor suspend */
#define	SPINOR_CMD_READ_STATUS		0x05	/* read status1 */
#define	SPIMEM_CMD_ENABLE_WRITE		0x06	/* enable write */
#define SPIMEM_TFLAG_WRITE_DATA		0x08
#define SPINOR_CMD_WR_VOL_CFG	 	 0x81  /*Write volatile Configuration Registe*/
#define SPINOR_CMD_RD_VOL_CFG	 	 0x85  /*read  volatile Configuration Registe*/




LOG_MODULE_REGISTER(spi_flash_acts, CONFIG_FLASH_LOG_LEVEL);

#if defined(CONFIG_SPI_FLASH_GPIO_2CS) && (CONFIG_SPI_FLASH_GPIO_2CS == 1)
static const struct device *spi_gpio_cs_dev;
unsigned int nor_cs0_size, nor_cs1_size;
unsigned char nor_cs0_delaytran, nor_cs1_delaytran;

__ramfunc static void spi_flash_acts_cs_gpio(struct spi_info *si, int value)
{
	if (spi_gpio_cs_dev) {
		if (value) {
			sys_write32(GPIO_BIT(CONFIG_SPI_FLASH_1_GPIO_CS_PIN), GPION_BSR(CONFIG_SPI_FLASH_1_GPIO_CS_PIN));
		} else {
			sys_write32(GPIO_BIT(CONFIG_SPI_FLASH_1_GPIO_CS_PIN), GPION_BRR(CONFIG_SPI_FLASH_1_GPIO_CS_PIN));
		}
	}
}

__ramfunc  void spi_flash1_cs_select(struct spinor_info *sni, int select)
{
	if(select){
		if(nor_cs1_size > 0x1000000)
			sni->flag |= SPINOR_FLAG_4BYTE_ADDRESS_MODE_EN;
		else
			sni->flag &= ~SPINOR_FLAG_4BYTE_ADDRESS_MODE_EN;
		sni->spi.set_cs = spi_flash_acts_cs_gpio;
		sni->spi.delay_chain = nor_cs1_delaytran;
	}else{
		if(nor_cs0_size > 0x1000000)
			sni->flag |= SPINOR_FLAG_4BYTE_ADDRESS_MODE_EN;
		else
			sni->flag &= ~SPINOR_FLAG_4BYTE_ADDRESS_MODE_EN;
		sni->spi.set_cs = NULL;
		sni->spi.delay_chain = nor_cs0_delaytran;
	}
}
/*for delaytran scan */
unsigned char spi_flash_set_delaytran(const struct device *dev, off_t offset,  unsigned char delaytran)
{
	unsigned char old;
	struct spinor_info *sni = DEV_DATA(dev);
	if(offset < nor_cs0_size){
		old = nor_cs0_delaytran;
		nor_cs0_delaytran = delaytran;
		sni->spi.delay_chain = nor_cs0_delaytran;
	}else{
		old = nor_cs1_delaytran;
		nor_cs1_delaytran = delaytran;
	}
	return old;
}

#else
/*for delaytran scan */
unsigned char spi_flash_set_delaytran(const struct device *dev, off_t offset,  unsigned char delaytran)
{
	unsigned char old;
	struct spinor_info *sni = DEV_DATA(dev);
	old = sni->spi.delay_chain;
	sni->spi.delay_chain = delaytran;
	return old;
}
#endif


__sleepfunc void spi_flash_exit_continuous(struct spi_info *si)
{
	p_spinor_api->continuous_read_reset((struct spinor_info *)si);
}

__sleepfunc  void spi_flash_acts_prepare(struct spi_info *si)
{
#if defined(CONFIG_SPI_FLASH_GPIO_2CS) && (CONFIG_SPI_FLASH_GPIO_2CS == 1)
	void * bak_cs;
	bak_cs = si->set_cs;
	si->set_cs = NULL;
#endif
	/* wait for spi ready */
	while(!(sys_read32(SPI_STA(si->base)) & SPI_STA_READY)){

	}

	if(!(si->flag & SPI_FLAG_NO_NEED_EXIT_CONTINUOUS_READ)) {
		#ifdef CONFIG_SPI0_NOR_QPI_MODE
		if(!(si->flag & SPI_FLAG_QPI_MODE))
		#endif
			spi_flash_exit_continuous(si);
	}
#if defined(CONFIG_SPI_FLASH_GPIO_2CS) && (CONFIG_SPI_FLASH_GPIO_2CS == 1)
	if(bak_cs != NULL)
		si->set_cs = bak_cs;
#endif
}

#ifdef CONFIG_SPI0_NOR_DTR_MODE

__ramfunc static  void spi0_dtr_set_clk(uint32_t rate_hz)
{
	uint32_t core_pll, div, real_rate, val; 
	core_pll =  MHZ(((sys_read32(COREPLL_CTL)&0x3F)*8));
	div = (core_pll+rate_hz-1)/rate_hz;
	real_rate = core_pll/div;
	val = (div-1)|(1<<8) | (1<<12);
	sys_write32(val, CMU_SPI0CLK);
}

__ramfunc static unsigned int spi0_cache_enter_dtr_mode(struct spinor_info *sni, uint32_t clk_mhz)
{
	struct acts_spi_reg *spi= (struct acts_spi_reg *)sni->spi.base;
	spi->ctrl |= 1<<19;
	spi->delaychain |=  1<< 8;  // 10 dummy
	//sys_set_bit(CMU_SPI0CLK, 12); // CLKD DDR MODE
	spi0_dtr_set_clk(MHZ(clk_mhz) * 2);
	return  p_spinor_api->read_chipid(sni)& 0xffffff;// read chipid for exit contimue mode;
}
#if 0
__ramfunc static unsigned int spi0_cache_exit_dtr_mode(struct spinor_info *sni)
{
	//struct acts_spi_reg *spi= (struct acts_spi_reg *)sni->spi.base;
	//spi->ctrl &= ~(1<<19);
	//sys_clear_bit(CMU_SPI0CLK, 12);
	return 0;
}
#endif
#endif

#ifdef CONFIG_SPI0_NOR_QPI_MODE
#define SPIMEM_TFLAG_MIO_CMD_ADDR_DATA	0x04
__ramfunc static void xspi_nor_enable_qpi(struct spinor_info *sni)
{
	if(sni->spi.bus_width != 4)
		return;
	p_spinor_api->transfer(sni, XSPI_NOR_CMD_QPI_ENABLE, 0, 0, NULL, 0, 0, 0);
	sni->spi.flag &= ~SPI_FLAG_QPI_MODE;
}
__ramfunc static void xspi_nor_disable_qpi(struct spinor_info *sni)
{
	if(sni->spi.bus_width != 4)
		return;
	p_spinor_api->transfer(sni, XSPI_NOR_CMD_QPI_DISABLE, 0, 0, NULL, 0, 0, SPIMEM_TFLAG_MIO_CMD_ADDR_DATA);
	sni->spi.flag |= SPI_FLAG_QPI_MODE;

}
__ramfunc static void xspi_nor_qpi_init(struct spinor_info *sni)
{
	struct acts_spi_reg *spi= (struct acts_spi_reg *)sni->spi.base;

	if(sni->spi.bus_width == 4){
		xspi_nor_enable_qpi(sni);
		sni->spi.flag |= SPI_FLAG_QPI_MODE;
		p_spinor_api->transfer(sni, XSPI_NOR_CMD_SETPARA_QPI, 2<<4, 1, NULL, 0, 0, SPIMEM_TFLAG_MIO_CMD_ADDR_DATA); // set 6 dummy clk
		sni->spi.flag &= ~SPI_FLAG_QPI_MODE;

		#ifdef CONFIG_SPI0_NOR_DTR_MODE
		spi->ctrl |= 1<<18; // spi 0  enable dtr qpi
		#else
		spi->ctrl |= 1<<17; // spi 0  enable qpi
		#endif
	}	
}

#endif


static volatile unsigned int xip_lock_cnt = 0;
void spi0_nor_xip_lock(void)
{
	uint32_t key;
	key= irq_lock();
	xip_lock_cnt++;
	irq_unlock(key);
	if(xip_lock_cnt > 20){
		printk("spi0_nor_xip_lock err=%d\n", xip_lock_cnt);
		k_panic();
	}
}

void spi0_nor_xip_unlock(void)
{
	uint32_t key;
	key= irq_lock();
	if(xip_lock_cnt){
		xip_lock_cnt--;
	}else{
		printk("spi0_nor_xip_unlock err\n");
		k_panic();
	}
	irq_unlock(key);

}
#define SPINOR_FUN_READ		0
#define SPINOR_FUN_WRITE	1
#define SPINOR_FUN_ERASE	2
#define SPINOR_FUN_SUSPEND	3

static unsigned int spinor_xip_lock_check(int fun)
{
	unsigned int key, tcnt = 0;
	while(1){
		key = irq_lock();
		if(xip_lock_cnt == 0)
			break;
		irq_unlock(key);
		if(!k_is_in_isr()){
			k_msleep(2);
		}else{
			printk("nor w/e in irq\n");
			soc_udelay(5000);// wait master transfer finished
			break;
		}
		tcnt += 2;
		if(tcnt > 1000){
			printk("err: nor xip check=%d over 1S\n", fun);
			tcnt = 0;
			if(k_is_in_isr())// if panic, over 1s,break to save ramdump
				xip_lock_cnt = 0;
		}
	}
	return key;
}



#ifdef CONFIG_SPI_XIP_READ
static void spi_flash_xip_init(void)
{
	int err = soc_memctrl_mapping(CONFIG_SPI_XIP_VADDR, 0 , 0);
	if (err) {
		LOG_ERR(" flash xip map fail %d\n", err);
	}else{
		LOG_INF("flash xip map ok v=0x%x\n", CONFIG_SPI_XIP_VADDR);
	}
}

#if defined(CONFIG_SPI_FLASH_GPIO_2CS) && (CONFIG_SPI_FLASH_GPIO_2CS == 1)
static int spi_flash_acts_read(const struct device *dev, uint64_t offset, void *data, uint64_t len)
{
	unsigned int xip_start;
    struct spinor_info *sni = DEV_DATA(dev);
    int ret = 0;
    size_t tmplen;
	uint32_t key;
	if(offset >= nor_cs0_size){
		offset -= nor_cs0_size;
		tmplen = len;
		while(tmplen > 0) {
			if(tmplen <  0x8000)
				len = tmplen;
			else
				len = 0x8000;
			key = spinor_xip_lock_check(SPINOR_FUN_READ);
			spi_flash1_cs_select(sni, 1);
			ret = p_spinor_api->read(sni, offset, data, len);	
			spi_flash1_cs_select(sni, 0);
			irq_unlock(key);
			offset += len;
			data  = (void *)((unsigned int )data + len);
			tmplen -= len;
		}
	}else{
		xip_start = CONFIG_SPI_XIP_VADDR + offset;
		pbrom_libc_api->p_memcpy(data, (void *)xip_start, len);
	}
	return ret;
}

#else
#define XIP_NOR_MAX_LEN 0x2000000 // xip only support 32MB
static int spi_flash_acts_read(const struct device *dev, uint64_t offset, void *data, uint64_t len)
{
	unsigned int xip_start, key;
	size_t tlen;
	int ret;
	struct spinor_info *sni = DEV_DATA(dev);
	if(offset  < XIP_NOR_MAX_LEN){
		xip_start = CONFIG_SPI_XIP_VADDR + offset;
		if(offset+len > XIP_NOR_MAX_LEN){
			tlen = XIP_NOR_MAX_LEN - offset;
			pbrom_libc_api->p_memcpy(data, (void *)xip_start, tlen);
			offset = XIP_NOR_MAX_LEN;
			len -= tlen;
			data =(void *)((unsigned int)data + tlen);
		}else{
			pbrom_libc_api->p_memcpy(data, (void *)xip_start, len);
			return 0;
		}
	}
	while(len) {
		if(len <  0x8000)
			tlen = len;
		else
			tlen = 0x8000;
		key = spinor_xip_lock_check(SPINOR_FUN_READ);
		ret = p_spinor_api->read(sni, offset, data, tlen);	
		irq_unlock(key);
		offset += tlen;
		data  = (void *)((unsigned int )data + tlen);
		len -= tlen;
	}
	return 0;
}

#endif

#else
__ramfunc int spi_flash_acts_read(const struct device *dev, uint64_t offset, void *data, uint64_t len)
{
    struct spinor_info *sni = DEV_DATA(dev);
    int ret = 0;
    size_t tmplen;

#if defined(CONFIG_SPI_FLASH_GPIO_2CS) && (CONFIG_SPI_FLASH_GPIO_2CS == 1)
	int cs_sel;
	if(offset < nor_cs0_size){
		cs_sel = 0;
	}else{
		cs_sel = 1;
		offset -= nor_cs0_size;
	}
#endif

    tmplen = len;
    while(tmplen > 0) {
        if(tmplen <  0x8000)
            len = tmplen;
        else
            len = 0x8000;
        uint32_t key = spinor_xip_lock_check(SPINOR_FUN_READ);
#if defined(CONFIG_SPI_FLASH_GPIO_2CS) && (CONFIG_SPI_FLASH_GPIO_2CS == 1)
		if(cs_sel) {
			spi_flash1_cs_select(sni, 1);
		}
#endif
		#ifdef CONFIG_SPI0_NOR_QPI_MODE
		xspi_nor_disable_qpi(sni);
		#endif
        ret = p_spinor_api->read(sni, offset, data, len);
		#ifdef CONFIG_SPI0_NOR_QPI_MODE
		xspi_nor_enable_qpi(sni);
		#endif
#if defined(CONFIG_SPI_FLASH_GPIO_2CS) && (CONFIG_SPI_FLASH_GPIO_2CS == 1)
		if(cs_sel){
			spi_flash1_cs_select(sni, 0);
		}
#endif
        irq_unlock(key);
        offset += len;
        data  = (void *)((unsigned int )data + len);
        tmplen -= len;
    }
    return ret;
}
#endif




#ifdef CONFIG_NOR_SUSPEND_RESUME
/*
XT25F64F: suspend&resume 会导致0x0 地址翻转，需要fix ic bug，发resume 命令之前要读一下erase 地址，
然后再发resume 命令(resume 命令不能发退出continue 模式指令(2 个ff))。
*/
#define XT25F64F_CHIPID 0x17400b
__ramfunc static void spinor_suspend(struct spinor_info *sni)
{
	int i, j;
	// program/erase suspend
	for(j = 0; j < 3; j++){
		p_spinor_api->write_cmd(sni, SPINOR_CMD_PROGRAM_ERASE_SUSPEND);
		soc_udelay(30);
		for(i = 0; i < 100; i++) { //max 500us, tSUS must 30us
			soc_udelay(5);
			if (0 == (p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS) & 0x1)){
				break;
			}
		}
		if(i != 100){
			break;
		}
	}	
}

__ramfunc static bool spinor_resume_and_check_idle(struct spinor_info *sni, unsigned int addr)
{
	bool ret;
	uint32_t key, i;
	char tmp[8];	
	key = spinor_xip_lock_check(SPINOR_FUN_SUSPEND);
	// program/erase resum
	if(sni->chipid == XT25F64F_CHIPID){ /*fix XT25F64F nor bug*/
		p_spinor_api->read(sni, addr, tmp, 4);
		sni->spi.flag |= SPI_FLAG_NO_NEED_EXIT_CONTINUOUS_READ;
		p_spinor_api->write_cmd(sni, SPINOR_CMD_PROGRAM_ERASE_RESUME);	
		sni->spi.flag &= ~SPI_FLAG_NO_NEED_EXIT_CONTINUOUS_READ;
	}else{		
		p_spinor_api->write_cmd(sni, SPINOR_CMD_PROGRAM_ERASE_RESUME);
	}
	soc_udelay(30);
	for(i = 0; i < 100; i++){ // wait to exit suspend
		soc_udelay(5);
		if (0 == (p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2) & 0x80)){
			break;
		}
	}

	if (0 == (p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS) & 0x1)) {
		ret = true;
	}else {
		for(i = 0; i < 20; i++){ // handle 1000 us
			soc_udelay(50);
			if (0 == (p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS) & 0x1)){
				break;
			}
		}
		if(i != 20){
			 ret = true;
		}else{		
			spinor_suspend(sni);
			ret = false;
		}
	}
	irq_unlock(key);
	return ret;
}
__ramfunc static void spinor_wait_finished(struct spinor_info *sni, unsigned int addr)
{
	int i;
	uint32_t key;  
	for(i = 0; i < 20000; i++){ //2000*500us= 10000ms overtimer
		if (spinor_resume_and_check_idle(sni, addr))
			break;
		if(!k_is_in_isr()){
			if((i & 0x1) == 0)
				k_msleep(1);
		}
	}
	if(i == 20000){
		LOG_INF("nor resume error\n");
		key =  spinor_xip_lock_check(SPINOR_FUN_SUSPEND);
		while(p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS) & 0x1); // wait nor ready
		irq_unlock(key);
	}
}

void spinor_resume_finished(struct spinor_info *sni);
static void spi_flash_suspend_finished(struct spinor_info *sni)
{
	if(!k_is_in_isr())
		return;

	if(sni->flag & SPINOR_FLAG_NO_WAIT_READY){
		spinor_resume_finished(sni);
		sni->flag &= ~SPINOR_FLAG_NO_WAIT_READY;
	}
}
K_MUTEX_DEFINE(spinor_w_mutex);
static void spi_flash_w_lock(void)
{
	if(!k_is_in_isr()){
		k_mutex_lock(&spinor_w_mutex, K_FOREVER);
	}
}
static void spi_flash_w_unlock(void)
{
	if(!k_is_in_isr()){
		k_mutex_unlock(&spinor_w_mutex);
	}
}

__ramfunc  void spinor_resume_finished(struct spinor_info *sni)
{
	LOG_INF("nor is suspend, wait resume finished\n");
	p_spinor_api->write_cmd(sni, SPINOR_CMD_PROGRAM_ERASE_RESUME);
	soc_udelay(5);
	while(p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS) & 0x1); // wait nor ready
}

#else  // CONFIG_NOR_SUSPEND_RESUME must NOT define

#if defined(CONFIG_SPI_FLASH_GPIO_2CS) && (CONFIG_SPI_FLASH_GPIO_2CS == 1)
__ramfunc static int spinor_2cs_wait_ready(struct spinor_info *sni)
{
    unsigned char status;
	uint32_t key;
    while (1) {
		key = spinor_xip_lock_check(SPINOR_FUN_WRITE);
		spi_flash1_cs_select(sni, 1);
        status = p_spinor_api->read_status(sni, SPINOR_CMD_READ_STATUS);
		spi_flash1_cs_select(sni, 0);
		irq_unlock(key);
        if (!(status & 0x1))
            break;
		if(!k_is_in_isr()){
			k_msleep(2);
		}else{
			soc_udelay(2000);
		}
    }
    return 0;
}
__ramfunc void spinor_cs0_check_irq(struct spinor_info *sni)
{
	if (k_is_in_isr()) {
		if (sni->flag & SPINOR_FLAG_NO_WAIT_READY) {
			soc_udelay(100000);//delay 100ms for nor2 erase timer
			sni->flag &= ~SPINOR_FLAG_NO_WAIT_READY;
		}
	}
}
#endif  //end #if defined(CONFIG_SPI_FLASH_GPIO_2CS) && (CONFIG_SPI_FLASH_GPIO_2CS == 1)

#endif  // endif #ifdef CONFIG_NOR_SUSPEND_RESUME

#ifdef CONFIG_ACTIONS_PRINTK_DMA
extern int check_panic_exe(void);
#endif
static int spi_flash_not_wr(void)
{
	#ifdef CONFIG_ACTIONS_PRINTK_DMA
	if (k_is_in_isr() && !check_panic_exe()) {
		printk("flash not allow write in irq\n");
		k_panic();
		return 1;
	}
	#endif
	return 0;
}

__ramfunc int spi_flash_acts_write(const struct device *dev, uint64_t offset, const void *data, uint64_t len)
{
	struct spinor_info *sni = DEV_DATA(dev);
	int ret = 0;
	int wlen;
	uint32_t key;
#if defined(CONFIG_SPI_FLASH_GPIO_2CS) && (CONFIG_SPI_FLASH_GPIO_2CS == 1)
	int cs_sel;
	if(offset < nor_cs0_size){
		cs_sel = 0;
		spinor_cs0_check_irq(sni);
	}else{
		cs_sel = 1;
		offset -= nor_cs0_size;
		spinor_2cs_wait_ready(sni);
	}
#endif

	if (spi_flash_not_wr())
		return -1;

#ifdef CONFIG_NOR_SUSPEND_RESUME
	spi_flash_w_lock();
	spi_flash_suspend_finished(sni);
#endif

	while(len > 0) {
		if(len > SPINOR_WRITE_PAGE_SIZE)
			wlen = SPINOR_WRITE_PAGE_SIZE;
		else
			wlen = len;
		key = spinor_xip_lock_check(SPINOR_FUN_WRITE);
#if defined(CONFIG_SPI_FLASH_GPIO_2CS) && (CONFIG_SPI_FLASH_GPIO_2CS == 1)
		if(cs_sel) {
			spi_flash1_cs_select(sni, 1);
		}
#endif

#ifdef CONFIG_SPI0_NOR_QPI_MODE
		xspi_nor_disable_qpi(sni);
#endif

#ifdef CONFIG_SPI0_NOR_DTR_MODE
		//if(sni->flag & SPI_FLAG_WR_4IO)
		//	spi0_cache_exit_dtr_mode(sni);
#endif
		ret = p_spinor_api->write(sni, offset, data, wlen);
#ifdef CONFIG_SPI0_NOR_DTR_MODE
		//if(sni->flag & SPI_FLAG_WR_4IO)
			//spi0_cache_enter_dtr_mode(sni);
#endif

#ifdef CONFIG_SPI0_NOR_QPI_MODE
		xspi_nor_enable_qpi(sni);
#endif
#if defined(CONFIG_SPI_FLASH_GPIO_2CS) && (CONFIG_SPI_FLASH_GPIO_2CS == 1)
		if(cs_sel) {
			spi_flash1_cs_select(sni, 0);
		}
#endif
		irq_unlock(key);
		offset += wlen;
		data  = (void *)((unsigned int )data + wlen);
		len -= wlen;
	}
#if defined(CONFIG_SPI_FLASH_GPIO_2CS) && (CONFIG_SPI_FLASH_GPIO_2CS == 1)
	if(!cs_sel) {
#endif
#ifdef CONFIG_SPI_XIP_READ
		soc_memctrl_cache_invalid();
#endif
#if defined(CONFIG_SPI_FLASH_GPIO_2CS) && (CONFIG_SPI_FLASH_GPIO_2CS == 1)
	}
#endif

#ifdef CONFIG_NOR_SUSPEND_RESUME
	spi_flash_w_unlock();
#endif
	return ret ;
}

__ramfunc int spi_flash_acts_erase(const struct device *dev, uint64_t offset, uint64_t size)
{
	struct spinor_info *sni = DEV_DATA(dev);
	int ret = 0;
	uint32_t key;
	size_t erase_size = SPINOR_ERASE_SECTOR_SIZE;
	uint32_t t0,t1, t2;
	int use_block = 0;

	if (spi_flash_not_wr())
		return -1;

#ifndef CONFIG_SPINOR_TEST_DELAYCHAIN
	LOG_INF("nor_e:offset=0x%llx,len=0x%llx\n", offset, size);
#endif

#ifdef CONFIG_NOR_SUSPEND_RESUME
	bool b_suspend = true;
	use_block = 1;
	if((size >= SPINOR_ERASE_BLOCK_SIZE*4) || (k_is_in_isr())) // erase 256kb or panic, not suspend &resume
		b_suspend = false;
	spi_flash_w_lock();
	spi_flash_suspend_finished(sni);
	if(b_suspend)
		sni->flag |= SPINOR_FLAG_NO_WAIT_READY;
#else
	if(size >= SPINOR_ERASE_BLOCK_SIZE*4) // erase 256kb, use block
		use_block = 1;
#endif
#if defined(CONFIG_SPI_FLASH_GPIO_2CS) && (CONFIG_SPI_FLASH_GPIO_2CS == 1)
		int cs_sel;
		if(offset < nor_cs0_size){
			cs_sel = 0;
			spinor_cs0_check_irq(sni);			
		}else{
			cs_sel = 1;
			offset -= nor_cs0_size;
			spinor_2cs_wait_ready(sni);
			use_block = 1;
		}
#endif

	while (size > 0) {
		if(use_block) {
			if (size < SPINOR_ERASE_BLOCK_SIZE) {
				erase_size = SPINOR_ERASE_SECTOR_SIZE;
	        } else if (offset & SPINOR_ERASE_BLOCK_MASK) {
				erase_size = SPINOR_ERASE_SECTOR_SIZE;
			} else {
				erase_size = SPINOR_ERASE_BLOCK_SIZE;
			}
		}

		key= spinor_xip_lock_check(SPINOR_FUN_ERASE);
#if defined(CONFIG_SPI_FLASH_GPIO_2CS) && (CONFIG_SPI_FLASH_GPIO_2CS == 1)
		if(cs_sel) {
			sni->flag |= SPINOR_FLAG_NO_WAIT_READY;
			spi_flash1_cs_select(sni, 1);
		}
#endif

		t0 = k_cycle_get_32();
#ifdef CONFIG_SPI0_NOR_QPI_MODE
		xspi_nor_disable_qpi(sni);
#endif
		ret = p_spinor_api->erase(sni, offset, erase_size);
#ifdef CONFIG_SPI0_NOR_QPI_MODE
		xspi_nor_enable_qpi(sni);
#endif
#ifdef CONFIG_NOR_SUSPEND_RESUME
		if(b_suspend)
			spinor_suspend(sni);
#endif

#ifdef CONFIG_SPINOR_TEST_DELAYCHAIN
		soc_udelay(100000); // try fail, nor status may not finished, delay erase finished
#endif
		t1 = k_cycle_get_32();
#if defined(CONFIG_SPI_FLASH_GPIO_2CS) && (CONFIG_SPI_FLASH_GPIO_2CS == 1)
		if(cs_sel) {
			spi_flash1_cs_select(sni, 0);
		}
#endif
		irq_unlock(key);
#if defined(CONFIG_SPI_FLASH_GPIO_2CS) && (CONFIG_SPI_FLASH_GPIO_2CS == 1)
		if(cs_sel) {
			spinor_2cs_wait_ready(sni);
			sni->flag &= ~SPINOR_FLAG_NO_WAIT_READY;
		}
#endif

#ifdef CONFIG_NOR_SUSPEND_RESUME
		if(b_suspend)
			spinor_wait_finished(sni, offset);
#endif
		t2 = k_cycle_get_32();
		#ifndef CONFIG_SPINOR_TEST_DELAYCHAIN
		LOG_INF("nor_e:off=0x%x,len=0x%x, tran=%d us, wait=%d\n", (uint32_t)offset, erase_size,
				k_cyc_to_us_ceil32(t1-t0), k_cyc_to_us_ceil32(t2-t1));
		#endif
		size -= erase_size;
		offset += erase_size;
	}

#ifdef CONFIG_SPI_XIP_READ
	soc_memctrl_cache_invalid();
#endif

#ifdef CONFIG_NOR_SUSPEND_RESUME
	sni->flag &= ~SPINOR_FLAG_NO_WAIT_READY;
	spi_flash_w_unlock();
#endif
	return ret ;
}

static  __ramfunc void xspi_delay(void)
{
	volatile int i = 100000;

	while (i--)
		;
}

__ramfunc void xspi_nor_enable_status_qe(struct spinor_info *sni)
{
	uint16_t status;

	/* MACRONIX's spinor has different QE bit */
	if (XSPI_NOR_MANU_ID_MACRONIX == (sni->chipid & 0xff)) {
		status = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS);
		if (!(status & 0x40)) {
			/* set QE bit to disable HOLD/WP pin function */
			status |= 0x40;
			p_spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS,
						(u8_t *)&status, 1);
		}

		return;
	}

	/* check QE bit */
	status = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);

	if (!(status & 0x2)) {
		/* set QE bit to disable HOLD/WP pin function, for WinBond */
		status |= 0x2;
		p_spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS2,
					(u8_t *)&status, 1);

		/* check QE bit again */
		status = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);

		if (!(status & 0x2)) {
			/* oh, let's try old write status cmd, for GigaDevice/Berg */
			status = ((status | 0x2) << 8) |
				 p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS);
			p_spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS,
						(u8_t *)&status, 2);
		}
	}

	xspi_delay();

}

static __ramfunc void xspi_setup_bus_width(struct spinor_info *sni, u8_t bus_width)
{
	struct acts_spi_reg *spi= (struct acts_spi_reg *)sni->spi.base;

	spi->ctrl = (spi->ctrl & ~(0x3 << 10)) | (((bus_width & 0x7) / 2 + 1) << 10);

	xspi_delay();
}

static __sleepfunc void xspi_setup_delaychain(struct spinor_info *sni, u8_t delay)
{
	struct acts_spi_reg *spi= (struct acts_spi_reg *)sni->spi.base;
	spi->delaychain = (spi->delaychain & ~(0x3F << 0)) | (delay << 0);
	xspi_delay();
}

#if IS_ENABLED(CONFIG_SPINOR_TEST_DELAYCHAIN)
extern int nor_test_delaychain(const struct device *dev);
#endif

#if IS_ENABLED(CONFIG_NOR_ACTS_DQ_MODE_ENABLE)
extern void nor_dual_quad_read_mode_try(struct spinor_info *sni);
#endif

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL

#include "flash_delaytran_table.c"

//static const struct nor_delaychain_tbl *g_p_chipid_tbl;
static const  struct id_nor_delaychain_tbl *g_chipid_tbl;

static  void nor0_delaytran_init(uint32_t chip_id)
{
	uint8_t i;
	g_chipid_tbl = &chipid_dl_tbl[0];
	for (i = 1; i < ARRAY_SIZE(chipid_dl_tbl); i++) {
		if (chipid_dl_tbl[i].chip_id  == chip_id) {
			g_chipid_tbl = &chipid_dl_tbl[i];
			printk("nor find dl tbl=%d\n", i);
			break;
		}
	}
}
#if defined(CONFIG_SPI_FLASH_GPIO_2CS) && (CONFIG_SPI_FLASH_GPIO_2CS == 1)
static const struct nor_delaychain_tbl *g_cs1_p_chipid_tbl = NULL;
static  void nor1_delaytran_init(uint32_t chip_id)
{
	uint8_t i;
	g_cs1_p_chipid_tbl = chipid_dl_tbl[0].tbl;
	for (i = 1; i < ARRAY_SIZE(chipid_dl_tbl); i++) {
		if (chipid_dl_tbl[i].chip_id  == chip_id) {
			g_cs1_p_chipid_tbl = chipid_dl_tbl[i].tbl;
			printk("nor cs1 find dl tbl=%d\n", i);
			break;
		}
	}
}
#endif
static __ramfunc void nor_set_delaychain_by_vdd(struct spinor_info *sni, uint16_t vdd)
{
	uint8_t i;
	const struct nor_delaychain_tbl *ptbl;
	ptbl = g_chipid_tbl->tbl;
	for (i = 0; i < CHIP_ID_TBL_NUM; i++) {
		if (ptbl[i].vdd_volt == vdd) {
			xspi_setup_delaychain(sni, ptbl[i].delay);
			sni->spi.delay_chain = ptbl[i].delay; //same as xip 
			#if defined(CONFIG_SPI_FLASH_GPIO_2CS) && (CONFIG_SPI_FLASH_GPIO_2CS == 1)
			nor_cs0_delaytran = sni->spi.delay_chain;
			if(g_cs1_p_chipid_tbl == NULL){
				nor_cs1_delaytran = nor_cs0_delaytran;
			}else{
				nor_cs1_delaytran = g_cs1_p_chipid_tbl[i].delay;
			}
			#endif
			break;
		}
	}
}

static __ramfunc void spi0_set_clk_by_vdd(uint16_t vdd_volt)
{
#ifdef CONFIG_SPI0_NOR_DTR_MODE
	if (vdd_volt <= 1000) {
		spi0_dtr_set_clk(MHZ(48) * 2);
	} else {
		spi0_dtr_set_clk(MHZ(g_chipid_tbl->max_clk) * 2);
	}
#else
	if (vdd_volt < 1000) {
		clk_set_rate(CLOCK_ID_SPI0, MHZ(64));
	} else {
		clk_set_rate(CLOCK_ID_SPI0, MHZ(g_chipid_tbl->max_clk));
	}
#endif
}

__dvfs_notifier_func static void nor_dvfs_notify(void *user_data, struct dvfs_freqs *dvfs_freq)
{
	struct spinor_info *sni = (struct spinor_info *)user_data;
	struct dvfs_level *old_dvfs_level, *new_dvfs_level;
	uint32_t key;

	if (!dvfs_freq) {
		printk("dvfs notify invalid param");
		return ;
	}

	if (dvfs_freq->old_level == dvfs_freq->new_level)
		return ;

	old_dvfs_level = dvfs_get_info_by_level_id(dvfs_freq->old_level);
	new_dvfs_level = dvfs_get_info_by_level_id(dvfs_freq->new_level);

	if (old_dvfs_level->vdd_volt == new_dvfs_level->vdd_volt) {
		return;
	}

	key = irq_lock();

	if (old_dvfs_level->vdd_volt > new_dvfs_level->vdd_volt) {
		/* vdd voltage decrease */
		if (dvfs_freq->state == DVFS_EVENT_PRE_CHANGE) {
			spi0_set_clk_by_vdd(new_dvfs_level->vdd_volt);
			nor_set_delaychain_by_vdd(sni, new_dvfs_level->vdd_volt);

			printk("nor delaychain update by vdd:%d => %d\n",
					old_dvfs_level->vdd_volt, new_dvfs_level->vdd_volt);
		}
	} else {
		/* vdd voltage increase */
		if (dvfs_freq->state == DVFS_EVENT_POST_CHANGE) {
			nor_set_delaychain_by_vdd(sni, new_dvfs_level->vdd_volt);
			spi0_set_clk_by_vdd(new_dvfs_level->vdd_volt);
			printk("nor delaychain update by vdd:%d => %d\n",
					old_dvfs_level->vdd_volt, new_dvfs_level->vdd_volt);
		}
	}

	irq_unlock(key);
}

static struct spinor_info spi_flash_acts_data;
static struct dvfs_notifier __dvfs_notifier_data nor_dvsf_notifier = {
	.dvfs_notify_func_t = nor_dvfs_notify,
	.user_data = &spi_flash_acts_data,
};

#endif /* CONFIG_ACTS_DVFS_DYNAMIC_LEVEL */
void spinor_test_uid_securty(const struct device *dev);

__ramfunc static int spinor_wait_ready(struct spinor_info *sni)
{
    unsigned char status;

    while (1) {
        status = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS);
        if (!(status & 0x1))
            break;
    }
    return 0;
}
__ramfunc static int spi_flash_cfg_read(struct spinor_info *sni, uint32_t addr, uint8_t *cfg)
{
    u8_t addr_len;
    u8_t cid = (sni->chipid & 0xff0000) >> 16;
    if (cid > 24){
        addr_len = 4;
    }else{
        addr_len = 3;
    }

	return  p_spinor_api->transfer(sni, SPINOR_CMD_RD_VOL_CFG, addr, addr_len, cfg, 1, 1, 0);
}
__ramfunc static int spi_flash_cfg_write(struct spinor_info *sni, uint32_t addr, uint8_t *cfg)
{
	int ret;
    u8_t addr_len;
    u8_t cid = (sni->chipid & 0xff0000) >> 16;
    if (cid > 24){
        addr_len = 4;
    }else{
        addr_len = 3;
    }
	p_spinor_api->write_cmd(sni, SPIMEM_CMD_ENABLE_WRITE);
	ret =  p_spinor_api->transfer(sni, SPINOR_CMD_WR_VOL_CFG, addr, addr_len, cfg, 1, 0, SPIMEM_TFLAG_WRITE_DATA);
	spinor_wait_ready(sni);
	return ret;
}

#define GD25B512M_CHIP_ID	 0x1a47c8
__ramfunc void spi_flash_init_by_chpid(struct spinor_info *sni)
{
	uint8_t cfg = 0;
	if(sni->chipid == GD25B512M_CHIP_ID){
		spi_flash_cfg_read(sni, 6, &cfg);
		if(cfg &0x01){
			cfg &= 0xfe;
			spi_flash_cfg_write(sni, 6, &cfg); // xip enableed
			printk("eable xip\n");
		}
	}

}

__ramfunc int spi_flash_acts_init(const struct device *dev)
{
	struct spinor_info *sni = DEV_DATA(dev);
	uint32_t key;
	uint8_t status, status2, status3;
	unsigned char cid;
	sni->spi.prepare_hook = spi_flash_acts_prepare;

	key = irq_lock();
	#ifndef CONFIG_SPI0_NOR_DTR_MODE  //dtr mode not set clk
	clk_set_rate(CLOCK_ID_SPI0, MHZ(CONFIG_SPI_FLASH_FREQ_MHZ/2)); // set low clk for read chipid
	#endif
	sni->chipid = p_spinor_api->read_chipid(sni) & 0xffffff;
	cid =  (sni->chipid & 0xff0000)>>16;
	printk("read spi nor chipid:0x%x, cid=%d\n", sni->chipid, cid);
	#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	nor0_delaytran_init(sni->chipid);
	sni->spi.freq_khz =  KHZ(g_chipid_tbl->max_clk);
	#endif	
	status = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS);
	status2 = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);
	status3 = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS3);

	printk("spinor status: {0x%02x 0x%02x 0x%02x}\n", status, status2, status3);

#ifdef CONFIG_NOR_SUSPEND_RESUME
		if(status2 & (NOR_STATUS2_SUS1|NOR_STATUS2_SUS2))
			spinor_resume_finished(sni);
#endif

	if(cid > 24){ //capacity = 2^cid (byte)
		p_spinor_api->set_addr_mode(sni, 4); // set 4byte mode
		sys_set_bit(sni->spi.base , 16); //spi clt bit16 4byte mode
	}
	spi_flash_init_by_chpid(sni);
	/* configure delay chain */
#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	nor_set_delaychain_by_vdd(sni, 1200);
#else
	xspi_setup_delaychain(sni, sni->spi.delay_chain);
#endif

#if IS_ENABLED(CONFIG_NOR_ACTS_DQ_MODE_ENABLE)
	nor_dual_quad_read_mode_try(sni);
	printk("bus width : %d, and cache read use ", sni->spi.bus_width);

#else
	if(sni->spi.bus_width == 4) {
		printk("nor is 4 line mode\n");
#ifndef CONFIG_SPI_4X_READ
		sni->spi.flag |= SPI_FLAG_SPI_4XIO;
#endif
		xspi_nor_enable_status_qe(sni);
		/* enable 4x mode */
		xspi_setup_bus_width(sni, 4);
	} else if(sni->spi.bus_width == 2) {
		printk("nor is 2 line mode\n");
		/* enable 2x mode */
		xspi_setup_bus_width(sni, 2);
	} else {
		sni->spi.bus_width = 1;
		printk("nor is 1 line mode\n");
		/* enable 1x mode */
		xspi_setup_bus_width(sni, 1);
	}
#endif

#ifdef CONFIG_SPI0_NOR_DTR_MODE
		///*dtr mode set 4line to set mode must  xip run code*/
		if(sni->spi.bus_width == 4){
			#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
			sni->chipid = spi0_cache_enter_dtr_mode(sni, sni->spi.freq_khz/1000);
			#else
			sni->chipid = spi0_cache_enter_dtr_mode(sni, CONFIG_SPI_FLASH_FREQ_MHZ);
			#endif
			printk("spinor dtr mode , chipid:0x%x, clkreg=0x%x\n", sni->chipid, sys_read32(CMU_SPI0CLK));
			//while(loop);
		}
#else
	/* setup SPI clock rate */
#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	clk_set_rate(CLOCK_ID_SPI0, MHZ(g_chipid_tbl->max_clk));
#else
	clk_set_rate(CLOCK_ID_SPI0, MHZ(CONFIG_SPI_FLASH_FREQ_MHZ));
#endif

#endif
	
	/* check delay chain workable */
	sni->chipid = p_spinor_api->read_chipid(sni) & 0xffffff;

	printk("read again spi nor chipid:0x%x\n", sni->chipid);

#if defined(CONFIG_SPI_FLASH_GPIO_2CS) && (CONFIG_SPI_FLASH_GPIO_2CS == 1)
	unsigned int chip_id;
	spi_gpio_cs_dev = device_get_binding(CONFIG_GPIO_PIN2NAME(CONFIG_SPI_FLASH_1_GPIO_CS_PIN));
	if (!spi_gpio_cs_dev) {
		printk("failed to get gpio:%d device", CONFIG_SPI_FLASH_1_GPIO_CS_PIN);
		irq_unlock(key);
		return -1;
	}
	clk_set_rate(CLOCK_ID_SPI0, MHZ(CONFIG_SPI_FLASH_FREQ_MHZ/2)); // set low clk for read cs1 delaytran
	nor_cs0_size = 1 << ((sni->chipid >> 16)&0xff);
	gpio_pin_configure(spi_gpio_cs_dev, CONFIG_SPI_FLASH_1_GPIO_CS_PIN % 32, GPIO_OUTPUT_HIGH);
	gpio_pin_set(spi_gpio_cs_dev, CONFIG_SPI_FLASH_1_GPIO_CS_PIN % 32, 1);
	printk("use GPIO:%d as spi cs pin\n", CONFIG_SPI_FLASH_1_GPIO_CS_PIN);
	spi_flash1_cs_select(sni, 1);
	p_spinor_api->write_cmd(sni, 0xAB); //exit deep power down
	soc_udelay(100);
	p_spinor_api->write_cmd(sni, 0xff);// reset nor 
	chip_id = sni->chipid; // back nor0 chipid
	sni->chipid  = p_spinor_api->read_chipid(sni);
	nor_cs1_size = 1 << ((sni->chipid >> 16)&0xff);
	printk("cs0 nor size=0x%x, cs1 nor chipid:0x%x, size=0x%x\n", nor_cs0_size, sni->chipid, nor_cs1_size);
	if(nor_cs1_size > 0x1000000)
		p_spinor_api->set_addr_mode(sni, 4); // set 4byte mode
	xspi_nor_enable_status_qe(sni);
	status = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS);
	status2 = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);
	status3 = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS3);
	printk("cs1 nor status: {0x%02x 0x%02x 0x%02x}\n", status, status2, status3);
	spi_flash1_cs_select(sni, 0);

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	nor1_delaytran_init(sni->chipid);// init cs1 delaytran table
	nor_set_delaychain_by_vdd(sni, 1200); //init default delaytran by vdd
#else
	nor_cs0_delaytran = sni->spi.delay_chain;
	nor_cs1_delaytran = nor_cs0_delaytran;
#endif
	sni->chipid = chip_id;
#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
		clk_set_rate(CLOCK_ID_SPI0, MHZ(g_chipid_tbl->max_clk));
#else
		clk_set_rate(CLOCK_ID_SPI0, MHZ(CONFIG_SPI_FLASH_FREQ_MHZ));
#endif

#endif  //#if defined(CONFIG_SPI_FLASH_GPIO_2CS) && (CONFIG_SPI_FLASH_GPIO_2CS == 1)

	status = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS);
	status2 = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);
	status3 = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS3);

	printk("spinor status: {0x%02x 0x%02x 0x%02x}\n", status, status2, status3);
#ifdef CONFIG_SPI0_NOR_DTR_MODE
	//volatile unsigned loop = 1;
	if(sni->spi.bus_width == 4){
		//while(loop);
			#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
			sni->chipid = spi0_cache_enter_dtr_mode(sni, g_chipid_tbl->max_clk);
			#else
			sni->chipid = spi0_cache_enter_dtr_mode(sni, CONFIG_SPI_FLASH_FREQ_MHZ);
			#endif
		printk("spinor dtr mode , chipid:0x%x\n", sni->chipid);
		//while(loop);
	}
#endif
#ifdef CONFIG_SPI0_NOR_QPI_MODE
	printk("qpi enable\n");
	xspi_nor_qpi_init(sni);
	printk("qpi enable ok\n");
#endif
	//printk("nor power down\n");
	//p_spinor_api->transfer(sni, 0xB9, 0, 0, NULL, 0, 0, 0); // power down
	//printk("nor power down end\n");

#if IS_ENABLED(CONFIG_SPINOR_TEST_DELAYCHAIN)
	nor_test_delaychain(dev);
#endif

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	dvfs_register_notifier(&nor_dvsf_notifier);
#endif
#ifdef CONFIG_SPI_XIP_READ
	spi_flash_xip_init();
#endif
#ifdef CONFIG_NOR_SECURIYT_SUSPPORT
	spinor_test_uid_securty(dev);
#endif

	irq_unlock(key);

	flash_write_protection_set(dev, true);

	return 0;
}


#if IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT)
static void spi_flash_acts_pages_layout(
				const struct device *dev,
				const struct flash_pages_layout **layout,
				size_t *layout_size)
{
	*layout = &(DEV_CFG(dev)->pages_layout);
	*layout_size = 1;
}
#endif /* IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT) */

#if IS_ENABLED(CONFIG_NOR_ACTS_DATA_PROTECTION_ENABLE)
extern int nor_write_protection(const struct device *dev, bool enable);
#endif

__ramfunc int spi_flash_acts_write_protection(const struct device *dev, bool enable)
{
#if IS_ENABLED(CONFIG_NOR_ACTS_DATA_PROTECTION_ENABLE)
	nor_write_protection(dev, enable);
#endif
	return 0;
}

static const struct flash_parameters flash_acts_parameters = {
	.write_block_size = 0x1000,
	.erase_value = 0xff,
};

static const struct flash_parameters *
spi_flash_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_acts_parameters;
}
#ifdef CONFIG_PM_DEVICE

int spi_flash_pm_control(const struct device *device, enum pm_device_action action)
{
//	if(action == PM_DEVICE_ACTION_LATE_RESUME){
//		sys_write32((sys_read32(SPICACHE_CTL) & ~(0x3 << 5)) | (0x1 << 5) , SPICACHE_CTL);
		//printk("late reusme = 0x%x\n",  sys_read32(SPICACHE_CTL));
//	}else if(action == PM_DEVICE_ACTION_EARLY_SUSPEND){
//		sys_write32((sys_read32(SPICACHE_CTL) & ~(0x3 << 5)) | (0x2 << 5) , SPICACHE_CTL);
		//printk("nor early suspend = 0x%x\n",  sys_read32(SPICACHE_CTL));
//    }
#if defined(CONFIG_SPI_FLASH_GPIO_2CS) && (CONFIG_SPI_FLASH_GPIO_2CS == 1)
	struct spinor_info *sni = DEV_DATA(device); 
	int i;
	unsigned int chipid;
	if(action == PM_DEVICE_ACTION_RESUME){
		printk("spi0 cs2 resume ...\n");
		spi_flash1_cs_select(sni, 1);
		for(i = 0; i < 3; i++){
			p_spinor_api->write_cmd(sni, 0xAB); //exit deep power down
			soc_udelay(40);
			chipid = p_spinor_api->read_chipid(sni) & 0xffffff;
			if(sni->chipid != chipid){
				printk("%d cs2 resume 0x%x != 0x%x  fail \n", i, chipid, sni->chipid);
				soc_udelay(1000);
			}else{
				break;
			}
		}
		spi_flash1_cs_select(sni, 0);
	}else if(action == PM_DEVICE_ACTION_SUSPEND){
		printk("spi0 cs2 suspend ...\n");
		spi_flash1_cs_select(sni, 1);
		p_spinor_api->write_cmd(sni, 0xB9); // enter deep power down
		spi_flash1_cs_select(sni, 0);
	}
#endif
    return 0;
}
#else
#define spi_flash_pm_control 	NULL
#endif


#if defined(CONFIG_SPI_FLASH_GPIO_2CS) && (CONFIG_SPI_FLASH_GPIO_2CS == 1)

K_MUTEX_DEFINE(spinor_cs2_w_mutex);

static void spi_flash_cs2_lock(void)
{
	if(!k_is_in_isr()){
		k_mutex_lock(&spinor_cs2_w_mutex, K_FOREVER);
	}
}
static void spi_flash_cs2_unlock(void)
{
	if(!k_is_in_isr()){
		k_mutex_unlock(&spinor_cs2_w_mutex);
	}
}


int spi_flash_acts_2cs_read(const struct device *dev, uint64_t offset, void *data, uint64_t len)
{
	size_t tlen;
	int ret;
	if(offset  < nor_cs0_size){
		if(offset+len > nor_cs0_size){
			tlen = nor_cs0_size - offset;
			spi_flash_acts_read(dev, offset, data, tlen);
			offset = nor_cs0_size;
			data = (void *)((unsigned int )data + tlen);
			len -= tlen;
		}
	}
	if(offset  < nor_cs0_size){// read cs0 not lock
		ret = spi_flash_acts_read(dev, offset, data, len);
	}else{
		spi_flash_cs2_lock();
		ret = spi_flash_acts_read(dev, offset, data, len);
		spi_flash_cs2_unlock();
	}
	return ret;
}

int spi_flash_acts_2cs_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	size_t tlen;
	int ret;
	if (spi_flash_not_wr())
		return -1;
	spi_flash_cs2_lock();
	if(offset  < nor_cs0_size){
		if(offset+len > nor_cs0_size){
			tlen = nor_cs0_size - offset;
			spi_flash_acts_write(dev, offset, data, tlen);
			offset = nor_cs0_size;
			data = (const void *)((unsigned int )data + tlen);
			len -= tlen;
		}
	}
	ret = spi_flash_acts_write(dev, offset, data, len);
	spi_flash_cs2_unlock();
	return ret;
}
int spi_flash_acts_2cs_erase(const struct device *dev, off_t offset, size_t size)
{
	size_t tlen;
	int ret;
	if (spi_flash_not_wr())
		return -1;
	spi_flash_cs2_lock();
	if(offset  < nor_cs0_size){
		if(offset+size > nor_cs0_size){
			tlen = nor_cs0_size - offset;
			spi_flash_acts_erase(dev, offset, tlen);
			offset = nor_cs0_size;
			size -= tlen;
		}
	}
	ret = spi_flash_acts_erase(dev,  offset, size);
	spi_flash_cs2_unlock();
	return ret;

}

static struct flash_driver_api spi_flash_nor_api = {
	.read = spi_flash_acts_2cs_read,
	.write = spi_flash_acts_2cs_write,
	.erase = spi_flash_acts_2cs_erase,
	.write_protection = spi_flash_acts_write_protection,
	.get_parameters = spi_flash_get_parameters,
#if IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = spi_flash_acts_pages_layout,
#endif
};

#else

static struct flash_driver_api spi_flash_nor_api = {
	.read = spi_flash_acts_read,
	.write = spi_flash_acts_write,
	.erase = spi_flash_acts_erase,
	.write_protection = spi_flash_acts_write_protection,
	.get_parameters = spi_flash_get_parameters,
#if IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = spi_flash_acts_pages_layout,
#endif
};

#endif
/* system XIP spinor */
static struct spinor_info __act_s2_sleep_data spi_flash_acts_data = {
	.spi = {
		.base = SPI0_REG_BASE,
		.bus_width = CONFIG_SPI_FLASH_BUS_WIDTH,
		.delay_chain = CONFIG_SPI_FLASH_DELAY_CHAIN,
#if (CONFIG_DMA_SPINOR_RESEVER_CHAN < CONFIG_DMA_0_PCHAN_NUM)
		.dma_base= (DMA_REG_BASE + 0x100 + (CONFIG_DMA_SPINOR_RESEVER_CHAN * 0x100)),
#endif
		.flag = 0,

	},
	.flag = 0,

};

__sleepfunc void  sys_norflash_power_ctrl(uint32_t is_powerdown)
{
	struct spinor_info *sni = &spi_flash_acts_data;

	if (is_powerdown){
		p_spinor_api->write_cmd(sni, 0xB9); // enter deep power down
		soc_udelay(5); // max 3us
	} else {
		//sni->spi.flag |= SPI_FLAG_NO_NEED_EXIT_CONTINUOUS_READ;
		p_spinor_api->write_cmd(sni, 0xAB);
		//sni->spi.flag &= ~SPI_FLAG_NO_NEED_EXIT_CONTINUOUS_READ;
		soc_udelay(40); // max 30us
	}
}



#if 0
void spinor_set(int bus_width, int use_dma, int nxio)
{
	spi_flash_acts_data.spi.bus_width = bus_width;

	if(use_dma)
		spi_flash_acts_data.spi.dma_base = (DMA_REG_BASE + 0x100 + (CONFIG_DMA_SPINOR_RESEVER_CHAN * 0x100));
	else
		spi_flash_acts_data.spi.dma_base = 0;
	if(nxio){
		spi_flash_acts_data.spi.flag |= SPI_FLAG_SPI_NXIO | SPI_FLAG_WR_4IO;
	}else{
		spi_flash_acts_data.spi.flag &= ~(SPI_FLAG_SPI_NXIO | SPI_FLAG_WR_4IO);		
	}	
}
#endif

static const struct spi_flash_acts_config spi_acts_config = {
#if IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT)
	.pages_layout = {
			.pages_count = CONFIG_SPI_FLASH_CHIP_SIZE/0x1000,
			.pages_size  = 0x1000,
		},
#endif
	.chip_size	 = CONFIG_SPI_FLASH_CHIP_SIZE,
	.page_size	 = 0x1000,
};

#if IS_ENABLED(CONFIG_SPI_FLASH_0)
DEVICE_DEFINE(spi_flash_acts, CONFIG_SPI_FLASH_NAME, &spi_flash_acts_init, spi_flash_pm_control,
		    &spi_flash_acts_data, &spi_acts_config, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &spi_flash_nor_api);
#endif




#if IS_ENABLED(CONFIG_SPI_FLASH_2)


static K_MUTEX_DEFINE(flash_2_mutex);

static int spi_flash_2_acts_read(const struct device *dev, uint64_t offset, void *data, uint64_t len)
{
    struct spinor_info *sni = DEV_DATA(dev);
    int ret = 0;
    size_t tmplen;
	k_mutex_lock(&flash_2_mutex, K_FOREVER);
    tmplen = len;
    while(tmplen > 0) {
        if(tmplen <  0x8000)
            len = tmplen;
        else
            len = 0x8000;

        ret = p_spinor_api->read(sni, offset, data, len);
        offset += len;
        data  = (void *)((char *)data + len);
        tmplen -= len;
    }

	k_mutex_unlock(&flash_2_mutex);

    return ret;
}

static int spi_flash_2_acts_write(const struct device *dev, uint64_t offset, const void *data, uint64_t len)
{
	struct spinor_info *sni = DEV_DATA(dev);
	int ret;
	k_mutex_lock(&flash_2_mutex, K_FOREVER);
	ret = p_spinor_api->write(sni, offset, data, len);
	k_mutex_unlock(&flash_2_mutex);
	return ret ;
}

static int spi_flash_2_acts_erase(const struct device *dev, uint64_t offset, uint64_t size)
{
	struct spinor_info *sni = DEV_DATA(dev);
	int ret;
	k_mutex_lock(&flash_2_mutex, K_FOREVER);
	ret = p_spinor_api->erase(sni, offset, size);
	k_mutex_unlock(&flash_2_mutex);
	return ret ;
}


static int spi_flash_2_pwoer(struct spinor_info *sni, bool on)
{
#if IS_ENABLED(CONFIG_SPI_FLASH_2_USE_GPIO_POWER)
    int ret;
	int gpio_value = CONFIG_SPI_FLASH_2_GPIO_POWER_LEVEL;
    const struct device *power_gpio_dev;
    uint8_t power_gpio = CONFIG_SPI_FLASH_2_POWER_GPIO % 32;
    power_gpio_dev = device_get_binding(CONFIG_GPIO_PIN2NAME(CONFIG_SPI_FLASH_2_POWER_GPIO));
    if (!power_gpio_dev) {
        LOG_ERR("Failed to bind nor  power GPIO(%d:%s)", power_gpio, CONFIG_GPIO_PIN2NAME(CONFIG_SPI_FLASH_2_POWER_GPIO));
        return -1;
    }
    ret = gpio_pin_configure(power_gpio_dev, power_gpio, GPIO_OUTPUT);
    if (ret) {
        LOG_ERR("Failed to config output GPIO:%d", power_gpio);
        return ret;
    }
    if (on) {
        /* power on nor */
        gpio_pin_set(power_gpio_dev, power_gpio, gpio_value);       
    } else {
        /* power off nor */
        gpio_pin_set(power_gpio_dev, power_gpio, !gpio_value);
    }
#else
    if (on) {
        p_spinor_api->write_cmd(sni, 0xAB); //exit deep power down
    } else {
		p_spinor_api->write_cmd(sni, 0xB9); // enter deep power down
    }
#endif

    return 0;
}


#ifdef CONFIG_PM_DEVICE
int spi_flash_2_pm_control(const struct device *device, enum pm_device_action action)
{
	struct spinor_info *sni = DEV_DATA(device);	
	static uint32_t bk_spi3_cs_pinctl;
	if(action == PM_DEVICE_ACTION_RESUME){
		/* recovery spi3 cs */
		sys_write32(bk_spi3_cs_pinctl, GPION_CTL(CONFIG_SPI_FLASH_2_GPIO_CS_PIN));
        LOG_INF("spi2 nor resume ...\n");
		spi_flash_2_pwoer(sni, true);
	}else if(action == PM_DEVICE_ACTION_SUSPEND){
        LOG_INF("spi2 nor suspend ...\n");
		spi_flash_2_pwoer(sni, false);
		/* make nor cs high ,for spi3 control poweoff after cpu enter s3*/
		bk_spi3_cs_pinctl = sys_read32(GPION_CTL(CONFIG_SPI_FLASH_2_GPIO_CS_PIN));
		sys_write32(1 << (CONFIG_SPI_FLASH_2_GPIO_CS_PIN % 32), GPION_BSR(CONFIG_SPI_FLASH_2_GPIO_CS_PIN));
		sys_write32(0x1040, GPION_CTL(CONFIG_SPI_FLASH_2_GPIO_CS_PIN));
		sys_write32(1 << (CONFIG_SPI_FLASH_2_GPIO_CS_PIN % 32), GPION_BSR(CONFIG_SPI_FLASH_2_GPIO_CS_PIN));
    }
    return 0;
}
#else
#define spi_flash_2_pm_control 	NULL
#endif

static int spi_flash_2_acts_init(const struct device *dev)
{
	struct spinor_info *sni = DEV_DATA(dev);
	uint8_t status, status2, status3;
	unsigned char cid;
	printk("spi3 flash init\n");

	/* enable spi3 controller clock */
	acts_clock_peripheral_enable(CLOCK_ID_SPI3);

	/* reset spi3 controller */
	acts_reset_peripheral(RESET_ID_SPI3);

	/* setup SPI3 clock rate */
	clk_set_rate(CLOCK_ID_SPI3, MHZ(CONFIG_SPI_FLASH_2_FREQ_MHZ));

	spi_flash_2_pwoer(sni, true);
	sni->chipid = p_spinor_api->read_chipid(sni);
	cid =  (sni->chipid & 0xff0000)>>16;
	printk("read spi3 nor chipid:0x%x, cid=%d\n", sni->chipid, cid);

	status = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS);
	status2 = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);
	status3 = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS3);

	printk("spi3 nor status: {0x%02x 0x%02x 0x%02x}\n", status, status2, status3);
	
	if(cid > 24){ //capacity = 2^cid (byte)
		p_spinor_api->set_addr_mode(sni, 4); // set 4byte mode
	}

	if(sni->spi.bus_width == 4) {
		printk("data nor is 4 line mode\n");
		sni->spi.flag |= SPI_FLAG_SPI_4XIO;
		/* check QE bit */
		if (!(status2 & 0x2)) {
			/* set QE bit to disable HOLD/WP pin function, for WinBond */
			status2 |= 0x2;
			p_spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS2,
					(u8_t *)&status2, 1);
		}
	} else if(sni->spi.bus_width == 2) {
		printk("data nor is 2 line mode\n");
	} else {
		sni->spi.bus_width = 1;
		printk("data nor is 1 line mode\n");
	}

	/* check delay chain workable */
	sni->chipid = p_spinor_api->read_chipid(sni);

	printk("read again spi3 nor chipid:0x%x\n", sni->chipid);


	status = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS);
	status2 = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);
	status3 = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS3);

	printk("spi3 nor status: {0x%02x 0x%02x 0x%02x}\n", status, status2, status3);

#if IS_ENABLED(CONFIG_SPINOR_TEST_DELAYCHAIN)
	//nor_test_delaychain(dev);
#endif

	return 0;
}



static struct flash_driver_api spi_flash_2_nor_api = {
	.read = spi_flash_2_acts_read,
	.write = spi_flash_2_acts_write,
	.erase = spi_flash_2_acts_erase,
	.get_parameters = spi_flash_get_parameters,
#if IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = spi_flash_acts_pages_layout,
#endif
};

static struct spinor_info spi_flash_2_acts_data = {
	.spi = {
		.base = SPI3_REG_BASE,
		.bus_width = CONFIG_SPI_FLASH_2_BUS_WIDTH,
		.delay_chain = CONFIG_SPI_FLASH_2_DELAY_CHAIN,
#if (CONFIG_DMA_SPINOR_RESEVER_CHAN < CONFIG_DMA_0_PCHAN_NUM)
		.dma_base= (DMA_REG_BASE + 0x100 + (CONFIG_DMA_SPINOR_RESEVER_CHAN * 0x100)),
#endif
		.flag = SPI_FLAG_NO_IRQ_LOCK,

	},
	.flag = 0,
};

#if 0
void spinor3_set(int bus_width, int use_dma, int nxio)
{
	spi_flash_2_acts_data.spi.bus_width = bus_width;

	if(use_dma)
		spi_flash_2_acts_data.spi.dma_base = (DMA_REG_BASE + 0x100 + (CONFIG_DMA_SPINOR_RESEVER_CHAN * 0x100));
	else
		spi_flash_2_acts_data.spi.dma_base = 0;
	if(nxio){
		spi_flash_2_acts_data.spi.flag |= SPI_FLAG_SPI_NXIO | SPI_FLAG_WR_4IO;
	}else{
		spi_flash_2_acts_data.spi.flag &= ~(SPI_FLAG_SPI_NXIO | SPI_FLAG_WR_4IO);
		
	}	
}
void spinor03_set_dt(int spi, int delaytran)
{
	if(spi == 0)
		spi_flash_acts_data.spi.delay_chain = delaytran;
	else
		spi_flash_2_acts_data.spi.delay_chain = delaytran;
}
#endif

static const struct spi_flash_acts_config spi_flash_2_acts_config = {
#if IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT)
	.pages_layout = {
			.pages_count = CONFIG_SPI_FLASH_2_CHIP_SIZE/0x1000,
			.pages_size  = 0x1000,
		},
#endif
	.chip_size	 = CONFIG_SPI_FLASH_2_CHIP_SIZE,
	.page_size	 = 0x1000,
};

DEVICE_DEFINE(spi_flash_2_acts, CONFIG_SPI_FLASH_2_NAME, &spi_flash_2_acts_init, spi_flash_2_pm_control,
		    &spi_flash_2_acts_data, &spi_flash_2_acts_config, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_OBJECTS, &spi_flash_2_nor_api);
#endif



#ifdef CONFIG_NOR_SECURIYT_SUSPPORT



#define SPINOR_CMD_SECURITY_ERASE   0x44	/* erase security registers cmd*/
#define SPINOR_CMD_SECURITY_PROGRAM 0x42	/* program security registers cmd*/
#define SPINOR_CMD_SECURITY_READ    0x48	/* read security registers cmd*/
#define SPINOR_CMD_UID_READ   		0x4B	/* Read Unique ID cmd*/

__ramfunc static int spinor_erase_security(struct spinor_info *sni, unsigned int addr)
{
    u32_t key;
    u8_t addr_mode;
    u8_t cid = (sni->chipid & 0xff0000) >> 16;
    if (cid > 24)
        addr_mode = 4;
    else
        addr_mode = 3;
    key = irq_lock(); //ota diff upgrade, must be irq lock
	p_spinor_api->write_cmd(sni, SPIMEM_CMD_ENABLE_WRITE);
    p_spinor_api->transfer(sni, SPINOR_CMD_SECURITY_ERASE, addr, addr_mode, 0, 0, 0, SPIMEM_TFLAG_WRITE_DATA);
    spinor_wait_ready(sni);
    irq_unlock(key);
    return 0;
}

__ramfunc static int spinor_write_security(struct spinor_info *sni, unsigned int addr, const void *data, int len)
{
    u32_t key;
    u8_t addr_mode;
    u8_t cid = (sni->chipid & 0xff0000) >> 16;
    if (cid > 24)
        addr_mode = 4;
    else
        addr_mode = 3;
    key = irq_lock();
    p_spinor_api->write_cmd(sni, SPIMEM_CMD_ENABLE_WRITE);
    p_spinor_api->transfer(sni, SPINOR_CMD_SECURITY_PROGRAM, addr, addr_mode, (u8_t *)data, len, 0, SPIMEM_TFLAG_WRITE_DATA);
    spinor_wait_ready(sni);
    irq_unlock(key);
    return 0;
}
__ramfunc static int spinor_read_security(struct spinor_info *sni, unsigned int addr,  void *data, int len)
{
    u32_t key;
    u8_t addr_mode;
    u8_t cid = (sni->chipid & 0xff0000) >> 16;
    if (cid > 24)
        addr_mode = 4;
    else
        addr_mode = 3;
    key = irq_lock();
    p_spinor_api->transfer(sni, SPINOR_CMD_SECURITY_READ, addr, addr_mode, (u8_t *)data, len, 1, 0);
    irq_unlock(key);
    return 0;
}

__ramfunc static int spinor_read_uid(struct spinor_info *sni,  void *data, int len)
{
    u32_t key;
    u8_t addr_mode;
    u8_t cid = (sni->chipid & 0xff0000) >> 16;
    if (cid > 24)
        addr_mode = 4;
    else
        addr_mode = 3;
    key = irq_lock();
    p_spinor_api->transfer(sni, SPINOR_CMD_UID_READ, 0, addr_mode, (u8_t *)data, len, 1, 0);
    irq_unlock(key);
    return 0;
}

#define NOR_SE_PAGE_SIZE  				256
#define NOR_SE_PAGE_MASK 				(NOR_SE_PAGE_SIZE-1)
#define NOR_SE_MAX_SIZE_EACH_REGN	  	1024   /**/

/*security_regn 0-3*/
int spi_flash_security_erase(const struct device *dev, unsigned int security_regn)
{
    struct spinor_info *sni = DEV_DATA(dev);
	return spinor_erase_security(sni, security_regn<<12);
}

int spi_flash_security_write(const struct device *dev, unsigned int security_regn, unsigned int offset, void *data, unsigned int len)
{
	unsigned int wlen, unlen;
    struct spinor_info *sni = DEV_DATA(dev);
	if(offset + len >= NOR_SE_MAX_SIZE_EACH_REGN)
		return -1;
	unlen = offset & NOR_SE_PAGE_MASK;
	while(len){
		if(unlen){
			wlen = NOR_SE_PAGE_SIZE - unlen;
			if(wlen > len)
				wlen = len;
			unlen = 0;
		}else{
			if(len < NOR_SE_PAGE_SIZE)
				wlen = len;
			else
				wlen = NOR_SE_PAGE_SIZE;
		}
		spinor_write_security(sni, (security_regn<<12)|offset, data, wlen);
		data = (unsigned char *)data + wlen;
        len -= wlen;
        offset += wlen;
	}
	return 0;
}

int spi_flash_security_read(const struct device *dev, unsigned int security_regn, unsigned int offset, void *data, unsigned int len)
{
    struct spinor_info *sni = DEV_DATA(dev);
	if(offset + len >= NOR_SE_MAX_SIZE_EACH_REGN)
		return -1;

	return spinor_read_security(sni, (security_regn<<12)|offset, data, len);
}

int spi_flash_uid_read(const struct device *dev, void *uid, unsigned int len)
{
	 struct spinor_info *sni = DEV_DATA(dev);
	 return spinor_read_uid(sni, uid, len);
}


#include <string.h>
static unsigned int g_tmp_buf[256];
void spinor_test_uid_securty(const struct device *dev)
{
	unsigned int  start_ms,end_ms, i, k;
	unsigned int *pb = g_tmp_buf;
	spi_flash_uid_read(dev, pb, 16);
	printk("uid=0x%x, 0x%x, 0x%x, 0x%x\n", pb[0], pb[1], pb[2], pb[3]);
	for(i = 1; i < 4; i++){// test security1-security3
		start_ms = k_cycle_get_32();
		spi_flash_security_erase(dev, i);
		end_ms = k_cyc_to_ms_near32(k_cycle_get_32() -start_ms) ;
		printk("scurity erase %d use=%d ms\n", i, end_ms);
		spi_flash_security_read(dev, i, 200,  pb, NOR_SE_PAGE_SIZE);
		for(k = 0; k < NOR_SE_PAGE_SIZE/4; k++) {// check erase ok
			if(pb[k] != 0xffffffff){
				printk("erase check fail %d : off=0x%x, 0x%x!=0xffffffff\n", i, k*4, pb[k]);
				break;
			}
		}
		for(k = 0; k < NOR_SE_PAGE_SIZE/4; k++) {
			pb[k] = k + 0x12345600*i;
		}
		start_ms = k_cycle_get_32();
		spi_flash_security_write(dev, i, 200, pb, NOR_SE_PAGE_SIZE);
		end_ms = k_cyc_to_ms_near32(k_cycle_get_32() -start_ms) ;
		printk("scurity write 1KB %d use=%d ms\n", i, end_ms);
	}
	for(i = 1; i < 4; i++){
		memset(pb, 0, NOR_SE_PAGE_SIZE);
		spi_flash_security_read(dev, i, 200,  pb, NOR_SE_PAGE_SIZE);
		for(k = 0; k < NOR_SE_PAGE_SIZE/4; k++){
			if(pb[k] != k + 0x12345600*i){
				printk("scurity read cmp fail:%d,off=0x%x,0x%x!=0x%x\n",i, k*4, pb[k],  k + 0x12345600*i);
				break;
			}
		}
	}
	printk("secutrity test finished\n");
}

#endif


